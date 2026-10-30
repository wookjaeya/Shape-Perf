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

  // ---- the profile-free mode (kz-static extract --no-profile; wrappers.hpp) ----
  // Empty in the profile-driven mode, where nothing below changes what is extracted.  The
  // operations are by function key (the name of a function with external linkage), so that two
  // functions of one name with internal linkage stay apart; a key of the P0 table is its name.
  bool no_profile = false;
  bool collect_wrapper_inputs = false;  // the first pass: control-flow skeletons, function-pointer arguments
  std::string builtin_profile_text;      // the fixed built-in profile the mode loads, for the record
  std::set<std::string> delay_keys, terminal_keys, process_terminal_keys, conditional_terminal_keys;
  std::set<std::string> spawn_keys, lock_keys, unlock_keys;
  std::map<std::string, uint32_t> spawn_entry_param;                   // spawn operation -> the parameter of the entry
  std::map<std::string, std::set<std::string>> spawn_script_entries;   // spawn call key -> entries named by the startup script
  std::set<std::string> guard_calls;                                   // call keys whose guards the derived rows report

  // ---- the extended facts F1-F3 (READS, RETURNS, RET_CONSTS, LOOP_EXITS; kz_facts.h revision 6) ----
  // Always on in the profile-free mode (its second pass), on request (--extended-facts) in the
  // profile-driven one.  Off, nothing is read for them and the facts are written as before.
  bool extended_facts = false;
  // The extended facts F4 and F6 (ARGFLOW, PARAM_FLOW, PTR_FIELDS, GLOBAL_IMAGES; revision 7) come with
  // them.  The composition of PARAM_FLOW has two bounds that no reference fixes (tool parameters,
  // recorded as unspecified_by_reference): the number of calls a composed row passes, and the number
  // of composed rows per parameter.  kNone: no bound.
  uint32_t argflow_depth = kNone;
  uint32_t argflow_size = kNone;
  // F5 and the target-independent rules (kz_facts.h revision 8) come with them too.  Their completeness
  // pass is an extraction of its own (the built-in profile, the fields of the record types the reads
  // name as targets) that keeps the raw A-TYPE hits; nothing else of it is written.
  bool keep_raw_atypes = false;

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
