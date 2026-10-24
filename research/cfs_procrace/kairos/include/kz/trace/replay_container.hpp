// replay_container.hpp -- the output container of a real-record replay (`kz replay --out`).
//
// Schema revision 2 sections (SPEC 2.3 "Formats added in revision 1.3", ADR-054-ADR-056; docs/schema.md
// section 10), one container of kind `verdicts` whose parent is the replayed run container:
//   SUMMARIES     one per occurrence, in the order of the replay result: the summary view with the execution
//                 relation; `cell` = KZ_NONE32 (a replay writes no verdict cell), `model` = 0, `adequacy` =
//                 the occurrence's OBS_ADEQUACY record, `identity_first`/`n_identities` its identities;
//   MODEL_IDS     one record: an observed result (sigma_E), no model and no schedule profile (empty ids,
//                 model level unset), the basis of the run (`basis_id`, hex) and the profile that interpreted
//                 it (`profile_id`, hex of phi::profile_hash); `scope_id` is empty (not established);
//   OBS_ADEQUACY  one per occurrence: what the record establishes for what its conditions read, the reasons,
//                 the start evidence, the checked interval in positions of the replayed trace, the number of
//                 loss intervals overlapping it; `probes` null and `unobserved` empty (not written yet);
//   IDENTITIES    per occurrence: the executing task of the use first (with the passed identifier), then the
//                 actor of every registration or publication the execution relation used, each with its
//                 four identity categories as far as the record states them;
//   ACTOR_OWNERS  copied from the run container; its `thread`, `creator_thread` and `creation_event` are
//                 indices into the parent run container's THREADS and RAW_EVENTS;
//   BASIS         the inputs of the replay the caller gave (constants, versions, options), policy `unset`:
//                 the container records the value, the caller's reference is not checked here.
// Nothing here is a verdict about the program beyond the replay result it copies; the container holds no
// time and no host information, so the same inputs give the same bytes.
#pragma once

#include <string>
#include <utility>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"
#include "kz/trace/real_trace.hpp"

namespace kz::trace {

struct ReplayContainerInput {
  const Container* run = nullptr;  // the replayed run container (parent, ACTOR_OWNERS); null: no parent
  Digest profile_sha256{};         // the profile that interpreted the record
  Digest tool_sha256{};
  std::vector<std::pair<std::string, std::string>> basis;  // replay inputs, name -> value
};

// `report` gives the thread identities (tid, generation, creating executor, owner evidence); a report
// without identities leaves the thread ids unknown (flags without exec-known / creator-known).
Status write_replay_container(const ReplayContainerInput& in, const eval::Trace& trace, const eval::Result& result, const RealTraceReport& report, Bytes* out);

}  // namespace kz::trace
