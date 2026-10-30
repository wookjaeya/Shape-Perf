// facts_text.cpp -- see facts_text.hpp.
#include "facts_text.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <tuple>
#include <sstream>

#include "kz/core/dump.hpp"
#include "kz/core/registry.hpp"

namespace kzs {

namespace {

const char* const kSections[] = {"basis",   "files",   "functions", "calls",     "derives",  "loops",       "iters",  "order", "init",
                                 "consts",  "writers", "atype",     "locs",      "targets",  "spawnparams", "image",  "terminals",
                                 "outcalls", "fwrites", "p0",      "derived",   "spawnwrap", "reads",  "returns", "retconsts", "loopexits",
                                 "argflow", "paramflow", "ptrfields", "gimages"};

class View {
 public:
  View(const kz::Container& c, const TextOptions& o) : c_(c), o_(o) {}

  kz::Status run(std::string* out) {
    kz::Status s = load();
    if (!s.ok()) return s;
    std::ostringstream os;
    os << "facts-view 1\n";
    if (o_.full_hashes) os << "artifact " << kz::to_hex(c_.artifact_hash()) << "\n";
    const struct {
      const char* name;
      void (View::*fn)(std::ostream&);
    } table[] = {{"basis", &View::basis},         {"files", &View::files},       {"functions", &View::functions}, {"calls", &View::calls},
                 {"derives", &View::derives},     {"loops", &View::loops},       {"iters", &View::iters},         {"order", &View::order},          {"init", &View::init},
                 {"consts", &View::consts},       {"writers", &View::writers},   {"atype", &View::atype},         {"locs", &View::locs},
                 {"targets", &View::targets},     {"spawnparams", &View::spawnparams}, {"image", &View::image},   {"terminals", &View::terminals},
                 {"outcalls", &View::outcalls},   {"fwrites", &View::fwrites}};
    for (const auto& t : table)
      if (o_.sections.empty() || o_.sections.count(t.name) != 0) (this->*t.fn)(os);
    // the sections of the profile-free mode (revision 5) and the extended facts (revision 6), when the
    // container has them: a container without them reads as before
    const struct {
      const char* name;
      uint32_t id;
      void (View::*fn)(std::ostream&);
    } added[] = {{"p0", KZ_SEC_P0_TABLE, &View::p0},           {"derived", KZ_SEC_OP_DERIV, &View::derived}, {"spawnwrap", KZ_SEC_SPAWN_WRAP, &View::spawnwrap},
                 {"reads", KZ_SEC_READS, &View::reads},        {"returns", KZ_SEC_RETURNS, &View::returns},  {"retconsts", KZ_SEC_RET_CONSTS, &View::retconsts},
                 {"loopexits", KZ_SEC_LOOP_EXITS, &View::loopexits},  {"argflow", KZ_SEC_ARGFLOW, &View::argflow},     {"paramflow", KZ_SEC_PARAM_FLOW, &View::paramflow},
                 {"ptrfields", KZ_SEC_PTR_FIELDS, &View::ptrfields},  {"gimages", KZ_SEC_GLOBAL_IMAGES, &View::gimages}};
    for (const auto& t : added)
      if (c_.find_section(t.id) != nullptr && (o_.sections.empty() || o_.sections.count(t.name) != 0)) (this->*t.fn)(os);
    *out = os.str();
    return kz::Status::success();
  }

 private:
  template <class T>
  kz::Status read(uint32_t id, std::vector<T>* v) {
    return c_.read_section(id, v);
  }
  kz::Status load() {
    kz::Status s;
    if (!(s = read(KZ_SEC_BASIS, &basis_)).ok()) return s;
    if (!(s = read(KZ_SEC_FILE_HASHES, &files_)).ok()) return s;
    if (!(s = read(KZ_SEC_FUNCS, &funcs_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACTS, &calls_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_ARGS, &args_)).ok()) return s;
    if (!(s = read(KZ_SEC_DERIVES, &derives_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_EXPRS, &exprs_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_LOOPS, &loops_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_INIT, &inits_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_CONSTS, &consts_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_ATYPE, &atypes_)).ok()) return s;
    if (!(s = read(KZ_SEC_WRITERS, &writers_)).ok()) return s;
    if (!(s = read(KZ_SEC_CALL_TARGETS, &targets_)).ok()) return s;
    if (!(s = read(KZ_SEC_STATIC_IMAGE, &images_)).ok()) return s;
    if (!(s = read(KZ_SEC_FACT_LOCS, &locs_)).ok()) return s;
    if (!(s = read(KZ_SEC_TERMINALS, &terminals_)).ok()) return s;
    if (!(s = read(KZ_SEC_OUTCALLS, &outcalls_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOOP_ITERS, &iters_)).ok()) return s;
    if (!(s = read(KZ_SEC_SPAWN_PARAMS, &spawn_params_)).ok()) return s;
    if (!(s = read(KZ_SEC_FUNC_WRITES, &fwrites_)).ok()) return s;
    if (!(s = read(KZ_SEC_P0_TABLE, &p0_)).ok()) return s;
    if (!(s = read(KZ_SEC_OP_DERIV, &opderiv_)).ok()) return s;
    if (!(s = read(KZ_SEC_SPAWN_WRAP, &spawnw_)).ok()) return s;
    if (!(s = read(KZ_SEC_READS, &reads_)).ok()) return s;
    if (!(s = read(KZ_SEC_RETURNS, &returns_)).ok()) return s;
    if (!(s = read(KZ_SEC_RET_CONSTS, &retconsts_)).ok()) return s;
    if (!(s = read(KZ_SEC_LOOP_EXITS, &loopexits_)).ok()) return s;
    if (!(s = read(KZ_SEC_ARGFLOW, &argflow_)).ok()) return s;
    if (!(s = read(KZ_SEC_PARAM_FLOW, &pflow_)).ok()) return s;
    if (!(s = read(KZ_SEC_PTR_FIELDS, &ptrfields_)).ok()) return s;
    if (!(s = read(KZ_SEC_GLOBAL_IMAGES, &gimages_)).ok()) return s;
    for (size_t i = 0; i < calls_.size(); ++i) call_by_site_[calls_[i].site] = i;
    for (size_t i = 0; i < loops_.size(); ++i) loop_by_site_[loops_[i].site] = i;
    return kz::Status::success();
  }

