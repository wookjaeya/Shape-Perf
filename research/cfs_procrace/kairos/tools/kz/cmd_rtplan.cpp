// kz rtplan -- converts a kzp-hold 1 plan for one instrumented build into the runtime's serialized plan
// (kzrt-plan 1, kairos/runtime/include/kz_rt_format.h), which the POSIX start-up object reads from KZ_RT_PLAN in
// control mode (the G-I-H group of the same binary).  The conversion rules are in probe/hold/rt_plan.hpp; what
// kzrt-plan 1 cannot express is refused, never approximated.
//
//   kz rtplan <plan.kzh> --manifest <logical.kzs> --binds <binds.kzs> --out <plan.kzrtp> [--record <file>]
//
// --manifest: the logical site manifest the build's compiler binding read (its sha256 is the build identity's,
// which the runtime compares before loading); --binds: the build's merged binding list.  The record (stdout, and
// --record) names the inputs, every point and its mapping, and the differences from F0's kz hold.
// Nothing here is a research condition.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "command.hpp"
#include "hold_plan.hpp"
#include "kz/core/sha256.hpp"
#include "rt_plan.hpp"

namespace {

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz rtplan <plan.kzh> --manifest <logical.kzs> --binds <binds.kzs> --out <plan.kzrtp> [--record <file>]\n"
               "  converts a kzp-hold 1 plan for one instrumented build (kzrt-plan 1, read from KZ_RT_PLAN in control mode)\n");
}

bool read_file(const std::string& path, std::string* out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return false;
  std::ostringstream ss;
  ss << f.rdbuf();
  *out = ss.str();
  return true;
}

bool write_file(const std::string& path, const std::string& bytes) {
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) return false;
  f.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  return static_cast<bool>(f);
}

int run(int argc, char** argv) {
  std::string plan_path, manifest_path, binds_path, out_path, record_path;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "kz rtplan: %s needs a value\n", a.c_str());
        return false;
      }
      *dst = argv[++i];
      return true;
    };
    if (a == "--help" || a == "-h") {
      usage(stdout);
      return 0;
    } else if (a == "--manifest") {
      if (!value(&manifest_path)) return 2;
    } else if (a == "--binds") {
      if (!value(&binds_path)) return 2;
    } else if (a == "--out") {
      if (!value(&out_path)) return 2;
    } else if (a == "--record") {
      if (!value(&record_path)) return 2;
    } else if (!a.empty() && a[0] != '-' && plan_path.empty()) {
      plan_path = a;
    } else {
      usage(stderr);
      return 2;
    }
  }
  if (plan_path.empty() || manifest_path.empty() || binds_path.empty() || out_path.empty()) {
    usage(stderr);
    return 2;
  }
  std::string plan_text, manifest_text, binds_text, err;
  if (!read_file(plan_path, &plan_text) || !read_file(manifest_path, &manifest_text) || !read_file(binds_path, &binds_text)) {
    std::fprintf(stderr, "kz rtplan: cannot read an input\n");
    return 2;
  }
  kz::hold::HoldPlan plan;
  kz::Status s = kz::hold::parse_hold_plan(plan_text, &plan);
  if (!s.ok()) {
    std::fprintf(stderr, "kz rtplan: %s: %s\n", plan_path.c_str(), s.message.c_str());
    return 1;
  }
  kzrt::Manifest m;
  if (!kzrt::parse_manifest(manifest_text, manifest_path, &m, &err) || !kzrt::parse_manifest(binds_text, binds_path, &m, &err)) {
    std::fprintf(stderr, "kz rtplan: %s\n", err.c_str());
    return 1;
  }
  kz::hold::RtPlan rp;
  s = kz::hold::convert_rt_plan(plan, plan_text, m, manifest_text, binds_text, &rp);
  if (!s.ok()) {
    std::fprintf(stderr, "kz rtplan: %s: %s\n", plan_path.c_str(), s.message.c_str());
    return 1;
  }
  const std::string bytes = kz::hold::encode_rt_plan(rp);
  if (!write_file(out_path, bytes)) {
    std::fprintf(stderr, "kz rtplan: cannot write %s\n", out_path.c_str());
    return 2;
  }
  std::string rec = "# kz rtplan record\nplan " + plan_path + "\nmanifest " + manifest_path + "\nbinds " + binds_path + "\n" + kz::hold::rt_plan_record(rp) +
                    "out " + out_path + " bytes=" + std::to_string(bytes.size()) + " sha256=" + kz::to_hex(kz::sha256(bytes)) + "\n";
  std::fputs(rec.c_str(), stdout);
  if (!record_path.empty() && !write_file(record_path, rec)) {
    std::fprintf(stderr, "kz rtplan: cannot write %s\n", record_path.c_str());
    return 2;
  }
  return 0;
}

}  // namespace

KZ_COMMAND(rtplan, "convert a kzp-hold 1 plan for an instrumented build (kzrt-plan 1)", run);
