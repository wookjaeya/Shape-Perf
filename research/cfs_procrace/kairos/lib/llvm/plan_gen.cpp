// plan_gen.cpp -- see plan_gen.hpp and docs/plan_manifest.md.
#include "plan_gen.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <tuple>

#include "kz/core/sha256.hpp"
#include "rel_facts.hpp"

namespace kzs {

const char* plan_rules_revision() { return "plan rules revision 2 (2026-11-16)"; }
const char* plan_rules_v4_revision() {
  return "plan rules revision 4 (2026-11-18): manifest revision 4 (kzs-sites 4), the path A control selection, value requests, the wait rule and the object rules; "
         "both handling labels on the row line, the key-clearing calls as object-delete sites, the event sends of the other outcomes apart";
}
const char* plan_control_revision() { return "control selection rule revision 1 (2026-11-16)"; }
const char* plan_requests_revision() { return "value requests rule revision 1 (2026-11-16)"; }
const char* plan_priority_revision() {
  return "method priority port revision 1 (2026-11-16): label, order class, reach mark and disposition of the method document's S1 rule over the row text";
}

const std::vector<RelReason>& plan_reasons() {
  static const std::vector<RelReason> table = {
      {"budget_not_run", "row outside the budget", "a row the budget cuts: it is listed by key, with no site"},
      {"plan_missing", "need not planned", "a need the rules of this revision do not plan (wait and lock results, event sends, an owner request the facts do not give, a stop event with no framework-layer entry)"},
      {"record_insufficient", "key text", "a key argument with no non-null test of the program before the point: recorded as a pointer only"},
      {"record_insufficient", "two calls of one callee on one line", "a call site whose function calls the same callee twice on its line: the binding by line cannot tell them apart"},
      {"record_insufficient", "more values than the slots of an event", "a site that would record more than four values: the values past the fourth are not recorded"},
      {"record_insufficient", "site position not in the facts", "a position of a row's line that names no call, store or function of the facts"},
      {"link_unlinked", "no creation-argument channel", "a thread-creation call whose creation argument is one constant for several roots: the argument does not tie a thread to its creation"},
      {"actor_not_extracted", "no creation site resolved", "an actor root whose SPAWN_WRAP chain reaches no thread-creation primitive call"},
      {"truncated", "source path not unique", "a site whose translation unit path suffix names more than one unit of the basis"},
      // plan rules revision 3 (manifest revision 4)
      {"budget_not_run", "control row outside the control budget", "a row of the path A control selection that the control budget cuts: it is listed by key, with no site"},
      {"plan_missing", "wait predicate not recognised", "a wait the row's sites follow whose predicate neither FACT_LOOPS nor LOOP_DEFS recognises: its condition reads are not planned"},
      {"plan_missing", "no namespace data entry", "a planned path A row whose lookup and creators match no applying entry of the namespace data given: the deletes and retires of its object kind are not planned"},
      {"plan_missing", "no rule of this revision", "a need of a path the rules of this revision do not plan (the lock calls of path A and B rows, a branch taken, an effect without a call)"},
      {"record_insufficient", "the exit is no statement", "a condition exit of a wait loop: the point after the loop is not in the facts, the exit taken is recorded by its condition reads only"},
  };
  return table;
}

namespace {

constexpr uint32_t kN = KZ_NONE32;
constexpr uint32_t kSlots = 4;      // values per event bound by the plugin (kzgcc.cpp build_obs: slots 0-3)
constexpr uint32_t kStrMax = 64;    // KZ_RT_STR_MAX (runtime/include/kz_rt.h:82)

std::string reason(size_t i) { return reason_text(plan_reasons()[i]); }
enum R { kBudget, kNotPlanned, kKeyText, kTwoCalls, kSlotsFull, kNoPos, kNoChannel, kNoCreation, kPathNotUnique, kControlCut, kWaitPred, kNoNamespace, kNoRule, kExitNoStmt };

std::string quoted(const std::string& s) {
  std::string t;
  for (char ch : s) t += ch == '"' ? '\'' : (ch == '\n' ? ' ' : ch);
  return "\"" + t + "\"";
}

// ======================================================================== the priority port
// The rule of the method document's first step over the text of one row (the revised S1 rule; the probe that applies it
// to the relations text is plan/ovm_rev1_20261116/probe/s1_rev1.py, against which docs/plan_manifest.md §2 reports the
// agreement).  Every regular expression below reads the rendered row.
bool fail_class(long long v) { return v < 0 || (v >= 0xC0000000LL && v <= 0xFFFFFFFFLL); }

std::vector<std::pair<long long, std::string>> consts_of(const std::string& text) {
  static const std::regex re(R"(returns (-?\d+) \[([^\]]*)\])");
  std::vector<std::pair<long long, std::string>> out;
  for (std::sregex_iterator it(text.begin(), text.end(), re), end; it != end; ++it) out.emplace_back(std::atoll((*it)[1].str().c_str()), (*it)[2].str());
  return out;
}

bool starts(const std::string& s, const std::string& p) { return s.compare(0, p.size(), p) == 0; }

PlanPriority classify(const RelRow& row) {
  PlanPriority p;
  p.id = std::string(1, row.path) + std::to_string(row.number);
  p.key = row.key;
  // the lines after the head, as the probe reads them: every line indented by six or more, stripped
  std::vector<std::string> L;
  {
    std::istringstream in(render_row(row));
    std::string l;
    std::getline(in, l);
    while (std::getline(in, l)) {
      if (!starts(l, "      ")) continue;
      size_t a = l.find_first_not_of(' '), b = l.find_last_not_of(" \t\r");
      L.push_back(a == std::string::npos ? std::string() : l.substr(a, b - a + 1));
    }
  }
  auto get = [&](const std::string& pre) {
    std::vector<std::string> v;
    for (const std::string& x : L)
      if (starts(x, pre)) v.push_back(x);
    return v;
  };
  auto first_or = [&](const std::string& pre, const std::string& dflt) {
    std::vector<std::string> v = get(pre);
    return v.empty() ? dflt : v[0];
  };
  const std::string feas = first_or("feasibility:", "feasibility: ?").substr(13);
  const std::string flip = first_or("flip:", "flip: ?").substr(6);
  std::string flipg = flip.substr(0, flip.find(':'));
  flipg = flipg.substr(0, flipg.find(' '));
  std::string lab, oc, reach;
  long long pair = 0;
  bool fail = false;
  if (row.path == 'A') {
    const std::string act = first_or("actors:", "");
    std::smatch m;
    std::string res = "?";
    static const std::regex re_res(R"(result=(\w+))");
    if (std::regex_search(act, m, re_res)) res = m[1].str();
    std::vector<std::pair<long long, std::string>> ab;
    const size_t sr = flip.find("search returns:");
    if (sr != std::string::npos) ab = consts_of(flip.substr(sr + 15));
    std::vector<long long> failc;
    for (const auto& c : ab)
      if (fail_class(c.first)) failc.push_back(c.first);
    fail = !failc.empty();
    const bool term = !get("handling: terminal").empty();
    bool probe = false;
    for (const std::string& x : L)
      if ((starts(x, "handling:") || starts(x, "not a pair:")) && x.find("probe-create") != std::string::npos) probe = true;
    bool retry = false;
    static const std::regex re_cmp(R"(\((?:ne|eq) local:\w+ (-?\d+)\))");
    for (const std::string& x : get("handling: in the loop")) {
      if (x.find(" delay ") == std::string::npos) continue;
      std::string ex;
      const size_t e = x.find(" exit ");
      if (e != std::string::npos) {
        ex = x.substr(e + 6);
        ex = ex.substr(0, ex.find(" delay "));
      }
      for (std::sregex_iterator it(ex.begin(), ex.end(), re_cmp), end; it != end; ++it)
        if (fail_class(std::atoll((*it)[1].str().c_str()))) retry = true;
    }
    if (probe || retry) lab = "H";
    else if (term) lab = "R";
    else if (fail && res == "discarded") lab = "J";
    else if (fail && res == "tested") lab = "T?";
    else if (fail) lab = "E";
    else if (!ab.empty()) lab = "0";
    else lab = "E";
    static const std::regex re_pair(R"((\d+) creator\(s\))");
    const std::string dep = first_or("dependency:", "");
    if (std::regex_search(dep, m, re_pair)) pair = std::atoll(m[1].str().c_str());
    if (pair > 0) oc = starts(feas, "undecidable: the facts give no static order") ? "RELOC" : "UNK";
    else oc = "NOPAIR";
    std::set<long long> stated;
    std::string all;
    for (const std::string& x : L) all += (all.empty() ? "" : " ") + x;
    static const std::regex re_st(R"(returns (-?\d+) \(no match)");
    for (std::sregex_iterator it(all.begin(), all.end(), re_st), end; it != end; ++it) stated.insert(std::atoll((*it)[1].str().c_str()));
    if (failc.empty()) reach = "NO-FAILCONST";
    else if (std::any_of(failc.begin(), failc.end(), [&](long long v) { return stated.count(v) != 0; })) reach = "REACH-STATED";
    else if (flipg == "static-possible") reach = "REACH-FLIP";
    else reach = "CALLEE-ONLY";
  } else if (row.path == 'B') {
    const std::string h = first_or("handling: discarded", "");
    std::smatch m;
    long long d = 0, t = 0;
    static const std::regex re_h(R"(discarded (\d+), used (\d+), tested (\d+))");
    if (std::regex_search(h, m, re_h)) {
      d = std::atoll(m[1].str().c_str());
      t = std::atoll(m[3].str().c_str());
    }
    std::vector<std::string> cc = get("callee-check:");
    std::vector<std::pair<long long, std::string>> ab;
    for (const std::string& x : cc)
      for (const auto& c : consts_of(x)) ab.push_back(c);
    for (const auto& c : ab)
      if (fail_class(c.first)) fail = true;
    if (fail && d > 0) lab = "J";
    else if (fail && t > 0) lab = "T?";
    else if (fail) lab = "E";
    else if (!cc.empty()) lab = "0";
    else lab = "E";
    static const std::regex re_cl(R"(reader actors created before it: ([^;.]*); after it: ([^.]*)\.)");
    bool before = false, any_cl = false;
    for (std::sregex_iterator it(feas.begin(), feas.end(), re_cl), end; it != end; ++it) {
      any_cl = true;
      std::string b = (*it)[1].str();
      b = b.substr(b.find_first_not_of(' ') == std::string::npos ? 0 : b.find_first_not_of(' '));
      if (!starts(b, "none")) before = true;
    }
    if (starts(feas, "proven-impossible")) oc = "ENF";
    else if (before) oc = "FIXED";
    else if (any_cl || starts(feas, "undecidable: fixed unconfirmed")) oc = "COND";
    else if (starts(feas, "undecidable: no static order")) oc = "RELOC";
    else oc = "UNK";
    if (cc.empty()) reach = "NO-CHECK";
    else if (!fail) reach = "NO-FAILCONST";
    else if (flipg == "none") reach = "INDEPENDENT";
    else if (flipg == "static-possible") reach = "REACH-FLIP";
    else if (flipg == "static-undetermined") reach = "UNEVALUATED";
    else reach = "CALLEE-ONLY";
    pair = 1;
  } else {
    static const std::regex re_e(R"(returns (-?\d+) \[([^\]]*)\] with the earlier value)");
    bool early_fail = false;
    for (std::sregex_iterator it(flip.begin(), flip.end(), re_e), end; it != end; ++it)
      if (fail_class(std::atoll((*it)[1].str().c_str()))) early_fail = true;
    const std::vector<std::pair<long long, std::string>> allc = consts_of(flip);
    fail = early_fail;
    lab = early_fail ? "F" : (!allc.empty() ? "0" : "E");
    static const std::regex re_lock(R"(\block\b)");
    if (starts(feas, "proven-impossible")) oc = "ENF";
    else if (starts(feas, "possible")) oc = "RELOC";
    else if (starts(feas, "not graded")) oc = "NOPAIR";
    else if (starts(feas, "undecidable") && std::regex_search(feas.substr(0, feas.find(';')), re_lock)) oc = "COND";
    else oc = "UNK";
    if (early_fail) reach = "REACH-STATED";
    else if (std::any_of(allc.begin(), allc.end(), [](const std::pair<long long, std::string>& c) { return fail_class(c.first); })) reach = "CALLEE-ONLY";
    else if (flipg == "static-undetermined") reach = "UNEVALUATED";
    else if (flipg == "none") reach = "INDEPENDENT";
    else reach = "NO-FAILCONST";
    pair = 1;
  }
  const bool counterpart = pair > 0;
  std::string d, why;
  if (oc == "ENF") d = "D01", why = "the row proves the opposite order impossible";
  else if (oc == "FIXED") d = "D02", why = "a reader actor created before the writer (fixed-order candidate); label " + lab;
  else if (oc == "COND") d = "D03", why = "the order rests on a premise the facts do not establish; label " + lab;
  else if (lab == "H") d = "D04", why = "probe-create, or a delayed retry loop on a failure status";
  else if (lab == "0") d = "D05", why = "no failure-class absence constant";
  else if ((lab == "R" || lab == "J") && !counterpart) d = "D06", why = "label " + lab + ", no creator in another actor";
  else if (lab == "J" && oc == "RELOC" && reach == "INDEPENDENT") d = "D08", why = "label J, the row evaluates the constant returns as independent of the value";
  else if ((lab == "R" || lab == "J") && oc == "RELOC") d = "D07", why = "label " + lab + ", counterpart, no static order; reach " + reach;
  else if (lab == "R" || lab == "J") d = "D09", why = "label " + lab + ", order class " + oc;
  else if (lab == "F") d = "D10", why = "failure absence constant with the earlier value; the caller's handling is not in the row; order class " + oc;
  else if (lab == "T?") d = "D11", why = "the status is tested; the effect of the absence branch is not in the row; order class " + oc;
  else if (lab == "E") d = "D12", why = "the row gives no absence result; order class " + oc;
  else d = "D13", why = "no rule matched";
  p.label = lab;
  for (const std::string& x : L)
    if (starts(x, "handling_label: ")) {
      p.fact_label = x.substr(16);
      p.fact_label = p.fact_label.substr(0, p.fact_label.find(' '));
    }
  p.label_class = (lab == "R" || lab == "J" || lab == "H" || lab == "0") ? "decided" : (lab == "T?" || lab == "F") ? "effect-undetermined" : "absence-undetermined";
  p.order_class = oc;
  p.reach = reach;
  p.disposition = d;
  p.why = why;
  p.tier = d == "D07" ? 1 : oc == "RELOC" ? 2 : 3;
  return p;
}

// ======================================================================== sites
struct Val {
  std::string role, from, type = "u64", string_mode, ref;
  uint32_t bound = 0;
};

struct Site {
  std::string tu, func, phase, place, callee;
  uint32_t line = 0;
  std::string op;
  std::set<std::string> rules, refs;
  std::vector<Val> vals;
  uint32_t id = 0;
  // manifest revision 4: the column and ordinal of a call, the field of a load (0 / empty: none; revision 2 sets none)
  uint32_t col = 0, nth = 0;
  std::string field;
  std::string key() const {
    std::string k = tu + "|" + func + "|" + phase + "|" + place + "|" + callee + "|" + std::to_string(line);
    if (col != 0 || nth != 0) k += "|col=" + std::to_string(col) + "|nth=" + std::to_string(nth);
    if (!field.empty()) k += "|field=" + field;
    return k;
  }
};

struct Need {
  std::string subject, what, state, why, note;
  std::set<std::string> sites;  // manifest revision 4: the site keys of a planned need
};

// A value request (manifest revision 4): what value would decide a row the method cannot label from the facts.
struct Request {
  std::string row, id, label, value, what, state, site, slot, why, note;
};

class Gen : public RelFacts {
 public:
 private:
  struct NsOp {
    std::string what, fn, def_file, at_file;
    uint32_t def_line = 0, at_line = 0;
    int line = 0;
  };
  struct NsEntry {
    std::string name;
    std::vector<NsOp> ops;
    int line = 0;
  };
  struct NsFile {
    std::string path;
    std::map<std::string, std::string> sha;
    std::vector<NsEntry> entries;
  };
  struct SpawnBind {
    uint32_t start = 0, arg = 0, out = KZ_NONE32;
    std::string ref;
  };
 public:
  Gen(const kz::Container& facts, const kz::Container* basis, const std::string& framework, const PlanOptions& o)
      : RelFacts(facts), basis_c_(basis), framework_(framework), o_(o) {}