  std::string str(const kz_str& s) const {
    std::string out;
    c_.string(s, &out);
    return out;
  }
  static std::string idx(uint32_t v) { return v == KZ_NONE32 ? "-" : std::to_string(v); }
  std::string fn_name(uint32_t fn) const {
    if (fn == KZ_NONE32) return "-";
    if (fn >= funcs_.size()) return "fn#" + std::to_string(fn) + "?";
    std::string n = str(funcs_[fn].name);
    if (funcs_[fn].flags & KZ_FUNC_FLAG_STATIC) n += "@" + str(funcs_[fn].file) + ":" + std::to_string(funcs_[fn].line);
    return n;
  }
  std::string enum_text(const char* name, uint64_t v) const {
    const kz::EnumDesc* d = kz::find_enum(name);
    if (d == nullptr) return std::to_string(v);
    const char* t = kz::enum_display(*d, v);
    return t != nullptr ? t : "#" + std::to_string(v);
  }
  std::string flags_text(const char* name, uint64_t v) const {
    const kz::EnumDesc* d = kz::find_enum(name);
    std::string out = "{";
    bool first = true;
    if (d != nullptr)
      for (size_t i = 0; i < d->nitems; ++i)
        if (d->items[i].value != 0 && (v & d->items[i].value) == d->items[i].value) {
          if (!first) out += ",";
          out += d->items[i].display;
          first = false;
        }
    return out + "}";
  }
  std::string pos(const kz_str& file, uint32_t line, uint32_t col) const {
    std::string p = str(file) + ":" + std::to_string(line);
    if (col != 0) p += ":" + std::to_string(col);
    return p;
  }
  std::string quoted(const std::string& s) const { return "\"" + kz::escape_string(s) + "\""; }

  std::string expr(uint32_t root, int depth = 0) const {
    if (root == KZ_NONE32) return "-";
    if (root >= exprs_.size()) return "expr#" + std::to_string(root) + "?";
    if (depth > 40) return "...";
    const kz_fact_expr& e = exprs_[root];
    const std::string name = str(e.name);
    const bool leaf = e.a == KZ_NONE32 && e.b == KZ_NONE32;
    if (leaf && e.op == KZ_EXPR_OP_CONST) return std::to_string(e.value);
    if (leaf && e.op == KZ_EXPR_OP_LOAD) {
      // a string with a NUL byte is written escaped: the view is text, and a NUL would end it for a C reader
      std::string t = enum_text("KZ_EXPR_REF", e.flags) + ":" + (name.find('\0') == std::string::npos ? name : kz::escape_string(name));
      if (e.flags == KZ_EXPR_REF_PARAM) t += "#" + std::to_string(e.value);
      return t;
    }
    std::string t = "(" + enum_text("KZ_EXPR_OP", e.op);
    if (!name.empty()) t += " " + name;
    if (e.op == KZ_EXPR_OP_CONST || e.op == KZ_EXPR_OP_FORALL || e.op == KZ_EXPR_OP_CURSOR) t += " " + std::to_string(e.value);
    if (e.a != KZ_NONE32) t += " " + expr(e.a, depth + 1);
    if (e.b != KZ_NONE32) t += " " + expr(e.b, depth + 1);
    return t + ")";
  }

  bool want_loc(const kz_str& type, const kz_str& field) const {
    if (o_.loc.empty()) return true;
    return str(type) + "." + str(field) == o_.loc;
  }
  bool want_callee(const std::string& name) const { return o_.callee.empty() || o_.callee == name; }

