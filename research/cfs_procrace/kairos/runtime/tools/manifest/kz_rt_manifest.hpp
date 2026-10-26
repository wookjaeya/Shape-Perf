// kz_rt_manifest.hpp -- the site manifest "kzs-sites 1" (host side; event_semantics.md section 3).
//
// One text format, line oriented ('#' starts a comment outside quotes; tokens are separated by white
// space; a double-quoted part of a token may contain white space and '#'; the quotes are removed).
// Several files may be read into one Manifest (the logical sites written from the profile, then a
// backend's binding list); every file starts with the header line.
//
//   kzs-sites 1
//   source <name> rev=<revision> [ref=<text>]
//   module <id> name=<text> source=<source-name>
//   site <id> label=<label> op=<operation> phase=before|effect|after|exit|point tu=<path> func=<function>
//        [callee=<function>] [line=<n>] [ref=<file:line at the source revision>]
//   value <site-id> <slot 0..7> role=<role> from=arg<k>|ret|out<k>|operand<k> type=u8|u16|u32|u64|s8|s16|s32|s64|ptr
//        [string=pointer-only|validated] [bound=<1..64>] [ref=<text>]
//   control <site-id> ctx=task|lock-held|isr|osal-internal|terminating|unsupported [ref=<text>] [note=<text>]
//   bind <id> site=<site-id> backend=<name> module=<module-id> at=<location text>
//        status=bound|inlined|duplicated|removed|unsupported [vals=<mask>] [reason=<text>]
//   equiv <site-id> backends=<a>,<b> status=unconfirmed|confirmed [ref=<text>]
//
// Ids are decimal 1..2147483647, unique per statement kind; a site label is unique.  string=validated
// needs type=ptr, a bound and a ref (where the original code validated the pointer before this point);
// without it a pointer value is recorded as a pointer only.  status=removed|unsupported needs a reason
// (no hook is inserted there; the point is a declared coverage gap).  vals (default: every declared slot)
// may name only declared slots; it is the valid mask the binding passes to the hook.
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

struct Site {
  uint32_t id = 0;
  std::string label, op, phase, tu, func, callee, ref;
  uint32_t src_line = 0;
  std::vector<Value> values;
  bool has_control = false;
  std::string ctx, ctl_ref, ctl_note;
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

// KZ_RT_CTX_* for a ctx= word; 0 if unknown.
uint32_t ctx_code(const std::string& word);
const char* ctx_name(uint32_t code);

}  // namespace kzrt
