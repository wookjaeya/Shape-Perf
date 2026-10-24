// profile.hpp -- the observation part of a profile (.kzp, "kzp-observe 1"): which sites to probe,
// where each is found by name, and what its four raw values mean.  Framework knowledge enters
// the observer only through such a file, with the provenance of every entry.
//
// Grammar (line oriented; '#' starts a comment outside quotes; tokens are separated by white
// space; a quoted token "..." may contain white space, \" and \\):
//
//   kzp-observe 1                                  first statement
//   binary <name>                                  the file name of the traced binary (a check, not a path);
//                                                  repeated for a plan over several files: the first is the
//                                                  primary (the executable), the others are files the process
//                                                  maps (shared objects it loads), each with its own basis
//   prov <id> tier=<tier> kind=<kind> [side=<side>] file="<path>" lines=<from>-<to> cite="<text>" [authored="<text>"]
//             [root=<name>] [sha256=<64 hex digits>]
//   site <label> <entry|ret|point> <target> [decoder=<name>] [prov=<id>] [s0=<slot> .. s3=<slot>]
//
//   target:  func=<symbol> [in=<file suffix>]                 for entry and ret
//            at=<file suffix>:<line> after-call=<symbol>      for point: the instruction after the call
//                                                             that ends the line's code (a PLT stub
//                                                             of the symbol counts as the symbol)
//            at=<file suffix>:<line> line-start               for point: the first instruction of the
//                                                             line's code, before the statement runs
//            func=<symbol> after-call=<symbol>                for point, without a line table: the
//                                                             instruction after the one call of the
//                                                             callee inside the function (PLT stub
//                                                             included); refused if the function makes
//                                                             no such call or more than one, or if a
//                                                             branch of the function reaches the return
//                                                             site (the same rules as at=...after-call)
//            func=<symbol> enter=<G>[><H>...]               for point: the entry of the last function of
//                                                             the chain, reached from func= through one
//                                                             direct call or jump (a tail jump: jmp or a
//                                                             conditional jump) per hop, each the only
//                                                             transfer to that callee in its caller; a
//                                                             hop through a PLT stub is refused (the callee
//                                                             is in another file).  For a callee reached by
//                                                             a tail jump, where an after-call point never
//                                                             fires.  The events are attributed to an open
//                                                             call of func= by the return address at the
//                                                             top of the stack (slot 3, return_address):
//                                                             the plan must probe func='s entry and return
//   bin=<name>  (any site) the declared binary the site is in; default the primary
//   slot:    <src>:<width>:<role>      src = a register name | stack+<byte offset> | ip
//                                                | +<offset>(<register>) | -<offset>(<register>)
//            width = u8 | u16 | u32 | u64 | string (string: a memory source only)
//            A memory source is read by the kernel's probe-event fetch arguments (uprobe_events: the
//            integer, or the NUL-terminated string, at register + offset when the probe fires); the
//            observer then needs tracefs (kz observe --tracefs).
//   decoder: generic (default) | spawn-create | spawn-return | thread-entry | owner-request |
//            child-request | request-return | spawn-id   (kz_raw.h, KZ_DECODER)
//
// Tiers and kinds are the names of the schema (contract-backed, admitted, code-derived,
// issue-derived, researcher, unreferenced; api_doc, requirement, code_admission, code_pattern,
// issue, researcher).  A site without prov= has tier `unreferenced`; the reports count tiers.
// `sha256` is the hash of the cited lines (each with its newline), the value that
// `sed -n 'A,Bp' file | sha256sum` prints: the anchor tool recomputes it from the source root and
// refuses a basis whose cited text differs from the declaration.  `root` names which source root
// the file is relative to (the default root is --source-root; others are --root name=dir).
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "kz/core/sha256.hpp"
#include "kz/core/status.hpp"
#include "kz/schema/kz_schema.h"
#include "kz/trace/decode_sites.hpp"

namespace kz::plan {

struct ProvDecl {
  std::string id;
  uint8_t tier = KZ_TIER_UNREFERENCED;
  uint8_t kind = KZ_PROV_KIND_NONE;
  uint8_t side = KZ_PROV_SIDE_NA;
  std::string file, cite, authored;
  uint32_t line_from = 0, line_to = 0;
  std::string root;          // which source root `file` is relative to ("" = the default root)
  bool has_sha256 = false;   // the profile declares the hash of the cited lines
  Digest sha256 = {};
  uint32_t source_line = 0;  // line of the declaration in the profile
};

struct ProfileSite {
  std::string label;
  uint16_t kind = 0;              // KZ_SITE_KIND_*
  std::string func, in_suffix;    // entry, ret
  std::string at_file;            // point
  uint32_t at_line = 0;
  std::string after_call;
  bool line_start = false;        // point: the first instruction of the line (KZ_TARGET_KIND_LINE_START)
  std::vector<std::string> enter; // point: the chain of callees from func= (KZ_TARGET_KIND_ENTRY_REACHED)
  uint16_t decoder = KZ_DECODER_GENERIC;
  int prov = -1;                  // index into Profile::provs, or -1
  std::vector<kz::trace::SlotSpec> slots;
  uint32_t source_line = 0;
  uint16_t bin = 0;               // index into Profile::binaries (0 = the primary)
  // A point site with func= and after-call= (no at=): the call is found in the function's code, not
  // through the line table.
  bool function_scoped() const { return kind == KZ_SITE_KIND_POINT_PROBE && at_file.empty() && !func.empty() && enter.empty(); }
  // A point at the entry of a function reached from func= (enter=).
  bool entry_reached() const { return kind == KZ_SITE_KIND_POINT_PROBE && at_file.empty() && !func.empty() && !enter.empty(); }
};

struct Profile {
  std::string binary_name;              // the primary binary (= binaries[0])
  std::vector<std::string> binaries;    // every declared binary, in declaration order
  std::vector<ProvDecl> provs;
  std::vector<ProfileSite> sites;
  Digest sha256 = {};             // of the profile text as read
};

Status parse_profile(std::string_view text, Profile* out);

}  // namespace kz::plan
