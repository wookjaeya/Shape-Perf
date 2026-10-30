// kz-dump -- print a KZ container as canonical text.
//
//   kz-dump <file>            verify the container and print its canonical dump
//   kz-dump --no-verify <file>  print the dump of a container that parses but fails a content check
//   kz-dump --check <file>    verify only (structure, reserved fields, references); exit status
//   kz-dump --hash <file>     print the artifact hash (SHA-256 of the whole file)
//   kz-dump --layout          print the schema layout and its fingerprint
//   kz-dump --identity        print the tool hash of this build and the source identities it is made of
//
// This is the stand-alone form of `kz dump`; it links libkz-core only.  Exit status:
// 0 = ok, 1 = the container is damaged or fails a content check, 2 = usage or I/O error.

#include <cstdio>
#include <cstring>
#include <string>

#include "kz/core/container.hpp"
#include "kz/core/dump.hpp"
#include "kz/core/identity.hpp"

namespace {

int usage() {
  std::fprintf(stderr, "usage: kz-dump <file> | --check <file> | --hash <file> | --no-verify <file> | --layout | --identity\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc == 2 && std::strcmp(argv[1], "--layout") == 0) {
    std::fputs(kz::layout_text().c_str(), stdout);
    std::printf("fingerprint=%s\n", kz::to_hex(kz::schema_fingerprint()).c_str());
    return 0;
  }
  if (argc == 2 && std::strcmp(argv[1], "--identity") == 0) {
    // The tool hash that this tool would write into the header of an artifact (kz/core/identity.hpp).
    std::printf("tool=kz-dump\ntool_sha256=%s\ntool_source=%s\ncore_source=%s\n", kz::to_hex(KZ_THIS_TOOL_HASH("kz-dump")).c_str(),
                KZ_TOOL_SOURCE_SHA256, std::string(kz::core_source_identity()).c_str());
    return 0;
  }
  std::string mode = "dump";
  std::string path;
  if (argc == 2 && argv[1][0] != '-') {
    path = argv[1];
  } else if (argc == 3 && (std::strcmp(argv[1], "--check") == 0 || std::strcmp(argv[1], "--hash") == 0 || std::strcmp(argv[1], "--no-verify") == 0)) {
    mode = argv[1] + 2;
    path = argv[2];
  } else {
    return usage();
  }
  kz::Container c;
  kz::Status s = kz::Container::read_file(path, &c);
  if (s.code == kz::Err::Io) {
    std::fprintf(stderr, "kz-dump: %s\n", s.describe().c_str());
    return 2;
  }
  if (s.ok() && mode != "no-verify") s = c.verify_content();
  if (!s.ok()) {
    std::fprintf(stderr, "kz-dump: %s: %s\n", path.c_str(), s.describe().c_str());
    return 1;
  }
  if (mode == "hash") std::printf("%s\n", kz::to_hex(c.artifact_hash()).c_str());
  else if (mode == "dump" || mode == "no-verify") std::fputs(kz::dump_container(c).c_str(), stdout);
  return 0;
}
