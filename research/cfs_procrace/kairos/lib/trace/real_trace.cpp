// real_trace.cpp -- the real-record trace of a run container (see real_trace.hpp).

#include "kz/trace/real_trace.hpp"

#include "kz/core/registry.hpp"

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
  if (!s.ok()) return s;
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
    }
    switch (m.kind) {
      case MapKind::Call: report->sites.push_back(labels[i] + ": call of " + m.name); break;
      case MapKind::Return: report->sites.push_back(labels[i] + ": return of " + m.name); break;
      case MapKind::Point: report->sites.push_back(labels[i] + ": point " + m.name); break;
      case MapKind::None: report->sites.push_back(labels[i] + ": not mapped"); break;
    }
  }
  std::map<std::pair<uint32_t, uint8_t>, std::string> role_of;
  for (const kz_site_slot& sl : slots) {
    std::string r;
    run.string(sl.role, &r);
    role_of[{sl.site, sl.slot}] = r;
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
    ti.thread_flags = threads[i].flags;
    const auto cr = out->creators.find(thread_names[i]);
    if (cr != out->creators.end()) ti.creator = cr->second;
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
    ++report->owners_determined;
  }
  report->owners_undetermined = threads.size() - report->owners_confirmed - report->owners_inferred;

  // ---- the events ----
  for (const kz_raw_ev& e : events) {
    if (e.site >= site_map.size() || site_map[e.site].kind == MapKind::None) {
      ++report->unmapped_events;
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
      if (m.kind == MapKind::Point) {
        if (key == "value") {
          te.values["value"] = v;
          continue;
        }
        const auto& names = point_names[m.name];
        const auto it = names.find(key);
        if (it != names.end()) te.values[it->second] = v;
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
        if (m.kind == MapKind::Call && prm.role != "out") te.values["arg." + prm.name] = v;
        if (m.kind == MapKind::Return && prm.role == "out") te.values["out." + prm.name] = v;
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

  // ---- losses ----
  std::vector<eval::LossInterval> lv;
  const bool flagged = !meta.empty() && (meta.front().flags & KZ_RUN_FLAG_LOSS_PRESENT) != 0;
  report->loss_present = flagged || !losses.empty() || meta.empty();
  for (const kz_loss& l : losses) {
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
             (ti.app_name.empty() ? std::string() : " app '" + ti.app_name + "'") + ", evidence " + flags_text("KZ_OWNER_EVIDENCE", ti.evidence) + "\n";
    }
  }
  out += std::string("start ") + (r.starts_at_exec ? "at the first execution mapping of the traced file (INIT applies)" : "not shown to be the process start (start state unknown)") + "\n";
  out += std::string("loss ") + (r.loss_present ? "reported" : "none reported") + (r.runmeta ? "" : " (no RUNMETA: loss evidence absent, the record is treated as lossy)") +
         ", loss intervals " + std::to_string(r.loss_intervals) + "\n";
  out += "writer and alias coverage, lifetimes, order between actors: not established by a run container\n";
  return out;
}

}  // namespace kz::trace