  // ---- sections ----
  void basis(std::ostream& os) {
    os << "[basis] " << basis_.size() << "\n";
    for (const kz_basis_item& b : basis_) os << "  " << str(b.name) << " = " << str(b.value) << "  (" << enum_text("KZ_POLICY", b.policy) << ")\n";
  }
  void files(std::ostream& os) {
    os << "[files] " << files_.size() << "\n";
    for (size_t i = 0; i < files_.size(); ++i) {
      const std::string hex = kz::to_hex(files_[i].sha256, 32);
      os << "  " << i << " " << str(files_[i].path) << " " << (o_.full_hashes ? hex : hex.substr(0, 12)) << " " << files_[i].size << "\n";
    }
  }
  void functions(std::ostream& os) {
    os << "[functions] " << funcs_.size() << "\n";
    for (const kz_fact_func& f : funcs_) {
      os << "  fn " << f.fn << " " << str(f.name) << " " << pos(f.file, f.line, 0);
      if (f.end_line != 0) os << "-" << f.end_line;
      os << " params=" << f.nparams << " " << flags_text("KZ_FUNC_FLAG", f.flags) << "\n";
    }
  }
  std::string arg_text(const kz_fact_arg& a) const {
    std::string t = std::to_string(a.index) + ":" + enum_text("KZ_ARG_KIND", a.kind);
    if (a.kind == KZ_ARG_KIND_CONST) t += " " + std::to_string(a.value);
    const std::string sym = str(a.symbol);
    if (!sym.empty() && a.kind != KZ_ARG_KIND_TOP) t += " " + (a.kind == KZ_ARG_KIND_STRING ? quoted(sym) : sym);
    else if (!sym.empty()) t += " ~" + sym;
    if (a.role != KZ_ARG_ROLE_NONE) t += " role=" + enum_text("KZ_ARG_ROLE", a.role);
    if (a.kind == KZ_ARG_KIND_ADDR_FIELD) t += " of " + str(a.loc_type) + "." + str(a.loc_field);
    if (a.fn != KZ_NONE32) t += " fn=" + fn_name(a.fn);
    return t;
  }
  void calls(std::ostream& os) {
    size_t n = 0;
    for (const kz_fact_call& c : calls_)
      if (want_callee(str(c.callee))) ++n;
    os << "[calls] " << n << "\n";
    for (const kz_fact_call& c : calls_) {
      if (!want_callee(str(c.callee))) continue;
      os << "  site " << c.site << " " << pos(c.file, c.line, c.col) << " " << fn_name(c.caller_fn) << " -> ";
      os << (c.kind == KZ_CALL_KIND_INDIRECT ? std::string("<indirect>") : str(c.callee)) << " " << enum_text("KZ_CALL_KIND", c.kind) << " "
         << flags_text("KZ_CALL_FLAG", c.flags);
      if (c.loop != KZ_NONE32) os << " loop=" << c.loop << " depth=" << c.loop_depth;
      os << "\n";
      for (uint32_t i = 0; i < c.nargs && c.first_arg + i < args_.size(); ++i) os << "      arg " << arg_text(args_[c.first_arg + i]) << "\n";
    }
  }
  void derives(std::ostream& os) {
    std::vector<const kz_fact_derives*> sel;
    for (const kz_fact_derives& d : derives_)
      if (want_loc(d.loc_type, d.loc_field) && want_callee(str(d.callee))) sel.push_back(&d);
    os << "[derives] " << sel.size() << "\n";
    for (const kz_fact_derives* d : sel) {
      os << "  site " << d->site << " arg " << d->arg_index << " of " << str(d->callee) << ": " << enum_text("KZ_DERIVES_STATUS", d->status);
      if (d->status == KZ_DERIVES_STATUS_DERIVED)
        os << " from " << str(d->loc_type) << "." << str(d->loc_field) << " read at " << pos(d->file, d->line, d->col) << " hops=" << d->hops << " chain=" << expr(d->chain_root);
      else if (d->status == KZ_DERIVES_STATUS_NOT_DERIVED)
        os << " (" << enum_text("KZ_DERIVES_REASON", d->reason) << ")";
      os << "\n";
    }
  }
  void loops(std::ostream& os) {
    os << "[loops] " << loops_.size() << "\n";
    for (const kz_fact_loop& l : loops_) {
      os << "  loop " << l.loop << " site " << l.site << " " << pos(l.file, l.line, l.col) << " in " << fn_name(l.fn) << " depth=" << l.depth << " "
         << enum_text("KZ_LOOP_KIND", l.kind) << " trip=" << enum_text("KZ_TRIP_STATUS", l.trip_status);
      if (l.trip_status == KZ_TRIP_STATUS_EXACT || l.trip_status == KZ_TRIP_STATUS_UPPER_BOUND) os << "(" << l.trip << ")";
      os << " exits=" << l.n_exits << " calls=" << l.n_calls;
      const std::string table = str(l.table);
      if (!table.empty()) os << " table=" << table;
      const std::string note = str(l.trip_note);
      if (!note.empty()) os << " (" << note << ")";
      os << "\n";
      if (l.delay_site != KZ_NONE32) os << "      delay-call site " << l.delay_site << "\n";
      if (l.lock_site != KZ_NONE32 || l.lock_kind != KZ_LOCK_KIND_NONE) os << "      lock " << enum_text("KZ_LOCK_KIND", l.lock_kind) << " site " << idx(l.lock_site) << "\n";
      if (l.exit_root != KZ_NONE32) os << "      exit " << expr(l.exit_root) << "\n";
      if (l.modset_root != KZ_NONE32) os << "      mod " << list(l.modset_root) << "\n";
      if (l.budget_root != KZ_NONE32) os << "      budget " << list(l.budget_root) << "\n";
      if (l.cond_root != KZ_NONE32) os << "      condition-exits " << expr(l.cond_root) << "\n";
      if (l.pred_root != KZ_NONE32) os << "      predicate " << enum_text("KZ_PRED_FORM", l.pred_form) << " " << expr(l.pred_root) << "\n";
      os << "      rule " << enum_text("KZ_LOOP_CERT", l.cert) << " " << flags_text("KZ_LOOP_FLAG", l.lflags);
      if (l.parent != KZ_NONE32) os << " parent=" << l.parent;
      os << ": " << str(l.cert_note) << "\n";
    }
  }
  void iters(std::ostream& os) {
    os << "[iters] " << iters_.size() << "\n";
    for (const kz_fact_iter& it : iters_) {
      if (it.kind == KZ_ITER_KIND_ELEMENT) {
        os << "  loop " << it.loop << " iter " << it.iter << " element " << enum_text("KZ_IMAGE_KIND", it.value_kind) << " " << str(it.text);
        if (it.line != 0) os << " at " << pos(it.file, it.line, 0);
        os << "\n";
        continue;
      }
      std::string callee;
      auto cit = call_by_site_.find(it.site);
      if (cit != call_by_site_.end()) callee = calls_[cit->second].kind == KZ_CALL_KIND_INDIRECT ? std::string("<indirect>") : str(calls_[cit->second].callee);
      if (it.kind == KZ_ITER_KIND_CALL) {
        os << "    call site " << it.site << " " << callee << " " << enum_text("KZ_ITER_STATUS", it.status);
        if (it.guard_root != KZ_NONE32) os << " if " << expr(it.guard_root);
        os << "\n";
        continue;
      }
      os << "      " << (it.kind == KZ_ITER_KIND_CALLEE ? std::string("callee") : "arg " + std::to_string(it.arg_index)) << " = " << enum_text("KZ_IMAGE_KIND", it.value_kind);
      if (it.value_kind == KZ_IMAGE_KIND_INT) os << " " << it.value;
      else if (it.fn != KZ_NONE32) os << " " << fn_name(it.fn);
      else if (!str(it.text).empty()) os << " " << (it.value_kind == KZ_IMAGE_KIND_STRING ? quoted(str(it.text)) : str(it.text));
      if (it.spawn != KZ_ITER_SPAWN_NONE) os << " [" << enum_text("KZ_ITER_SPAWN", it.spawn) << "]";
      if (it.line != 0) os << " at " << pos(it.file, it.line, 0);
      os << "\n";
    }
  }
  // The order a table walk establishes, read off the unrolled iterations: for every call whose
  // argument is the entry or the task-main of a spawn, and for every indirect call through a
  // field of the element, the iterations in which the call is reached, in order.
  void order(std::ostream& os) {
    struct Key {
      uint32_t loop, site, arg;
      bool operator<(const Key& o) const { return std::tie(loop, site, arg) < std::tie(o.loop, o.site, o.arg); }
    };
    std::map<Key, std::vector<const kz_fact_iter*>> rows;
    std::map<std::pair<uint32_t, uint32_t>, const kz_fact_iter*> call_row, element_row;  // (loop, iter[, site])
    std::map<std::tuple<uint32_t, uint32_t, uint32_t>, const kz_fact_iter*> call_of;
    for (const kz_fact_iter& it : iters_) {
      if (it.kind == KZ_ITER_KIND_ELEMENT) element_row[{it.loop, it.iter}] = &it;
      if (it.kind == KZ_ITER_KIND_CALL) call_of[std::make_tuple(it.loop, it.iter, it.site)] = &it;
      if ((it.kind == KZ_ITER_KIND_ARG && it.spawn != KZ_ITER_SPAWN_NONE) || it.kind == KZ_ITER_KIND_CALLEE)
        rows[Key{it.loop, it.site, it.kind == KZ_ITER_KIND_CALLEE ? KZ_NONE32 : it.arg_index}].push_back(&it);
    }
    os << "[order] " << rows.size() << "\n";
    for (const auto& kv : rows) {
      const Key& k = kv.first;
      std::string callee;
      auto cit = call_by_site_.find(k.site);
      if (cit != call_by_site_.end()) callee = calls_[cit->second].kind == KZ_CALL_KIND_INDIRECT ? std::string("<indirect>") : str(calls_[cit->second].callee);
      os << "  loop " << k.loop;
      if (k.loop < loops_.size()) os << " " << pos(loops_[k.loop].file, loops_[k.loop].line, 0) << " in " << fn_name(loops_[k.loop].fn);
      os << ": call site " << k.site << " " << callee;
      if (k.arg == KZ_NONE32) os << " (the callee per iteration)";
      else os << " arg " << k.arg << " (" << enum_text("KZ_ITER_SPAWN", kv.second.front()->spawn) << ")";
      os << "\n";
      for (const kz_fact_iter* it : kv.second) {
        auto er = element_row.find({it->loop, it->iter});
        auto cr = call_of.find(std::make_tuple(it->loop, it->iter, it->site));
        os << "    iter " << it->iter << " " << (er != element_row.end() ? str(er->second->text) : std::string("?")) << " -> ";
        if (it->fn != KZ_NONE32) os << fn_name(it->fn);
        else os << enum_text("KZ_IMAGE_KIND", it->value_kind) << (str(it->text).empty() ? "" : " " + str(it->text));
        if (cr != call_of.end()) {
          os << "  " << enum_text("KZ_ITER_STATUS", cr->second->status);
          if (cr->second->guard_root != KZ_NONE32) os << " if " << expr(cr->second->guard_root);
        }
        os << "\n";
      }
    }
  }
  void spawnparams(std::ostream& os) {
    os << "[spawnparams] " << spawn_params_.size() << "\n";
    for (const kz_fact_spawn_param& sp : spawn_params_) {
      os << "  " << fn_name(sp.fn) << " param " << sp.param << " " << enum_text("KZ_SPAWN_PARAM_KIND", sp.kind) << ": spawn site " << idx(sp.spawn_site);
      if (sp.entry_fn != KZ_NONE32) os << " entry " << fn_name(sp.entry_fn);
      if (sp.call_site != KZ_NONE32) {
        os << " called at site " << sp.call_site;
        auto cit = call_by_site_.find(sp.call_site);
        if (cit != call_by_site_.end()) os << " " << pos(calls_[cit->second].file, calls_[cit->second].line, calls_[cit->second].col);
      }
      os << " via " << str(sp.via) << "\n";
    }
  }
  void fwrites(std::ostream& os) {
    std::vector<const kz_fact_fwrite*> sel;
    for (const kz_fact_fwrite& f : fwrites_) {
      if (!o_.loc.empty()) {
        const std::string t = str(f.type), fl = str(f.field);
        const size_t dot = o_.loc.rfind('.');
        const std::string lt = dot == std::string::npos ? o_.loc : o_.loc.substr(0, dot), lf = dot == std::string::npos ? "" : o_.loc.substr(dot + 1);
        if (!(t == lt && (fl == lf || fl == "*"))) continue;
      }
      sel.push_back(&f);
    }
    os << "[fwrites] " << sel.size() << "\n";
    for (const kz_fact_fwrite* f : sel) {
      os << "  " << fn_name(f->fn) << " " << enum_text("KZ_FWRITE_KIND", f->kind);
      if (f->kind != KZ_FWRITE_KIND_UNTYPED) os << " " << str(f->type) << "." << str(f->field);
      os << " at " << pos(f->file, f->line, 0) << "\n";
    }
  }
  // ---- the profile-free mode (revision 5) ----
  std::string site_text(uint32_t site) const {
    if (site == KZ_NONE32) return "-";
    std::string t = "site " + std::to_string(site);
    auto cit = call_by_site_.find(site);
    if (cit != call_by_site_.end()) t += " " + pos(calls_[cit->second].file, calls_[cit->second].line, calls_[cit->second].col);
    return t;
  }
  void p0(std::ostream& os) {
    os << "[p0] " << p0_.size() << "\n";
    for (const kz_fact_p0& e : p0_) {
      os << "  " << str(e.name) << " " << enum_text("KZ_P0_KIND", e.kind) << " (" << str(e.family) << ")";
      if (e.out_param != KZ_NONE32) os << " out=" << e.out_param;
      if (e.entry_param != KZ_NONE32) os << " entry=" << e.entry_param;
      if (e.arg_param != KZ_NONE32) os << " arg=" << e.arg_param;
      os << " calls=" << e.n_calls << " " << flags_text("KZ_P0_FLAG", e.flags) << (e.fn != KZ_NONE32 ? "" : " (not in the program)") << "\n";
      os << "      " << str(e.semantics) << "\n";
      os << "      version " << str(e.version) << "; source " << str(e.source) << "\n";
    }
  }
  void derived(std::ostream& os) {
    os << "[derived] " << opderiv_.size() << "\n";
    for (const kz_fact_opderiv& r : opderiv_) {
      os << "  " << fn_name(r.fn) << " " << enum_text("KZ_OPDERIV_KIND", r.kind) << " " << enum_text("KZ_OPDERIV_MODE", r.mode) << " " << flags_text("KZ_OPDERIV_FLAG", r.flags)
         << ": " << site_text(r.site) << " -> " << fn_name(r.next_fn) << " depth=" << r.depth << " guard=" << expr(r.guard_root) << "\n";
      os << "      chain " << str(r.chain) << "\n";
    }
  }
  void spawnwrap(std::ostream& os) {
    os << "[spawnwrap] " << spawnw_.size() << "\n";
    for (const kz_fact_spawnw& r : spawnw_) {
      os << "  " << fn_name(r.fn) << " " << enum_text("KZ_SPAWNW_KIND", r.kind);
      if (r.param != KZ_NONE32) os << " param " << r.param;
      os << " " << flags_text("KZ_SPAWNW_FLAG", r.flags) << ": " << site_text(r.site) << " -> " << fn_name(r.next_fn) << " depth=" << r.depth;
      if (r.entry_fn != KZ_NONE32 || r.kind == KZ_SPAWNW_KIND_SCRIPT_ENTRY) os << " entry " << fn_name(r.entry_fn);
      if (r.call_site != KZ_NONE32) os << " call " << site_text(r.call_site);
      os << "\n      chain " << str(r.chain) << "\n";
      const std::string via = str(r.via);
      if (!via.empty()) os << "      via " << via << "\n";
    }
  }

