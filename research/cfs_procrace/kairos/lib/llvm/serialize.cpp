// serialize.cpp -- see serialize.hpp.
#include "serialize.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

#include "p0.hpp"

namespace kzs {

namespace {

template <class T>
T zero() {
  T t;
  std::memset(&t, 0, sizeof t);
  return t;
}

// Assigns FACT_EXPRS indices to trees, post-order, on first use.
class ExprTable {
 public:
  explicit ExprTable(kz::ContainerWriter* w) : w_(w) {}
  uint32_t index_of(ExprRef e) {
    if (e == nullptr) return KZ_NONE32;
    auto it = idx_.find(e);
    if (it != idx_.end()) return it->second;
    uint32_t a = KZ_NONE32, b = KZ_NONE32;
    if (e->a != nullptr) a = index_of(e->a);
    if (e->b != nullptr) b = index_of(e->b);
    kz_fact_expr r = zero<kz_fact_expr>();
    r.op = e->op;
    r.flags = e->ref;
    r.a = a;
    r.b = b;
    r.value = e->value;
    r.name = w_->intern(e->name);
    rows_.push_back(r);
    const uint32_t i = static_cast<uint32_t>(rows_.size() - 1);
    idx_[e] = i;
    return i;
  }
  const std::vector<kz_fact_expr>& rows() const { return rows_; }

 private:
  kz::ContainerWriter* w_;
  std::map<ExprRef, uint32_t> idx_;  // lookup only; never iterated
  std::vector<kz_fact_expr> rows_;
};

}  // namespace

kz::Status hash_file(const std::string& path, kz::Digest* digest, uint64_t* size) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return kz::Status::error(kz::Err::Io, "cannot read " + path);
  kz::Sha256 h;
  char buf[65536];
  uint64_t total = 0;
  while (f) {
    f.read(buf, sizeof buf);
    const std::streamsize n = f.gcount();
    if (n > 0) {
      h.update(buf, static_cast<size_t>(n));
      total += static_cast<uint64_t>(n);
    }
  }
  *digest = h.finish();
  *size = total;
  return kz::Status::success();
}

