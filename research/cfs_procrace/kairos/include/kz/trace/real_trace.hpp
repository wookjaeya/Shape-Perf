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
//   owners      ACTOR_OWNERS (schema revision 2) gives each thread's app_owner level and key; a thread
//               without a record there, and every thread of an earlier container, has an undetermined
//               owner: no owner line, so the replay reads it as unknown (ownership_undetermined), never
//               as a guess.  The creating executor is the thread that forked the thread (FORK, ptid).
//   constants   a run container carries no value of a `ref eval` constant: the caller supplies them.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/status.hpp"

namespace kz::trace {

struct RealTraceReport {
  uint64_t raw_events = 0;
  uint64_t events = 0;             // trace events made
  uint64_t unmapped_events = 0;    // events of sites that name no operation or point of the profile
  uint64_t unreadable_events = 0;  // events with TRUNC: kept without slot values
  std::vector<std::string> sites;  // per site: "<label>: call|return of OP", "<label>: point NAME" or "<label>: not mapped"
  uint64_t threads = 0;
  uint64_t owners_determined = 0;  // threads whose owner level is inferred or confirmed
  bool owner_section = false;      // the container has ACTOR_OWNERS
  bool runmeta = false;
  bool loss_present = false;
  uint64_t loss_intervals = 0;
  bool starts_at_exec = false;
};

// Normalized parameter / role name: lower case, without '_'.
std::string role_key(const std::string& name);

Status real_trace_from_run(const Container& run, const phi::Profile& p, eval::Trace* out, RealTraceReport* report);

// The text of a report (one item per line).
std::string report_text(const RealTraceReport& r);

}  // namespace kz::trace