  // ---- the extended facts F1-F3 (revision 6) ----
  void reads(std::ostream& os) {
    std::vector<const kz_fact_read*> sel;
    for (const kz_fact_read& r : reads_)
      if (want_loc(r.type, r.field)) sel.push_back(&r);
    os << "[reads] " << sel.size() << "\n";
    for (const kz_fact_read* r : sel) {
      os << "  " << fn_name(r->fn) << " reads " << enum_text("KZ_READ_KIND", r->kind) << " " << str(r->type) << "." << str(r->field) << " at " << pos(r->file, r->line, r->col)
         << " root=" << enum_text("KZ_READ_ROOT", r->root_kind) << ":" << str(r->root);
      if (r->root_param != KZ_NONE32) os << "#" << r->root_param;
      os << " use=" << flags_text("KZ_READ_USE", r->use) << " " << flags_text("KZ_READ_FLAG", r->flags) << " [" << str(r->text) << "]";
      if (r->index_root != KZ_NONE32) os << " index=" << list(r->index_root);
      if (r->site != KZ_NONE32) os << " arg " << r->arg_index << " of " << site_text(r->site);
      if (r->loop != KZ_NONE32) os << " loop=" << r->loop;
      if (r->ctx_root != KZ_NONE32) os << " ctx=" << expr(r->ctx_root);
      if (r->guard_root != KZ_NONE32) os << " guard=" << expr(r->guard_root);
      os << "\n";
    }
  }
  void returns(std::ostream& os) {
    os << "[returns] " << returns_.size() << "\n";
    for (const kz_fact_return& r : returns_) {
      os << "  " << fn_name(r.fn) << " returns at " << pos(r.file, r.line, r.col) << " alt " << r.alt << "/" << r.n_alts << " " << enum_text("KZ_RET_VALUE", r.vkind) << " "
         << expr(r.value_root) << " [" << str(r.text) << "] " << flags_text("KZ_RET_FLAG", r.flags);
      if (r.site != KZ_NONE32) os << " call " << site_text(r.site);
      if (r.loop != KZ_NONE32) os << " loop=" << r.loop;
      os << "\n";
      if (r.def_line != 0) os << "      definition of " << str(r.local) << " at " << pos(r.def_file, r.def_line, r.def_col) << " guard=" << expr(r.def_guard_root) << "\n";
      os << "      guard=" << expr(r.guard_root) << " when=" << expr(r.cond_root) << "\n";
    }
  }
  void retconsts(std::ostream& os) {
    os << "[retconsts] " << retconsts_.size() << "\n";
    for (const kz_fact_retconst& r : retconsts_)
      os << "  " << fn_name(r.fn) << " returns " << r.value << " [" << str(r.text) << "] rows=" << r.n_rows << " " << flags_text("KZ_RET_FLAG", r.flags)
         << " when=" << (r.cond_root == KZ_NONE32 ? std::string("always") : expr(r.cond_root)) << "\n";
  }
  void loopexits(std::ostream& os) {
    os << "[loopexits] " << loopexits_.size() << "\n";
    for (const kz_fact_loopexit& r : loopexits_) {
      if (r.kind == KZ_LOOPEXIT_KIND_LOOP) {
        os << "  loop " << r.loop << " " << pos(r.file, r.line, r.col) << " in " << fn_name(r.fn) << " exits=" << r.n << " " << flags_text("KZ_LOOPEXIT_FLAG", r.flags)
           << " exit=" << expr(r.root) << " mods=" << list(r.aux_root) << "\n";
      } else {
        os << "      exit " << r.exit << " " << enum_text("KZ_LOOPEXIT_KIND", r.kind) << " at " << pos(r.file, r.line, r.col) << " " << flags_text("KZ_LOOPEXIT_FLAG", r.flags)
           << " when=" << expr(r.root) << " by=" << expr(r.aux_root) << "\n";
      }
    }
  }

