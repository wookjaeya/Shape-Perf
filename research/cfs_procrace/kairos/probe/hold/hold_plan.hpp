// hold_plan.hpp -- the control part of a case plan for `kz hold` (format "kzp-hold 1").
//
// A hold plan is read together with an observation profile (kzp-observe 1) and its basis: every
// gate and release names an observation site by label, and the site's resolved file offset and
// binary come from the basis.  The hold plan adds only the control wiring that the observer has
// no notion of (SPEC 4.13): which site is a gate (a point where a thread is held with a hardware
// execution breakpoint, text unmodified) and which is a release (an observed event whose arrival
// resumes the held thread), and the condition that decides which call of a shared API the gate or
// release applies to.  No framework name or line number lives here; those are in the profile and
// the basis.
//
// Grammar (line oriented; '#' starts a comment outside quotes; tokens separated by white space;
// a quoted token may contain white space):
//
//   kzp-hold 1                                       the first statement
//   gate <label> site=<observe-site-label> [when <cond>] [occurrence first|each]
//   release <label> site=<observe-site-label> releases <gate-label> [when <cond>] [occurrence first|each]
//
//   cond:  name <slot-role> "<text>"     the string at that slot (a fetch/register source) equals <text>
//          caller-in <binary-name>       the return address at the top of the stack (entry sites) is in
//                                        that declared binary's mapped range (the caller is that module)
//          (both may be given; all stated conditions must hold)
//
//   occurrence first (default) | each    first: only the first qualifying hit acts; each: every hit acts.
//
// The gate's and the release's site must be sites of the given observation profile.  A gate site is
// an `entry` or `point` site; a release site is any site whose address a thread reaches.  `name`
// reads the slot by its role (the profile's slot role), as a NUL-terminated string from the stopped
// thread's memory (the base register and offset of that slot).  `caller-in` reads the stack top
// (the slot with role "return_address", or the stack pointer) and tests the module it points into.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/status.hpp"

namespace kz::hold {

struct HoldCond {
  bool has_name = false;
  std::string name_role;   // the slot role whose string value must equal name_text
  std::string name_text;
  bool has_caller_in = false;
  std::string caller_binary;  // the declared binary the return address must be in
};

struct GateDecl {
  std::string label;
  std::string site_label;    // the observation site that gives the address
  HoldCond cond;
  bool occurrence_each = false;
};

struct ReleaseDecl {
  std::string label;
  std::string site_label;
  std::string releases;      // the gate label this release resumes
  HoldCond cond;
  bool occurrence_each = false;
};

struct HoldPlan {
  std::vector<GateDecl> gates;
  std::vector<ReleaseDecl> releases;
};

Status parse_hold_plan(std::string_view text, HoldPlan* out);

}  // namespace kz::hold
