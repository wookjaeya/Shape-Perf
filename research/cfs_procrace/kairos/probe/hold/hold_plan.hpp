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
//   gate <label> site=<observe-site-label> [when <cond>] [occurrence first|each|<n>] [unless-released]
//   release <label> site=<observe-site-label> releases <gate-label> [when <cond>] [occurrence first|each|<n>]
//
//   cond:  name <slot-role> "<text>"     the string at that slot (a fetch/register source) equals <text>
//          caller-in <binary-name>       the return address at the top of the stack (entry sites) is in
//                                        that declared binary's mapped range (the caller is that module)
//          (both may be given; all stated conditions must hold)
//
//   occurrence first (default) | each | <n>
//                                        first: only the first qualifying hit acts; each: every hit acts;
//                                        <n> (a decimal 1..1000000): only the n-th qualifying hit acts ("first" is 1).
//                                        A qualifying hit is a hit of the point's address that meets its conditions,
//                                        by any thread, counted from the start of the run (one count per point).
//
//   unless-released (gates only, not with each): the gate does not hold a thread once one of its releases has
//                                        acted -- a release that acts while its gate holds no thread marks the gate
//                                        satisfied, since the order the gate enforces (its step after the release's
//                                        event) already holds.  Without it such a release is consumed and the gate
//                                        still holds its hit later (the thread is then held to the end of the run).
//
// Several points may share one site (one address): every point of that address is evaluated at each hit, the
// gates first (in plan order; the first gate that acts holds the thread, a further gate that acts on the same
// hit is consumed without a hold), then the releases; the releases of a hit whose thread a gate holds act when
// that thread is resumed (it passes the address then).  A release acts on its hit also when its gate does not
// hold a thread at that moment (it is then consumed).  A thread resumed by a release keeps its debug registers
// armed: its later hits act like any other thread's (a gate that already acted on it does not hold it again).
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
  uint32_t occurrence_nth = 1;  // the qualifying hit that acts (1 = first); unused with each
  bool unless_released = false; // a release of it that acted before its hit satisfies it (no hold)
};

struct ReleaseDecl {
  std::string label;
  std::string site_label;
  std::string releases;      // the gate label this release resumes
  HoldCond cond;
  bool occurrence_each = false;
  uint32_t occurrence_nth = 1;  // the qualifying hit that acts (1 = first); unused with each
};

struct HoldPlan {
  std::vector<GateDecl> gates;
  std::vector<ReleaseDecl> releases;
};

Status parse_hold_plan(std::string_view text, HoldPlan* out);

// The occurrence of a point as the plan text writes it: "each", "first" (n = 1) or the decimal n.
std::string occurrence_text(bool each, uint32_t nth);

}  // namespace kz::hold
