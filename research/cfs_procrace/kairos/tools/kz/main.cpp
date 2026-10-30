// kz -- the command-line front end of Kairos' core (libkz-core and the libraries that do not need
// LLVM).  Sub-commands register themselves (command.hpp).
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

#include "command.hpp"

namespace kzcli {

namespace {
std::vector<Command>& commands() {
  static std::vector<Command> v;
  return v;
}

void usage(FILE* f) {
  std::vector<Command> sorted = commands();
  std::sort(sorted.begin(), sorted.end(), [](const Command& a, const Command& b) { return std::strcmp(a.name, b.name) < 0; });
  std::fprintf(f, "usage: kz <command> [arguments]\n\ncommands:\n");
  for (const Command& c : sorted) std::fprintf(f, "  %-10s %s\n", c.name, c.summary);
  std::fprintf(f, "\n`kz <command> --help` prints the usage of a command.\n");
}
}  // namespace

bool register_command(const Command& c) {
  commands().push_back(c);
  return true;
}

}  // namespace kzcli

int main(int argc, char** argv) {
  if (argc < 2 || std::strcmp(argv[1], "--help") == 0 || std::strcmp(argv[1], "help") == 0 || std::strcmp(argv[1], "-h") == 0) {
    kzcli::usage(argc < 2 ? stderr : stdout);
    return argc < 2 ? 1 : 0;
  }
  for (const kzcli::Command& c : kzcli::commands())
    if (std::strcmp(c.name, argv[1]) == 0) return c.run(argc - 1, argv + 1);
  std::fprintf(stderr, "kz: unknown command '%s'\n\n", argv[1]);
  kzcli::usage(stderr);
  return 1;
}
