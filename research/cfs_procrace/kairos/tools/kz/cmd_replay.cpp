// kz replay -- the real-record replay of a run container (SPEC 4.8, ADR-055; docs/observer.md section 13).
//
//   kz replay <run.kzt> --profile <dir|file> [--profile ...] [--const NAME=VALUE]... [--version COMPONENT=VERSION]...
//             [--predicate RULE=EXPR]... [--emit-trace]
//
// Builds the real-record trace of the container (kz/trace/real_trace.hpp) and replays it with the profile
// in the real-record mode of kz/core/eval_trace.hpp: a resource the record does not fix is unknown, a result
// the explanations of the record disagree on is INCONCLUSIVE with its reasons, PENDING and OPEN are
// INCONCLUSIVE(horizon-open).  With --emit-trace it prints the trace (text form) instead, for `kz-phi
// replay` or for a derived record.  The values of `ref eval` constants (table sizes) are not in a run
// container: give them with --const, each from its reference (CONDITIONS_POLICY); the replay refuses a
// profile whose table size is missing.  The output analyses the record it is given; whether that record is
// a development run or an experiment run is stated by the record's own conditions, not here.
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/trace/real_trace.hpp"

namespace {

namespace fs = std::filesystem;

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz replay <run.kzt> --profile <dir|file> [--profile ...] [--const NAME=VALUE]... [--version COMPONENT=VERSION]...\n"
               "                 [--predicate RULE=EXPR]... [--emit-trace]\n"
               "  --profile P         the profile (a directory stands for its *.kzp files, in name order)\n"
               "  --const NAME=VALUE  the value of a `ref eval` constant (a table size), from its reference\n"
               "  --version C=V       the version of a component, for the guards of rules and effects\n"
               "  --predicate R=E     the awaited predicate of an observation rule that takes it from the code\n"
               "  --emit-trace        print the real-record trace instead of replaying it\n");
}

bool collect(const std::vector<std::string>& args, std::vector<kz::phi::SourceText>* out) {
  std::vector<std::string> paths;
  for (const std::string& a : args) {
    std::error_code ec;
    if (fs::is_directory(a, ec)) {
      std::vector<std::string> in_dir;
      for (const fs::directory_entry& e : fs::directory_iterator(a, ec))
        if (e.is_regular_file() && e.path().extension() == ".kzp") in_dir.push_back(e.path().string());
      std::sort(in_dir.begin(), in_dir.end());
      paths.insert(paths.end(), in_dir.begin(), in_dir.end());
    } else {
      paths.push_back(a);
    }
  }
  for (const std::string& p : paths) {
    kz::Bytes b;
    const kz::Status s = kz::read_whole_file(p, &b);
    if (!s.ok()) {
      std::fprintf(stderr, "kz replay: %s\n", s.describe().c_str());
      return false;
    }
    out->push_back(kz::phi::SourceText{p, std::string(b.begin(), b.end())});
  }
  return !out->empty();
}

