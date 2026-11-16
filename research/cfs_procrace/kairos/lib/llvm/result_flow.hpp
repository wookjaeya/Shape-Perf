// result_flow.hpp -- result flow and effects (`kz-static extract --no-profile --result-flow`; facts sections RESULT_FLOW and
// RESULT_EFFECT, kz_facts.h revision 10; docs/static_facts.md 5u).
//
// The walk of each body (ast_fn.cpp) writes the rows; this file holds what is not tied to Clang: the values each function
// may return, built from the first pass of the profile-free mode (its RETURNS, followed through the results of the calls
// they return and through the parameters a return gives back unchanged, resolved by the arguments of each call), which
// the second pass uses for the outcome classes of every tested or returned call result.  No case or API name.
#pragma once

#include <memory>
#include <string>

#include "model.hpp"

namespace kzs {

// The revision of the rules of the option (BASIS extract.result_flow).
const char* result_flow_revision();

// The values each function of the first pass's model may return.
std::shared_ptr<const ResultValues> build_result_values(const Model& first);

}  // namespace kzs
