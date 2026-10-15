// finalize.hpp -- the whole-program phase of the extraction (no Clang types).
//
// After every translation unit has been read, finalize() resolves what needs the whole closed
// program:
//   * the function table and the indices of calls, loops and located facts (the "sites");
//   * indirect calls and spawn entries, from the function-pointer flow graph;
//   * out-pointer summaries, and from them the writers instantiated at the calls that pass the
//     address of an extraction target, or the address-escape entries when the callee does not
//     keep to stores;
//   * the A-TYPE listing: which targets have an untyped view, and the untyped write sites that
//     then count;
//   * the walks over null-terminated tables (trip counts and the element objects' images);
//   * the early-initialization flags of writers, and the per-target summary rows.
// Every table is sorted by a key that does not depend on the order in which the units were read.
#pragma once

#include "model.hpp"
#include "spec.hpp"

namespace kzs {

void finalize(Model* model, const ExtractSpec& spec);

// The access path of a table walk, resolved against the images of the global objects.  Exposed
// for tests.  Returns false (with `why`) when the root image or the path is missing.
bool resolve_table_walk(const Model& model, const Model::WalkSpec& walk, std::vector<ImageFact>* elements, int64_t* trip, std::string* why);

// Solves the function-pointer flow graph: the functions each node may hold, and whether the
// node also has a source that cannot be followed.  Exposed for tests.
struct FlowSolution {
  std::map<std::string, std::set<std::string>> funcs;  // node -> function keys
  std::map<std::string, bool> unknown;                 // node -> has an unknown source
};
FlowSolution solve_flow(const std::vector<FlowEdge>& edges);

}  // namespace kzs
