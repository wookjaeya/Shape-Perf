// The output container of a real-record replay (kz/trace/replay_container.hpp, `kz replay --out`): a
// round trip on the mini profile (tests/unit/support/phi_mini.hpp) and a small synthetic run container.
// Everything here is a unit-test value, not a research condition (CONDITIONS_POLICY).
//
//   - the container parses, verifies, names the run as its parent and carries its basis hash;
//   - SUMMARIES are the replay's summaries (observation, relation, flags), with valid indices into
//     MODEL_IDS, OBS_ADEQUACY and IDENTITIES; the identities are the use's executing task first, then the
//     actors of the registrations the relation used;
//   - ACTOR_OWNERS is copied with its strings, BASIS holds the replay inputs;
//   - the same inputs give the same bytes; kz-dump's canonical dump shows the new fields.

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/verdict.hpp"
#include "kz/trace/replay_container.hpp"
#include "support/kz_test.hpp"
#include "support/phi_mini.hpp"

namespace {

using kz::Container;
using kz::eval::Result;
using kz::eval::Trace;

kz::phi::Profile load() {
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  KZ_CHECK_OK(kz::phi::load_profile(kzmini::files(), &p, &diags));
  return p;
}

// A real record: W registers slot 2's identity, X (another application) and W both send with it.
const char* const kTrace =
    "record real\n"
    "start_evidence init\n"
    "order established\n"
    "adequacy kind slot start=known writers=covered aliases=covered lifetime=known\n"
    "adequacy kind reg start=known writers=covered aliases=covered lifetime=known\n"
    "observed create_all.reserve\n"
    "observed create_all.publish\n"
    "observed sign_in.clear_reg\n"
    "observed sign_in.set_reg\n"
    "observed send.who_reg\n"
    "owner_level W confirmed\n"
    "owner_level X inferred\n"
    "owner_id W 0x1002\n"
    "owner_id X 0x1003\n"
    "creator W T0\n"
    "const TABLE 4\n"
    "owner W 2\n"
    "owner X 3\n"
    "T0 create_all.reserve pt lp.n=2\n"
    "T0 create_all.publish pt lp.n=2 lp.pid=0x1002\n"
    "W sign_in call\n"
    "W sign_in.set_reg pt\n"
    "W sign_in ret result=0\n"
    "X send.who_valid pt arg.Who=0x1002\n"
    "X send call arg.Who=0x1002\n"
    "X send.who_reg pt\n"
    "X send ret result=0\n"
    "W send.who_valid pt arg.Who=0x1002\n"
    "W send call arg.Who=0x1002\n"
    "W send.who_reg pt\n"
    "W send ret result=0\n";

// A run container with two app_owner records (the only section the writer copies).
Container make_run() {
  kz::ContainerWriter w(KZ_ARTIFACT_RUN);
  w.set_basis_hash(kz::sha256(std::string("unit-test basis")));
  std::vector<kz_app_owner> owners(2);
  std::memset(owners.data(), 0, owners.size() * sizeof(kz_app_owner));
  owners[0].thread = 0;
  owners[0].creator_thread = 2;
  owners[0].creation_event = 7;
  owners[0].owner_key = 0x1002;
  owners[0].level = KZ_OWNER_LEVEL_CONFIRMED;
  owners[0].evidence = KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_ANNOUNCED_ID;
  owners[0].app_name = w.intern("APP_W");
  owners[1].thread = 1;
  owners[1].creator_thread = KZ_NONE32;
  owners[1].creation_event = KZ_NONE32;
  owners[1].owner_key = 0x1003;
  owners[1].level = KZ_OWNER_LEVEL_INFERRED;
  owners[1].evidence = KZ_OWNER_EVIDENCE_MATCH_BY_TIME | KZ_OWNER_EVIDENCE_ID_UNVERIFIED;
  owners[1].app_name = w.intern("APP_X");
  KZ_CHECK_OK(w.add_section(KZ_SEC_ACTOR_OWNERS, owners));
  kz::Bytes b;
  KZ_CHECK_OK(w.finish(&b));
  Container c;
  KZ_CHECK_OK(Container::parse(b, &c));
  return c;
}

kz::trace::RealTraceReport make_report() {
  kz::trace::RealTraceReport r;
  r.identities.resize(3);
  r.identities[0].actor = "W";
  r.identities[0].tid = 11;
  r.identities[0].creator_thread = 2;
  r.identities[0].evidence = KZ_OWNER_EVIDENCE_CREATION_REQUEST | KZ_OWNER_EVIDENCE_ANNOUNCED_ID;
  r.identities[0].app_name = "APP_W";
  r.identities[1].actor = "X";
  r.identities[1].tid = 12;
  r.identities[1].app_name = "APP_X";
  r.identities[2].actor = "T0";
  r.identities[2].tid = 10;
  return r;
}

void test_round_trip() {
  const kz::phi::Profile p = load();
  Trace t;
  KZ_CHECK_OK(kz::eval::parse_trace_text(kTrace, &t));
  Result r;
  KZ_CHECK_OK(kz::eval::replay(p, t, kz::eval::Options(), &r));
  KZ_CHECK_EQ(r.occurrences.size(), static_cast<size_t>(2));
  const Container run = make_run();
  const kz::trace::RealTraceReport rep = make_report();
  kz::trace::ReplayContainerInput in;
  in.run = &run;
  in.profile_sha256 = kz::phi::profile_hash(p);
  in.tool_sha256 = kz::sha256(std::string("unit-test tool"));
  in.basis = {{"replay.mode", "real-record"}, {"replay.const.TABLE_SIZE", "4"}};
  kz::Bytes bytes, again;
  KZ_CHECK_OK(kz::trace::write_replay_container(in, t, r, rep, &bytes));
  KZ_CHECK_OK(kz::trace::write_replay_container(in, t, r, rep, &again));
  KZ_CHECK(bytes == again);  // no time, no host: the same inputs give the same bytes

  Container c;
  KZ_CHECK_OK(Container::parse(bytes, &c));
  KZ_CHECK_OK(c.verify_content());
  KZ_CHECK_EQ(c.header().artifact_kind, static_cast<uint32_t>(KZ_ARTIFACT_VERDICTS));
  KZ_CHECK_EQ(c.parents().size(), static_cast<size_t>(1));
  KZ_CHECK(std::memcmp(c.parents()[0].sha256, run.artifact_hash().data(), 32) == 0);
  KZ_CHECK_OK(c.verify_parent(run));
  KZ_CHECK(std::memcmp(c.header().basis_sha256, run.header().basis_sha256, 32) == 0);
  KZ_CHECK(std::memcmp(c.header().profile_sha256, in.profile_sha256.data(), 32) == 0);

  std::vector<kz_summary> sums;
  std::vector<kz_model_id> models;
  std::vector<kz_obs_adequacy> adeq;
  std::vector<kz_identity> ids;
  std::vector<kz_app_owner> owners;
  std::vector<kz_basis_item> basis;
  KZ_CHECK_OK(c.read_section(KZ_SEC_SUMMARIES, &sums));
  KZ_CHECK_OK(c.read_section(KZ_SEC_MODEL_IDS, &models));
  KZ_CHECK_OK(c.read_section(KZ_SEC_OBS_ADEQUACY, &adeq));
  KZ_CHECK_OK(c.read_section(KZ_SEC_IDENTITIES, &ids));
  KZ_CHECK_OK(c.read_section(KZ_SEC_ACTOR_OWNERS, &owners));
  KZ_CHECK_OK(c.read_section(KZ_SEC_BASIS, &basis));
  KZ_CHECK_EQ(sums.size(), static_cast<size_t>(2));
  KZ_CHECK_EQ(adeq.size(), static_cast<size_t>(2));
  KZ_CHECK_EQ(models.size(), static_cast<size_t>(1));
  auto str = [&](const kz_str& s) {
    std::string out;
    KZ_CHECK(c.string(s, &out));
    return out;
  };
  for (size_t i = 0; i < sums.size(); ++i) {
    const kz_summary& s = sums[i];
    const kz::eval::Occurrence& o = r.occurrences[i];
    KZ_CHECK(kz::validate_summary(s).empty());
    KZ_CHECK_EQ(s.observation, o.summary.observation);
    KZ_CHECK_EQ(s.relation, o.relation);
    KZ_CHECK_EQ(s.flags, o.summary.flags);
    KZ_CHECK_EQ(s.cell, KZ_NONE32);
    KZ_CHECK_EQ(s.model, 0u);
    KZ_CHECK_EQ(s.adequacy, static_cast<uint32_t>(i));
    KZ_CHECK_EQ(s.n_identities, static_cast<uint32_t>(1 + o.counterparts.size()));
    KZ_CHECK(s.identity_first + s.n_identities <= ids.size());
    KZ_CHECK_EQ(adeq[i].first_event, o.call);
    KZ_CHECK_EQ(adeq[i].established, o.adequacy);
    KZ_CHECK_EQ(adeq[i].end_reason, static_cast<uint8_t>(KZ_INTERVAL_END_DECISION));
    KZ_CHECK_EQ(adeq[i].start_evidence, static_cast<uint8_t>(KZ_START_EVIDENCE_INIT));
    const kz_identity& exec = ids[s.identity_first];
    KZ_CHECK_EQ(str(exec.exec_name), o.actor);
    KZ_CHECK((exec.flags & KZ_IDENTITY_FLAG_PASSED_ID_PRESENT) != 0 && exec.passed_id == 0x1002u);
    KZ_CHECK_EQ(str(exec.passed_role), std::string("Who"));
  }
  // X: another application with an inferred owner; W: its own registration
  KZ_CHECK_EQ(sums[0].relation, static_cast<uint8_t>(KZ_EXEC_RELATION_INTER_APPLICATION));
  KZ_CHECK((sums[0].flags & KZ_SUMMARY_FLAG_RELATION_INFERRED) != 0);
  KZ_CHECK((sums[0].flags & KZ_SUMMARY_FLAG_CORE_STATUS_UNKNOWN) != 0);
  KZ_CHECK_EQ(sums[1].relation, static_cast<uint8_t>(KZ_EXEC_RELATION_INTRA_APPLICATION));
  const kz_identity& x = ids[sums[0].identity_first];
  const kz_identity& w = ids[sums[0].identity_first + 1];
  KZ_CHECK(x.exec_tid == 12 && (x.flags & KZ_IDENTITY_FLAG_EXEC_KNOWN) != 0 && (x.flags & KZ_IDENTITY_FLAG_CREATOR_KNOWN) == 0);
  KZ_CHECK(x.owner_key == 3 && x.owner_level == KZ_OWNER_LEVEL_INFERRED);
  KZ_CHECK(w.exec_tid == 11 && str(w.exec_name) == "W" && str(w.owner_name) == "APP_W");
  KZ_CHECK(w.owner_key == 2 && w.owner_level == KZ_OWNER_LEVEL_CONFIRMED);
  KZ_CHECK((w.flags & KZ_IDENTITY_FLAG_CREATOR_KNOWN) != 0 && w.creator_tid == 10 && str(w.creator_name) == "T0");
  KZ_CHECK((w.flags & KZ_IDENTITY_FLAG_PASSED_ID_PRESENT) == 0);
  KZ_CHECK_EQ(models[0].sigma, static_cast<uint8_t>(KZ_SIGMA_E));
  KZ_CHECK_EQ(models[0].model_level, static_cast<uint8_t>(KZ_MLEVEL_NONE));
  KZ_CHECK_EQ(str(models[0].profile_id), kz::to_hex(in.profile_sha256));
  KZ_CHECK_EQ(str(models[0].basis_id), kz::to_hex(kz::sha256(std::string("unit-test basis"))));
  KZ_CHECK_EQ(str(models[0].model_id), std::string());
  KZ_CHECK_EQ(owners.size(), static_cast<size_t>(2));
  KZ_CHECK(owners[0].owner_key == 0x1002 && str(owners[0].app_name) == "APP_W" && owners[0].creation_event == 7);
  KZ_CHECK(owners[1].level == KZ_OWNER_LEVEL_INFERRED && str(owners[1].app_name) == "APP_X");
  KZ_CHECK_EQ(basis.size(), static_cast<size_t>(2));
  KZ_CHECK(str(basis[1].name) == "replay.const.TABLE_SIZE" && str(basis[1].value) == "4" && basis[1].policy == KZ_POLICY_UNSET);

  const std::string dump = kz::dump_container(c);
  KZ_CHECK(dump.find("kind=verdicts") != std::string::npos);
  KZ_CHECK(dump.find("relation=inter-application") != std::string::npos);
  const bool flags_shown = dump.find("flags={real-record,outcome-narrowed,relation-inferred,core-status-unknown}") != std::string::npos;
  KZ_CHECK(flags_shown);
  if (!flags_shown) std::fprintf(stderr, "%s\n", dump.c_str());
  KZ_CHECK(dump.find("exec_name=\"X\"") != std::string::npos);
  KZ_CHECK(dump.find("app_name=\"APP_W\"") != std::string::npos);

  // without a run container: no parent, no ACTOR_OWNERS, zero basis hash, empty basis id
  kz::trace::ReplayContainerInput bare = in;
  bare.run = nullptr;
  kz::Bytes b2;
  KZ_CHECK_OK(kz::trace::write_replay_container(bare, t, r, kz::trace::RealTraceReport(), &b2));
  Container c2;
  KZ_CHECK_OK(Container::parse(b2, &c2));
  KZ_CHECK_OK(c2.verify_content());
  KZ_CHECK(c2.parents().empty());
  KZ_CHECK(c2.find_section(KZ_SEC_ACTOR_OWNERS) == nullptr);
  std::vector<kz_identity> ids2;
  KZ_CHECK_OK(c2.read_section(KZ_SEC_IDENTITIES, &ids2));
  KZ_CHECK(!ids2.empty() && (ids2[0].flags & KZ_IDENTITY_FLAG_EXEC_KNOWN) == 0);
}

}  // namespace

int main() {
  test_round_trip();
  return kztest::finish("test_trace_replay_out");
}