  kz::Status run(PlanOutput* out) {
    kz::Status s = check_options();
    if (!s.ok()) return s;
    if (!(s = prepare()).ok()) return s;
    if (!(s = c_.read_section(KZ_SEC_FILE_HASHES, &files_)).ok()) return s;
    if (!(s = c_.read_section(KZ_SEC_FACT_ARGS, &args_)).ok()) return s;
    // ARG_TYPES (facts of --object-access; absent otherwise): the extent of a character array a key argument is
    {
      std::vector<kz_fact_argtype> at;
      if (!(s = c_.read_section(KZ_SEC_ARG_TYPES, &at)).ok()) return s;
      for (const kz_fact_argtype& r : at) argtype_[{r.call, r.pos}] = r;
    }
    if (!(s = read_framework()).ok()) return s;
    if (!(s = read_basis()).ok()) return s;
    if (v4_) {
      if (!(s = read_namespaces()).ok()) return s;
      if (!(s = c_.read_section(KZ_SEC_OBJECT_ACCESS, &objacc_)).ok()) return s;
      for (const kz_fact_sem& m : sem_)
        if (m.column == KZ_SEM_COLUMN_ARG_ROLE && m.pos != kN) {
          // the role is the item's first word (the item may name how it was read after it)
          const std::string item = str(m.item);
          const std::string role = item.substr(0, item.find(' '));
          if (!role.empty()) arg_roles_[str(m.api)].emplace(m.pos, role);
        }
    }
    for (size_t i = 0; i < calls_.size(); ++i) call_at_[{str(calls_[i].file), calls_[i].line, calls_[i].col}] = static_cast<uint32_t>(i);
    if (!(s = relations_rows(c_, RelationsOptions{}, &rep_)).ok()) return s;
    // the priority order and the budget
    for (RelGroup& g : rep_.groups)
      for (RelRow& r : g.rows) rows_.push_back(&r);
    for (const RelRow* r : rows_) prio_.push_back(classify(*r));
    std::vector<size_t> order(rows_.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return prio_[a].tier < prio_[b].tier; });
    size_t limit = order.size();
    if (budget_kind_ == "tier") {
      limit = 0;
      while (limit < order.size() && prio_[order[limit]].tier <= budget_tier_) ++limit;
    } else if (budget_kind_ == "rows") {
      limit = std::min<size_t>(budget_rows_, order.size());
    }
    for (size_t pos = 0; pos < order.size(); ++pos) {
      position_[order[pos]] = static_cast<uint32_t>(pos + 1);
      if (pos < limit) planned_.push_back(order[pos]);
    }
    if (!v4_) {
      for (size_t i : planned_) plan_row(i);
    } else {
      select_controls();
      std::set<size_t> done;
      for (size_t i : planned_)
        if (done.insert(i).second) plan_row(i);
      for (size_t i : control_planned_)
        if (done.insert(i).second) plan_row(i);
      rows_planned_ = done;
    }
    plan_actors();
    plan_stop_events();
    if (v4_) plan_object_lifetimes();
    finish_sites();
    write(out);
    return kz::Status::success();
  }

 private:
  // ---------------------------------------------------------------- options and inputs
  kz::Status check_options() {
    if (o_.select != "observation-set") return kz::Status::error(kz::Err::Invalid, "--select: this revision has the selection observation-set only");
    if (o_.order != "method-priority") return kz::Status::error(kz::Err::Invalid, "--order: this revision has the order method-priority only");
    const std::string& b = o_.budget;
    if (b == "all") budget_kind_ = "all";
    else if (b == "tier:D07") budget_kind_ = "tier", budget_tier_ = 1;
    else if (b == "tier:directed") budget_kind_ = "tier", budget_tier_ = 2;
    else if (starts(b, "rows:") && b.size() > 5 && b.find_first_not_of("0123456789", 5) == std::string::npos)
      budget_kind_ = "rows", budget_rows_ = static_cast<uint32_t>(std::strtoul(b.c_str() + 5, nullptr, 10));
    else return kz::Status::error(kz::Err::Invalid, "--budget is tier:D07, tier:directed, all or rows:<n>");
    if (o_.budget_reason.empty()) return kz::Status::error(kz::Err::Invalid, "--budget-reason is required (a budget is recorded with its reason)");
    if (o_.manifest_revision != 3 && o_.manifest_revision != 4) return kz::Status::error(kz::Err::Invalid, "--manifest is 3 or 4");
    v4_ = o_.manifest_revision == 4;
    if (!v4_) {
      if (!o_.control_budget.empty() || !o_.control_budget_reason.empty() || !o_.namespace_data.empty() || !o_.requests_path.empty())
        return kz::Status::error(kz::Err::Invalid, "--control-budget, --control-budget-reason, --namespaces and --requests-out need --manifest 4");
      return kz::Status::success();
    }
    const std::string& cb = o_.control_budget;
    if (cb == "all") control_rows_ = UINT32_MAX;
    else if (starts(cb, "rows:") && cb.size() > 5 && cb.find_first_not_of("0123456789", 5) == std::string::npos)
      control_rows_ = static_cast<uint32_t>(std::strtoul(cb.c_str() + 5, nullptr, 10));
    else return kz::Status::error(kz::Err::Invalid, "--control-budget is all or rows:<n> (with --manifest 4)");
    if (o_.control_budget_reason.empty()) return kz::Status::error(kz::Err::Invalid, "--control-budget-reason is required (a budget is recorded with its reason)");
    if (o_.requests_path.empty()) return kz::Status::error(kz::Err::Invalid, "--requests-out is required with --manifest 4");
    return kz::Status::success();
  }

