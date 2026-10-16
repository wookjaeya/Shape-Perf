// phi_expr.cpp -- construction, cloning and printing of core expressions and patterns.

#include <cstdio>

#include "kz/core/phi.hpp"

namespace kz {
namespace phi {

ExprPtr clone_expr(const Expr& e) {
  ExprPtr c(new Expr());
  c->kind = e.kind;
  c->loc = e.loc;
  c->bval = e.bval;
  c->ival = e.ival;
  c->text = e.text;
  c->kind_name = e.kind_name;
  c->cmp = e.cmp;
  c->fn = e.fn;
  c->quant = e.quant;
  c->sort = e.sort;
  c->ref = e.ref;
  c->ref_index = e.ref_index;
  c->ref_aux = e.ref_aux;
  for (const ExprPtr& a : e.args) c->args.push_back(a ? clone_expr(*a) : ExprPtr());
  return c;
}

ExprPtr make_bool(bool v) {
  ExprPtr e(new Expr());
  e->kind = ExprKind::BoolLit;
  e->bval = v;
  return e;
}

ExprPtr make_int(int64_t v) {
  ExprPtr e(new Expr());
  e->kind = ExprKind::IntLit;
  e->ival = v;
  return e;
}

ExprPtr make_name(std::string path) {
  ExprPtr e(new Expr());
  e->kind = ExprKind::Name;
  e->text = std::move(path);
  return e;
}

ExprPtr make_not(ExprPtr x) {
  ExprPtr e(new Expr());
  e->kind = ExprKind::Not;
  e->args.push_back(std::move(x));
  return e;
}

ExprPtr make_binary_list(ExprKind kind, std::vector<ExprPtr> operands) {
  ExprPtr e(new Expr());
  e->kind = kind;
  for (ExprPtr& o : operands) {
    if (o->kind == kind) {
      for (ExprPtr& c : o->args) e->args.push_back(std::move(c));
    } else {
      e->args.push_back(std::move(o));
    }
  }
  return e;
}

ExprPtr make_cmp(CmpOp op, ExprPtr a, ExprPtr b) {
  ExprPtr e(new Expr());
  e->kind = ExprKind::Cmp;
  e->cmp = op;
  e->args.push_back(std::move(a));
  e->args.push_back(std::move(b));
  return e;
}

ExprPtr make_call(CallFn fn, std::string name, std::vector<ExprPtr> args) {
  ExprPtr e(new Expr());
  e->kind = ExprKind::Call;
  e->fn = fn;
  e->text = std::move(name);
  e->args = std::move(args);
  return e;
}

const char* cmp_text(CmpOp op) {
  switch (op) {
    case CmpOp::Eq: return "=";
    case CmpOp::Ne: return "!=";
    case CmpOp::Lt: return "<";
    case CmpOp::Le: return "<=";
    case CmpOp::Gt: return ">";
    case CmpOp::Ge: return ">=";
  }
  return "?";
}

const char* stratum_text(Stratum s) {
  switch (s) {
    case Stratum::S: return "S";
    case Stratum::O: return "O";
    case Stratum::C: return "C";
  }
  return "?";
}

const char* robust_text(Robust r) {
  switch (r) {
    case Robust::Unconditional: return "unconditional";
    case Robust::ObsFatal: return "obs_fatal";
    case Robust::ObsSoft: return "obs_soft";
    case Robust::ScheduleChosen: return "schedule_chosen";
    case Robust::Assumed: return "assumed";
  }
  return "?";
}

const char* template_text(RuleTemplate t) {
  switch (t) {
    case RuleTemplate::ProgramOrder: return "program_order";
    case RuleTemplate::Spawn: return "spawn";
    case RuleTemplate::Phase: return "phase";
    case RuleTemplate::Observe: return "observe";
  }
  return "?";
}

const char* allowed_text(AllowedKind k) {
  switch (k) {
    case AllowedKind::None: return "none";
    case AllowedKind::Unspecified: return "unspecified_by_reference";
    case AllowedKind::DocumentedDrop: return "documented_drop";
    case AllowedKind::ReturnedAndTested: return "returned_and_tested";
    case AllowedKind::RetryUntilReady: return "retry_until_ready";
    case AllowedKind::Fallback: return "fallback";
    case AllowedKind::DeferByDesign: return "defer_by_design";
    case AllowedKind::Exit: return "exit";
  }
  return "?";
}

const char* effect_text(EffectKind k) {
  switch (k) {
    case EffectKind::Set: return "lifecycle";
    case EffectKind::Raise: return "lifecycle";
    case EffectKind::Use: return "use";
    case EffectKind::StoreOut: return "store";
    case EffectKind::StoreLoc: return "store";
    case EffectKind::SetTag: return "settag";
    case EffectKind::Mark: return "mark";
    case EffectKind::Sync: return "sync";
    case EffectKind::Env: return "env";
  }
  return "?";
}

const char* outcome_class_text(OutcomeClass c) {
  switch (c) {
    case OutcomeClass::Ready: return "ready";
    case OutcomeClass::NotReady: return "not_ready";
    case OutcomeClass::Error: return "error";
  }
  return "?";
}

std::string quote_string(const std::string& s) {
  std::string out = "\"";
  for (char c : s) {
    if (c == '\\') out += "\\\\";
    else if (c == '"') out += "\\\"";
    else if (c == '\n') out += "\\n";
    else if (c == '\t') out += "\\t";
    else out.push_back(c);
  }
  out.push_back('"');
  return out;
}

namespace {

// Precedence: Imp 1, Or 2, And 3, Not 4, Cmp/In 5, atoms 6.
int prec_of(const Expr& e) {
  switch (e.kind) {
    case ExprKind::Imp: return 1;
    case ExprKind::Or: return 2;
    case ExprKind::And: return 3;
    case ExprKind::Not: return 4;
    case ExprKind::Cmp:
    case ExprKind::In: return 5;
    default: return 6;
  }
}

void write(const Expr* e, int min_prec, std::string* out);

void write_child(const Expr* e, int min_prec, std::string* out) {
  if (e == nullptr) {
    *out += "<null>";
    return;
  }
  const bool paren = prec_of(*e) < min_prec;
  if (paren) out->push_back('(');
  write(e, 0, out);
  if (paren) out->push_back(')');
}

void write(const Expr* e, int /*unused*/, std::string* out) {
  switch (e->kind) {
    case ExprKind::BoolLit: *out += e->bval ? "true" : "false"; return;
    case ExprKind::IntLit: {
      const int64_t v = e->ival;
      if (v > 1000000) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "0x%llx", static_cast<unsigned long long>(v));
        *out += buf;
      } else {
        *out += std::to_string(v);
      }
      return;
    }
    case ExprKind::StrLit: *out += quote_string(e->text); return;
    case ExprKind::Name: *out += e->text; return;
    case ExprKind::Not:
      out->push_back('!');
      write_child(e->args[0].get(), 4, out);
      return;
    case ExprKind::And:
    case ExprKind::Or: {
      const char* sep = e->kind == ExprKind::And ? " && " : " || ";
      const int own = e->kind == ExprKind::And ? 3 : 2;
      for (size_t i = 0; i < e->args.size(); ++i) {
        if (i != 0) *out += sep;
        // nested lists of the same kind print flat: parsing flattens them anyway
        const Expr* c = e->args[i].get();
        if (c != nullptr && c->kind == e->kind) {
          write(c, 0, out);
        } else {
          write_child(c, own + 1, out);
        }
      }
      return;
    }
    case ExprKind::Imp:
      write_child(e->args[0].get(), 2, out);
      *out += " => ";
      write_child(e->args[1].get(), 1, out);
      return;
    case ExprKind::Cmp:
      write_child(e->args[0].get(), 6, out);
      *out += " ";
      *out += cmp_text(e->cmp);
      *out += " ";
      write_child(e->args[1].get(), 6, out);
      return;
    case ExprKind::In:
      write_child(e->args[0].get(), 6, out);
      *out += " in {";
      for (size_t i = 1; i < e->args.size(); ++i) {
        if (i != 1) *out += ", ";
        write_child(e->args[i].get(), 6, out);
      }
      *out += "}";
      return;
    case ExprKind::Call:
      *out += e->text;
      out->push_back('(');
      for (size_t i = 0; i < e->args.size(); ++i) {
        if (i != 0) *out += ", ";
        write_child(e->args[i].get(), 0, out);
      }
      out->push_back(')');
      return;
    case ExprKind::Quant:
      *out += e->quant == QuantKind::Forall ? "forall " : "exists ";
      *out += e->text + ": " + e->kind_name;
      if (e->args[0]) {
        *out += " where ";
        write_child(e->args[0].get(), 0, out);
      }
      *out += " { ";
      write_child(e->args[1].get(), 0, out);
      *out += " }";
      return;
  }
}

}  // namespace

