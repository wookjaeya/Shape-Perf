// kz replay -- the real-record replay of a run container (SPEC 4.8, ADR-055; docs/observer.md section 13).
//
//   kz replay <run.kzt> --profile <dir|file> [--profile ...] [--const NAME=VALUE]... [--version COMPONENT=VERSION]...
//             [--predicate RULE=EXPR]... [--basis <basis.kz> --core-request-caller FUNC...] [--outcome-narrowing on|off]
//             [--compare-narrowing] [--facts <facts.kz> --static-basis <basis.kz>] [--out <file.kz>] [--emit-trace]
//             [--clearing-writes counted|excluded] [--owner-identity-targets on|off]
//
// Builds the real-record trace of the container (kz/trace/real_trace.hpp) and replays it with the profile
// in the real-record mode of kz/core/eval_trace.hpp: a resource the record does not fix is unknown, a result
// the explanations of the record disagree on is INCONCLUSIVE with its reasons, PENDING and OPEN are
// INCONCLUSIVE(horizon-open).  With --emit-trace it prints the trace (text form) instead, for `kz-phi
// replay` or for a derived record.  The values of `ref eval` constants (table sizes) are not in a run
// container: give them with --const, each from its reference (CONDITIONS_POLICY); the replay refuses a
// profile whose table size is missing.  The output analyses the record it is given; whether that record is
// a development run or an experiment run is stated by the record's own conditions, not here.
//
// The execution relation of every occurrence is computed from the identities (real_trace.hpp, eval_trace.hpp
// Occurrence::relation).  Whether an owner is a core service is established only with --basis (the run's
// parent, for the symbols) and --core-request-caller (each name from its reference); without them a pair of
// different applications is inter-application with the flag core-status-unknown.  --outcome-narrowing off
// replays with the alternative of the pending PI decision P1 (the returned code fixes nothing the record
// leaves open); --compare-narrowing prints the counts of both readings side by side (the main output stays
// the selected one).  --out writes the summaries, model ids, adequacy, identities and app_owner records of
// the selected reading to a container (kz/trace/replay_container.hpp) that kz-dump reads.
// --clearing-writes excluded replays with the alternative of the pending PI decision P10 (a write that clears the
// key resource or stores an invalid identity into a captured location does not make its actor an actor of the
// execution relation; current: counted).  --owner-identity-targets off keeps the effect operands that only the
// writer's confirmed creation identity fixes open (the reading before 2026-10-24; default on).  The counts end
// with the basis of every decided occurrence: state (with how many rest on the writer's identity), outcome
// (fixed by the returned code), or withheld.
//
// --facts gives the static closure of the traced binary (a kz-static FACTS container) with --static-basis, its
// parent: writer and alias coverage are then set where the closure says so (kz/trace/static_closure.hpp); the
// other conditions of ADR-055 still apply.  The closure is refused unless its basis names the binary the run's
// basis (--basis) names (binary.sha256) and it was extracted with the same profile (profile hash).
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "command.hpp"
#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/identity.hpp"
#include "kz/core/phi.hpp"
#include "kz/core/registry.hpp"
#include "kz/trace/identity_symbols.hpp"
#include "kz/trace/real_trace.hpp"
#include "kz/trace/replay_container.hpp"

namespace {

namespace fs = std::filesystem;

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz replay <run.kzt> --profile <dir|file> [--profile ...] [--const NAME=VALUE]... [--version COMPONENT=VERSION]...\n"
               "                 [--predicate RULE=EXPR]... [--basis B --core-request-caller FUNC...] [--outcome-narrowing on|off]\n"
               "                 [--compare-narrowing] [--facts F --static-basis S] [--out FILE.kz] [--emit-trace]\n"
               "                 [--clearing-writes counted|excluded] [--owner-identity-targets on|off]\n"
               "  --profile P         the profile (a directory stands for its *.kzp files, in name order)\n"
               "  --const NAME=VALUE  the value of a `ref eval` constant (a table size), from its reference\n"
               "  --version C=V       the version of a component, for the guards of rules and effects\n"
               "  --predicate R=E     the awaited predicate of an observation rule that takes it from the code\n"
               "  --basis B           the run's basis container (its parent): symbols for the core status of owners\n"
               "  --core-request-caller F  a function whose creation requests start core applications, from its reference\n"
               "  --outcome-narrowing on|off  whether the returned code fixes conditions the record leaves open (P1; default on)\n"
               "  --compare-narrowing also replay with the other setting and print both counts side by side\n"
               "  --facts F           the static closure (kz-static facts) of the traced binary: writer and alias coverage\n"
               "  --static-basis S    the kz-static basis container that F was extracted from (needs --basis)\n"
               "  --out FILE          write the summary view records of the selected reading to a container\n"
               "  --emit-trace        print the real-record trace instead of replaying it\n"
               "  --clearing-writes counted|excluded  whether a clearing write makes its actor an actor of the relation (P10; default counted)\n"
               "  --owner-identity-targets on|off  whether a confirmed owner's identity fixes the operands of its effects (default on)\n");
}