  // The framework-layer data file: lines "role <role> function=<name> def=<file>:<line> def_sha256=<hex> decl=<file>:<line>
  // decl_sha256=<hex> version=<text>" and comments.  Nothing else: the file names no other file to read.
  kz::Status read_framework() {
    std::istringstream in(framework_);
    std::string l;
    int n = 0;
    static const std::regex re(R"re(^role (run-loop|app-exit) function=(\S+) def=(\S+):(\d+) def_sha256=([0-9a-f]{64}) decl=(\S+):(\d+) decl_sha256=([0-9a-f]{64}) version="([^"]*)"$)re");
    while (std::getline(in, l)) {
      ++n;
      if (l.empty() || l[0] == '#') continue;
      std::smatch m;
      if (!std::regex_match(l, m, re))
        return kz::Status::error(kz::Err::Invalid, "framework-layer data line " + std::to_string(n) +
                                                        " is not an entry (role <run-loop|app-exit> function= def= def_sha256= decl= decl_sha256= version=); the generator reads no other file");
      FwEntry e;
      e.role = m[1].str();
      e.function = m[2].str();
      e.def_file = m[3].str();
      e.def_line = static_cast<uint32_t>(std::strtoul(m[4].str().c_str(), nullptr, 10));
      e.def_sha = m[5].str();
      e.decl = m[6].str() + ":" + m[7].str();
      e.version = m[9].str();
      e.line = n;
      fw_.push_back(e);
    }
    return kz::Status::success();
  }

  // The namespace data files (semantics/framework_namespaces_*.txt, written by the research scan env/tools/semantics_scan.py):
  // the statements below and comments; nothing else (the file names no other file).  Of an entry (a `kind` of the
  // object-id generation or a `table` of the table-copy generation) the generator keeps the functions that look up or
  // create (lookup, create, copy, create-check) and those that delete or retire (delete, clear) with their positions.
  kz::Status read_namespaces() {
    static const std::regex re_file(R"(^file (\S+) sha256=([0-9a-f]{64})$)");
    static const std::regex re_op(R"(^  (lookup|create|copy|create-check|delete|clear) (\S+) def=(\S+):(\d+)(?: at=(\S+):(\d+))?(?: public=\S+)?$)");
    static const std::set<std::string> other = {"semantics-data", "layer", "component", "rule", "generation", "compare", "chain-status", "end"};
    static const std::set<std::string> other_sub = {"stored-name", "extent", "other-store", "compare"};
    for (size_t fi = 0; fi < o_.namespace_data.size(); ++fi) {
      const auto& [path, text] = o_.namespace_data[fi];
      std::istringstream in(text);
      std::string l;
      int n = 0;
      NsFile f;
      f.path = path;
      NsEntry* cur = nullptr;
      while (std::getline(in, l)) {
        ++n;
        if (l.empty() || l[0] == '#') continue;
        std::smatch m;
        if (std::regex_match(l, m, re_file)) {
          f.sha[m[1].str()] = m[2].str();
          continue;
        }
        const std::string w = l.substr(0, l.find(' '));
        if (w == "kind" || w == "table") {
          f.entries.push_back(NsEntry{});
          cur = &f.entries.back();
          cur->name = l.substr(w.size() + 1);
          cur->name = cur->name.substr(0, cur->name.find(' '));
          cur->line = n;
          continue;
        }
        if (std::regex_match(l, m, re_op)) {
          if (cur == nullptr) return kz::Status::error(kz::Err::Invalid, "namespace data " + path + " line " + std::to_string(n) + ": an operation before its kind or table");
          NsOp op;
          op.what = m[1].str();
          op.fn = m[2].str();
          op.def_file = m[3].str();
          op.def_line = static_cast<uint32_t>(std::strtoul(m[4].str().c_str(), nullptr, 10));
          op.at_file = m[5].str();
          op.at_line = m[6].matched ? static_cast<uint32_t>(std::strtoul(m[6].str().c_str(), nullptr, 10)) : 0;
          op.line = n;
          cur->ops.push_back(op);
          continue;
        }
        const std::string sub = l.size() > 2 && l[0] == ' ' && l[1] == ' ' ? l.substr(2, l.find(' ', 2) - 2) : std::string();
        if (other.count(w) != 0 || (!sub.empty() && other_sub.count(sub) != 0)) continue;
        return kz::Status::error(kz::Err::Invalid, "namespace data " + path + " line " + std::to_string(n) + " is no statement of the namespace data; the generator reads no other file");
      }
      ns_.push_back(f);
    }
    return kz::Status::success();
  }

  kz::Status read_basis() {
    if (basis_c_ == nullptr) return kz::Status::success();
    kz::Status s;
    if (!(s = basis_c_->read_section(KZ_SEC_BASIS, &bitems_)).ok()) return s;
    if (!(s = basis_c_->read_section(KZ_SEC_TUS, &tus_)).ok()) return s;
    for (const kz_basis_item& b : bitems_) {
      std::string v;
      basis_c_->string(b.value, &v);
      std::string k;
      basis_c_->string(b.name, &k);
      bmap_[k] = v;
    }
    for (const kz_tu& t : tus_) {
      std::string src, mem;
      basis_c_->string(t.source, &src);
      basis_c_->string(t.member, &mem);
      tu_src_.push_back(src);
      tu_member_.push_back(mem);
    }
    // the common directory of the units outside the bundle root (absolute paths): a unit path is written relative to it
    std::vector<std::string> abs;
    for (const std::string& p : tu_src_)
      if (!p.empty() && p[0] == '/') abs.push_back(p);
    if (!abs.empty()) {
      std::string c = abs[0].substr(0, abs[0].rfind('/') + 1);
      for (const std::string& p : abs)
        while (!c.empty() && p.compare(0, c.size(), c) != 0) {
          c.pop_back();
          c = c.substr(0, c.rfind('/') + 1);
        }
      out_root_ = c;
    }
    return kz::Status::success();
  }

  // ---------------------------------------------------------------- facts helpers
  std::string plain(uint32_t fn) const { return fn < funcs_.size() ? str(funcs_[fn].name) : "-"; }
  std::string file_path(uint32_t fh) const { return fh < files_.size() ? str(files_[fh].path) : std::string(); }
  std::string file_sha(uint32_t fh) const { return fh < files_.size() ? kz::to_hex(files_[fh].sha256, 32) : std::string(); }
  // a unit path as a site's tu=: relative to the bundle root as recorded, or relative to the common directory of the
  // units outside it
  std::string tu_text(const std::string& p) const {
    if (!out_root_.empty() && p.compare(0, out_root_.size(), out_root_) == 0) return p.substr(out_root_.size());
    return p;
  }
  bool tu_unique(const std::string& tu) const {
    if (tu_src_.empty()) return true;
    size_t n = 0;
    for (const std::string& s : tu_src_)
      if (s == tu || (s.size() > tu.size() && s.compare(s.size() - tu.size(), tu.size(), tu) == 0 && s[s.size() - tu.size() - 1] == '/')) ++n;
    return n <= 1;
  }
  const kz_fact_arg* arg(uint32_t ci, uint32_t k) const {
    const kz_fact_call& c = calls_[ci];
    if (k >= c.nargs || c.first_arg + k >= args_.size()) return nullptr;
    return &args_[c.first_arg + k];
  }
  // the defined function whose body holds (file, line): the smallest range
  uint32_t fn_at(const std::string& file, uint32_t line) const {
    uint32_t best = kN, span = 0;
    for (size_t i = 0; i < funcs_.size(); ++i) {
      const kz_fact_func& f = funcs_[i];
      if (!defined(static_cast<uint32_t>(i)) || f.line > line || f.end_line < line || str(f.file) != file) continue;
      if (best == kN || f.end_line - f.line < span) best = static_cast<uint32_t>(i), span = f.end_line - f.line;
    }
    return best;
  }
  uint32_t fn_named(const std::string& name) const {
    const std::string base = base_name(name);
    auto it = fn_by_name_.find(base);
    if (it == fn_by_name_.end()) return kN;
    for (uint32_t f : it->second)
      if (defined(f) && fname(f) == name) return f;
    for (uint32_t f : it->second)
      if (defined(f)) return f;
    return kN;
  }
  bool returns_value(uint32_t ci) const {
    const kz_fact_call& c = calls_[ci];
    if (c.flags & (KZ_CALL_FLAG_RESULT_USED | KZ_CALL_FLAG_RESULT_TESTED)) return true;
    return defined(c.callee_fn) && retconsts_of_.count(c.callee_fn) != 0;
  }
  // the parameters of a function it stores through (PARAM_FLOW out-store rows, any depth), or the P0 out parameter
  std::set<uint32_t> out_params(uint32_t ci) const {
    std::set<uint32_t> out;
    const kz_fact_call& c = calls_[ci];
    if (defined(c.callee_fn)) {
      for (const auto& [key, rows] : pflow_of_)
        if (key.first == c.callee_fn)
          for (uint32_t r : rows)
            if (pflow_[r].kind == KZ_PFLOW_KIND_OUT_STORE) out.insert(key.second);
    } else {
      for (const kz_fact_p0& p : p0_)
        if (str(p.name) == str(c.callee) && p.out_param != kN) out.insert(p.out_param);
    }
    return out;
  }
  // parameters of fn whose value reaches a byte-string comparison of P0 (the key parameters of a name search)
  bool reaches_compare(uint32_t fn, uint32_t k) const {
    auto it = pflow_of_.find({fn, k});
    if (it == pflow_of_.end()) return false;
    for (uint32_t r : it->second) {
      const kz_fact_pflow& p = pflow_[r];
      if (p.kind != KZ_PFLOW_KIND_CALL_ARG) continue;
      const uint32_t sink = p.depth == 0 ? p.site : p.sink_site;
      auto cit = call_by_site_.find(sink);
      if (cit != call_by_site_.end() && compare_names_.count(str(calls_[cit->second].callee)) != 0) return true;
    }
    return false;
  }

  // ---------------------------------------------------------------- expressions
  std::string ex(uint32_t root, int depth = 0) const {
    if (root == kN) return "-";
    if (root >= exprs_.size()) return "expr#" + std::to_string(root) + "?";
    if (depth > 40) return "...";
    const kz_fact_expr& e = exprs_[root];
    const std::string name = str(e.name);
    const bool leaf = e.a == kN && e.b == kN;
    if (leaf && e.op == KZ_EXPR_OP_CONST) return std::to_string(e.value);
    if (leaf && e.op == KZ_EXPR_OP_LOAD) {
      std::string t = enum_text("KZ_EXPR_REF", e.flags) + ":" + name;
      if (e.flags == KZ_EXPR_REF_PARAM) t += "#" + std::to_string(e.value);
      return t;
    }
    std::string t = "(" + enum_text("KZ_EXPR_OP", e.op);
    if (!name.empty()) t += " " + name;
    if (e.op == KZ_EXPR_OP_CONST || e.op == KZ_EXPR_OP_FORALL || e.op == KZ_EXPR_OP_CURSOR) t += " " + std::to_string(e.value);
    if (e.a != kN) t += " " + ex(e.a, depth + 1);
    if (e.b != kN) t += " " + ex(e.b, depth + 1);
    return t + ")";
  }
  bool is_param(uint32_t r, uint32_t k) const {
    return r < exprs_.size() && exprs_[r].op == KZ_EXPR_OP_LOAD && exprs_[r].a == kN && exprs_[r].b == kN && exprs_[r].flags == KZ_EXPR_REF_PARAM &&
           static_cast<uint32_t>(exprs_[r].value) == k;
  }
  bool is_zero(uint32_t r) const { return r < exprs_.size() && exprs_[r].op == KZ_EXPR_OP_CONST && exprs_[r].a == kN && exprs_[r].b == kN && exprs_[r].value == 0; }
  // a conjunct of the guard (after the negations are moved in: not (or a b) = (and (not a) (not b))) says that parameter k
  // is not null: (ne param 0), (not (eq param 0)), or param as a truth value
  bool nonnull_test(uint32_t guard, uint32_t k, std::string* text) const {
    std::vector<std::pair<uint32_t, bool>> work = {{guard, false}};  // (node, negated)
    while (!work.empty()) {
      const auto [r, neg] = work.back();
      work.pop_back();
      if (r >= exprs_.size()) continue;
      const kz_fact_expr& e = exprs_[r];
      if ((e.op == KZ_EXPR_OP_AND && !neg) || (e.op == KZ_EXPR_OP_OR && neg)) {
        work.push_back({e.a, neg});
        work.push_back({e.b, neg});
        continue;
      }
      if (e.op == KZ_EXPR_OP_NOT) {
        work.push_back({e.a, !neg});
        continue;
      }
      bool hit = false;
      if (!neg && e.op == KZ_EXPR_OP_NE && ((is_param(e.a, k) && is_zero(e.b)) || (is_zero(e.a) && is_param(e.b, k)))) hit = true;
      if (neg && e.op == KZ_EXPR_OP_EQ && ((is_param(e.a, k) && is_zero(e.b)) || (is_zero(e.a) && is_param(e.b, k)))) hit = true;
      if (!neg && is_param(r, k)) hit = true;
      if (hit) {
        *text = (neg ? "(not " + ex(r) + ")" : ex(r)) + " in the guard " + ex(guard);
        return true;
      }
    }
    return false;
  }

  // ---------------------------------------------------------------- sites
  Site* site_for(Site s) {
    const std::string k = s.key();
    auto it = sites_.find(k);
    if (it == sites_.end()) it = sites_.emplace(k, s).first;
    return &it->second;
  }
  void add_val(Site* s, const Val& v) {
    for (Val& x : s->vals)
      if (x.from == v.from) {
        if (v.string_mode == "validated" && (x.string_mode != "validated" || v.bound < x.bound)) x = v;
        return;
      }
    if (s->vals.size() >= kSlots) {
      need(s->key(), "value " + v.from, "not-planned", reason(kSlotsFull), "");
      return;
    }
    s->vals.push_back(v);
  }
  // a call site: at-call or after-call of calls_[ci]; nullptr (with a gap) where the line has another call of the callee
  Site* call_site(uint32_t ci, bool after, const std::string& rule, const std::string& ref) {
    const kz_fact_call& c = calls_[ci];
    if (c.kind == KZ_CALL_KIND_INDIRECT || str(c.callee).empty()) return nullptr;
    // revision 2 binds by line and reports a second call of the callee on the line; revision 3 names the call by its
    // column and its ordinal among the calls of the callee in the function (source order)
    uint32_t nth = 0;
    for (uint32_t o : calls_of_.count(c.caller_fn) != 0 ? calls_of_.at(c.caller_fn) : std::vector<uint32_t>{}) {
      if (str(calls_[o].callee) != str(c.callee)) continue;
      if (v4_ && (calls_[o].line < c.line || (calls_[o].line == c.line && calls_[o].col <= c.col))) ++nth;
      if (!v4_ && o != ci && calls_[o].line == c.line) {
        gap(call_pos(ci), (after ? "after-call " : "at-call ") + str(c.callee) + " in " + plain(c.caller_fn), reason(kTwoCalls), rule);
        return nullptr;
      }
    }
    Site s;
    if (v4_) {
      s.col = c.col;
      s.nth = nth;
    }
    s.tu = tu_text(file_path(c.tu));
    if (!tu_unique(s.tu)) {
      gap(call_pos(ci), "unit " + s.tu, reason(kPathNotUnique), rule);
      return nullptr;
    }
    s.func = plain(c.caller_fn);
    s.phase = "point";
    s.place = after ? "after-call" : "at-call";
    s.callee = str(c.callee);
    s.line = c.line;
    s.op = "call." + s.callee;
    Site* p = site_for(s);
    p->rules.insert(rule);
    p->refs.insert(ref.empty() ? call_pos(ci) : ref);
    return p;
  }
  // at-call with the arguments (those asked for first, then the others in order, up to the slots)
  Site* at_call(uint32_t ci, const std::string& rule, const std::string& ref, const std::vector<Val>& asked = {}) {
    Site* s = call_site(ci, false, rule, ref);
    if (s == nullptr) return nullptr;
    for (const Val& v : asked) add_val(s, v);
    for (uint32_t k = 0; k < calls_[ci].nargs && s->vals.size() < kSlots; ++k) {
      Val v;
      v.role = "arg" + std::to_string(k);
      v.from = "arg" + std::to_string(k);
      add_val(s, v);
    }
    return s;
  }
  Site* after_call(uint32_t ci, const std::string& rule, const std::string& ref, const std::set<uint32_t>& outs = {}, const std::vector<Val>& asked = {}) {
    Site* s = call_site(ci, true, rule, ref);
    if (s == nullptr) return nullptr;
    for (const Val& v : asked) add_val(s, v);
    if (returns_value(ci)) {
      Val v;
      v.role = "status";
      v.from = "ret";
      v.type = "s64";
      add_val(s, v);
    }
    for (uint32_t k : outs) {
      if (k >= calls_[ci].nargs) continue;
      Val v;
      v.role = "out" + std::to_string(k);
      v.from = "out" + std::to_string(k);
      add_val(s, v);
    }
    return s;
  }
  Site* entry_site(uint32_t fn, const std::string& rule, const std::string& ref, bool params) {
    if (!defined(fn)) return nullptr;
    Site s;
    s.tu = tu_text(file_path(funcs_[fn].tu));
    if (!tu_unique(s.tu)) {
      gap(fname(fn), "unit " + s.tu, reason(kPathNotUnique), rule);
      return nullptr;
    }
    s.func = plain(fn);
    s.phase = "before";
    s.op = "entry." + s.func;
    Site* p = site_for(s);
    p->rules.insert(rule);
    p->refs.insert(ref);
    if (params)
      for (uint32_t k = 0; k < funcs_[fn].nparams && k < kSlots; ++k) {
        Val v;
        v.role = "param" + std::to_string(k);
        v.from = "arg" + std::to_string(k);
        add_val(p, v);
      }
    return p;
  }
  Site* store_site(uint32_t fn, uint32_t line, const std::string& rule, const std::string& ref) {
    if (!defined(fn) || line == 0) return nullptr;
    Site s;
    s.tu = tu_text(file_path(funcs_[fn].tu));
    if (!tu_unique(s.tu)) {
      gap(ref, "unit " + s.tu, reason(kPathNotUnique), rule);
      return nullptr;
    }
    s.func = plain(fn);
    s.phase = "effect";
    s.place = "store";
    s.line = line;
    s.op = "store." + s.func;
    Site* p = site_for(s);
    p->rules.insert(rule);
    p->refs.insert(ref);
    Val v0, v1;
    v0.role = "value";
    v0.from = "operand0";
    v1.role = "address";
    v1.from = "operand1";
    add_val(p, v0);
    add_val(p, v1);
    return p;
  }
  // manifest revision 4: right after each load of field (Type.field) on the line in fn: the value and the address
  Site* load_site(uint32_t fn, uint32_t line, const std::string& field, const std::string& rule, const std::string& ref) {
    if (!defined(fn) || line == 0 || field.find('.') == std::string::npos) return nullptr;
    Site s;
    s.tu = tu_text(file_path(funcs_[fn].tu));
    if (!tu_unique(s.tu)) {
      gap(ref, "unit " + s.tu, reason(kPathNotUnique), rule);
      return nullptr;
    }
    s.func = plain(fn);
    s.phase = "point";
    s.place = "load";
    s.line = line;
    s.field = field;
    s.op = "load." + s.func;
    Site* p = site_for(s);
    p->rules.insert(rule);
    p->refs.insert(ref);
    Val v0, v1;
    v0.role = "value";
    v0.from = "operand0";
    v1.role = "address";
    v1.from = "operand1";
    v1.type = "ptr";
    add_val(p, v0);
    add_val(p, v1);
    return p;
  }
  // the first statement of each run of the line's statements in fn (a wait's exit statement)
  Site* line_site(uint32_t fn, uint32_t line, const std::string& rule, const std::string& ref) {
    if (!defined(fn) || line == 0) return nullptr;
    Site s;
    s.tu = tu_text(file_path(funcs_[fn].tu));
    if (!tu_unique(s.tu)) {
      gap(ref, "unit " + s.tu, reason(kPathNotUnique), rule);
      return nullptr;
    }
    s.func = plain(fn);
    s.phase = "point";
    s.place = "line-start";
    s.line = line;
    s.op = "line." + s.func;
    Site* p = site_for(s);
    p->rules.insert(rule);
    p->refs.insert(ref);
    return p;
  }
  // need and gap lines, each once, in the order first written
  void need(const std::string& subject, const std::string& what, const std::string& state, const std::string& why, const std::string& note,
            const std::set<std::string>& sites = {}) {
    if (seen_.insert("n|" + subject + "|" + what + "|" + state + "|" + why + "|" + note).second) needs_.push_back(Need{subject, what, state, why, note, sites});
  }
  // a planned need (manifest revision 4) with the keys of the sites that record it, or the not-planned need
  void need_sites(const std::string& subject, const std::string& what, const std::set<std::string>& sites, R why_not, const std::string& note_not,
                  const std::string& note = "") {
    if (!sites.empty()) need(subject, what, "planned", "-", note, sites);
    else need(subject, what, "not-planned", reason(why_not), note_not);
  }
  void gap(const std::string& subject, const std::string& what, const std::string& why, const std::string& rule) {
    if (seen_.insert("g|" + subject + "|" + what + "|" + why + "|" + rule).second) gaps_.push_back(Need{subject, what, "gap", why, rule, {}});
  }

  // ---------------------------------------------------------------- rules over a row
  // positions file:line:col in a text, as (file, line, col)
  std::vector<std::tuple<std::string, uint32_t, uint32_t>> positions(const std::string& t) const {
    static const std::regex re(R"(([^\s(\[]+):(\d+):(\d+))");
    std::vector<std::tuple<std::string, uint32_t, uint32_t>> v;
    for (std::sregex_iterator it(t.begin(), t.end(), re), end; it != end; ++it)
      v.emplace_back((*it)[1].str(), static_cast<uint32_t>(std::strtoul((*it)[2].str().c_str(), nullptr, 10)), static_cast<uint32_t>(std::strtoul((*it)[3].str().c_str(), nullptr, 10)));
    return v;
  }
  uint32_t call_of(const std::tuple<std::string, uint32_t, uint32_t>& p) const {
    auto it = call_at_.find(p);
    return it == call_at_.end() ? kN : it->second;
  }
  static std::string pos_text(const std::tuple<std::string, uint32_t, uint32_t>& p) {
    return std::get<0>(p) + ":" + std::to_string(std::get<1>(p)) + ":" + std::to_string(std::get<2>(p));
  }
  // the actor names a line lists ("{name (kind), ...}")
  void actors_in(const std::string& t) {
    static const std::regex re(R"(([A-Za-z_][A-Za-z0-9_]*(?:@\S+?)?) \((script-entry|spawn-entry|table-image-entry|table-walk|trampoline|[a-z-]+)\))");
    for (std::sregex_iterator it(t.begin(), t.end(), re), end; it != end; ++it) {
      const std::string kind = (*it)[2].str();
      if (kind == "process" || kind == "no root reaches it") continue;
      const uint32_t f = fn_named((*it)[1].str());
      if (f != kN && root_kind_.count(f) != 0 && root_kind_.at(f) != "process") actor_roots_.insert(f);
    }
  }

  // use-call and key-text for one call of a key-parameter function (the row's use or a creator)
  void key_call(uint32_t ci, uint32_t kfn, uint32_t kparam, const std::string& rule, const std::string& rowref) {
    Val key;
    key.role = "key";
    key.from = "arg" + std::to_string(kparam);
    key.type = "ptr";
    key.string_mode = "pointer-only";
    // revision 2: a key argument that is a character array of a known extent which the program reads at a point that
    // dominates the call (ARG_TYPES, facts of --object-access) is a validated string at the call itself, bounded by
    // that extent and the runtime's copy bound: the copy stays inside the array the program itself reads
    if (kparam != kN) {
      auto ait = argtype_.find({ci, kparam});
      if (ait != argtype_.end() && ait->second.type_class == KZ_ARGT_CLASS_CHAR_ARRAY && ait->second.extent > 0 && ait->second.read_dom == KZ_ARGT_READ_DOMINATING) {
        const kz_fact_argtype& a = ait->second;
        key.string_mode = "validated";
        key.bound = std::min<uint32_t>(a.extent, kStrMax);
        std::string rd = "a read";
        if (a.read < reads_.size()) rd = "the read at " + pos(str(reads_[a.read].file), reads_[a.read].line, reads_[a.read].col);
        key.ref = "argument " + std::to_string(kparam) + " is the character array " + str(a.decl_object) + " of extent " + std::to_string(a.extent) +
                  (a.decl_fh != KZ_NONE32 ? " (declared at " + file_path(a.decl_fh) + ":" + std::to_string(a.decl_line) + ")" : std::string()) + ", " + rd +
                  " dominates the call (ARG_TYPES); bound min(extent " + std::to_string(a.extent) + ", KZ_RT_STR_MAX " + std::to_string(kStrMax) +
                  "); premise: the string is NUL-terminated within the array or the cut is flagged";
      }
    }
    const bool string_key = kparam != kN && defined(kfn) && reaches_compare(kfn, kparam);
    at_call(ci, rule, rowref, kparam != kN ? std::vector<Val>{key} : std::vector<Val>{});
    if (Site* a = after_call(ci, rule, rowref, out_params(ci))) last_after_.insert(a->key());
    if (!string_key) return;
    // key-text: the first call (by position) the key parameter reaches at depth 0 under a non-null test of it
    uint32_t best = kN, best_n = 0;
    std::string test;
    auto it = pflow_of_.find({kfn, kparam});
    if (it != pflow_of_.end())
      for (uint32_t r : it->second) {
        const kz_fact_pflow& p = pflow_[r];
        if (p.depth != 0 || p.kind != KZ_PFLOW_KIND_CALL_ARG || p.guard_root == kN) continue;
        auto cit = call_by_site_.find(p.site);
        if (cit == call_by_site_.end()) continue;
        std::string t;
        if (!nonnull_test(p.guard_root, kparam, &t)) continue;
        const uint32_t c = cit->second;
        if (best == kN || calls_[c].line < calls_[best].line || (calls_[c].line == calls_[best].line && calls_[c].col < calls_[best].col))
          best = c, best_n = p.next_param, test = t;
      }
    if (best == kN) {
      gap(rowref, "key text of " + plain(kfn) + " parameter " + std::to_string(kparam) + ": no non-null test of it before a call in the facts; recorded as a pointer", reason(kKeyText), "key-text");
      return;
    }
    // the bound: the program's own constant length check of the parameter in the same function (memchr(p, 0, N), ISO C
    // 7.24.5.1; strnlen(p, N), POSIX.1-2017), else the runtime's copy bound
    uint32_t bound = kStrMax;
    std::string bref = "KZ_RT_STR_MAX " + std::to_string(kStrMax) + " (runtime/include/kz_rt.h:82; the runtime copies up to the NUL or the bound and flags a cut)";
    for (uint32_t r : it->second) {
      const kz_fact_pflow& p = pflow_[r];
      if (p.depth != 0 || p.kind != KZ_PFLOW_KIND_CALL_ARG) continue;
      auto cit = call_by_site_.find(p.site);
      if (cit == call_by_site_.end()) continue;
      const uint32_t c = cit->second;
      const std::string cal = str(calls_[c].callee);
      const kz_fact_arg* len = nullptr;
      if (cal == "memchr" && p.next_param == 0) {
        const kz_fact_arg* ch = arg(c, 1);
        if (ch != nullptr && ch->kind == KZ_ARG_KIND_CONST && ch->value == 0) len = arg(c, 2);
      } else if (cal == "strnlen" && p.next_param == 0) {
        len = arg(c, 1);
      }
      if (len == nullptr || len->kind != KZ_ARG_KIND_CONST || len->value <= 0 || (calls_[c].flags & KZ_CALL_FLAG_RESULT_TESTED) == 0) continue;
      if (static_cast<uint64_t>(len->value) < bound) {
        bound = static_cast<uint32_t>(len->value);
        bref = cal + " of the parameter with the length " + std::to_string(len->value) + " at " + call_pos(c) + ", its result tested";
      }
    }
    Val v;
    v.role = "key_text";
    v.from = "arg" + std::to_string(best_n);
    v.type = "ptr";
    v.string_mode = "validated";
    v.bound = bound;
    v.ref = "non-null test " + test + " of parameter " + std::to_string(kparam) + " of " + plain(kfn) + " before " + call_pos(best) + "; bound " + bref +
            "; premise: the string is NUL-terminated within the bound or the cut is flagged";
    at_call(best, "key-text", rowref, {v});
  }

  void plan_row(size_t i) {
    const RelRow& r = *rows_[i];
    const std::string rowref = "row " + r.key;
    const std::string text = render_row(r);
    actors_in(text);
    std::vector<std::string> lines;
    {
      std::istringstream in(text);
      std::string l;
      while (std::getline(in, l)) {
        size_t a = l.find_first_not_of(' ');
        lines.push_back(a == std::string::npos ? std::string() : l.substr(a));
      }
    }
    const size_t before = sites_.size();
    bool probe = false;
    for (const std::string& l : lines) probe = probe || starts(l, "handling: probe-create");
    std::set<std::string> use_after;  // manifest revision 4: the after-call sites of the row's use
    if (r.path == 'A') {
      last_after_.clear();
      if (r.call != kN) key_call(r.call, r.callee_fn, r.param, "use-call", rowref);
      use_after = last_after_;
      for (const std::string& l : lines) {
        // revision 3: the creates the row's not-a-pair lines exclude by key constants (and, in a probe-create row, the
        // row's own create) are creator sites as well (control selection rule, docs/plan_manifest.md section 7)
        const bool creator_line = starts(l, "creator:") ||
                                  (v4_ && (starts(l, "not a pair: key constants differ") || (probe && starts(l, "not a pair: probe-create"))));
        if (creator_line) {
          std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(l);
          uint32_t ci = ps.empty() ? kN : call_of(ps[0]);
          if (v4_)
            for (size_t k = 1; ci == kN && k < ps.size(); ++k) ci = call_of(ps[k]);
          if (v4_ && ci != kN) row_creators_[i].insert(ci);
          if (ci == kN) {
            gap(rowref, "creator position", reason(kNoPos), "creator-call");
            continue;
          }
          uint32_t kp = kN;
          const uint32_t cf = calls_[ci].callee_fn;
          if (defined(cf))
            for (uint32_t k = 0; k < funcs_[cf].nparams; ++k)
              if (reaches_compare(cf, k)) {
                kp = k;
                break;
              }
          key_call(ci, cf, kp, "creator-call", rowref);
        } else if (v4_ && starts(l, "key clearing: ")) {
          // plan rules revision 4: a call relations names key-clearing (every write of the key it reaches stores a
          // constant) is a delete or reset of the row's object kind: an object-delete site, no creator site
          std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(l);
          const uint32_t ci = ps.empty() ? kN : call_of(ps[0]);
          if (ci == kN) {
            gap(rowref, "key-clearing position", reason(kNoPos), "object-delete");
            continue;
          }
          if (Site* st = at_call(ci, "object-delete", rowref + " key-clearing call " + call_pos(ci), {})) row_clearing_[i].insert(st->key());
        } else if (starts(l, "handling:")) {
          handling(l.substr(10), r, rowref);
        }
      }
      if (!v4_)
        need(rowref, "event-send", "not-planned", reason(kNotPlanned), "the effects of a result (an event or a message sent on it) are not in the facts of this revision");
    } else if (r.path == 'B') {
      for (uint32_t ci : r.site_calls) {
        Val v;
        v.role = "field_value";
        v.from = "arg" + std::to_string(r.param);
        at_call(ci, "use-call", rowref, r.param != kN ? std::vector<Val>{v} : std::vector<Val>{});
        if (Site* a = after_call(ci, "use-call", rowref)) use_after.insert(a->key());
      }
      static const std::regex re_w(R"(^(assign|inc-dec|aggregate-assign|memset|memcpy|out-pointer|external|unknown_effect) at (\S+) in (\S+))");
      for (const std::string& l : lines) {
        std::smatch m;
        if (!std::regex_search(l, m, re_w)) continue;
        writer(m[1].str(), m[2].str(), m[3].str(), r, rowref);
      }
    } else {
      if (r.call != kN) {
        at_call(r.call, "spawn-call", rowref);
        after_call(r.call, "spawn-call", rowref, out_params(r.call));
      }
      for (const std::string& l : lines)
        if (starts(l, "write after the spawn:")) {
          std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(l);
          if (ps.empty() || r.caller_fn == kN) continue;
          store_site(r.caller_fn, std::get<1>(ps[0]), "writer-after-spawn", rowref + " " + pos_text(ps[0]));
        }
      if (!v4_) need(rowref, "read-value", "not-planned", reason(kNotPlanned), "a read of a field is no site kind of the manifest revision 3 (loads are not bound)");
      else read_rules(r, rowref);
    }
    if (!v4_) {
      need(rowref, "wait-result", "not-planned", reason(kNotPlanned), "rule added in the manifest revision 4");
      need(rowref, "lock-result", "not-planned", reason(kNotPlanned), "rule added in the manifest revision 4");
      (void)before;
      return;
    }
    // ---- plan rules revision 3
    if (r.path != 'C') need_sites(rowref, "use-result", use_after, kNoPos, "no after-call site of the row's use (see the gap lines)");
    if (r.path != 'C') requests_for(i, r, rowref, lines);
    // the waits the row's sites follow in their functions
    std::vector<std::pair<uint32_t, uint32_t>> anchors;
    auto anchor_call = [&](uint32_t ci) {
      if (ci != kN && ci < calls_.size()) anchors.emplace_back(calls_[ci].caller_fn, calls_[ci].line);
    };
    if (r.path == 'A') {
      anchor_call(r.call);
      for (uint32_t ci : row_creators_[i]) anchor_call(ci);
    } else if (r.path == 'B') {
      for (uint32_t ci : r.site_calls) anchor_call(ci);
    } else {
      if (r.read != kN && r.read < reads_.size()) anchors.emplace_back(reads_[r.read].fn, reads_[r.read].line);
      anchor_call(r.call);
    }
    waits_for(rowref, anchors);
    if (r.path != 'C') need(rowref, "lock-result", "not-planned", reason(kNoRule), "the lock rule of this revision plans the lock calls of path C rows (the creator's and the reader's)");
    else lock_rules(r, rowref);
  }

  // ---------------------------------------------------------------- plan rules revision 3: path C reads and locks
  // the load of the field the row reads, and the calls whose results are the subscripts on the read's access path
  // (OBJECT_ACCESS index root; READS index root when the facts have no OBJECT_ACCESS row for the read)
  void read_rules(const RelRow& r, const std::string& rowref) {
    std::set<std::string> keys;
    if (r.read == kN || r.read >= reads_.size()) {
      need(rowref, "read-value", "not-planned", reason(kNoPos), "the row names no READS row");
      return;
    }
    const kz_fact_read& rd = reads_[r.read];
    const std::string field = str(rd.type) + "." + str(rd.field);
    const std::string rref = rowref + " read " + pos(str(rd.file), rd.line, rd.col);
    if (Site* s = load_site(rd.fn, rd.line, field, "read-load", rref)) keys.insert(s->key());
    need_sites(rowref, "read-value", keys, kNoPos, "the read's position names no defined function of the facts");
    uint32_t ir = kN;
    std::string src;
    for (const kz_fact_objacc& o : objacc_)
      if (o.read == r.read && o.index_root != kN) {
        ir = o.index_root;
        src = "OBJECT_ACCESS";
        break;
      }
    if (ir == kN && rd.index_root != kN) ir = rd.index_root, src = "READS";
    std::set<std::string> callees;
    collect_calls(ir, &callees, 0);
    std::set<std::string> ik;
    for (const std::string& x : callees) {
      // the call of x in the reading function at or before the read's line, the last such call
      uint32_t best = kN;
      for (uint32_t ci : calls_of_.count(rd.fn) != 0 ? calls_of_.at(rd.fn) : std::vector<uint32_t>{})
        if (str(calls_[ci].callee) == x && calls_[ci].line <= rd.line && (best == kN || calls_[ci].line > calls_[best].line ||
                                                                         (calls_[ci].line == calls_[best].line && calls_[ci].col > calls_[best].col)))
          best = ci;
      if (best == kN) {
        gap(rowref, "subscript call " + x + " of the read: no call of it in " + plain(rd.fn) + " before the read", reason(kNoPos), "object-index");
        continue;
      }
      if (Site* s = after_call(best, "object-index", rref + " subscript (" + src + ") " + x)) ik.insert(s->key());
    }
    if (ir == kN) need(rowref, "read-subscript", "not-planned", reason(kNoRule), "the read's access path has no subscript (OBJECT_ACCESS, READS)");
    else need_sites(rowref, "read-subscript", ik, kNoRule, "no subscript of the read is a call result (" + src + " index root " + ex(ir) + ")");
  }
  // the call-result names of an expression
  void collect_calls(uint32_t root, std::set<std::string>* out, int depth) const {
    if (root == kN || root >= exprs_.size() || depth > 40) return;
    const kz_fact_expr& e = exprs_[root];
    if (e.op == KZ_EXPR_OP_CALL_RESULT && !str(e.name).empty()) out->insert(base_name(str(e.name)));
    collect_calls(e.a, out, depth + 1);
    collect_calls(e.b, out, depth + 1);
  }
  // the field loads (Type.field) of an expression, outside the arguments of a call result (the call's after-call
  // records its result; its arguments are no read of the condition)
  void collect_fields(uint32_t root, std::set<std::string>* out, int depth) const {
    if (root == kN || root >= exprs_.size() || depth > 40) return;
    const kz_fact_expr& e = exprs_[root];
    if (e.op == KZ_EXPR_OP_CALL_RESULT) return;
    if (e.op == KZ_EXPR_OP_LOAD && e.flags == KZ_EXPR_REF_FIELD && str(e.name).find('.') != std::string::npos) out->insert(str(e.name));
    collect_fields(e.a, out, depth + 1);
    collect_fields(e.b, out, depth + 1);
  }
  // the lock calls of the creator's and the reader's functions: the calls of a lock operation of LOCK_OPS (a primitive of
  // the P0 vocabulary or an operation derived from one), after-call of an acquisition or a wait (its result and its first
  // argument), at-call of a release
  void lock_rules(const RelRow& r, const std::string& rowref) {
    std::map<std::string, uint8_t> op_kind;
    for (const kz_fact_lockop& o : lockops_)
      if (o.fn != kN && o.fn < funcs_.size() && o.kind != KZ_LOCKOP_KIND_NONE) op_kind.emplace(str(funcs_[o.fn].name), o.kind);
    std::set<std::string> keys;
    for (uint32_t fn : {r.caller_fn, r.read_fn}) {
      if (fn == kN || calls_of_.count(fn) == 0) continue;
      for (uint32_t ci : calls_of_.at(fn)) {
        const auto it = op_kind.find(str(calls_[ci].callee));
        if (it == op_kind.end()) continue;
        const std::string lref = rowref + " lock " + enum_text("KZ_LOCKOP_KIND", it->second) + " " + call_pos(ci);
        Val v;
        v.role = "lock";
        v.from = "arg0";
        const std::vector<Val> lv = calls_[ci].nargs > 0 ? std::vector<Val>{v} : std::vector<Val>{};
        Site* st = it->second == KZ_LOCKOP_KIND_RELEASE ? at_call(ci, "lock-call", lref, lv) : after_call(ci, "lock-call", lref, {}, lv);
        if (st != nullptr) keys.insert(st->key());
      }
    }
    need_sites(rowref, "lock-result", keys, kNotPlanned, "no call of a lock operation of LOCK_OPS in the creator's or the reader's function");
  }

  // ---------------------------------------------------------------- plan rules revision 3: waits
  void waits_for(const std::string& rowref, const std::vector<std::pair<uint32_t, uint32_t>>& anchors) {
    std::set<uint32_t> ws;
    for (const auto& [fn, line] : anchors)
      for (size_t w = 0; w < loops_.size(); ++w) {
        const kz_fact_loop& L = loops_[w];
        if (fn == kN || L.fn != fn || L.line > line) continue;
        if (L.kind != KZ_LOOP_KIND_POLL && L.kind != KZ_LOOP_KIND_COUNT_BOUNDED) continue;
        ws.insert(static_cast<uint32_t>(w));
      }
    if (ws.empty()) {
      need(rowref, "wait-result", "not-planned", reason(kNotPlanned), "no poll or count-bounded loop (FACT_LOOPS) at or before the row's sites in their functions");
      return;
    }
    for (uint32_t w : ws) plan_wait(w, rowref);
  }
  bool in_loop(uint32_t loop_index, uint32_t l) const {
    for (int d = 0; l != kN && d < 64; ++d) {
      if (l == loop_index) return true;
      auto it = loop_by_index_.find(l);
      if (it == loop_by_index_.end()) return false;
      l = loops_[it->second].parent;
    }
    return false;
  }
  void plan_wait(uint32_t w, const std::string& rowref) {
    const kz_fact_loop& L = loops_[w];
    const std::string wpos = pos(str(L.file), L.line, L.col);
    const std::string what = "wait-result " + wpos;
    const std::string wref = rowref + " wait " + wpos;
    std::string form = L.pred_form != KZ_PRED_FORM_NONE ? "FACT_LOOPS " + enum_text("KZ_PRED_FORM", L.pred_form) : std::string();
    auto hit = loopdef_head_.find(L.site);
    const bool by_defs = hit != loopdef_head_.end() && loopdefs_[hit->second].status == KZ_LOOPDEF_STATUS_RECOGNISED;
    if (form.empty() && by_defs) form = "LOOP_DEFS " + str(loopdefs_[hit->second].pred);
    if (form.empty()) {
      const std::string ld = hit == loopdef_head_.end() ? std::string("no LOOP_DEFS row") : "LOOP_DEFS " + enum_text("KZ_LOOPDEF_STATUS", loopdefs_[hit->second].status);
      need(rowref, what, "not-planned", reason(kWaitPred), "FACT_LOOPS predicate form none; " + ld);
      return;
    }
    std::set<std::string> keys;
    const uint32_t fn = L.fn;
    // the calls whose results the condition reads, and the fields it loads
    std::set<std::string> callees, fields;
    collect_calls(L.exit_root, &callees, 0);
    collect_fields(L.exit_root, &fields, 0);
    if (by_defs)
      for (const kz_fact_loopdef& d : loopdefs_) {
        // the definitions in the body and in the condition: those the predicate reads in an iteration
        if (d.loop != L.site || str(d.local).empty() || d.value_root == kN) continue;
        if (d.where != KZ_LOOPDEF_WHERE_IN_BODY && d.where != KZ_LOOPDEF_WHERE_IN_CONDITION) continue;
        std::set<std::string> dc, dfl;
        collect_calls(d.value_root, &dc, 0);
        collect_fields(d.value_root, &dfl, 0);
        for (uint32_t ci : calls_of_.count(fn) != 0 ? calls_of_.at(fn) : std::vector<uint32_t>{})
          if (calls_[ci].line == d.line && dc.count(base_name(str(calls_[ci].callee))) != 0)
            if (Site* s = after_call(ci, "wait-cond", wref + " definition " + pos(str(L.file), d.line, d.col))) keys.insert(s->key());
        for (const std::string& f : dfl)
          if (Site* s = load_site(fn, d.line, f, "wait-cond", wref + " definition " + pos(str(L.file), d.line, d.col))) keys.insert(s->key());
      }
    auto ait = awaits_of_.find(fn);
    if (ait != awaits_of_.end())
      for (uint32_t a : ait->second)
        if (awaits_[a].loop == L.site) collect_fields(awaits_[a].relevant_root, &fields, 0);
    for (uint32_t ci : calls_of_.count(fn) != 0 ? calls_of_.at(fn) : std::vector<uint32_t>{})
      if (in_loop(L.loop, calls_[ci].loop) && callees.count(base_name(str(calls_[ci].callee))) != 0)
        if (Site* s = after_call(ci, "wait-cond", wref)) keys.insert(s->key());
    for (const kz_fact_read& rd : reads_) {
      if (rd.fn != fn || !in_loop(L.loop, rd.loop)) continue;
      const std::string f = str(rd.type) + "." + str(rd.field);
      if (fields.count(f) != 0)
        if (Site* s = load_site(fn, rd.line, f, "wait-cond", wref)) keys.insert(s->key());
    }
    // the exits taken: the exit statements (break, return, goto); a condition exit is no statement
    auto eit = loopexits_of_.find(L.loop);
    if (eit != loopexits_of_.end())
      for (uint32_t e : eit->second) {
        const kz_fact_loopexit& x = loopexits_[e];
        if (x.exit == kN) continue;
        if (x.kind == KZ_LOOPEXIT_KIND_BREAK || x.kind == KZ_LOOPEXIT_KIND_RETURN || x.kind == KZ_LOOPEXIT_KIND_GOTO) {
          if (Site* s = line_site(fn, x.line, "wait-exit", wref + " exit " + std::to_string(x.exit) + " " + enum_text("KZ_LOOPEXIT_KIND", x.kind))) keys.insert(s->key());
        } else {
          gap(wref, "exit " + std::to_string(x.exit) + " (" + enum_text("KZ_LOOPEXIT_KIND", x.kind) + ") at " + pos(str(x.file), x.line, x.col), reason(kExitNoStmt), "wait-exit");
        }
      }
    need_sites(rowref, what, keys, kWaitPred, "the predicate (" + form + ") names no call or field load of the loop in the facts", "predicate " + form);
  }

  // ---------------------------------------------------------------- plan rules revision 3: effects and value requests
  // The effect calls of the row's use (RESULT_EFFECT rows of the call with an event send, a system log, an application
  // exit or a terminal and its call), at-call with the argument a SEMANTICS arg-role row names; and, for a row the method
  // cannot label from the facts (handling label T?, J? or F, or an open effect of its absence line), the value requests:
  // what value of the record would decide it and which site records it.
  void requests_for(size_t i, const RelRow& r, const std::string& rowref, const std::vector<std::string>& lines) {
    std::string label;
    bool open = false;
    for (const std::string& l : lines) {
      if (starts(l, "handling_label: ")) {
        label = l.substr(16);
        label = label.substr(0, label.find(' '));
      }
      if (starts(l, "absence:") && l.find(" open") != std::string::npos) open = true;
    }
    const std::string lab = label.empty() ? prio_[i].label : label;
    const bool asked = lab == "T?" || lab == "J?" || lab == "F" || open;
    const std::vector<uint32_t> uses = r.path == 'A' ? (r.call != kN ? std::vector<uint32_t>{r.call} : std::vector<uint32_t>{}) : r.site_calls;
    const std::string id = std::string(1, r.path) + std::to_string(r.number);
    const std::string why = "label " + lab + (open ? ", an open effect in its absence line" : "");
    std::set<std::string> effect_keys, other_effect_keys;
    if (asked) ++request_rows_;
    auto request = [&](const std::string& value, const std::string& what, const std::string& state, const std::string& site, const std::string& slot,
                       const std::string& reason_text, const std::string& note) {
      if (asked) requests_.push_back(Request{r.key, id, lab, value, what, state, site, slot, reason_text, note + (note.empty() ? "" : "; ") + why});
    };
    for (uint32_t ci : uses) {
      if (ci == kN || ci >= calls_.size()) continue;
      const std::string cpos = call_pos(ci);
      // the result and the out values at the after-call of the use (planned by the use-call rule)
      Site* a = find_call_site(ci, true);
      if (a == nullptr) request("result", "the result of " + str(calls_[ci].callee) + " at " + cpos, "not-planned", "-", "-", reason(kNoPos), "no after-call site of the use");
      else
        for (const Val& v : a->vals) request(v.from == "ret" ? "result" : v.from, v.role + " of " + str(calls_[ci].callee) + " at " + cpos, "planned", a->key(), v.from, "-", "");
      auto eit = reffect_of_site_.find(calls_[ci].site);
      if (eit != reffect_of_site_.end())
        for (uint32_t e : eit->second) {
          const kz_fact_reffect& x = reffect_[e];
          if (x.effect != KZ_REFF_EFFECT_EVENT_SEND && x.effect != KZ_REFF_EFFECT_SYSLOG && x.effect != KZ_REFF_EFFECT_EXIT_APP && x.effect != KZ_REFF_EFFECT_TERMINAL)
            continue;
          const std::string ek = enum_text("KZ_REFF_EFFECT", x.effect);
          const std::string otext = enum_text("KZ_REFF_OUTCOME", x.outcome) + " " + std::to_string(x.outcome_value);
          auto cit = x.effect_site == kN ? call_by_site_.end() : call_by_site_.find(x.effect_site);
          if (cit == call_by_site_.end()) {
            request("effect", ek + " on the outcome " + otext, "not-planned", "-", "-", reason(kNoRule), "the effect has no call in the facts");
            continue;
          }
          const uint32_t ec = cit->second;
          std::vector<Val> asked_vals;
          std::string slot = "-";
          auto rit = arg_roles_.find(str(calls_[ec].callee));
          if (rit != arg_roles_.end())
            for (const auto& [k, role] : rit->second) {
              Val v;
              v.role = role;
              v.from = "arg" + std::to_string(k);
              asked_vals.push_back(v);
              if (slot == "-") slot = v.from;
            }
          const std::string eref = rowref + " effect " + ek + " " + call_pos(ec) + " on " + otext + " of " + cpos +
                                   (x.status == KZ_REFF_STATUS_OPEN ? " (open: " + str(x.reason) + ")" : std::string());
          Site* es = at_call(ec, "effect-call", eref, asked_vals);
          if (es == nullptr) {
            request("effect", ek + " " + call_pos(ec), "not-planned", "-", "-", reason(kNoPos), "no at-call site of the effect call (see the gap lines)");
            continue;
          }
          // plan rules revision 4: the event sends on the row's failure and absence outcomes are its event-send need; those
          // on another outcome (the success value's, often open effects reached on a later pass) are a need of their own
          if (x.effect == KZ_REFF_EFFECT_EVENT_SEND || x.effect == KZ_REFF_EFFECT_SYSLOG)
            (x.outcome == KZ_REFF_OUTCOME_FAILURE || x.outcome == KZ_REFF_OUTCOME_ABSENCE ? effect_keys : other_effect_keys).insert(es->key());
          request("effect", ek + " " + str(calls_[ec].callee) + " at " + call_pos(ec) + " on the outcome " + otext, "planned", es->key(), slot, "-",
                  x.status == KZ_REFF_STATUS_OPEN ? "open: " + str(x.reason) : "complete");
        }
      request("branch", "the branch the result takes at " + cpos, "not-planned", "-", "-", reason(kNoRule),
              "a branch is no site kind of the manifest; the result and the effect calls reached stand for it");
    }
    if (r.path == 'A') {
      need_sites(rowref, "event-send", effect_keys, kNotPlanned, "no event send or system log of RESULT_EFFECT with a call on a failure or absence outcome of the row's use");
      for (const std::string& k : effect_keys) other_effect_keys.erase(k);
      if (!other_effect_keys.empty()) need_sites(rowref, "event-send-other-outcome", other_effect_keys, kNotPlanned, "-");
    }
  }
  // the site of a call already planned (nullptr when none)
  Site* find_call_site(uint32_t ci, bool after) {
    const kz_fact_call& c = calls_[ci];
    for (auto& [k, st] : sites_)
      if (st.place == (after ? "after-call" : "at-call") && st.func == plain(c.caller_fn) && st.callee == str(c.callee) && st.line == c.line && (!v4_ || st.col == c.col))
        return &st;
    return nullptr;
  }

  // ---------------------------------------------------------------- plan rules revision 3: the path A control selection
  // (docs/plan_manifest.md section 7): (a) the rows with a probe-create handling line, (b) the rows with a creator site
  // (a creator line, or a not-a-pair line of key constants), in the order of the relations text, under the control budget
  void select_controls() {
    std::vector<size_t> a, b;
    for (size_t i = 0; i < rows_.size(); ++i) {
      const RelRow& r = *rows_[i];
      if (r.path != 'A') continue;
      bool probe = false;
      size_t sites = 0;
      std::istringstream in(render_row(r));
      std::string l;
      while (std::getline(in, l)) {
        const size_t f = l.find_first_not_of(' ');
        if (f == std::string::npos) continue;
        l = l.substr(f);
        if (starts(l, "handling: probe-create")) probe = true;
        if (starts(l, "creator:") || starts(l, "not a pair: key constants differ")) ++sites;
      }
      if (probe) a.push_back(i);
      else if (sites > 0) b.push_back(i);
      if (probe || sites > 0) control_kind_[i] = probe && sites > 0 ? "a+b" : probe ? "a" : "b";
    }
    control_order_ = a;
    control_order_.insert(control_order_.end(), b.begin(), b.end());
    for (size_t k = 0; k < control_order_.size(); ++k)
      if (k < control_rows_) control_planned_.push_back(control_order_[k]);
  }

  // ---------------------------------------------------------------- plan rules revision 3: deletes and retires
  // The namespace data entries whose lookup or create functions are the callee of a planned path A row's use or
  // creator calls; for each, the delete functions that apply to the build (the entry of the function with its
  // parameters, the after-call of the calls on the line of its retire call) and the retire stores (clear), so that a
  // delete or retire of the name between the use and the create can be observed (judgement rules 8.2 (마)).
  bool op_applies(const NsFile& f, const NsOp& op, uint32_t* fn_out) const {
    auto sit = f.sha.find(op.def_file);
    if (sit == f.sha.end()) return false;
    for (size_t i = 0; i < files_.size(); ++i) {
      const std::string p = str(files_[i].path);
      const bool tail = p == op.def_file || (p.size() > op.def_file.size() && p.compare(p.size() - op.def_file.size(), op.def_file.size(), op.def_file) == 0 &&
                                              p[p.size() - op.def_file.size() - 1] == '/');
      if (!tail || kz::to_hex(files_[i].sha256, 32) != sit->second) continue;
      for (uint32_t fn : fn_by_name_.count(op.fn) != 0 ? fn_by_name_.at(op.fn) : std::vector<uint32_t>{})
        if (defined(fn) && funcs_[fn].line == op.def_line && str(funcs_[fn].file) == p) {
          *fn_out = fn;
          return true;
        }
    }
    return false;
  }
  void plan_object_lifetimes() {
    for (size_t i : rows_planned_) {
      const RelRow& r = *rows_[i];
      if (r.path != 'A') continue;
      const std::string rowref = "row " + r.key;
      std::set<std::string> names;
      if (r.call != kN) names.insert(str(calls_[r.call].callee));
      for (uint32_t ci : row_creators_[i]) names.insert(str(calls_[ci].callee));
      std::set<std::string> keys;
      bool entry_found = false;
      // plan rules revision 4: the key-clearing calls of the row (relations rules revision 12) are delete sites
      auto rc = row_clearing_.find(i);
      if (rc != row_clearing_.end() && !rc->second.empty()) {
        keys = rc->second;
        entry_found = true;
      }
      for (const NsFile& f : ns_)
        for (const NsEntry& e : f.entries) {
          bool match = false;
          for (const NsOp& op : e.ops)
            if ((op.what == "lookup" || op.what == "create" || op.what == "copy" || op.what == "create-check") && names.count(op.fn) != 0) {
              uint32_t fn = kN;
              if (op_applies(f, op, &fn)) match = true;
            }
          if (!match) continue;
          entry_found = true;
          for (const NsOp& op : e.ops) {
            if (op.what != "delete" && op.what != "clear") continue;
            uint32_t fn = kN;
            const std::string dref = "namespace data " + f.path + " line " + std::to_string(op.line) + ": " + e.name + " " + op.what + " " + op.fn;
            std::string state = "applies";
            if (!op_applies(f, op, &fn)) {
              state = "does not apply (no file of that sha256 or no definition on that line)";
            } else if (op.what == "delete") {
              if (Site* st = entry_site(fn, "object-delete", dref, true)) keys.insert(st->key());
              for (uint32_t ci : calls_of_.count(fn) != 0 ? calls_of_.at(fn) : std::vector<uint32_t>{})
                if (calls_[ci].line == op.at_line)
                  if (Site* st = after_call(ci, "object-delete", dref + " retire call " + call_pos(ci))) keys.insert(st->key());
            } else {
              if (Site* st = store_site(fn, op.at_line, "object-retire", dref + " at " + op.at_file + ":" + std::to_string(op.at_line))) keys.insert(st->key());
            }
            lifetimes_.insert(e.name + " " + op.what + " " + op.fn + " " + op.def_file + ":" + std::to_string(op.def_line) + " data " + f.path + " line " +
                              std::to_string(op.line) + ": " + state);
          }
        }
      if (!entry_found) need(rowref, "object-delete", "not-planned", reason(kNoNamespace), "the row's lookup and creators: " + [&] {
        std::string t;
        for (const std::string& n : names) t += (t.empty() ? "" : " ") + n;
        return t;
      }());
      else need_sites(rowref, "object-delete", keys, kNoNamespace, "an entry matches but none of its deletes and retires applies to the build");
    }
  }

  // a writer line of path B: "<kind> at <pos> in <function>[ via <callee>] ..."
  void writer(const std::string& kind, const std::string& p, const std::string& fn_text, const RelRow& r, const std::string& rowref) {
    std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(p);
    if (ps.empty()) return;
    const uint32_t fn = fn_named(fn_text);
    if (kind == "assign" || kind == "inc-dec" || kind == "aggregate-assign") {
      if (fn == kN) {
        gap(rowref, "writer " + p, reason(kNoPos), "writer");
        return;
      }
      store_site(fn, std::get<1>(ps[0]), "writer", rowref + " " + p);
      return;
    }
    const uint32_t ci = call_of(ps[0]);
    if (ci == kN) {
      gap(rowref, "writer " + kind + " " + p, reason(kNoPos), "writer");
      return;
    }
    std::set<uint32_t> outs;
    if (kind == "out-pointer") {
      const size_t dot = r.subject.rfind('.');
      for (uint32_t k = 0; k < calls_[ci].nargs; ++k) {
        const kz_fact_arg* a = arg(ci, k);
        if (a != nullptr && a->kind == KZ_ARG_KIND_ADDR_FIELD && dot != std::string::npos && str(a->loc_type) == r.subject.substr(0, dot) &&
            str(a->loc_field) == r.subject.substr(dot + 1))
          outs.insert(k);
      }
      if (outs.empty()) outs = out_params(ci);
    }
    at_call(ci, "writer", rowref + " " + p);
    after_call(ci, "writer", rowref + " " + p, outs);
  }

  // a handling line of path A
  void handling(const std::string& l, const RelRow& r, const std::string& rowref) {
    (void)r;
    if (l.find(" returns the result of ") != std::string::npos) {
      std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(l);
      for (const auto& p : ps) {
        const uint32_t ci = call_of(p);
        if (ci != kN) after_call(ci, "handling-chain", rowref);
      }
      return;
    }
    if (!starts(l, "terminal ")) return;
    // the part of the terminal line the rules derived for this row (the F8 explanation after " (F8 " is about the callee)
    std::string t = l;
    const size_t f8 = t.find(" (F8 ");
    if (f8 != std::string::npos) t = t.substr(0, f8);
    std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(t);
    if (ps.empty()) return;
    const uint32_t tci = call_of(ps[0]);
    if (tci == kN) {
      gap(rowref, "terminal " + pos_text(ps[0]), reason(kNoPos), "handling-chain");
      return;
    }
    at_call(tci, "handling-chain", rowref);
    const uint32_t tfn = calls_[tci].caller_fn;
    for (size_t k = 1; k < ps.size(); ++k) {
      const uint32_t ci = call_of(ps[k]);
      if (ci != kN) {
        after_call(ci, "handling-chain", rowref);
        continue;
      }
      const uint32_t fn = fn_at(std::get<0>(ps[k]), std::get<1>(ps[k]));
      if (fn == kN) {
        gap(rowref, "handling position " + pos_text(ps[k]), reason(kNoPos), "handling-chain");
        continue;
      }
      store_site(fn, std::get<1>(ps[k]), "handling-chain", rowref + " " + pos_text(ps[k]));
    }
    // the calls whose results the guards of the line test, in the terminal's function
    static const std::regex re(R"(\(call-result (\S+))");
    std::set<std::string> names;
    for (std::sregex_iterator it(t.begin(), t.end(), re), end; it != end; ++it) names.insert((*it)[1].str());
    if (calls_of_.count(tfn) != 0)
      for (uint32_t ci : calls_of_.at(tfn))
        if (names.count(str(calls_[ci].callee)) != 0) after_call(ci, "handling-chain", rowref);
    // the result a guard's local holds: "the result of X is used in F" (the call of X in F)
    static const std::regex re_used(R"(the result of (\S+) is used in (\S+?):)");
    for (std::sregex_iterator it(t.begin(), t.end(), re_used), end; it != end; ++it) {
      const uint32_t f = fn_named((*it)[2].str());
      const std::string callee = base_name((*it)[1].str());
      if (f == kN || calls_of_.count(f) == 0) continue;
      for (uint32_t ci : calls_of_.at(f))
        if (str(calls_[ci].callee) == callee) after_call(ci, "handling-chain", rowref);
    }
  }

  // ---------------------------------------------------------------- actor rule
  bool is_create_prim(uint32_t ci) const {
    const kz_fact_call& c = calls_[ci];
    if (defined(c.callee_fn)) return false;
    auto it = p0_kind_.find(str(c.callee));
    return it != p0_kind_.end() && it->second == KZ_P0_KIND_THREAD_CREATE;
  }
  const kz_fact_p0* p0_of(uint32_t ci) const {
    for (const kz_fact_p0& p : p0_)
      if (str(p.name) == str(calls_[ci].callee)) return &p;
    return nullptr;
  }

  void plan_actors() {
    for (uint32_t e : script_entries_)
      if (root_kind_.count(e) != 0) actor_roots_.insert(e);
    // the creation primitive calls of each root, through its SPAWN_WRAP chain (or its own spawn call)
    std::map<uint32_t, std::set<uint32_t>> roots_of_prim;
    std::map<uint32_t, std::set<uint32_t>> starts_of_prim;
    for (uint32_t root : actor_roots_) {
      const std::string rref = "actor " + fname(root) + " (" + root_kind_.at(root) + ")";
      entry_site(root, "actor-root", rref, false);
      std::set<uint32_t> prims, chain_sites, fixed_entries;
      std::vector<uint32_t> work;
      std::set<uint32_t> seen;
      for (size_t i = 0; i < spawnw_.size(); ++i)
        if (spawnw_[i].entry_fn == root) work.push_back(static_cast<uint32_t>(i));
      for (const auto& [site, entries] : spawn_entries_)
        if (entries.count(root) != 0) {
          auto cit = call_by_site_.find(site);
          if (cit == call_by_site_.end()) continue;
          chain_sites.insert(site);
          if (is_create_prim(cit->second)) {
            prims.insert(cit->second);
            continue;
          }
          // a spawn call of a derived spawn operation: its chain continues in the operation
          const uint32_t next = calls_[cit->second].callee_fn;
          for (size_t i = 0; i < spawnw_.size(); ++i)
            if (spawnw_[i].fn == next) work.push_back(static_cast<uint32_t>(i));
        }
      while (!work.empty()) {
        const uint32_t w = work.back();
        work.pop_back();
        if (!seen.insert(w).second) continue;
        const kz_fact_spawnw& row = spawnw_[w];
        chain_sites.insert(row.site);
        if (row.kind == KZ_SPAWNW_KIND_FIXED_ENTRY) fixed_entries.insert(w);
        auto cit = call_by_site_.find(row.site);
        if (cit == call_by_site_.end()) continue;
        if (is_create_prim(cit->second)) {
          prims.insert(cit->second);
          continue;
        }
        const uint32_t next = calls_[cit->second].callee_fn;
        for (size_t i = 0; i < spawnw_.size(); ++i)
          if (spawnw_[i].fn == next && seen.count(static_cast<uint32_t>(i)) == 0) work.push_back(static_cast<uint32_t>(i));
      }
      if (prims.empty()) {
        gap(rref, "creation", reason(kNoCreation), "actor");
        continue;
      }
      for (uint32_t pc : prims) {
        roots_of_prim[pc].insert(root);
        // the start routines at that call: a function argument, or the fixed entries the chain passes to its wrapper
        const kz_fact_p0* p0 = p0_of(pc);
        const kz_fact_arg* ea = p0 != nullptr && p0->entry_param != kN ? arg(pc, p0->entry_param) : nullptr;
        // (for this root; where neither is found, the entry the chain resolves for the root is what the call starts)
        std::set<uint32_t> mine;
        if (ea != nullptr && ea->kind == KZ_ARG_KIND_FUNC && defined(ea->fn)) mine.insert(ea->fn);
        for (uint32_t w : fixed_entries) {
          auto cit = call_by_site_.find(spawnw_[w].site);
          if (cit != call_by_site_.end() && calls_[cit->second].callee_fn == calls_[pc].caller_fn && defined(spawnw_[w].entry_fn)) mine.insert(spawnw_[w].entry_fn);
        }
        if (mine.empty()) mine.insert(root);
        starts_of_prim[pc].insert(mine.begin(), mine.end());
      }
      // the owner request: a store of a parameter after a spawn of the chain, in the spawn's function
      bool owner = false;
      for (uint32_t site : chain_sites) {
        auto ait = after_spawn_.find(site);
        auto cit = call_by_site_.find(site);
        if (ait == after_spawn_.end() || cit == call_by_site_.end()) continue;
        const uint32_t fn = calls_[cit->second].caller_fn;
        for (uint32_t row : ait->second) {
          if (row >= lockset_.size()) continue;
          const kz_fact_lockset& w = lockset_[row];
          if (w.kind != KZ_LOCKSET_KIND_WRITE || w.fn != fn) continue;
          for (uint32_t k = 0; k < (defined(fn) ? funcs_[fn].nparams : 0); ++k) {
            auto pit = pflow_of_.find({fn, k});
            if (pit == pflow_of_.end()) continue;
            for (uint32_t pr : pit->second) {
              const kz_fact_pflow& p = pflow_[pr];
              if (p.depth != 0 || p.kind != KZ_PFLOW_KIND_STORE || p.line != w.line || p.col != w.col || str(p.file) != str(w.file)) continue;
              store_site(fn, p.line, "actor-owner", rref + " parameter " + std::to_string(k) + " of " + plain(fn) + " stored at " + pos(str(p.file), p.line, p.col) +
                                                        " after the spawn " + call_pos(cit->second));
              owner = true;
            }
          }
        }
      }
      if (!owner) need(rref, "owner-request", "not-planned", reason(kNotPlanned), "the facts show no store of a parameter after a spawn of the chain in the spawn's function");
    }
    for (const auto& [pc, roots] : roots_of_prim) {
      const kz_fact_p0* p0 = p0_of(pc);
      const std::string pref = "creation " + call_pos(pc);
      std::vector<Val> asked;
      if (p0 != nullptr && p0->entry_param != kN) {
        Val v;
        v.role = "start_routine";
        v.from = "arg" + std::to_string(p0->entry_param);
        asked.push_back(v);
      }
      if (p0 != nullptr && p0->arg_param != kN) {
        Val v;
        v.role = "creation_arg";
        v.from = "arg" + std::to_string(p0->arg_param);
        asked.push_back(v);
      }
      Site* cs = at_call(pc, "actor-create", pref, asked);
      // revision 3: the creation call is made through the runtime's creation wrapper (the spawn binding), whose token
      // the spawn-token reader reads
      if (v4_ && cs != nullptr) {
        if (p0 != nullptr && p0->entry_param != kN && p0->arg_param != kN && p0->entry_param != p0->arg_param)
          spawns_[cs->key()] = SpawnBind{p0->entry_param, p0->arg_param, p0->out_param,
                                        "P0 thread-creation entry " + str(p0->name) + ": start routine argument " + std::to_string(p0->entry_param) +
                                            ", creation argument " + std::to_string(p0->arg_param) +
                                            (p0->out_param != kN ? ", thread handle out argument " + std::to_string(p0->out_param) : std::string()) +
                                            " (P0_TABLE)"};
        else
          gap(pref, "spawn binding", reason(kNoCreation), "actor-create");
      }
      std::set<uint32_t> outs;
      if (p0 != nullptr && p0->out_param != kN) outs.insert(p0->out_param);
      after_call(pc, "actor-create", pref, outs);
      // a creation argument that is one constant: every thread created through the call gets the same value
      const kz_fact_arg* ca = p0 != nullptr && p0->arg_param != kN ? arg(pc, p0->arg_param) : nullptr;
      const bool constant = ca != nullptr && ca->kind == KZ_ARG_KIND_CONST;
      bool shared_const = constant && roots.size() > 1;
      if (constant)
        for (const auto& [other, oroots] : roots_of_prim) {
          if (other == pc) continue;
          const kz_fact_p0* q = p0_of(other);
          const kz_fact_arg* oa = q != nullptr && q->arg_param != kN ? arg(other, q->arg_param) : nullptr;
          if (oa != nullptr && oa->kind == KZ_ARG_KIND_CONST && oa->value == ca->value) shared_const = true;
        }
      for (uint32_t st : starts_of_prim[pc]) entry_site(st, "actor-start", pref + " start routine " + fname(st), !shared_const);
      std::string rs;
      for (uint32_t rt : roots) rs += (rs.empty() ? "" : " ") + fname(rt);
      if (shared_const)
        gap(pref, "creation argument " + std::to_string(ca->value) + " for the roots " + rs, reason(kNoChannel), "actor");
      creations_.push_back(call_pos(pc) + " roots " + rs + "; start routines " + [&] {
        std::string t;
        for (uint32_t st : starts_of_prim[pc]) t += (t.empty() ? "" : " ") + fname(st);
        return t;
      }() + (shared_const ? "; tie: none (one constant argument)" : "; tie: creation argument = start routine parameter 0 (premise: the argument reaches the start routine unchanged; a repeated value makes the tie undetermined)"));
    }
  }

  // ---------------------------------------------------------------- stop-event rule
  void plan_stop_events() {
    std::set<std::string> roles_done;
    for (const FwEntry& e : fw_) {
      // the entry applies when the facts hold its defining file with that sha256 and define the function on that line
      uint32_t fh = kN;
      for (size_t i = 0; i < files_.size(); ++i) {
        const std::string p = str(files_[i].path);
        const bool tail = p == e.def_file || (p.size() > e.def_file.size() && p.compare(p.size() - e.def_file.size(), e.def_file.size(), e.def_file) == 0 &&
                                               p[p.size() - e.def_file.size() - 1] == '/');
        if (tail && kz::to_hex(files_[i].sha256, 32) == e.def_sha) fh = static_cast<uint32_t>(i);
      }
      if (fh == kN) continue;
      uint32_t fn = kN;
      for (uint32_t f : fn_by_name_.count(e.function) != 0 ? fn_by_name_.at(e.function) : std::vector<uint32_t>{})
        if (defined(f) && funcs_[f].line == e.def_line && str(funcs_[f].file) == file_path(fh)) fn = f;
      if (fn == kN) continue;
      roles_done.insert(e.role);
      const std::string ref = "framework-layer data line " + std::to_string(e.line) + ": " + e.role + " " + e.function + " " + file_path(fh) + ":" + std::to_string(e.def_line) +
                              " (" + e.version + ")";
      entry_site(fn, "stop-event", ref, true);
      stops_.push_back(e.role + " " + e.function + " " + file_path(fh) + ":" + std::to_string(e.def_line) + " decl " + e.decl + " version " + quoted(e.version) + " data line " +
                       std::to_string(e.line));
    }
    for (const char* role : {"run-loop", "app-exit"})
      if (roles_done.count(role) == 0) need("plan", std::string("stop-event ") + role, "not-planned", reason(kNotPlanned), "no entry of the framework-layer data matches a file and a definition of the facts");
  }

  // ---------------------------------------------------------------- output
  void finish_sites() {
    uint32_t id = 0;
    for (auto& [k, s] : sites_) s.id = ++id;
  }
  std::string label(const Site& s) const {
    std::string r;
    for (const std::string& x : s.rules) r += (r.empty() ? "" : "+") + x;
    char buf[16];
    std::snprintf(buf, sizeof buf, "s%04u", s.id);
    return std::string(buf) + "." + r;
  }

  void write(PlanOutput* out) {
    // ---- the module table and the executables, from the basis
    std::ostringstream mt, ex;
    std::vector<std::string> names;
    std::map<std::string, std::vector<std::string>> units_of;
    std::string core;
    if (basis_c_ != nullptr) {
      const std::string bin = bmap_.count("binary.path") != 0 ? bmap_.at("binary.path") : std::string();
      core = bin.substr(bin.rfind('/') + 1);
      if (core.empty()) core = "program";
      names.push_back(core);
      for (size_t i = 0; i < tu_member_.size(); ++i) {
        const std::string& m = tu_member_[i];
        if (!starts(m, "module ")) {
          units_of[core].push_back(tu_src_[i]);
          continue;
        }
        const std::string n = m.substr(7, m.find(" (") == std::string::npos ? std::string::npos : m.find(" (") - 7);
        if (units_of.count(n) == 0) names.push_back(n);
        units_of[n].push_back(tu_src_[i]);
      }
    } else {
      core = "program";
      names.push_back(core);
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    const std::string broot = bmap_.count("bundle.root") != 0 ? bmap_.at("bundle.root") : std::string();
    auto absolute = [&](const std::string& p) { return !p.empty() && p[0] == '/' ? p : (broot.empty() ? p : broot + "/" + p); };
    std::map<std::string, uint32_t> id_of;
    for (size_t i = 0; i < names.size(); ++i) id_of[names[i]] = static_cast<uint32_t>(i + 1);
    std::vector<std::string> mod_lines;
    for (const std::string& n : names) {
      if (n == core) continue;
      // the common directory of the module's units; where a unit of another module or of the executable is under it, a
      // prefix per unit directory, else per unit
      const std::vector<std::string>& u = units_of[n];
      std::string c = u[0].substr(0, u[0].rfind('/') + 1);
      for (const std::string& p : u)
        while (!c.empty() && p.compare(0, c.size(), c) != 0) {
          c.pop_back();
          c = c.substr(0, c.rfind('/') + 1);
        }
      auto clash = [&](const std::string& pre) {
        for (const auto& [on, ou] : units_of)
          if (on != n)
            for (const std::string& p : ou)
              if (p.compare(0, pre.size(), pre) == 0) return true;
        return false;
      };
      std::set<std::string> pres;
      if (!c.empty() && !clash(c)) pres.insert(c);
      else
        for (const std::string& p : u) {
          const std::string d = p.substr(0, p.rfind('/') + 1);
          pres.insert(clash(d) ? p : d);
        }
      for (const std::string& pre : pres) mod_lines.push_back(std::to_string(id_of[n]) + "\t" + n + "\t" + absolute(pre));
    }
    std::sort(mod_lines.begin(), mod_lines.end(), [](const std::string& a, const std::string& b) {
      return std::strtoul(a.c_str(), nullptr, 10) != std::strtoul(b.c_str(), nullptr, 10) ? std::strtoul(a.c_str(), nullptr, 10) < std::strtoul(b.c_str(), nullptr, 10) : a < b;
    });
    std::vector<std::string> exes;
    if (basis_c_ != nullptr) {
      if (bmap_.count("binary.path") != 0) exes.push_back("basis\t" + absolute(bmap_.at("binary.path")));
      for (const std::string& n : names) {
        auto it = bmap_.find("module[" + n + "]");
        if (it == bmap_.end()) continue;
        const size_t a = it->second.find(" -> ");
        if (a == std::string::npos) continue;
        std::string p = it->second.substr(a + 4);
        p = p.substr(0, p.find(' '));
        exes.push_back("basis\t" + absolute(p));
      }
    }

    // ---- the sidecar
    std::ostringstream sc;
    // ---- revision 3: the value requests first (the sidecar names their sha256)
    std::string req_sha;
    size_t req_planned = 0;
    if (v4_) {
      std::ostringstream rq;
      rq << "kz-plan-requests 1\n";
      rq << "# value requests of the planned rows: what value of a record would decide a row the method cannot label from the facts, and which site of the plan records it\n";
      rq << "rule " << plan_requests_revision() << ": rows of path A and B with the handling label T?, J? or F, or an open effect in their absence line; the result and out values at the use's after-call, the effect calls of RESULT_EFFECT with the argument a SEMANTICS arg-role row names, the branch taken (no site kind)\n";
      rq << "facts-artifact " << kz::to_hex(c_.artifact_hash()) << "\n";
      size_t n = 0;
      for (const Request& q : requests_) {
        const auto it = sites_.find(q.site);
        if (q.state == "planned") ++req_planned;
        rq << "request " << ++n << " row=" << q.row << " id=" << q.id << " label=" << q.label << " value=" << q.value << " state=" << q.state
           << " site=" << (it == sites_.end() ? std::string("-") : std::to_string(it->second.id)) << " slot=" << q.slot << " reason=" << q.why << " what=" << quoted(q.what)
           << " note=" << quoted(q.note) << "\n";
      }
      rq << "end rows=" << request_rows_ << " requests=" << requests_.size() << " planned=" << req_planned << "\n";
      out->requests = rq.str();
      req_sha = kz::to_hex(kz::sha256(out->requests));
    }
    sc << (v4_ ? "kz-plan-sidecar 2\n" : "kz-plan-sidecar 1\n");
    sc << "generator " << o_.generator << "\n";
    for (const PlanInputFile& f : o_.inputs) sc << "input " << f.role << " sha256=" << f.sha256 << " path=" << f.path << "\n";
    sc << "facts-artifact " << kz::to_hex(c_.artifact_hash()) << "\n";
    if (basis_c_ != nullptr) sc << "basis-artifact " << kz::to_hex(basis_c_->artifact_hash()) << "\n";
    sc << "rules " << (v4_ ? plan_rules_v4_revision() : plan_rules_revision()) << "; " << relations_rules_revision() << "\n";
    if (v4_) {
      sc << "manifest-revision 4 (kzs-sites 4)\n";
      sc << "plan-id " << plan_id() << "\n";
    }
    sc << "selection " << o_.select << ": every row of the relations (method document 3.1, the revised rule: no label and no proof of a row removes it)\n";
    sc << "priority " << o_.order << ": " << plan_priority_revision() << "; tier 1 = disposition D07 (label R or J, counterpart, order class RELOC), tier 2 = the other rows of order class RELOC, tier 3 = the rest; within a tier the order of the relations text\n";
    size_t n1 = 0, n2 = 0, n3 = 0;
    for (const PlanPriority& p : prio_) (p.tier == 1 ? n1 : p.tier == 2 ? n2 : n3)++;
    sc << "classes rows " << prio_.size() << " tier1 " << n1 << " tier2 " << n2 << " tier3 " << n3 << "\n";
    sc << "budget value=" << o_.budget << " reason=" << quoted(o_.budget_reason) << " disclosure=" << quoted(o_.budget_disclosure.empty() ? "none given" : o_.budget_disclosure)
       << " planned=" << planned_.size() << " truncation=" << (prio_.size() - planned_.size()) << " rows cut (row lines with state cut)"
       << " unresolved=" << quoted(planned_.size() < prio_.size() ? "whether the rows cut fit the record pool (no event volume measured)" : "none")
       << " sensitivity=" << quoted("not measured") << "\n";
    if (v4_) {
      sc << "control " << plan_control_revision() << ": (a) path A rows with a probe-create handling line, (b) path A rows with a creator site (creator lines and not-a-pair lines of key constants), (a) then (b) in the order of the relations text; planned apart from the tier order (docs/plan_manifest.md section 7)\n";
      size_t na = 0, nb = 0;
      for (const auto& [i, k] : control_kind_) (k == "b" ? nb : na)++;
      sc << "control-budget value=" << o_.control_budget << " reason=" << quoted(o_.control_budget_reason) << " rows=" << control_order_.size() << " (a " << na << ", b " << nb
         << ") planned=" << control_planned_.size() << " cut=" << (control_order_.size() - control_planned_.size())
         << " complete=" << (control_planned_.size() == control_order_.size() ? "yes" : "no") << " unresolved="
         << quoted("whether the control rows fit the record pool (no event volume measured)") << " sensitivity=" << quoted("not measured") << "\n";
      // the file by its name only: the sidecar names no output path (the regeneration check compares it byte for byte)
      sc << "requests sha256=" << req_sha << " file=" << o_.requests_path.substr(o_.requests_path.rfind('/') + 1) << " rows=" << request_rows_ << " requests=" << requests_.size() << " planned=" << req_planned << "\n";
      for (const auto& [path, text] : o_.namespace_data) sc << "namespace-data " << path << " entries=" << [&] {
        for (const NsFile& f : ns_)
          if (f.path == path) return f.entries.size();
        return static_cast<size_t>(0);
      }() << "\n";
    }
    sc << "bound string KZ_RT_STR_MAX=" << kStrMax << " source=runtime/include/kz_rt.h:82 reason=" << quoted("the runtime's copy bound; a key text is bounded by the program's own constant length check where the facts show one")
       << " truncation=" << quoted("the runtime flags a cut string per value") << "\n";
    if (!argtype_.empty())
      sc << "bound string-extent source=ARG_TYPES reason="
         << quoted("a key argument that is a character array read at a point that dominates the call is copied at the call within min(its extent, KZ_RT_STR_MAX)")
         << "\n";
    sc << "bound slots=" << kSlots << " source=instrument/gcc/kzgcc.cpp build_obs reason=" << quoted("values per event bound by kzgcc 0.3") << "\n";
    sc << "bound sites<=4095 source=instrument/gcc/kzgcc.cpp plugin_init reason=" << quoted("the binding id scheme") << " sites=" << sites_.size() << "\n";
    sc << "param tu-root " << (out_root_.empty() ? "-" : out_root_) << " (units outside the bundle root are named relative to it)\n";
    sc << "param bundle-root " << (broot.empty() ? "-" : broot) << "\n";
    for (size_t i = 0; i < prio_.size(); ++i) {
      const PlanPriority& p = prio_[i];
      const bool in = std::find(planned_.begin(), planned_.end(), i) != planned_.end();
      sc << "row " << p.key << " " << p.id << " tier=" << p.tier << " position=" << position_.at(i) << " label=" << p.label;
      // plan rules revision 4: the fact-derived label beside the text label the tier order uses
      if (v4_) sc << " fact-label=" << p.fact_label;
      sc << " class=" << p.label_class
         << " order=" << p.order_class << " reach=" << p.reach << " disposition=" << p.disposition << " state=" << (in ? "planned" : "cut");
      const auto ck = control_kind_.find(i);
      const bool cin = std::find(control_planned_.begin(), control_planned_.end(), i) != control_planned_.end();
      if (v4_ && ck != control_kind_.end()) sc << " control=" << ck->second << " control-state=" << (cin ? "planned" : "cut");
      sc << " why=" << quoted(p.why) << "\n";
      if (!in && !(v4_ && cin)) sc << "need row " << p.key << " observation not-planned reason=" << reason(kBudget) << "\n";
      if (v4_ && ck != control_kind_.end() && !cin) sc << "need row " << p.key << " control not-planned reason=" << reason(kControlCut) << "\n";
    }
    for (const auto& [k, s] : sites_) {
      sc << "site " << s.id << " " << label(s) << " key=" << quoted(k) << " refs=";
      std::string r;
      for (const std::string& x : s.refs) r += (r.empty() ? "" : "; ") + x;
      sc << quoted(r) << "\n";
    }
    // the use-site key to site map
    std::map<std::string, std::set<uint32_t>> by_row;
    for (const auto& [k, s] : sites_)
      for (const std::string& ref : s.refs)
        if (starts(ref, "row ")) by_row[ref.substr(4, 16)].insert(s.id);
    for (const auto& [key, ids] : by_row) {
      sc << "map " << key;
      for (uint32_t id : ids) sc << " " << id;
      sc << "\n";
    }
    for (const Need& n : needs_) {
      sc << "need " << n.subject << " " << n.what << " " << n.state << " reason=" << n.why << " note=" << quoted(n.note);
      if (v4_ && !n.sites.empty()) {
        std::set<uint32_t> sid;
        for (const std::string& k2 : n.sites) {
          const auto it = sites_.find(k2);
          if (it != sites_.end()) sid.insert(it->second.id);
        }
        sc << " sites=";
        std::string ids;
        for (uint32_t id2 : sid) ids += (ids.empty() ? "" : ",") + std::to_string(id2);
        sc << ids;
      }
      sc << "\n";
    }
    for (const Need& n : gaps_) sc << "gap " << n.subject << " " << quoted(n.what) << " reason=" << n.why << " rule=" << n.note << "\n";
    for (const std::string& c : creations_) sc << "creation " << c << "\n";
    for (const std::string& s : stops_) sc << "stop-event " << s << "\n";
    for (const std::string& l : lifetimes_) sc << "lifetime " << l << "\n";
    for (const auto& [k, b] : spawns_) {
      const auto it = sites_.find(k);
      if (it != sites_.end()) sc << "spawn " << it->second.id << " wrapper=" << kSpawnWrapper << " " << b.ref << "\n";
    }
    for (const std::string& m : mod_lines) sc << "module " << m << "\n";
    sc << "default-module " << id_of[core] << " " << core << "\n";
    for (const std::string& e : exes) sc << "exe " << e << "\n";
    out->sidecar = sc.str();
    const std::string side_sha = kz::to_hex(kz::sha256(out->sidecar));

    // ---- the manifest
    std::ostringstream m;
    m << "# kz plan written by kz-static plan (" << (v4_ ? plan_rules_v4_revision() : plan_rules_revision()) << "; " << relations_rules_revision() << ")\n";
    m << "# sidecar sha256=" << side_sha << "\n";
    m << "# every site: a label from the sorted site keys with its rule names, a ref to the facts it came from; the plan states what to record, never what to expect\n";
    m << (v4_ ? "kzs-sites 4\n" : "kzs-sites 3\n");
    if (v4_)
      m << "plan " << plan_id() << " facts=" << kz::to_hex(c_.artifact_hash()) << " relrules=" << quoted(relations_rules_revision()) << " obsrules=" << quoted(plan_rules_v4_revision())
        << " semantics=" << quoted(semantics_text()) << " select=" << o_.select << " budget=" << quoted(o_.budget) << " control=" << quoted(o_.control_budget)
        << " ref=" << quoted("sidecar sha256 " + side_sha) << "\n";
    const std::string rev = basis_c_ != nullptr ? "basis-" + kz::to_hex(basis_c_->artifact_hash()) : "facts-" + kz::to_hex(c_.artifact_hash());
    m << "source build rev=" << rev << " ref="
      << quoted(basis_c_ != nullptr ? "the analysis basis: every source file read is hashed in its FILE_HASHES; no source tree revision checked (basis source.worktree_state)"
                                    : "facts extracted without a basis: every source file read is hashed in their FILE_HASHES")
      << "\n";
    for (const std::string& n : names) m << "module " << id_of[n] << " name=" << n << " source=build\n";
    for (const auto& [k, s] : sites_) {
      m << "site " << s.id << " label=" << label(s) << " op=" << s.op << " phase=" << s.phase << " tu=" << s.tu << " func=" << s.func;
      if (!s.callee.empty()) m << " callee=" << s.callee;
      if (s.line != 0) m << " line=" << s.line;
      if (!s.place.empty()) m << " place=" << s.place;
      if (v4_) {
        m << " skey=" << skey(k);
        std::set<std::string> rows;
        for (const std::string& x : s.refs)
          if (starts(x, "row ") && x.size() >= 20) rows.insert(x.substr(4, 16));
        if (!rows.empty()) {
          std::string rl;
          for (const std::string& x : rows) rl += (rl.empty() ? "" : ",") + x;
          m << " row=" << rl;
        }
        if (s.col != 0) m << " col=" << s.col;
        if (s.nth != 0) m << " nth=" << s.nth;
        if (!s.field.empty()) m << " field=" << s.field;
      }
      std::string r;
      for (const std::string& x : s.refs) r += (r.empty() ? "" : "; ") + x;
      if (r.size() > 400) r = r.substr(0, 400) + " ...(sidecar)";
      m << " ref=" << quoted(r) << "\n";
      for (size_t i = 0; i < s.vals.size(); ++i) {
        const Val& v = s.vals[i];
        m << "value " << s.id << " " << i << " role=" << v.role << " from=" << v.from << " type=" << v.type;
        if (!v.string_mode.empty()) m << " string=" << v.string_mode;
        if (v.string_mode == "validated") m << " bound=" << v.bound;
        if (!v.ref.empty()) m << " ref=" << quoted(v.ref);
        m << "\n";
      }
      const auto sp = spawns_.find(k);
      if (v4_ && sp != spawns_.end()) {
        m << "spawn " << s.id << " wrapper=" << kSpawnWrapper << " start=arg" << sp->second.start << " arg=arg" << sp->second.arg;
        if (sp->second.out != kN) m << " out=arg" << sp->second.out;
        m << " ref=" << quoted(sp->second.ref) << "\n";
        m << "identity " << s.id << " decoder=spawn-token\n";
      }
    }
    if (v4_) {
      uint32_t n = 0;
      for (const Need& x : needs_) {
        m << "need " << ++n << " subject=" << quoted(x.subject) << " what=" << quoted(x.what) << " state=" << x.state;
        std::string ids;
        std::set<uint32_t> sid;
        for (const std::string& k2 : x.sites) {
          const auto it = sites_.find(k2);
          if (it != sites_.end()) sid.insert(it->second.id);
        }
        for (uint32_t id2 : sid) ids += (ids.empty() ? "" : ",") + std::to_string(id2);
        if (!ids.empty()) m << " sites=" << ids;
        if (x.state != "planned") m << " reason=" << quoted(x.why);
        if (!x.note.empty()) m << " note=" << quoted(x.note);
        m << "\n";
      }
      n = 0;
      for (const Need& x : gaps_) m << "gap " << ++n << " subject=" << quoted(x.subject) << " what=" << quoted(x.what) << " reason=" << quoted(x.why) << " rule=" << quoted(x.note) << "\n";
    }
    out->manifest = m.str();

    mt << "# kz module table; plan sidecar " << o_.sidecar_path << " sha256=" << side_sha << "\n";
    mt << "# the reference build's source paths (basis); a derived build in another directory maps them to its copies\n";
    for (const std::string& l : mod_lines) mt << l << "\n";
    mt << "default\t" << id_of[core] << "\n";
    out->modules = mt.str();
    ex << "# kz executables to check; plan sidecar " << o_.sidecar_path << " sha256=" << side_sha << "\n";
    ex << "# the reference build's paths (basis); a derived build maps them to its install tree\n";
    for (const std::string& e : exes) ex << e << "\n";
    out->exes = ex.str();
  }

  // ---- revision 3 helpers
  static std::string skey(const std::string& key) { return kz::to_hex(kz::sha256(key)).substr(0, 16); }
  std::string semantics_text() const {
    auto it = basis_map_.find("extract.semantics");
    if (it == basis_map_.end()) return "none (the facts have no SEMANTICS section)";
    const std::string& v = it->second;
    return v.substr(0, v.find(':'));
  }
  std::string plan_id() const {
    const std::string t = "facts=" + kz::to_hex(c_.artifact_hash()) + "|obsrules=" + plan_rules_v4_revision() + "|relrules=" + relations_rules_revision() +
                          "|select=" + o_.select + "|order=" + o_.order + "|budget=" + o_.budget + "|control=" + o_.control_budget + "|controlrule=" +
                          plan_control_revision() + "|requests=" + plan_requests_revision() + "|semantics=" + semantics_text();
    return kz::to_hex(kz::sha256(t)).substr(0, 16);
  }
  // the runtime's creation wrapper the spawn binding names (the runtime step that adds it records SPAWN, SPAWN_RET and
  // TBEGIN with the creation token; docs/plan_manifest.md section 8)
  static constexpr const char* kSpawnWrapper = "kz_rt_posix_create";
  bool v4_ = false;
  uint32_t control_rows_ = 0;
  std::vector<NsFile> ns_;
  std::vector<kz_fact_objacc> objacc_;
  std::map<std::string, std::map<uint32_t, std::string>> arg_roles_;  // api -> argument position -> SEMANTICS arg role
  std::map<size_t, std::string> control_kind_;                        // row -> a | b | a+b
  std::vector<size_t> control_order_, control_planned_;
  std::set<size_t> rows_planned_;
  std::map<size_t, std::set<std::string>> row_clearing_;              // row -> the site keys of its key-clearing calls (plan rules revision 4)
  std::map<size_t, std::set<uint32_t>> row_creators_;                 // row -> its creator calls (creator and not-a-pair lines)
  std::set<std::string> last_after_;
  std::vector<Request> requests_;
  size_t request_rows_ = 0;
  std::set<std::string> lifetimes_;
  std::map<std::string, SpawnBind> spawns_;  // site key -> its spawn binding

  struct FwEntry {
    std::string role, function, def_file, def_sha, decl, version;
    uint32_t def_line = 0;
    int line = 0;
  };

  const kz::Container* basis_c_;
  const std::string& framework_;
  const PlanOptions& o_;
  std::string budget_kind_;
  uint32_t budget_tier_ = 0, budget_rows_ = 0;
  std::vector<kz_file_hash> files_;
  std::map<std::pair<uint32_t, uint32_t>, kz_fact_argtype> argtype_;  // (FACTS index, argument) -> ARG_TYPES row
  std::vector<kz_fact_arg> args_;
  std::vector<kz_basis_item> bitems_;
  std::vector<kz_tu> tus_;
  std::map<std::string, std::string> bmap_;
  std::vector<std::string> tu_src_, tu_member_;
  std::string out_root_;
  std::vector<FwEntry> fw_;
  std::map<std::tuple<std::string, uint32_t, uint32_t>, uint32_t> call_at_;
  RelReport rep_;
  std::vector<RelRow*> rows_;
  std::vector<PlanPriority> prio_;
  std::map<size_t, uint32_t> position_;
  std::vector<size_t> planned_;
  std::map<std::string, Site> sites_;
  std::vector<Need> needs_, gaps_;
  std::set<std::string> seen_;
  std::set<uint32_t> actor_roots_;
  std::vector<std::string> creations_, stops_;
};

}  // namespace

std::vector<PlanPriority> plan_priorities(const RelReport& report) {
  std::vector<PlanPriority> v;
  for (const RelGroup& g : report.groups)
    for (const RelRow& r : g.rows) v.push_back(classify(r));
  return v;
}

kz::Status plan_generate(const kz::Container& facts, const kz::Container* basis, const std::string& framework, const PlanOptions& options, PlanOutput* out) {
  if (facts.header().artifact_kind != KZ_ARTIFACT_FACTS) return kz::Status::error(kz::Err::Invalid, "the first input is not a facts container");
  kz::Digest zero{};
  if (basis == nullptr && facts.basis_hash() != zero)
    return kz::Status::error(kz::Err::Invalid, "the facts were read under the basis " + kz::to_hex(facts.basis_hash()) +
                                                   ": give it with --basis (the generator opens only the files named on its command line)");
  if (basis != nullptr && facts.basis_hash() != basis->artifact_hash())
    return kz::Status::error(kz::Err::Invalid, "the basis given is not the basis the facts were read under (facts header basis " + kz::to_hex(facts.basis_hash()) + ")");
  Gen g(facts, basis, framework, options);
  return g.run(out);
}

}  // namespace kzs
