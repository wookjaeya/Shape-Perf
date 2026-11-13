// kz predict -- bounded explicit-state prediction anchored in a real record (SPEC 4.9 rev 1.3; docs/predict.md).
//
//   kz predict <run.kzt> --profile <dir|file> [--profile ...] --model <file.kzm> [--model ...]
//              [--const NAME=VALUE]... [--basis B] [--facts F --static-basis S] [--outcome-narrowing on|off]
//              [--max-states N] [--edges variable|fixed] [--outcomes rederive|keep] [--bind MODEL=RECORD]...
//              [--prov-root DIR] [--observe-profile FILE] [--out-dir DIR]
//
// The record is replayed exactly as `kz replay` does (the same profile, constants, basis, closure and P1
// setting); its observation judgement is printed first.  The model (kzp-predict 1, kz/predict/model.hpp) is
// resolved against the profile, anchored in the record (an execution of the model must make the record's cone
// steps under every recorded edge between them), and then searched: every interleaving of its steps under the
// constraints only (program order, creation, guards, stratum-S edges), every result computed from the state at
// its step.  Each class of executions found is re-judged by the replay of its model trace; a candidate the replay
// contradicts is rejected.  For a new in-scope candidate a hold plan (kzp-hold 1) is built from the model's hold
// points and checked in the model under the supervisor's semantics (what it forces, whether it can stall).
//
// --max-states is the visited-state cap of each search: a researcher choice, printed with the result; reaching it
// is reported and the search is then incomplete.  --edges fixed and --outcomes keep are the two ablations of the
// observed-run-dependent orders (docs/predict.md).  --prov-root re-reads the model's own citations (cite lines)
// under the pinned bundle; --observe-profile checks that the plan's sites exist in the observation profile with a
// kind the hold can use.  --out-dir writes predict.txt, anchor.trace, W<n>.trace (the model trace of each
// candidate, for `kz-phi replay --trace`) and W<n>.kzh (the plans).
//
// Before the search the record's judgement and the model replay of the anchor are compared per occurrence and fact
// (contradiction / assumption of the model / gap of the record); a contradiction refuses the prediction.  Exit status:
// 0 done (also when the model is not anchored), 1 usage, 2 an input or analysis error, 3 a citation or plan problem,
// 4 the fitness check found a contradiction (no search).
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "command.hpp"
#include "hold_plan.hpp"
#include "kz/core/container.hpp"
#include "kz/core/eval_trace.hpp"
#include "kz/core/phi.hpp"
#include "kz/predict/predict.hpp"
#include "kz/trace/identity_symbols.hpp"
#include "kz/trace/real_trace.hpp"
#include "profile.hpp"

namespace {

namespace fs = std::filesystem;

void usage(FILE* f) {
  std::fprintf(f,
               "usage: kz predict <run.kzt> --profile <dir|file> [--profile ...] --model <file.kzm> [--model ...]\n"
               "                  [--const NAME=VALUE]... [--basis B] [--facts F --static-basis S] [--outcome-narrowing on|off]\n"
               "                  [--max-states N] [--edges variable|fixed] [--outcomes rederive|keep] [--bind MODEL=RECORD]...\n"
               "                  [--prov-root DIR] [--observe-profile FILE] [--out-dir DIR]\n"
               "  --model F           a predict model (kzp-predict 1); several files are one model, in the order given\n"
               "  --max-states N      the visited-state cap of each search (researcher choice; default 2000000, printed)\n"
               "  --edges fixed       ablation: the record's observed-run-dependent orders become constraints\n"
               "  --outcomes keep     ablation: the record's results are kept and those orders dropped\n"
               "  --bind M=R          the model actor M is the record's actor R (default: the model's record name)\n"
               "  --prov-root DIR     re-read the model's cite lines under the pinned bundle DIR\n"
               "  --observe-profile F check the plans' sites against the observation profile F (kzp-observe 1)\n"
               "  --out-dir DIR       write predict.txt, anchor.trace, W<n>.trace and W<n>.kzh\n"
               "  the other options are those of kz replay\n");
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
      std::fprintf(stderr, "kz predict: %s\n", s.describe().c_str());
      return false;
    }
    out->push_back(kz::phi::SourceText{p, std::string(b.begin(), b.end())});
  }
  return !out->empty();
}

