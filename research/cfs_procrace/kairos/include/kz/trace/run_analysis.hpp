// run_analysis.hpp -- the public inputs and results of the record analyses that write the schema revision 12 side
// tables of a run's neutral layer (kz/schema/kz_neutral.h sections 112-118, kz/trace/neutral.hpp NeutralRun):
//
//   same-build verdict   the links between a run and the build its plan and facts describe (NBUILD);
//   token join           the actors tied to their creating calls by the creation token (NACTOR_TOKEN);
//   sufficiency          the state of every planned need and the cause of every call without a return (NSUFFICE,
//                        NCALLEND);
//   lifetimes            the generations of keyed resources (NLIFE);
//   order instances      the instances of static order edges in the run and the order observed per relation row
//                        (NORDER, NORDER_OBS).
//
// Only the structures are defined here (schema revision 12 puts every public structure of this work in include/ once).
// The passes that fill them are declared and defined beside their code in lib/trace, which keeps a later pass from
// changing include/ and with it the identity of every tool (cmake/KzSourceIdentity.cmake).  No pass exists in this
// revision: a conversion writes none of the side tables.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "kz/trace/neutral.hpp"

namespace kz::trace {

// ---- same-build verdict (NBUILD) ------------------------------------------------------------------------------------
// One file the check compares: its path, the sha256 the check expects (hex, empty = not known) and, for an executable
// or module, the build-id the run recorded (empty = not recorded).
struct SameBuildFile {
  std::string path;
  std::string sha256;
  std::string build_id;
};

// What a conversion is given to decide whether the run is of the build its plan and facts describe.
struct SameBuildInput {
  std::string manifest_path;      // the site manifest the dump names (its sha256 is in the dump)
  std::string plan_path;          // the plan the manifest was generated from
  std::string plan_facts_sha256;  // the facts the plan names (hex)
  std::string facts_path;         // the facts given to the conversion
  std::string basis_path;         // the derived build's basis (its binary and module hashes)
  std::string derived_facts_path; // the derived build's facts, compared with the plan's facts section by section
  std::vector<SameBuildFile> executables;  // the run's executables and modules (from the run's file lines or given)
};

// The verdict: one NBuild per link and the verdict of the whole (KZ_NBUILD_VERDICT; `confirmed` only when every link
// is), which is also the NBuild record of link `run`.
struct SameBuildResult {
  uint8_t verdict = KZ_NBUILD_VERDICT_UNCONFIRMED;
  std::vector<NBuild> links;
};

// ---- token join (NACTOR_TOKEN) --------------------------------------------------------------------------------------
// One NToken per actor with a creation token or a join result, and the counts of the join's outcomes by KZ_NTIE value
// and of the self-reported id checks by KZ_NSELF value.
struct TokenJoinResult {
  std::vector<NToken> tokens;
  std::map<uint8_t, uint32_t> by_tie;
  std::map<uint8_t, uint32_t> by_self_check;
};

// ---- sufficiency (NSUFFICE, NCALLEND) -------------------------------------------------------------------------------
// The plan and the facts a run's needs are read from.
struct SufficeInput {
  std::string manifest_path;  // the generated site manifest
  std::string sidecar_path;   // the plan's sidecar (the needs and the gaps)
  std::string facts_path;     // the facts of the plan (static no-return verdicts)
};

// One NSuffice per (need, site) and one NCallEnd per call without a return, with the counts by KZ_NSUFF and
// KZ_NCALLEND value.
struct SufficeResult {
  std::vector<NSuffice> needs;
  std::vector<NCallEnd> call_ends;
  std::map<uint8_t, uint32_t> by_state;
  std::map<uint8_t, uint32_t> by_cause;
};

// ---- lifetimes (NLIFE) ----------------------------------------------------------------------------------------------
// One NLife per generation, with the counts of each finding flag (KZ_NLIFE_FLAG value -> generations carrying it).
struct LifetimeResult {
  std::vector<NLife> lifetimes;
  std::map<uint32_t, uint32_t> by_flag;
};

// ---- order instances (NORDER, NORDER_OBS) ---------------------------------------------------------------------------
// The static order output of the run's build (the edges file of `kz-static order` and its text) and the facts hash it
// was computed from, which must be the run's build's (else the instances are refused as a build mismatch).
struct OrderInstanceInput {
  std::string edges_path;   // the edges TSV
  std::string order_path;   // the order text (rows and their static order values)
  std::string facts_sha256; // hex
};

// One NOrder per edge instance and one NOrderObs per (relation row, consumer actor), with the counts by
// KZ_NORDER_RESULT and KZ_NOBS value.  The observed order is kept apart from the static order: never merged.
struct OrderInstanceResult {
  std::vector<NOrder> instances;
  std::vector<NOrderObs> observed;
  std::map<uint8_t, uint32_t> by_result;
  std::map<uint8_t, uint32_t> by_observed;
};

}  // namespace kz::trace
