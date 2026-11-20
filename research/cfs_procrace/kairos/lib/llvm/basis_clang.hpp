// basis_clang.hpp -- the Clang-based services of the analysis basis.  No Clang type appears here.
//
//   load_compile_db      the recorded commands (compile_commands.json), with Clang's own reader
//   evaluate_constants   configuration values, constant-evaluated in the translation unit of
//                        their use (SPEC 4.1 step 4): the unit's text is parsed with a probe
//                        declaration appended, so macros, enumerators and sizeof are folded by
//                        the compiler front end, not by this tool
//   read_unit_images     the evaluated static initializers of the global objects of one unit
//   clang_version_text   which Clang this tool is
#pragma once

#include <map>
#include <string>
#include <vector>

#include "extract.hpp"
#include "kz/core/status.hpp"

namespace kzs {

struct CdbEntry {
  std::string directory, file, output;
  std::vector<std::string> command_line;
};

kz::Status load_compile_db(const std::string& path, std::vector<CdbEntry>* out);

struct ConstQuery {
  std::string name;
  bool is_string = false;
};

struct ConstResult {
  std::string name;
  bool ok = false;
  bool is_string = false;
  int64_t value = 0;
  std::string text;
  std::string macro_file;   // where the name is defined as a macro, if it is one (bundle-relative)
  uint32_t macro_line = 0;
  std::string macro_body;   // the replacement text as written
  std::string error;
};

kz::Status evaluate_constants(const UnitCommand& unit, const ReadOptions& options, const std::vector<ConstQuery>& queries, std::vector<ConstResult>* out);

kz::Status read_unit_images(const UnitCommand& unit, const ReadOptions& options, std::map<std::string, std::vector<ImageFact>>* images);

std::string clang_version_text();

}  // namespace kzs
