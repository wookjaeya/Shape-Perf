// rt_plan.hpp -- converts a "kzp-hold 1" plan (hold_plan.hpp) for one instrumented build into the runtime's plan
// (kz_rt_point[], serialized as "kzrt-plan 1", kairos/runtime/include/kz_rt_format.h): the host converter of the
// cooperative gate (SPEC rev 1.4 section 4.13, kairos/docs/event_semantics.md sections 4 and 11).
//
// Inputs: the plan, the logical site manifest the build's compiler binding read (kzs-sites 2) and the build's
// binding list (the merged per-unit binding files).  The plan's site labels are the F0 observation profile's labels;
// the manifest has a site of each label (its `equiv` line names the F0 site).  The meaning is that of kzp-hold 1
// (H1-H7): every point is one counter of qualifying hits of any thread; gates and releases keep the plan order.
// The conversion of a point:
//   - site: the manifest site of the label.  With `caller-in <module>` the label must be a function entry (F0 reads
//     the return address there); the point is then evaluated at the build's call-side site of that function in
//     that module (a site with callee=<func> place=at-call, right before the call) and only at its bindings in that
//     module (kz_rt_point.bind[]).  That these are all calls of the function in the module is the manifest's
//     statement (its refs), not checked here.  Several such sites are refused (one counter cannot span sites in
//     kz_rt_point).
//   - the evaluated site must have a `control` line (kz_rt_ctl is called only there).
//   - `name <role> "<text>"`: the slot of that role at the evaluated site, which must be string=validated with a
//     bound longer than the text (so equality is decidable from the copy); otherwise refused.
//   - occurrence first/<n>/each -> 1/n/0; unless-released -> KZ_RT_PT_UNLESS_RELEASED; no runtime timeout.
// What the conversion cannot express is refused (the plan is not converted), never approximated.  Where the
// runtime's realization differs from F0's (position of the point, string source, gate-busy for each gates, refused
// contexts, release evaluated after its site's observation record), the difference is listed per plan (versioned:
// "kzp-hold 1 on kzrt-plan 1").
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hold_plan.hpp"
#include "kz/core/status.hpp"
#include "kz_rt.h"
#include "kz_rt_manifest.hpp"

namespace kz::hold {

struct RtPoint {
  kz_rt_point p{};
  std::string name;            // the .kzh label
  std::string plan_site;       // the .kzh site label
  const kzrt::Site* site = nullptr;  // the evaluated site
  std::string mapping;         // how the point was mapped (one line, for the record)
};

struct RtPlan {
  std::vector<RtPoint> points;
  uint8_t manifest_sha[32] = {};  // sha256 of the logical manifest file
  uint8_t source_sha[32] = {};    // sha256 of the .kzh text
  uint8_t binds_sha[32] = {};     // sha256 of the binding list file
  std::vector<std::string> modules;      // "<id>=<name>" of the modules with bindings in this build
  std::vector<std::string> differences;  // the versioned differences from F0 that apply to this plan
};

// Converts `plan` (parsed from `plan_text`) for the build described by the logical manifest and the binding list
// (both already parsed into `m`; their file bytes give the sha256s).  Refuses (Err::Invalid, with the point and
// reason) what kzrt-plan 1 cannot express.
Status convert_rt_plan(const HoldPlan& plan, std::string_view plan_text, const kzrt::Manifest& m, std::string_view manifest_text,
                       std::string_view binds_text, RtPlan* out);

// The "kzrt-plan 1" bytes of a converted plan.
std::string encode_rt_plan(const RtPlan& plan);

// A text record of the conversion: inputs, every point and its mapping, the differences.
std::string rt_plan_record(const RtPlan& plan);

}  // namespace kz::hold