kz::Status serialize_facts(const Model& m, const ExtractSpec& spec, const SerializeOptions& opt, kz::Bytes* out) {
  const Model::Resolved& R = m.resolved;
  kz::ContainerWriter w(KZ_ARTIFACT_FACTS);
  if (opt.has_basis) w.set_basis_hash(opt.basis_hash);
  w.set_profile_hash(opt.profile_hash);
  w.set_tool_hash(opt.tool_hash);
  if (opt.has_basis) {
    kz::Status s = w.add_parent(KZ_ARTIFACT_BASIS, kParentRoleBasis, opt.basis_hash);
    if (!s.ok()) return s;
  }

  // ---- files
  std::set<std::string> files;
  auto add = [&](const std::string& f) {
    if (!f.empty()) files.insert(f);
  };
  for (const auto& kv : m.funcs) {
    add(kv.second.pos.file);
    add(kv.second.tu);
  }
  for (const CallFact& c : m.calls) {
    add(c.pos.file);
    add(c.tu);
  }
  for (const DerivesFact& d : m.derives) add(d.capture.file);
  for (const LoopFact& l : m.loops) {
    add(l.pos.file);
    add(l.tu);
  }
  for (const InitFact& i : m.inits) {
    add(i.pos.file);
    add(i.tu);
  }
  for (const ConstFact& c : m.consts) {
    add(c.pos.file);
    add(c.tu);
  }
  for (const ATypeFact& a : m.atypes) {
    add(a.pos.file);
    add(a.tu);
  }
  for (const WriterFact& x : m.writers) {
    add(x.pos.file);
    add(x.via.file);
    add(x.tu);
  }
  for (const TerminalFact& t : m.terminals) {
    add(t.pos.file);
    add(t.tu);
  }
  for (const OutCallFact& o : m.outcalls) {
    add(o.pos.file);
    add(o.tu);
  }
  for (const ImageFact& i : m.images) add(i.pos.file);
  for (const IterFact& i : m.iters) add(i.pos.file);
  for (const WriteEvent& we : m.write_events) add(we.pos.file);
  for (const std::string& f : m.main_files) add(f);
  std::map<std::string, uint32_t> fh;
  std::vector<kz_file_hash> file_rows;
  for (const std::string& f : files) {
    if (f == "<invalid>" || f == "<builtin>") continue;
    std::string path = f;
    if (!path.empty() && path[0] != '/' && !opt.root.empty()) path = (opt.root.back() == '/' ? opt.root : opt.root + "/") + f;
    kz_file_hash r = zero<kz_file_hash>();
    kz::Digest d;
    uint64_t size = 0;
    kz::Status s = hash_file(path, &d, &size);
    if (!s.ok()) return s;
    r.path = w.intern(f);
    std::memcpy(r.sha256, d.data(), 32);
    r.size = size;
    r.prov = KZ_NONE32;
    fh[f] = static_cast<uint32_t>(file_rows.size());
    file_rows.push_back(r);
  }
  auto fh_of = [&](const std::string& f) -> uint32_t {
    auto it = fh.find(f);
    return it == fh.end() ? KZ_NONE32 : it->second;
  };
  auto fn_of = [&](const std::string& key) -> uint32_t {
    if (key.empty()) return KZ_NONE32;
    auto it = R.fn_index.find(key);
    return it == R.fn_index.end() ? KZ_NONE32 : it->second;
  };
  auto site_of_call = [&](const std::string& ck) -> uint32_t {
    auto it = R.call_site.find(ck);
    return it == R.call_site.end() ? KZ_NONE32 : it->second;
  };

  // ---- functions
  std::vector<kz_fact_func> funcs;
  for (const std::string& key : R.func_keys) {
    const FuncFact& f = m.funcs.at(key);
    kz_fact_func r = zero<kz_fact_func>();
    r.fn = static_cast<uint32_t>(funcs.size());
    r.tu = (f.flags & KZ_FUNC_FLAG_DEFINED) ? fh_of(f.tu) : KZ_NONE32;
    r.name = w.intern(f.name);
    r.file = w.intern(f.pos.file);
    r.line = f.pos.line;
    r.col = f.pos.col;
    r.end_line = f.end_line;
    r.fh = fh_of(f.pos.file);
    r.flags = f.flags;
    r.nparams = f.nparams;
    funcs.push_back(r);
  }

  // ---- expression trees are numbered in the order of the facts that use them
  ExprTable exprs(&w);

  // ---- calls and arguments
  std::vector<kz_fact_call> calls;
  std::vector<kz_fact_arg> args;
  for (size_t ci = 0; ci < m.calls.size(); ++ci) {
    const CallFact& c = m.calls[ci];
    const std::string ck = c.pos.str() + "#" + std::to_string(c.ordinal);
    kz_fact_call r = zero<kz_fact_call>();
    r.site = site_of_call(ck);
    r.tu = fh_of(c.tu);
    r.file = w.intern(c.pos.file);
    r.caller = w.intern(c.caller_name);
    r.callee = w.intern(c.callee_name);
    r.line = c.pos.line;
    r.col = c.pos.col;
    r.first_arg = static_cast<uint32_t>(args.size());
    r.nargs = static_cast<uint32_t>(c.args.size());
    r.flags = c.flags;
    r.loop_depth = c.loop_depth;
    r.caller_fn = fn_of(c.caller_key);
    r.callee_fn = fn_of(c.callee_key);
    r.fh = fh_of(c.pos.file);
    auto lit = R.loop_index.find(c.loop_key);
    r.loop = c.loop_key.empty() || lit == R.loop_index.end() ? KZ_NONE32 : static_cast<uint32_t>(lit->second);
    r.kind = c.kind;
    calls.push_back(r);
    for (size_t i = 0; i < c.args.size(); ++i) {
      const ArgFact& a = c.args[i];
      kz_fact_arg x = zero<kz_fact_arg>();
      x.site = r.site;
      x.index = static_cast<uint32_t>(i);
      x.kind = a.kind;
      x.role = a.role;
      x.value = a.value;
      x.symbol = w.intern(a.symbol);
      x.loc_type = w.intern(a.loc_type);
      x.loc_field = w.intern(a.loc_field);
      x.fn = fn_of(a.fn_key);
      args.push_back(x);
    }
  }

  // ---- derives
  std::vector<kz_fact_derives> derives;
  for (const DerivesFact& d : m.derives) {
    kz_fact_derives r = zero<kz_fact_derives>();
    r.site = site_of_call(d.call_key);
    r.arg_index = d.arg_index;
    r.loc_type = w.intern(d.loc_type);
    r.loc_field = w.intern(d.loc_field);
    r.chain_root = exprs.index_of(d.chain);
    r.file = w.intern(d.capture.file);
    r.line = d.capture.line;
    r.col = d.capture.col;
    r.fh = fh_of(d.capture.file);
    r.hops = d.hops;
    r.callee = w.intern(d.callee);
    r.status = d.status;
    r.reason = d.reason;
    derives.push_back(r);
  }

  // ---- loops
  std::vector<kz_fact_loop> loops;
  for (size_t li = 0; li < m.loops.size(); ++li) {
    const LoopFact& l = m.loops[li];
    kz_fact_loop r = zero<kz_fact_loop>();
    r.site = R.loop_site.at(l.key);
    r.line = l.pos.line;
    r.file = w.intern(l.pos.file);
    r.func = w.intern(l.func_name);
    r.delay_site = site_of_call(l.delay_call_key);
    r.exit_root = exprs.index_of(l.exit_expr);
    r.modset_root = exprs.index_of(l.modset);
    r.budget_root = exprs.index_of(l.budget);
    r.lock_site = site_of_call(l.lock_call_key);
    r.lock_kind = l.lock_kind;
    r.cert = l.cert;
    r.pred_form = l.pred_form;
    r.lflags = l.lflags;
    auto pit = l.parent_key.empty() ? R.loop_index.end() : R.loop_index.find(l.parent_key);
    r.parent = pit == R.loop_index.end() ? KZ_NONE32 : static_cast<uint32_t>(pit->second);
    r.pred_root = exprs.index_of(l.pred);
    r.cond_root = exprs.index_of(l.cond);
    r.cert_note = w.intern(l.cert_note.empty() ? l.pred_note : l.cert_note);
    r.loop = static_cast<uint32_t>(li);
    r.fn = fn_of(l.func_key);
    r.col = l.pos.col;
    r.fh = fh_of(l.pos.file);
    r.depth = l.depth;
    r.kind = l.kind;
    r.trip_status = l.trip_status;
    r.n_exits = l.n_exits;
    r.n_calls = l.n_calls;
    r.trip = l.trip;
    r.table = w.intern(l.table);
    r.trip_note = w.intern(l.trip_note);
    loops.push_back(r);
  }

  // ---- INIT
  std::vector<kz_fact_init> inits;
  for (const InitFact& i : m.inits) {
    kz_fact_init r = zero<kz_fact_init>();
    r.loc_type = w.intern(i.loc_type);
    r.loc_field = w.intern(i.loc_field);
    r.value = i.value;
    r.source = i.source;
    r.prov = KZ_NONE32;
    r.object = w.intern(i.object);
    r.file = w.intern(i.pos.file);
    r.line = i.pos.line;
    r.col = i.pos.col;
    r.fh = fh_of(i.pos.file);
    inits.push_back(r);
  }

  // ---- constants
  std::vector<kz_fact_const> consts;
  for (const ConstFact& c : m.consts) {
    kz_fact_const r = zero<kz_fact_const>();
    r.name = w.intern(c.name);
    r.file = w.intern(c.pos.file);
    r.value = c.value;
    r.line = c.pos.line;
    r.tu = fh_of(c.tu);
    r.prov = KZ_NONE32;
    r.text = w.intern(c.text);
    r.fh = fh_of(c.pos.file);
    r.kind = c.kind;
    consts.push_back(r);
  }

  // ---- A-TYPE
  std::vector<kz_fact_atype> atypes;
  for (size_t i = 0; i < m.atypes.size(); ++i) {
    const ATypeFact& a = m.atypes[i];
    kz_fact_atype r = zero<kz_fact_atype>();
    r.kind = a.kind;
    r.site = R.atype_site[i];
    r.loc_type = w.intern(a.loc_type);
    r.loc_field = w.intern(a.loc_field);
    r.file = w.intern(a.pos.file);
    r.line = a.pos.line;
    r.col = a.pos.col;
    r.fh = fh_of(a.pos.file);
    r.fn = fn_of(a.func_key);
    r.detail = w.intern(a.detail);
    atypes.push_back(r);
  }

  // ---- writers
  std::vector<kz_fact_writer> writers;
  for (size_t i = 0; i < m.writers.size(); ++i) {
    const WriterFact& x = m.writers[i];
    kz_fact_writer r = zero<kz_fact_writer>();
    r.loc_type = w.intern(x.loc_type);
    r.loc_field = w.intern(x.loc_field);
    r.file = w.intern(x.pos.file);
    r.func = w.intern(x.func_name);
    r.line = x.pos.line;
    r.col = x.pos.col;
    r.site = R.writer_site[i];
    r.kind = x.kind;
    r.value_kind = x.value_kind;
    // The transition status (SPEC 4.2 step 7) needs steps 6-7 (transition filtering against a
    // conjunct, mapping to the profile's effects): the census does them.  The extractor did
    // steps 2-3 only, so it claims no status.
    r.tstatus = KZ_TSTATUS_NA;
    r.kstatus = KZ_KSTATUS_NA;
    r.wd = 0;  // the W / W_must / D classification belongs to the census, which knows the conjunct
    r.guard_root = exprs.index_of(x.guard);
    r.key = w.intern(x.key);
    r.value = x.value;
    r.value_text = w.intern(x.value_text);
    r.via_file = w.intern(x.via.file);
    r.fh = fh_of(x.pos.file);
    r.fn = fn_of(x.func_key);
    r.via_line = x.via.line;
    r.via_fn = fn_of(x.via_func_key);
    r.flags = x.flags;
    writers.push_back(r);
  }

  // ---- call targets
  std::vector<kz_fact_target> targets;
  for (const TargetFact& t : m.targets) {
    kz_fact_target r = zero<kz_fact_target>();
    r.site = site_of_call(t.call_key);
    r.fn = fn_of(t.fn_key);
    r.kind = t.kind;
    r.reason = t.fn_key.empty() ? t.reason : static_cast<uint8_t>(KZ_CALL_TARGET_REASON_RESOLVED);
    r.n = t.n;
    r.via = w.intern(t.via);
    targets.push_back(r);
  }

  // ---- static image
  std::vector<kz_fact_image> images;
  for (const ImageFact& i : m.images) {
    kz_fact_image r = zero<kz_fact_image>();
    r.object = w.intern(i.object);
    r.path = w.intern(i.path);
    r.value = i.value;
    r.text = w.intern(i.text);
    r.fn = fn_of(i.fn_key);
    r.kind = i.kind;
    r.file = w.intern(i.pos.file);
    r.line = i.pos.line;
    r.fh = fh_of(i.pos.file);
    images.push_back(r);
  }

  // ---- location summaries
  std::vector<kz_fact_loc> locs;
  for (const LocFact& l : m.locs) {
    kz_fact_loc r = zero<kz_fact_loc>();
    r.loc_type = w.intern(l.loc_type);
    r.loc_field = w.intern(l.loc_field);
    r.n_writers = l.n_writers;
    r.n_direct = l.n_direct;
    r.n_data_fn = l.n_data_fn;
    r.n_outptr = l.n_outptr;
    r.n_untyped = l.n_untyped;
    r.n_escape = l.n_escape;
    r.n_cast = l.n_cast;
    r.n_union = l.n_union;
    r.n_unknown_effect = l.n_unknown;
    r.n_init = l.n_init;
    r.n_unresolved_indirect = l.n_unresolved_indirect;
    r.status = l.status;
    locs.push_back(r);
  }

  // ---- terminals and out-pointer calls
  std::vector<kz_fact_terminal> terminals;
  for (const TerminalFact& t : m.terminals) {
    kz_fact_terminal r = zero<kz_fact_terminal>();
    r.site = site_of_call(t.call_key);
    r.fn = fn_of(t.func_key);
    r.guard_root = exprs.index_of(t.guard);
    r.depth = t.depth;
    r.op = w.intern(t.op);
    r.file = w.intern(t.pos.file);
    r.line = t.pos.line;
    r.col = t.pos.col;
    r.fh = fh_of(t.pos.file);
    r.flags = t.flags;
    terminals.push_back(r);
  }
  std::vector<kz_fact_outcall> outcalls;
  for (const OutCallFact& o : m.outcalls) {
    kz_fact_outcall r = zero<kz_fact_outcall>();
    r.site = site_of_call(o.call_key);
    r.arg_index = o.arg_index;
    r.callee_fn = fn_of(o.callee_key);
    r.n_stores = o.n_stores;
    r.loc_type = w.intern(o.loc_type);
    r.loc_field = w.intern(o.loc_field);
    r.callee = w.intern(o.callee_name);
    r.file = w.intern(o.pos.file);
    r.line = o.pos.line;
    r.col = o.pos.col;
    r.fh = fh_of(o.pos.file);
    r.summary = o.summary;
    outcalls.push_back(r);
  }

  // ---- unrolled iterations of table walks
  std::vector<kz_fact_iter> iters;
  {
    struct Row {
      uint32_t loop, iter, site_order, kind, arg;
      const IterFact* f;
    };
    std::vector<Row> rows;
    for (const IterFact& f : m.iters) {
      auto lit = R.loop_index.find(f.loop_key);
      if (lit == R.loop_index.end()) continue;
      const uint32_t site = f.call_key.empty() ? KZ_NONE32 : site_of_call(f.call_key);
      // the element first, then per call: the call row, its callee, its arguments
      rows.push_back(Row{static_cast<uint32_t>(lit->second), f.iter, f.kind == KZ_ITER_KIND_ELEMENT ? 0u : site + 1u,
                         f.kind == KZ_ITER_KIND_CALL ? 0u : f.kind == KZ_ITER_KIND_CALLEE ? 1u : 2u, f.arg_index, &f});
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) {
      return std::tie(a.loop, a.iter, a.site_order, a.kind, a.arg) < std::tie(b.loop, b.iter, b.site_order, b.kind, b.arg);
    });
    for (const Row& row : rows) {
      const IterFact& f = *row.f;
      kz_fact_iter r = zero<kz_fact_iter>();
      r.loop = row.loop;
      r.iter = f.iter;
      r.site = f.call_key.empty() ? KZ_NONE32 : site_of_call(f.call_key);
      r.arg_index = f.kind == KZ_ITER_KIND_ARG ? f.arg_index : KZ_NONE32;
      r.kind = f.kind;
      r.status = f.status;
      r.value_kind = f.value_kind;
      r.spawn = f.spawn;
      r.guard_root = exprs.index_of(f.guard);
      r.fn = fn_of(f.fn_key);
      r.value = f.value;
      r.text = w.intern(f.text);
      r.file = w.intern(f.pos.file);
      r.line = f.pos.line;
      r.fh = fh_of(f.pos.file);
      iters.push_back(r);
    }
  }

  // ---- spawn parameters
  std::vector<kz_fact_spawn_param> spawn_params;
  for (const SpawnParamFact& sp : m.spawn_params) {
    kz_fact_spawn_param r = zero<kz_fact_spawn_param>();
    r.fn = fn_of(sp.func_key);
    r.param = sp.param;
    r.spawn_site = site_of_call(sp.spawn_call_key);
    r.entry_fn = fn_of(sp.entry_key);
    r.call_site = sp.call_key.empty() ? KZ_NONE32 : site_of_call(sp.call_key);
    r.kind = sp.kind;
    r.via = w.intern(sp.via);
    spawn_params.push_back(r);
  }
  std::sort(spawn_params.begin(), spawn_params.end(), [](const kz_fact_spawn_param& a, const kz_fact_spawn_param& b) {
    return std::tie(a.fn, a.param, a.spawn_site, a.kind, a.entry_fn, a.call_site) < std::tie(b.fn, b.param, b.spawn_site, b.kind, b.entry_fn, b.call_site);
  });

  // ---- the fields each function writes in its own body (first write of each)
  std::vector<kz_fact_fwrite> fwrites;
  {
    std::map<std::tuple<uint32_t, std::string, std::string, uint8_t>, const WriteEvent*> first;
    for (const WriteEvent& e : m.write_events) {
      const auto key = std::make_tuple(fn_of(e.func_key), e.type, e.field, e.kind);
      auto it = first.find(key);
      if (it == first.end() || e.pos < it->second->pos) first[key] = &e;
    }
    for (const auto& kv : first) {
      const WriteEvent& e = *kv.second;
      kz_fact_fwrite r = zero<kz_fact_fwrite>();
      r.fn = std::get<0>(kv.first);
      r.kind = e.kind;
      r.type = w.intern(e.type);
      r.field = w.intern(e.field);
      r.file = w.intern(e.pos.file);
      r.line = e.pos.line;
      r.fh = fh_of(e.pos.file);
      fwrites.push_back(r);
    }
  }

  // ---- the profile-free mode (kz_facts.h revision 5): P0_TABLE, OP_DERIV, SPAWN_WRAP.  Written only
  // in that mode, so that a profile-driven extraction writes its facts as before.
  std::vector<kz_fact_p0> p0_rows;
  std::vector<kz_fact_opderiv> op_rows;
  std::vector<kz_fact_spawnw> spawn_rows;
  if (spec.no_profile) {
    auto param = [](int p) { return p >= 0 ? static_cast<uint32_t>(p) : KZ_NONE32; };
    for (const P0Fact& f : m.p0_facts) {
      if (f.entry >= p0_table().size()) continue;
      const P0Entry& e = p0_table()[f.entry];
      kz_fact_p0 r = zero<kz_fact_p0>();
      r.fn = fn_of(f.fn_key);
      r.out_param = param(e.out);
      r.entry_param = param(e.entry);
      r.arg_param = param(e.arg);
      r.n_calls = f.n_calls;
      r.kind = e.kind;
      r.flags = f.flags;
      r.name = w.intern(e.name);
      r.family = w.intern(e.family);
      r.semantics = w.intern(e.semantics);
      r.version = w.intern(e.version);
      r.source = w.intern(e.source);
      p0_rows.push_back(r);
    }
    for (const OpDerivFact& f : m.op_derivs) {
      kz_fact_opderiv r = zero<kz_fact_opderiv>();
      r.fn = fn_of(f.func_key);
      r.site = f.call_key.empty() ? KZ_NONE32 : site_of_call(f.call_key);
      r.next_fn = fn_of(f.next_key);
      r.guard_root = exprs.index_of(f.guard);
      r.depth = f.depth;
      r.kind = f.kind;
      r.mode = f.mode;
      r.flags = f.flags;
      r.primitive = w.intern(f.primitive);
      r.chain = w.intern(f.chain);
      op_rows.push_back(r);
    }
    std::sort(op_rows.begin(), op_rows.end(), [](const kz_fact_opderiv& a, const kz_fact_opderiv& b) {
      return std::tie(a.fn, a.kind, a.site, a.next_fn) < std::tie(b.fn, b.kind, b.site, b.next_fn);
    });
    for (const SpawnWrapFact& f : m.spawn_wraps) {
      kz_fact_spawnw r = zero<kz_fact_spawnw>();
      r.fn = fn_of(f.func_key);
      r.param = f.param == kNone ? KZ_NONE32 : f.param;
      r.site = f.call_key.empty() ? KZ_NONE32 : site_of_call(f.call_key);
      r.next_fn = fn_of(f.next_key);
      r.entry_fn = fn_of(f.entry_key);
      r.call_site = f.icall_key.empty() ? KZ_NONE32 : site_of_call(f.icall_key);
      r.depth = f.depth;
      r.kind = f.kind;
      r.flags = f.flags;
      r.primitive = w.intern(f.primitive);
      r.chain = w.intern(f.chain);
      r.via = w.intern(f.via);
      spawn_rows.push_back(r);
    }
    std::stable_sort(spawn_rows.begin(), spawn_rows.end(), [](const kz_fact_spawnw& a, const kz_fact_spawnw& b) {
      return std::tie(a.kind, a.fn, a.site, a.param, a.entry_fn) < std::tie(b.kind, b.fn, b.site, b.param, b.entry_fn);
    });
  }

  // ---- BASIS and PROVENANCE: the extraction parameters, the counts, the hand-entered inputs
  std::vector<kz_prov> provs;
  std::vector<kz_basis_item> basis;
  auto item = [&](const std::string& name, const std::string& value, uint8_t policy, uint32_t prov) {
    kz_basis_item b = zero<kz_basis_item>();
    b.name = w.intern(name);
    b.value = w.intern(value);
    b.policy = policy;
    b.prov = prov;
    b.deviation = KZ_NONE32;
    basis.push_back(b);
  };
  std::map<std::string, uint32_t> prov_of_origin;
  auto prov_for = [&](const std::string& origin) -> uint32_t {
    auto it = prov_of_origin.find(origin);
    if (it != prov_of_origin.end()) return it->second;
    kz_prov p = zero<kz_prov>();
    p.tier = KZ_TIER_RESEARCHER;
    p.kind = KZ_PROV_KIND_RESEARCHER;
    p.side = KZ_PROV_SIDE_NA;
    p.file = w.intern("");
    p.cite = w.intern(origin);
    p.authored = w.intern("entered by a person: " + origin);
    provs.push_back(p);
    return prov_of_origin[origin] = static_cast<uint32_t>(provs.size() - 1);
  };
  item("extract.build_identity", opt.build_identity, KZ_POLICY_UNSET, KZ_NONE32);
  item("extract.profile", spec.profile_name + " / " + spec.profile_revision, KZ_POLICY_UNSET, KZ_NONE32);
  for (const TargetField& t : spec.fields) {
    const bool from_profile = t.origin.compare(0, 7, "profile") == 0;
    item("target." + t.field.type + "." + t.field.field, t.origin, from_profile ? KZ_POLICY_FROM_REFERENCE : KZ_POLICY_DECLARED, from_profile ? KZ_NONE32 : prov_for(t.origin));
  }
  for (const DeclaredItem& d : spec.declared) item(d.name, d.value, KZ_POLICY_DECLARED, prov_for(d.origin));
  // the assumptions the extraction itself declared (counted in the report)
  for (const Model::Assumption& a : m.assumptions)
    item(a.name, a.value, KZ_POLICY_DECLARED, prov_for("kz-static extract: declared assumption of the table-walk unrolling (SPEC 4.3), not checked"));
  auto num = [](uint64_t v) { return std::to_string(v); };
  item("count.translation_units", num(m.main_files.size()), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.functions", num(R.func_keys.size()), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.functions_read", num(m.stats.functions_read), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.functions_skipped_system_header", num(m.stats.functions_skipped_system), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.calls", num(m.calls.size()), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.indirect_calls", num(R.indirect_total), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.indirect_calls_unresolved", num(R.indirect_unresolved), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.loops", num(m.loops.size()), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.loops_unrolled", num(R.loops_unroll), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.loops_await", num(R.loops_await), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.loops_star_closed", num(R.loops_star), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.sites", num(R.n_sites), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.untyped_write_sites_all", num(R.atype_untyped_sites_total), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.untyped_write_sites_counted", num(R.atype_untyped_sites_relevant), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.unknown_effect_pointer_passes", num(R.reach_passes_unknown), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.declared_assumptions", num(m.assumptions.size()), KZ_POLICY_UNSET, KZ_NONE32);
  item("count.key_collisions", num(R.key_collisions), KZ_POLICY_UNSET, KZ_NONE32);
  for (const auto& n : opt.notes) item(n.first, n.second, KZ_POLICY_UNSET, KZ_NONE32);

  // ---- add the sections (the writer sorts them canonically)
  kz::Status s;
  auto put = [&](uint32_t section, auto& rows) -> bool {
    s = w.add_section(section, rows);
    return s.ok();
  };
  if (!put(KZ_SEC_BASIS, basis) || !put(KZ_SEC_FILE_HASHES, file_rows) || !put(KZ_SEC_PROVENANCE, provs)) return s;
  if (!put(KZ_SEC_FUNCS, funcs) || !put(KZ_SEC_FACTS, calls) || !put(KZ_SEC_FACT_ARGS, args) || !put(KZ_SEC_DERIVES, derives)) return s;
  if (!put(KZ_SEC_FACT_LOOPS, loops) || !put(KZ_SEC_FACT_INIT, inits) || !put(KZ_SEC_FACT_CONSTS, consts)) return s;
  if (!put(KZ_SEC_FACT_ATYPE, atypes) || !put(KZ_SEC_WRITERS, writers) || !put(KZ_SEC_CALL_TARGETS, targets)) return s;
  if (!put(KZ_SEC_STATIC_IMAGE, images) || !put(KZ_SEC_FACT_LOCS, locs) || !put(KZ_SEC_TERMINALS, terminals)) return s;
  if (!put(KZ_SEC_OUTCALLS, outcalls) || !put(KZ_SEC_LOOP_ITERS, iters) || !put(KZ_SEC_SPAWN_PARAMS, spawn_params) || !put(KZ_SEC_FUNC_WRITES, fwrites)) return s;
  if (spec.no_profile && (!put(KZ_SEC_P0_TABLE, p0_rows) || !put(KZ_SEC_OP_DERIV, op_rows) || !put(KZ_SEC_SPAWN_WRAP, spawn_rows))) return s;
  std::vector<kz_fact_expr> expr_rows = exprs.rows();
  if (!put(KZ_SEC_FACT_EXPRS, expr_rows)) return s;
  return w.finish(out);
}

}  // namespace kzs