  // ---- the extended facts F4 and F6 (revision 7) ----
  void argflow(std::ostream& os) {
    std::vector<const kz_fact_argflow*> sel;
    for (const kz_fact_argflow& a : argflow_) {
      auto it = call_by_site_.find(a.site);
      const std::string callee = it == call_by_site_.end() ? "" : str(calls_[it->second].callee);
      if (want_callee(callee) && want_loc(a.type, a.field)) sel.push_back(&a);
    }
    os << "[argflow] " << sel.size() << "\n";
    for (const kz_fact_argflow* a : sel) {
      os << "  arg " << a->arg_index << " of " << site_text(a->site) << " [" << str(a->text) << "] <- " << enum_text("KZ_ARGSRC", a->src);
      if (a->src == KZ_ARGSRC_CONST) os << " " << a->value;
      if (!str(a->type).empty()) os << " " << str(a->type) << "." << str(a->field);
      const std::string root = str(a->root);
      if (!root.empty()) os << " root=" << (a->src == KZ_ARGSRC_STRING ? quoted(root) : root);
      if (a->param != KZ_NONE32) os << " param=" << a->param;
      if (a->base != KZ_ARGBASE_NONE) os << " base=" << enum_text("KZ_ARGBASE", a->base);
      const std::string obj = str(a->object);
      if (!obj.empty()) os << " object=" << obj;
      os << " hops=" << a->hops << " " << flags_text("KZ_ARGFLOW_FLAG", a->flags);
      if (a->reason != KZ_DERIVES_REASON_NONE) os << " (" << enum_text("KZ_DERIVES_REASON", a->reason) << ")";
      if (a->index_root != KZ_NONE32) os << " index=" << list(a->index_root);
      if (a->read != KZ_NONE32) os << " read=" << a->read;
      os << " chain=" << expr(a->chain_root) << "\n";
    }
  }
  void paramflow(std::ostream& os) {
    os << "[paramflow] " << pflow_.size() << "\n";
    for (const kz_fact_pflow& r : pflow_) {
      os << "  " << fn_name(r.fn) << "#" << r.param << " d" << r.depth << " " << enum_text("KZ_PFLOW_KIND", r.kind);
      if (r.kind == KZ_PFLOW_KIND_OPEN) {
        os << " " << flags_text("KZ_PFLOW_FLAG", r.flags) << " left out " << r.n << " [" << str(r.text) << "]\n";
        continue;
      }
      if (r.depth == 0 && r.kind == KZ_PFLOW_KIND_CALL_ARG) {
        os << " -> " << (r.next_fn != KZ_NONE32 ? fn_name(r.next_fn) : std::string("?")) << "#" << idx(r.next_param) << " at " << site_text(r.site);
      } else if (r.depth > 0) {
        os << " in " << fn_name(r.sink_fn) << " at " << pos(r.file, r.line, r.col) << " via " << site_text(r.site);
        if (r.sink_site != KZ_NONE32) os << " (call " << site_text(r.sink_site) << " arg " << r.n << ")";
      } else {
        os << " at " << pos(r.file, r.line, r.col);
      }
      os << " " << flags_text("KZ_PFLOW_FLAG", r.flags) << " [" << str(r.text) << "]";
      if (r.expr_root != KZ_NONE32) os << " expr=" << expr(r.expr_root);
      if (r.guard_root != KZ_NONE32) os << " guard=" << expr(r.guard_root);
      os << "\n";
      const std::string ch = str(r.chain);
      if (!ch.empty()) os << "      chain " << ch << "\n";
    }
  }
  void ptrfields(std::ostream& os) {
    os << "[ptrfields] " << ptrfields_.size() << "\n";
    for (size_t i = 0; i < ptrfields_.size(); ++i) {
      const kz_fact_ptrfield& r = ptrfields_[i];
      os << "  " << i << " " << enum_text("KZ_PTRF_KIND", r.kind) << " " << str(r.type) << "." << str(r.field) << " at " << pos(r.file, r.line, r.col);
      if (r.kind == KZ_PTRF_KIND_READ) {
        os << " in " << fn_name(r.fn) << " read=" << idx(r.read) << " root=" << str(r.root) << " " << enum_text("KZ_PTRF_STATUS", r.status) << " candidates=" << r.n;
        if (r.write != KZ_NONE32) os << " write=" << r.write << " -> " << str(r.object) << " path=" << str(r.path);
        if (r.index_root != KZ_NONE32) os << " index=" << expr(r.index_root);
        if (r.idx_n > 0) os << " elements=" << r.elem << "+" << r.idx_step << "k (k<" << r.idx_n << ")";
        os << " " << flags_text("KZ_PTRF_FLAG", r.flags) << "\n";
        // the static values the resolved elements have (GLOBAL_IMAGES), when the elements are known
        if (r.write != KZ_NONE32 && r.idx_n > 0 && r.idx_n <= 64) {
          const std::string obj = str(r.object), rest = str(r.path);
          for (uint32_t k = 0; k < r.idx_n; ++k) {
            const int64_t e = r.elem + static_cast<int64_t>(k) * (r.idx_n > 1 ? r.idx_step : 0);
            const std::string want = "[" + std::to_string(e) + "]" + rest;
            for (const kz_fact_gimage& g : gimages_)
              if (str(g.object) == obj && (str(g.path) == want || str(g.path) == rest)) os << "      " << obj << str(g.path) << " = " << image_value(g) << "\n";
          }
        }
      } else {
        os << (r.fn != KZ_NONE32 ? " in " + fn_name(r.fn) : std::string()) << " root=" << str(r.root) << " -> " << str(r.object) << "[" << r.elem << "] [" << str(r.path) << "] other-writes=" << r.n
           << " " << flags_text("KZ_PTRF_FLAG", r.flags);
        if (r.guard_root != KZ_NONE32) os << " guard=" << expr(r.guard_root);
        os << "\n";
      }
    }
  }
  std::string image_value(const kz_fact_gimage& g) const {
    std::string t = enum_text("KZ_IMAGE_KIND", g.kind);
    switch (g.kind) {
      case KZ_IMAGE_KIND_INT: t += " " + std::to_string(g.value); break;
      case KZ_IMAGE_KIND_STRING: t += " " + quoted(str(g.text)); break;
      case KZ_IMAGE_KIND_ADDR:
      case KZ_IMAGE_KIND_FUNC: t += " " + str(g.text); break;
      default: break;
    }
    t += " at " + pos(g.file, g.line, g.col);
    const std::string mac = str(g.macro);
    if (!mac.empty()) t += " macro " + mac + " spelled at " + pos(g.spell_file, g.spell_line, g.spell_col);
    return t + " " + flags_text("KZ_GIMAGE_FLAG", g.flags);
  }
  void gimages(std::ostream& os) {
    os << "[gimages] " << gimages_.size() << "\n";
    for (const kz_fact_gimage& g : gimages_) {
      os << "  " << str(g.object) << str(g.path) << " " << image_value(g) << " reach=" << flags_text("KZ_GIMAGE_REACH", g.reach);
      const std::string owner = str(g.owner);
      if (!owner.empty()) os << " field=" << owner << "." << str(g.field);
      os << "\n";
    }
  }