bool write_text(const std::string& path, const std::string& text) {
  kz::Bytes b(text.begin(), text.end());
  const kz::Status s = kz::write_whole_file(path, b);
  if (!s.ok()) std::fprintf(stderr, "kz predict: %s: %s\n", path.c_str(), s.describe().c_str());
  return s.ok();
}

int run(int argc, char** argv) {
  std::string path, basis_path, narrowing = "on", facts_path, static_basis_path, out_dir, prov_root, observe_path, edges = "variable", outcomes = "rederive";
  std::vector<std::string> profile_args, consts, models, binds;
  uint64_t max_states = 2000000;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto value = [&](std::string* dst) {
      if (i + 1 >= argc) {
        std::fprintf(stderr, "kz predict: %s needs a value\n", a.c_str());
        return false;
      }
      *dst = argv[++i];
      return true;
    };
    std::string v;
    if (a == "--help" || a == "-h") {
      usage(stdout);
      return 0;
    }
    if (a == "--profile" || a == "--const" || a == "--model" || a == "--bind") {
      if (!value(&v)) return 1;
      (a == "--profile" ? profile_args : a == "--const" ? consts : a == "--model" ? models : binds).push_back(v);
    } else if (a == "--basis" || a == "--facts" || a == "--static-basis" || a == "--outcome-narrowing" || a == "--out-dir" || a == "--prov-root" || a == "--observe-profile" ||
               a == "--edges" || a == "--outcomes") {
      if (!value(&v)) return 1;
      (a == "--basis"             ? basis_path
       : a == "--facts"           ? facts_path
       : a == "--static-basis"    ? static_basis_path
       : a == "--outcome-narrowing" ? narrowing
       : a == "--out-dir"         ? out_dir
       : a == "--prov-root"       ? prov_root
       : a == "--observe-profile" ? observe_path
       : a == "--edges"           ? edges
                                  : outcomes) = v;
    } else if (a == "--max-states") {
      if (!value(&v)) return 1;
      char* end = nullptr;
      max_states = std::strtoull(v.c_str(), &end, 10);
      if (end == nullptr || *end != '\0' || max_states == 0) {
        std::fprintf(stderr, "kz predict: --max-states needs a positive integer\n");
        return 1;
      }
    } else if (!a.empty() && a[0] == '-') {
      std::fprintf(stderr, "kz predict: unknown option %s\n", a.c_str());
      return 1;
    } else if (path.empty()) {
      path = a;
    } else {
      std::fprintf(stderr, "kz predict: unexpected argument %s\n", a.c_str());
      return 1;
    }
  }
  if (path.empty() || profile_args.empty() || models.empty() || (narrowing != "on" && narrowing != "off") || (edges != "variable" && edges != "fixed") ||
      (outcomes != "rederive" && outcomes != "keep") || (facts_path.empty() != static_basis_path.empty()) || (!facts_path.empty() && basis_path.empty())) {
    usage(stderr);
    return 1;
  }
  std::vector<kz::phi::SourceText> files;
  if (!collect(profile_args, &files)) return 2;
  kz::phi::Profile p;
  std::vector<kz::phi::Diag> diags;
  kz::Status s = kz::phi::load_profile(files, &p, &diags);
  for (const kz::phi::Diag& d : diags) std::fprintf(stderr, "%s\n", kz::phi::format_diag(p.files, d).c_str());
  if (!s.ok()) return 2;
  kz::Container c;
  s = kz::Container::read_file(path, &c);
  if (s.ok()) s = c.verify_content();
  if (!s.ok()) {
    std::fprintf(stderr, "kz predict: %s: %s\n", path.c_str(), s.describe().c_str());
    return 2;
  }
  kz::trace::RealTraceOptions ropt;
  for (const std::string& kv : consts) {
    const size_t eq = kv.find('=');
    char* end = nullptr;
    errno = 0;
    const long long val = eq == std::string::npos ? 0 : std::strtoll(kv.c_str() + eq + 1, &end, 0);
    if (eq == std::string::npos || eq == 0 || end == nullptr || *end != '\0' || errno != 0) {
      std::fprintf(stderr, "kz predict: --const %s: expected NAME=INTEGER\n", kv.c_str());
      return 1;
    }
    ropt.consts[kv.substr(0, eq)] = val;
  }
  kz::trace::SymbolIndex symbols;
  kz::Container basis, facts, static_basis;
  if (!basis_path.empty()) {
    s = kz::Container::read_file(basis_path, &basis);
    if (s.ok()) s = basis.verify_content();
    if (s.ok()) s = c.verify_parent(basis);
    if (s.ok()) s = kz::trace::load_symbols(basis, &symbols);
    if (!s.ok()) {
      std::fprintf(stderr, "kz predict: --basis %s: %s\n", basis_path.c_str(), s.describe().c_str());
      return 2;
    }
    ropt.symbols = &symbols;
  }
  if (!facts_path.empty()) {
    s = kz::Container::read_file(facts_path, &facts);
    if (s.ok()) s = facts.verify_content();
    if (s.ok()) s = kz::Container::read_file(static_basis_path, &static_basis);
    if (s.ok()) s = static_basis.verify_content();
    if (s.ok()) s = facts.verify_parent(static_basis);
    if (s.ok() && kz::Digest(facts.profile_hash()) != kz::phi::profile_hash(p))
      s = kz::Status::error(kz::Err::Invalid, "the facts were extracted with another profile");
    if (!s.ok()) {
      std::fprintf(stderr, "kz predict: --facts %s: %s\n", facts_path.c_str(), s.describe().c_str());
      return 2;
    }
    ropt.facts = &facts;
  }
  kz::eval::Options options;
  options.outcome_narrowing = narrowing == "on";
  ropt.reading = &options;  // the linearization reads as the replay below does (real_trace.hpp, "order")
  kz::eval::Trace trace;
  kz::trace::RealTraceReport report;
  s = kz::trace::real_trace_from_run(c, p, ropt, &trace, &report);
  if (!s.ok()) {
    std::fprintf(stderr, "kz predict: %s\n", s.describe().c_str());
    return 2;
  }
  for (const auto& kv : ropt.consts) trace.ref_consts[kv.first] = kv.second;
  kz::eval::Result result;
  s = kz::eval::replay(p, trace, options, &result);
  if (!s.ok()) {
    std::fprintf(stderr, "kz predict: %s\n", s.describe().c_str());
    return 2;
  }
  // the model
  kz::predict::Model model;
  std::vector<std::string> errors;
  for (const std::string& mf : models) {
    kz::Bytes b;
    s = kz::read_whole_file(mf, &b);
    if (s.ok()) s = kz::predict::parse_model(mf, std::string(b.begin(), b.end()), &model, &errors);
    if (!s.ok() && errors.empty()) errors.push_back(mf + ": " + s.describe());
  }
  for (const std::string& bd : binds) {
    const size_t eq = bd.find('=');
    const int32_t ai = eq == std::string::npos ? -1 : model.find_actor(bd.substr(0, eq));
    if (ai < 0) errors.push_back("--bind " + bd + ": no such model actor");
    else model.actors[static_cast<size_t>(ai)].record = bd.substr(eq + 1);
  }
  if (errors.empty()) kz::predict::resolve_model(p, ropt.consts, &model, &errors);
  for (const std::string& e : errors) std::fprintf(stderr, "kz predict: %s\n", e.c_str());
  if (!errors.empty()) return 2;
  std::vector<std::string> cite_problems;
  if (!prov_root.empty()) kz::predict::verify_cites(model, prov_root, &cite_problems);

  kz::predict::PredictInput in;
  in.profile = &p;
  in.model = &model;
  in.record = &trace;
  in.record_result = &result;
  in.ref_consts = ropt.consts;
  in.replay_options = options;
  in.max_states = max_states;
  in.edges = edges == "fixed" ? kz::predict::EdgeMode::Fixed : kz::predict::EdgeMode::Variable;
  in.outcomes = outcomes == "keep" ? kz::predict::OutcomeMode::Keep : kz::predict::OutcomeMode::Rederive;
  kz::predict::PredictReport rep;
  s = kz::predict::predict(in, &rep);
  if (!s.ok()) {
    std::fprintf(stderr, "kz predict: %s\n", s.describe().c_str());
    return 2;
  }
  std::string text = "kz predict " + path + " (real record, artifact_sha256 " + kz::to_hex(c.artifact_hash()) + ")\n";
  text += "profile hash " + kz::to_hex(kz::phi::profile_hash(p)) + "; outcome narrowing " + narrowing + "; max states " + std::to_string(max_states) +
          (max_states == 2000000 ? " (the default of kz predict, a researcher choice)" : " (given, a researcher choice)") + "\n";
  if (!prov_root.empty()) {
    text += "model citations re-read under " + prov_root + ": " + (cite_problems.empty() ? std::to_string(model.cites.size()) + " of " + std::to_string(model.cites.size()) + " match" : "PROBLEMS") + "\n";
    for (const std::string& pr : cite_problems) text += "  " + pr + "\n";
  } else {
    text += "model citations not re-read (no --prov-root)\n";
  }
  text += kz::predict::report_text(in, rep);
  // the plans: written, parsed back as kzp-hold 1, and checked against the observation profile when given
  kz::plan::Profile observe;
  bool have_observe = false;
  if (!observe_path.empty()) {
    kz::Bytes ob;
    s = kz::read_whole_file(observe_path, &ob);
    if (s.ok()) s = kz::plan::parse_profile(std::string_view(reinterpret_cast<const char*>(ob.data()), ob.size()), &observe);
    if (!s.ok()) {
      std::fprintf(stderr, "kz predict: --observe-profile %s: %s\n", observe_path.c_str(), s.describe().c_str());
      return 2;
    }
    have_observe = true;
  }
  int status = cite_problems.empty() ? 0 : 3;
  if (rep.fitness.contradiction) status = 4;  // the anchor contradicts the record: no prediction (docs/predict.md)
  if (!out_dir.empty()) {
    std::error_code ec;
    fs::create_directories(out_dir, ec);
  }
  std::string plans_note;
  for (const kz::predict::Witness& w : rep.witnesses)
    for (size_t k = 0; k < w.plans.size(); ++k) {
      const kz::predict::PlanResult& pr = w.plans[k];
      const std::string pt = kz::predict::plan_text(in, w, k, path);
      kz::hold::HoldPlan hp;
      s = kz::hold::parse_hold_plan(pt, &hp);
      std::string verdict = s.ok() ? "parses as kzp-hold 1" : "DOES NOT PARSE: " + s.describe();
      bool bad = !s.ok();
      if (s.ok() && have_observe) {
        for (const kz::predict::ControlPoint& cp : pr.plan.points) {
          const kz::plan::ProfileSite* site = nullptr;
          for (const kz::plan::ProfileSite& ps : observe.sites)
            if (ps.label == cp.site) site = &ps;
          if (site == nullptr) {
            verdict += "; site " + cp.site + " NOT IN the observation profile";
            bad = true;
          } else if (site->kind == KZ_SITE_KIND_RET_PROBE) {
            verdict += "; site " + cp.site + " is a return probe (its address is the function entry): NOT USABLE";
            bad = true;
          } else if (!cp.release && site->kind != KZ_SITE_KIND_ENTRY_PROBE && site->kind != KZ_SITE_KIND_POINT_PROBE) {
            verdict += "; gate site " + cp.site + " is neither an entry nor a point";
            bad = true;
          }
        }
        if (!bad) verdict += "; every site is in " + observe_path + " with a usable kind";
      }
      if (bad) status = 3;
      plans_note += pr.name + ".kzh: " + verdict + "\n";
      if (!out_dir.empty() && !write_text(out_dir + "/" + pr.name + ".kzh", pt)) return 2;
    }
  if (!plans_note.empty()) text += "\nplans\n" + plans_note;
  std::fputs(text.c_str(), stdout);
  if (!out_dir.empty()) {
    std::error_code ec;
    fs::create_directories(out_dir, ec);
    if (!write_text(out_dir + "/predict.txt", text)) return 2;
    if (rep.anchored) {
      const kz::eval::Trace at = kz::predict::execution_trace(model, rep.anchor, in.ref_consts, trace.owners);
      if (!write_text(out_dir + "/anchor.trace", kz::eval::trace_text(at))) return 2;
    }
    for (const kz::predict::Witness& w : rep.witnesses)
      if (!write_text(out_dir + "/W" + std::to_string(w.id) + ".trace", kz::eval::trace_text(w.trace))) return 2;
  }
  return status;
}

}  // namespace

KZ_COMMAND(predict, "bounded explicit-state prediction on a predict model, anchored in a record (witnesses, hold plans)", run);
