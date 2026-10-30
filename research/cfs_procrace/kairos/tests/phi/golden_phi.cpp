// Golden tests of the profile and of the rules of the V1 subset.
//
//   kz_phi_golden <case> <profile-dir> <data-dir> <trace-dir>
//
// Cases:
//   profile_canonical   the canonical text of the profile
//   profile_summary     the profile hash, the provenance statistics, the Allowed derivation
//   replay_<name>       the replay of a synthetic trace with the profile (edges of every rule,
//                       statuses of the readiness specification, findings, the final state)
//
// With KZ_GOLDEN_UPDATE=1 in the environment the golden file is rewritten instead of compared;
// review the diff before accepting it.  The traces are synthetic: every value in them is a
// unit-test value, not a research condition and not a result.
//
// The awaited predicate of the observation rule is supplied here, as a stand-in for the extractor
// that derives it from the code (the profile says `predicate from_code`).

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/phi_allowed.hpp"

namespace fs = std::filesystem;

namespace {

struct ReplayCase {
  const char* name;
  const char* trace;
  bool predicate;
  const char* cfe_version;  // nullptr: no version given
};

const ReplayCase kReplayCases[] = {
    {"startup_ok", "startup_ok", true, "7.0.1"},
    {"startup_old_version", "startup_ok", true, "6.4.1"},
    {"startup_no_version", "startup_ok", true, nullptr},
    {"startup_no_predicate", "startup_ok", false, "7.0.1"},
    {"kid_illegal", "kid_illegal", true, "7.0.1"},
    {"kid_unregistered", "kid_unregistered", true, "7.0.1"},
    {"kid_ordinals", "kid_ordinals", true, "7.0.1"},
    {"kid_disagree", "kid_disagree", true, "7.0.1"},
    {"shutdown_scope", "shutdown_scope", true, "7.0.1"},
    {"poll_timeout", "poll_timeout", true, "7.0.1"},
    {"poll_timeout_wrong", "poll_timeout_wrong", true, "7.0.1"},
    {"kid_outcomes", "kid_outcomes", true, "7.0.1"},
    {"effects_core", "effects_core", true, "7.0.1"},
    {"effects_external", "effects_external", true, "7.0.1"},
    {"cleanup_window", "cleanup_window", true, "7.0.1"},
    // C1 (profile c1-bp-b051eb0 with the variant directories the test names): the roles of the call
    // contexts, the caller-level Allowed per CF revision.
    {"c1_pre_fail", "c1_pre_fail", false, nullptr},
    {"c1_post_retry", "c1_post_retry", false, nullptr},
    {"c1_post_retry_altP3", "c1_post_retry", false, nullptr},
    {"c1_role_unbound", "c1_role_unbound", false, nullptr},
};

const char* const kPredicate = "forall r: es.app where state(r) != FREE { state(es.appstate(r)) >= RUNNING }";

bool read_file(const std::string& path, std::string* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  *out = ss.str();
  return true;
}

// `dirs` is one directory or several separated by ':' (a profile with variant directories, e.g. C1).
bool load_profile(const std::string& dirs, kz::phi::Profile* p) {
  std::vector<std::string> paths;
  std::error_code ec;
  const std::string dir = dirs;
  for (size_t b = 0; b <= dirs.size();) {
    size_t e = dirs.find(':', b);
    if (e == std::string::npos) e = dirs.size();
    std::vector<std::string> in_dir;
    for (const fs::directory_entry& de : fs::directory_iterator(dirs.substr(b, e - b), ec))
      if (de.is_regular_file() && de.path().extension() == ".kzp") in_dir.push_back(de.path().string());
    std::sort(in_dir.begin(), in_dir.end());
    paths.insert(paths.end(), in_dir.begin(), in_dir.end());
    b = e + 1;
  }
  std::vector<kz::phi::SourceText> files;
  for (const std::string& path : paths) {
    kz::phi::SourceText t;
    t.name = path;
    if (!read_file(path, &t.text)) {
      std::fprintf(stderr, "cannot read %s\n", path.c_str());
      return false;
    }
    files.push_back(std::move(t));
  }
  if (files.empty()) {
    std::fprintf(stderr, "no .kzp file in %s\n", dir.c_str());
    return false;
  }
  std::vector<kz::phi::Diag> diags;
  const kz::Status s = kz::phi::load_profile(files, p, &diags);
  for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p->files, d).c_str());
  if (!s.ok()) {
    std::fprintf(stderr, "the profile does not load: %s\n", s.describe().c_str());
    return false;
  }
  return true;
}

