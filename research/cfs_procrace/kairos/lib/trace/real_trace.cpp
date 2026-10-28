// real_trace.cpp -- the real-record trace of a run container, read through the neutral event layer
// (see real_trace.hpp and neutral.hpp).

#include "kz/trace/real_trace.hpp"

#include "kz/core/registry.hpp"
#include "kz/trace/decode_sites.hpp"
#include "kz/trace/neutral.hpp"

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
  // A call or return observed at the call site (an instrumented binding of a call; NSITE flag call-site) whose label
  // names a point of the profile is that point as well: F0 observes the two with two probes (the callee's entry or
  // return, and the point at the call), a call-site binding with one hook.  The point follows a return and precedes a call.
  std::string point_also;
};

// The value slots of a neutral event (kzrt: up to KZ_RT_MAX_VALS = 8; F0: a[0..3]).
constexpr uint8_t kMaxSlots = 8;

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
  NeutralRun n;
  bool from_sections = false;
  Status s = neutral_of_run(run, &n, &from_sections);
  if (!s.ok()) return s;
  s = real_trace_from_neutral(n, p, opt, out, report);
  report->neutral_sections = from_sections;
  return s;
}

Status real_trace_from_neutral(const NeutralRun& n, const phi::Profile& p, const RealTraceOptions& opt, eval::Trace* out, RealTraceReport* report) {
  *out = eval::Trace();
  *report = RealTraceReport();
  out->record = eval::RecordKind::Real;
  if (!p.checked) return Status::error(Err::Invalid, "the profile has not been checked");
  Status s = check_neutral(n);
  if (!s.ok()) return s;
  const std::vector<kz_nevent>& events = n.events;
  const std::vector<NSite>& sites = n.sites;
  const std::vector<NActor>& actors = n.actors;
  report->raw_events = events.size();
  report->owner_section = (n.run.r.flags & KZ_NRUN_FLAG_OWNERS_RECORDED) != 0;
  report->runmeta = (n.run.r.flags & KZ_NRUN_FLAG_LOSS_EVIDENCE) != 0;
  report->backend = n.run.r.backend;
  report->start = n.run.r.start_evidence;
  // the actors that the backend's identity records give (the report's threads, in their order)
  std::vector<uint32_t> joined;
  for (size_t i = 0; i < actors.size(); ++i)
    if ((actors[i].r.flags & KZ_NACTOR_FLAG_JOIN_RECORD) != 0) joined.push_back(static_cast<uint32_t>(i));
  report->threads = joined.size();

  // ---- the point names of the profile, with the values each point carries ----
  std::map<std::string, std::map<std::string, std::string>> point_names;  // point -> role key -> value name
  std::map<std::string, int32_t> point_op;  // a linearization point -> the operation whose conjunct or effect it is
  for (const phi::OpDecl& op : p.ops) {
    auto add = [&](const std::string& label, const phi::LpSpec& lp) {
      if (lp.kind == phi::LpKind::None) return;
      const std::string name = eval::lp_name(op, label, lp);
      point_op.emplace(name, static_cast<int32_t>(&op - p.ops.data()));
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
  auto profile_point = [&](const std::string& label) -> std::string {
    if (point_names.count(label) != 0) return label;
    const size_t at = label.find('@');
    if (at != std::string::npos && capture_points.count(label.substr(0, at)) != 0) return label.substr(0, at);  // header, "capture sites"
    return std::string();
  };
  std::vector<SiteMap> site_map(sites.size());
  for (size_t i = 0; i < sites.size(); ++i) {
    const NSite& ns = sites[i];
    const std::string& label = ns.label;
    const bool attached = (ns.r.flags & KZ_NSITE_FLAG_ATTACHED) != 0;
    SiteMap& m = site_map[i];
    if (ns.r.kind == KZ_NSITE_KIND_CALL || ns.r.kind == KZ_NSITE_KIND_RETURN) {
      for (size_t oi = 0; oi < p.ops.size(); ++oi)
        if (!p.ops[oi].symbol.empty() && p.ops[oi].symbol == ns.symbol) {
          m.kind = ns.r.kind == KZ_NSITE_KIND_CALL ? MapKind::Call : MapKind::Return;
          m.op = static_cast<int32_t>(oi);
          m.name = p.ops[oi].name;
        }
      if ((ns.r.flags & KZ_NSITE_FLAG_CALL_SITE) != 0) {
        const std::string pt = profile_point(label);
        if (!pt.empty() && m.kind == MapKind::None) {
          m.kind = MapKind::Point;
          m.name = pt;
        } else if (!pt.empty()) {
          m.point_also = pt;
        }
        if (!pt.empty() && attached) out->observed_points.insert(pt);
      }
    } else if (ns.r.kind == KZ_NSITE_KIND_POINT) {
      const std::string pt = profile_point(label);
      if (!pt.empty()) {
        m.kind = MapKind::Point;
        m.name = pt;
        if (attached) out->observed_points.insert(pt);
      }
    }
    // the probe list of a result names the backend's site record where there is one (F0 SITES), else the neutral site
    auto entry = [&](uint8_t kind, const std::string& name) {
      SiteMapEntry e{static_cast<uint32_t>(i), kind, name};
      e.section = KZ_SEC_NSITES;
      if (ns.r.raw.section == KZ_SEC_SITES) {
        e.section = KZ_SEC_SITES;
        e.site = ns.r.raw.index;
      }
      report->site_map.push_back(e);
    };
    entry(static_cast<uint8_t>(m.kind == MapKind::Call ? 1 : m.kind == MapKind::Return ? 2 : m.kind == MapKind::Point ? 3 : 0), m.name);
    if (!m.point_also.empty()) entry(3, m.point_also);
    const std::string also = m.point_also.empty() ? std::string() : " and point " + m.point_also + " (at the call site)";
    const std::string unattached = attached ? std::string() : " (not observed for the whole record: a binding is removed or unsupported)";
    switch (m.kind) {
      case MapKind::Call: report->sites.push_back(label + ": call of " + m.name + also + unattached); break;
      case MapKind::Return: report->sites.push_back(label + ": return of " + m.name + also + unattached); break;
      case MapKind::Point: report->sites.push_back(label + ": point " + m.name + unattached); break;
      case MapKind::None: report->sites.push_back(label + ": not mapped"); break;
    }
  }
  std::map<std::pair<uint32_t, uint8_t>, std::string> role_of;
  std::set<std::pair<uint32_t, uint8_t>> string_slot;
  // slots that hold the stored value of a store statement (the instrumented family's `from=operand0`, kzs-sites 2
  // place=store or a store through a parameter: the operand the original code stored, not a later read)
  std::set<std::pair<uint32_t, uint8_t>> store_operand;
  for (const NSlot& sl : n.slots) {
    if (sl.from == "operand0") store_operand.insert({sl.r.site, sl.r.slot});
    role_of[{sl.r.site, sl.r.slot}] = sl.role;
    if (sl.r.kind == KZ_NVAL_TEXT) string_slot.insert({sl.r.site, sl.r.slot});
  }
  // the observed values of an event, by slot
  auto value_of = [&](const kz_nevent& e, uint8_t slot) -> const NValue* {
    for (uint32_t k = 0; k < e.nvals; ++k)
      if (n.values[e.first_value + k].r.slot == slot) return &n.values[e.first_value + k];
    return nullptr;
  };
  // A point site that names no point of the profile but whose slot role names an `out` parameter of an
  // operation: inside a call of that operation it gives the value the call stores through the parameter
  // (out.NAME on that call's return; header, "out values").
  for (size_t i = 0; i < sites.size(); ++i) {
    if (site_map[i].kind != MapKind::None || sites[i].r.kind != KZ_NSITE_KIND_POINT) continue;
    for (uint8_t k = 0; k < kMaxSlots; ++k) {
      const auto r = role_of.find({static_cast<uint32_t>(i), k});
      if (r == role_of.end()) continue;
      std::string ops, param;
      for (const phi::OpDecl& op : p.ops)
        for (const phi::OpParam& prm : op.params)
          if (prm.role == "out" && role_key(prm.name) == role_key(r->second)) {
            ops += (ops.empty() ? "" : ", ") + op.name;
            param = prm.name;
          }
      if (!ops.empty()) report->sites[i] = sites[i].label + ": out value " + param + " of the actor's innermost open call (if it is one of " + ops + ")";
    }
  }

  // ---- the actors (neutral.hpp actor_names) ----
  const std::vector<std::string> names = actor_names(n);
  for (const NActor& a : actors)
    if (a.r.creator != KZ_NONE32) out->creators[names[&a - actors.data()]] = names[a.r.creator];
  report->identities.resize(joined.size());
  std::vector<int64_t> ident_of(actors.size(), -1);  // actor -> index in report->identities
  for (size_t j = 0; j < joined.size(); ++j) {
    const NActor& a = actors[joined[j]];
    ThreadIdentity& ti = report->identities[j];
    ident_of[joined[j]] = static_cast<int64_t>(j);
    ti.actor = names[joined[j]];
    ti.actor_index = joined[j];
    ti.binding = a.r.binding;
    ti.tid = static_cast<uint32_t>(a.r.native_key);
    ti.generation = a.r.generation;
    ti.thread_flags = a.r.join_flags;
    const auto cr = out->creators.find(ti.actor);
    if (cr != out->creators.end()) ti.creator = cr->second;
  }
  // Core status of owners (header, "core"): per owner identity, from its OWNER_REQUESTs' calling functions.
  report->core_evidence = opt.symbols != nullptr && !opt.core_request_callers.empty();
  std::map<uint32_t, int> core_vote;  // owner identity -> bit 1: a request from a core caller, bit 2: from another function
  if (report->core_evidence) {
    for (size_t i = 0; i < actors.size(); ++i) {
      const NActor& a = actors[i];
      if (ident_of[i] < 0 || (a.r.flags & KZ_NACTOR_FLAG_OWNER_RECORDED) == 0 || a.r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED || a.r.owner_id == KZ_NONE32) continue;
      std::string caller = a.request_func;  // named by the backend's binding, or resolved from the calling code's address
      if (caller.empty() && (a.r.flags & KZ_NACTOR_FLAG_REQUEST_ADDR) != 0) {
        const SymbolEntry* sym = opt.symbols->find(a.r.request_addr);
        if (sym != nullptr) caller = sym->name;
      }
      if (caller.empty()) continue;
      report->identities[static_cast<size_t>(ident_of[i])].request_caller = caller;
      core_vote[a.r.owner_id] |= opt.core_request_callers.count(caller) != 0 ? 1 : 2;
    }
  }
  for (size_t i = 0; i < actors.size(); ++i) {
    const NActor& a = actors[i];
    if (ident_of[i] < 0 || (a.r.flags & KZ_NACTOR_FLAG_OWNER_RECORDED) == 0) continue;
    const std::string& an = names[i];
    ThreadIdentity& ti = report->identities[static_cast<size_t>(ident_of[i])];
    ti.level = a.r.owner_level;
    ti.evidence = a.r.owner_evidence;
    ti.owner_id = a.r.owner_id;
    ti.app_name = a.owner_app;
    ti.creator_thread = a.r.owner_creator != KZ_NONE32 && ident_of[a.r.owner_creator] >= 0 ? static_cast<uint32_t>(ident_of[a.r.owner_creator]) : KZ_NONE32;
    if (a.r.owner_level == KZ_OWNER_LEVEL_CONFIRMED) ++report->owners_confirmed;
    else if (a.r.owner_level == KZ_OWNER_LEVEL_INFERRED) ++report->owners_inferred;
    if (a.r.owner_level == KZ_OWNER_LEVEL_UNDETERMINED || a.r.owner_id == KZ_NONE32) continue;
    int64_t key = 0;
    if (!owner_record_key(p, opt.consts, a.r.owner_id, &key)) {
      ++report->owners_unresolved;  // kept undetermined for the replay: never "no owner"
      continue;
    }
    ti.key_resolved = true;
    ti.key = key;
    out->owner_levels[an] = a.r.owner_level;
    out->owners[an] = {key};
    out->owner_ids[an] = a.r.owner_id;
    ++report->owners_determined;
    const auto cv = core_vote.find(a.r.owner_id);
    if (cv != core_vote.end() && (cv->second == 1 || cv->second == 2)) {
      ti.core = cv->second == 1 ? 1 : 0;
      out->owner_core[an] = cv->second == 1;
      ++(cv->second == 1 ? report->owners_core : report->owners_not_core);
    }
  }
  // The initial thread of the process (P10, 2026-10-25): where the profile's context declares `initial_thread not_app`
  // (its sources show that the thread that runs main is never a task of the framework, so never an application's task; SPEC 1.2
  // T_raw), the actor the backend establishes as the process's initial thread (F0: the thread whose id is the process id, the
  // thread-group leader: gettid(2) equals getpid(2) for it, seen as the first incarnation of that id) is stated not to be an
  // application's task.  A thread for which the record states a determined owner keeps it (the declaration and the record would
  // disagree: reported, not resolved).
  if (p.context.present && p.context.initial_thread_not_app) {
    std::string cites;
    for (const std::string& c : p.context.not_app_prov) cites += (cites.empty() ? "" : ", ") + c;
    for (uint32_t i : joined) {
      const NActor& a = actors[i];
      if ((a.r.flags & KZ_NACTOR_FLAG_INITIAL) == 0) continue;
      const std::string& an = names[i];
      if (out->owner_levels.count(an) != 0) {
        report->not_app_lines.push_back(an + ": the initial thread, but the record states a determined owner: kept as the record states it (not resolved)");
        continue;
      }
      // F0's thread-group leader, or an instrumented run's actor whose Linux TID the record shows equal to the process id
      const bool tid_pid = a.r.binding == KZ_NBIND_LINUX_TID || (a.r.native_kind == KZ_NKEY_LINUX_TID && a.r.process != KZ_NONE32 && a.r.native_key == a.r.process);
      const std::string why = tid_pid ? "thread id = process id" : "the backend's initial-thread record";
      out->not_app[an] = "the initial thread of process " + std::to_string(a.r.process) + " (" + why + "); profile context initial_thread not_app: " + cites;
      report->not_app_lines.push_back(an + ": " + out->not_app[an]);
    }
  }

  // ---- spawn out-parameters bound by the id (header, "spawn out") ----
  std::map<uint64_t, uint32_t> bound_at_return;  // NEVENTS index of the creating call's return -> the id
  for (const NActor& a : actors)
    if (a.r.spawn_return != KZ_NONE32) bound_at_return[a.r.spawn_return] = a.r.announced_id;
  report->owners_undetermined = report->threads - report->owners_confirmed - report->owners_inferred;

  // ---- the events ----
  // the open calls of each actor in the trace being built (operation index), with the out values that
  // points inside them gave
  struct OpenOp {
    int32_t op;
    std::map<std::string, int64_t> outs;
    uint64_t call = eval::kNoEvent;  // the call's trace event
    std::map<std::string, std::string> stored;  // outs that are the operand of a store inside the call: where
  };
  std::map<std::string, std::vector<OpenOp>> open_ops;
  // per neutral event: the first and last trace event made from it (the control edges, below)
  std::vector<uint64_t> first_tid(events.size(), eval::kNoEvent), last_tid(events.size(), eval::kNoEvent);
  for (size_t ei = 0; ei < events.size(); ++ei) {
    const kz_nevent& e = events[ei];
    const bool unreadable = (e.flags & KZ_NEV_FLAG_NO_VALUES) != 0;
    if (e.site == KZ_NONE32 || site_map[e.site].kind == MapKind::None) {
      ++report->unmapped_events;
      // an out value from a point inside the actor's innermost open call
      if (e.site != KZ_NONE32 && sites[e.site].r.kind == KZ_NSITE_KIND_POINT && !unreadable) {
        std::vector<OpenOp>& st = open_ops[names[e.actor]];
        if (!st.empty()) {
          const phi::OpDecl& op = p.ops[static_cast<size_t>(st.back().op)];
          for (uint8_t k = 0; k < kMaxSlots; ++k) {
            const auto r = role_of.find({e.site, k});
            if (r == role_of.end() || string_slot.count({e.site, k}) != 0) continue;
            const NValue* v = value_of(e, k);
            if (v == nullptr || v->r.kind == KZ_NVAL_TEXT) continue;
            for (const phi::OpParam& prm : op.params)
              if (prm.role == "out" && role_key(prm.name) == role_key(r->second)) {
                st.back().outs["out." + prm.name] = static_cast<int64_t>(v->r.v);
                ++report->out_values_from_points;
                if (store_operand.count({e.site, k}) != 0) {
                  st.back().stored["out." + prm.name] = sites[e.site].label + " #" + std::to_string(ei) + ": the operand of the store";
                  ++report->out_values_stored;
                } else {
                  st.back().stored.erase("out." + prm.name);  // a later value of another kind: no longer the store's
                }
              }
          }
        }
      }
      continue;
    }
    const SiteMap& m = site_map[e.site];
    if (unreadable) ++report->unreadable_events;
    // The trace event of a site read as `kind` (the operation's call or return, or the point `point`).
    auto make = [&](MapKind kind, const std::string& point, bool counted) {
      eval::TraceEvent te;
      te.actor = names[e.actor];
      te.site = kind == MapKind::Point ? point : m.name;
      te.phase = kind == MapKind::Call ? eval::Phase::Call : kind == MapKind::Return ? eval::Phase::Return : eval::Phase::Point;
      for (uint8_t k = 0; k < kMaxSlots && !unreadable; ++k) {
        const auto r = role_of.find({e.site, k});
        if (r == role_of.end()) continue;
        const NValue* val = value_of(e, k);
        if (val == nullptr) continue;  // not observed: the value stays unknown (never 0)
        const std::string key = role_key(r->second);
        const int64_t v = static_cast<int64_t>(val->r.v);
        // a string slot gives a text (a Str parameter or value), never its length as a number
        const bool is_text = string_slot.count({e.site, k}) != 0;
        if (is_text && val->r.kind != KZ_NVAL_TEXT) continue;  // no text was taken: the value stays unknown
        if (is_text && counted) ++report->string_values;
        if (kind == MapKind::Point) {
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
              if (counted) ++report->cell_values;
            }
            continue;
          }
          const auto& pn = point_names[point];
          const auto it = pn.find(key);
          if (it != pn.end()) {
            if (is_text) te.texts[it->second] = val->text;
            else te.values[it->second] = v;
          }
          continue;
        }
        const phi::OpDecl& op = p.ops[static_cast<size_t>(m.op)];
        if (kind == MapKind::Return && (key == "status" || key == "result")) {
          te.has_result = true;
          te.result = v;
          continue;
        }
        for (const phi::OpParam& prm : op.params) {
          if (role_key(prm.name) != key) continue;
          const std::string name = (kind == MapKind::Call ? "arg." : "out.") + prm.name;
          if ((kind == MapKind::Call) != (prm.role != "out")) continue;
          if (is_text) te.texts[name] = val->text;
          else te.values[name] = v;
        }
      }
      return te;
    };
    first_tid[ei] = out->events.size();
    // a point at the call site precedes the call it observes
    if (m.kind == MapKind::Call && !m.point_also.empty()) {
      out->add(make(MapKind::Point, m.point_also, false));
      ++report->events;
    }
    eval::TraceEvent te = make(m.kind, m.name, true);
    // the open calls of the actor (out values from points inside them go to their return)
    {
      std::vector<OpenOp>& st = open_ops[te.actor];
      if (m.kind == MapKind::Call) {
        st.push_back(OpenOp{m.op, {}, static_cast<uint64_t>(out->events.size()), {}});
      } else if (m.kind == MapKind::Return) {
        size_t k = st.size();
        while (k > 0 && st[k - 1].op != m.op) --k;
        if (k > 0) {
          if (!unreadable)
            for (const auto& ov : st[k - 1].outs)
              if (te.values.count(ov.first) == 0) {
                te.values[ov.first] = ov.second;
                const auto so = st[k - 1].stored.find(ov.first);
                if (so != st[k - 1].stored.end()) te.texts["store." + ov.first] = so->second;  // a direct capture of the store
              }
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
    // An argument from a linearization point of the operation of the actor's innermost open call (header,
    // "arguments"): the call event takes the point's value of a parameter the call event lacks.
    if (m.kind == MapKind::Point && !unreadable) {
      const auto po = point_op.find(m.name);
      std::vector<OpenOp>& st = open_ops[te.actor];
      if (po != point_op.end() && !st.empty() && st.back().op == po->second && st.back().call < out->events.size()) {
        eval::TraceEvent& call = out->events[static_cast<size_t>(st.back().call)];
        for (const auto& t : te.texts)
          if (t.first.compare(0, 4, "arg.") == 0 && call.texts.count(t.first) == 0 && call.values.count(t.first) == 0) {
            call.texts[t.first] = t.second;
            ++report->args_from_points;
          }
        for (const auto& v : te.values)
          if (v.first.compare(0, 4, "arg.") == 0 && call.texts.count(v.first) == 0 && call.values.count(v.first) == 0) {
            call.values[v.first] = v.second;
            ++report->args_from_points;
          }
      }
    }
    out->add(std::move(te));
    ++report->events;
    // a point at the call site follows the return it observes
    if (m.kind == MapKind::Return && !m.point_also.empty()) {
      out->add(make(MapKind::Point, m.point_also, false));
      ++report->events;
    }
    last_tid[ei] = out->events.size() - 1;
  }

  // ---- the realized control of a controlled run (P11, decided 2026-10-26; header, "control") ----
  // A release that resumed a held thread orders the releasing hit before the held thread's continuation: the edge goes
  // from the last trace event of the hit's actor at or before the hit (the hit's own event, or the one before it where
  // its site is not mapped) to the held thread's first trace event after its gate hit (the gate's observation record is
  // written before the wait; the held thread does nothing between).  The gate hold it ended is that thread's latest hold
  // of the release's gate before the release (ctl_seq, the controller's order).  A deferred release (the hit of a held
  // thread, acted on when it was resumed) orders that hit the same way.  No edge from a satisfied or consumed release, a
  // timeout, a refusal: the order of those runs came from the scheduler.  An edge that would go back in the merged order
  // is not made (reported): the order of edges would then not stand.
  report->control_records = n.ctl.size();
  if (!n.ctl.empty()) {
    std::vector<std::vector<size_t>> by_actor(actors.size());
    for (size_t ei = 0; ei < events.size(); ++ei)
      if (first_tid[ei] != eval::kNoEvent && events[ei].actor < actors.size()) by_actor[events[ei].actor].push_back(ei);
    auto first_after = [&](uint32_t a, uint32_t ev) -> uint64_t {
      if (a >= actors.size() || ev >= events.size()) return eval::kNoEvent;
      for (size_t x : by_actor[a])
        if (events[x].aseq > events[ev].aseq) return first_tid[x];
      return eval::kNoEvent;
    };
    auto last_at_or_before = [&](uint32_t a, uint32_t ev) -> uint64_t {
      uint64_t r = eval::kNoEvent;
      if (a >= actors.size() || ev >= events.size()) return r;
      for (size_t x : by_actor[a])
        if (events[x].aseq <= events[ev].aseq) r = last_tid[x];
      return r;
    };
    for (const kz_nctl& c : n.ctl) {
      if (c.what != KZ_NCTL_WHAT_FIRE || c.reason != KZ_NCTL_REASON_RESUMED) continue;
      ++report->control_resumes;
      const std::string what = "gate" + std::to_string(c.gate) + "/release" + std::to_string(c.point) + ((c.cflags & KZ_NCTL_FLAG_DEFERRED) != 0 ? "/deferred" : "");
      const kz_nctl* hold = nullptr;
      for (const kz_nctl& h : n.ctl)
        if (h.what == KZ_NCTL_WHAT_GATE_HOLD && h.actor == c.other && h.gate == c.gate && h.ctl_seq < c.ctl_seq && (hold == nullptr || h.ctl_seq > hold->ctl_seq)) hold = &h;
      const uint32_t hitter = c.subject != KZ_NONE32 ? c.subject : c.actor;
      if (hold == nullptr || hold->event == KZ_NONE32 || c.event == KZ_NONE32) {
        report->control_lines.push_back(what + ": no edge (the hold or the release hit has no observation event in the record)");
        continue;
      }
      const uint64_t from = last_at_or_before(hitter, c.event), to = first_after(c.other, hold->event);
      if (from == eval::kNoEvent || to == eval::kNoEvent) {
        report->control_lines.push_back(what + ": no edge (no mapped event of the releasing actor at or before its hit, or of the held actor after its gate)");
        continue;
      }
      if (from >= to) {
        report->control_lines.push_back(what + ": no edge (#" + std::to_string(from) + " is not before #" + std::to_string(to) + " in the merged order)");
        continue;
      }
      eval::HbEdge edge;
      edge.from = from;
      edge.to = to;
      edge.rule = "control:" + what;
      edge.source = 2;
      out->control_edges.push_back(edge);
      ++report->control_edges;
      const eval::TraceEvent& fe = out->events[static_cast<size_t>(from)];
      const eval::TraceEvent& tv = out->events[static_cast<size_t>(to)];
      report->control_lines.push_back(what + ": #" + std::to_string(from) + " " + fe.actor + " " + fe.site + " " + eval::phase_text(fe.phase) + " -> #" +
                                      std::to_string(to) + " " + tv.actor + " " + tv.site + " " + eval::phase_text(tv.phase));
    }
  }

  // ---- the start of the record ----
  report->starts_at_exec = n.run.r.start_evidence != KZ_NSTART_NONE;
  // Start state (IN-03, OB-05; 2026-10-24): what the record covers from the process start is the program entry,
  // so a location with an INIT fact of the profile (`init NAME: loc L = ...`, with its citation) has a known
  // start state.  A kind's start state is not declared here: the replay takes it from an observed effect that
  // sets every resource of the kind (the initialization code, e.g. the memset of a whole table), from that
  // event on (eval_trace.hpp).  Lifetime (ADR-055, IN-04): a kind of scope `process` whose every effect that
  // returns a resource to the kind's initial state (where the profile ends a lifetime) has its point observed,
  // in a record that covers the process from its start: the profile's life cycle is then tracked from the
  // first instance on.
  if (report->starts_at_exec) {
    out->start_evidence = KZ_START_EVIDENCE_INIT;
    for (const phi::KindDecl& k : p.kinds) {
      if (!k.derived.empty()) continue;
      eval::Adequacy a;
      bool ends_observed = k.scope.empty() || k.scope == "process";
      for (const phi::OpDecl& op : p.ops)
        for (const phi::EffectDecl& ef : op.effects) {
          if (ef.kind != phi::EffectKind::Set || ef.to_state != k.initial) continue;
          const int32_t ki = !ef.all_kind.empty() ? p.find_kind(ef.all_kind) : (ef.res ? ef.res->sort.kind_index : -1);
          if (ki < 0 || p.kinds[static_cast<size_t>(ki)].name != k.name) continue;
          const std::string pt = ef.lp.kind == phi::LpKind::None ? std::string() : eval::lp_name(op, ef.label, ef.lp);
          if (pt.empty() || out->observed_points.count(pt) == 0) {
            ends_observed = false;
            report->lifetime_lines.push_back("kind " + k.name + ": lifetime not established: the end of a lifetime " + op.name + "." + ef.label + " is not observed");
          }
        }
      a.lifetime_known = ends_observed;
      if (ends_observed) report->lifetime_lines.push_back("kind " + k.name + ": lifetime established (scope process, every end of a lifetime observed, record from the process start)");
      out->kind_adequacy[k.name] = a;
    }
    for (const phi::LocDecl& l : p.locs) {
      eval::Adequacy a;
      for (const phi::InitDecl& in : p.inits)
        if (in.is_loc && in.loc_name == l.name) a.start_known = true;
      out->loc_adequacy[l.name] = a;
    }
  }

  // ---- the static closure (header, "closure") ----
  if (opt.facts != nullptr) {
    std::vector<ObservedSite> obs;
    for (size_t i = 0; i < sites.size(); ++i) {
      const std::string& pt = site_map[i].kind == MapKind::Point ? site_map[i].name : site_map[i].point_also;
      if (pt.empty()) continue;
      ObservedSite o;
      o.point = pt;
      o.file = sites[i].source;
      o.line = sites[i].r.line;
      obs.push_back(std::move(o));
    }
    ClosureReport cr;
    s = apply_static_closure(*opt.facts, p, obs, out, &cr);
    if (!s.ok()) return s;
    report->closure_given = true;
    report->closure_lines = cr.lines;
  }

  // ---- losses ----
  // A loss of the control path only (the controller could not evaluate a hit) leaves the observation whole: reported,
  // not an interval.  Every other loss is an interval of the events it may affect.
  std::vector<eval::LossInterval> lv;
  report->loss_present = (n.run.r.flags & KZ_NRUN_FLAG_LOSS_PRESENT) != 0;
  for (const kz_nloss& l : n.losses) {
    if (l.unit_kind == KZ_NLOSS_UNIT_CPU_RING && l.kind != KZ_NLOSS_THROTTLE) report->loss_cpus.push_back({l.unit, l.count});
    else if (n.run.r.backend != KZ_NBACKEND_F0) report->loss_lines.push_back(loss_text(l));
    if (l.kind == KZ_NLOSS_CTL_SKIPPED || l.kind == KZ_NLOSS_CTL_NOCOND) continue;
    eval::LossInterval li;
    li.begin = l.begin;
    li.end = l.end == KZ_NONE32 ? eval::kNoEvent : l.end;
    lv.push_back(li);
  }
  // positions in the trace (only mapped events are kept): the first kept event at or after a neutral position
  std::vector<uint64_t> kept_before(events.size() + 1, 0);
  {
    uint64_t k = 0;
    for (size_t i = 0; i < events.size(); ++i) {
      kept_before[i] = k;
      if (events[i].site != KZ_NONE32 && site_map[events[i].site].kind != MapKind::None) k += site_map[events[i].site].point_also.empty() ? 1 : 2;
    }
    kept_before[events.size()] = k;
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
    out += "string values (fetch slots) " + std::to_string(r.string_values) + ", out values from points inside a call " + std::to_string(r.out_values_from_points) +
           (r.out_values_stored != 0 ? " (" + std::to_string(r.out_values_stored) + " of them the operand of a store)" : std::string()) + "\n";
  if (r.args_from_points != 0)
    out += "arguments from linearization points inside the call " + std::to_string(r.args_from_points) + " (the call event lacked them)\n";
  out += std::string("core status of owners: ") +
         (r.core_evidence ? "core " + std::to_string(r.owners_core) + ", not core " + std::to_string(r.owners_not_core) + ", not established " +
                                std::to_string(r.threads - r.owners_core - r.owners_not_core)
                          : std::string("not established (no basis symbols or no core request caller given)")) +
         "\n";
  for (const std::string& l : r.not_app_lines) out += "not an application's task: " + l + "\n";
  out += std::string("start ") +
         (!r.starts_at_exec ? "not shown to be the process start (start state unknown)"
          : r.start == KZ_NSTART_BOOTSTRAP
              ? "at the first instrumented hook (the runtime's static bootstrap buffer, no pre-init loss; INIT applies; a kind's start state comes from an observed effect on all of it)"
              : "at the first execution mapping of the traced file (INIT applies; a kind's start state comes from an observed effect on all of it)") +
         "\n";
  for (const std::string& l : r.lifetime_lines) out += l + "\n";
  if (r.control_records != 0)
    out += "realized control: records " + std::to_string(r.control_records) + ", releases that resumed a held thread " + std::to_string(r.control_resumes) +
           ", control edges " + std::to_string(r.control_edges) + " (P11: an order of this controlled run, a basis kind of its own)\n";
  for (const std::string& l : r.control_lines) out += "control " + l + "\n";
  out += std::string("loss ") + (r.loss_present ? "reported" : "none reported") + (r.runmeta ? "" : " (no RUNMETA: loss evidence absent, the record is treated as lossy)") +
         ", loss intervals " + std::to_string(r.loss_intervals) + "\n";
  for (const auto& lc : r.loss_cpus)
    out += "loss report: CPU ring " + std::to_string(lc.first) + ", " + std::to_string(lc.second) + " record(s); the task whose events were lost is not established\n";
  for (const std::string& l : r.loss_lines) out += "loss record: " + l + "\n";
  if (r.cell_values != 0) out += "values loaded from a named location (value(L) slots) " + std::to_string(r.cell_values) + "\n";
  if (r.closure_given) {
    for (const std::string& l : r.closure_lines) out += l + "\n";
    out += "writer and alias coverage: from the static closure above; order between actors: from the happens-before edges in the replay (the merged order is not used)\n";
  } else {
    out += "writer and alias coverage: not established by a run container (no static closure given); order between actors: from the happens-before edges in the replay\n";
  }
  return out;
}

}  // namespace kz::trace
