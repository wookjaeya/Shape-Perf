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
//                 loss intervals overlapping it; `probes` the list in REF_LISTS of the parent's SITES the
//                 conditions rest on (eval::Occurrence::probes mapped to sites through the adapter's site map;
//                 every site of a run container is attached for the whole record and was active, the observer
//                 fails the run otherwise), null when there is none or no parent; `unobserved` the names of
//                 eval::Occurrence::unobserved, ';'-separated (empty: nothing found missing);
//   REF_LISTS     the probe lists: each a run of references to the parent's SITES ended by a null reference,
//                 equal lists written once;
//   IDENTITIES    per occurrence: the executing task of the use first (with the passed identifier), then the
//                 actor of every registration or publication the execution relation used, each with its
//                 four identity categories as far as the record states them;
//   ACTOR_OWNERS  copied from the run container; its `thread`, `creator_thread` and `creation_event` are
//                 indices into the parent run container's THREADS and RAW_EVENTS;
//   BASIS         the inputs of the replay the caller gave (constants, versions, options), policy `unset`:
//                 the container records the value, the caller's reference is not checked here;
//   OCC_BASIS     (schema revision 4, kz_replay.h) one per occurrence, in the order of SUMMARIES: the verdict and summary
//                 view, every basis kind (store-derived-order and control-order too, which summary.flags has no room
//                 for), the declared assumptions, the outcome check, the path-independent key (rspec, operation, call
//                 context, owning application, resource key, text arguments, ordinal) and its run of OCC_EDGES;
//   OCC_EDGES     the edges between actors the occurrence's result rests on (eval::Occurrence::support into
//                 eval::Result::order_edges): rule, kind, endpoints in the replayed trace;
//   REPLAY_READ   one record: every option of the reading (ReplayContainerInput::options; unset without it), the
//                 control edges of the trace and the events its linearization moved.
// Nothing here is a verdict about the program beyond the replay result it copies; the container holds no
// time and no host information, so the same inputs give the same bytes.  docs/replay_output.md describes the reading
// of these sections and of a container written before them.
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
  const eval::Options* options = nullptr;  // the reading in force (REPLAY_READ); null: recorded as unset
};

// `report` gives the thread identities (tid, generation, creating executor, owner evidence); a report
// without identities leaves the thread ids unknown (flags without exec-known / creator-known).
Status write_replay_container(const ReplayContainerInput& in, const eval::Trace& trace, const eval::Result& result, const RealTraceReport& report, Bytes* out);

}  // namespace kz::trace