std::string case_summary(const kz::phi::Profile& p) {
  std::string out = "profile " + kz::phi::quote_string(p.name) + "\n";
  out += "revision " + kz::phi::quote_string(p.revision) + "\n";
  out += "hash " + kz::to_hex(kz::phi::profile_hash(p)) + "\n\n";
  out += kz::phi::stats_text(kz::phi::profile_stats(p));
  out += "\nreadiness specifications\n";
  for (const kz::phi::RspecDecl& r : p.rspecs) {
    const kz::phi::CallerAllowed a = kz::phi::derive_caller_allowed(p, r);
    out += "  " + r.name + ": caller-level Allowed " + kz::phi::caller_allowed_text(a.kind) + " (" + a.reason + ")\n";
  }
  return out;
}

std::string case_replay(const kz::phi::Profile& p, const ReplayCase& c, const std::string& trace_dir) {
  std::string text;
  if (!read_file(trace_dir + "/" + c.trace + ".trace", &text)) {
    std::fprintf(stderr, "cannot read trace %s\n", c.trace);
    std::exit(2);
  }
  kz::eval::Trace trace;
  const kz::Status ps = kz::eval::parse_trace_text(text, &trace);
  if (!ps.ok()) {
    std::fprintf(stderr, "trace %s: %s\n", c.trace, ps.describe().c_str());
    std::exit(2);
  }
  kz::eval::Options options;
  kz::phi::ExprPtr predicate;
  if (c.predicate) {
    std::vector<kz::phi::Diag> diags;
    const kz::Status rs = kz::eval::resolve_rule_predicate(p, "R19", kPredicate, &predicate, &diags);
    for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
    if (!rs.ok()) {
      std::fprintf(stderr, "predicate: %s\n", rs.describe().c_str());
      std::exit(2);
    }
    options.predicates["R19"] = predicate.get();
  }
  if (c.cfe_version != nullptr) options.versions["cfe"] = c.cfe_version;
  kz::eval::Result result;
  const kz::Status rs = kz::eval::replay(p, trace, options, &result);
  if (!rs.ok()) {
    std::fprintf(stderr, "replay %s: %s\n", c.name, rs.describe().c_str());
    std::exit(2);
  }
  std::string out = std::string("# trace ") + c.trace + ".trace, predicate " + (c.predicate ? "supplied" : "not supplied") + ", cfe version " +
                    (c.cfe_version != nullptr ? c.cfe_version : "not given") + "\n";
  out += kz::eval::dump_result(p, trace, result);
  return out;
}

int compare(const std::string& name, const std::string& got, const std::string& golden_dir) {
  const std::string path = golden_dir + "/" + name + ".golden";
  const char* upd = std::getenv("KZ_GOLDEN_UPDATE");
  if (upd != nullptr && std::strcmp(upd, "1") == 0) {
    std::ofstream out(path, std::ios::binary);
    out << got;
    std::printf("updated %s\n", path.c_str());
    return 0;
  }
  std::string want;
  if (!read_file(path, &want)) {
    std::fprintf(stderr, "missing golden file %s (KZ_GOLDEN_UPDATE=1 writes it)\n", path.c_str());
    return 1;
  }
  if (want == got) {
    std::printf("%s: matches\n", name.c_str());
    return 0;
  }
  std::fprintf(stderr, "%s: differs from %s\n", name.c_str(), path.c_str());
  // the first differing line, for the log
  std::istringstream a(want), b(got);
  std::string la, lb;
  for (int n = 1;; ++n) {
    const bool ga = static_cast<bool>(std::getline(a, la)), gb = static_cast<bool>(std::getline(b, lb));
    if (!ga && !gb) break;
    if (!ga || !gb || la != lb) {
      std::fprintf(stderr, "  line %d\n    golden: %s\n    got   : %s\n", n, ga ? la.c_str() : "(end)", gb ? lb.c_str() : "(end)");
      break;
    }
  }
  return 1;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 5 && argc != 6) {
    std::fprintf(stderr, "usage: kz_phi_golden <case> <profile-dir[:dir...]> <data-dir> <trace-dir> [<golden name>]\n");
    return 2;
  }
  const std::string name = argv[1];
  const std::string golden = argc == 6 ? argv[5] : name;  // a profile with variants names its golden files
  kz::phi::Profile p;
  if (!load_profile(argv[2], &p)) return 2;
  if (name == "profile_canonical") return compare(golden, kz::phi::canonical_text(p), argv[3]);
  if (name == "profile_summary") return compare(golden, case_summary(p), argv[3]);
  if (name.rfind("replay_", 0) == 0) {
    for (const ReplayCase& c : kReplayCases)
      if (name == std::string("replay_") + c.name) return compare(golden, case_replay(p, c, argv[4]), argv[3]);
  }
  std::fprintf(stderr, "unknown case %s\n", name.c_str());
  return 2;
}
