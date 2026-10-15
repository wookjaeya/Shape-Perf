// extract.hpp -- reading translation units with Clang: the public entry of the AST-level
// extraction.  This header names no Clang type, so the callers (the command line, the tests)
// can include it without LLVM headers.
#pragma once

#include <string>
#include <vector>

#include "model.hpp"
#include "spec.hpp"

namespace kzs {

struct ReadOptions {
  std::string root;           // recorded paths are relative to it when they lie under it
  std::string resource_dir;   // Clang's builtin headers (-resource-dir)
  std::vector<std::string> extra_args;  // appended to every command (analysis-only adaptations)
  bool verbose = false;
};

// One translation unit as the build recorded it.
struct UnitCommand {
  std::string directory;                  // working directory of the recorded command
  std::string source;                     // absolute path of the main file
  std::string object;                     // the object file it produces (for the report)
  std::vector<std::string> command_line;  // argv of the recorded command
};

// Reads the units one after the other, in the order given, into `model`.  Stops at the first
// unit that does not parse and says which.  The model still has to be finalized (finalize.hpp).
bool extract_units(const std::vector<UnitCommand>& units, const ExtractSpec& spec, const ReadOptions& options, Model* model, std::string* error);

// The text of every `-D`, `-I`, `-std=` ... argument of a recorded command that matters to the
// meaning of the code, with the compiler, the output and the input removed.  Deterministic.
std::string normalized_flags(const std::vector<std::string>& command_line);
// The same as a list: the recorded command without the compiler, the output and the input.
std::vector<std::string> flag_list(const std::vector<std::string>& command_line);

}  // namespace kzs
