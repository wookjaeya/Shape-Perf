// spec.hpp -- what the extractor looks for: fields, operations and loop vocabulary.
//
// Nothing here names a framework.  The lists come from the profile (locations, bindings, store
// patterns, operation parameters and terminal declarations, spawn and poll rules) and from the
// command line.  Every entry remembers where it came from, and the entries that a person typed
// (command line, the built-in table of C library data functions) are counted as hand-entered in
// the report and recorded in the facts container with provenance tier `researcher`.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "kz/core/phi.hpp"
#include "model.hpp"

namespace kzs {

struct TargetField {
  FieldRef field;
  std::string origin;  // "profile loc NAME", "profile bind", "profile store pattern", "command line"
};

// A call whose parameters have roles (stamp / key / id / out), from an operation of the profile.
struct OpInfo {
  std::string symbol;
  std::vector<uint8_t> roles;  // KZ_ARG_ROLE_* per parameter
  std::vector<bool> out;       // parameter has the `out` role
  bool terminal = false;
  bool process_terminal = false;
  std::string origin;
};

// A libc-style function that writes through one pointer argument (summary, SPEC 4.2 step 2).
struct DataFn {
  std::string name;
  int dest = 0;                 // index of the destination argument
  int src = -1;                 // index of the source argument, or -1 (fill byte at `fill`)
  int fill = -1;                // memset: index of the fill value
  bool zero_fill = false;       // bzero: the fill value is zero without an argument
  uint8_t writer_kind = KZ_WRITER_KIND_MEMCPY;
  std::string origin;
};

struct DeclaredItem {
  std::string name, value, origin;
};

struct ExtractSpec {
  std::vector<TargetField> fields;
  std::map<std::string, OpInfo> ops;  // by callee name
  std::set<std::string> delay_ops, spawn_ops, lock_ops, unlock_ops, terminal_ops;
  std::set<std::string> poll_fns;     // functions that the profile names as poll loops
  std::map<std::string, DataFn> data_fns;
  std::vector<DeclaredItem> declared;  // hand-entered inputs, for the BASIS section and the report
  std::string profile_name, profile_revision;

  bool has_field(const FieldRef& f) const;
  std::set<std::string> field_types() const;
};

// Adds what the profile declares: locations, bindings, store patterns, operations, rules.
void spec_from_profile(const kz::phi::Profile& profile, ExtractSpec* spec);
// The built-in table of data functions (hand-entered, recorded as such).
void spec_add_builtin_data_fns(ExtractSpec* spec);
// "Type.field" -> FieldRef; false if malformed.
bool parse_field_ref(const std::string& text, FieldRef* out);

}  // namespace kzs
