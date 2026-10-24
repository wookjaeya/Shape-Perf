// replay_container.cpp -- the output container of a real-record replay (see replay_container.hpp).

#include "kz/trace/replay_container.hpp"

#include <algorithm>
#include <cstring>
#include <map>

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
    a.probes = zeroed<kz_ref>();
    a.unobserved = w.intern("");
    adequacy.push_back(a);
  }

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
  if (s.ok()) s = w.add_section(KZ_SEC_SUMMARIES, summaries);
  if (s.ok()) s = w.add_section(KZ_SEC_MODEL_IDS, models);
  if (s.ok()) s = w.add_section(KZ_SEC_OBS_ADEQUACY, adequacy);
  if (s.ok()) s = w.add_section(KZ_SEC_IDENTITIES, identities);
  if (s.ok() && in.run != nullptr && in.run->find_section(KZ_SEC_ACTOR_OWNERS) != nullptr) s = w.add_section(KZ_SEC_ACTOR_OWNERS, owners);
  if (!s.ok()) return s;
  return w.finish(out);
}

}  // namespace kz::trace
