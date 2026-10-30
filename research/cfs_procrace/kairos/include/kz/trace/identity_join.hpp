// identity_join.hpp -- the identity bootstrap of SPEC 5.7, offline.
//
// Nothing here ran in the traced process: the observer never calls the framework's own lookup
// (SPEC 5.7 first paragraph).  The join records three kinds of evidence and keeps which of them
// established each fact (the KZ_THREAD_FLAG bits).  Only ONE of them decides which creator-side
// call made a thread; the others are recorded, not cross-checked:
//
//   creation relation   the kernel's FORK record names the creating thread of every new thread
//                       (FORK_SEEN);
//   creator side        THE MATCHING KEY: an interval [spawn-create event, spawn-return event] on
//                       the creating thread contains the FORK of the thread it created.  This is
//                       containment of timestamps of one thread (the creator's probe events and
//                       the kernel's FORK record of the same thread), so a match holds under the
//                       clock assumption A-CLK-PO (SPEC 5.4); every matched or ambiguous record
//                       carries MATCH_BY_TIME.  It is never an order between threads: a FORK
//                       claimed by exactly one innermost interval is a match, a FORK that two
//                       intervals could claim, or an interval with two candidate FORKs, is
//                       reported as ambiguous and assigned to no one;
//   child side          the id the thread announces at its entry site, and the kernel thread
//                       name (COMM) the thread set for itself.  Both are attached by tid and
//                       recorded; neither is compared with the creator side.  SPEC 5.7 item 2
//                       names the spawn call's out-parameter (the id it assigns) as creator-side
//                       evidence; the perf-sample backend cannot dereference it, so the announced
//                       id is UNVERIFIED (flag ID_UNVERIFIED).  Checking it is deferred to a
//                       backend that reads memory at the spawn return.
//
// Creation side (W2, 2026-10-24; SPEC 1.2 "Creating application", 5.7 items 2 and 4, ADR-056), when
// the plan has the sites:
//   creation request    an OWNER_REQUEST (the application is an argument) or CHILD_REQUEST (the
//                       application is the requester's own) open on the creator thread when the
//                       spawn call begins; the outermost open one decides;
//   assigned id         a SPAWN_ID event inside the spawn call: the id the call assigns and passes to
//                       the new thread.  Equal to the id the child announces: ID_MATCHED, the link
//                       rests on the id (time no longer the only key); different: ID_MISMATCH, no link.
// From these the join derives app_owner per thread (OwnerRecord): `confirmed` when the link is by id,
// `inferred` when it is by FORK time alone (match-by-time, id-unverified), `undetermined` otherwise
// (no request, an ambiguous or contradicted link, a thread created without a probed spawn call, a
// child whose creator's owner is undetermined).  The owner key is the value the request passed, in
// the id space of the profile's owner kind; nothing is looked up in a framework table (EV-03).
//
// Events with TRUNC set are never used as keys or ids (kz_raw.h): they are counted
// (creates_unreadable, returns_unreadable, entries_unreadable).  A spawn site whose slots do not
// declare what the decoder reads is refused (check_site).
//
// The kernel thread name is the framework's task name truncated to the kernel limit; two tasks
// whose names share the truncated prefix are told apart by the announced id, not by the name.
// Names are not looked up by pointer here: the creator site's name pointer is kept raw (the
// perf-sample backend cannot dereference it).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"
#include "kz/trace/decode_merge.hpp"
#include "kz/trace/identity_symbols.hpp"

namespace kz::trace {

struct ThreadRecord {
  kz_thread rec;  // rec.name and rec.entry_symbol are not set: the strings are written when the container is built
  std::string name;
  std::string entry_symbol;
};

struct JoinStats {
  uint64_t threads = 0;
  uint64_t spawn_matched = 0;           // threads tied to exactly one creator-side event
  uint64_t spawn_ambiguous = 0;         // threads with more than one candidate
  uint64_t creates_without_fork = 0;    // creator-side intervals whose FORK was not seen
  uint64_t creates_without_return = 0;  // creator-side entry events with no matching return event
  uint64_t returns_without_create = 0;  // return events with no entry event
  uint64_t entries_unmatched = 0;       // threads that announced an id but have no matched creator event
  uint64_t duplicate_entries = 0;       // a second announcement by the same thread incarnation
  uint64_t threads_without_name = 0;
  uint64_t names_after_first_event = 0;
  uint64_t tid_reuses = 0;
  uint64_t events_without_thread = 0;   // raw events whose thread no side event ever named
  uint64_t creates_unreadable = 0;      // spawn-create events with TRUNC: no interval opened
  uint64_t returns_unreadable = 0;      // spawn-return events with TRUNC: the open call stays open
  uint64_t entries_unreadable = 0;      // thread-entry events with TRUNC: no id recorded
  uint64_t matched_by_time = 0;         // records whose creator link rests on timestamp containment
  uint64_t ids_unverified = 0;          // records with an announced id that nothing was compared with
  // Creation side (W2).
  uint64_t ids_matched = 0;             // threads whose announced id equals the id their spawn call assigned
  uint64_t ids_mismatched = 0;          // creator-side ids that contradict the thread the FORK time gave
  uint64_t spawn_ids_outside_create = 0;  // SPAWN_ID events with no spawn call open on their thread
  uint64_t spawn_ids_unreadable = 0;
  uint64_t spawn_ids_repeated = 0;      // spawn calls with more than one SPAWN_ID event (none of them used)
  uint64_t requests = 0;                // creation requests seen
  uint64_t requests_unreadable = 0;
  uint64_t requests_without_return = 0;
  uint64_t request_returns_without_request = 0;
  uint64_t owners_confirmed = 0, owners_inferred = 0, owners_undetermined = 0;
};

// app_owner of one thread (the record of section ACTOR_OWNERS; kz_raw.h).  rec.app_name is not set:
// the string is written when the container is built.
struct OwnerRecord {
  kz_app_owner rec;
  std::string app_name;
};

struct JoinInput {
  const std::vector<kz_raw_ev>* events = nullptr;
  const std::vector<SiteSpec>* sites = nullptr;
  const std::vector<SideEvent>* side = nullptr;
  const SymbolIndex* symbols = nullptr;  // optional: resolves the creator-side entry pointer
  bool bias_known = false;
  uint64_t load_bias = 0;
};

// One record per thread incarnation, ordered by (pid, tid, generation).  With `owners`, also one
// app_owner record per thread, in the same order (owners[i].rec.thread == i).
Status join_identities(const JoinInput& in, std::vector<ThreadRecord>* out, JoinStats* stats, std::vector<OwnerRecord>* owners = nullptr);

}  // namespace kz::trace
