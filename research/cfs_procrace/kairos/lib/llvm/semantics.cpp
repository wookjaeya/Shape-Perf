// semantics.cpp -- see semantics.hpp.
#include "outcome.hpp"
#include "semantics.hpp"

#include <algorithm>
#include <fstream>
#include <functional>
#include <set>
#include <sstream>
#include <tuple>

#include "kz/core/sha256.hpp"
#include "p0.hpp"
#include "rel_facts.hpp"

namespace kzs {

const char* semantics_revision() { return "semantics join revision 1 (2026-11-16)"; }

namespace {

kz::Status bad(const std::string& path, size_t line, const std::string& why) {
  return kz::Status::error(kz::Err::Invalid, path + ":" + std::to_string(line) + ": " + why);
}

std::vector<std::string> words(const std::string& l) {
  std::vector<std::string> w;
  std::istringstream in(l);
  for (std::string x; in >> x;) w.push_back(x);
  return w;
}

// "key=value" -> value ("" when the word has another key)
std::string val(const std::string& w, const char* key) {
  const std::string k = std::string(key) + "=";
  return w.compare(0, k.size(), k) == 0 ? w.substr(k.size()) : std::string();
}

bool file_line(const std::string& w, std::string* file, uint32_t* line) {
  const size_t c = w.rfind(':');
  if (c == std::string::npos || c + 1 >= w.size()) return false;
  const std::string n = w.substr(c + 1);
  if (n.find_first_not_of("0123456789") != std::string::npos || n.size() > 9) return false;
  *file = w.substr(0, c);
  *line = static_cast<uint32_t>(std::stoul(n));
  return true;
}

bool integer(const std::string& w, int64_t* v) {
  if (w.empty()) return false;
  size_t i = w[0] == '-' ? 1 : 0;
  if (i >= w.size() || w.find_first_not_of("0123456789", i) != std::string::npos || w.size() > 20) return false;
  *v = std::stoll(w);
  return true;
}

std::string base_name(const std::string& p) {
  const size_t s = p.rfind('/');
  return s == std::string::npos ? p : p.substr(s + 1);
}

// does the recorded path `p` name the file `rel` (relative to a component root): equal, or ending with "/rel"
bool names_file(const std::string& p, const std::string& rel) {
  return p == rel || (p.size() > rel.size() && p.compare(p.size() - rel.size(), rel.size(), rel) == 0 && p[p.size() - rel.size() - 1] == '/');
}

}  // namespace

kz::Status load_semantics_file(const std::string& path, SemDataFile* out) {
  kz::Bytes bytes;
  kz::Status s = kz::read_whole_file(path, &bytes);
  if (!s.ok()) return s;
  const std::string text(bytes.begin(), bytes.end());
  *out = SemDataFile();
  out->path = path;
  out->sha256 = kz::to_hex(kz::sha256(text));
  std::map<std::string, size_t> api_at;
  std::istringstream in(text);
  std::string l;
  size_t no = 0;
  bool head = false, ended = false;
  while (std::getline(in, l)) {
    ++no;
    if (l.empty() || l[0] == '#') continue;
    if (!head) {
      if (l != "semantics-data 1") return bad(path, no, "not a semantics data file (the first line that is no comment must be \"semantics-data 1\")");
      head = true;
      continue;
    }
    if (ended) return bad(path, no, "a line after the end line");
    const std::vector<std::string> w = words(l);
    const std::string& k = w[0];
    if (k == "layer" && w.size() == 2) {
      if (w[1] != "framework" && w[1] != "p0") return bad(path, no, "unknown layer " + w[1]);
      out->layer = w[1];
    } else if (k == "component" && w.size() >= 3) {
      out->label = w[1];
      const size_t q = l.find("version=\"");
      if (q == std::string::npos || l.back() != '"') return bad(path, no, "component line without version=\"...\"");
      out->version = l.substr(q + 9, l.size() - q - 10);
    } else if (k == "rule") {
      out->rule = l.substr(5);
    } else if (k == "file" && w.size() == 3 && !val(w[2], "sha256").empty()) {
      out->files[w[1]] = val(w[2], "sha256");
    } else if ((k == "group" || k == "status-space") && w.size() == 5) {
      SemDataFile::Const c;
      c.kind = w[1];
      c.name = w[2];
      if (!integer(w[3], &c.value) || !file_line(w[4], &c.file, &c.line)) return bad(path, no, "bad " + k + " line");
      (k == "group" ? out->groups : out->status_space).push_back(c);
    } else if (k == "api" && w.size() >= 3) {
      SemDataFile::Api a;
      a.name = w[1];
      for (size_t i = 2; i < w.size(); ++i) {
        if (!val(w[i], "decl").empty()) {
          if (!file_line(val(w[i], "decl"), &a.decl_file, &a.decl_line)) return bad(path, no, "bad decl=");
        } else if (!val(w[i], "def").empty()) {
          if (val(w[i], "def") != "-") {
            if (!file_line(val(w[i], "def"), &a.def_file, &a.def_line)) return bad(path, no, "bad def=");
          }
        } else if (!val(w[i], "return").empty()) {
          a.ret_kind = val(w[i], "return");
        } else if (!val(w[i], "by").empty()) {
          a.by = val(w[i], "by");
        } else if (!val(w[i], "rule").empty()) {
          a.rule = val(w[i], "rule");
        } else if (!val(w[i], "attrs").empty()) {
          a.attrs = val(w[i], "attrs") == "-" ? std::string() : val(w[i], "attrs");
        } else if (!val(w[i], "table").empty()) {
          a.table = val(w[i], "table");
        } else {
          return bad(path, no, "unknown field " + w[i]);
        }
      }
      if (a.decl_file.empty()) return bad(path, no, "api line without decl=");
      if (api_at.count(a.name) != 0) return bad(path, no, "a second api line of " + a.name);
      api_at[a.name] = out->apis.size();
      out->apis.push_back(a);
    } else if (k == "ret" && w.size() == 5) {
      auto it = api_at.find(w[1]);
      if (it == api_at.end()) return bad(path, no, "ret line before the api line of " + w[1]);
      SemDataFile::Ret r;
      r.name = w[2];
      if (!integer(w[3], &r.value) || !file_line(w[4], &r.file, &r.line)) return bad(path, no, "bad ret line");
      out->apis[it->second].rets.push_back(r);
    } else if (k == "arg" && w.size() == 6) {
      auto it = api_at.find(w[1]);
      if (it == api_at.end()) return bad(path, no, "arg line before the api line of " + w[1]);
      SemDataFile::Arg a;
      int64_t p = 0;
      if (!integer(w[2], &p) || p < 0 || val(w[4], "by").empty() || !file_line(w[5], &a.file, &a.line)) return bad(path, no, "bad arg line");
      a.pos = static_cast<uint32_t>(p);
      a.kind = w[3];
      a.by = val(w[4], "by");
      out->apis[it->second].args.push_back(a);
    } else if (k == "body-ret" || k == "missing") {
      // the evidence of rule E3 and the P0 names without a prototype: read by the provenance check, not joined
    } else if (k == "source" && w.size() == 3 && !val(w[2], "sha256").empty()) {
      out->source_name = w[1];
      out->source_sha256 = val(w[2], "sha256");
    } else if (k == "end") {
      ended = true;
    } else {
      return bad(path, no, "not a line of the semantics data format: " + l.substr(0, 60));
    }
  }
  if (!head) return bad(path, 0, "empty file");
  if (!ended) return bad(path, no, "no end line (a cut file)");
  if (out->layer.empty() || out->label.empty()) return bad(path, no, "no layer or component line");
  for (const SemDataFile::Api& a : out->apis) {
    if (out->files.count(a.decl_file) == 0) return bad(path, 0, a.name + ": its declaration file is not pinned by a file line");
    if (!a.def_file.empty() && out->files.count(a.def_file) == 0) return bad(path, 0, a.name + ": its definition file is not pinned by a file line");
  }
  return kz::Status::success();
}

kz::Status semantics_rows(const kz::Container& facts, const std::vector<SemDataFile>& data, SemResult* out) {
  *out = SemResult();
  RelFacts rf(facts);
  kz::Status s = rf.load();
  if (!s.ok()) return s;
  rf.index();
  std::vector<kz_file_hash> fh;
  if (!(s = facts.read_section(KZ_SEC_FILE_HASHES, &fh)).ok()) return s;
  // FILE_HASHES by base name: (index, path, sha256)
  std::map<std::string, std::vector<std::tuple<uint32_t, std::string, std::string>>> by_base;
  for (size_t i = 0; i < fh.size(); ++i) {
    const std::string p = rf.str(fh[i].path);
    by_base[base_name(p)].push_back({static_cast<uint32_t>(i), p, kz::to_hex(fh[i].sha256, sizeof fh[i].sha256)});
  }
  auto held = [&](const std::string& rel, const std::string& sha) -> uint32_t {
    auto it = by_base.find(base_name(rel));
    if (it == by_base.end()) return KZ_NONE32;
    for (const auto& [i, p, h] : it->second)
      if (h == sha && names_file(p, rel)) return i;
    return KZ_NONE32;
  };
  auto fh_of_path = [&](const std::string& p) -> uint32_t {
    auto it = by_base.find(base_name(p));
    if (it == by_base.end()) return KZ_NONE32;
    for (const auto& [i, q, h] : it->second)
      if (q == p) return i;
    return KZ_NONE32;
  };
  // the functions the build calls (direct calls)
  std::set<uint32_t> called;
  for (const kz_fact_call& c : rf.calls_)
    if (c.callee_fn != KZ_NONE32) called.insert(c.callee_fn);
  auto called_fn = [&](const std::string& name) -> std::vector<uint32_t> {
    std::vector<uint32_t> v;
    auto it = rf.fn_by_name_.find(name);
    if (it != rf.fn_by_name_.end())
      for (uint32_t f : it->second)
        if (called.count(f) != 0) v.push_back(f);
    return v;
  };
  std::vector<SemRow>& rows = out->rows;
  auto row = [&](uint8_t layer, uint8_t column, uint8_t verify, uint8_t origin, const std::string& api, uint32_t fn, const std::string& component, const std::string& item,
                 const std::string& source) -> SemRow& {
    SemRow r;
    r.layer = layer;
    r.column = column;
    r.verify = verify;
    r.origin = origin;
    r.api = api;
    r.fn = fn;
    r.component = component;
    r.item = item;
    r.source = source;
    if (fn != KZ_NONE32 && fn < rf.funcs_.size()) {
      r.decl_fh = rf.funcs_[fn].fh;
      r.decl_line = rf.funcs_[fn].line;
    }
    rows.push_back(r);
    return rows.back();
  };

  // ---- P0: the entries of the P0 tables the build calls
  for (const kz_fact_p0& p : rf.p0_) {
    if (p.n_calls == 0) continue;
    const std::string name = rf.str(p.name), src = rf.str(p.source), ver = rf.str(p.version);
    const uint8_t v = src == "unverified" ? KZ_SEM_VERIFY_UNVERIFIED : KZ_SEM_VERIFY_STANDARD_COLLATED;
    const std::string source = src == "unverified" ? std::string("P0 table: source unverified") : "P0 table: " + src;
    row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_ROLE, v, KZ_SEM_ORIGIN_DECLARED, name, p.fn, ver, rf.enum_text("KZ_P0_KIND", p.kind) + " (" + rf.str(p.family) + "): " + rf.str(p.semantics),
        source);
    if (p.out_param != KZ_NONE32)
      row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_OUT_BEHAVIOUR, v, KZ_SEM_ORIGIN_DECLARED, name, p.fn, ver, "arg " + std::to_string(p.out_param) + " is written", source).pos = p.out_param;
    if (p.flags & KZ_P0_FLAG_CONDITIONAL)
      row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_BLOCKING, v, KZ_SEM_ORIGIN_DECLARED, name, p.fn, ver, "may return without its effect (a try or timed member)", source);
  }
  for (const P0CompareEntry& e : p0_compare_table())
    for (uint32_t f : called_fn(e.name)) {
      const std::string src = e.source;
      const uint8_t v = src == "unverified" ? KZ_SEM_VERIFY_UNVERIFIED : KZ_SEM_VERIFY_STANDARD_COLLATED;
      row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_ROLE, v, KZ_SEM_ORIGIN_DECLARED, e.name, f, e.version, std::string(e.family) + ": " + e.semantics,
          src == "unverified" ? std::string("P0 comparison table: source unverified") : "P0 comparison table: " + src);
    }

  // ---- the data files
  std::vector<std::string> entry_notes;
  for (size_t di = 0; di < data.size(); ++di) {
    const SemDataFile& d = data[di];
    const std::string dname = base_name(d.path);
    size_t applied = 0, by_def = 0, by_decl = 0, not_held = 0, attrs_rows = 0;
    for (const SemDataFile::Api& a : d.apis) {
      const std::vector<uint32_t> fns = called_fn(a.name);
      if (fns.empty()) continue;
      const uint32_t decl_fh = held(a.decl_file, d.files.at(a.decl_file));
      for (uint32_t fn : fns) {
        const kz_fact_func& F = rf.funcs_[fn];
        const std::string ffile = rf.str(F.file);
        bool def_ok = false, decl_ok = false;
        if (d.layer == "p0") {
          // the declaration the facts record for the function is the pinned one
          decl_ok = names_file(ffile, a.decl_file) && F.line == a.decl_line && fh_of_path(ffile) != KZ_NONE32 && fh_of_path(ffile) == decl_fh;
        } else {
          def_ok = !a.def_file.empty() && rf.defined(fn) && names_file(ffile, a.def_file) && F.line == a.def_line &&
                   held(a.def_file, d.files.at(a.def_file)) != KZ_NONE32;
          decl_ok = decl_fh != KZ_NONE32;
        }
        if (!def_ok && !decl_ok) {
          ++not_held;
          continue;
        }
        ++applied;
        if (def_ok) ++by_def;
        else ++by_decl;
        const std::string pin = def_ok ? "definition " + a.def_file + ":" + std::to_string(a.def_line) : "declaration " + a.decl_file + ":" + std::to_string(a.decl_line);
        const std::string where = " (" + dname + ", " + d.label + "; applies by its " + pin + ")";
        if (d.layer == "p0") {
          // the attributes, split at the commas outside parentheses (nonnull(1,2))
          std::vector<std::string> attrs(1);
          int depth = 0;
          for (char ch : a.attrs) {
            if (ch == '(') ++depth;
            if (ch == ')') --depth;
            if (ch == ',' && depth == 0) attrs.emplace_back();
            else attrs.back() += ch;
          }
          for (const std::string& at : attrs) {
            const std::string src = a.decl_file + ":" + std::to_string(a.decl_line) + where;
            if (at == "pure")
              row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_OUT_BEHAVIOUR, KZ_SEM_VERIFY_DOC_COLLATED, KZ_SEM_ORIGIN_DECLARED, a.name, fn, d.version,
                  "declared pure: no effect but its return value (no store through an argument)", src)
                  .src_line = a.decl_line;
            else if (at == "const")
              row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_OUT_BEHAVIOUR, KZ_SEM_VERIFY_DOC_COLLATED, KZ_SEM_ORIGIN_DECLARED, a.name, fn, d.version,
                  "declared const: no effect, and it reads no memory but its arguments", src)
                  .src_line = a.decl_line;
            else if (at == "noreturn")
              row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_ROLE, KZ_SEM_VERIFY_DOC_COLLATED, KZ_SEM_ORIGIN_DECLARED, a.name, fn, d.version, "declared noreturn: does not return", src)
                  .src_line = a.decl_line;
            else if (at.compare(0, 8, "nonnull(") == 0 && at.back() == ')') {
              std::string list, sep;
              // positions of the declaration are 1-based (the attribute's); the item names arguments 0-based
              std::string inner = at.substr(8, at.size() - 9);
              std::replace(inner.begin(), inner.end(), ',', ' ');
              std::istringstream ps(inner);
              for (std::string n; ps >> n;) {
                int64_t k = 0;
                if (integer(n, &k) && k >= 1) {
                  list += sep + "arg " + std::to_string(k - 1);
                  sep = ", ";
                }
              }
              row(KZ_SEM_LAYER_P0, KZ_SEM_COLUMN_PRECONDITION, KZ_SEM_VERIFY_DOC_COLLATED, KZ_SEM_ORIGIN_DECLARED, a.name, fn, d.version, "declared nonnull: " + list + " must not be null",
                  src)
                  .src_line = a.decl_line;
            } else {
              continue;  // nothrow, warn-unused-result: no semantics item
            }
            ++attrs_rows;
          }
          continue;
        }
        // framework: documented returns, argument roles and domains
        // a documented return of a function whose body in these facts returns no value (every RETURNS row of it has no
        // value: a function declared void) is a mismatch of the documentation and the declaration: unverified, not collated
        bool no_value = false;
        if (rf.defined(fn)) {
          auto vit = rf.returns_of_.find(fn);
          no_value = true;
          if (vit != rf.returns_of_.end())
            for (uint32_t ri : vit->second)
              if (rf.returns_[ri].vkind != KZ_RET_VALUE_NONE) no_value = false;
        }
        for (const SemDataFile::Ret& r : a.rets) {
          SemRow& x = row(KZ_SEM_LAYER_FRAMEWORK, KZ_SEM_COLUMN_DOC_RETURN, no_value ? KZ_SEM_VERIFY_UNVERIFIED : KZ_SEM_VERIFY_DOC_COLLATED, KZ_SEM_ORIGIN_DOC_ONLY, a.name, fn, d.version,
                          no_value ? r.name + " (doc-declaration mismatch: the body in these facts returns no value)" : r.name,
                          r.file + ":" + std::to_string(r.line) + where);
          x.value = r.value;
          x.src_line = r.line;
          x.flags = KZ_SEM_FLAG_VALUE_SET | KZ_SEM_FLAG_VERSION_PINNED;
        }
        for (const SemDataFile::Arg& g : a.args) {
          std::string dom, sep;
          for (const SemDataFile::Const& c : d.groups)
            if (c.kind == g.kind) {
              dom += sep + c.name + "=" + std::to_string(c.value);
              sep = ", ";
            }
          const uint8_t origin = g.by == "typed" ? KZ_SEM_ORIGIN_DECLARED : KZ_SEM_ORIGIN_DOC_ONLY;
          const std::string src = g.file + ":" + std::to_string(g.line) + where;
          SemRow& x = row(KZ_SEM_LAYER_FRAMEWORK, KZ_SEM_COLUMN_ARG_ROLE, KZ_SEM_VERIFY_DOC_COLLATED, origin, a.name, fn, d.version, g.kind + " (" + g.by + ")", src);
          x.pos = g.pos;
          x.src_line = g.line;
          x.flags = KZ_SEM_FLAG_VERSION_PINNED;
          SemRow& y = row(KZ_SEM_LAYER_FRAMEWORK, KZ_SEM_COLUMN_ARG_DOMAIN, KZ_SEM_VERIFY_DOC_COLLATED, origin, a.name, fn, d.version, g.kind + ": " + dom, src);
          y.pos = g.pos;
          y.src_line = g.line;
          y.flags = KZ_SEM_FLAG_VERSION_PINNED;
        }
        // the constants the body returns in these facts (RET_CONSTS, through the call results it returns), and the returns
        // not followed
        // the outcome classes (outcome.hpp, static_facts.md 5w): the space of the entry's component; success and failure
        // rows only for an entry whose return kind is status
        OutcomeSpace space;
        for (const SemDataFile::Const& c : d.status_space) {
          if (c.kind == "success") {
            space.has_success = true;
            space.success = c.value;
          } else {
            space.mask |= static_cast<uint64_t>(c.value) & 0xFFFFFFFFULL;
          }
        }
        const bool status_return = a.ret_kind == "status";
        {
          SemRow& x = row(KZ_SEM_LAYER_FRAMEWORK, KZ_SEM_COLUMN_NONE, KZ_SEM_VERIFY_DOC_COLLATED, a.by == "typed" ? KZ_SEM_ORIGIN_DECLARED : KZ_SEM_ORIGIN_DOC_ONLY, a.name, fn, d.version,
                          std::string(outcome_return_kind_item()) + (a.ret_kind.empty() ? std::string("unstated") : a.ret_kind) + " (" + (a.by.empty() ? std::string("-") : a.by) + ")",
                          a.decl_file + ":" + std::to_string(a.decl_line) + where);
          x.src_line = a.decl_line;
          x.flags = KZ_SEM_FLAG_VERSION_PINNED;
        }
        // a constant that comes through a definition RETURNS flags redefined later on a path to the return is marked "not
        // shown to reach the return" (relations rules revision 9, static_facts.md 5o): the body's set is then not established
        std::map<int64_t, std::string> body;
        std::vector<std::string> open;
        size_t marked = 0;
        if (rf.defined(fn)) {
          std::set<uint32_t> seen;
          std::set<int64_t> clean;
          std::function<void(uint32_t, const std::string&, bool)> rec = [&](uint32_t f, const std::string& chain, bool killed) {
            if (!rf.defined(f) || !seen.insert(f).second) return;
            auto it = rf.retconsts_of_.find(f);
            if (it != rf.retconsts_of_.end())
              for (uint32_t i : it->second) {
                const kz_fact_retconst& r = rf.retconsts_[i];
                const bool k = killed || (r.flags & KZ_RET_FLAG_REDEFINED) != 0;
                if (!k) clean.insert(r.value);
                if (body.count(r.value) == 0) body[r.value] = rf.str(r.text) + (chain.empty() ? std::string() : " through " + chain);
              }
            auto rit = rf.returns_of_.find(f);
            if (rit == rf.returns_of_.end()) return;
            for (uint32_t ri : rit->second) {
              const kz_fact_return& r = rf.returns_[ri];
              if (r.vkind == KZ_RET_VALUE_CONST || r.vkind == KZ_RET_VALUE_STRING) continue;
              const std::string at = rf.str(r.file) + ":" + std::to_string(r.line);
              if (r.vkind == KZ_RET_VALUE_CALL && r.value_root < rf.exprs_.size() && rf.exprs_[r.value_root].op == KZ_EXPR_OP_CALL_RESULT) {
                const std::string callee = rf.str(rf.exprs_[r.value_root].name);
                bool any = false;
                auto fit = rf.fn_by_name_.find(callee);
                if (fit != rf.fn_by_name_.end())
                  for (uint32_t g : fit->second)
                    if (rf.defined(g)) {
                      any = true;
                      rec(g, (chain.empty() ? rf.short_fname(f) : chain) + " -> " + rf.short_fname(g), killed || (r.flags & KZ_RET_FLAG_REDEFINED) != 0);
                    }
                if (!any) open.push_back(rf.short_fname(f) + " returns the result of " + callee + " at " + at + " (no body in these facts)");
                continue;
              }
              open.push_back(rf.short_fname(f) + " returns " + rf.enum_text("KZ_RET_VALUE", r.vkind) + " at " + at);
            }
          };
          rec(fn, "", false);
          std::sort(open.begin(), open.end());
          open.erase(std::unique(open.begin(), open.end()), open.end());
          for (auto& [v, t] : body)
            if (clean.count(v) == 0) {
              t += "; not shown to reach the return: a definition redefined later on a path to the return";
              ++marked;
            }
          for (const auto& [v, t] : body) {
            const uint8_t cls = status_return ? outcome_class(space, v) : static_cast<uint8_t>(KZ_REFF_OUTCOME_NONE);
            const uint8_t col = cls == KZ_REFF_OUTCOME_SUCCESS ? KZ_SEM_COLUMN_SUCCESS : cls == KZ_REFF_OUTCOME_FAILURE ? KZ_SEM_COLUMN_FAILURE : KZ_SEM_COLUMN_NONE;
            const std::string item = cls == KZ_REFF_OUTCOME_OTHER ? "returned constant of the class other: " + t
                                     : cls == KZ_REFF_OUTCOME_NONE ? "returned constant (the return kind is " + (a.ret_kind.empty() ? std::string("unstated") : a.ret_kind) + ": no outcome class): " + t
                                                                   : t;
            SemRow& x = row(KZ_SEM_LAYER_FRAMEWORK, col, KZ_SEM_VERIFY_BODY_DERIVED, KZ_SEM_ORIGIN_DERIVED, a.name, fn, d.version,
                            item, "RET_CONSTS of " + rf.short_fname(fn) + " in these facts");
            x.value = v;
            x.flags = KZ_SEM_FLAG_VALUE_SET;
          }
        }
        // the agreement of the documented returns and the body's
        {
          std::set<int64_t> doc;
          std::map<int64_t, std::string> doc_name;
          for (const SemDataFile::Ret& r : a.rets) {
            doc.insert(r.value);
            doc_name[r.value] = r.name;
          }
          std::string item;
          uint8_t v = KZ_SEM_VERIFY_UNVERIFIED;
          auto list = [](const std::vector<std::string>& xs) {
            std::string t, sep;
            for (const std::string& x : xs) {
              t += sep + x;
              sep = ", ";
            }
            return t;
          };
          std::vector<std::string> doc_only, body_only;
          for (int64_t x : doc)
            if (body.count(x) == 0) doc_only.push_back(doc_name[x] + " (" + std::to_string(x) + ")");
          for (const auto& [x, t] : body)
            if (doc.count(x) == 0) body_only.push_back(t + " (" + std::to_string(x) + ")");
          if (!rf.defined(fn)) {
            item = "not compared: no body in these facts";
          } else if (doc.empty()) {
            item = "not compared: no documented return";
          } else if (!open.empty() || marked != 0) {
            item = "not compared: the body's set is not established (";
            if (!open.empty()) item += std::to_string(open.size()) + " returns not followed: " + open.front() + (open.size() > 1 ? ", ..." : "");
            if (marked != 0) item += std::string(open.empty() ? "" : "; ") + std::to_string(marked) + " constants not shown to reach the return";
            item += ")";
            if (!body_only.empty()) item += "; the body may return a value the documentation omits: " + list(body_only);
          } else if (doc_only.empty() && body_only.empty()) {
            item = "derived = documented (" + std::to_string(doc.size()) + " values)";
            v = KZ_SEM_VERIFY_BODY_DERIVED;
          } else {
            v = KZ_SEM_VERIFY_BODY_DERIVED;
            if (!doc_only.empty()) item = "the documentation lists a value the body cannot return: " + list(doc_only);
            if (!body_only.empty()) item += std::string(item.empty() ? "" : "; ") + "the body returns a value the documentation omits: " + list(body_only);
          }
          row(KZ_SEM_LAYER_FRAMEWORK, KZ_SEM_COLUMN_AGREEMENT, v, KZ_SEM_ORIGIN_DERIVED, a.name, fn, d.version, item, "RET_CONSTS and RETURNS of these facts" + where);
        }
        // the roles the facts derive
        {
          std::set<std::string> roles;
          auto oit = rf.opderiv_of_.find(fn);
          if (oit != rf.opderiv_of_.end())
            for (uint32_t i : oit->second) {
              const kz_fact_opderiv& o = rf.opderiv_[i];
              roles.insert("derived " + rf.enum_text("KZ_OPDERIV_KIND", o.kind) + " operation (" + rf.enum_text("KZ_OPDERIV_MODE", o.mode) + ", primitive " + rf.str(o.primitive) + ")");
            }
          for (const kz_fact_spawnw& w : rf.spawnw_)
            if (w.fn == fn && w.kind != KZ_SPAWNW_KIND_SCRIPT_ENTRY)
              roles.insert("derived spawn operation (" + rf.enum_text("KZ_SPAWNW_KIND", w.kind) + ", primitive " + rf.str(w.primitive) + ")");
          for (const std::string& t : roles) row(KZ_SEM_LAYER_FRAMEWORK, KZ_SEM_COLUMN_ROLE, KZ_SEM_VERIFY_BODY_DERIVED, KZ_SEM_ORIGIN_DERIVED, a.name, fn, d.version, t, "OP_DERIV and SPAWN_WRAP of these facts");
        }
      }
    }
    // the status space of a component one of whose entries applies
    if (d.layer == "framework" && applied != 0)
      for (const SemDataFile::Const& c : d.status_space) {
        SemRow& x = row(KZ_SEM_LAYER_STATUS, c.kind == "success" ? KZ_SEM_COLUMN_SUCCESS : KZ_SEM_COLUMN_NONE, KZ_SEM_VERIFY_DOC_COLLATED, KZ_SEM_ORIGIN_DOC_ONLY, "", KZ_NONE32,
                        d.version, c.kind == "success" ? "success constant " + c.name : outcome_severity_item() + c.name, c.file + ":" + std::to_string(c.line) + " (" + dname + ", " + d.label + ")");
        x.value = c.value;
        x.src_line = c.line;
        x.decl_fh = d.files.count(c.file) ? held(c.file, d.files.at(c.file)) : KZ_NONE32;
        x.decl_line = x.decl_fh != KZ_NONE32 ? c.line : 0;
        x.flags = KZ_SEM_FLAG_VALUE_SET | KZ_SEM_FLAG_VERSION_PINNED;
      }
    std::string note = d.label + ": entries " + std::to_string(d.apis.size()) + ", applied " + std::to_string(applied) + " for a function the build calls (by the definition " +
                       std::to_string(by_def) + ", by the declaration " + std::to_string(by_decl) + "), called but neither pin held " + std::to_string(not_held);
    if (d.layer == "p0") note += ", declaration attribute rows " + std::to_string(attrs_rows);
    entry_notes.push_back(note);
  }
  // a stable order: layer, component, api, column, position, item, source
  std::stable_sort(rows.begin(), rows.end(), [](const SemRow& a, const SemRow& b) {
    return std::tie(a.layer, a.component, a.api, a.column, a.pos, a.item, a.source) < std::tie(b.layer, b.component, b.api, b.column, b.pos, b.item, b.source);
  });
  rows.erase(std::unique(rows.begin(), rows.end(),
                         [](const SemRow& a, const SemRow& b) {
                           return a.layer == b.layer && a.component == b.component && a.api == b.api && a.column == b.column && a.pos == b.pos && a.item == b.item &&
                                  a.source == b.source && a.fn == b.fn;
                         }),
             rows.end());
  // the counts
  size_t n[4] = {0, 0, 0, 0}, c[4] = {0, 0, 0, 0};
  for (const SemRow& r : rows) {
    if (r.layer > 3) continue;
    ++n[r.layer];
    if (r.verify != KZ_SEM_VERIFY_UNVERIFIED) ++c[r.layer];
  }
  out->notes.push_back({"extract.semantics", std::string(semantics_revision()) + ": the semantics data joined to the build (--semantics; docs/semantics_coverage.md, static_facts.md 5t), " +
                                                 std::to_string(data.size()) + " files"});
  out->notes.push_back({"extract.semantics_outcome_rule", std::string(outcome_rule_revision()) + ": the success and failure columns of the framework rows are the outcome classes of the "
                                                             "body's constants in the status space of the entry's component, for entries whose return kind is status; the "
                                                             "severity constants of a status space are rows of their own (static_facts.md 5w)"});
  for (size_t i = 0; i < data.size(); ++i)
    out->notes.push_back({"extract.semantics[" + std::to_string(i) + "]", base_name(data[i].path) + " sha256=" + data[i].sha256 + " layer=" + data[i].layer + " component=" + data[i].label});
  out->notes.push_back({"count.semantics", "rows " + std::to_string(rows.size()) + " (collated / rows: P0 " + std::to_string(c[1]) + "/" + std::to_string(n[1]) + ", framework " +
                                               std::to_string(c[2]) + "/" + std::to_string(n[2]) + ", status-space " + std::to_string(c[3]) + "/" + std::to_string(n[3]) +
                                               "; collated: a verification state other than unverified)"});
  for (size_t i = 0; i < entry_notes.size(); ++i) out->notes.push_back({"count.semantics_entries[" + std::to_string(i) + "]", entry_notes[i]});
  return kz::Status::success();
}

}  // namespace kzs
