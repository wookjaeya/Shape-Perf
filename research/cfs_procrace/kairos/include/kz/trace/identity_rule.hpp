// identity_rule.hpp -- the identity rules both execution families share (SPEC 1.2 "Creating application", 5.7,
// ADR-056; docs/event_semantics.md 2.3 and 3).
//
// The families differ in HOW a task is tied to the call that created it: F0's identity join (identity_join.hpp)
// has the kernel's fork record and the creating call's interval on the same thread, and, where the plan reads it,
// the id the call assigned; the instrumented family has no kernel creation record, only the values its hooks
// recorded.  From the tie on, one rule decides the owning application, and it is here:
//
//   owner_from_request    the creation request open on the creating actor when the spawn call began names the
//                         application: an owner request by its owner argument, a child request by the creating
//                         task's own app_owner (at most that task's level).  The level of the tie is `confirmed`
//                         when it rests on the id (the id the task announced equals the id the call assigned) and
//                         `inferred` when it rests on the fork time alone.  identity_join.cpp (F0) and
//                         join_actor_identities (below) both call it.
//
//   join_actor_identities the identity join of a neutral layer whose backend recorded no creation relation (the
//                         kzrt family, kzrt_convert.hpp), with the identity reads its site manifest declares
//                         (kzs-sites 2 `identity`, the meaning of F0's KZ_DECODER).  Per actor in aseq order (the
//                         target's program order; never an order between actors):
//       spawn-create      opens a spawn call on the actor; the outermost creation request open on the actor then
//                         is the call's request;
//       spawn-id          inside the innermost open spawn call of the same actor: the id the call assigns and
//                         passes to the new task (a second one in one call: neither is used);
//       spawn-return      closes the open spawn call its `link` names (the aseq of the opening record of the same
//                         call instance; calls opened after it and still open are abandoned).  Its id, the id the
//                         call assigned as read after the call, is used only where the call's status slot holds
//                         the value the manifest names as success (an out parameter keeps its old content when
//                         the call fails); it must agree with the call's spawn-id (a disagreement: no id);
//       owner-request / child-request / request-return   open and close creation requests the same way (by link);
//                         an owner request whose owner value was not observed names no application;
//       thread-entry      the id the actor announces (the first one; a later one is counted, not used).
//   The tie: an actor that announced id X is the task of the one spawn call that assigned X, when exactly one call
//   assigned X and exactly one actor announced it (and the two are different actors).  Then: creator = the calling
//   actor, creator_event = the call's opening event, spawn_return = its return event, join flags entry-seen,
//   spawn-matched, id-matched; the owner by owner_from_request at level `confirmed`.  Two calls or two actors with
//   the same id: spawn-ambiguous, no tie.  Nothing else ties an actor to a call: no time, no name, no order of
//   records between actors.  An actor without a tie has an undetermined owner (never "no owner", never 0).
//   The request's calling function is known where the request site is bound at the call (manifest `callee`): it is
//   the site's `func` (NActor.request_func).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"
#include "kz/trace/neutral.hpp"

namespace kz::trace {

// The creation request open on the creating actor when the spawn call began (the outermost one).
struct CreationRequest {
  bool child = false;                 // a child request (the requester's own application), else an owner request
  bool owner_readable = false;        // an owner request whose owner argument was observed
  uint32_t owner_id = KZ_NONE32;      // that argument
  bool nested_owner_request = false;  // an owner request ran inside this child request (its argument is not read)
};

// An owner as the rule states it: the identity value, the level (KZ_OWNER_LEVEL) and the evidence (KZ_OWNER_EVIDENCE).
struct OwnerDecision {
  uint32_t owner_key = KZ_NONE32;
  uint8_t level = KZ_OWNER_LEVEL_UNDETERMINED;
  uint32_t evidence = 0;
};

enum class OwnerSource { None, Request, Creator };

// Applies the creation request to *o (whose evidence the caller has filled with the tie's evidence).  `tie_level`
// is KZ_OWNER_LEVEL_CONFIRMED (tie by the id) or KZ_OWNER_LEVEL_INFERRED (by fork time alone).  `creator` is the
// creating actor's own decision, or nullptr where the creating actor is not known (or is the task itself); it is
// read only for a child request.  Returns where the owner came from (the caller copies a display name from there).
OwnerSource owner_from_request(const CreationRequest& rq, uint8_t tie_level, const OwnerDecision* creator, OwnerDecision* o);

// The identity read of one neutral site (from the manifest's `identity` statement), indexed like NeutralRun::sites.
struct IdentityRead {
  uint16_t decoder = KZ_DECODER_NONE;  // KZ_DECODER_*
  int id_slot = -1, owner_slot = -1;   // value slots (-1: none)
  int status_slot = -1;                // spawn-return: the call's status slot, with `ok` the value of success
  uint64_t ok = 0;
  uint8_t status_width = 8;            // bytes compared
  std::string request_func;            // the calling function where the site is bound at the call, else empty
};

struct ActorJoinStats {
  uint64_t identity_events = 0;           // events of identity sites
  uint64_t events_without_actor = 0;      // identity events of unresolved actors (no cell): not used
  uint64_t entries = 0, entries_unreadable = 0, duplicate_entries = 0;
  uint64_t creates = 0, creates_without_return = 0, returns_without_create = 0;
  uint64_t spawn_ids_outside_create = 0, spawn_ids_unreadable = 0, spawn_ids_repeated = 0;
  uint64_t out_ids_used = 0, out_ids_not_ok = 0, ids_conflicting = 0;
  uint64_t requests = 0, requests_without_return = 0, request_returns_without_request = 0;
  uint64_t ids_matched = 0, ambiguous_ids = 0, entries_unmatched = 0;
  uint64_t owners_confirmed = 0, owners_undetermined = 0;
};

// The join described above.  `reads` has one entry per NSITES index (decoder none where the site has no identity
// read).  Writes the actors' creator, creator_event, spawn_return, announced_id, join_flags, owner_* and
// request_func, the flag owner-recorded on every actor of the run (kzrt-actor, kzrt-provisional), and the run flag
// owners-recorded.  Unresolved actors (an event without a cell) take no part.
Status join_actor_identities(const std::vector<IdentityRead>& reads, NeutralRun* n, ActorJoinStats* stats);

// One line of counts, for the conversion notes.
std::string actor_join_text(const ActorJoinStats& s);

}  // namespace kz::trace
