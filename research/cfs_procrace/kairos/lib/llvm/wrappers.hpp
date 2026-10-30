// wrappers.hpp -- the profile-free mode of kz-static (`basis --no-profile`, `extract --no-profile`).
//
// The mode reads no profile and takes no vocabulary from the command line.  It loads one fixed
// built-in profile (the two statements the profile loader needs at least, no vocabulary) and takes
// everything else from the build (the basis), the P0 table of library primitives (p0.hpp), the
// loader semantics of the recipe's startup-script block (as the basis records it) and the rules
// below.  The extraction runs twice over the same units:
//
//   1. a first pass with the built-in profile that also records, per function body, the control-flow
//      graph and, per direct call, the function-pointer arguments (spec.collect_wrapper_inputs);
//   2. the derivation (derive_wrappers) over the finalized first pass:
//      F8 DELAY-W / TERM-W  the functions that reach a delay or a terminal primitive of P0, to a
//                           fixed point over the strongly connected components of the call graph,
//                           callees first; per function `must` (every path), `noreturn` (terminal:
//                           no path returns, some path terminates) or `may` (some path, under the
//                           guards of its calls), with the call chain;
//      F7 SPAWN-W           the spawn operations, from the thread-creation primitives of P0 to a
//                           fixed point: a parameter that flows to the entry argument of a spawn
//                           call, or (stored in a field) to an indirect call in the code the
//                           spawned entry reaches, also through a function that makes a spawn call
//                           with a fixed entry (one level; such a function is reported but is no
//                           spawn operation); and, for a spawn whose entry is data converted to a
//                           function pointer in a function that reaches the symbol-lookup
//                           primitive, the entries the startup script names;
//   3. a second pass with the derived operations as the vocabulary, in place of a profile's delay,
//      terminal and spawn operations (and the P0 mutex primitives as lock operations): its call,
//      loop, target, spawn-parameter and terminal facts follow from them.
//
// The facts of the second pass are written with three more sections (P0_TABLE, OP_DERIV,
// SPAWN_WRAP; kz_facts.h revision 5) and the extended facts F1-F3 (READS, RETURNS, RET_CONSTS,
// LOOP_EXITS; revision 6), and record the built-in profile and the mode.  No Clang types.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "extract.hpp"
#include "kz/core/container.hpp"
#include "kz/core/phi.hpp"
#include "model.hpp"
#include "spec.hpp"

namespace kzs {

// The fixed built-in profile of the mode, and the revision of the derivation rules.
const char* builtin_profile_text();
kz::Status load_builtin_profile(kz::phi::Profile* out);
const char* wrapper_rules_revision();

// The P0 entries other than the data functions, as hand-entered inputs of the report and the
// BASIS section (the data functions are added by spec_add_builtin_data_fns).
void spec_add_p0_entries(ExtractSpec* spec);

// One line of the startup script as the basis records it (startup.entry[k] = "<kind> <file>, <symbol>, ...").
struct StartupEntry {
  uint32_t index = 0;
  std::string kind;    // "app", "library": the loader semantics of the recipe
  std::string symbol;  // the second field: the entry symbol
  std::string text;    // the recorded value
};
std::vector<StartupEntry> startup_entries_from_basis(const kz::Container& basis);

struct Derivation {
  // the vocabulary of the second pass (function keys)
  std::set<std::string> delay_keys, terminal_keys, process_terminal_keys, conditional_terminal_keys;
  std::set<std::string> spawn_keys, lock_keys, unlock_keys;
  std::map<std::string, uint32_t> spawn_entry_param;
  std::map<std::string, std::set<std::string>> spawn_script_entries;  // spawn call key -> entries
  // the rows; the guards of OP_DERIV are filled from the second pass
  std::vector<P0Fact> p0;
  std::vector<OpDerivFact> ops;
  std::vector<SpawnWrapFact> spawns;
  // for the report
  uint32_t functions_with_body = 0, cfg_built = 0, sccs = 0, sccs_recursive = 0;
  uint32_t delay_must = 0, delay_may = 0, term_must = 0, term_noreturn = 0, term_may = 0;
  uint32_t spawn_entry_arg = 0, spawn_called_by_entry = 0, spawn_fixed = 0;
  uint32_t script_calls = 0, script_rows = 0, script_in_closure = 0;
  // functions that call a fixed-entry spawn operation and are no spawn operation themselves: the
  // fixed-entry rule does not carry over to them (one level)
  uint32_t fixed_callers_not_followed = 0;
  // functions that are `may` delay operations: not in the vocabulary (only `must` is); loops whose
  // own body calls one, which would change kind if they were
  uint32_t loops_calling_may_delay = 0;
};

// Derives the operations from a model read with spec.collect_wrapper_inputs and finalized.
void derive_wrappers(const Model& first, const std::vector<StartupEntry>& script, Derivation* out);
// The vocabulary of a derivation, added to the spec of the second pass.
void apply_derivation(const Derivation& d, ExtractSpec* spec);

// The whole mode over the units: the first pass, the derivation, the second pass.  `base` is the
// spec of the built-in profile with the data functions and the P0 entries.  `model` receives the
// finalized second pass with the derived rows, `spec` its vocabulary.
bool extract_profile_free(const std::vector<UnitCommand>& units, const ExtractSpec& base, const ReadOptions& options, const std::vector<StartupEntry>& script,
                          Model* model, ExtractSpec* spec, Derivation* derivation, std::string* error);

}  // namespace kzs