  std::string list(uint32_t root) const {
    std::string out = "[";
    bool first = true;
    uint32_t cur = root;
    for (int guard = 0; cur != KZ_NONE32 && guard < 10000; ++guard) {
      if (cur >= exprs_.size()) break;
      const kz_fact_expr& e = exprs_[cur];
      if (e.op != KZ_EXPR_OP_SEQ && e.op != KZ_EXPR_OP_LOCSET) {
        out += (first ? "" : ", ") + expr(cur);
        break;
      }
      out += (first ? "" : ", ") + expr(e.a);
      first = false;
      cur = e.b;
    }
    return out + "]";
  }
  void init(std::ostream& os) {
    std::vector<const kz_fact_init*> sel;
    for (const kz_fact_init& i : inits_)
      if (want_loc(i.loc_type, i.loc_field)) sel.push_back(&i);
    os << "[init] " << sel.size() << "\n";
    for (const kz_fact_init* i : sel) {
      os << "  " << str(i->loc_type) << "." << str(i->loc_field) << " in " << str(i->object) << " " << enum_text("KZ_INIT_SOURCE", i->source);
      if (i->source == KZ_INIT_SOURCE_ZERO || i->source == KZ_INIT_SOURCE_STATIC_INIT) os << " value=" << i->value;
      os << " defined at " << pos(i->file, i->line, i->col) << "\n";
    }
  }
  void consts(std::ostream& os) {
    os << "[consts] " << consts_.size() << "\n";
    for (const kz_fact_const& k : consts_) os << "  " << str(k.name) << " = " << (k.kind == KZ_CONST_KIND_STRING ? quoted(str(k.text)) : std::to_string(k.value)) << " at " << pos(k.file, k.line, 0) << "\n";
  }
  void writers(std::ostream& os) {
    std::vector<const kz_fact_writer*> sel;
    for (const kz_fact_writer& w : writers_)
      if (want_loc(w.loc_type, w.loc_field)) sel.push_back(&w);
    os << "[writers] " << sel.size() << "\n";
    for (const kz_fact_writer* w : sel) {
      os << "  " << str(w->loc_type) << "." << str(w->loc_field) << " " << enum_text("KZ_WRITER_KIND", w->kind) << " at " << pos(w->file, w->line, w->col) << " in " << str(w->func);
      if (w->kind == KZ_WRITER_KIND_OUTPTR) os << " via " << fn_name(w->via_fn) << " " << pos(w->via_file, w->via_line, 0);
      os << " value=" << enum_text("KZ_ARG_KIND", w->value_kind);
      if (w->value_kind == KZ_ARG_KIND_CONST) os << " " << w->value;
      os << " [" << str(w->value_text) << "] key=" << str(w->key);
      if (w->guard_root != KZ_NONE32) os << " guard=" << expr(w->guard_root);
      os << " " << flags_text("KZ_WRITER_FLAG", w->flags) << " " << enum_text("KZ_TSTATUS", w->tstatus) << "\n";
    }
  }
  void atype(std::ostream& os) {
    std::vector<const kz_fact_atype*> sel;
    for (const kz_fact_atype& a : atypes_)
      if (want_loc(a.loc_type, a.loc_field)) sel.push_back(&a);
    os << "[atype] " << sel.size() << "\n";
    for (const kz_fact_atype* a : sel)
      os << "  " << str(a->loc_type) << "." << str(a->loc_field) << " " << enum_text("KZ_ATYPE_KIND", a->kind) << " at " << pos(a->file, a->line, a->col) << " in " << fn_name(a->fn) << ": " << str(a->detail) << "\n";
  }
  void locs(std::ostream& os) {
    size_t n = 0;
    for (const kz_fact_loc& l : locs_)
      if (want_loc(l.loc_type, l.loc_field)) ++n;
    os << "[locs] " << n << "\n";
    for (const kz_fact_loc& l : locs_) {
      if (!want_loc(l.loc_type, l.loc_field)) continue;
      os << "  " << str(l.loc_type) << "." << str(l.loc_field) << " " << enum_text("KZ_LOC_STATUS", l.status) << " writers=" << l.n_writers << " (direct " << l.n_direct
         << ", data-fn " << l.n_data_fn << ", out-pointer " << l.n_outptr << ") atype: untyped=" << l.n_untyped << " escape=" << l.n_escape << " cast=" << l.n_cast
         << " union=" << l.n_union << " unknown-effect=" << l.n_unknown_effect << " init=" << l.n_init << " unresolved-indirect=" << l.n_unresolved_indirect << "\n";
    }
  }
  void targets(std::ostream& os) {
    std::vector<const kz_fact_target*> sel;
    for (const kz_fact_target& t : targets_) {
      auto it = call_by_site_.find(t.site);
      const std::string callee = it == call_by_site_.end() ? "" : str(calls_[it->second].callee);
      if (want_callee(callee)) sel.push_back(&t);
    }
    os << "[targets] " << sel.size() << "\n";
    for (const kz_fact_target* t : sel) {
      auto it = call_by_site_.find(t->site);
      os << "  site " << t->site;
      if (it != call_by_site_.end()) os << " " << pos(calls_[it->second].file, calls_[it->second].line, calls_[it->second].col) << " in " << fn_name(calls_[it->second].caller_fn);
      os << " " << enum_text("KZ_CALL_TARGET_KIND", t->kind) << " -> ";
      if (t->fn != KZ_NONE32) os << fn_name(t->fn);
      else os << "UNRESOLVED(" << enum_text("KZ_CALL_TARGET_REASON", t->reason) << ")";
      os << " via " << str(t->via) << " of " << t->n << "\n";
    }
  }
  void image(std::ostream& os) {
    os << "[image] " << images_.size() << "\n";
    for (const kz_fact_image& i : images_) {
      os << "  " << str(i.object) << str(i.path) << " " << enum_text("KZ_IMAGE_KIND", i.kind);
      switch (i.kind) {
        case KZ_IMAGE_KIND_INT: os << " " << i.value; break;
        case KZ_IMAGE_KIND_STRING: os << " " << quoted(str(i.text)); break;
        case KZ_IMAGE_KIND_ADDR:
        case KZ_IMAGE_KIND_FUNC: os << " " << str(i.text); break;
        default: break;
      }
      os << "\n";
    }
  }
  void terminals(std::ostream& os) {
    os << "[terminals] " << terminals_.size() << "\n";
    for (const kz_fact_terminal& t : terminals_)
      os << "  site " << t.site << " " << str(t.op) << " at " << pos(t.file, t.line, t.col) << " in " << fn_name(t.fn) << " depth=" << t.depth << " " << flags_text("KZ_TERMINAL_FLAG", t.flags)
         << " guard=" << expr(t.guard_root) << "\n";
  }
  void outcalls(std::ostream& os) {
    std::vector<const kz_fact_outcall*> sel;
    for (const kz_fact_outcall& o : outcalls_)
      if (want_loc(o.loc_type, o.loc_field) && want_callee(str(o.callee))) sel.push_back(&o);
    os << "[outcalls] " << sel.size() << "\n";
    for (const kz_fact_outcall* o : sel)
      os << "  site " << o->site << " " << pos(o->file, o->line, o->col) << " " << str(o->callee) << " arg " << o->arg_index << " = &" << str(o->loc_type) << "." << str(o->loc_field) << " "
         << enum_text("KZ_OUTPTR_SUMMARY", o->summary) << " stores=" << o->n_stores << "\n";
  }

