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

const char* plan_rules_revision() { return "plan rules revision 1 (2026-11-16)"; }
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
  };
  return table;
}

namespace {

constexpr uint32_t kN = KZ_NONE32;
constexpr uint32_t kSlots = 4;      // values per event bound by the plugin (kzgcc.cpp build_obs: slots 0-3)
constexpr uint32_t kStrMax = 64;    // KZ_RT_STR_MAX (runtime/include/kz_rt.h:82)

std::string reason(size_t i) { return reason_text(plan_reasons()[i]); }
enum R { kBudget, kNotPlanned, kKeyText, kTwoCalls, kSlotsFull, kNoPos, kNoChannel, kNoCreation, kPathNotUnique };

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
  std::string key() const { return tu + "|" + func + "|" + phase + "|" + place + "|" + callee + "|" + std::to_string(line); }
};

struct Need {
  std::string subject, what, state, why, note;
};

class Gen : public RelFacts {
 public:
  Gen(const kz::Container& facts, const kz::Container* basis, const std::string& framework, const PlanOptions& o)
      : RelFacts(facts), basis_c_(basis), framework_(framework), o_(o) {}

  kz::Status run(PlanOutput* out) {
    kz::Status s = check_options();
    if (!s.ok()) return s;
    if (!(s = prepare()).ok()) return s;
    if (!(s = c_.read_section(KZ_SEC_FILE_HASHES, &files_)).ok()) return s;
    if (!(s = c_.read_section(KZ_SEC_FACT_ARGS, &args_)).ok()) return s;
    if (!(s = read_framework()).ok()) return s;
    if (!(s = read_basis()).ok()) return s;
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
    for (size_t i : planned_) plan_row(i);
    plan_actors();
    plan_stop_events();
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
    for (uint32_t o : calls_of_.count(c.caller_fn) != 0 ? calls_of_.at(c.caller_fn) : std::vector<uint32_t>{})
      if (o != ci && calls_[o].line == c.line && str(calls_[o].callee) == str(c.callee)) {
        gap(call_pos(ci), (after ? "after-call " : "at-call ") + str(c.callee) + " in " + plain(c.caller_fn), reason(kTwoCalls), rule);
        return nullptr;
      }
    Site s;
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
  void at_call(uint32_t ci, const std::string& rule, const std::string& ref, const std::vector<Val>& asked = {}) {
    Site* s = call_site(ci, false, rule, ref);
    if (s == nullptr) return;
    for (const Val& v : asked) add_val(s, v);
    for (uint32_t k = 0; k < calls_[ci].nargs && s->vals.size() < kSlots; ++k) {
      Val v;
      v.role = "arg" + std::to_string(k);
      v.from = "arg" + std::to_string(k);
      add_val(s, v);
    }
  }
  void after_call(uint32_t ci, const std::string& rule, const std::string& ref, const std::set<uint32_t>& outs = {}) {
    Site* s = call_site(ci, true, rule, ref);
    if (s == nullptr) return;
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
  }
  void entry_site(uint32_t fn, const std::string& rule, const std::string& ref, bool params) {
    if (!defined(fn)) return;
    Site s;
    s.tu = tu_text(file_path(funcs_[fn].tu));
    if (!tu_unique(s.tu)) {
      gap(fname(fn), "unit " + s.tu, reason(kPathNotUnique), rule);
      return;
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
  }
  void store_site(uint32_t fn, uint32_t line, const std::string& rule, const std::string& ref) {
    if (!defined(fn) || line == 0) return;
    Site s;
    s.tu = tu_text(file_path(funcs_[fn].tu));
    if (!tu_unique(s.tu)) {
      gap(ref, "unit " + s.tu, reason(kPathNotUnique), rule);
      return;
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
  }
  // need and gap lines, each once, in the order first written
  void need(const std::string& subject, const std::string& what, const std::string& state, const std::string& why, const std::string& note) {
    if (seen_.insert("n|" + subject + "|" + what + "|" + state + "|" + why + "|" + note).second) needs_.push_back(Need{subject, what, state, why, note});
  }
  void gap(const std::string& subject, const std::string& what, const std::string& why, const std::string& rule) {
    if (seen_.insert("g|" + subject + "|" + what + "|" + why + "|" + rule).second) gaps_.push_back(Need{subject, what, "gap", why, rule});
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
    const bool string_key = kparam != kN && defined(kfn) && reaches_compare(kfn, kparam);
    at_call(ci, rule, rowref, kparam != kN ? std::vector<Val>{key} : std::vector<Val>{});
    after_call(ci, rule, rowref, out_params(ci));
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
    if (r.path == 'A') {
      if (r.call != kN) key_call(r.call, r.callee_fn, r.param, "use-call", rowref);
      for (const std::string& l : lines) {
        if (starts(l, "creator:")) {
          std::vector<std::tuple<std::string, uint32_t, uint32_t>> ps = positions(l);
          const uint32_t ci = ps.empty() ? kN : call_of(ps[0]);
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
        } else if (starts(l, "handling:")) {
          handling(l.substr(10), r, rowref);
        }
      }
      need(rowref, "event-send", "not-planned", reason(kNotPlanned), "the effects of a result (an event or a message sent on it) are not in the facts of this revision");
    } else if (r.path == 'B') {
      for (uint32_t ci : r.site_calls) {
        Val v;
        v.role = "field_value";
        v.from = "arg" + std::to_string(r.param);
        at_call(ci, "use-call", rowref, r.param != kN ? std::vector<Val>{v} : std::vector<Val>{});
        after_call(ci, "use-call", rowref);
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
      need(rowref, "read-value", "not-planned", reason(kNotPlanned), "a read of a field is no site kind of the manifest revision 3 (loads are not bound)");
    }
    need(rowref, "wait-result", "not-planned", reason(kNotPlanned), "rule added in the manifest revision 4");
    need(rowref, "lock-result", "not-planned", reason(kNotPlanned), "rule added in the manifest revision 4");
    (void)before;
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
      at_call(pc, "actor-create", pref, asked);
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
    sc << "kz-plan-sidecar 1\n";
    sc << "generator " << o_.generator << "\n";
    for (const PlanInputFile& f : o_.inputs) sc << "input " << f.role << " sha256=" << f.sha256 << " path=" << f.path << "\n";
    sc << "facts-artifact " << kz::to_hex(c_.artifact_hash()) << "\n";
    if (basis_c_ != nullptr) sc << "basis-artifact " << kz::to_hex(basis_c_->artifact_hash()) << "\n";
    sc << "rules " << plan_rules_revision() << "; " << relations_rules_revision() << "\n";
    sc << "selection " << o_.select << ": every row of the relations (method document 3.1, the revised rule: no label and no proof of a row removes it)\n";
    sc << "priority " << o_.order << ": " << plan_priority_revision() << "; tier 1 = disposition D07 (label R or J, counterpart, order class RELOC), tier 2 = the other rows of order class RELOC, tier 3 = the rest; within a tier the order of the relations text\n";
    size_t n1 = 0, n2 = 0, n3 = 0;
    for (const PlanPriority& p : prio_) (p.tier == 1 ? n1 : p.tier == 2 ? n2 : n3)++;
    sc << "classes rows " << prio_.size() << " tier1 " << n1 << " tier2 " << n2 << " tier3 " << n3 << "\n";
    sc << "budget value=" << o_.budget << " reason=" << quoted(o_.budget_reason) << " disclosure=" << quoted(o_.budget_disclosure.empty() ? "none given" : o_.budget_disclosure)
       << " planned=" << planned_.size() << " truncation=" << (prio_.size() - planned_.size()) << " rows cut (row lines with state cut)"
       << " unresolved=" << quoted(planned_.size() < prio_.size() ? "whether the rows cut fit the record pool (no event volume measured)" : "none")
       << " sensitivity=" << quoted("not measured") << "\n";
    sc << "bound string KZ_RT_STR_MAX=" << kStrMax << " source=runtime/include/kz_rt.h:82 reason=" << quoted("the runtime's copy bound; a key text is bounded by the program's own constant length check where the facts show one")
       << " truncation=" << quoted("the runtime flags a cut string per value") << "\n";
    sc << "bound slots=" << kSlots << " source=instrument/gcc/kzgcc.cpp build_obs reason=" << quoted("values per event bound by kzgcc 0.3") << "\n";
    sc << "bound sites<=4095 source=instrument/gcc/kzgcc.cpp plugin_init reason=" << quoted("the binding id scheme") << " sites=" << sites_.size() << "\n";
    sc << "param tu-root " << (out_root_.empty() ? "-" : out_root_) << " (units outside the bundle root are named relative to it)\n";
    sc << "param bundle-root " << (broot.empty() ? "-" : broot) << "\n";
    for (size_t i = 0; i < prio_.size(); ++i) {
      const PlanPriority& p = prio_[i];
      const bool in = std::find(planned_.begin(), planned_.end(), i) != planned_.end();
      sc << "row " << p.key << " " << p.id << " tier=" << p.tier << " position=" << position_.at(i) << " label=" << p.label << " class=" << p.label_class
         << " order=" << p.order_class << " reach=" << p.reach << " disposition=" << p.disposition << " state=" << (in ? "planned" : "cut") << " why=" << quoted(p.why) << "\n";
      if (!in) sc << "need row " << p.key << " observation not-planned reason=" << reason(kBudget) << "\n";
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
    for (const Need& n : needs_) sc << "need " << n.subject << " " << n.what << " " << n.state << " reason=" << n.why << " note=" << quoted(n.note) << "\n";
    for (const Need& n : gaps_) sc << "gap " << n.subject << " " << quoted(n.what) << " reason=" << n.why << " rule=" << n.note << "\n";
    for (const std::string& c : creations_) sc << "creation " << c << "\n";
    for (const std::string& s : stops_) sc << "stop-event " << s << "\n";
    for (const std::string& m : mod_lines) sc << "module " << m << "\n";
    sc << "default-module " << id_of[core] << " " << core << "\n";
    for (const std::string& e : exes) sc << "exe " << e << "\n";
    out->sidecar = sc.str();
    const std::string side_sha = kz::to_hex(kz::sha256(out->sidecar));

    // ---- the manifest
    std::ostringstream m;
    m << "# kz plan written by kz-static plan (" << plan_rules_revision() << "; " << relations_rules_revision() << ")\n";
    m << "# sidecar sha256=" << side_sha << "\n";
    m << "# every site: a label from the sorted site keys with its rule names, a ref to the facts it came from; the plan states what to record, never what to expect\n";
    m << "kzs-sites 3\n";
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
