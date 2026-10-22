// command.hpp -- how a sub-command of `kz` is added.
//
// A command is one file, cmd_<name>.cpp, that defines `int run(int argc, char** argv)` and
// registers it with a static KZ_COMMAND(name, summary, run).  The CMake file compiles every
// cmd_*.cpp into the executable, so adding a command touches no shared source file.  Commands
// that need a library add it to KZ_CLI_LIBS in CMakeLists.txt.
#pragma once

namespace kzcli {

struct Command {
  const char* name;
  const char* summary;
  int (*run)(int argc, char** argv);
};

// Registers a command; returns true (so that it can initialise a static).
bool register_command(const Command& c);

}  // namespace kzcli

#define KZ_COMMAND(NAME, SUMMARY, FN) \
  static const bool kz_command_registered_##NAME = ::kzcli::register_command(::kzcli::Command{#NAME, SUMMARY, FN})
