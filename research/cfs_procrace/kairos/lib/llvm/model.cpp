// model.cpp -- see model.hpp.
#include "model.hpp"

#include <cstdio>

namespace kzs {

ExprRef ExprPool::make(uint16_t op, uint16_t ref, int64_t value, const std::string& name, ExprRef a, ExprRef b) {
  // Children are already hash-consed, so structural equality is pointer equality of the
  // children.  The pointer values only build the lookup key; they never reach an output.
  char buf[96];
  std::snprintf(buf, sizeof buf, "%u|%u|%lld|%p|%p|", static_cast<unsigned>(op), static_cast<unsigned>(ref),
                static_cast<long long>(value), static_cast<const void*>(a), static_cast<const void*>(b));
  std::string key = buf;
  key += name;
  auto it = index_.find(key);
  if (it != index_.end()) return it->second;
  nodes_.emplace_back();
  ExprNode& n = nodes_.back();
  n.op = op;
  n.ref = ref;
  n.value = value;
  n.name = name;
  n.a = a;
  n.b = b;
  index_[key] = &n;
  return &n;
}

namespace {
bool is_const(ExprRef e) { return e != nullptr && e->op == KZ_EXPR_OP_CONST && e->a == nullptr && e->b == nullptr; }
}  // namespace

ExprRef ExprPool::unary(uint16_t op, ExprRef a) {
  if (op == KZ_EXPR_OP_NOT && is_const(a)) return constant(a->value == 0 ? 1 : 0);
  return make(op, 0, 0, "", a, nullptr);
}

ExprRef ExprPool::binary(uint16_t op, ExprRef a, ExprRef b) {
  if (is_const(a) && is_const(b)) {
    const int64_t x = a->value, y = b->value;
    switch (op) {
      case KZ_EXPR_OP_AND: return constant((x != 0 && y != 0) ? 1 : 0);
      case KZ_EXPR_OP_OR: return constant((x != 0 || y != 0) ? 1 : 0);
      case KZ_EXPR_OP_EQ: return constant(x == y ? 1 : 0);
      case KZ_EXPR_OP_NE: return constant(x != y ? 1 : 0);
      case KZ_EXPR_OP_LT: return constant(x < y ? 1 : 0);
      case KZ_EXPR_OP_LE: return constant(x <= y ? 1 : 0);
      case KZ_EXPR_OP_GT: return constant(x > y ? 1 : 0);
      case KZ_EXPR_OP_GE: return constant(x >= y ? 1 : 0);
      default: break;
    }
  }
  if (op == KZ_EXPR_OP_AND) {
    if (is_const(a)) return a->value != 0 ? b : a;
    if (is_const(b)) return b->value != 0 ? a : b;
  } else if (op == KZ_EXPR_OP_OR) {
    if (is_const(a)) return a->value != 0 ? a : b;
    if (is_const(b)) return b->value != 0 ? b : a;
  }
  return make(op, 0, 0, "", a, b);
}

ExprRef ExprPool::list(uint16_t op, const std::vector<ExprRef>& elems) {
  ExprRef rest = nullptr;
  for (size_t i = elems.size(); i > 0; --i) rest = make(op, 0, 0, "", elems[i - 1], rest);
  return rest;
}

ExprRef ExprPool::conj(uint16_t op, const std::vector<ExprRef>& operands) {
  ExprRef acc = nullptr;
  for (ExprRef e : operands) {
    if (e == nullptr) continue;
    acc = acc == nullptr ? e : binary(op, acc, e);
  }
  return acc;
}

namespace {

const char* op_name(uint16_t op) {
  switch (op) {
#define KZ_OPN(P, id, value, display) \
  case value: return display;
    KZ_ENUM_EXPR_OP(KZ_OPN, 0)
#undef KZ_OPN
    default: return "?";
  }
}

const char* ref_name(uint16_t ref) {
  switch (ref) {
#define KZ_REFN(P, id, value, display) \
  case value: return display;
    KZ_ENUM_EXPR_REF(KZ_REFN, 0)
#undef KZ_REFN
    default: return "?";
  }
}

// The levels of an expression text (a tool parameter no reference fixes: unspecified_by_reference; BASIS
// param.walk_bounds; the texts stored in a fact that are cut are counted, count.walk_bounds_hit).
constexpr int kEmitDepth = 64;

void emit(ExprRef e, std::string* out, int depth, bool* cut) {
  if (e == nullptr) {
    *out += "-";
    return;
  }
  if (depth > kEmitDepth) {
    *out += "...";
    if (cut != nullptr) *cut = true;
    return;
  }
  const bool leaf = e->a == nullptr && e->b == nullptr;
  if (leaf && e->op == KZ_EXPR_OP_CONST) {
    *out += std::to_string(e->value);
    return;
  }
  if (leaf && e->op == KZ_EXPR_OP_LOAD) {
    *out += std::string(ref_name(e->ref)) + ":" + e->name;
    if (e->ref == KZ_EXPR_REF_PARAM) *out += "#" + std::to_string(e->value);
    return;
  }
  *out += "(";
  *out += op_name(e->op);
  if (!e->name.empty()) *out += " " + e->name;
  if (e->op == KZ_EXPR_OP_CONST || e->op == KZ_EXPR_OP_FORALL || e->op == KZ_EXPR_OP_CURSOR) *out += " " + std::to_string(e->value);
  if (e->a != nullptr) {
    *out += " ";
    emit(e->a, out, depth + 1, cut);
  }
  if (e->b != nullptr) {
    *out += " ";
    emit(e->b, out, depth + 1, cut);
  }
  *out += ")";
}

}  // namespace

std::string expr_text(ExprRef e, bool* cut) {
  std::string out;
  if (cut != nullptr) *cut = false;
  emit(e, &out, 0, cut);
  return out;
}

}  // namespace kzs
