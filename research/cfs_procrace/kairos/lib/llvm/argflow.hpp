// argflow.hpp -- the whole-program part of the extended facts F4 and F6 (kz_facts.h revision 7; no Clang types).
//
// The walk (ast_fn.cpp) collects, with spec.extended_facts, the source of every call argument
// (Model::argflows), the uses of copies of each parameter in its function's body other than as a call
// argument (Model::param_uses), and the stores of global addresses to pointer fields with the order the
// writer's control-flow graph gives them (Model::ptr_writes).  finalize_flow_facts() then writes:
//
//   PARAM_FLOW     per function parameter, its uses of depth 0 (the ARGFLOW rows of a parameter source
//                  are its call arguments; the out-pointer summaries give the stores through it) and the
//                  uses it reaches through calls of functions with a body, composed bottom-up over the
//                  strongly connected components of the call graph, callees first, to a fixed point with
//                  the shortest number of calls; an indirect call contributes the composition of each
//                  resolved target (`may`).  Two bounds no reference fixes (ExtractSpec::argflow_depth,
//                  argflow_size; none by default) cut the composition; what they cut is an OPEN row;
//   PTR_FIELDS     the address writes (and static initializers) of each pointer field, the other typed
//                  writes of the field that remain, and every read through such a field resolved to the
//                  object when one address write holds it and precedes the read in program order;
//   GLOBAL_IMAGES  the static-image leaves of every global object an ARGFLOW row or a PTR_FIELDS row
//                  names, with what writes the object or the leaf's field.
#pragma once

#include "model.hpp"
#include "spec.hpp"

namespace kzs {

// Runs at the end of finalize() when spec.extended_facts is set (the call sites are numbered by then).
void finalize_flow_facts(Model* model, const ExtractSpec& spec);

}  // namespace kzs
