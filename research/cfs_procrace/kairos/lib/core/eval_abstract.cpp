// eval_abstract.cpp -- the abstract domain: intervals, sets of states, three-valued Booleans.
//
// Every operation over-approximates the concrete operation of eval_concrete.cpp; the property
// test (tests/property/test_eval_agreement.cpp) checks this on generated expressions and states.

#include <algorithm>
#include <limits>

#include "kz/core/eval.hpp"

namespace kz {
namespace eval {

using phi::CmpOp;
using phi::Sort;
using phi::SortKind;

namespace {

constexpr int64_t kMin = std::numeric_limits<int64_t>::min();
constexpr int64_t kMax = std::numeric_limits<int64_t>::max();
constexpr int64_t kId32Max = 0xFFFFFFFFll;

AVal tri_val(Tri t) {
  AVal a;
  a.sort = SortKind::Bool;
  a.tri = t;
  return a;
}

AVal range(SortKind s, int64_t lo, int64_t hi, int32_t kind = -1) {
  AVal a;
  a.sort = s;
  a.lo = lo;
  a.hi = hi;
  a.kind = kind;
  return a;
}

AVal mask_val(SortKind s, uint64_t mask, int32_t kind, int32_t tag = -1) {
  AVal a;
  a.sort = s;
  a.mask = mask;
  a.kind = kind;
  a.tag = tag;
  return a;
}

uint64_t bit(int32_t ordinal) { return ordinal >= 0 && ordinal < 64 ? (1ull << ordinal) : 0ull; }

int lowest(uint64_t m) {
  for (int i = 0; i < 64; ++i)
    if (m & (1ull << i)) return i;
  return 0;
}
int highest(uint64_t m) {
  for (int i = 63; i >= 0; --i)
    if (m & (1ull << i)) return i;
  return 0;
}

bool single(uint64_t m) { return m != 0 && (m & (m - 1)) == 0; }

Tri tri_of(bool b) { return b ? Tri::True : Tri::False; }

Tri tri_not(Tri t) { return t == Tri::Maybe ? Tri::Maybe : (t == Tri::True ? Tri::False : Tri::True); }

std::pair<int64_t, int64_t> hull(std::pair<int64_t, int64_t> a, std::pair<int64_t, int64_t> b) {
  return {std::min(a.first, b.first), std::max(a.second, b.second)};
}

// The interval of sort Id that stands for every residue modulo 2^32 of the values in [lo, hi]
// (the concrete domain reads an Id modulo 2^32, eval.hpp).  A span of 2^32 values or more, an
// interval whose residues wrap around, and an empty (malformed) interval give the whole space.
std::pair<int64_t, int64_t> id_interval(int64_t lo, int64_t hi) {
  if (lo > hi) return {0, kId32Max};
  if (lo >= 0 && hi <= kId32Max) return {lo, hi};
  const uint64_t span = static_cast<uint64_t>(hi) - static_cast<uint64_t>(lo);  // no overflow: hi >= lo
  if (span >= static_cast<uint64_t>(kId32Max)) return {0, kId32Max};
  const int64_t a = id_value(lo), b = id_value(hi);
  return a <= b ? std::pair<int64_t, int64_t>{a, b} : std::pair<int64_t, int64_t>{0, kId32Max};
}
std::pair<int64_t, int64_t> id_interval(std::pair<int64_t, int64_t> iv) { return id_interval(iv.first, iv.second); }

bool within_box(const std::vector<int64_t>& key, const std::vector<std::pair<int64_t, int64_t>>& box) {
  if (key.size() != box.size()) return false;
  for (size_t i = 0; i < key.size(); ++i)
    if (key[i] < box[i].first || key[i] > box[i].second) return false;
  return true;
}

// Every ordinal of a list of n names (n <= 64).
uint64_t all_of(size_t n) { return n >= 64 ? ~0ull : ((1ull << n) - 1); }

bool all_singletons(const std::vector<std::pair<int64_t, int64_t>>& box, std::vector<int64_t>* key) {
  key->clear();
  for (const auto& iv : box) {
    if (iv.first != iv.second) return false;
    key->push_back(iv.first);
  }
  return true;
}

}  // namespace

AVal exact(const CVal& c) {
  switch (c.sort) {
    case SortKind::Bool: return tri_val(tri_of(c.v != 0));
    case SortKind::State:
    case SortKind::Tag: return mask_val(c.sort, bit(static_cast<int32_t>(c.v)), c.kind, c.tag);
    case SortKind::Res: {
      AVal a;
      a.sort = SortKind::Res;
      a.kind = c.kind;
      a.may_be_undefined = !c.defined;
      a.must_be_undefined = !c.defined;
      for (int64_t k : c.key) a.key.push_back({k, k});
      return a;
    }
    default: return range(c.sort, c.v, c.v, c.kind);
  }
}

bool contains(const AVal& a, const CVal& c) {
  switch (c.sort) {
    case SortKind::Bool: return a.sort == SortKind::Bool && (a.tri == Tri::Maybe || a.tri == tri_of(c.v != 0));
    case SortKind::State:
    case SortKind::Tag: return a.sort == c.sort && c.v >= 0 && c.v < 64 && (a.mask & bit(static_cast<int32_t>(c.v))) != 0;
    case SortKind::Res: {
      if (a.sort != SortKind::Res) return false;
      if (!c.defined) return a.may_be_undefined;
      if (a.must_be_undefined) return false;
      if (a.kind != c.kind || a.key.size() != c.key.size()) return false;
      return within_box(c.key, a.key);
    }
    default: return (a.sort == c.sort || (a.sort == SortKind::Int && c.sort == SortKind::Id) || (a.sort == SortKind::Id && c.sort == SortKind::Int)) &&
                    c.v >= a.lo && c.v <= a.hi;
  }
}

AVal AbstractDomain::lit_bool(bool b) const { return tri_val(tri_of(b)); }
AVal AbstractDomain::lit_int(int64_t v) const { return range(SortKind::Int, v, v); }
AVal AbstractDomain::lit_str(const std::string& s) const {
  const int64_t h = str_handle(s);
  return range(SortKind::Str, h, h);
}

AVal AbstractDomain::lnot(const AVal& a) const { return tri_val(tri_not(a.tri)); }

AVal AbstractDomain::land(const AVal& a, const AVal& b) const {
  if (a.tri == Tri::False || b.tri == Tri::False) return tri_val(Tri::False);
  if (a.tri == Tri::True && b.tri == Tri::True) return tri_val(Tri::True);
  return tri_val(Tri::Maybe);
}

AVal AbstractDomain::lor(const AVal& a, const AVal& b) const {
  if (a.tri == Tri::True || b.tri == Tri::True) return tri_val(Tri::True);
  if (a.tri == Tri::False && b.tri == Tri::False) return tri_val(Tri::False);
  return tri_val(Tri::Maybe);
}

AVal AbstractDomain::cmp(CmpOp op, const AVal& a, const AVal& b) const {
  if (a.sort == SortKind::Res || b.sort == SortKind::Res) {
    if (op != CmpOp::Eq && op != CmpOp::Ne) return tri_val(Tri::False);  // concrete: false for every ordering of resources
    Tri eq = Tri::Maybe;
    if (a.must_be_undefined && b.must_be_undefined) {
      eq = Tri::True;
    } else if ((a.must_be_undefined && !b.may_be_undefined) || (b.must_be_undefined && !a.may_be_undefined)) {
      eq = Tri::False;
    } else if (!a.may_be_undefined && !b.may_be_undefined) {
      if (a.kind != b.kind || a.key.size() != b.key.size()) {
        eq = Tri::False;
      } else {
        bool disjoint = false, equal = true;
        for (size_t i = 0; i < a.key.size(); ++i) {
          if (a.key[i].second < b.key[i].first || b.key[i].second < a.key[i].first) disjoint = true;
          if (!(a.key[i].first == a.key[i].second && b.key[i].first == b.key[i].second && a.key[i].first == b.key[i].first)) equal = false;
        }
        eq = disjoint ? Tri::False : (equal ? Tri::True : Tri::Maybe);
      }
    }
    return tri_val(op == CmpOp::Eq ? eq : tri_not(eq));
  }
  if (a.sort == SortKind::Bool || b.sort == SortKind::Bool) {
    if (op != CmpOp::Eq && op != CmpOp::Ne) {
      // concrete: the ordering of the 0/1 values; keep it simple and sound
      return tri_val(Tri::Maybe);
    }
    Tri eq = (a.tri == Tri::Maybe || b.tri == Tri::Maybe) ? Tri::Maybe : tri_of(a.tri == b.tri);
    return tri_val(op == CmpOp::Eq ? eq : tri_not(eq));
  }
  if (a.sort == SortKind::State || a.sort == SortKind::Tag || b.sort == SortKind::State || b.sort == SortKind::Tag) {
    if (a.mask == 0 || b.mask == 0) return tri_val(Tri::Maybe);
    const int amin = lowest(a.mask), amax = highest(a.mask), bmin = lowest(b.mask), bmax = highest(b.mask);
    Tri eq = (a.mask & b.mask) == 0 ? Tri::False : (single(a.mask) && single(b.mask) ? Tri::True : Tri::Maybe);
    switch (op) {
      case CmpOp::Eq: return tri_val(eq);
      case CmpOp::Ne: return tri_val(tri_not(eq));
      case CmpOp::Lt: return tri_val(amax < bmin ? Tri::True : (amin >= bmax ? Tri::False : Tri::Maybe));
      case CmpOp::Le: return tri_val(amax <= bmin ? Tri::True : (amin > bmax ? Tri::False : Tri::Maybe));
      case CmpOp::Gt: return tri_val(amin > bmax ? Tri::True : (amax <= bmin ? Tri::False : Tri::Maybe));
      case CmpOp::Ge: return tri_val(amin >= bmax ? Tri::True : (amax < bmin ? Tri::False : Tri::Maybe));
    }
  }
  const bool disjoint = a.hi < b.lo || b.hi < a.lo;
  const bool equal = a.lo == a.hi && b.lo == b.hi && a.lo == b.lo;
  switch (op) {
    case CmpOp::Eq: return tri_val(disjoint ? Tri::False : (equal ? Tri::True : Tri::Maybe));
    case CmpOp::Ne: return tri_val(disjoint ? Tri::True : (equal ? Tri::False : Tri::Maybe));
    case CmpOp::Lt: return tri_val(a.hi < b.lo ? Tri::True : (a.lo >= b.hi ? Tri::False : Tri::Maybe));
    case CmpOp::Le: return tri_val(a.hi <= b.lo ? Tri::True : (a.lo > b.hi ? Tri::False : Tri::Maybe));
    case CmpOp::Gt: return tri_val(a.lo > b.hi ? Tri::True : (a.hi <= b.lo ? Tri::False : Tri::Maybe));
    case CmpOp::Ge: return tri_val(a.lo >= b.hi ? Tri::True : (a.hi < b.lo ? Tri::False : Tri::Maybe));
  }
  return tri_val(Tri::Maybe);
}

AVal AbstractDomain::arg(int32_t index, const Sort& sort) const {
  const SortKind k = sort.kind == SortKind::Unknown ? SortKind::Int : sort.kind;
  const int32_t kind = sort.kind == SortKind::Id ? sort.kind_index : -1;
  if (index >= 0 && static_cast<size_t>(index) < s_.args.size()) {
    const std::pair<int64_t, int64_t> iv = s_.args[static_cast<size_t>(index)];
    if (k == SortKind::Id) {
      const std::pair<int64_t, int64_t> id = id_interval(iv);
      return range(k, id.first, id.second, kind);
    }
    return range(k, iv.first, iv.second, kind);
  }
  // An argument the state does not describe is unknown: any value of its sort.
  return range(k, k == SortKind::Id ? 0 : kMin, k == SortKind::Id ? kId32Max : kMax, kind);
}

AVal AbstractDomain::exec_actor() const { return range(SortKind::Actor, s_.exec.first, s_.exec.second); }
AVal AbstractDomain::result_code() const { return range(SortKind::Int, s_.result.first, s_.result.second); }

AVal AbstractDomain::constant(int32_t index) const {
  const phi::ConstDecl& c = p_.consts[static_cast<size_t>(index)];
  switch (c.value) {
    case phi::ConstDecl::Value::Int: return range(SortKind::Int, c.ival, c.ival);
    case phi::ConstDecl::Value::Str: return lit_str(c.sval);
    case phi::ConstDecl::Value::Bool: return tri_val(tri_of(c.bval));
    case phi::ConstDecl::Value::RefEval: {
      const auto it = s_.consts.find(index);
      if (it != s_.consts.end()) return range(SortKind::Int, it->second.first, it->second.second);
      return range(SortKind::Int, kMin, kMax);
    }
  }
  return range(SortKind::Int, kMin, kMax);
}

AVal AbstractDomain::state_constant(int32_t kind, int32_t ordinal) const { return mask_val(SortKind::State, bit(ordinal), kind); }
AVal AbstractDomain::tag_constant(int32_t kind, int32_t tag, int32_t ordinal) const { return mask_val(SortKind::Tag, bit(ordinal), kind, tag); }
AVal AbstractDomain::loc_ref(int32_t index) const { return range(SortKind::Loc, index, index); }

AVal AbstractDomain::make_res(int32_t kind, const std::vector<AVal>& keys) const {
  AVal r;
  r.sort = SortKind::Res;
  r.kind = kind;
  for (const AVal& k : keys) {
    if (k.sort == SortKind::Res) {
      r.may_be_undefined = r.may_be_undefined || k.may_be_undefined;
      r.must_be_undefined = r.must_be_undefined || k.must_be_undefined;
      r.key.insert(r.key.end(), k.key.begin(), k.key.end());
    } else {
      r.key.push_back({k.lo, k.hi});
    }
  }
  if (r.must_be_undefined) r.key.clear();
  return r;
}

AVal AbstractDomain::state_of(const AVal& res) const {
  const phi::KindDecl& k = p_.kinds[static_cast<size_t>(res.kind)];
  const uint64_t init = bit(k.initial_ordinal);
  if (res.must_be_undefined) return mask_val(SortKind::State, init, res.kind);
  // A resource that is not listed is in the initial state, or in any state of an open kind.
  const uint64_t unlisted = s_.open_kinds.count(res.kind) != 0 ? all_of(k.states.size()) : init;
  uint64_t m = 0;
  std::vector<int64_t> key;
  if (all_singletons(res.key, &key)) {
    const auto it = s_.res.find(ResId{res.kind, key});
    m = it != s_.res.end() ? it->second.states : unlisted;
  } else {
    m = unlisted;  // a resource in the box that is not listed
    for (const auto& kv : s_.res)
      if (kv.first.kind == res.kind && within_box(kv.first.key, res.key)) m |= kv.second.states;
  }
  if (res.may_be_undefined) m |= init;
  return mask_val(SortKind::State, m, res.kind);
}

AVal AbstractDomain::key_of(const AVal& res, size_t index, const Sort& sort) const {
  if (sort.kind == SortKind::Res) {
    AVal r;
    r.sort = SortKind::Res;
    r.kind = sort.kind_index;
    const size_t w = key_width(p_, sort.kind_index);
    if (res.must_be_undefined || index + w > res.key.size()) {
      r.may_be_undefined = r.must_be_undefined = true;
      return r;
    }
    r.may_be_undefined = res.may_be_undefined;
    r.key.assign(res.key.begin() + static_cast<std::ptrdiff_t>(index), res.key.begin() + static_cast<std::ptrdiff_t>(index + w));
    return r;
  }
  const SortKind k = sort.kind == SortKind::Unknown ? SortKind::Int : sort.kind;
  const int32_t kind = sort.kind == SortKind::Id ? sort.kind_index : -1;
  if (res.must_be_undefined || index >= res.key.size()) return range(k, 0, 0, kind);
  std::pair<int64_t, int64_t> iv = res.key[index];
  if (k == SortKind::Id) iv = id_interval(iv);
  if (res.may_be_undefined) iv = hull(iv, {0, 0});
  return range(k, iv.first, iv.second, kind);
}

AVal AbstractDomain::gen_of(const AVal&) const { return range(SortKind::Int, 0, 0); }

AVal AbstractDomain::valid(const AVal& id) const {
  if (id.sort != SortKind::Id) return tri_val(Tri::False);  // concrete: a value that is not an identifier is never valid
  if (id.kind < 0 || static_cast<size_t>(id.kind) >= p_.kinds.size()) return tri_val(Tri::Maybe);
  const phi::IdSpace& ids = p_.kinds[static_cast<size_t>(id.kind)].ids;
  if (!ids.present) return tri_val(Tri::False);  // concrete: false
  if (id.lo < 0 || id.hi > kId32Max) return tri_val(Tri::Maybe);
  // The valid ids are base .. base + serial_max.  The checker keeps both inside 32 bits; for a
  // profile that was not checked nothing is claimed (and nothing is added that could overflow).
  if (ids.base < 0 || ids.serial_max < 0 || ids.base > kId32Max || ids.serial_max > kId32Max - ids.base) return tri_val(Tri::Maybe);
  const int64_t lo = ids.base, hi = ids.base + ids.serial_max;
  if (id.lo >= lo && id.hi <= hi) return tri_val(Tri::True);
  if (id.hi < lo || id.lo > hi) return tri_val(Tri::False);
  return tri_val(Tri::Maybe);
}

AVal AbstractDomain::value_of(const AVal& loc) const {
  // The declared locations the interval can name, and whether it can name an undeclared one (the
  // concrete value of an undeclared location is 0 and has sort Int).  The loop runs over declared
  // locations only, so an unconstrained interval costs nothing and nothing overflows.
  const int64_t declared = static_cast<int64_t>(p_.locs.size());
  const bool malformed = loc.lo > loc.hi;
  const int64_t first = malformed ? 0 : std::max<int64_t>(loc.lo, 0);
  const int64_t last = malformed ? declared - 1 : std::min<int64_t>(loc.hi, declared - 1);
  bool out_of_range = malformed || loc.lo < 0 || loc.hi >= declared;
  bool any = false, all_id = true, any_id = false;
  int32_t id_kind = -2;
  bool id_kind_mixed = false;
  std::pair<int64_t, int64_t> iv{0, 0};
  for (int64_t li = first; li <= last; ++li) {
    const Sort& h = p_.locs[static_cast<size_t>(li)].holds.resolved;
    const auto it = s_.locs.find(static_cast<int32_t>(li));
    std::pair<int64_t, int64_t> v = it != s_.locs.end() ? it->second : std::pair<int64_t, int64_t>{0, 0};
    if (it == s_.locs.end() && s_.open_locs) v = {kMin, kMax};  // a real record: any value of its sort (an Id: below)
    if (h.kind == SortKind::Id) {
      v = id_interval(v);  // the concrete value of an Id location is read modulo 2^32
      any_id = true;
      if (id_kind == -2) id_kind = h.kind_index;
      else if (id_kind != h.kind_index) id_kind_mixed = true;
    } else {
      all_id = false;
    }
    iv = any ? hull(iv, v) : v;
    any = true;
  }
  if (out_of_range) iv = any ? hull(iv, {0, 0}) : std::pair<int64_t, int64_t>{0, 0};
  if (any_id && all_id && !out_of_range) return range(SortKind::Id, iv.first, iv.second, id_kind_mixed ? -1 : id_kind);
  if (any_id) return range(SortKind::Id, iv.first, iv.second, -1);
  return range(SortKind::Int, iv.first, iv.second);
}

AVal AbstractDomain::tag_of(const AVal& res, int32_t tag) const {
  uint64_t m = 0;
  if (res.must_be_undefined) return mask_val(SortKind::Tag, bit(0), res.kind, tag);
  std::vector<int64_t> key;
  auto tag_mask = [&](const AbstractRes& r) {
    const auto it = r.tags.find(tag);
    return it != r.tags.end() ? it->second : bit(0);
  };
  // A resource that is not listed has the first value, or any value in an open kind.
  uint64_t unlisted = bit(0);
  if (s_.open_kinds.count(res.kind) != 0) {
    const phi::KindDecl& k = p_.kinds[static_cast<size_t>(res.kind)];
    if (tag >= 0 && static_cast<size_t>(tag) < k.tags.size()) unlisted = all_of(k.tags[static_cast<size_t>(tag)].values.size());
  }
  if (all_singletons(res.key, &key)) {
    const auto it = s_.res.find(ResId{res.kind, key});
    m = it != s_.res.end() ? tag_mask(it->second) : unlisted;
  } else {
    m = unlisted;
    for (const auto& kv : s_.res)
      if (kv.first.kind == res.kind && within_box(kv.first.key, res.key)) m |= tag_mask(kv.second);
  }
  if (res.may_be_undefined) m |= bit(0);
  return mask_val(SortKind::Tag, m, res.kind, tag);
}

AVal AbstractDomain::id_of(const AVal& res) const {
  const phi::KindDecl& k = p_.kinds[static_cast<size_t>(res.kind)];
  const int64_t undef = k.ids.present ? id_value(k.ids.undefined) : 0;
  std::pair<int64_t, int64_t> iv{undef, undef};
  if (res.must_be_undefined) return range(SortKind::Id, iv.first, iv.second, res.kind);
  std::vector<int64_t> key;
  auto id_range = [&](const AbstractRes& r) { return r.has_id ? id_interval(r.id) : std::pair<int64_t, int64_t>{undef, undef}; };
  // A resource of an open kind that is not listed may carry any id (only kinds with ids carry one).
  const bool open = k.ids.present && s_.open_kinds.count(res.kind) != 0;
  if (all_singletons(res.key, &key)) {
    const auto it = s_.res.find(ResId{res.kind, key});
    if (it != s_.res.end()) iv = id_range(it->second);
    else if (open) iv = {0, kId32Max};
  } else {
    if (open) iv = {0, kId32Max};
    for (const auto& kv : s_.res)
      if (kv.first.kind == res.kind && within_box(kv.first.key, res.key)) iv = hull(iv, id_range(kv.second));
  }
  if (res.may_be_undefined) iv = hull(iv, {undef, undef});
  return range(SortKind::Id, iv.first, iv.second, res.kind);
}

AVal AbstractDomain::owner_of(const AVal& actor) const {
  AVal r;
  r.sort = SortKind::Res;
  r.kind = p_.context.owner_kind_index;
  const bool maybe_exec = !(actor.hi < s_.exec.first || actor.lo > s_.exec.second);
  const bool sure_exec = actor.lo == actor.hi && s_.exec.first == s_.exec.second && actor.lo == s_.exec.first;
  if (maybe_exec && s_.owner_any && r.kind >= 0) {
    // A real record whose owner is undetermined: any record of the owner kind, or undefined.
    for (size_t i = 0; i < key_width(p_, r.kind); ++i) r.key.push_back({kMin, kMax});
    r.may_be_undefined = true;
    return r;
  }
  if (!maybe_exec || (s_.owners.empty())) {
    r.may_be_undefined = r.must_be_undefined = true;
    return r;
  }
  // A listed owner that is not a record of the owner kind of the context (another kind, another key
  // width) is not an owner: the concrete domain reads it as undefined.  Only the well-formed owners
  // are hulled, all of the context kind; an ill-formed one only adds "maybe undefined".
  const int32_t ok_kind = p_.context.owner_kind_index;
  const size_t width = ok_kind >= 0 ? key_width(p_, ok_kind) : 0;
  bool first = true, ill_formed = false;
  for (const ResId& o : s_.owners) {
    if (o.kind != ok_kind || o.key.size() != width) {
      ill_formed = true;
      continue;
    }
    if (first) {
      for (int64_t k : o.key) r.key.push_back({k, k});
      first = false;
    } else {
      for (size_t i = 0; i < o.key.size(); ++i) r.key[i] = hull(r.key[i], {o.key[i], o.key[i]});
    }
  }
  if (first) {  // no well-formed owner: the owner is undefined in every concrete state described
    r.may_be_undefined = r.must_be_undefined = true;
    r.key.clear();
    return r;
  }
  r.may_be_undefined = !sure_exec || s_.owner_none || ill_formed;
  return r;
}

std::vector<AVal> AbstractDomain::instances_of(int32_t kind) const {
  std::vector<AVal> out;
  const auto it = s_.instances.find(kind);
  if (it == s_.instances.end()) return out;
  for (const std::vector<int64_t>& key : it->second) {
    AVal r;
    r.sort = SortKind::Res;
    r.kind = kind;
    for (int64_t k : key) r.key.push_back({k, k});
    out.push_back(std::move(r));
  }
  return out;
}

AVal AbstractDomain::select_state(int32_t kind, const std::vector<AVal>& conds, const std::vector<int32_t>& ordinals, int32_t fallback) const {
  uint64_t m = 0;
  bool decided = false;
  for (size_t i = 0; i < conds.size() && !decided; ++i) {
    if (conds[i].tri == Tri::False) continue;
    m |= bit(ordinals[i]);
    if (conds[i].tri == Tri::True) decided = true;
  }
  if (!decided) m |= bit(fallback);
  return mask_val(SortKind::State, m, kind);
}

}  // namespace eval
}  // namespace kz
