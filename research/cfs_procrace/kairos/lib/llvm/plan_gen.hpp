// plan_gen.hpp -- `kz-static plan`: an observation plan derived by rule from one facts container.
//
// The generator takes the rows of `kz-static relations` (computed in-process by relations_rows, over the facts loader
// RelFacts that the relations share) and plans the method's observation set: every row, in the order of the method's
// priority field, under a recorded budget.  The priority is a port of the rule of the method document (label from the
// row text, order class, reach mark, one disposition per row; docs/plan_manifest.md §2): tier 1 = the priority directed
// candidates (disposition D07: label R or J, a counterpart, order class RELOC), tier 2 = the other directed candidates
// (order class RELOC), tier 3 = every other row.  The budget cuts the order after a tier or a number of rows; the rows
// cut are listed by their use-site key.
//
// For each planned row the observation rules (docs/plan_manifest.md §3) derive sites from the row and the facts:
//   use-call        at-call of the row's use (its arguments) and after-call (its result and the out values of the
//                   parameters the callee stores through);
//   key-text        the key string at a call the key parameter reaches in the key-parameter function under the program's
//                   own non-null test of that parameter (PARAM_FLOW guard), bounded by the program's own constant
//                   length check of the parameter there (a bounded NUL search, ISO C memchr / POSIX strnlen) or else by
//                   the runtime's copy bound; without such a test the key is recorded as a pointer and a gap is named;
//   creator-call    the same two rules at every creator of the row;
//   handling-chain  the returns along the row's handling lines, the stores and guard calls of its terminal line, and the
//                   at-call of the terminal call;
//   writer          path B: the writes of the shared field the row lists (stores, the calls that write it);
//   spawn-call      path C: the spawn call and the creator's writes after it;
//   actor           for every actor root of a planned row and every startup-script entry: the P0 thread-creation calls
//                   the SPAWN_WRAP chain of the root reaches (at-call: start routine and creation argument; after-call:
//                   result and the thread handle), the entry of each start routine with its first parameter, the entry of
//                   the root, and the stores of a parameter after a spawn of the chain in the spawn's function (the owner
//                   request); a creation argument that is one constant for several roots carries no tie (a gap);
//   stop-event      the entry of the run-loop and the app-exit API of the build's framework version, from the
//                   framework-layer data file (one entry per role per version, matched by the sha256 of the defining
//                   file and the definition line in the facts).
// Wait and lock results are not planned in this revision: each planned row names them as needs not planned.
//
// Output: a site manifest "kzs-sites 3" (the plugin kzgcc 0.3 and the runtime 1.4 read it: source, module, site, value
// statements; no identity, control or bind statements) whose comment header carries the sha256 of the sidecar; the
// sidecar (the plan header with every input file and its sha256, the generator identity, the rules and priority
// revisions, the budget with value, reason, truncation, unresolved and sensitivity, the bound parameters; one line per
// row with its tier and position; the use-site key to site map; need and gap lines; the module table; the executables
// to check; the stop-event table); the module table and the executable list for the derive script.  Every site has a
// label from the sorted site keys with its rule names and a ref to the facts it came from.  The plan states what to
// record, never what to expect.  The generator takes no field, callee or case name.  Deterministic.  No Clang types.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "relations.hpp"

namespace kzs {

const char* plan_rules_revision();
const char* plan_priority_revision();

// One input file of the generator: what it is, the path as given and the sha256 of the bytes read.
struct PlanInputFile {
  std::string role, path, sha256;
};

struct PlanOptions {
  std::string select;            // "observation-set" (the only selection of this revision)
  std::string order;             // "method-priority" (the only order of this revision)
  std::string budget;            // "tier:1" | "tier:2" | "all" | "rows:<n>"
  std::string budget_reason;     // recorded verbatim
  std::string budget_disclosure; // recorded verbatim
  std::string generator;         // the generator identity line (the tool, its identity hashes), given by the tool
  std::vector<PlanInputFile> inputs;  // every file the tool opened, in the order opened
  std::string sidecar_path;      // where the sidecar is written (the module table's first line names it)
};

// The method priority of one row (the port of the rule): label, order class, reach mark, disposition, why.
struct PlanPriority {
  std::string id;     // "A21" (the row's number in the relations text)
  std::string key;    // the row's use-site key
  std::string label, label_class, order_class, reach, disposition, why;
  uint32_t tier = 3;  // 1 priority directed candidate (D07), 2 other directed candidate (RELOC), 3 the rest
};

// The priority of every row of a report, in the report's order.
std::vector<PlanPriority> plan_priorities(const RelReport& report);

struct PlanOutput {
  std::string manifest, sidecar, modules, exes;
};

// facts: the facts container; basis: its basis container (nullptr for facts extracted without one); framework: the text
// of the framework-layer data file.  Errors: a bad option, a basis that is not the facts' basis, a data file line that
// is not an entry (the data file names no other file: the generator opens only the files named on its command line).
kz::Status plan_generate(const kz::Container& facts, const kz::Container* basis, const std::string& framework, const PlanOptions& options, PlanOutput* out);

// every reason the generator can write in a need or gap line, as reason and sub-reason (the reason vocabulary of the
// judgement rules, JUDGEMENT_RULES_REV1.md 3.5)
const std::vector<RelReason>& plan_reasons();

}  // namespace kzs
