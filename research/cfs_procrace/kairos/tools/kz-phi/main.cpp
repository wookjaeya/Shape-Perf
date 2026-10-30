// kz-phi -- the command line of the profile language.
//
//   kz-phi check  <dir|file>...                 load and check; print the diagnostics
//   kz-phi dump   <dir|file>...                 print the canonical text
//   kz-phi hash   <dir|file>...                 print the profile hash
//   kz-phi stats  <dir|file>...                 provenance tiers and hand-entered entries
//   kz-phi verify-prov --root <bundle> <dir|file>...
//                                               re-read every citation under the bundle and compare the hash, look up the
//                                               identifiers that entries name in the texts they cite, and compare each
//                                               source revision pin with the commit checked out (read from git metadata
//                                               files; uncommitted edits are not detected)
//   kz-phi replay --trace <file> [--real-record] [--predicate RULE=EXPR]... [--version COMPONENT=VERSION]... <dir|file>...
//                                               replay a trace (text form, eval_trace.hpp) and print the result;
//                                               --real-record replays it as a real record (SPEC 4.8, ADR-055), as a
//                                               `record real` line in the trace does; without either it is a complete
//                                               model trace
//
// A directory stands for the *.kzp files in it, in the order of their names.  Exit status:
// 0 = ok, 1 = the profile or a citation is wrong, 2 = usage or I/O error.

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/phi_provenance.hpp"

namespace fs = std::filesystem;

namespace {

int usage() {
  std::fprintf(stderr,
               "usage: kz-phi check|dump|hash|stats <dir|file>...\n"
               "       kz-phi verify-prov --root <bundle> <dir|file>...\n"
               "       kz-phi replay --trace <file> [--real-record] [--predicate RULE=EXPR]... [--version COMPONENT=VERSION]... <dir|file>...\n");
  return 2;
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
  if (paths.empty()) {
    std::fprintf(stderr, "kz-phi: no .kzp file given\n");
    return false;
  }
  for (const std::string& p : paths) {
    kz::Bytes b;
    const kz::Status s = kz::read_whole_file(p, &b);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return false;
    }
    out->push_back(kz::phi::SourceText{p, std::string(b.begin(), b.end())});
  }
  return true;
}

