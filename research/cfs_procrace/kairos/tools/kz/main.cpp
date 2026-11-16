// kz -- the command-line front end of Kairos' core (libkz-core and the libraries that do not need
// LLVM).  Sub-commands register themselves (command.hpp).
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "command.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"

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
  std::fprintf(f, "\n`kz <command> --help` prints the usage of a command; `kz --identity` the tool hash, the source identities and the\n"
               "schema revision of this build.\n");
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
  if (argc == 2 && std::strcmp(argv[1], "--identity") == 0) {
    // The tool hash this tool writes into the header of an artifact (kz/core/identity.hpp), the source identities it is
    // made of, and the schema revision and fingerprint of the records it reads and writes.
    std::printf("tool=kz\ntool_sha256=%s\ntool_source=%s\ncore_source=%s\nschema_revision=%u\nschema_fingerprint=%s\n",
                kz::to_hex(KZ_THIS_TOOL_HASH("kz")).c_str(), KZ_TOOL_SOURCE_SHA256, std::string(kz::core_source_identity()).c_str(),
                static_cast<unsigned>(KZ_SCHEMA_REVISION), kz::to_hex(kz::schema_fingerprint()).c_str());
    return 0;
  }
  for (const kzcli::Command& c : kzcli::commands())
    if (std::strcmp(c.name, argv[1]) == 0) return c.run(argc - 1, argv + 1);
  std::fprintf(stderr, "kz: unknown command '%s'\n\n", argv[1]);
  kzcli::usage(stderr);
  return 1;
}
