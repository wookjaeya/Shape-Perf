// replay_container.cpp -- the output container of a real-record replay (see replay_container.hpp).

#include "kz/trace/replay_container.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

namespace kz::trace {

namespace {

template <class T>
T zeroed() {
  T v;
  std::memset(&v, 0, sizeof v);
  return v;
}

}  // namespace

Status write_replay_container(const ReplayContainerInput& in, const eval::Trace& trace, const eval::Result& result, const RealTraceReport& report, Bytes* out) {
  ContainerWriter w(KZ_ARTIFACT_VERDICTS);
  Digest basis = kZeroDigest;
  if (in.run != nullptr) {
    std::memcpy(basis.data(), in.run->header().basis_sha256, basis.size());
    Status s = w.add_parent(KZ_ARTIFACT_RUN, 0, in.run->artifact_hash());
    if (!s.ok()) return s;
  }
  w.set_basis_hash(basis);
  w.set_profile_hash(in.profile_sha256);
  w.set_tool_hash(in.tool_sha256);

  // BASIS: the replay inputs.
  std::vector<kz_basis_item> items;
  for (const auto& kv : in.basis) {
    kz_basis_item b = zeroed<kz_basis_item>();
    b.name = w.intern(kv.first);
    b.value = w.intern(kv.second);
    b.policy = KZ_POLICY_UNSET;
    b.prov = KZ_NONE32;
    b.deviation = KZ_NONE32;
    items.push_back(b);
  }

  // MODEL_IDS: one record for every summary of this container.
  kz_model_id mid = zeroed<kz_model_id>();
  mid.model_id = w.intern("");
  mid.model_level = KZ_MLEVEL_NONE;
  mid.sigma = KZ_SIGMA_E;
  mid.schedule_profile_id = w.intern("");
  mid.basis_id = w.intern(basis == kZeroDigest ? std::string() : to_hex(basis));
  mid.scope_id = w.intern("");
  mid.profile_id = w.intern(to_hex(in.profile_sha256));
  std::vector<kz_model_id> models{mid};

  // Thread identities by actor name.
  std::map<std::string, size_t> by_actor;
  for (size_t i = 0; i < report.identities.size(); ++i) by_actor[report.identities[i].actor] = i;
  auto identity_of = [&](const std::string& actor) {
    kz_identity id = zeroed<kz_identity>();
    id.owner_key = KZ_NONE32;
    id.exec_name = w.intern(actor);
    id.creator_name = w.intern("");
    id.owner_name = w.intern("");
    id.passed_role = w.intern("");
    const auto ti = by_actor.find(actor);
    if (ti != by_actor.end()) {
      const ThreadIdentity& t = report.identities[ti->second];
      id.exec_tid = t.tid;
      id.exec_generation = t.generation;
      id.flags |= KZ_IDENTITY_FLAG_EXEC_KNOWN;
      id.owner_evidence = t.evidence;
      id.owner_name = w.intern(t.app_name);
      if (t.creator_thread != KZ_NONE32 && t.creator_thread < report.identities.size()) {
        const ThreadIdentity& c = report.identities[t.creator_thread];
        id.creator_tid = c.tid;
        id.creator_generation = c.generation;
        id.creator_name = w.intern(c.actor);
        id.flags |= KZ_IDENTITY_FLAG_CREATOR_KNOWN;
      }
    }
    if (!(id.flags & KZ_IDENTITY_FLAG_CREATOR_KNOWN)) {
      const auto cr = trace.creators.find(actor);
      if (cr != trace.creators.end()) id.creator_name = w.intern(cr->second);  // by name only: no thread id
    }
    const auto lv = trace.owner_levels.find(actor);
    const auto ok = trace.owners.find(actor);
    if (lv != trace.owner_levels.end() && lv->second != KZ_OWNER_LEVEL_UNDETERMINED && ok != trace.owners.end() && ok->second.size() == 1) {
      id.owner_level = lv->second;
      id.owner_key = static_cast<uint32_t>(ok->second.front());
    }
    return id;
  };

  std::vector<kz_summary> summaries;
  std::vector<kz_obs_adequacy> adequacy;
  std::vector<kz_identity> identities;
  // REF_LISTS: the probe lists of OBS_ADEQUACY, each a run of references to the parent's SITES (NSITES for a run
  // without F0 sites, an instrumented run) ended by a null reference; equal lists are written once.
  std::vector<kz_ref> ref_lists;
  std::map<std::vector<uint32_t>, uint32_t> list_at;
  auto probe_list = [&](const eval::Occurrence& o) {
    kz_ref r = zeroed<kz_ref>();
    if (in.run == nullptr) return r;
    std::set<uint32_t> ids;
    uint32_t section = KZ_SEC_SITES;  // the backend's site records (F0 SITES), or the neutral sites without them
    for (const std::string& name : o.probes) {
      uint8_t kind = 3;
      std::string n = name;
      if (name.rfind("call ", 0) == 0) {
        kind = 1;
        n = name.substr(5);
      } else if (name.rfind("return ", 0) == 0) {
        kind = 2;
        n = name.substr(7);
      }
      for (const SiteMapEntry& e : report.site_map)
        if (e.kind == kind && e.name == n) {
          ids.insert(e.site);
          section = e.section;
        }
    }
    if (ids.empty()) return r;
    const std::vector<uint32_t> key(ids.begin(), ids.end());
    auto it = list_at.find(key);
    if (it == list_at.end()) {
      it = list_at.emplace(key, static_cast<uint32_t>(ref_lists.size())).first;
      const Digest run_hash = in.run->artifact_hash();
      for (uint32_t id : key) {
        kz_ref e = zeroed<kz_ref>();
        std::memcpy(e.artifact_sha256, run_hash.data(), run_hash.size());
        e.section = section;
        e.index = id;
        ref_lists.push_back(e);
      }
      ref_lists.push_back(zeroed<kz_ref>());  // the end of the list
    }
    r.section = KZ_SEC_REF_LISTS;
    r.index = it->second;
    return r;
  };
  for (const eval::Occurrence& o : result.occurrences) {
    kz_summary s = o.summary;
    s.cell = KZ_NONE32;
    s.model = 0;
    s.adequacy = static_cast<uint32_t>(adequacy.size());
    s.identity_first = static_cast<uint32_t>(identities.size());
    s.conf_reason = w.intern("");

    kz_identity exec = identity_of(o.actor);
    exec.passed_role = w.intern(o.passed_role);
    if (o.has_passed) {
      exec.passed_id = static_cast<uint32_t>(o.passed);
      exec.flags |= KZ_IDENTITY_FLAG_PASSED_ID_PRESENT;
    }
    identities.push_back(exec);
    for (const std::string& c : o.counterparts) identities.push_back(identity_of(c));
    s.n_identities = static_cast<uint32_t>(identities.size()) - s.identity_first;
    summaries.push_back(s);

    kz_obs_adequacy a = zeroed<kz_obs_adequacy>();
    a.established = o.adequacy;
    a.absent = 0;
    a.reasons = o.reasons;
    a.start_evidence = trace.start_evidence;
    a.first_event = o.call;
    const bool decided = o.decided_at != eval::DecidedAt::None;
    a.last_event = decided ? o.decision : o.returned ? o.ret : (trace.events.empty() ? o.call : trace.events.size() - 1);
    uint64_t losses = 0;
    for (const eval::LossInterval& l : trace.losses)
      if (l.begin <= a.last_event && l.end > a.first_event) ++losses;
    a.loss_reports = losses;
    if (decided) a.end_reason = KZ_INTERVAL_END_DECISION;
    else if (losses != 0) a.end_reason = KZ_INTERVAL_END_LOSS;
    else if (result.terminated) a.end_reason = KZ_INTERVAL_END_PROCESS_TERMINAL;
    else a.end_reason = KZ_INTERVAL_END_RECORD_END;
    a.probes = probe_list(o);
    std::string unobs;
    for (const std::string& u : o.unobserved) unobs += (unobs.empty() ? "" : ";") + u;
    a.unobserved = w.intern(unobs);
    adequacy.push_back(a);
  }

  // OCC_BASIS and OCC_EDGES (schema revision 4): the per-occurrence basis, its key and the edges it rests on.
  std::vector<kz_occ_basis> occ;
  std::vector<kz_occ_edge> occ_edges;
  auto hex_join = [](const std::vector<int64_t>& v) {
    std::string text;
    char buf[24];
    for (int64_t x : v) {
      std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(x));
      text += (text.empty() ? "" : ",") + std::string(buf);
    }
    return text;
  };
  for (size_t i = 0; i < result.occurrences.size(); ++i) {
    const eval::Occurrence& o = result.occurrences[i];
    kz_occ_basis b = zeroed<kz_occ_basis>();
    b.summary = static_cast<uint32_t>(i);
    b.ordinal = static_cast<uint32_t>(o.ordinal);
    b.call = static_cast<uint32_t>(o.call);
    b.decision = o.decided_at != eval::DecidedAt::None ? static_cast<uint32_t>(o.decision) : KZ_NONE32;
    static const std::pair<uint16_t, uint32_t> kBasis[] = {{KZ_SUMMARY_FLAG_BASIS_DIRECT_CAPTURE, KZ_OCC_BASIS_DIRECT_CAPTURE},
                                                           {KZ_SUMMARY_FLAG_BASIS_STATE_ORDER, KZ_OCC_BASIS_STATE_ORDER},
                                                           {KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER, KZ_OCC_BASIS_RETURN_DERIVED_ORDER},
                                                           {KZ_SUMMARY_FLAG_OUTCOME_NARROWED, KZ_OCC_BASIS_OUTCOME_NARROWED}};
    for (const auto& k : kBasis)
      if ((o.basis & k.first) != 0) b.basis |= k.second;
    if (o.basis_store_order) b.basis |= KZ_OCC_BASIS_STORE_DERIVED_ORDER;
    if (o.control_order) b.basis |= KZ_OCC_BASIS_CONTROL_ORDER;
    if ((o.assumptions & KZ_SUMMARY_FLAG_ASSUMES_VIA_SEPARATION) != 0) b.assumptions |= KZ_OCC_ASSUME_VIA_KEY_SEPARATION;
    if ((o.assumptions & KZ_SUMMARY_FLAG_ASSUMES_OWNER_IDENTITY) != 0) b.assumptions |= KZ_OCC_ASSUME_OWNER_IDENTITY_TARGETS;
    if ((o.assumptions & KZ_SUMMARY_FLAG_ASSUMES_ROLE) != 0) b.assumptions |= KZ_OCC_ASSUME_ROLE;
    if ((o.basis & KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER) != 0 || o.basis_store_order) b.assumptions |= KZ_OCC_ASSUME_READS_FROM;
    b.obs_reasons = o.summary.obs_reasons;
    b.status = o.summary.status;
    b.status_flags = o.summary.status_flags;
    b.observation = o.summary.observation;
    b.outcome = static_cast<uint8_t>(o.outcome);
    b.relation = o.summary.relation;
    b.owner_key = KZ_NONE32;
    const auto lv = trace.owner_levels.find(o.actor);
    const auto ok = trace.owners.find(o.actor);
    if (lv != trace.owner_levels.end() && lv->second != KZ_OWNER_LEVEL_UNDETERMINED && ok != trace.owners.end() && ok->second.size() == 1) {
      b.owner_level = lv->second;
      b.owner_key = static_cast<uint32_t>(ok->second.front());
    }
    const auto ti = by_actor.find(o.actor);
    b.owner_app = w.intern(ti != by_actor.end() ? report.identities[ti->second].app_name : std::string());
    // the task's place in its application (revision 5), with the owner it is a place in
    if (b.owner_key != KZ_NONE32 && ti != by_actor.end() && report.identities[ti->second].task_ordinal <= 0xFFFFu)
      b.task = static_cast<uint16_t>(report.identities[ti->second].task_ordinal);
    b.rspec = w.intern(o.rspec);
    b.op = w.intern(o.call < trace.events.size() ? trace.events[static_cast<size_t>(o.call)].site : std::string());
    b.context = w.intern(o.context);
    b.resource = w.intern(o.key_known ? hex_join(o.key) : std::string());
    std::string name;
    if (o.call < trace.events.size())
      for (const auto& t : trace.events[static_cast<size_t>(o.call)].texts)
        if (t.first.compare(0, 4, "arg.") == 0) name += (name.empty() ? "" : ";") + t.first.substr(4) + "=" + t.second;
    b.name = w.intern(name);
    std::string via;
    for (const std::string& v : o.via_names) via += (via.empty() ? "" : ";") + v;
    b.via = w.intern(via);
    b.first_edge = static_cast<uint32_t>(occ_edges.size());
    for (uint32_t ei : o.support) {
      if (ei >= result.order_edges.size()) continue;
      const eval::HbEdge& e = result.order_edges[ei];
      kz_occ_edge x = zeroed<kz_occ_edge>();
      x.occurrence = static_cast<uint32_t>(i);
      x.from = static_cast<uint32_t>(e.from);
      x.to = static_cast<uint32_t>(e.to);
      x.kind = e.source == 2 ? KZ_OCC_EDGE_KIND_CONTROL : e.source == 1 ? KZ_OCC_EDGE_KIND_READS_FROM : KZ_OCC_EDGE_KIND_RULE;
      if (e.by_store) x.flags |= KZ_OCC_EDGE_FLAG_BY_STORE;
      x.rule = w.intern(e.rule);
      occ_edges.push_back(x);
    }
    b.n_edges = static_cast<uint32_t>(occ_edges.size()) - b.first_edge;
    occ.push_back(b);
  }
  // REPLAY_READ: the reading in force.
  kz_replay_read rd = zeroed<kz_replay_read>();
  rd.format = 2;  // OCC_BASIS task filled (schema revision 5)
  rd.control_edges = static_cast<uint32_t>(trace.control_edges.size());
  rd.reordered = static_cast<uint32_t>(trace.reordered);
  if (in.options != nullptr) {
    auto sw = [](bool on) { return static_cast<uint8_t>(on ? KZ_READ_SWITCH_ON : KZ_READ_SWITCH_OFF); };
    rd.outcome_narrowing = sw(in.options->outcome_narrowing);
    rd.owner_identity_targets = sw(in.options->owner_identity_targets);
    rd.reads_from_edges = sw(in.options->reads_from_edges);
    rd.reads_from_outcome = sw(in.options->reads_from_outcome);
    rd.control_order = sw(in.options->control_order);
    rd.relation_rule = in.options->relation_rule == eval::Options::RelationRule::Occurrence  ? KZ_RELATION_RULE_P10_OCCURRENCE
                       : in.options->relation_rule == eval::Options::RelationRule::AllWrites ? KZ_RELATION_RULE_CLEARING_COUNTED
                                                                                             : KZ_RELATION_RULE_CLEARING_EXCLUDED;
  }
  std::vector<kz_replay_read> reading{rd};

