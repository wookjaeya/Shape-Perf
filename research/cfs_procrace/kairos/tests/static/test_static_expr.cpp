// The expression pool of the extractor: hash-consing, folding of constants in conditions, text.
#include "model.hpp"
#include "support/kz_test.hpp"

using namespace kzs;

int main() {
  ExprPool p;

  // structurally equal trees are one node; different trees are not
  ExprRef a = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_LOCAL, "x"), p.constant(3));
  ExprRef b = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_LOCAL, "x"), p.constant(3));
  ExprRef c = p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_LOCAL, "x"), p.constant(4));
  KZ_CHECK(a == b);
  KZ_CHECK(a != c);
  KZ_CHECK(p.load(KZ_EXPR_REF_LOCAL, "x") != p.load(KZ_EXPR_REF_GLOBAL, "x"));
  KZ_CHECK_EQ(expr_text(a), std::string("(eq local:x 3)"));

  // constants fold: `while (1)` leaves no (ne 1 0) behind
  ExprRef one = p.constant(1), zero = p.constant(0);
  KZ_CHECK(p.binary(KZ_EXPR_OP_NE, one, zero) == one);
  KZ_CHECK(p.binary(KZ_EXPR_OP_EQ, one, zero) == zero);
  KZ_CHECK(p.binary(KZ_EXPR_OP_LT, zero, one) == one);
  KZ_CHECK(p.binary(KZ_EXPR_OP_GE, zero, one) == zero);
  KZ_CHECK(p.unary(KZ_EXPR_OP_NOT, one) == zero);
  KZ_CHECK(p.unary(KZ_EXPR_OP_NOT, zero) == one);

  // and / or with a constant operand: the other operand, or the constant
  KZ_CHECK(p.binary(KZ_EXPR_OP_AND, one, a) == a);
  KZ_CHECK(p.binary(KZ_EXPR_OP_AND, a, one) == a);
  KZ_CHECK(p.binary(KZ_EXPR_OP_AND, zero, a) == zero);
  KZ_CHECK(p.binary(KZ_EXPR_OP_AND, a, zero) == zero);
  KZ_CHECK(p.binary(KZ_EXPR_OP_OR, zero, a) == a);
  KZ_CHECK(p.binary(KZ_EXPR_OP_OR, a, zero) == a);
  KZ_CHECK(p.binary(KZ_EXPR_OP_OR, one, a) == one);
  KZ_CHECK(p.binary(KZ_EXPR_OP_OR, a, one) == one);
  // an operand that is not a constant is kept
  KZ_CHECK_EQ(expr_text(p.binary(KZ_EXPR_OP_AND, a, c)), std::string("(and (eq local:x 3) (eq local:x 4))"));
  // arithmetic is not folded: it keeps the shape the source has
  KZ_CHECK_EQ(expr_text(p.binary(KZ_EXPR_OP_ADD, p.constant(1), p.constant(2))), std::string("(add 1 2)"));

  // conj: left-nested, skips null operands, empty gives null
  KZ_CHECK(p.conj(KZ_EXPR_OP_OR, {}) == nullptr);
  KZ_CHECK(p.conj(KZ_EXPR_OP_OR, {nullptr, a, nullptr}) == a);
  KZ_CHECK_EQ(expr_text(p.conj(KZ_EXPR_OP_OR, {a, c, b})), std::string("(or (or (eq local:x 3) (eq local:x 4)) (eq local:x 3))"));

  // lists are cons cells in order
  ExprRef l = p.list(KZ_EXPR_OP_SEQ, {p.constant(1), p.constant(2)});
  KZ_CHECK(l != nullptr && l->a == p.constant(1) && l->b != nullptr && l->b->a == p.constant(2) && l->b->b == nullptr);
  KZ_CHECK(p.list(KZ_EXPR_OP_SEQ, {}) == nullptr);

  // the same pool never grows when a tree is built twice
  const size_t before = p.size();
  p.binary(KZ_EXPR_OP_EQ, p.load(KZ_EXPR_REF_LOCAL, "x"), p.constant(3));
  KZ_CHECK_EQ(p.size(), before);
  return kztest::finish("test_static_expr");
}