  const kz::Container& c_;
  TextOptions o_;
  std::vector<kz_basis_item> basis_;
  std::vector<kz_file_hash> files_;
  std::vector<kz_fact_func> funcs_;
  std::vector<kz_fact_call> calls_;
  std::vector<kz_fact_arg> args_;
  std::vector<kz_fact_derives> derives_;
  std::vector<kz_fact_expr> exprs_;
  std::vector<kz_fact_loop> loops_;
  std::vector<kz_fact_init> inits_;
  std::vector<kz_fact_const> consts_;
  std::vector<kz_fact_atype> atypes_;
  std::vector<kz_fact_writer> writers_;
  std::vector<kz_fact_target> targets_;
  std::vector<kz_fact_image> images_;
  std::vector<kz_fact_loc> locs_;
  std::vector<kz_fact_terminal> terminals_;
  std::vector<kz_fact_outcall> outcalls_;
  std::vector<kz_fact_iter> iters_;
  std::vector<kz_fact_spawn_param> spawn_params_;
  std::vector<kz_fact_fwrite> fwrites_;
  std::vector<kz_fact_p0> p0_;
  std::vector<kz_fact_opderiv> opderiv_;
  std::vector<kz_fact_spawnw> spawnw_;
  std::vector<kz_fact_read> reads_;
  std::vector<kz_fact_return> returns_;
  std::vector<kz_fact_retconst> retconsts_;
  std::vector<kz_fact_loopexit> loopexits_;
  std::vector<kz_fact_argflow> argflow_;
  std::vector<kz_fact_pflow> pflow_;
  std::vector<kz_fact_ptrfield> ptrfields_;
  std::vector<kz_fact_gimage> gimages_;
  std::map<uint32_t, size_t> call_by_site_, loop_by_site_;
};

}  // namespace

const char* const* facts_text_sections(size_t* count) {
  *count = sizeof kSections / sizeof kSections[0];
  return kSections;
}

kz::Status facts_text(const kz::Container& c, const TextOptions& options, std::string* out) {
  View v(c, options);
  return v.run(out);
}

}  // namespace kzs