  // ACTOR_OWNERS, copied from the run container (indices into the parent's sections).
  std::vector<kz_app_owner> owners;
  if (in.run != nullptr) {
    Status s = in.run->read_section(KZ_SEC_ACTOR_OWNERS, &owners);
    if (!s.ok()) return s;
    for (kz_app_owner& o : owners) {
      std::string name;
      in.run->string(o.app_name, &name);
      o.app_name = w.intern(name);
    }
  }

  Status s = w.add_section(KZ_SEC_BASIS, items);
  if (s.ok() && !ref_lists.empty()) s = w.add_section(KZ_SEC_REF_LISTS, ref_lists);
  if (s.ok()) s = w.add_section(KZ_SEC_SUMMARIES, summaries);
  if (s.ok()) s = w.add_section(KZ_SEC_MODEL_IDS, models);
  if (s.ok()) s = w.add_section(KZ_SEC_OBS_ADEQUACY, adequacy);
  if (s.ok()) s = w.add_section(KZ_SEC_IDENTITIES, identities);
  if (s.ok() && in.run != nullptr && in.run->find_section(KZ_SEC_ACTOR_OWNERS) != nullptr) s = w.add_section(KZ_SEC_ACTOR_OWNERS, owners);
  if (s.ok()) s = w.add_section(KZ_SEC_OCC_BASIS, occ);
  if (s.ok()) s = w.add_section(KZ_SEC_OCC_EDGES, occ_edges);
  if (s.ok()) s = w.add_section(KZ_SEC_REPLAY_READ, reading);
  if (!s.ok()) return s;
  return w.finish(out);
}

}  // namespace kz::trace