std::string expr_text(const Expr& e) {
  std::string out;
  write(&e, 0, &out);
  return out;
}

std::string pattern_text(const Pattern& p) {
  std::string out = "fn(" + quote_string(p.fn) + ").";
  switch (p.op) {
    case PatOp::Call: {
      out += "call(" + quote_string(p.callee);
      for (const ArgPred& a : p.args) {
        out += ", arg[" + std::to_string(a.index) + "] ";
        switch (a.cmp) {
          case ArgCmp::Eq:
          case ArgCmp::Ne:
            out += a.cmp == ArgCmp::Eq ? "== " : "!= ";
            out += a.is_name ? a.name : std::to_string(a.ival);
            break;
          case ArgCmp::AddrOf: out += "is addr(" + a.name + ")"; break;
          case ArgCmp::Any: out += "any"; break;
        }
      }
      out += ")";
      break;
    }
    case PatOp::Store: out += "store(" + p.type_name + "." + p.field + ")"; break;
    case PatOp::Load: out += "load(" + p.type_name + "." + p.field + ")"; break;
    case PatOp::Memset: out += "memset(" + p.type_name + ")"; break;
    case PatOp::LoopPoll: out += "loop(poll)"; break;
    case PatOp::Return: out += "return"; break;
  }
  if (p.nth != 0) out += " nth " + std::to_string(p.nth);
  if (p.macro == MacroFlag::Yes) out += " macro yes";
  if (p.macro == MacroFlag::No) out += " macro no";
  return out;
}

}  // namespace phi
}  // namespace kz
