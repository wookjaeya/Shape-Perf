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
  // Front-end arguments of this unit only, for the analysis (the basis recipe's front_end_diagnostic: -Wno-error=<name>
  // for a diagnostic the front end raises as an error by default and the build compiler does not), appended after the
  // arguments every unit gets.  Never part of the recorded command.
  std::vector<std::string> analysis_args;
};

// Reads the units one after the other, in the order given, into `model`.  Stops at the first
// unit that does not parse and says which.  The model still has to be finalized (finalize.hpp).
bool extract_units(const std::vector<UnitCommand>& units, const ExtractSpec& spec, const ReadOptions& options, Model* model, std::string* error);

// One error or warning of the front end in a unit (read_unit_diagnostics).
struct UnitDiagnostic {
  bool error = false;          // reported as an error (else as a warning)
  bool default_error = false;  // controlled by a warning option and an error by default (Clang's DefaultError)
  unsigned id = 0;             // the front end's diagnostic id (the same in two parses by one front end)
  std::string option;          // the <name> of the -W<name> that controls it; empty when none does
  std::string file;            // its expansion location, the path as the extraction records one
  uint32_t line = 0, column = 0;
  std::string message;
};

// The errors and warnings of one unit as extract_units reads it (the same command and arguments, the unit's
// analysis_args included, then `extra`), except that warnings are reported (no -w) and the number of errors is not
// limited (-ferror-limit=0): for the basis, which occurrences a recipe front_end_diagnostic lowers, and where.  A unit
// that does not parse is listed with its errors; false (and `error`) only when the front end did not run on it.
bool read_unit_diagnostics(const UnitCommand& unit, const ReadOptions& options, const std::vector<std::string>& extra, std::vector<UnitDiagnostic>* out,
                           std::string* error);

// The text of every `-D`, `-I`, `-std=` ... argument of a recorded command that matters to the
// meaning of the code, with the compiler, the output and the input removed.  Deterministic.
std::string normalized_flags(const std::vector<std::string>& command_line);
// The same as a list: the recorded command without the compiler, the output and the input.
std::vector<std::string> flag_list(const std::vector<std::string>& command_line);

// A build-compiler plugin option of a recorded command (-fplugin=<file>, -fplugin-arg-<name>-<key>[=<value>]): the
// commands of a derived, instrumented build carry them (SPEC 1.7 B').  The basis records them as the commands ran;
// the front end is given every command without them (front_end_command), since it cannot load a plugin of the build
// compiler.  That the program it reads is unchanged holds for a plugin that changes neither preprocessing nor parsing
// (the basis states this as a deviation; kzgcc registers passes and observing callbacks only).
bool is_compiler_plugin_option(const std::string& arg);
std::vector<std::string> front_end_command(const std::vector<std::string>& command_line);

}  // namespace kzs
