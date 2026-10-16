// unroll.hpp -- the parts of the certificate-view rules of SPEC 4.3 that need the whole program
// (no Clang types, so the tests run without Clang).
//
//   peval / truth          partial evaluation of an element-form expression (ELEM loads) against
//                          the static-initializer image of one table element
//   find_spawn_params      the parameters that carry the function a spawned task runs
//   unroll_table_walks     one row per (iteration, call) of every resolved table walk: is the call
//                          reached in this iteration, and with which element values
//   decide_loop_rules      per loop: unroll / await / star-closed, the L1 preconditions at the
//                          level of the facts, and why
#pragma once

#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "finalize.hpp"
#include "model.hpp"
#include "spec.hpp"

namespace kzs {

// The value of one field of a table element, from the static-initializer image.
struct ElemValue {
  bool known = false;
  uint8_t kind = KZ_IMAGE_KIND_NONE;  // INT, NULLPTR, FUNC, ADDR, STRING
  int64_t value = 0;
  std::string text;    // FUNC: name, ADDR: object, STRING: the string
  std::string fn_key;  // FUNC
  Pos pos;             // where the value is written
};
// Field name -> value; the empty name is the element pointer itself.
using ElemLookup = std::function<ElemValue(const std::string& field)>;

// Replaces the ELEM loads that the lookup knows by their values and folds what becomes
// constant: comparisons of constants, a known non-null pointer (a function, an object's address,
// a string) against 0, two functions or two objects by name, and the Boolean connectives with a
// known operand.  What is left does not depend on the known element fields.
ExprRef peval(ExprPool* pool, ExprRef e, const ElemLookup& lookup);
// 1 when `e` is known to hold (a non-zero constant, a function, an address, a string), 0 when it
// is known not to, -1 otherwise.
int truth(ExprRef e);

// Whole-program inputs the rules share (built by finalize).
struct ProgramGraph {
  std::map<std::string, std::set<std::string>> cg;                // caller key -> callee keys (direct and resolved indirect)
  std::map<std::string, std::set<std::string>> indirect_targets;  // call key -> function keys
  std::set<std::string> unresolved_calls;                         // call keys of indirect calls with an unknown target
  std::map<std::string, std::string> call_caller;                 // call key -> caller key
};

void find_spawn_params(Model* m, const ProgramGraph& g);
void unroll_table_walks(Model* m, const std::map<std::string, std::vector<ImageFact>>& entries);
void decide_loop_rules(Model* m, const ExtractSpec& spec, const ProgramGraph& g);

}  // namespace kzs