// Loads and checks; prints every diagnostic.  Returns 0, or 1 if the profile has errors.
int load(const std::vector<kz::phi::SourceText>& files, kz::phi::Profile* p) {
  std::vector<kz::phi::Diag> diags;
  const kz::Status s = kz::phi::load_profile(files, p, &diags);
  for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p->files, d).c_str());
  if (!s.ok()) return 1;
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) return usage();
  const std::string cmd = argv[1];
  std::vector<std::string> args;
  std::string root, trace_path;
  std::vector<std::string> predicates, versions;
  bool real_record = false;
  for (int i = 2; i < argc; ++i) {
    if (std::strcmp(argv[i], "--root") == 0) {
      if (i + 1 >= argc) return usage();
      root = argv[++i];
    } else if (std::strcmp(argv[i], "--trace") == 0) {
      if (i + 1 >= argc) return usage();
      trace_path = argv[++i];
    } else if (std::strcmp(argv[i], "--predicate") == 0) {
      if (i + 1 >= argc) return usage();
      predicates.push_back(argv[++i]);
    } else if (std::strcmp(argv[i], "--version") == 0) {
      if (i + 1 >= argc) return usage();
      versions.push_back(argv[++i]);
    } else if (std::strcmp(argv[i], "--real-record") == 0) {
      real_record = true;
    } else {
      args.push_back(argv[i]);
    }
  }
  std::vector<kz::phi::SourceText> files;
  if (!collect(args, &files)) return 2;
  kz::phi::Profile p;
  const int rc = load(files, &p);
  if (rc != 0) return rc;

  if (cmd == "check") {
    const kz::phi::ProfileStats st = kz::phi::profile_stats(p);
    std::printf("ok: %zu provenance records, %zu entries with provenance, %zu hand-entered, %zu declared-trusted\n", st.prov_records, st.entries,
                st.hand_entered, st.declared_trusted);
    return 0;
  }
  if (cmd == "dump") {
    std::fputs(kz::phi::canonical_text(p).c_str(), stdout);
    return 0;
  }
  if (cmd == "hash") {
    std::printf("%s\n", kz::to_hex(kz::phi::profile_hash(p)).c_str());
    return 0;
  }
  if (cmd == "stats") {
    std::fputs(kz::phi::stats_text(kz::phi::profile_stats(p)).c_str(), stdout);
    return 0;
  }
  if (cmd == "replay") {
    if (trace_path.empty()) return usage();
    kz::Bytes tb;
    kz::Status s = kz::read_whole_file(trace_path, &tb);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return 2;
    }
    kz::eval::Trace trace;
    s = kz::eval::parse_trace_text(std::string(tb.begin(), tb.end()), &trace);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return 2;
    }
    if (real_record) trace.record = kz::eval::RecordKind::Real;
    kz::eval::Options options;
    std::vector<kz::phi::ExprPtr> owned;
    for (const std::string& pr : predicates) {
      const size_t eq = pr.find('=');
      if (eq == std::string::npos) return usage();
      kz::phi::ExprPtr e;
      std::vector<kz::phi::Diag> diags;
      s = kz::eval::resolve_rule_predicate(p, pr.substr(0, eq), pr.substr(eq + 1), &e, &diags);
      for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
      if (!s.ok()) {
        std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
        return 2;
      }
      options.predicates[pr.substr(0, eq)] = e.get();
      owned.push_back(std::move(e));
    }
    for (const std::string& v : versions) {
      const size_t eq = v.find('=');
      if (eq == std::string::npos) return usage();
      options.versions[v.substr(0, eq)] = v.substr(eq + 1);
    }
    kz::eval::Result result;
    s = kz::eval::replay(p, trace, options, &result);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return 2;
    }
    std::fputs(kz::eval::dump_result(p, trace, result).c_str(), stdout);
    return 0;
  }
  if (cmd == "verify-prov") {
    if (root.empty()) return usage();
    std::vector<kz::phi::ProvProblem> problems;
    kz::Status s = kz::phi::verify_provenance(p, root, &problems);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return 2;
    }
    std::vector<kz::phi::SymbolProblem> symbol_problems;
    kz::phi::SymbolReport report;
    s = kz::phi::verify_symbols(p, root, &symbol_problems, &report);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return 2;
    }
    std::vector<kz::phi::RevisionCheck> revs;
    s = kz::phi::verify_revisions(p, root, &revs);
    if (!s.ok()) {
      std::fprintf(stderr, "kz-phi: %s\n", s.describe().c_str());
      return 2;
    }
    for (const kz::phi::ProvProblem& pr : problems) std::printf("%s: %s\n", pr.prov.c_str(), pr.message.c_str());
    for (const kz::phi::SymbolProblem& sp : symbol_problems) std::printf("%s: %s\n", sp.entry.c_str(), sp.message.c_str());
    size_t cited = 0;
    for (const kz::phi::ProvDecl& d : p.provs)
      if (!d.file.empty()) ++cited;
    // The first line says what the check is: the cited text is the text the profile hashed.  It does
    // not say the text supports the claim; the second line is a weaker, mechanical step toward that.
    std::printf("%zu citations hash-checked (the text is unchanged), %zu problems\n", cited, problems.size());
    // The revision pins: the commit checked out in each source tree, read from its git metadata.  An
    // uncommitted edit of the tree is not detected here; the hash check above covers the cited lines.
    size_t rev_match = 0, rev_mismatch = 0, rev_unverifiable = 0;
    for (const kz::phi::RevisionCheck& c : revs) {
      if (c.state == kz::phi::RevState::Match) ++rev_match;
      else if (c.state == kz::phi::RevState::Mismatch) ++rev_mismatch;
      else ++rev_unverifiable;
    }
    std::printf("%zu source revision pins: %zu match the checked-out commit, %zu mismatch, %zu unverifiable (uncommitted edits are not detected)\n", revs.size(), rev_match,
                rev_mismatch, rev_unverifiable);
    for (const kz::phi::RevisionCheck& c : revs) {
      if (c.state == kz::phi::RevState::Match) continue;
      std::printf("  source %s (root \"%s\") pinned %s: %s%s%s\n", c.source.c_str(), c.root.c_str(), c.pinned.c_str(), kz::phi::rev_state_text(c.state),
                  c.found.empty() ? "" : (", checked out " + c.found).c_str(), c.message.empty() ? "" : (", " + c.message).c_str());
    }
    std::printf("%zu identifier claims: %zu looked up in the cited texts, %zu unverifiable (hand-entered records only), %zu problems\n", report.claims,
                report.checked, report.unverifiable, symbol_problems.size());
    for (const std::string& u : report.unverifiable_claims) std::printf("  unverifiable (no cited text): %s\n", u.c_str());
    if (!report.idle_records.empty()) {
      std::printf("%zu cited records show none of the identifiers that the entries citing them name (context texts, or a wrong range: read them):\n", report.idle_records.size());
      for (const std::string& r : report.idle_records) std::printf("  %s\n", r.c_str());
    }
    return problems.empty() && symbol_problems.empty() && rev_mismatch == 0 ? 0 : 1;
  }
  return usage();
}
