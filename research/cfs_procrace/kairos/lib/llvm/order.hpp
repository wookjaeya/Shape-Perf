// order.hpp -- `kz-static order`: the order edges of a facts container, with their kinds and premises, and the static
// order of each relations row.
//
// The query reads one facts container (the facts `kz-static relations` reads) and writes, for the whole program:
//
//   edges  HB-spawn   a spawn call before the first event of each thread it starts (the thread-creation primitive's
//                     semantics), with the creation primitive the call reaches, the status condition (the constants of
//                     the spawn operation that are no failure) and the flag "child without success" (the operation may
//                     report a failure after the thread was created);
//          HB-seq     program order of one thread, in the forms these facts give it: an event before a spawn call of its
//                     function that cannot reach it afterwards (LOCK_PATHS after-spawn), an event inside a call, a thread's
//                     start before every event of its code, and the iterations of a loop that spawns from a table;
//          L0-script  the spawn calls of two startup-script lines in the order of the lines (layer L0: the loader reads
//                     the lines in order on one thread);
//          HB-lock    a write after a spawn before the child's read, when relations path C grades the other order
//                     proven-impossible (one static-instance lock held from one acquisition across the spawn and the
//                     write, and held by the read);
//          HB-await   a write of the field a recognised wait predicate reads, in another thread, before a use the waiting
//                     thread reaches after the wait;
//          kind       hard (every execution in which both instances occur has them in this order, under the premises),
//                     conditional (only when the condition holds), observed-only (no rule of this revision orders them; a
//                     run record may show an order: the start of two children of one creator);
//   waits  every poll and count-bounded loop: its exits split into condition and budget parts, the budget, the predicate,
//          the awaited locations and their writers in other threads, and per dependent use (the reads and calls after the
//          loop in its function, the calls of its function) what the budget exit does to it: skip, continue, no budget,
//          inside the wait, or unknown with the reason;
//   rows   per relations row (paths A, B and C, by use-site key): order.static -- hard_before / conditional_before (the
//          write or creation before the use), fixed_early_proved / fixed_early_candidate (the use before it), unordered,
//          unknown with the reason -- per pair of events and threads, the path of edges, the edges that start the threads,
//          and the relations row's feasibility of the other order.
//
// The closure of hard_before and fixed_early_proved follows hard edges only; a conditional edge makes a path conditional
// and never enters the hard closure.  No field, callee or case name is taken.  Deterministic.  No Clang types.
#pragma once

#include <string>

#include "kz/core/container.hpp"

namespace kzs {

const char* order_rules_revision();
const char* order_loop_lock_revision();  // order rules revision 4: what facts with LOOP_DEFS and LOCK_HANDLES add
const char* order_result_flow_revision();  // order rules revision 3: what facts with RESULT_FLOW and RESULT_EFFECT add

struct OrderOptions {
  // the measurement of the caller-dependent spawn rule (static_facts.md 5v): when set, the report is written there; the
  // order text and the edges are not changed by it
  std::string* caller_dependent_report = nullptr;
};

// the text (`text`) and one line per edge (`tsv`, with a header line)
kz::Status order_report(const kz::Container& facts, const OrderOptions& options, std::string* text, std::string* tsv);

}  // namespace kzs
