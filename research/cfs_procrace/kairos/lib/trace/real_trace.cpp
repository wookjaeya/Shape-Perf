// real_trace.cpp -- the real-record trace of a run container (see real_trace.hpp).

#include "kz/trace/real_trace.hpp"

#include "kz/core/registry.hpp"
#include "kz/trace/decode_sites.hpp"

#include <algorithm>
#include <cstdio>
#include <cctype>
#include <map>
#include <set>

namespace kz::trace {

namespace {

enum class MapKind { None, Call, Return, Point };

struct SiteMap {
  MapKind kind = MapKind::None;
  int32_t op = -1;
  std::string name;  // the operation, or the point
};

uint64_t first_at_or_after(const std::vector<kz_raw_ev>& ev, uint64_t t) {
  for (size_t i = 0; i < ev.size(); ++i)
    if (ev[i].t_ns >= t) return i;
  return ev.size();
}

uint64_t first_after(const std::vector<kz_raw_ev>& ev, uint64_t t) {
  for (size_t i = 0; i < ev.size(); ++i)
    if (ev[i].t_ns > t) return i;
  return ev.size();
}

}  // namespace

bool owner_record_key(const phi::Profile& p, const std::map<std::string, int64_t>& consts, uint32_t id, int64_t* key) {
  if (!p.context.present || p.context.owner_kind_index < 0) return false;
  const phi::KindDecl& k = p.kinds[static_cast<size_t>(p.context.owner_kind_index)];
  if (!k.ids.present || k.params.size() != 1 || k.table.empty()) return false;
  const int32_t ci = p.find_const(k.table);
  if (ci < 0) return false;
  const phi::ConstDecl& c = p.consts[static_cast<size_t>(ci)];
  int64_t n = -1;
  if (c.value == phi::ConstDecl::Value::Int) {
    n = c.ival;
  } else if (c.value == phi::ConstDecl::Value::RefEval) {
    const auto it = consts.find(c.name);
    if (it != consts.end()) n = it->second;
  }
  if (n <= 0) return false;
  // valid(x): (x - base) mod 2^32 <= serial_max; the record is (x - base) mod 2^32 modulo the table size.
  const uint32_t serial = id - static_cast<uint32_t>(k.ids.base);
  if (static_cast<int64_t>(serial) > k.ids.serial_max) return false;
  *key = static_cast<int64_t>(serial % static_cast<uint64_t>(n));
  return true;
}

std::string role_key(const std::string& name) {
  std::string out;
  for (char c : name)
    if (c != '_') out += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

Status real_trace_from_run(const Container& run, const phi::Profile& p, eval::Trace* out, RealTraceReport* report) {
  return real_trace_from_run(run, p, RealTraceOptions(), out, report);
}

Status real_trace_from_run(const Container& run, const phi::Profile& p, const RealTraceOptions& opt, eval::Trace* out, RealTraceReport* report) {
  *out = eval::Trace();
  *report = RealTraceReport();
  out->record = eval::RecordKind::Real;
  if (!p.checked) return Status::error(Err::Invalid, "the profile has not been checked");
  if (run.header().artifact_kind != KZ_ARTIFACT_RUN) return Status::error(Err::Invalid, "not a run container");

  std::vector<kz_raw_ev> events;
  std::vector<kz_site> sites;
  std::vector<kz_site_slot> slots;
  std::vector<kz_thread> threads;
  std::vector<kz_map_ev> maps;
  std::vector<kz_runmeta> meta;
  std::vector<kz_loss> losses;
  std::vector<kz_app_owner> owners;
  Status s = run.read_section(KZ_SEC_RAW_EVENTS, &events);
  if (s.ok()) s = run.read_section(KZ_SEC_SITES, &sites);
  if (s.ok()) s = run.read_section(KZ_SEC_SITE_SLOTS, &slots);
  if (s.ok()) s = run.read_section(KZ_SEC_THREADS, &threads);
  if (s.ok()) s = run.read_section(KZ_SEC_MAPS, &maps);
  if (s.ok()) s = run.read_section(KZ_SEC_RUNMETA, &meta);
  if (s.ok()) s = run.read_section(KZ_SEC_LOSS, &losses);
  if (s.ok()) s = run.read_section(KZ_SEC_ACTOR_OWNERS, &owners);
  std::vector<kz_ev_str> strings;  // the texts of string slots (containers with fetch slots only)
  if (s.ok() && run.find_section(KZ_SEC_EVENT_STRINGS) != nullptr) s = run.read_section(KZ_SEC_EVENT_STRINGS, &strings);
  if (!s.ok()) return s;
  std::map<std::pair<uint32_t, uint8_t>, std::string> text_of;
  for (const kz_ev_str& e : strings) {
    std::string t;
    if (!run.string(e.text, &t)) return Status::error(Err::BadRef, "EVENT_STRINGS: a string does not resolve");
    text_of[{e.event, e.slot}] = t;
  }
  report->raw_events = events.size();
  report->threads = threads.size();
  report->owner_section = run.find_section(KZ_SEC_ACTOR_OWNERS) != nullptr;
  report->runmeta = !meta.empty();

  // ---- the point names of the profile, with the values each point carries ----
  std::map<std::string, std::map<std::string, std::string>> point_names;  // point -> role key -> value name
  for (const phi::OpDecl& op : p.ops) {
    auto add = [&](const std::string& label, const phi::LpSpec& lp) {
      if (lp.kind == phi::LpKind::None) return;
      const std::string name = eval::lp_name(op, label, lp);
      std::map<std::string, std::string>& m = point_names[name];
      for (const phi::OpParam& v : lp.values) m[role_key(v.name)] = "lp." + v.name;
      for (const phi::OpParam& prm : op.params)
        if (prm.role != "out" && m.count(role_key(prm.name)) == 0) m[role_key(prm.name)] = "arg." + prm.name;
    };
    for (const phi::ConjunctDecl& c : op.conjuncts) add(c.label, c.lp);
    for (const phi::EffectDecl& e : op.effects) add(e.label, e.lp);
    if (op.terminal.present && op.terminal.process_terminal && op.terminal.after.present) point_names[eval::terminal_name(op)];
  }
  for (const phi::SiteDecl& sd : p.sites) point_names[sd.name];
  std::set<std::string> capture_points;
  for (const phi::OpDecl& op : p.ops)
    for (const phi::ConjunctDecl& c : op.conjuncts)
      if (c.lp.kind == phi::LpKind::Capture) capture_points.insert(eval::lp_name(op, c.label, c.lp));

  // ---- the sites ----
  std::vector<SiteMap> site_map(sites.size());
  std::vector<std::string> labels(sites.size());
  for (size_t i = 0; i < sites.size(); ++i) {
    if (sites[i].site != i) return Status::error(Err::Invalid, "the site table is not indexed by site id");
    std::string symbol;
    run.string(sites[i].symbol, &symbol);
    run.string(sites[i].label, &labels[i]);
    SiteMap& m = site_map[i];
    if (sites[i].kind == KZ_SITE_KIND_ENTRY_PROBE || sites[i].kind == KZ_SITE_KIND_RET_PROBE) {
      for (size_t oi = 0; oi < p.ops.size(); ++oi)
        if (!p.ops[oi].symbol.empty() && p.ops[oi].symbol == symbol) {
          m.kind = sites[i].kind == KZ_SITE_KIND_ENTRY_PROBE ? MapKind::Call : MapKind::Return;
          m.op = static_cast<int32_t>(oi);
          m.name = p.ops[oi].name;
        }
    } else if (sites[i].kind == KZ_SITE_KIND_POINT_PROBE && point_names.count(labels[i]) != 0) {
      m.kind = MapKind::Point;
      m.name = labels[i];
      out->observed_points.insert(labels[i]);
    } else if (sites[i].kind == KZ_SITE_KIND_POINT_PROBE && labels[i].find('@') != std::string::npos &&
               capture_points.count(labels[i].substr(0, labels[i].find('@'))) != 0) {
      // one of several call sites of a capture point (header, "capture sites")
      m.kind = MapKind::Point;
      m.name = labels[i].substr(0, labels[i].find('@'));
      out->observed_points.insert(m.name);
    }
    report->site_map.push_back(SiteMapEntry{static_cast<uint32_t>(i), static_cast<uint8_t>(m.kind == MapKind::Call ? 1 : m.kind == MapKind::Return ? 2 : m.kind == MapKind::Point ? 3 : 0), m.name});
    switch (m.kind) {
      case MapKind::Call: report->sites.push_back(labels[i] + ": call of " + m.name); break;
      case MapKind::Return: report->sites.push_back(labels[i] + ": return of " + m.name); break;
      case MapKind::Point: report->sites.push_back(labels[i] + ": point " + m.name); break;
      case MapKind::None: report->sites.push_back(labels[i] + ": not mapped"); break;
    }
  }
  std::map<std::pair<uint32_t, uint8_t>, std::string> role_of;
  std::set<std::pair<uint32_t, uint8_t>> string_slot;
  for (const kz_site_slot& sl : slots) {
    std::string r;
    run.string(sl.role, &r);
    role_of[{sl.site, sl.slot}] = r;
    if (sl.src == KZ_SLOT_SRC_STR) string_slot.insert({sl.site, sl.slot});
  }
  // A point site that names no point of the profile but whose slot role names an `out` parameter of an
  // operation: inside a call of that operation it gives the value the call stores through the parameter
  // (out.NAME on that call's return; header, "out values").
  for (size_t i = 0; i < sites.size(); ++i) {
    if (site_map[i].kind != MapKind::None || sites[i].kind != KZ_SITE_KIND_POINT_PROBE) continue;
    for (uint8_t k = 0; k < 4; ++k) {
      const auto r = role_of.find({static_cast<uint32_t>(i), k});
      if (r == role_of.end()) continue;
      std::string ops, param;
      for (const phi::OpDecl& op : p.ops)
        for (const phi::OpParam& prm : op.params)
          if (prm.role == "out" && role_key(prm.name) == role_key(r->second)) {
            ops += (ops.empty() ? "" : ", ") + op.name;
            param = prm.name;
          }
      if (!ops.empty()) report->sites[i] = labels[i] + ": out value " + param + " of the actor's innermost open call (if it is one of " + ops + ")";
    }
  }

  // ---- the actors: one per thread incarnation ----
  std::vector<std::string> thread_names(threads.size());
  std::map<std::string, int> base_count;
  auto base_name = [&](size_t i) {
    std::string n;
    run.string(threads[i].name, &n);
    return n.empty() ? "tid" + std::to_string(threads[i].tid) : n;
  };
  for (size_t i = 0; i < threads.size(); ++i) ++base_count[base_name(i)];
  for (size_t i = 0; i < threads.size(); ++i) {
    std::string n = base_name(i);
    if (base_count[n] > 1 && n.rfind("tid", 0) != 0) n += ".tid" + std::to_string(threads[i].tid);
    if (threads[i].generation != 0) n += ".g" + std::to_string(threads[i].generation);
    thread_names[i] = n;
  }
  // the incarnation of `tid` alive at time t: the latest generation forked no later than t
  auto incarnation = [&](uint32_t tid, uint64_t t) -> int64_t {
    int64_t best = -1;
    for (size_t i = 0; i < threads.size(); ++i) {
      if (threads[i].tid != tid) continue;
      if (best < 0 || (threads[i].t_fork <= t && threads[i].generation >= threads[static_cast<size_t>(best)].generation)) best = static_cast<int64_t>(i);
    }
    return best;
  };
  auto actor_of = [&](uint32_t tid, uint64_t t) {
    const int64_t i = incarnation(tid, t);
    return i >= 0 ? thread_names[static_cast<size_t>(i)] : "tid" + std::to_string(tid);
  };
  for (size_t i = 0; i < threads.size(); ++i) {
    if ((threads[i].flags & KZ_THREAD_FLAG_FORK_SEEN) == 0 || threads[i].ptid == KZ_NONE32) continue;
    out->creators[thread_names[i]] = actor_of(threads[i].ptid, threads[i].t_fork);
  }
  report->identities.resize(threads.size());
  for (size_t i = 0; i < threads.size(); ++i) {
    ThreadIdentity& ti = report->identities[i];
    ti.actor = thread_names[i];
    ti.tid = threads[i].tid;
    ti.generation = threads[i].generation;
    ti.thread_flags = threads[i].flags;
    const auto cr = out->creators.find(thread_names[i]);
    if (cr != out->creators.end()) ti.creator = cr->second;
  }
  // the slot of a site that holds the return address (role return_address), or -1
  auto return_slot = [&](uint32_t site) {
    for (uint8_t k = 0; k < 4; ++k) {
      const auto r = role_of.find({site, k});
      if (r != role_of.end() && r->second == kRoleReturnAddress) return static_cast<int>(k);
    }
    return -1;
  };
  // Core status of owners (header, "core"): per owner identity, from its OWNER_REQUESTs' calling functions.
  report->core_evidence = opt.symbols != nullptr && !opt.core_request_callers.empty();
  std::map<uint32_t, int> core_vote;  // owner identity -> bit 1: a request from a core caller, bit 2: from another function
  if (report->core_evidence) {
    for (const kz_app_owner& o : owners) {
      if (o.thread >= threads.size() || o.level == KZ_OWNER_LEVEL_UNDETERMINED || o.owner_key == KZ_NONE32 || o.creation_event >= events.size()) continue;
      const kz_raw_ev& rq = events[o.creation_event];
      if (rq.site >= sites.size() || sites[rq.site].decoder != KZ_DECODER_OWNER_REQUEST || (rq.flags & KZ_RAW_FLAG_TRUNC) != 0) continue;
      const int slot = return_slot(rq.site);
      uint64_t link = 0;
      if (slot < 0 || !link_address(maps, threads[o.thread].pid, rq.a[slot], &link)) continue;
      const SymbolEntry* sym = opt.symbols->find(link);
      if (sym == nullptr) continue;
      report->identities[o.thread].request_caller = sym->name;
      core_vote[o.owner_key] |= opt.core_request_callers.count(sym->name) != 0 ? 1 : 2;
    }
  }
  for (const kz_app_owner& o : owners) {
    if (o.thread >= threads.size()) continue;
    const std::string& a = thread_names[o.thread];
    ThreadIdentity& ti = report->identities[o.thread];
    ti.level = o.level;
    ti.evidence = o.evidence;
    ti.owner_id = o.owner_key;
    run.string(o.app_name, &ti.app_name);
    if (o.level == KZ_OWNER_LEVEL_CONFIRMED) ++report->owners_confirmed;
    else if (o.level == KZ_OWNER_LEVEL_INFERRED) ++report->owners_inferred;
    if (o.level == KZ_OWNER_LEVEL_UNDETERMINED || o.owner_key == KZ_NONE32) continue;
    int64_t key = 0;
    if (!owner_record_key(p, opt.consts, o.owner_key, &key)) {
      ++report->owners_unresolved;  // kept undetermined for the replay: never "no owner"
      continue;
    }
    ti.key_resolved = true;
    ti.key = key;
    out->owner_levels[a] = o.level;
    out->owners[a] = {key};
    out->owner_ids[a] = o.owner_key;
    ++report->owners_determined;
    const auto cv = core_vote.find(o.owner_key);
    if (cv != core_vote.end() && (cv->second == 1 || cv->second == 2)) {
      ti.core = cv->second == 1 ? 1 : 0;
      out->owner_core[a] = cv->second == 1;
      ++(cv->second == 1 ? report->owners_core : report->owners_not_core);
    }
  }
  for (const kz_app_owner& o : owners)
    if (o.thread < threads.size()) report->identities[o.thread].creator_thread = o.creator_thread;

  // ---- spawn out-parameters bound by the id (header, "spawn out") ----
  std::map<uint32_t, uint32_t> assigned_by_create;  // RAW_EVENTS index of a spawn-create event -> the id
  for (const kz_thread& t : threads)
    if ((t.flags & KZ_THREAD_FLAG_ID_MATCHED) != 0 && (t.flags & KZ_THREAD_FLAG_SPAWN_MATCHED) != 0 && t.creator_event != KZ_NONE32)
      assigned_by_create[t.creator_event] = t.announced_id;
  std::map<uint64_t, uint32_t> bound_at_return;  // RAW_EVENTS index of a spawn-return event -> the id
  {
    std::map<uint32_t, std::vector<std::pair<uint64_t, uint64_t>>> open;  // tid -> (create index, return address)
    for (size_t i = 0; i < events.size(); ++i) {
      const kz_raw_ev& e = events[i];
      if (e.site >= sites.size() || (e.flags & KZ_RAW_FLAG_TRUNC) != 0) continue;
      if (sites[e.site].decoder == KZ_DECODER_SPAWN_CREATE) {
        open[e.tid].push_back({i, e.a[kSlotReturnAddress]});
      } else if (sites[e.site].decoder == KZ_DECODER_SPAWN_RETURN) {
        std::vector<std::pair<uint64_t, uint64_t>>& st = open[e.tid];
        size_t k = st.size();
        while (k > 0 && st[k - 1].second != e.a[kSlotReturnAddress]) --k;
        if (k == 0) continue;
        const auto as = assigned_by_create.find(static_cast<uint32_t>(st[k - 1].first));
        if (as != assigned_by_create.end()) bound_at_return[i] = as->second;
        st.resize(k - 1);
      }
    }
  }
  report->owners_undetermined = threads.size() - report->owners_confirmed - report->owners_inferred;

  // ---- the events ----
  // the open calls of each actor in the trace being built (operation index), with the out values that
  // points inside them gave
  struct OpenOp {
    int32_t op;
    std::map<std::string, int64_t> outs;
  };
  std::map<std::string, std::vector<OpenOp>> open_ops;
  for (size_t ei = 0; ei < events.size(); ++ei) {
    const kz_raw_ev& e = events[ei];
    if (e.site >= site_map.size() || site_map[e.site].kind == MapKind::None) {
      ++report->unmapped_events;
      // an out value from a point inside the actor's innermost open call
      if (e.site < sites.size() && sites[e.site].kind == KZ_SITE_KIND_POINT_PROBE && (e.flags & KZ_RAW_FLAG_TRUNC) == 0) {
        std::vector<OpenOp>& st = open_ops[actor_of(e.tid, e.t_ns)];
        if (!st.empty()) {
          const phi::OpDecl& op = p.ops[static_cast<size_t>(st.back().op)];
          for (uint8_t k = 0; k < 4; ++k) {
            const auto r = role_of.find({e.site, k});
            if (r == role_of.end() || string_slot.count({e.site, k}) != 0) continue;
            for (const phi::OpParam& prm : op.params)
              if (prm.role == "out" && role_key(prm.name) == role_key(r->second)) {
                st.back().outs["out." + prm.name] = static_cast<int64_t>(e.a[k]);
                ++report->out_values_from_points;
              }
          }
        }
      }
      continue;
    }
    const SiteMap& m = site_map[e.site];
    eval::TraceEvent te;
    te.actor = actor_of(e.tid, e.t_ns);
    te.site = m.name;
    te.phase = m.kind == MapKind::Call ? eval::Phase::Call : m.kind == MapKind::Return ? eval::Phase::Return : eval::Phase::Point;
    const bool unreadable = (e.flags & KZ_RAW_FLAG_TRUNC) != 0;
    if (unreadable) ++report->unreadable_events;
    for (uint8_t k = 0; k < 4 && !unreadable; ++k) {
      const auto r = role_of.find({e.site, k});
      if (r == role_of.end()) continue;
      const std::string key = role_key(r->second);
      const int64_t v = static_cast<int64_t>(e.a[k]);
      // a string slot gives a text (a Str parameter or value), never its length as a number
      const bool is_text = string_slot.count({e.site, k}) != 0;
      const auto tx = text_of.find({static_cast<uint32_t>(ei), k});
      if (is_text && tx == text_of.end()) continue;  // the kernel gave no string: the value stays unknown
      if (is_text) ++report->string_values;
      if (m.kind == MapKind::Point) {
        if (key == "value") {
          if (!is_text) te.values["value"] = v;
          continue;
        }
        // `value(L)`: the value loaded from the location L (header, "cells")
        const std::string& raw = r->second;
        if (raw.size() > 7 && raw.compare(0, 6, "value(") == 0 && raw.back() == ')') {
          if (!is_text) {
            te.values["value"] = v;
            te.texts["cell"] = raw.substr(6, raw.size() - 7);
            ++report->cell_values;
          }
          continue;
        }
        const auto& names = point_names[m.name];
        const auto it = names.find(key);
        if (it != names.end()) {
          if (is_text) te.texts[it->second] = tx->second;
          else te.values[it->second] = v;
        }
        continue;
      }
      const phi::OpDecl& op = p.ops[static_cast<size_t>(m.op)];
      if (m.kind == MapKind::Return && (key == "status" || key == "result")) {
        te.has_result = true;
        te.result = v;
        continue;
      }
      for (const phi::OpParam& prm : op.params) {
        if (role_key(prm.name) != key) continue;
        const std::string name = (m.kind == MapKind::Call ? "arg." : "out.") + prm.name;
        if ((m.kind == MapKind::Call) != (prm.role != "out")) continue;
        if (is_text) te.texts[name] = tx->second;
        else te.values[name] = v;
      }
    }
    // the open calls of the actor (out values from points inside them go to their return)
    {
      std::vector<OpenOp>& st = open_ops[te.actor];
      if (m.kind == MapKind::Call) {
        st.push_back(OpenOp{m.op, {}});
      } else if (m.kind == MapKind::Return) {
        size_t k = st.size();
        while (k > 0 && st[k - 1].op != m.op) --k;
        if (k > 0) {
          if (!unreadable)
            for (const auto& ov : st[k - 1].outs)
              if (te.values.count(ov.first) == 0) te.values[ov.first] = ov.second;
          st.resize(k - 1);
        }
      }
    }
    if (m.kind == MapKind::Return) {
      const phi::OpDecl& op = p.ops[static_cast<size_t>(m.op)];
      for (const phi::HbRuleDecl& rule : p.rules) {
        if (rule.tmpl != phi::RuleTemplate::Spawn || rule.create_op != op.name) continue;
        bool has_out = false;
        for (const phi::OpParam& prm : op.params) has_out = has_out || (prm.role == "out" && prm.name == rule.create_out);
        if (!has_out) continue;
        ++report->spawn_returns;
        const auto b = bound_at_return.find(ei);
        if (b == bound_at_return.end() || unreadable) continue;
        te.values["out." + rule.create_out] = static_cast<int64_t>(b->second);
        ++report->spawn_outs_bound;
      }
    }
    out->add(std::move(te));
    ++report->events;
  }

  // ---- the start of the record ----
  if (!events.empty())
    for (const kz_map_ev& mp : maps)
      if (mp.t_ns <= events.front().t_ns) report->starts_at_exec = true;
  if (report->starts_at_exec) {
    out->start_evidence = KZ_START_EVIDENCE_INIT;
    for (const phi::KindDecl& k : p.kinds) {
      if (!k.derived.empty()) continue;
      eval::Adequacy a;
      a.start_known = true;
      out->kind_adequacy[k.name] = a;
    }
    for (const phi::LocDecl& l : p.locs) {
      eval::Adequacy a;
      a.start_known = true;
      out->loc_adequacy[l.name] = a;
    }
  }

  // ---- the static closure (header, "closure") ----
  if (opt.facts != nullptr) {
    std::vector<ObservedSite> obs;
    for (size_t i = 0; i < sites.size(); ++i) {
      if (site_map[i].kind != MapKind::Point) continue;
      ObservedSite o;
      o.point = site_map[i].name;
      run.string(sites[i].source, &o.file);
      o.line = sites[i].line;
      obs.push_back(std::move(o));
    }
    ClosureReport cr;
    s = apply_static_closure(*opt.facts, p, obs, out, &cr);
    if (!s.ok()) return s;
    report->closure_given = true;
    report->closure_lines = cr.lines;
  }

  // ---- losses ----
  std::vector<eval::LossInterval> lv;
  const bool flagged = !meta.empty() && (meta.front().flags & KZ_RUN_FLAG_LOSS_PRESENT) != 0;
  report->loss_present = flagged || !losses.empty() || meta.empty();
  for (const kz_loss& l : losses) {
    if (l.source != KZ_LOSS_SOURCE_PERF_THROTTLE) report->loss_cpus.push_back({l.cpu, l.count});
    eval::LossInterval li;
    if (l.source == KZ_LOSS_SOURCE_PERF_THROTTLE) {
      li.begin = first_at_or_after(events, l.t_ns);
      li.end = l.tseq_first == 1 ? eval::kNoEvent : first_after(events, l.t_ns + l.count);
    } else {
      li.begin = 0;
      li.end = first_after(events, l.t_ns);
    }
    lv.push_back(li);
  }
  if ((flagged && losses.empty()) || meta.empty()) lv.push_back(eval::LossInterval{0, eval::kNoEvent});
  // positions in the trace (only mapped events are kept): the first kept event at or after a raw position
  std::vector<uint64_t> kept_before(events.size() + 1, 0);
  {
    uint64_t n = 0;
    for (size_t i = 0; i < events.size(); ++i) {
      kept_before[i] = n;
      if (events[i].site < site_map.size() && site_map[events[i].site].kind != MapKind::None) ++n;
    }
    kept_before[events.size()] = n;
  }
  for (eval::LossInterval& li : lv) {
    li.begin = kept_before[std::min<uint64_t>(li.begin, events.size())];
    if (li.end != eval::kNoEvent) li.end = kept_before[std::min<uint64_t>(li.end, events.size())];
  }
  std::sort(lv.begin(), lv.end(), [](const eval::LossInterval& a, const eval::LossInterval& b) { return a.begin < b.begin; });
  for (const eval::LossInterval& li : lv) {
    if (!out->losses.empty() && (out->losses.back().end == eval::kNoEvent || out->losses.back().end >= li.begin)) {
      eval::LossInterval& last = out->losses.back();
      if (li.end == eval::kNoEvent || (last.end != eval::kNoEvent && li.end > last.end)) last.end = li.end;
      continue;
    }
    out->losses.push_back(li);
  }
  report->loss_intervals = out->losses.size();
  return Status::success();
}

std::string report_text(const RealTraceReport& r) {
  std::string out;
  out += "raw events " + std::to_string(r.raw_events) + ", trace events " + std::to_string(r.events) + ", events of unmapped sites " + std::to_string(r.unmapped_events) +
         ", events without slot values (TRUNC) " + std::to_string(r.unreadable_events) + "\n";
  for (const std::string& s : r.sites) out += "site " + s + "\n";
  out += "threads " + std::to_string(r.threads) + ", owners determined " + std::to_string(r.owners_determined) +
         (r.owner_section ? "" : " (the container has no ACTOR_OWNERS section: every owner is undetermined)") + "\n";
  if (r.owner_section) {
    out += "app_owner levels: confirmed " + std::to_string(r.owners_confirmed) + ", inferred " + std::to_string(r.owners_inferred) + ", undetermined " +
           std::to_string(r.owners_undetermined) + "; ids not mapped to a record key " + std::to_string(r.owners_unresolved) + "\n";
    auto flags_text = [](const char* en, uint32_t v) {
      const EnumDesc* d = find_enum(en);
      std::string t;
      if (d != nullptr)
        for (size_t i = 0; i < d->nitems; ++i)
          if ((v & static_cast<uint32_t>(d->items[i].value)) != 0) t += (t.empty() ? "" : ",") + std::string(d->items[i].display);
      return t.empty() ? std::string("-") : t;
    };
    static const char* const kLevel[] = {"undetermined", "inferred", "confirmed"};
    for (const ThreadIdentity& ti : r.identities) {
      char key[32];
      std::snprintf(key, sizeof key, "0x%x", ti.owner_id);
      out += "identity " + ti.actor + ": creator " + (ti.creator.empty() ? std::string("-") : ti.creator) + ", owner " + (ti.level <= 2 ? kLevel[ti.level] : "?") +
             (ti.owner_id == KZ_NONE32 ? std::string() : std::string(" id ") + key) + (ti.key_resolved ? " record " + std::to_string(ti.key) : std::string()) +
             (ti.app_name.empty() ? std::string() : " app '" + ti.app_name + "'") + ", evidence " + flags_text("KZ_OWNER_EVIDENCE", ti.evidence) +
             (ti.request_caller.empty() ? std::string() : ", requested from " + ti.request_caller) + (ti.core == 1 ? ", core service" : ti.core == 0 ? ", not a core service" : "") + "\n";
    }
  }
  out += "spawn returns " + std::to_string(r.spawn_returns) + ", out-parameter bound by the id " + std::to_string(r.spawn_outs_bound) + "\n";
  if (r.string_values != 0 || r.out_values_from_points != 0)
    out += "string values (fetch slots) " + std::to_string(r.string_values) + ", out values from points inside a call " + std::to_string(r.out_values_from_points) + "\n";
  out += std::string("core status of owners: ") +
         (r.core_evidence ? "core " + std::to_string(r.owners_core) + ", not core " + std::to_string(r.owners_not_core) + ", not established " +
                                std::to_string(r.threads - r.owners_core - r.owners_not_core)
                          : std::string("not established (no basis symbols or no core request caller given)")) +
         "\n";
  out += std::string("start ") + (r.starts_at_exec ? "at the first execution mapping of the traced file (INIT applies)" : "not shown to be the process start (start state unknown)") + "\n";
  out += std::string("loss ") + (r.loss_present ? "reported" : "none reported") + (r.runmeta ? "" : " (no RUNMETA: loss evidence absent, the record is treated as lossy)") +
         ", loss intervals " + std::to_string(r.loss_intervals) + "\n";
  for (const auto& lc : r.loss_cpus)
    out += "loss report: CPU ring " + std::to_string(lc.first) + ", " + std::to_string(lc.second) + " record(s); the task whose events were lost is not established\n";
  if (r.cell_values != 0) out += "values loaded from a named location (value(L) slots) " + std::to_string(r.cell_values) + "\n";
  if (r.closure_given) {
    for (const std::string& l : r.closure_lines) out += l + "\n";
    out += "lifetimes, order between actors: not established by a run container; writer and alias coverage: from the static closure above\n";
  } else {
    out += "writer and alias coverage, lifetimes, order between actors: not established by a run container (no static closure given)\n";
  }
  return out;
}

}  // namespace kz::trace
