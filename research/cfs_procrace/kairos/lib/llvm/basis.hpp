// basis.hpp -- the analysis basis B (SPEC 1.7): what the analysis of a closed program rests on.
//
// `build_basis` records, for the target of one link command:
//   Build   the commits of the source trees, the compiler and flags of every translation unit,
//           the generated files by hash (with the tables they hold), the link command and the
//           members it takes in;
//   Config  the platform constants evaluated in the unit of their use, the startup script the
//           reset type selects and its entries, and the units that script loads at run time
//           (recipe `module`): their link members join the program, and the binding of their
//           undefined symbols by the dynamic loader is recorded and compared with the binding by
//           name that the extraction of the merged program uses (SPEC 4.1 step 6);
//   Env     the front end that reads the program (an analysis-only substitute for the compiler
//           that built it, recorded as a deviation), and the identity of the built binary.
// Every item has a value, a policy status (CONDITIONS_POLICY) and, where a person entered it, a
// provenance record of tier `researcher`.  The result is a KZ container of kind BASIS; its
// artifact hash is the hash of B that every later artifact carries.
#pragma once

#include <string>
#include <vector>

#include "basis_clang.hpp"
#include "kz/core/container.hpp"
#include "kz/core/phi.hpp"

namespace kzs {

struct BasisInputs {
  std::string compile_db;     // compile_commands.json
  std::string build_dir;      // recipe paths (link command, generated files, binary) are relative to it
  std::string bundle_root;    // the source trees; recorded paths are relative to it
  std::string recipe_text;    // the recipe, if any
  std::string recipe_path;    // for the record
  std::string link_override;  // a link command file, instead of the recipe's
  ReadOptions read;           // resource dir and extra arguments of the front end
  kz::Digest profile_hash = {};
  kz::Digest tool_hash = {};
  std::string build_identity;
};

struct BasisReport {
  size_t cdb_entries = 0, link_inputs = 0, link_members = 0, link_included = 0, archives = 0;
  size_t lazy_not_pulled = 0, units = 0, unmatched_objects = 0;
  size_t revisions_checked = 0, revisions_matching = 0;
  size_t constants_ok = 0, constants_failed = 0, tables = 0, generated = 0;
  size_t hand_entered = 0;
  size_t startup_libraries = 0, startup_apps = 0;
  std::string startup_script;
  // the units the startup script loads, linked with the executable (recipe `module`, SPEC 4.1 step 6)
  bool modules_requested = false;
  size_t modules_in = 0, modules_outside = 0, module_units = 0;
  size_t bound_executable = 0, bound_library = 0, unbound = 0, across_units = 0;
  size_t binding_differs = 0, duplicate_definitions = 0;
  std::vector<std::string> warnings;
};

kz::Status build_basis(const BasisInputs& in, const kz::phi::Profile& profile, kz::Bytes* out, BasisReport* report);

// The translation units a basis records, as commands that can be parsed again.
kz::Status units_from_basis(const kz::Container& basis, std::vector<UnitCommand>* units, std::string* bundle_root);

// The text of the items of a basis container, one per line ("name = value  (policy)").
std::string basis_items_text(const kz::Container& basis);

}  // namespace kzs