std::string enum_name(const char* en, uint64_t v) {
  const kz::EnumDesc* d = kz::find_enum(en);
  const char* t = d != nullptr ? kz::enum_display(*d, v) : nullptr;
  return t != nullptr ? t : "#" + std::to_string(v);
}

// Counts of the summary view: observation, relation, relation x observation, outcome-narrowed.
std::string counts_text(const kz::eval::Result& r, const char* title) {
  std::map<std::string, uint64_t> obs, rel, cross;
  uint64_t narrowed = 0, inferred = 0, core_unknown = 0, by_state = 0, by_identity = 0, withheld = 0;
  for (const kz::eval::Occurrence& o : r.occurrences) {
    const bool err = (o.summary.flags & KZ_SUMMARY_FLAG_INPUT_ERROR) != 0;
    // the basis of the summary: withheld, fixed by the returned code (outcome-narrowed), or the computed state
    if (err || o.summary.observation == KZ_OBS_INCONCLUSIVE || o.summary.observation == KZ_OBS_NONE) {
      ++withheld;
    } else if ((o.summary.flags & KZ_SUMMARY_FLAG_OUTCOME_NARROWED) == 0) {
      ++by_state;
      for (const kz::eval::ConjunctResult& cr : o.conjuncts)
        if (cr.owner_identity) {
          ++by_identity;
          break;
        }
    }
    const std::string ob = err ? std::string("analysis-input-error") : enum_name("KZ_OBS", o.summary.observation);
    const std::string re = enum_name("KZ_EXEC_RELATION", o.summary.relation);
    ++obs[ob];
    ++rel[re];
    ++cross[re + " / " + ob];
    if ((o.summary.flags & KZ_SUMMARY_FLAG_OUTCOME_NARROWED) != 0) ++narrowed;
    if ((o.summary.flags & KZ_SUMMARY_FLAG_RELATION_INFERRED) != 0) ++inferred;
    if ((o.summary.flags & KZ_SUMMARY_FLAG_CORE_STATUS_UNKNOWN) != 0) ++core_unknown;
  }
  std::string out = std::string("summary counts (") + title + "): occurrences " + std::to_string(r.occurrences.size()) + ", outcome-narrowed " + std::to_string(narrowed) +
                    ", relation-inferred " + std::to_string(inferred) + ", core-status-unknown " + std::to_string(core_unknown) + (r.partial ? ", partial" : "") + "\n";
  for (const auto& kv : obs) out += "  observation " + kv.first + " " + std::to_string(kv.second) + "\n";
  for (const auto& kv : rel) out += "  relation " + kv.first + " " + std::to_string(kv.second) + "\n";
  for (const auto& kv : cross) out += "  relation/observation " + kv.first + " " + std::to_string(kv.second) + "\n";
  out += "  basis state " + std::to_string(by_state) + " (of them resting on a writer's confirmed identity " + std::to_string(by_identity) + "), outcome-narrowed " +
         std::to_string(narrowed) + ", withheld " + std::to_string(withheld) + "\n";
  return out;
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
  std::string path, basis_path, out_path, narrowing = "on", facts_path, static_basis_path, clearing = "counted", identity = "on";
  std::vector<std::string> profile_args, consts, versions, predicates, core_callers;
  bool emit = false, compare = false;
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
    } else if (a == "--basis" || a == "--out" || a == "--outcome-narrowing" || a == "--facts" || a == "--static-basis" || a == "--clearing-writes" ||
               a == "--owner-identity-targets") {
      std::vector<std::string> v;
      if (!value(&v)) return 1;
      (a == "--basis"                    ? basis_path
       : a == "--out"                    ? out_path
       : a == "--facts"                  ? facts_path
       : a == "--static-basis"           ? static_basis_path
       : a == "--clearing-writes"        ? clearing
       : a == "--owner-identity-targets" ? identity
                                         : narrowing) = v.front();
    } else if (a == "--core-request-caller") {
      if (!value(&core_callers)) return 1;
    } else if (a == "--compare-narrowing") {
      compare = true;
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
  if (path.empty() || profile_args.empty() || (narrowing != "on" && narrowing != "off") || (clearing != "counted" && clearing != "excluded") ||
      (identity != "on" && identity != "off") || (!core_callers.empty() && basis_path.empty()) ||
      (facts_path.empty() != static_basis_path.empty()) || (!facts_path.empty() && basis_path.empty())) {
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
  // The core status of owners: the basis symbols and the named calling functions (real_trace.hpp, "core").
  kz::trace::SymbolIndex symbols;
  kz::Container basis;
  if (!basis_path.empty()) {
    s = kz::Container::read_file(basis_path, &basis);
    if (s.ok()) s = basis.verify_content();
    if (s.ok() && basis.header().artifact_kind != KZ_ARTIFACT_BASIS) s = kz::Status::error(kz::Err::Invalid, "not a basis container");
    if (s.ok()) s = c.verify_parent(basis);
    if (s.ok()) s = kz::trace::load_symbols(basis, &symbols);
    if (!s.ok()) {
      std::fprintf(stderr, "kz replay: --basis %s: %s\n", basis_path.c_str(), s.describe().c_str());
      return 2;
    }
    ropt.symbols = &symbols;
    ropt.core_request_callers.insert(core_callers.begin(), core_callers.end());
  }
  // The static closure: the facts, their basis, the same binary as the run's basis, the same profile.
  kz::Container facts, static_basis;
  if (!facts_path.empty()) {
    auto item = [](const kz::Container& from, const char* name) {
      std::vector<kz_basis_item> items;
      std::string v;
      if (!from.read_section(KZ_SEC_BASIS, &items).ok()) return v;
      for (const kz_basis_item& b : items) {
        std::string n;
        from.string(b.name, &n);
        if (n == name) from.string(b.value, &v);
      }
      return v;
    };
    s = kz::Container::read_file(facts_path, &facts);
    if (s.ok()) s = facts.verify_content();
    if (s.ok() && facts.header().artifact_kind != KZ_ARTIFACT_FACTS) s = kz::Status::error(kz::Err::Invalid, "not a facts container");
    if (s.ok()) s = kz::Container::read_file(static_basis_path, &static_basis);
    if (s.ok()) s = static_basis.verify_content();
    if (s.ok()) s = facts.verify_parent(static_basis);
    if (s.ok() && (item(static_basis, "binary.sha256").empty() || item(static_basis, "binary.sha256") != item(basis, "binary.sha256")))
      s = kz::Status::error(kz::Err::Invalid, "the static basis names binary " + item(static_basis, "binary.sha256") + ", the run's basis " + item(basis, "binary.sha256"));
    if (s.ok() && kz::Digest(facts.profile_hash()) != kz::phi::profile_hash(p))
      s = kz::Status::error(kz::Err::Invalid, "the facts were extracted with profile " + kz::to_hex(facts.profile_hash()) + ", the replay uses " + kz::to_hex(kz::phi::profile_hash(p)));
    if (!s.ok()) {
      std::fprintf(stderr, "kz replay: --facts %s: %s\n", facts_path.c_str(), s.describe().c_str());
      return 2;
    }
    ropt.facts = &facts;
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
  options.outcome_narrowing = narrowing == "on";
  options.relation_clearing_writes = clearing == "counted";
  options.owner_identity_targets = identity == "on";
  kz::eval::Result result;
  s = kz::eval::replay(p, trace, options, &result);
  if (!s.ok()) {
    std::fprintf(stderr, "kz replay: %s\n", s.describe().c_str());
    return 2;
  }
  std::printf("kz replay %s (real record)\nartifact_sha256 %s\n", path.c_str(), kz::to_hex(c.artifact_hash()).c_str());
  std::printf("outcome narrowing %s%s\n", narrowing.c_str(), narrowing == "on" ? " (current behaviour; PI decision P1 pending)" : " (the alternative of PI decision P1)");
  std::printf("clearing writes %s%s\n", clearing.c_str(), clearing == "counted" ? " in the execution relation (current behaviour; PI decision P10 pending)" : " from the execution relation (the alternative of PI decision P10)");
  std::printf("owner identity targets %s%s\n", identity.c_str(), identity == "on" ? " (a confirmed owner's creation identity fixes the operands of its effects)" : " (effect operands stay open where the state does not fix them)");
  if (!facts_path.empty()) std::printf("static closure %s artifact_sha256 %s (basis %s)\n", facts_path.c_str(), kz::to_hex(facts.artifact_hash()).c_str(), kz::to_hex(static_basis.artifact_hash()).c_str());
  std::fputs(kz::trace::report_text(report).c_str(), stdout);
  std::printf("\n");
  std::fputs(kz::eval::dump_result(p, trace, result).c_str(), stdout);
  std::printf("\n");
  std::fputs(counts_text(result, narrowing == "on" ? "outcome narrowing on" : "outcome narrowing off").c_str(), stdout);
  if (compare) {
    kz::eval::Options other = options;
    other.outcome_narrowing = !options.outcome_narrowing;
    kz::eval::Result alt;
    s = kz::eval::replay(p, trace, other, &alt);
    if (!s.ok()) {
      std::fprintf(stderr, "kz replay: %s\n", s.describe().c_str());
      return 2;
    }
    std::fputs(counts_text(alt, other.outcome_narrowing ? "outcome narrowing on" : "outcome narrowing off").c_str(), stdout);
    uint64_t differ = 0;
    for (size_t i = 0; i < result.occurrences.size() && i < alt.occurrences.size(); ++i) {
      const kz_summary& a = result.occurrences[i].summary;
      const kz_summary& b = alt.occurrences[i].summary;
      if (a.observation != b.observation || a.obs_reasons != b.obs_reasons || a.status != b.status || a.status_flags != b.status_flags) ++differ;
    }
    std::printf("occurrences whose observation summary differs between the two readings: %llu\n", static_cast<unsigned long long>(differ));
  }
  if (!out_path.empty()) {
    kz::trace::ReplayContainerInput in;
    in.run = &c;
    in.profile_sha256 = kz::phi::profile_hash(p);
    in.tool_sha256 = KZ_THIS_TOOL_HASH("kz");
    in.basis.push_back({"replay.mode", "real-record"});
    in.basis.push_back({"replay.outcome_narrowing", narrowing});
    in.basis.push_back({"replay.clearing_writes", clearing});
    in.basis.push_back({"replay.owner_identity_targets", identity});
    for (const auto& kv : ropt.consts) in.basis.push_back({"replay.const." + kv.first, std::to_string(kv.second)});
    for (const auto& kv : options.versions) in.basis.push_back({"replay.version." + kv.first, kv.second});
    for (const std::string& pr : predicates) in.basis.push_back({"replay.predicate", pr});
    for (const std::string& f : core_callers) in.basis.push_back({"replay.core_request_caller", f});
    if (!facts_path.empty()) {
      in.basis.push_back({"replay.facts.sha256", kz::to_hex(facts.artifact_hash())});
      in.basis.push_back({"replay.static_basis.sha256", kz::to_hex(static_basis.artifact_hash())});
    }
    kz::Bytes bytes;
    s = kz::trace::write_replay_container(in, trace, result, report, &bytes);
    if (s.ok()) s = kz::write_whole_file(out_path, bytes);
    if (!s.ok()) {
      std::fprintf(stderr, "kz replay: --out %s: %s\n", out_path.c_str(), s.describe().c_str());
      return 2;
    }
    std::printf("wrote %s artifact_sha256 %s\n", out_path.c_str(), kz::to_hex(kz::sha256(bytes.data(), bytes.size())).c_str());
  }
  return 0;
}

}  // namespace

KZ_COMMAND(replay, "replay a run container as a real record with a profile (summary view, reasons for withholding)", run);
