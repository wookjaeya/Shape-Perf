// real_trace.hpp -- from a run container (.kzt) to the real-record trace of the replay.
//
// The observer records probe events; the replay (kz/core/eval_trace.hpp) reads a trace of calls,
// returns and points of a profile.  This adapter maps one to the other and states only what the
// container establishes (SPEC 4.8 "Explanation set", ADR-055, ADR-056; docs/observer.md section 13):
//
//   events      an ENTRY site on the symbol an operation binds is a Call of it, a RET site a Return, a
//               POINT site whose label is a point name of the profile (an `OP.LABEL` linearization point,
//               a site, a terminal step) is that Point.  A slot whose role names a parameter of the
//               operation (compared without case and underscores) gives `arg.NAME` (`out.NAME` at a
//               return); at a return the role "status" or "result" gives the result; at a point a role
//               naming a value of the point gives `lp.NAME`, the role "value" gives `value`.  An event
//               with TRUNC set keeps no slot value.  Events of other sites are counted, not replayed.
//               The order is the container's merged order (time, CPU, arrival); the trace does NOT
//               declare it established, so the replay never takes it as the order of different actors.
//   actors      one per thread incarnation (THREADS): the kernel name, made unique with the tid and the
//               generation where needed; "tid<N>" for a thread without a name.
//   observed    the labels of the point sites that map: they were attached for the whole record.
//   start       when the record covers the process from its first execution mapping of the traced file
//               (a MAPS record no later than the first event), every kind and location of the profile has
//               a known start state (INIT at program entry): start_evidence init, start=known.  Writer and
//               alias coverage and lifetimes are NOT established by a run container (they need the
//               plan's tracing table and the closure, SPEC 4.7, 4.1): left uncovered / unknown.
//   loss        a reported loss (RUNMETA loss-present, LOSS) becomes a loss interval: a throttle interval
//               from its start; a lost-records report from the start of the record (the kernel does not
//               say when the lost records were written) up to the report; a loss without a record (stream
//               errors) the whole record.  A container without RUNMETA has no loss evidence: the whole
//               record is treated as lossy, never as loss-free.
//   owners      ACTOR_OWNERS (schema revision 2) gives each thread's app_owner level and the identity of
//               the owning application in the id space of the profile's owner kind (the value the
//               creation request passed).  The adapter maps it to the owner kind's record key with the
//               kind's id rule: valid iff (id - base) mod 2^32 <= serial_max, key = (id - base) mod 2^32
//               modulo the table size (the profile's `ids` block and `table`; the size is a `ref eval`
//               constant the caller gives).  A thread without a record there, a record without a key, an
//               id the rule rejects or a table size not given: no owner line, so the replay reads the
//               owner as unknown (ownership_undetermined), never as a guess and never as "no owner".  The
//               creating executor is the thread that forked the thread (FORK, ptid).
//   constants   a run container carries no value of a `ref eval` constant: the caller supplies them.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/status.hpp"

namespace kz::trace {

struct RealTraceOptions {
  std::map<std::string, int64_t> consts;  // values of `ref eval` constants (table sizes), by name
};

// One thread's identities as the container states them (for the report).
struct ThreadIdentity {
  std::string actor;
  std::string creator;     // the creating executor (FORK parent), empty if not recorded
  uint8_t level = KZ_OWNER_LEVEL_UNDETERMINED;
  uint32_t evidence = 0;   // set KZ_OWNER_EVIDENCE
  uint32_t owner_id = KZ_NONE32;  // identity value of the owning application, or KZ_NONE32
  bool key_resolved = false;
  int64_t key = 0;         // record key in the owner kind (when resolved)
  std::string app_name;
  uint32_t thread_flags = 0;
};

struct RealTraceReport {
  uint64_t raw_events = 0;
  uint64_t events = 0;             // trace events made
  uint64_t unmapped_events = 0;    // events of sites that name no operation or point of the profile
  uint64_t unreadable_events = 0;  // events with TRUNC: kept without slot values
  std::vector<std::string> sites;  // per site: "<label>: call|return of OP", "<label>: point NAME" or "<label>: not mapped"
  uint64_t threads = 0;
  uint64_t owners_determined = 0;  // threads whose owner level is inferred or confirmed and whose key the replay uses
  uint64_t owners_confirmed = 0, owners_inferred = 0, owners_undetermined = 0;  // levels as the container states them
  uint64_t owners_unresolved = 0;  // inferred/confirmed with an id the adapter could not map to a record key
  bool owner_section = false;      // the container has ACTOR_OWNERS
  std::vector<ThreadIdentity> identities;  // per thread (THREADS order)
  bool runmeta = false;
  bool loss_present = false;
  uint64_t loss_intervals = 0;
  bool starts_at_exec = false;
};

// Normalized parameter / role name: lower case, without '_'.
std::string role_key(const std::string& name);

// The record key in the profile's owner kind of an application identity `id` (the owners rule above):
// false if the profile has no owner kind with an id space and a table, the table size is not given, or
// the id is not valid in the id space.
bool owner_record_key(const phi::Profile& p, const std::map<std::string, int64_t>& consts, uint32_t id, int64_t* key);

Status real_trace_from_run(const Container& run, const phi::Profile& p, eval::Trace* out, RealTraceReport* report);
Status real_trace_from_run(const Container& run, const phi::Profile& p, const RealTraceOptions& opt, eval::Trace* out, RealTraceReport* report);

// The text of a report (one item per line).
std::string report_text(const RealTraceReport& r);

}  // namespace kz::trace
