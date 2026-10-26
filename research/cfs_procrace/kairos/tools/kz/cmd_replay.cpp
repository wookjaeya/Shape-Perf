// kz replay -- the real-record replay of a run container (SPEC 4.8, ADR-055; docs/observer.md section 13).
//
//   kz replay <run.kzt> --profile <dir|file> [--profile ...] [--const NAME=VALUE]... [--version COMPONENT=VERSION]...
//             [--predicate RULE=EXPR]... [--basis <basis.kz> --core-request-caller FUNC...] [--outcome-narrowing on|off]
//             [--compare-narrowing] [--facts <facts.kz> --static-basis <basis.kz>] [--out <file.kz>] [--emit-trace]
//             [--clearing-writes counted|excluded] [--owner-identity-targets on|off] [--reads-from-edges on|off]
//             [--reads-from-outcome on|off]
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
// The record is read through the neutral event layer (schema revision 3, kz/trace/neutral.hpp): the container's neutral
// sections (`kz convert`, an F0 run or an instrumented run's kzrt-dump), or its F0 sections converted in memory with the
// same rules; the output names which.
//
// The execution relation of every occurrence is computed from the identities (real_trace.hpp, eval_trace.hpp
// Occurrence::relation).  Whether an owner is a core service is established only with --basis (the run's
// parent, for the symbols) and --core-request-caller (each name from its reference); without them a pair of
// different applications is inter-application with the flag core-status-unknown.  P1 (adopted conditionally
// 2026-10-25): the returned code fixes a condition the record leaves open only through a mapping whose
// applicability conditions (the profile's `narrow { ... }`) all hold in the record; --outcome-narrowing off
// replays with the code fixing nothing; --compare-narrowing prints the counts of both readings side by side (the main
// output stays the selected one).  --out writes the summaries, model ids, adequacy, identities and app_owner records of
// the selected reading to a container (kz/trace/replay_container.hpp) that kz-dump reads; the basis and the declared
// assumptions of each result are flags of its summary.
// P10 (decided 2026-10-25): the execution relation is computed per occurrence (eval_trace.hpp RelationItem: providers,
// affecting writes, writes excluded by scope or generation, unconfirmed influence).  --clearing-writes counted|excluded
// selects one of the two earlier readings instead (every observed write counts / clearing writes do not), for
// sensitivity only.  --owner-identity-targets off keeps the effect operands that only the writer's confirmed creation
// identity fixes open (the reading before 2026-10-24; default on).  --reads-from-edges off leaves out the edges of the
// profile's reads_from rules (SPEC 3.16 row RF; default on), to compare.  --reads-from-outcome off lets a use of such a
// rule be shown successful only by the store of its out value on the success path (`stored_out`, a direct capture of
// the store operand), never by its returned code (default on), to show which results stand without any returned code.
// The counts end with the basis of every
// occurrence (an occurrence may carry more than one), the declared assumptions, and the outcome checks, where a code
// that fixed a condition or agrees with a return-derived state is not counted as an independent agreement.
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
               "                 [--clearing-writes counted|excluded] [--owner-identity-targets on|off] [--reads-from-edges on|off]\n"
               "                 [--reads-from-outcome on|off] [--control-order on|off]\n"
               "  --profile P         the profile (a directory stands for its *.kzp files, in name order)\n"
               "  --const NAME=VALUE  the value of a `ref eval` constant (a table size), from its reference\n"
               "  --version C=V       the version of a component, for the guards of rules and effects\n"
               "  --predicate R=E     the awaited predicate of an observation rule that takes it from the code\n"
               "  --basis B           the run's basis container (its parent): symbols for the core status of owners\n"
               "  --core-request-caller F  a function whose creation requests start core applications, from its reference\n"
               "  --outcome-narrowing on|off  whether the returned code fixes conditions the record leaves open, where the mapping's\n"
               "                      conditions hold (P1 conditional; default on)\n"
               "  --compare-narrowing also replay with the other setting and print both counts side by side\n"
               "  --facts F           the static closure (kz-static facts) of the traced binary: writer and alias coverage\n"
               "  --static-basis S    the kz-static basis container that F was extracted from (needs --basis)\n"
               "  --out FILE          write the summary view records of the selected reading to a container\n"
               "  --emit-trace        print the real-record trace instead of replaying it\n"
               "  --clearing-writes counted|excluded  an earlier relation reading, for sensitivity only (default: the P10 rule per occurrence)\n"
               "  --owner-identity-targets on|off  whether a confirmed owner's identity fixes the operands of its effects (default on)\n"
               "  --reads-from-edges on|off  whether the reads_from rules of the profile make edges (SPEC 3.16 row RF; default on)\n"
               "  --reads-from-outcome on|off  whether a use's returned code may show it successful for those rules (off: only\n"
               "                     the success-path store of its out value, stored_out; default on)\n"
               "  --control-order on|off  whether a controlled run's realized holds and releases (NCTL) order events, as the basis kind\n"
               "                     control-order (P11 decided 2026-10-26; default on; off = the sensitivity reading)\n");
}

