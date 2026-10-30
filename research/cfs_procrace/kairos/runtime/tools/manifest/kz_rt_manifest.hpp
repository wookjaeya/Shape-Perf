// kz_rt_manifest.hpp -- the site manifest "kzs-sites 3" (also reads "kzs-sites 1" and "2"; host side;
// event_semantics.md section 3).
//
// One text format, line oriented ('#' starts a comment outside quotes; tokens are separated by white
// space; a double-quoted part of a token may contain white space and '#'; the quotes are removed).
// Several files may be read into one Manifest (the logical sites written from the profile, then a
// backend's binding list, then identity reads); every file starts with the header line, and each file may
// have any version.  Version 3 adds only identity readers (below); a file of version 1 or 2 reads as before.
//
//   kzs-sites 1|2|3
//   source <name> rev=<revision> [ref=<text>]
//   module <id> name=<text> source=<source-name>
//   site <id> label=<label> op=<operation> phase=before|effect|after|exit|point tu=<path> func=<function>
//        [callee=<function>] [line=<n>] [place=at-call|after-call|line-start|store] [ref=<file:line at the source revision>]
//   value <site-id> <slot 0..7> role=<role> from=arg<k>|ret|out<k>|operand<k>|param<k> type=u8|u16|u32|u64|s8|s16|s32|s64|ptr
//        [string=pointer-only|validated] [bound=<1..64>] [ref=<text>]
//   control <site-id> ctx=task|lock-held|isr|osal-internal|terminating|unsupported [ref=<text>] [note=<text>]
//   bind <id> site=<site-id> backend=<name> module=<module-id> at=<location text>
//        status=bound|inlined|duplicated|removed|unsupported [vals=<mask>] [reason=<text>]
//   equiv <site-id> backends=<a>,<b> status=unconfirmed|confirmed [ref=<text>]
//   identity <site-id> decoder=<reader> [id=<slot>] [owner=<slot>] [status=<slot> ok=<integer>] [ref=<text>]
//                                                                                         (version 2 and later)
//
// Ids are decimal 1..2147483647, unique per statement kind; a site label is unique.  string=validated
// needs type=ptr, a bound and a ref (where the original code validated the pointer before this point);
// without it a pointer value is recorded as a pointer only.  status=removed|unsupported needs a reason
// (no hook is inserted there; the point is a declared coverage gap).  vals (default: every declared slot)
// may name only declared slots; it is the valid mask the binding passes to the hook.
//
// place (kzs-sites 2): the statement a point, effect or exit site is bound to, in func= (never a call or return of
// the site's own function, so the event is a point, not the call of an operation):
//   at-call     right before each call of callee= (line= restricts it to the calls on that line); arg<k> are the call's
//               argument operands
//   after-call  right after each such call; ret is its result, out<k> = *arg<k> read right after it, arg<k> as above
//   line-start  before the first statement of each run of statements of line= in a basic block (needs line=)
//   store       right after each store statement of line= (a field, global or pointer target; needs line=):
//               operand0 = the stored value, operand1 = the address stored to
// Without place, phase=before|after are the entry/returns of func (no callee) or the call/return of callee at its call
// site, and phase=effect without callee is a store through a pointer parameter of func (kzs-sites 1 meaning).
// from=param<k> (kzs-sites 2): the value parameter k of func= received at its entry (the parameter's value, not its
// memory read again), at any binding of the site.
//
// identity (kzs-sites 2): what the site's event says about task identity, with the meaning of the F0 plan's
// `decoder=` (kz_raw.h KZ_DECODER; the reader names are the same), for the host's identity join
// (kzrt_convert.hpp).  At most one per site, after the values it names.  The slot is named explicitly (never
// found by role or name) and must be a declared integer value of at most 4 bytes:
//   spawn-create    phase=before  the call that creates a task opens (no slot)
//   spawn-return    phase=after   its return; [id= status= ok=] the id the call assigned, read after the call
//                                 (from=out<k>|ret), used only where the status slot (any declared integer) holds
//                                 ok (the value of success, decimal or 0x hex, compared in the slot's width): a
//                                 failed call leaves an out parameter with its old content
//   spawn-id        phase=before|point|effect, inside an open spawn-create call of the same actor:
//                                 id= the id the call assigns and passes to the new task
//   thread-entry    phase=before  child side: id= the id the task announces at its entry
//   owner-request   phase=before  a request to start a task for an application named by an argument:
//                                 owner= the identity of that application (e.g. the parent application id)
//   child-request   phase=before  a request to start a task for the requesting task's own application (no slot)
//   request-return  phase=after   the return of any request (no slot; version 3: [status= ok=] the request's status
//                                 and the value of success, as for spawn-return; used for an app-request only)
// Version 3 (2026-10-29, E-ID1 of research/cfs_procrace/plan/c5_execution_basis_20261028.md section 10.4): a request
// that creates an application and hands its identity value back only after the task is created:
//   app-request     phase=before  a request that creates the application whose value it publishes later (no slot)
//   owner-publish   phase=point|effect  owner= the application's value as the open app-request hands it to its
//                                 caller (e.g. the store into the caller's out parameter); never a table whose
//                                 ordering is analysed
//   owner-check     phase=point|effect  owner= another value of the same request instance that must agree with
//                                 the published one (a cross-check only: it never names an owner, a disagreement
//                                 withdraws it)
// A closing record (spawn-return, request-return) is paired with its opening record by the event's `link`
// (the aseq of the opening record of the same call instance): the binding must give the opening identity
// site's record as the link.  An owner-publish or owner-check record is paired with its app-request the same way:
// the binding must give the record that opened the function instance (the app-request site's) as the link.
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace kzrt {

struct Source {
  std::string name, rev, ref;
  int line = 0;
};

struct Module {
  uint32_t id = 0;
  std::string name, source;
  int line = 0;
};

struct Value {
  uint32_t slot = 0;
  std::string role, from, type;
  std::string string_mode = "pointer-only";  // for type=ptr: pointer-only | validated
  uint32_t bound = 0;
  std::string ref;
  int line = 0;
};

// An identity read (kzs-sites 2; app-request, owner-publish, owner-check and request-return status kzs-sites 3).
// Slots are -1 where not given.
struct Identity {
  std::string decoder;  // spawn-create | spawn-return | spawn-id | thread-entry | owner-request | child-request | request-return
                        // | app-request | owner-publish | owner-check
  int id_slot = -1, owner_slot = -1, status_slot = -1;
  int64_t ok = 0;  // spawn-return, request-return: the status value of success (with status_slot)
  std::string ref;
  int line = 0;
};

struct Site {
  uint32_t id = 0;
  std::string label, op, phase, tu, func, callee, ref;
  std::string place;  // kzs-sites 2: at-call | after-call | line-start | store, or empty
  uint32_t src_line = 0;
  std::vector<Value> values;
  bool has_control = false;
  std::string ctx, ctl_ref, ctl_note;
  bool has_identity = false;
  Identity identity;
  int line = 0;
  const Value* value(uint32_t slot) const;
  uint32_t declared_mask() const;
};

struct Bind {
  uint32_t id = 0, site = 0, module = 0;
  std::string backend, at, status, reason;
  uint32_t vals = 0;
  bool vals_given = false;
  int line = 0;
};

struct Equiv {
  uint32_t site = 0;
  std::string backends, status, ref;
  int line = 0;
};

struct Manifest {
  uint32_t version = 0;  // the highest header version of the files read (1, 2 or 3)
  std::vector<Source> sources;
  std::vector<Module> modules;
  std::vector<Site> sites;
  std::vector<Bind> binds;
  std::vector<Equiv> equivs;
  std::map<uint32_t, size_t> site_index, bind_index, module_index;
  std::map<std::string, size_t> label_index;

  const Site* site(uint32_t id) const;
  const Site* site_by_label(const std::string& label) const;
  const Bind* bind(uint32_t id) const;
  const Module* module(uint32_t id) const;
};

// Adds one file's statements to *m.  On error returns false and sets *error to "<name>:<line>: <message>".
bool parse_manifest(std::string_view text, const std::string& name, Manifest* m, std::string* error);

// True for the reader names of an identity statement (of any version).
bool identity_decoder_known(const std::string& name);
// The first header version that has the reader (2 or 3; 0 if unknown).
uint32_t identity_decoder_version(const std::string& name);

// KZ_RT_CTX_* for a ctx= word; 0 if unknown.
uint32_t ctx_code(const std::string& word);
const char* ctx_name(uint32_t code);

}  // namespace kzrt
