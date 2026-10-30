// eval_evaluator.hpp -- the evaluator template (included at the end of eval.hpp).
//
// There is exactly one place where the meaning of the core expression language is written
// down: this file.  A domain supplies the primitives; it never sees an expression.  Every
// operand is evaluated (no short-circuit), so a domain can be partial without the evaluator
// depending on evaluation order.
#pragma once

#include <algorithm>
#include <vector>

#include "kz/core/phi_ast.hpp"

namespace kz {
namespace eval {

template <class D>
class Evaluator {
 public:
  using Val = typename D::Val;

  Evaluator(const phi::Profile& p, const D& d) : p_(p), d_(d) {}

  // `vars` is the stack of bound variables; its first elements are the variables of the
  // environment the expression was resolved in (the parameters of a kind, in order).
  Val eval(const phi::Expr& e, std::vector<Val>* vars) const {
    using phi::ExprKind;
    switch (e.kind) {
      case ExprKind::BoolLit: return d_.lit_bool(e.bval);
      case ExprKind::IntLit: return d_.lit_int(e.ival);
      case ExprKind::StrLit: return d_.lit_str(e.text);
      case ExprKind::Name: return eval_name(e, vars);
      case ExprKind::Not: return d_.lnot(eval(*e.args[0], vars));
      case ExprKind::And: {
        Val acc = eval(*e.args[0], vars);
        for (size_t i = 1; i < e.args.size(); ++i) acc = d_.land(acc, eval(*e.args[i], vars));
        return acc;
      }
      case ExprKind::Or: {
        Val acc = eval(*e.args[0], vars);
        for (size_t i = 1; i < e.args.size(); ++i) acc = d_.lor(acc, eval(*e.args[i], vars));
        return acc;
      }
      case ExprKind::Imp: {
        Val a = eval(*e.args[0], vars);
        Val b = eval(*e.args[1], vars);
        return d_.lor(d_.lnot(a), b);
      }
      case ExprKind::Cmp: {
        Val a = eval(*e.args[0], vars);
        Val b = eval(*e.args[1], vars);
        return d_.cmp(e.cmp, a, b);
      }
      case ExprKind::In: {
        Val x = eval(*e.args[0], vars);
        Val acc = d_.cmp(phi::CmpOp::Eq, x, eval(*e.args[1], vars));
        for (size_t i = 2; i < e.args.size(); ++i) acc = d_.lor(acc, d_.cmp(phi::CmpOp::Eq, x, eval(*e.args[i], vars)));
        return acc;
      }
      case ExprKind::Call: return eval_call(e, vars);
      case ExprKind::Quant: return eval_quant(e, vars);
    }
    return d_.lit_bool(false);
  }

  Val eval(const phi::Expr& e) const {
    std::vector<Val> vars;
    return eval(e, &vars);
  }

  // The state of a resource, including the kinds whose state is derived from other resources
  // (the first derived rule that holds, tried from the highest state down, else the initial state).
  Val state_of(const Val& res, int32_t kind) const {
    const phi::KindDecl& k = p_.kinds[static_cast<size_t>(kind)];
    if (k.derived.empty()) return d_.state_of(res);
    std::vector<Val> kvars;
    size_t offset = 0;
    for (size_t i = 0; i < k.params.size(); ++i) {
      kvars.push_back(d_.key_of(res, offset, k.params[i].sort.resolved));
      offset += k.params[i].sort.resolved.kind == phi::SortKind::Res ? key_width(p_, k.params[i].sort.resolved.kind_index) : 1;
    }
    std::vector<size_t> order(k.derived.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::vector<int32_t> ordinal_of(k.derived.size(), 0);
    for (size_t i = 0; i < k.derived.size(); ++i)
      for (size_t s = 0; s < k.states.size(); ++s)
        if (k.states[s] == k.derived[i].state) ordinal_of[i] = static_cast<int32_t>(s);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return ordinal_of[a] > ordinal_of[b]; });
    std::vector<Val> conds;
    std::vector<int32_t> ordinals;
    for (size_t i : order) {
      conds.push_back(eval(*k.derived[i].when, &kvars));
      ordinals.push_back(ordinal_of[i]);
    }
    return d_.select_state(kind, conds, ordinals, k.initial_ordinal);
  }

 private:
  const phi::Profile& p_;
  const D& d_;

  Val eval_name(const phi::Expr& e, std::vector<Val>* vars) const {
    using phi::NameRef;
    switch (e.ref) {
      case NameRef::BoundVar: return (*vars)[static_cast<size_t>(e.ref_index)];
      case NameRef::Arg: return d_.arg(e.ref_index, e.sort);
      case NameRef::Exec: return d_.exec_actor();
      case NameRef::Result: return d_.result_code();
      case NameRef::Const: return d_.constant(e.ref_index);
      case NameRef::StateConst: return d_.state_constant(e.sort.kind_index, e.ref_index);
      case NameRef::TagConst: return d_.tag_constant(e.sort.kind_index, e.sort.tag_index, e.ref_index);
      case NameRef::LocRef: return d_.loc_ref(e.ref_index);
      case NameRef::Unresolved: break;
    }
    return d_.lit_bool(false);  // an unresolved name: the checker rejects it, the evaluator is not run
  }

  Val eval_call(const phi::Expr& e, std::vector<Val>* vars) const {
    using phi::CallFn;
    switch (e.fn) {
      case CallFn::State: {
        Val r = eval(*e.args[0], vars);
        return state_of(r, e.args[0]->sort.kind_index);
      }
      case CallFn::Key: return d_.key_of(eval(*e.args[0], vars), 0, e.sort);
      case CallFn::Gen: return d_.gen_of(eval(*e.args[0], vars));
      case CallFn::Valid: return d_.valid(eval(*e.args[0], vars));
      case CallFn::Value: return d_.value_of(eval(*e.args[0], vars));
      case CallFn::Tag: return d_.tag_of(eval(*e.args[0], vars), e.ref_aux);
      case CallFn::Id: return d_.id_of(eval(*e.args[0], vars));
      case CallFn::Owner: return d_.owner_of(eval(*e.args[0], vars));
      case CallFn::Res: {
        std::vector<Val> keys;
        for (const phi::ExprPtr& a : e.args) keys.push_back(eval(*a, vars));
        return d_.make_res(e.ref_index, keys);
      }
    }
    return d_.lit_bool(false);
  }

  Val eval_quant(const phi::Expr& e, std::vector<Val>* vars) const {
    const bool all = e.quant == phi::QuantKind::Forall;
    Val acc = d_.lit_bool(all);
    for (const Val& inst : d_.instances_of(e.ref_index)) {
      vars->push_back(inst);
      Val g = e.args[0] ? eval(*e.args[0], vars) : d_.lit_bool(true);
      Val b = eval(*e.args[1], vars);
      vars->pop_back();
      acc = all ? d_.land(acc, d_.lor(d_.lnot(g), b)) : d_.lor(acc, d_.land(g, b));
    }
    return acc;
  }
};

}  // namespace eval
}  // namespace kz