std::string enum_name(const char* en, uint64_t v) {
  const kz::EnumDesc* d = kz::find_enum(en);
  const char* t = d != nullptr ? kz::enum_display(*d, v) : nullptr;
  return t != nullptr ? t : "#" + std::to_string(v);
}

// Counts of the summary view: observation, relation, relation x observation, outcome-narrowed.
std::string counts_text(const kz::eval::Result& r, const char* title) {
  std::map<std::string, uint64_t> obs, rel, cross, basis, assumed, checks;
  uint64_t narrowed = 0, inferred = 0, core_unknown = 0, by_state = 0, by_identity = 0, withheld = 0, unconfirmed = 0, not_app = 0;
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
    if ((o.summary.flags & KZ_SUMMARY_FLAG_UNCONFIRMED_INFLUENCE) != 0) ++unconfirmed;
    if ((o.summary.flags & KZ_SUMMARY_FLAG_NOT_APP_ACTOR) != 0) ++not_app;
    // the basis kinds (an occurrence may carry more than one) and their combination
    static const std::pair<uint16_t, const char*> kBasis[] = {{KZ_SUMMARY_FLAG_BASIS_DIRECT_CAPTURE, "direct-capture"},
                                                              {KZ_SUMMARY_FLAG_BASIS_STATE_ORDER, "state-order"},
                                                              {KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER, "return-derived-order"},
                                                              {KZ_SUMMARY_FLAG_OUTCOME_NARROWED, "outcome-narrowed"}};
    std::string combo;
    for (const auto& b : kBasis)
      if ((o.basis & b.first) != 0) {
        ++basis[b.second];
        combo += (combo.empty() ? "" : "+") + std::string(b.second);
      }
    if (o.basis_store_order) {  // in memory only (summary.flags has no bit for it): reads_from by a success-path store
      ++basis["store-derived-order"];
      combo += (combo.empty() ? "" : "+") + std::string("store-derived-order");
    }
    if (o.control_order) {  // P11, in memory only: the realized control of a controlled run, a basis kind of its own
      ++basis["control-order"];
      combo += (combo.empty() ? "" : "+") + std::string("control-order");
    }
    ++basis["combination " + (combo.empty() ? std::string("none") : combo) + " / " + ob];
    static const std::pair<uint16_t, const char*> kAssumed[] = {{KZ_SUMMARY_FLAG_ASSUMES_VIA_SEPARATION, "via key separation"},
                                                                {KZ_SUMMARY_FLAG_ASSUMES_OWNER_IDENTITY, "owner identity targets (A-OWNER)"},
                                                                {KZ_SUMMARY_FLAG_BASIS_RETURN_DERIVED_ORDER, "reads-from edges (RF)"},
                                                                {KZ_SUMMARY_FLAG_ASSUMES_ROLE, "role of the call context"}};
    for (const auto& a : kAssumed)
      if ((o.summary.flags & a.first) != 0) ++assumed[a.second];
    ++checks[kz::eval::outcome_check_text(o.outcome)];
  }
  std::string out = std::string("summary counts (") + title + "): occurrences " + std::to_string(r.occurrences.size()) + ", outcome-narrowed " + std::to_string(narrowed) +
                    ", relation-inferred " + std::to_string(inferred) + ", core-status-unknown " + std::to_string(core_unknown) + (r.partial ? ", partial" : "") + "\n";
  for (const auto& kv : obs) out += "  observation " + kv.first + " " + std::to_string(kv.second) + "\n";
  for (const auto& kv : rel) out += "  relation " + kv.first + " " + std::to_string(kv.second) + "\n";
  for (const auto& kv : cross) out += "  relation/observation " + kv.first + " " + std::to_string(kv.second) + "\n";
  out += "  basis state " + std::to_string(by_state) + " (of them resting on a writer's confirmed identity " + std::to_string(by_identity) + "), outcome-narrowed " +
         std::to_string(narrowed) + ", withheld " + std::to_string(withheld) + "\n";
  for (const auto& kv : basis) out += "  basis " + kv.first + " " + std::to_string(kv.second) + "\n";
  for (const auto& kv : assumed) out += "  assumption " + kv.first + " " + std::to_string(kv.second) + "\n";
  for (const auto& kv : checks) out += "  outcome check " + kv.first + " " + std::to_string(kv.second) + "\n";
  out += "  relation with an unconfirmed influence listed apart " + std::to_string(unconfirmed) + ", with an actor confirmed not to be an application " + std::to_string(not_app) + "\n";
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
  std::string path, basis_path, out_path, narrowing = "on", facts_path, static_basis_path, clearing, identity = "on", reads_from = "on", rf_outcome = "on", control = "on";
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
               a == "--owner-identity-targets" || a == "--reads-from-edges" || a == "--reads-from-outcome" ||
               a == "--control-order") {
      std::vector<std::string> v;
      if (!value(&v)) return 1;
      (a == "--basis"                    ? basis_path
       : a == "--out"                    ? out_path
       : a == "--facts"                  ? facts_path
       : a == "--static-basis"           ? static_basis_path
       : a == "--clearing-writes"        ? clearing
       : a == "--owner-identity-targets" ? identity
       : a == "--reads-from-edges"       ? reads_from
       : a == "--reads-from-outcome"     ? rf_outcome
       : a == "--control-order"          ? control
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
  if (path.empty() || profile_args.empty() || (narrowing != "on" && narrowing != "off") || (!clearing.empty() && clearing != "counted" && clearing != "excluded") ||
      (identity != "on" && identity != "off") || (reads_from != "on" && reads_from != "off") || (rf_outcome != "on" && rf_outcome != "off") ||
      (control != "on" && control != "off") || (!core_callers.empty() && basis_path.empty()) ||
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

  bool rf_rules = false;  // the profile has reads_from rules (the option and its BASIS item are stated only then, or when off)
  for (const kz::phi::HbRuleDecl& r : p.rules) rf_rules = rf_rules || r.tmpl == kz::phi::RuleTemplate::ReadsFrom;

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
  options.relation_rule = clearing.empty() ? kz::eval::Options::RelationRule::Occurrence
                         : clearing == "counted" ? kz::eval::Options::RelationRule::AllWrites
                                                 : kz::eval::Options::RelationRule::NonClearing;
  options.owner_identity_targets = identity == "on";
  options.reads_from_edges = reads_from == "on";
  options.reads_from_outcome = rf_outcome == "on";
  options.control_order = control == "on";
  kz::eval::Result result;
  s = kz::eval::replay(p, trace, options, &result);
  if (!s.ok()) {
    std::fprintf(stderr, "kz replay: %s\n", s.describe().c_str());
    return 2;
  }
  std::printf("kz replay %s (real record)\nartifact_sha256 %s\n", path.c_str(), kz::to_hex(c.artifact_hash()).c_str());
  std::printf("record layer: %s\n", report.neutral_sections ? "the container's neutral event sections (schema revision 3)"
                                                             : "the container's F0 sections, converted in memory to the neutral event layer");
  std::printf("outcome narrowing %s%s\n", narrowing.c_str(),
              narrowing == "on" ? " (P1 adopted conditionally 2026-10-25: only through a mapping whose applicability conditions hold in the record)" : " (the code fixes nothing the record leaves open)");
  if (clearing.empty())
    std::printf("execution relation: the P10 rule (decided 2026-10-25): per occurrence, providers, affecting writes, writes excluded by scope or generation, unconfirmed influence\n");
  else
    std::printf("execution relation: an earlier reading for sensitivity only: clearing writes %s\n", clearing == "counted" ? "counted (every observed write)" : "excluded");
  std::printf("owner identity targets %s%s\n", identity.c_str(), identity == "on" ? " (a confirmed owner's creation identity fixes the operands of its effects)" : " (effect operands stay open where the state does not fix them)");
  if (rf_rules || reads_from == "off")
    std::printf("reads-from edges %s%s\n", reads_from.c_str(), reads_from == "on" ? " (the profile's reads_from rules make edges, SPEC 3.16 row RF)" : " (the edges of reads_from rules are left out)");
  if (rf_outcome == "off")
    std::printf("reads-from uses by returned code off (a use counts only where the record has the success-path store of its out value, stored_out)\n");
  if (!trace.control_edges.empty() || control == "off") {
    std::printf("control order %s%s; control edges %zu\n", control.c_str(),
                control == "on" ? " (P11 decided 2026-10-26: this controlled run's realized holds and releases order events, a basis kind of its own: control-order)"
                                : " (the program's synchronization only; the run's realized control is not an order: the reading kept as sensitivity)",
                trace.control_edges.size());
    for (const kz::eval::HbEdge& e : trace.control_edges)
      std::printf("control edge #%llu -> #%llu %s\n", static_cast<unsigned long long>(e.from), static_cast<unsigned long long>(e.to), e.rule.c_str());
  }
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
    in.basis.push_back({"replay.relation_rule", clearing.empty() ? std::string("P10-occurrence") : "sensitivity-clearing-writes-" + clearing});
    in.basis.push_back({"replay.owner_identity_targets", identity});
    if (rf_rules || reads_from == "off") in.basis.push_back({"replay.reads_from_edges", reads_from});
    if (rf_outcome == "off") in.basis.push_back({"replay.reads_from_outcome", rf_outcome});
    if (!trace.control_edges.empty() || control == "off") in.basis.push_back({"replay.control_order", control});
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