int run(int argc, char** argv) {
  std::string path;
  std::vector<std::string> profile_args, consts, versions, predicates;
  bool emit = false;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&](std::vector<std::string>* dst) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "kz replay: %s needs a value\n", a.c_str());
        return false;
      }
      dst->push_back(argv[++i]);
      return true;
    };
    if (a == "--help" || a == "-h") {
      usage(stdout);
      return 0;
    }
    if (a == "--profile") {
      if (!value(&profile_args)) return 1;
    } else if (a == "--const") {
      if (!value(&consts)) return 1;
    } else if (a == "--version") {
      if (!value(&versions)) return 1;
    } else if (a == "--predicate") {
      if (!value(&predicates)) return 1;
    } else if (a == "--emit-trace") {
      emit = true;
    } else if (!a.empty() && a[0] == '-') {
      std::fprintf(stderr, "kz replay: unknown option %s\n", a.c_str());
      return 1;
    } else if (path.empty()) {
      path = a;
    } else {
      std::fprintf(stderr, "kz replay: unexpected argument %s\n", a.c_str());
      return 1;
    }
  }
  if (path.empty() || profile_args.empty()) {
    usage(stderr);
    return 1;
  }
  std::vector<kz::phi::SourceText> files;
  if (!collect(profile_args, &files)) {
    std::fprintf(stderr, "kz replay: no profile file\n");
    return 2;
  }
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  kz::Status s = kz::phi::load_profile(files, &p, &diags);
  for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
  if (!s.ok()) return 2;

  kz::Container c;
  s = kz::Container::read_file(path, &c);
  if (s.ok()) s = c.verify_content();
  if (!s.ok()) {
    std::fprintf(stderr, "kz replay: %s: %s\n", path.c_str(), s.describe().c_str());
    return 2;
  }
  kz::trace::RealTraceOptions ropt;
  for (const std::string& kv : consts) {
    const size_t eq = kv.find('=');
    if (eq == std::string::npos || eq == 0) {
      std::fprintf(stderr, "kz replay: --const %s: expected NAME=VALUE\n", kv.c_str());
      return 1;
    }
    char* end = nullptr;
    errno = 0;
    const long long v = std::strtoll(kv.c_str() + eq + 1, &end, 0);
    if (end == nullptr || *end != '\0' || errno != 0) {
      std::fprintf(stderr, "kz replay: --const %s: the value is not an integer\n", kv.c_str());
      return 1;
    }
    ropt.consts[kv.substr(0, eq)] = v;
  }
  // The constants are given to the adapter as well: the owner kind's table size maps an owner identity
  // to its record key (real_trace.hpp, "owners").
  kz::eval::Trace trace;
  kz::trace::RealTraceReport report;
  s = kz::trace::real_trace_from_run(c, p, ropt, &trace, &report);
  if (!s.ok()) {
    std::fprintf(stderr, "kz replay: %s\n", s.describe().c_str());
    return 2;
  }
  for (const auto& kv : ropt.consts) trace.ref_consts[kv.first] = kv.second;
  if (emit) {
    std::fputs(kz::eval::trace_text(trace).c_str(), stdout);
    return 0;
  }
  kz::eval::Options options;
  std::vector<kz::phi::ExprPtr> owned;
  for (const std::string& pr : predicates) {
    const size_t eq = pr.find('=');
    if (eq == std::string::npos) {
      usage(stderr);
      return 1;
    }
    kz::phi::ExprPtr e;
    std::vector<kz::phi::Diag> pd;
    s = kz::eval::resolve_rule_predicate(p, pr.substr(0, eq), pr.substr(eq + 1), &e, &pd);
    if (!s.ok()) {
      std::fprintf(stderr, "kz replay: --predicate %s: %s\n", pr.c_str(), s.describe().c_str());
      return 1;
    }
    options.predicates[pr.substr(0, eq)] = e.get();
    owned.push_back(std::move(e));
  }
  for (const std::string& v : versions) {
    const size_t eq = v.find('=');
    if (eq == std::string::npos) {
      usage(stderr);
      return 1;
    }
    options.versions[v.substr(0, eq)] = v.substr(eq + 1);
  }
  kz::eval::Result result;
  s = kz::eval::replay(p, trace, options, &result);
  if (!s.ok()) {
    std::fprintf(stderr, "kz replay: %s\n", s.describe().c_str());
    return 2;
  }
  std::printf("kz replay %s (real record)\nartifact_sha256 %s\n", path.c_str(), kz::to_hex(c.artifact_hash()).c_str());
  std::fputs(kz::trace::report_text(report).c_str(), stdout);
  std::printf("\n");
  std::fputs(kz::eval::dump_result(p, trace, result).c_str(), stdout);
  return 0;
}

}  // namespace

KZ_COMMAND(replay, "replay a run container as a real record with a profile (summary view, reasons for withholding)", run);
