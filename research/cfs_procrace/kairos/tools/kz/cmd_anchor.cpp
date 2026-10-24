// kz anchor -- build the basis container (anchor table) of a binary for a profile.
//
//   kz anchor <binary> <profile.kzp> -o <basis.kz> [--source-root DIR] [--root NAME=DIR]... [--binary-path PATH] [--bin NAME]
//
// A profile over several files (several `binary` statements) has one basis per file: --bin names the
// file this basis anchors (default: the declared binary whose name is the file name of <binary>, else
// the primary).
// The binary is only read.  The basis records its identity (path, size, SHA-256, build-id), its
// function symbols and, for every site of the profile, the file offset of the probe target.
#include <limits.h>
#include <stdlib.h>

#include <cstdio>
#include <cstring>
#include <string>

#include "basis.hpp"
#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/core/identity.hpp"

namespace {

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz anchor <binary> <profile.kzp> -o <basis.kz> [options]\n"
               "  -o FILE             the basis container to write (required)\n"
               "  --source-root DIR   directory that the profile's cited files are relative to; the cited\n"
               "                      lines are hashed and checked against the profile's declarations\n"
               "  --root NAME=DIR     a further named source root (a prov entry says root=NAME)\n"
               "  --binary-path PATH  the path to record for the binary (default: its absolute path)\n"
               "  --bin NAME          the declared binary of the profile this basis anchors (default: the one named\n"
               "                      like the file, else the primary)\n");
}

int run(int argc, char** argv) {
  std::string binary, profile_path, out_path, source_root, binary_path, bin_name;
  std::vector<std::pair<std::string, std::string>> roots;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) { std::fprintf(stderr, "kz anchor: %s needs a value\n", a.c_str()); return false; }
      *dst = argv[++i];
      return true;
    };
    if (a == "--help" || a == "-h") { usage(stdout); return 0; }
    if (a == "-o") { if (!value(&out_path)) return 1; }
    else if (a == "--source-root") { if (!value(&source_root)) return 1; }
    else if (a == "--binary-path") { if (!value(&binary_path)) return 1; }
    else if (a == "--bin") { if (!value(&bin_name)) return 1; }
    else if (a == "--root") {
      std::string v;
      if (!value(&v)) return 1;
      const size_t eq = v.find('=');
      if (eq == std::string::npos || eq == 0) { std::fprintf(stderr, "kz anchor: --root NAME=DIR\n"); return 1; }
      roots.emplace_back(v.substr(0, eq), v.substr(eq + 1));
    } else if (!a.empty() && a[0] == '-') { std::fprintf(stderr, "kz anchor: unknown option %s\n", a.c_str()); return 1; }
    else if (binary.empty()) binary = a;
    else if (profile_path.empty()) profile_path = a;
    else { std::fprintf(stderr, "kz anchor: unexpected argument %s\n", a.c_str()); return 1; }
  }
  if (binary.empty() || profile_path.empty() || out_path.empty()) { usage(stderr); return 1; }

  kz::Bytes bin, prof;
  kz::Status s = kz::read_whole_file(binary, &bin);
  if (!s.ok()) { std::fprintf(stderr, "kz anchor: %s\n", s.describe().c_str()); return 2; }
  s = kz::read_whole_file(profile_path, &prof);
  if (!s.ok()) { std::fprintf(stderr, "kz anchor: %s\n", s.describe().c_str()); return 2; }
  kz::plan::Profile profile;
  s = kz::plan::parse_profile(std::string_view(reinterpret_cast<const char*>(prof.data()), prof.size()), &profile);
  if (!s.ok()) { std::fprintf(stderr, "kz anchor: %s\n", s.describe().c_str()); return 2; }

  kz::plan::BasisOptions opt;
  {
    const std::string base = binary.substr(binary.find_last_of('/') == std::string::npos ? 0 : binary.find_last_of('/') + 1);
    const std::string want = bin_name.empty() ? base : bin_name;
    size_t b = 0;
    while (b < profile.binaries.size() && profile.binaries[b] != want) ++b;
    if (b == profile.binaries.size()) {
      if (!bin_name.empty()) { std::fprintf(stderr, "kz anchor: the profile declares no binary '%s'\n", bin_name.c_str()); return 2; }
      b = 0;
    }
    opt.bin = static_cast<uint16_t>(b);
  }
  if (binary_path.empty()) {
    char real[PATH_MAX];
    binary_path = realpath(binary.c_str(), real) != nullptr ? std::string(real) : binary;
  }
  opt.binary_path = binary_path;
  opt.source_root = source_root;
  opt.roots = roots;
  opt.tool_hash = KZ_THIS_TOOL_HASH("kz");
  kz::Bytes out;
  kz::plan::BasisReport rep;
  s = kz::plan::build_basis(bin, profile, opt, &out, &rep);
  if (!s.ok()) { std::fprintf(stderr, "kz anchor: %s\n", s.describe().c_str()); return 2; }
  s = kz::write_whole_file(out_path, out);
  if (!s.ok()) { std::fprintf(stderr, "kz anchor: %s\n", s.describe().c_str()); return 3; }
  static const char* tiers[6] = {"unreferenced", "contract-backed", "admitted", "code-derived", "issue-derived", "researcher"};
  std::printf("basis %s\n", kz::to_hex(kz::sha256(out.data(), out.size())).c_str());
  std::printf("  binary %s (%zu bytes)\n", binary_path.c_str(), bin.size());
  std::printf("  %zu targets, %zu function symbols\n", rep.targets, rep.symbols);
  std::printf("  provenance entries:");
  for (int t = 0; t < 6; ++t)
    if (rep.tier_count[t] != 0) std::printf(" %s=%zu", tiers[t], rep.tier_count[t]);
  std::printf("\n  sites by tier:");
  for (int t = 0; t < 6; ++t)
    if (rep.sites_by_tier[t] != 0) std::printf(" %s=%zu", tiers[t], rep.sites_by_tier[t]);
  std::printf("\n  cited text: %zu verified against the declared hash, %zu hashed here (none declared), %zu declared but not verified (source root not given), %zu not hashed\n",
              rep.prov_verified, rep.prov_computed, rep.prov_declared, rep.prov_unhashed);
  return 0;
}

}  // namespace

KZ_COMMAND(anchor, "build the anchor table (basis container) of a binary for a profile", run);
