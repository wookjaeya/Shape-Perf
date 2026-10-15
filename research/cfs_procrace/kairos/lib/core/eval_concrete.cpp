// eval_concrete.cpp -- the concrete domain: exact values over a concrete state.

#include "kz/core/eval.hpp"

namespace kz {
namespace eval {

using phi::CmpOp;
using phi::Sort;
using phi::SortKind;

int64_t id_value(int64_t v) { return static_cast<int64_t>(static_cast<uint64_t>(v) & 0xFFFFFFFFull); }

int64_t str_handle(const std::string& s) {
  uint64_t h = 1469598103934665603ull;
  for (unsigned char c : s) {
    h ^= c;
    h *= 1099511628211ull;
  }
  return static_cast<int64_t>(h);
}

size_t key_width(const phi::Profile& p, int32_t kind) {
  // The checker rejects cyclic Res<...> parameters; the depth bound only keeps a profile that
  // was not checked from recursing without end.
  struct Rec {
    static size_t width(const phi::Profile& pr, int32_t k, int depth) {
      if (k < 0 || static_cast<size_t>(k) >= pr.kinds.size() || depth > 32) return 1;
      size_t w = 0;
      for (const phi::KindParam& kp : pr.kinds[static_cast<size_t>(k)].params)
        w += kp.sort.resolved.kind == SortKind::Res ? width(pr, kp.sort.resolved.kind_index, depth + 1) : 1;
      return w;
    }
  };
  return Rec::width(p, kind, 0);
}

namespace {

CVal make(SortKind s, int64_t v, int32_t kind = -1) {
  CVal c;
  c.sort = s;
  c.v = s == SortKind::Id ? id_value(v) : v;  // an Id is a 32-bit value wherever it enters
  c.kind = kind;
  return c;
}

CVal boolean(bool b) { return make(SortKind::Bool, b ? 1 : 0); }

}  // namespace

ConcreteState initial_state(const phi::Profile& p) {
  ConcreteState s;
  for (const phi::InitDecl& d : p.inits) {
    if (!d.value && !d.res) continue;
    ConcreteDomain dom(p, s);
    Evaluator<ConcreteDomain> ev(p, dom);
    if (d.is_loc) {
      const int32_t li = p.find_loc(d.loc_name);
      if (li >= 0 && d.value) s.loc_values[li] = ev.eval(*d.value).v;
    } else if (d.res) {
      const CVal r = ev.eval(*d.res);
      if (r.defined) {
        const phi::KindDecl& k = p.kinds[static_cast<size_t>(r.kind)];
        for (size_t i = 0; i < k.states.size(); ++i)
          if (k.states[i] == d.state) s.state[ResId{r.kind, r.key}] = static_cast<int32_t>(i);
      }
    }
  }
  return s;
}

CVal ConcreteDomain::lit_bool(bool b) const { return boolean(b); }
CVal ConcreteDomain::lit_int(int64_t v) const { return make(SortKind::Int, v); }
CVal ConcreteDomain::lit_str(const std::string& s) const { return make(SortKind::Str, str_handle(s)); }
CVal ConcreteDomain::lnot(const CVal& a) const { return boolean(a.v == 0); }
CVal ConcreteDomain::land(const CVal& a, const CVal& b) const { return boolean(a.v != 0 && b.v != 0); }
CVal ConcreteDomain::lor(const CVal& a, const CVal& b) const { return boolean(a.v != 0 || b.v != 0); }

CVal ConcreteDomain::cmp(CmpOp op, const CVal& a, const CVal& b) const {
  if (a.sort == SortKind::Res || b.sort == SortKind::Res) {
    const bool eq = (!a.defined || !b.defined) ? (!a.defined && !b.defined) : (a.kind == b.kind && a.key == b.key);
    return boolean(op == CmpOp::Eq ? eq : op == CmpOp::Ne ? !eq : false);
  }
  const int64_t x = a.v, y = b.v;
  switch (op) {
    case CmpOp::Eq: return boolean(x == y);
    case CmpOp::Ne: return boolean(x != y);
    case CmpOp::Lt: return boolean(x < y);
    case CmpOp::Le: return boolean(x <= y);
    case CmpOp::Gt: return boolean(x > y);
    case CmpOp::Ge: return boolean(x >= y);
  }
  return boolean(false);
}

CVal ConcreteDomain::arg(int32_t index, const Sort& sort) const {
  const int64_t v = index >= 0 && static_cast<size_t>(index) < s_.args.size() ? s_.args[static_cast<size_t>(index)] : 0;
  CVal c = make(sort.kind == SortKind::Unknown ? SortKind::Int : sort.kind, v, sort.kind == SortKind::Id ? sort.kind_index : -1);
  return c;
}

CVal ConcreteDomain::exec_actor() const { return make(SortKind::Actor, s_.exec); }
CVal ConcreteDomain::result_code() const { return make(SortKind::Int, s_.result); }

CVal ConcreteDomain::constant(int32_t index) const {
  const phi::ConstDecl& c = p_.consts[static_cast<size_t>(index)];
  switch (c.value) {
    case phi::ConstDecl::Value::Int: return make(SortKind::Int, c.ival);
    case phi::ConstDecl::Value::Str: return make(SortKind::Str, str_handle(c.sval));
    case phi::ConstDecl::Value::Bool: return boolean(c.bval);
    case phi::ConstDecl::Value::RefEval: {
      const auto it = s_.consts.find(index);
      return make(SortKind::Int, it == s_.consts.end() ? 0 : it->second);
    }
  }
  return make(SortKind::Int, 0);
}

CVal ConcreteDomain::state_constant(int32_t kind, int32_t ordinal) const { return make(SortKind::State, ordinal, kind); }

CVal ConcreteDomain::tag_constant(int32_t kind, int32_t tag, int32_t ordinal) const {
  CVal c = make(SortKind::Tag, ordinal, kind);
  c.tag = tag;
  return c;
}

CVal ConcreteDomain::loc_ref(int32_t index) const { return make(SortKind::Loc, index); }

CVal ConcreteDomain::make_res(int32_t kind, const std::vector<CVal>& keys) const {
  CVal r = make(SortKind::Res, 0, kind);
  for (const CVal& k : keys) {
    if (k.sort == SortKind::Res) {
      r.defined = r.defined && k.defined;
      r.key.insert(r.key.end(), k.key.begin(), k.key.end());
    } else {
      r.key.push_back(k.v);
    }
  }
  if (!r.defined) r.key.clear();
  return r;
}

CVal ConcreteDomain::state_of(const CVal& res) const {
  const phi::KindDecl& k = p_.kinds[static_cast<size_t>(res.kind)];
  int32_t st = k.initial_ordinal;
  if (res.defined) {
    const auto it = s_.state.find(ResId{res.kind, res.key});
    if (it != s_.state.end()) st = it->second;
  }
  return make(SortKind::State, st, res.kind);
}

CVal ConcreteDomain::key_of(const CVal& res, size_t index, const Sort& sort) const {
  if (sort.kind == SortKind::Res) {
    CVal r = make(SortKind::Res, 0, sort.kind_index);
    const size_t w = key_width(p_, sort.kind_index);
    if (!res.defined || index + w > res.key.size()) {
      r.defined = false;
      return r;
    }
    r.key.assign(res.key.begin() + static_cast<std::ptrdiff_t>(index), res.key.begin() + static_cast<std::ptrdiff_t>(index + w));
    return r;
  }
  const int64_t v = res.defined && index < res.key.size() ? res.key[index] : 0;
  return make(sort.kind == SortKind::Unknown ? SortKind::Int : sort.kind, v, sort.kind == SortKind::Id ? sort.kind_index : -1);
}

CVal ConcreteDomain::gen_of(const CVal&) const { return make(SortKind::Int, 0); }

CVal ConcreteDomain::valid(const CVal& id) const {
  if (id.sort != SortKind::Id) return boolean(false);  // a value that is not an identifier is never valid
  if (id.kind < 0 || static_cast<size_t>(id.kind) >= p_.kinds.size()) return boolean(false);
  const phi::IdSpace& ids = p_.kinds[static_cast<size_t>(id.kind)].ids;
  if (!ids.present) return boolean(false);
  const uint32_t serial = static_cast<uint32_t>(id.v) - static_cast<uint32_t>(ids.base);
  return boolean(serial <= static_cast<uint32_t>(ids.serial_max));
}

CVal ConcreteDomain::value_of(const CVal& loc) const {
  // The value of a declared location; an index that names no declared location (the index is
  // compared as it is, never truncated) holds 0 and is not an identifier.
  if (loc.v < 0 || loc.v >= static_cast<int64_t>(p_.locs.size())) return make(SortKind::Int, 0);
  const int32_t li = static_cast<int32_t>(loc.v);
  int64_t v = 0;
  const auto it = s_.loc_values.find(li);
  if (it != s_.loc_values.end()) v = it->second;
  const Sort& h = p_.locs[static_cast<size_t>(li)].holds.resolved;
  return make(h.kind == SortKind::Id ? SortKind::Id : SortKind::Int, v, h.kind == SortKind::Id ? h.kind_index : -1);
}

CVal ConcreteDomain::tag_of(const CVal& res, int32_t tag) const {
  int64_t v = 0;
  if (res.defined) {
    const auto it = s_.tags.find({ResId{res.kind, res.key}, tag});
    if (it != s_.tags.end()) v = it->second;
  }
  CVal c = make(SortKind::Tag, v, res.kind);
  c.tag = tag;
  return c;
}

CVal ConcreteDomain::id_of(const CVal& res) const {
  const phi::KindDecl& k = p_.kinds[static_cast<size_t>(res.kind)];
  int64_t v = k.ids.present ? k.ids.undefined : 0;
  if (res.defined) {
    const auto it = s_.ids.find(ResId{res.kind, res.key});
    if (it != s_.ids.end()) v = it->second;
  }
  return make(SortKind::Id, v, res.kind);
}

CVal ConcreteDomain::owner_of(const CVal& actor) const {
  CVal r = make(SortKind::Res, 0, p_.context.owner_kind_index);
  if (s_.has_owner && actor.v == s_.exec) {
    r.kind = s_.owner.kind;
    r.key = s_.owner.key;
  } else {
    r.defined = false;
  }
  return r;
}

std::vector<CVal> ConcreteDomain::instances_of(int32_t kind) const {
  std::vector<CVal> out;
  const auto it = s_.instances.find(kind);
  if (it == s_.instances.end()) return out;
  for (const std::vector<int64_t>& key : it->second) {
    CVal r = make(SortKind::Res, 0, kind);
    r.key = key;
    out.push_back(std::move(r));
  }
  return out;
}

CVal ConcreteDomain::select_state(int32_t kind, const std::vector<CVal>& conds, const std::vector<int32_t>& ordinals, int32_t fallback) const {
  for (size_t i = 0; i < conds.size(); ++i)
    if (conds[i].v != 0) return make(SortKind::State, ordinals[i], kind);
  return make(SortKind::State, fallback, kind);
}

}  // namespace eval
}  // namespace kz
