// ast_fn.cpp -- reading one function body (SPEC 4.1 step 3, 4.2 steps 2-3, 4.3 loop facts).
//
// FnWalker walks the body once, by explicit recursion, keeping three stacks: the conditions the
// current statement is guarded by (if/else, ?:, && and ||), the enclosing loops, and the
// enclosing breakable statements (loops and switches).  From that it writes:
//   * a call fact for every call, with its arguments, `derives` rows for the arguments whose
//     parameter has a role, out-pointer calls, spawn and terminal calls;
//   * writer facts for the extraction targets, and the A-TYPE listing entries (casts, address
//     escapes, union members, untyped writes);
//   * loop facts, including the exit conditions, mod set and budget of loops with a delay call,
//     and the exact trip count of counted loops;
//   * the out-pointer summary of every pointer parameter;
//   * the edges of the function-pointer flow graph;
//   * with spec.extended_facts (F1-F3), the field and global reads with their use, the return values
//     resolved through the definitions of the returned local, and the exits of every loop;
//   * with spec.extended_facts (F4), the source of every call argument, the uses of copies of the
//     parameters, and the stores of global addresses to pointer fields with the order the body's
//     control-flow graph gives them.
// Nothing here knows a framework: every name comes from the ExtractSpec.
#include <algorithm>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>

#include "ast_reader.hpp"
#include "clang/AST/APValue.h"
#include "clang/AST/Expr.h"
#include "clang/AST/ParentMapContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Stmt.h"
#include "clang/Analysis/CFG.h"
#include "llvm/Support/Casting.h"

namespace kzs {

using namespace clang;

namespace {

bool is_fp_type(QualType t) {
  t = t.getCanonicalType();
  while (t->isArrayType()) t = cast<ArrayType>(t.getTypePtr())->getElementType().getCanonicalType();
  return t->isFunctionPointerType() || t->isFunctionType();
}

// A pointer to a function pointer: the shape of an out parameter that delivers a callback.
bool is_fp_ptr_type(QualType t) {
  t = t.getCanonicalType();
  return t->isPointerType() && t->getPointeeType().getCanonicalType()->isFunctionPointerType();
}

bool is_untyped_pointee(QualType p) {
  p = p.getCanonicalType();
  if (p->isVoidType()) return true;
  if (p->isCharType()) return true;
  if (const auto* bt = p->getAs<BuiltinType>()) {
    switch (bt->getKind()) {
      case BuiltinType::Char_S:
      case BuiltinType::Char_U:
      case BuiltinType::SChar:
      case BuiltinType::UChar: return true;
      default: return false;
    }
  }
  return false;
}

// ---- the pre-pass: definitions of locals, address-taken locals, variables used in tests -----------

struct DefInfo {
  const Expr* rhs = nullptr;  // null for an arithmetic definition
  bool copy = false;
  SourceLocation loc;
};
struct LocalInfo {
  std::vector<DefInfo> defs;
  bool addr_taken = false;
};

class LocalPre : public RecursiveASTVisitor<LocalPre> {
 public:
  LocalPre(std::map<const VarDecl*, LocalInfo>* locals, std::set<const VarDecl*>* tested) : locals_(locals), tested_(tested) {}

  bool VisitVarDecl(VarDecl* vd) {
    if (!vd->isLocalVarDecl() || vd->isStaticLocal()) return true;
    if (const Expr* init = vd->getInit()) {
      DefInfo d;
      d.rhs = init;
      d.copy = !isa<InitListExpr>(init->IgnoreParens());
      d.loc = vd->getLocation();
      (*locals_)[vd].defs.push_back(d);
    } else {
      (void)(*locals_)[vd];
    }
    return true;
  }
  bool VisitBinaryOperator(BinaryOperator* bo) {
    if (bo->isAssignmentOp()) {
      if (const auto* dre = dyn_cast<DeclRefExpr>(bo->getLHS()->IgnoreParenImpCasts())) {
        if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
          DefInfo d;
          d.copy = bo->getOpcode() == BO_Assign;
          d.rhs = d.copy ? bo->getRHS() : nullptr;
          d.loc = bo->getOperatorLoc();
          (*locals_)[vd].defs.push_back(d);
        }
      }
    }
    if (bo->isComparisonOp() || bo->getOpcode() == BO_LAnd || bo->getOpcode() == BO_LOr) {
      collect_tested(bo->getLHS());
      collect_tested(bo->getRHS());
    }
    return true;
  }
  bool VisitUnaryOperator(UnaryOperator* uo) {
    const Expr* sub = uo->getSubExpr()->IgnoreParenImpCasts();
    const auto* dre = dyn_cast<DeclRefExpr>(sub);
    if (dre != nullptr) {
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
        if (uo->isIncrementDecrementOp()) {
          DefInfo d;
          d.loc = uo->getOperatorLoc();
          (*locals_)[vd].defs.push_back(d);
        } else if (uo->getOpcode() == UO_AddrOf) {
          (*locals_)[vd].addr_taken = true;
        }
      }
    }
    if (uo->getOpcode() == UO_LNot) collect_tested(uo->getSubExpr());
    return true;
  }
  bool VisitIfStmt(IfStmt* s) {
    collect_tested(s->getCond());
    return true;
  }
  bool VisitWhileStmt(WhileStmt* s) {
    collect_tested(s->getCond());
    return true;
  }
  bool VisitDoStmt(DoStmt* s) {
    collect_tested(s->getCond());
    return true;
  }
  bool VisitForStmt(ForStmt* s) {
    if (s->getCond() != nullptr) collect_tested(s->getCond());
    return true;
  }
  bool VisitConditionalOperator(ConditionalOperator* s) {
    collect_tested(s->getCond());
    return true;
  }
  bool VisitSwitchStmt(SwitchStmt* s) {
    collect_tested(s->getCond());
    return true;
  }

 private:
  void collect_tested(const Expr* e) {
    if (e == nullptr) return;
    struct V : RecursiveASTVisitor<V> {
      std::set<const VarDecl*>* out;
      bool VisitDeclRefExpr(DeclRefExpr* d) {
        if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()))
          if (vd->isLocalVarDecl()) out->insert(vd);
        return true;
      }
    } v;
    v.out = tested_;
    v.TraverseStmt(const_cast<Expr*>(e));
  }
  std::map<const VarDecl*, LocalInfo>* locals_;
  std::set<const VarDecl*>* tested_;
};

// ---- loop context ---------------------------------------------------------------------------------

struct LoopCtx {
  size_t index = 0;  // in Model::loops
  const Stmt* stmt = nullptr;
  SourceRange range;
  size_t guard_base = 0;
  std::vector<ExprRef> exits;
  std::set<std::string> mod_names;  // "field:T.f", "global:x", "local:x", "deref"
  std::set<std::string> callees;
  std::vector<std::pair<const VarDecl*, const Expr*>> decrements;  // variable and the decrement amount (null for --), in walk order
  std::set<const VarDecl*> other_mods;
  uint32_t n_exits = 0;
  uint32_t n_calls = 0;
  uint32_t shared_loads = 0, shared_loads_locked = 0;
  bool volatile_load = false;
  uint32_t lock_calls = 0;
  std::set<std::string> callees_locked, callees_unlocked;  // callee names called inside / outside a lock region
  // A counted for-loop: i = c0; i <op> bound; i += step.
  bool counted = false;
  const VarDecl* induction = nullptr;
  int64_t c0 = 0, bound = 0, step = 0;
  BinaryOperatorKind cmp = BO_LT;
  std::string bound_text;
  bool has_cond = false;
  std::string delay_call, lock_call;
  bool delay_direct = false;  // a delay call in this loop's own body (not in an inner loop)
  bool is_for = false;
  size_t body_guard_base = 0;  // guards_.size() when the body starts (after the loop condition)
  std::vector<ExprRef> triggers;  // per exit (parallel to `exits`): the condition that decides it
  std::vector<Pos> exit_pos;      // per exit: where it is (the loop itself for the loop condition)
  std::vector<uint8_t> exit_kind; // per exit: KZ_LOOPEXIT_KIND (the loop condition, break, return, goto)
  bool cond_exit = false;         // exits[0] is the negated loop condition
  const VarDecl* budget_var = nullptr;
  // A table walk: the element variable, and the walk found before the body is read.
  const VarDecl* walk_var = nullptr;
  bool walk_found = false;
  Model::WalkSpec walk;
};

// A definition of a local variable inside a loop, for the counter and cursor patterns of
// SPEC 4.3 (form (b): `cnt := 0; for i in [0,N): if phi(i) then cnt++; exit iff cnt = 0`).
struct LocalEvent {
  enum Kind { Reset0, Inc1, Other } kind = Other;
  const VarDecl* var = nullptr;
  std::vector<size_t> loops;  // Model::loops indices of the enclosing loops, outermost first
  size_t guard_depth = 0;     // guards_.size() at the event
  ExprRef guard_inner = nullptr;  // the guards from the innermost loop's body down to the event
  const Expr* rhs = nullptr;  // Other: the value assigned, if a plain assignment
  Pos pos;
};

// What a finished inner loop leaves for the counter pattern of its enclosing loop.
struct InnerInfo {
  const VarDecl* induction = nullptr;
  size_t body_guard_base = 0;
};

// The control-flow graph of a body as the extended facts read it (RETURNS: reaching definitions):
// Clang's CFG with every expression an element, and where each statement is in it.
struct CfgIndex {
  std::unique_ptr<CFG> cfg;
  std::vector<const CFGBlock*> blocks;                             // by block id
  std::map<const Stmt*, std::pair<unsigned, unsigned>> at;          // element -> (block id, index), lookup only
};

class FnWalker {
 public:
  FnWalker(TuReader& tu, const FunctionDecl* fd)
      : tu_(tu), m_(tu.model()), spec_(tu.spec()), ctx_(tu.ctx()), fd_(fd), sm_(tu.ctx().getSourceManager()) {}

  void run() {
    FuncFact& ff = tu_.ensure_func(fd_);
    fkey_ = ff.key;
    fname_ = ff.name;
    summary_.params.resize(fd_->getNumParams());
    for (unsigned i = 0; i < fd_->getNumParams(); ++i) summary_.params[i].is_pointer = fd_->getParamDecl(i)->getType()->isPointerType();
    LocalPre pre(&locals_, &tested_);
    pre.TraverseStmt(const_cast<Stmt*>(fd_->getBody()));
    assign_local_ids();
    ext_ = spec_.extended_facts;
    visit(fd_->getBody());
    m_.summaries[fkey_] = summary_;
    if (spec_.collect_wrapper_inputs) record_cfg();
    if (ext_) emit_returns();
    if (ext_) finish_flow_facts();
    if (ext_) record_lock_cfg();
  }

 private:
  // ---- local variables: one identity per declaration ----
  // Expression trees name a local by a string.  A name that two locals of the function share
  // (shadowing, reuse in sibling blocks) is made unique with the position of the declaration,
  // "n@line:col"; a name used once stays as written.  The id depends only on the declarations,
  // never on pointer values, and every lookup from a tree back to a declaration goes through
  // local_by_id_.
  void assign_local_ids() {
    std::map<std::string, uint32_t> uses;
    // the declarations in source order (the raw encoding of a location is its offset in the
    // unit, the same in every parse of the same input), not in pointer order
    std::vector<const VarDecl*> decls;
    for (const auto& kv : locals_) {
      decls.push_back(kv.first);
      ++uses[kv.first->getNameAsString()];
    }
    std::sort(decls.begin(), decls.end(), [](const VarDecl* a, const VarDecl* b) {
      return a->getLocation().getRawEncoding() < b->getLocation().getRawEncoding();
    });
    for (const VarDecl* vd : decls) {
      std::string id = vd->getNameAsString();
      if (uses[id] > 1) {
        const Pos p = tu_.pos_of(vd->getLocation());
        id += "@" + std::to_string(p.line) + ":" + std::to_string(p.col);
      }
      // two declarations at one position (one macro expansion): number them in source order
      const std::string base = id;
      for (uint32_t k = 2; local_by_id_.count(id) != 0; ++k) id = base + "#" + std::to_string(k);
      local_id_[vd] = id;
      local_by_id_[id] = vd;
    }
  }
  std::string lid(const VarDecl* vd) const {
    auto it = local_id_.find(vd);
    return it != local_id_.end() ? it->second : vd->getNameAsString();
  }

  // ---- the profile-free mode: the control-flow graph of the body (wrappers.hpp) ----
  // Clang's CFG with every expression a block element, so that each call is one element and the
  // calls of a block are in evaluation order.  A body the CFG builder does not take is left out:
  // the derivation then treats the function as one it cannot decide on every path.
  void record_cfg() {
    CFG::BuildOptions bo;
    bo.setAllAlwaysAdd();
    std::unique_ptr<CFG> cfg = CFG::buildCFG(fd_, const_cast<Stmt*>(fd_->getBody()), &ctx_, bo);
    if (!cfg) return;
    FuncCfg fc;
    fc.blocks.resize(cfg->getNumBlockIDs());
    fc.entry = cfg->getEntry().getBlockID();
    fc.exit = cfg->getExit().getBlockID();
    for (const CFGBlock* b : *cfg) {
      if (b == nullptr || b->getBlockID() >= fc.blocks.size()) continue;
      CfgBlock& out = fc.blocks[b->getBlockID()];
      out.noreturn = b->hasNoReturnElement();
      for (const CFGElement& el : *b) {
        const std::optional<CFGStmt> cs = el.getAs<CFGStmt>();
        if (!cs) continue;
        const auto* ce = dyn_cast<CallExpr>(cs->getStmt());
        if (ce == nullptr) continue;
        auto it = call_exprs_.find(ce);
        if (it != call_exprs_.end()) out.calls.push_back(it->second);
      }
      for (const CFGBlock::AdjacentBlock& a : b->succs())
        if (const CFGBlock* r = a.getReachableBlock()) out.succ.push_back(r->getBlockID());
    }
    m_.cfgs[fkey_] = std::move(fc);
  }
  // An argument that is an integer or a data pointer converted to a function pointer (a symbol
  // address looked up at run time has this shape); a function, or a function pointer passed on, is not.
  static bool fp_from_data(const Expr* e) {
    e = e->IgnoreParens();
    while (const auto* c = dyn_cast<CastExpr>(e)) {
      const Expr* sub = c->getSubExpr()->IgnoreParens();
      const QualType st = sub->getType();
      if (!st.isNull() && !is_fp_type(st)) return true;
      e = sub;
    }
    return false;
  }

  // ---- small helpers ----
  ExprPool& pool() { return m_.exprs; }

  const Stmt* parent_stmt(const Stmt* s) {
    for (const DynTypedNode& n : ctx_.getParents(*s))
      if (const Stmt* p = n.get<Stmt>()) return p;
    return nullptr;
  }
  const VarDecl* parent_var(const Stmt* s) {
    for (const DynTypedNode& n : ctx_.getParents(*s))
      if (const VarDecl* p = n.get<VarDecl>()) return p;
    return nullptr;
  }
  // Climbs over parentheses and implicit casts; returns the outermost expression of that chain.
  const Stmt* chain_top(const Stmt* s) {
    for (;;) {
      const Stmt* p = parent_stmt(s);
      if (p == nullptr) return s;
      if (isa<ParenExpr>(p)) {
        s = p;
        continue;
      }
      if (const auto* ic = dyn_cast<ImplicitCastExpr>(p)) {
        s = ic;
        continue;
      }
      return s;
    }
  }
  bool in_range(SourceLocation loc, SourceRange r) {
    const SourceLocation l = sm_.getExpansionLoc(loc), b = sm_.getExpansionLoc(r.getBegin()), e = sm_.getExpansionLoc(r.getEnd());
    return !sm_.isBeforeInTranslationUnit(l, b) && !sm_.isBeforeInTranslationUnit(e, l);
  }
  bool is_null_const(const Expr* e) { return e->isNullPointerConstant(ctx_, Expr::NPC_ValueDependentIsNotNull) != Expr::NPCK_NotNull; }

  // ---- expression trees ----
  ExprRef to_expr(const Expr* e, int depth = 0) {
    if (e == nullptr) return pool().top("null");
    if (depth > 24) return pool().top("too deep");
    e = e->IgnoreParens();
    int64_t c = 0;
    if (!isa<CallExpr>(e) && tu_.eval_int(e->IgnoreParenCasts(), &c)) return pool().constant(c);
    if (const auto* ic = dyn_cast<ImplicitCastExpr>(e)) return to_expr(ic->getSubExpr(), depth);
    if (const auto* cc = dyn_cast<CStyleCastExpr>(e)) return to_expr(cc->getSubExpr(), depth);
    if (const auto* dre = dyn_cast<DeclRefExpr>(e)) {
      if (elem_var_ != nullptr && dre->getDecl() == elem_var_) return pool().load(KZ_EXPR_REF_ELEM, "");
      if (const auto* pv = dyn_cast<ParmVarDecl>(dre->getDecl())) return pool().load(KZ_EXPR_REF_PARAM, pv->getNameAsString(), pv->getFunctionScopeIndex());
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl()))
        return vd->hasGlobalStorage() ? pool().load(KZ_EXPR_REF_GLOBAL, vd->getNameAsString()) : pool().load(KZ_EXPR_REF_LOCAL, lid(vd));
      if (const auto* fn = dyn_cast<FunctionDecl>(dre->getDecl())) return pool().load(KZ_EXPR_REF_FUNC, fn->getNameAsString());
      return pool().top("declref");
    }
    if (const auto* me = dyn_cast<MemberExpr>(e)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) {
        if (is_elem_member(me)) return pool().load(KZ_EXPR_REF_ELEM, fl->getNameAsString());
        return pool().load(KZ_EXPR_REF_FIELD, field_type_name(fl) + "." + fl->getNameAsString());
      }
      return pool().top("member");
    }
    if (const auto* sl = dyn_cast<StringLiteral>(e)) return pool().load(KZ_EXPR_REF_STRING, sl->getString().str());
    if (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) return pool().binary(KZ_EXPR_OP_INDEX, to_expr(as->getBase(), depth + 1), to_expr(as->getIdx(), depth + 1));
    if (const auto* uo = dyn_cast<UnaryOperator>(e)) {
      switch (uo->getOpcode()) {
        case UO_LNot: return pool().unary(KZ_EXPR_OP_NOT, to_expr(uo->getSubExpr(), depth + 1));
        case UO_Minus: return pool().unary(KZ_EXPR_OP_NEG, to_expr(uo->getSubExpr(), depth + 1));
        case UO_Not: return pool().unary(KZ_EXPR_OP_BITNOT, to_expr(uo->getSubExpr(), depth + 1));
        case UO_Deref: return pool().unary(KZ_EXPR_OP_DEREF, to_expr(uo->getSubExpr(), depth + 1));
        case UO_AddrOf: return pool().unary(KZ_EXPR_OP_ADDR, to_expr(uo->getSubExpr(), depth + 1));
        case UO_Plus: return to_expr(uo->getSubExpr(), depth);
        default: return pool().unary(KZ_EXPR_OP_STORE, to_expr(uo->getSubExpr(), depth + 1));  // ++ and --: a store, then the value
      }
    }
    if (const auto* bo = dyn_cast<BinaryOperator>(e)) {
      uint16_t op = KZ_EXPR_OP_TOP;
      switch (bo->getOpcode()) {
        case BO_LAnd: op = KZ_EXPR_OP_AND; break;
        case BO_LOr: op = KZ_EXPR_OP_OR; break;
        case BO_EQ: op = KZ_EXPR_OP_EQ; break;
        case BO_NE: op = KZ_EXPR_OP_NE; break;
        case BO_LT: op = KZ_EXPR_OP_LT; break;
        case BO_LE: op = KZ_EXPR_OP_LE; break;
        case BO_GT: op = KZ_EXPR_OP_GT; break;
        case BO_GE: op = KZ_EXPR_OP_GE; break;
        case BO_Add: op = KZ_EXPR_OP_ADD; break;
        case BO_Sub: op = KZ_EXPR_OP_SUB; break;
        case BO_Mul: op = KZ_EXPR_OP_MUL; break;
        case BO_Div: op = KZ_EXPR_OP_DIV; break;
        case BO_Rem: op = KZ_EXPR_OP_MOD; break;
        case BO_And: op = KZ_EXPR_OP_BITAND; break;
        case BO_Or: op = KZ_EXPR_OP_BITOR; break;
        case BO_Xor: op = KZ_EXPR_OP_BITXOR; break;
        case BO_Shl: op = KZ_EXPR_OP_SHL; break;
        case BO_Shr: op = KZ_EXPR_OP_SHR; break;
        case BO_Comma: return to_expr(bo->getRHS(), depth + 1);
        default: break;
      }
      if (bo->isAssignmentOp()) return pool().binary(KZ_EXPR_OP_STORE, to_expr(bo->getLHS(), depth + 1), to_expr(bo->getRHS(), depth + 1));
      if (op == KZ_EXPR_OP_TOP) return pool().top("binary");
      return pool().binary(op, to_expr(bo->getLHS(), depth + 1), to_expr(bo->getRHS(), depth + 1));
    }
    if (const auto* co = dyn_cast<ConditionalOperator>(e))
      return pool().binary(KZ_EXPR_OP_COND, to_expr(co->getCond(), depth + 1),
                           pool().binary(KZ_EXPR_OP_SEQ, to_expr(co->getTrueExpr(), depth + 1), to_expr(co->getFalseExpr(), depth + 1)));
    if (const auto* ce = dyn_cast<CallExpr>(e)) {
      const FunctionDecl* callee = ce->getDirectCallee();
      std::vector<ExprRef> args;
      for (const Expr* a : ce->arguments()) args.push_back(to_expr(a, depth + 1));
      return pool().call_result(callee != nullptr ? callee->getNameAsString() : "<indirect>", pool().list(KZ_EXPR_OP_SEQ, args));
    }
    return pool().top("expression");
  }

  // `e->f` where e is the element variable of the table walk being expressed (elem_var_).
  bool is_elem_member(const MemberExpr* me) const {
    if (elem_var_ == nullptr || !me->isArrow()) return false;
    const auto* dre = dyn_cast<DeclRefExpr>(me->getBase()->IgnoreParenImpCasts());
    return dre != nullptr && dre->getDecl() == elem_var_;
  }
  // Does `e` read the element variable `v`?
  static bool mentions_var(const Expr* e, const VarDecl* v) {
    if (e == nullptr) return false;
    struct V : RecursiveASTVisitor<V> {
      const VarDecl* var = nullptr;
      bool found = false;
      bool VisitDeclRefExpr(DeclRefExpr* d) {
        if (d->getDecl() == var) found = true;
        return !found;
      }
    } vis;
    vis.var = v;
    vis.TraverseStmt(const_cast<Expr*>(e));
    return vis.found;
  }

  // The condition `c` with the polarity it holds under.
  ExprRef cond_expr(const Expr* c, bool polarity) {
    const auto key = std::make_tuple(c, polarity, elem_var_);
    auto it = cond_cache_.find(key);
    if (it != cond_cache_.end()) return it->second;
    ExprRef e = to_expr(c);
    // A pointer or an integer used as a condition means "!= 0".
    if (e != nullptr && e->op != KZ_EXPR_OP_EQ && e->op != KZ_EXPR_OP_NE && e->op != KZ_EXPR_OP_LT && e->op != KZ_EXPR_OP_LE &&
        e->op != KZ_EXPR_OP_GT && e->op != KZ_EXPR_OP_GE && e->op != KZ_EXPR_OP_AND && e->op != KZ_EXPR_OP_OR && e->op != KZ_EXPR_OP_NOT)
      e = pool().binary(KZ_EXPR_OP_NE, e, pool().constant(0));
    if (!polarity) e = pool().unary(KZ_EXPR_OP_NOT, e);
    cond_cache_[key] = e;
    return e;
  }
  ExprRef guard_from(size_t base) {
    std::vector<ExprRef> terms;
    for (size_t i = base; i < guards_.size(); ++i) terms.push_back(cond_expr(guards_[i].cond, guards_[i].polarity));
    return pool().conj(KZ_EXPR_OP_AND, terms);
  }

  // ---- writers ----
  uint32_t current_flags(const Stmt* at) {
    uint32_t f = 0;
    if (!loops_.empty()) f |= KZ_WRITER_FLAG_IN_LOOP;
    if (at->getBeginLoc().isMacroID()) f |= KZ_WRITER_FLAG_IN_MACRO;
    return f;
  }

  void add_writer(size_t tgt, const Stmt* anchor, uint8_t kind, const ValueInfo& v, const std::string& key, uint32_t flags) {
    WriterFact w;
    w.loc_type = spec_.fields[tgt].field.type;
    w.loc_field = spec_.fields[tgt].field.field;
    w.pos = tu_.pos_of(anchor->getBeginLoc());
    tu_.next_key(w.pos, fkey_ + "#w", &w.ordinal);
    w.tu = tu_.main_file();
    w.func_key = fkey_;
    w.func_name = fname_;
    w.kind = kind;
    w.value_kind = v.kind;
    w.value = v.value;
    w.value_text = v.text;
    w.key = key;
    w.guard = guard_from(0);
    w.flags = flags | current_flags(anchor);
    m_.writers.push_back(w);
  }

  // ---- A-TYPE listing ----
  void add_atype(uint8_t kind, size_t tgt, const Stmt* at, const std::string& detail, const std::string& call_key = "", const std::string& cause_field = "") {
    ATypeFact a;
    a.kind = kind;
    a.type_name = spec_.fields[tgt].field.type;
    a.cause_field = cause_field;
    a.pos = tu_.pos_of(at->getBeginLoc());
    tu_.next_key(a.pos, fkey_ + "#a", &a.ordinal);
    a.func_key = fkey_;
    a.tu = tu_.main_file();
    a.detail = detail;
    a.call_key = call_key;
    m_.atypes.push_back(a);
  }

  // ---- FUNC_WRITES: what the function's own body writes ----
  void note_write(uint8_t kind, const std::string& type, const std::string& field, const Stmt* at) {
    WriteEvent ev;
    ev.func_key = fkey_;
    ev.kind = kind;
    ev.type = type;
    ev.field = field;
    ev.pos = tu_.pos_of(at->getBeginLoc());
    if (!loops_.empty()) ev.loop_key = m_.loops[loops_.back()->index].key;
    if (ext_) {
      ext_writes_.push_back(ExtWrite{at, type, field, kind, ev.pos, static_cast<uint32_t>(m_.write_sites.size())});
      m_.write_sites.push_back(WriteSite{fkey_, kind, type, field, ev.pos});
    }
    m_.write_events.push_back(ev);
  }
  // Every record type a value of type `t` holds by value (itself, nested records, array elements).
  void contained_records(QualType t, std::set<const RecordDecl*>* seen, std::vector<std::string>* out) {
    const RecordDecl* rd = tu_.record_of(t);
    if (rd == nullptr) return;
    if (const RecordDecl* def = rd->getDefinition()) rd = def;
    if (!seen->insert(rd).second) return;
    out->push_back(tu_.record_name(rd));
    for (const FieldDecl* fd : rd->fields()) contained_records(fd->getType(), seen, out);
  }
  void note_record_write(QualType t, const Stmt* at) {
    std::set<const RecordDecl*> seen;
    std::vector<std::string> names;
    contained_records(t, &seen, &names);
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const std::string& n : names) note_write(KZ_FWRITE_KIND_RECORD, n, "*", at);
  }
  void note_global_write(const DeclRefExpr* dre, const Stmt* at) {
    const auto* vd = dyn_cast<VarDecl>(dre->getDecl());
    if (vd == nullptr || !vd->hasGlobalStorage()) return;
    note_write(KZ_FWRITE_KIND_FIELD, "<global>", vd->getNameAsString(), at);
    if (vd->getType()->isRecordType() || vd->getType()->isArrayType()) note_record_write(vd->getType(), at);
  }

  // The name under which a field's owner type is reported: the spelling the targets use when
  // the field is a target, else the type's own name.
  std::string field_type_name(const FieldDecl* fl) {
    const std::vector<size_t> t = tu_.targets_of_field(fl);
    return t.empty() ? tu_.record_name(fl->getParent()) : spec_.fields[t.front()].field.type;
  }

  // ---- the statement walker ----
  void visit(const Stmt* s) {
    if (s == nullptr) return;
    switch (s->getStmtClass()) {
      case Stmt::IfStmtClass: return visit_if(cast<IfStmt>(s));
      case Stmt::WhileStmtClass: return visit_while(cast<WhileStmt>(s));
      case Stmt::DoStmtClass: return visit_do(cast<DoStmt>(s));
      case Stmt::ForStmtClass: return visit_for(cast<ForStmt>(s));
      case Stmt::SwitchStmtClass: return visit_switch(cast<SwitchStmt>(s));
      case Stmt::BreakStmtClass: return visit_break(cast<BreakStmt>(s));
      case Stmt::ReturnStmtClass: return visit_return(cast<ReturnStmt>(s));
      case Stmt::GotoStmtClass:
      case Stmt::IndirectGotoStmtClass: return visit_goto(s);
      case Stmt::ConditionalOperatorClass: return visit_conditional(cast<ConditionalOperator>(s));
      case Stmt::BinaryOperatorClass:
      case Stmt::CompoundAssignOperatorClass: return visit_binary(cast<BinaryOperator>(s));
      case Stmt::UnaryOperatorClass: return visit_unary(cast<UnaryOperator>(s));
      case Stmt::CallExprClass: return visit_call(cast<CallExpr>(s));
      case Stmt::MemberExprClass: return visit_member(cast<MemberExpr>(s));
      case Stmt::ImplicitCastExprClass:
      case Stmt::CStyleCastExprClass: return visit_cast(cast<CastExpr>(s));
      case Stmt::DeclRefExprClass: return visit_declref(cast<DeclRefExpr>(s));
      case Stmt::DeclStmtClass: return visit_declstmt(cast<DeclStmt>(s));
      case Stmt::InitListExprClass: return visit_initlist(cast<InitListExpr>(s));
      case Stmt::ArraySubscriptExprClass:
        if (ext_) note_param_subscript(cast<ArraySubscriptExpr>(s));
        break;
      case Stmt::UnaryExprOrTypeTraitExprClass: return;  // sizeof and alignof do not evaluate their operand
      case Stmt::CompoundStmtClass: return visit_compound(cast<CompoundStmt>(s));
      case Stmt::GCCAsmStmtClass:
      case Stmt::MSAsmStmtClass: return visit_asm(cast<AsmStmt>(s));
      default: break;
    }
    for (const Stmt* c : s->children()) visit(c);
  }

  // A statement that never falls through to the next one: a jump, or a block or an if/else whose
  // every path ends in one.  (Calls to terminal operations are not counted: T-TERM is a rule of
  // the skeleton, not of the extracted guards.)
  static bool always_leaves(const Stmt* s) {
    if (s == nullptr) return false;
    if (isa<BreakStmt>(s) || isa<ContinueStmt>(s) || isa<ReturnStmt>(s) || isa<GotoStmt>(s) || isa<IndirectGotoStmt>(s)) return true;
    if (const auto* cs = dyn_cast<CompoundStmt>(s)) return !cs->body_empty() && always_leaves(cs->body_back());
    if (const auto* is = dyn_cast<IfStmt>(s)) return is->getElse() != nullptr && always_leaves(is->getThen()) && always_leaves(is->getElse());
    return false;
  }

  // A block.  After `if (c) <leave>;` without an else, the rest of the block runs only when c was
  // false at the test: that is pushed as a guard until the block ends.  A label (or a case label)
  // is a join point: the guards of the early exits before it are dropped there.
  void visit_compound(const CompoundStmt* cs) {
    const size_t base = guards_.size();
    for (const Stmt* st : cs->body()) {
      if (isa<SwitchCase>(st) || isa<LabelStmt>(st))
        while (guards_.size() > base) guards_.pop_back();
      visit(st);
      if (const auto* is = dyn_cast<IfStmt>(st))
        if (is->getElse() == nullptr && is->getConditionVariable() == nullptr && is->getInit() == nullptr && always_leaves(is->getThen()))
          guards_.push_back({is->getCond(), false});
    }
    while (guards_.size() > base) guards_.pop_back();
  }

  // ---- control constructs ----
  void visit_if(const IfStmt* s) {
    visit(s->getInit());
    if (ext_) note_param_truth(s->getCond(), 0);
    visit(s->getCond());
    guards_.push_back({s->getCond(), true});
    visit(s->getThen());
    guards_.pop_back();
    if (s->getElse() != nullptr) {
      guards_.push_back({s->getCond(), false});
      visit(s->getElse());
      guards_.pop_back();
    }
  }
  void visit_conditional(const ConditionalOperator* s) {
    if (ext_) note_param_truth(s->getCond(), 0);
    visit(s->getCond());
    guards_.push_back({s->getCond(), true});
    visit(s->getTrueExpr());
    guards_.pop_back();
    guards_.push_back({s->getCond(), false});
    visit(s->getFalseExpr());
    guards_.pop_back();
  }
  void visit_switch(const SwitchStmt* s) {
    if (ext_) note_param_truth(s->getCond(), KZ_PFLOW_FLAG_SWITCH);
    visit(s->getCond());
    breakables_.push_back(nullptr);
    visit(s->getBody());
    breakables_.pop_back();
  }

  // Loops --------------------------------------------------------------------------------------------
  LoopCtx* begin_loop(const Stmt* s, const Expr* cond, bool is_for, bool cond_first) {
    LoopFact lf;
    lf.pos = tu_.pos_of(s->getBeginLoc());
    lf.key = lf.pos.str();
    lf.tu = tu_.main_file();
    lf.func_key = fkey_;
    lf.func_name = fname_;
    lf.depth = static_cast<uint16_t>(loops_.size() + 1);
    if (!loops_.empty()) lf.parent_key = m_.loops[loops_.back()->index].key;
    m_.loops.push_back(lf);
    auto ctx = std::make_unique<LoopCtx>();
    ctx->index = m_.loops.size() - 1;
    ctx->stmt = s;
    ctx->range = s->getSourceRange();
    ctx->guard_base = guards_.size();
    ctx->body_guard_base = guards_.size();
    ctx->is_for = is_for;
    ctx->has_cond = cond != nullptr;
    if (cond != nullptr && cond_first) {
      ctx->exits.push_back(cond_expr(cond, false));
      ctx->triggers.push_back(ctx->exits.back());
      ctx->exit_pos.push_back(lf.pos);
      ctx->exit_kind.push_back(KZ_LOOPEXIT_KIND_COND);
      ctx->cond_exit = true;
    }
    loops_.push_back(std::move(ctx));
    breakables_.push_back(loops_.back().get());
    return loops_.back().get();
  }

  void end_loop(LoopCtx* lc, const Stmt* body) {
    (void)body;
    LoopFact& lf = m_.loops[lc->index];
    lf.n_exits = lc->n_exits;
    lf.n_calls = lc->n_calls;
    lf.delay_call_key = lc->delay_call;
    lf.lock_call_key = lc->lock_call;
    lf.delay_direct = lc->delay_direct;
    // lock kind: all shared loads of the body inside one lock region, or a single atomic load
    if (lc->shared_loads > 0 && lc->shared_loads == lc->shared_loads_locked && lc->lock_calls == 1) lf.lock_kind = KZ_LOCK_KIND_ONE_REGION;
    else if (lc->shared_loads == 1 && lc->shared_loads_locked == 0 && lc->volatile_load) lf.lock_kind = KZ_LOCK_KIND_ATOMIC_READ;
    else lf.lock_kind = KZ_LOCK_KIND_NONE;
    // trip count of a counted loop
    bool trip_known = false;
    if (lc->counted && lc->induction != nullptr && lc->step != 0) {
      const bool modified = lc->other_mods.count(lc->induction) != 0 || locals_[lc->induction].addr_taken;
      if (!modified) {
        int64_t trip = 0;
        const int64_t span = lc->bound - lc->c0;
        const int64_t st = lc->step;
        switch (lc->cmp) {
          case BO_LT: trip = (st > 0 && span > 0) ? (span + st - 1) / st : 0; trip_known = st > 0; break;
          case BO_LE: trip = (st > 0 && span >= 0) ? span / st + 1 : 0; trip_known = st > 0; break;
          case BO_GT: trip = (st < 0 && -span > 0) ? (-span + (-st) - 1) / (-st) : 0; trip_known = st < 0; break;
          case BO_GE: trip = (st < 0 && -span >= 0) ? (-span) / (-st) + 1 : 0; trip_known = st < 0; break;
          case BO_NE:
            trip_known = span != 0 && st != 0 && span % st == 0 && (span / st) > 0;
            trip = trip_known ? span / st : 0;
            break;
          default: break;
        }
        if (trip_known) {
          lf.trip = trip;
          lf.trip_status = lc->n_exits > 0 ? KZ_TRIP_STATUS_UPPER_BOUND : KZ_TRIP_STATUS_EXACT;
          if (ext_) {  // F4: the values the induction variable takes (PTR_FIELDS)
            lf.induction = lid(lc->induction);
            lf.c0 = lc->c0;
            lf.step = lc->step;
          }
          ConstFact cf;
          cf.name = lc->bound_text;
          cf.pos = lf.pos;
          cf.tu = tu_.main_file();
          cf.value = lc->bound;
          cf.kind = KZ_CONST_KIND_INT;
          if (!lc->bound_text.empty()) m_.consts.push_back(cf);
        }
      }
    }
    if (!trip_known && lc->has_cond) lf.trip_status = KZ_TRIP_STATUS_UNKNOWN;
    // table walk?  (found before the body was read, so that the calls in it could be recorded
    // in element form)
    const bool walk = lc->walk_found;
    if (walk) {
      m_.walks.push_back(lc->walk);
      lf.table = lc->walk.text;
      lf.table_root = lc->walk.root_object;
    }
    // A loop is a poll or a retry loop only if the delay call is in its own body; a loop that
    // contains a polling loop is classified by its own shape.
    if (walk) {
      lf.kind = KZ_LOOP_KIND_TABLE_WALK;
    } else if (lc->delay_direct) {
      lf.kind = trip_known ? KZ_LOOP_KIND_COUNT_BOUNDED : KZ_LOOP_KIND_POLL;
    } else if (trip_known) {
      lf.kind = KZ_LOOP_KIND_CONST_TRIP;
    } else {
      lf.kind = KZ_LOOP_KIND_OTHER;
    }
    // trees: for loops that wait, and for table walks
    if (!lc->delay_call.empty() || walk) {
      lf.exit_expr = pool().conj(KZ_EXPR_OP_OR, lc->exits);
      lf.modset = loop_modset(lc);
      lf.budget = build_budget(lc);
    }
    if (lf.kind == KZ_LOOP_KIND_POLL || lf.kind == KZ_LOOP_KIND_COUNT_BOUNDED) extract_predicate(lc, &lf);
    restrict_lock_kind(lc, &lf);
    // F3 (LOOP_EXITS): the same trees for every loop, and each exit on its own
    if (ext_) {
      lf.has_exits = true;
      lf.exits_as_in_loops = !lc->delay_call.empty() || walk;
      lf.all_exit = pool().conj(KZ_EXPR_OP_OR, lc->exits);
      lf.all_modset = loop_modset(lc);
      for (size_t i = 0; i < lc->exits.size(); ++i) {
        LoopFact::Exit x;
        x.kind = i < lc->exit_kind.size() ? lc->exit_kind[i] : static_cast<uint8_t>(KZ_LOOPEXIT_KIND_NONE);
        x.pos = lc->exit_pos[i];
        x.when = lc->exits[i];
        x.trigger = i < lc->triggers.size() ? lc->triggers[i] : nullptr;
        if (x.when != nullptr && x.when->op == KZ_EXPR_OP_CONST && x.when->a == nullptr && x.when->value == 0) x.flags |= KZ_LOOPEXIT_FLAG_NEVER;
        if (x.trigger == nullptr) x.flags |= KZ_LOOPEXIT_FLAG_UNCONDITIONAL;
        lf.exits.push_back(x);
      }
    }
  }

  // The mod set of a loop body: the fields, globals and locals it writes and the functions it calls.
  ExprRef loop_modset(const LoopCtx* lc) {
    std::vector<ExprRef> mods;
    for (const std::string& n : lc->mod_names) {
      const size_t colon = n.find(':');
      const std::string cls = n.substr(0, colon), nm = n.substr(colon + 1);
      mods.push_back(pool().load(cls == "field" ? KZ_EXPR_REF_FIELD : cls == "global" ? KZ_EXPR_REF_GLOBAL : KZ_EXPR_REF_LOCAL, nm));
    }
    for (const std::string& n : lc->callees) mods.push_back(pool().call_result(n, nullptr));
    return pool().list(KZ_EXPR_OP_LOCSET, mods);
  }

  // L1 (ii) sees the loads of the loop body.  A call result in the test reads what the callee
  // reads, which the load count does not see: an atomic read is then not the whole test, and one
  // lock region covers the test only if every such call is made inside the region (and nowhere
  // outside it in the loop).  An indirect call result is never covered.
  void restrict_lock_kind(const LoopCtx* lc, LoopFact* lf) {
    if (lf->lock_kind == KZ_LOCK_KIND_NONE) return;
    std::set<std::string> called;
    collect_call_results(lf->pred, &called);
    collect_call_results(lf->cond, &called);
    if (called.empty()) return;
    if (lf->lock_kind == KZ_LOCK_KIND_ATOMIC_READ) {
      lf->lock_kind = KZ_LOCK_KIND_NONE;
      return;
    }
    for (const std::string& f : called)
      if (f == "<indirect>" || lc->callees_locked.count(f) == 0 || lc->callees_unlocked.count(f) != 0) {
        lf->lock_kind = KZ_LOCK_KIND_NONE;
        return;
      }
  }
  static void collect_call_results(ExprRef e, std::set<std::string>* out) {
    if (e == nullptr) return;
    if (e->op == KZ_EXPR_OP_CALL_RESULT) out->insert(e->name);
    collect_call_results(e->a, out);
    collect_call_results(e->b, out);
  }

  // ---- SPEC 4.3: the condition exits of a poll loop and the predicate they test -------------
  //
  // The exits are split by what decides them (the innermost guard at the exit, or the negated
  // loop condition): an exit decided by a test of the budget variable is a budget exit, every
  // other exit is a condition exit.  The condition exits, with the conjuncts that only test the
  // budget removed, give `cond`.  The predicate is `cond` with every counter-zero test of form
  // (b) rewritten as a `forall` over the constant-trip loop that counts; it is a direct test
  // (form (a)) when it reads no local variable at all.
  void extract_predicate(LoopCtx* lc, LoopFact* lf) {
    const std::string budget = lc->budget_var != nullptr ? lid(lc->budget_var) : std::string();
    std::vector<ExprRef> cond_terms;
    std::vector<size_t> cond_index;  // the exit each condition term comes from
    bool any_exit = false;
    for (size_t i = 0; i < lc->exits.size(); ++i) {
      ExprRef full = lc->exits[i];
      if (full != nullptr && full->op == KZ_EXPR_OP_CONST && full->a == nullptr && full->value == 0) continue;  // never taken (`while (1)`)
      any_exit = true;
      ExprRef trig = i < lc->triggers.size() ? lc->triggers[i] : nullptr;
      ExprRef term = full;
      if (!budget.empty() && trig != nullptr && expr_mentions_local(trig, budget)) {
        // The deciding test mentions the budget: its disjuncts that do not are condition exits
        // (`if (ready || left <= 0) break;`, `while (!ready && left > 0)`).
        std::vector<ExprRef> ds, keep;
        disjuncts(trig, &ds);
        for (ExprRef d : ds)
          if (!expr_mentions_local(d, budget)) keep.push_back(d);
        if (keep.empty()) continue;  // a budget exit
        ExprRef rest = nullptr;  // the other guards of the exit: `full` is (and rest trig) or trig
        if (full != trig && full != nullptr && full->op == KZ_EXPR_OP_AND && full->b == trig) rest = full->a;
        else if (full != trig) rest = full;
        term = pool().conj(KZ_EXPR_OP_AND, {rest, pool().conj(KZ_EXPR_OP_OR, keep)});
      }
      if (!budget.empty()) term = drop_conjuncts_on(term, budget);
      if (term == nullptr) term = pool().constant(1);  // the exit is taken whatever the condition
      cond_terms.push_back(term);
      cond_index.push_back(i);
    }
    if (cond_terms.empty()) {
      lf->pred_note = any_exit ? "no exit other than the budget" : "the loop has no exit";
      return;
    }
    lf->cond = pool().conj(KZ_EXPR_OP_OR, cond_terms);
    ExprRef pred = lf->cond;
    std::set<std::string> locals;
    collect_locals(pred, &locals);
    bool rewrote = false;
    for (const std::string& name : locals) {
      const VarDecl* var = local_by_name(lc, name);
      std::string why;
      size_t inner = kNone;
      ExprRef forall = var != nullptr ? counter_forall(lc, var, &inner, &why) : nullptr;
      // every exit that tests the counter must come after the counting loop; the loop condition
      // is tested before the reset of the first iteration, so it must not read the counter
      for (size_t k = 0; forall != nullptr && k < cond_terms.size(); ++k) {
        if (!expr_mentions_local(cond_terms[k], name)) continue;
        const size_t ex = cond_index[k];
        if (lc->cond_exit && ex == 0) {
          why = "the loop condition reads it before the reset";
          forall = nullptr;
        } else if (!(m_.loops[inner].pos < lc->exit_pos[ex])) {
          why = "an exit at " + lc->exit_pos[ex].str() + " tests it before the counting loop";
          forall = nullptr;
        }
      }
      if (forall == nullptr) {
        lf->pred_note = "the condition reads the local " + name + ", which is not a recognised counter" + (why.empty() ? "" : ": " + why);
        return;
      }
      pred = substitute_counter(pred, name, forall);
      rewrote = true;
    }
    std::set<std::string> left;
    collect_free_locals(pred, &left);
    if (!left.empty()) {
      lf->pred_note = "the condition still reads the local " + *left.begin() + " after the rewriting";
      return;
    }
    lf->pred = pred;
    if (!rewrote) {
      lf->pred_form = KZ_PRED_FORM_DIRECT;
      lf->pred_note = "a direct test of shared loads, parameters and call results";
    } else if (pred->op == KZ_EXPR_OP_FORALL) {
      lf->pred_form = KZ_PRED_FORM_COUNTER_ZERO;
      lf->pred_note = "a counter-zero test after a constant-trip loop, rewritten as forall";
    } else {
      lf->pred_form = KZ_PRED_FORM_COMBINED;
      lf->pred_note = "a combination of direct tests and counter-zero tests";
    }
  }

  // The disjuncts of a condition: `or` is split, and `not (and a b)` is `(not a) or (not b)`.
  void disjuncts(ExprRef e, std::vector<ExprRef>* out) {
    if (e == nullptr) return;
    if (e->op == KZ_EXPR_OP_OR) {
      disjuncts(e->a, out);
      disjuncts(e->b, out);
      return;
    }
    if (e->op == KZ_EXPR_OP_NOT && e->a != nullptr && e->a->op == KZ_EXPR_OP_AND) {
      disjuncts(pool().unary(KZ_EXPR_OP_NOT, e->a->a), out);
      disjuncts(pool().unary(KZ_EXPR_OP_NOT, e->a->b), out);
      return;
    }
    out->push_back(e);
  }

  // The conjunction `e` without its conjuncts that mention the local `name`.
  ExprRef drop_conjuncts_on(ExprRef e, const std::string& name) {
    if (e == nullptr) return nullptr;
    if (e->op == KZ_EXPR_OP_AND) {
      ExprRef a = drop_conjuncts_on(e->a, name), b = drop_conjuncts_on(e->b, name);
      if (a == nullptr) return b;
      if (b == nullptr) return a;
      return pool().binary(KZ_EXPR_OP_AND, a, b);
    }
    return expr_mentions_local(e, name) ? nullptr : e;
  }

  static void collect_locals(ExprRef e, std::set<std::string>* out) {
    if (e == nullptr) return;
    if (e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_LOCAL) out->insert(e->name);
    collect_locals(e->a, out);
    collect_locals(e->b, out);
  }
  // The locals outside `forall` nodes: inside one, the induction variable and the cursors are
  // bound, and counter_forall has checked that no other local occurs.
  static void collect_free_locals(ExprRef e, std::set<std::string>* out) {
    if (e == nullptr || e->op == KZ_EXPR_OP_FORALL) return;
    if (e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_LOCAL) out->insert(e->name);
    collect_free_locals(e->a, out);
    collect_free_locals(e->b, out);
  }

  // The declaration a LOCAL load of an expression tree names (its id, see assign_local_ids).
  const VarDecl* local_by_name(const LoopCtx* lc, const std::string& id) {
    (void)lc;
    auto it = local_by_id_.find(id);
    return it != local_by_id_.end() ? it->second : nullptr;
  }

  // `cnt == 0` (and its spellings) replaced by `forall`.  Any other use of the counter stays and
  // makes the rewriting fail (the caller checks that no local is left).
  ExprRef substitute_counter(ExprRef e, const std::string& name, ExprRef forall) {
    if (e == nullptr) return nullptr;
    auto is_counter = [&](ExprRef x) { return x != nullptr && x->op == KZ_EXPR_OP_LOAD && x->ref == KZ_EXPR_REF_LOCAL && x->name == name; };
    auto is_zero = [](ExprRef x) { return x != nullptr && x->op == KZ_EXPR_OP_CONST && x->a == nullptr && x->value == 0; };
    if (e->op == KZ_EXPR_OP_EQ && ((is_counter(e->a) && is_zero(e->b)) || (is_zero(e->a) && is_counter(e->b)))) return forall;
    if (e->op == KZ_EXPR_OP_NE && ((is_counter(e->a) && is_zero(e->b)) || (is_zero(e->a) && is_counter(e->b)))) return pool().unary(KZ_EXPR_OP_NOT, forall);
    if (e->op == KZ_EXPR_OP_LOAD || e->op == KZ_EXPR_OP_CONST) return e;
    ExprRef a = substitute_counter(e->a, name, forall), b = substitute_counter(e->b, name, forall);
    if (a == e->a && b == e->b) return e;
    if (e->op == KZ_EXPR_OP_NOT) return pool().unary(KZ_EXPR_OP_NOT, a);
    if (e->op == KZ_EXPR_OP_AND || e->op == KZ_EXPR_OP_OR) return pool().binary(e->op, a, b);
    return pool().make(e->op, e->ref, e->value, e->name, a, b);
  }

  // Form (b) for the counter `var` of loop `lc`: one reset to 0 at the top level of the loop's
  // body, increments by one only inside one directly nested loop with an exact trip count and no
  // exit, and no other definition inside the loop.  Then `var == 0` after the inner loop means
  // that the increment's guard held for no iteration: forall i < N: not guard(i).  Locals the
  // guard reads must be the inner loop's induction variable or cursors: a pointer set once before
  // the inner loop and advanced by one element, unconditionally, in every iteration.
  ExprRef counter_forall(LoopCtx* lc, const VarDecl* var, size_t* inner_out, std::string* why) {
    const size_t L = lc->index;
    if (locals_[var].addr_taken) {
      *why = "its address is taken";
      return nullptr;
    }
    const LocalEvent* reset = nullptr;
    std::vector<const LocalEvent*> incs;
    for (const LocalEvent& ev : local_events_) {
      if (ev.var != var || std::find(ev.loops.begin(), ev.loops.end(), L) == ev.loops.end()) continue;
      if (ev.kind == LocalEvent::Reset0 && ev.loops.back() == L && ev.guard_depth == lc->body_guard_base && reset == nullptr) {
        reset = &ev;
        continue;
      }
      if (ev.kind == LocalEvent::Inc1 && ev.loops.size() >= 2 && ev.loops[ev.loops.size() - 2] == L) {
        incs.push_back(&ev);
        continue;
      }
      *why = "it is defined at " + ev.pos.str() + " otherwise than by one reset to 0 and increments in one inner loop";
      return nullptr;
    }
    if (reset == nullptr) {
      *why = "no unconditional reset to 0 at the top of the loop body";
      return nullptr;
    }
    if (incs.empty()) {
      *why = "no increment";
      return nullptr;
    }
    const size_t L2 = incs.front()->loops.back();
    for (const LocalEvent* ev : incs)
      if (ev->loops.back() != L2) {
        *why = "it is incremented in more than one inner loop";
        return nullptr;
      }
    const LoopFact& inner = m_.loops[L2];
    if (!(inner.kind == KZ_LOOP_KIND_CONST_TRIP && inner.trip_status == KZ_TRIP_STATUS_EXACT && inner.n_exits == 0)) {
      *why = "the inner loop at " + inner.pos.str() + " has no exact constant trip count";
      return nullptr;
    }
    if (inner.pos < reset->pos) {
      *why = "the reset comes after the inner loop";
      return nullptr;
    }
    auto it = inner_ctx_.find(L2);
    if (it == inner_ctx_.end()) {
      *why = "the inner loop was not recorded";
      return nullptr;
    }
    const InnerInfo& info = it->second;
    std::vector<ExprRef> phis;
    for (const LocalEvent* ev : incs) phis.push_back(ev->guard_inner != nullptr ? ev->guard_inner : pool().constant(1));
    ExprRef phi = pool().conj(KZ_EXPR_OP_OR, phis);
    // the locals of the guard: the induction variable, cursors
    std::set<std::string> names;
    collect_locals(phi, &names);
    std::vector<ExprRef> cursors;
    for (const std::string& n : names) {
      if (info.induction != nullptr && n == lid(info.induction)) continue;
      const VarDecl* p = local_by_name(lc, n);
      ExprRef cur = p != nullptr ? cursor_of(lc, L2, info, p) : nullptr;
      if (cur == nullptr) {
        *why = "the increment's guard reads the local " + n + ", which is neither the induction variable nor a cursor";
        return nullptr;
      }
      cursors.push_back(cur);
    }
    const std::string iv = info.induction != nullptr ? lid(info.induction) : std::string("i");
    *inner_out = L2;
    return pool().make(KZ_EXPR_OP_FORALL, 0, inner.trip, iv, pool().unary(KZ_EXPR_OP_NOT, phi), pool().list(KZ_EXPR_OP_SEQ, cursors));
  }

  // A cursor of inner loop L2 (inside loop lc): one plain assignment `p = base` at the top level
  // of lc's body before L2, one unconditional `++p` (or `p++`, `p += 1`) at the top level of L2's
  // body, nothing else inside lc.
  ExprRef cursor_of(LoopCtx* lc, size_t L2, const struct InnerInfo& info, const VarDecl* p) {
    const size_t L = lc->index;
    if (!p->getType()->isPointerType() || locals_[p].addr_taken) return nullptr;
    const LocalEvent* set = nullptr;
    const LocalEvent* inc = nullptr;
    for (const LocalEvent& ev : local_events_) {
      if (ev.var != p || std::find(ev.loops.begin(), ev.loops.end(), L) == ev.loops.end()) continue;
      if (ev.kind == LocalEvent::Other && ev.rhs != nullptr && ev.loops.back() == L && ev.guard_depth == lc->body_guard_base && set == nullptr) {
        set = &ev;
        continue;
      }
      if (ev.kind == LocalEvent::Inc1 && ev.loops.back() == L2 && ev.guard_depth == info.body_guard_base && inc == nullptr) {
        inc = &ev;
        continue;
      }
      return nullptr;
    }
    if (set == nullptr || inc == nullptr || m_.loops[L2].pos < set->pos) return nullptr;
    int64_t len = 0;
    const Expr* base = set->rhs->IgnoreParenImpCasts();
    if (const auto* cat = ctx_.getAsConstantArrayType(base->getType())) len = static_cast<int64_t>(cat->getSize().getZExtValue());
    return pool().make(KZ_EXPR_OP_CURSOR, 0, len, lid(p), to_expr(base), nullptr);
  }

  ExprRef build_budget(LoopCtx* lc) {
    for (const auto& kv : lc->decrements) {
      const VarDecl* v = kv.first;
      if (lc->other_mods.count(v) != 0) continue;
      // the variable must be tested by an exit condition
      bool tested = false;
      for (ExprRef x : lc->exits) tested |= expr_mentions_local(x, lid(v));
      if (!tested) continue;
      lc->budget_var = v;
      const LocalInfo& li = locals_[v];
      const Expr* init = nullptr;
      int outside = 0;
      for (const DefInfo& d : li.defs) {
        if (in_range(d.loc, lc->range)) continue;
        ++outside;
        init = d.rhs;
      }
      std::vector<ExprRef> parts;
      parts.push_back(pool().load(KZ_EXPR_REF_LOCAL, lid(v)));
      parts.push_back(outside == 1 && init != nullptr ? to_expr(init) : pool().top("no single initial value"));
      const Expr* step = kv.second;
      parts.push_back(step != nullptr ? to_expr(step) : pool().constant(1));
      // the definitions inside the loop of the variable that is subtracted
      if (step != nullptr) {
        if (const auto* dre = dyn_cast<DeclRefExpr>(step->IgnoreParenImpCasts())) {
          if (const auto* sv = dyn_cast<VarDecl>(dre->getDecl())) {
            for (const DefInfo& d : locals_[sv].defs)
              if (in_range(d.loc, lc->range) && d.rhs != nullptr) parts.push_back(pool().binary(KZ_EXPR_OP_STORE, pool().load(KZ_EXPR_REF_LOCAL, lid(sv)), to_expr(d.rhs)));
          }
        }
      }
      return pool().list(KZ_EXPR_OP_SEQ, parts);
    }
    return nullptr;
  }

  static void add_decrement(LoopCtx* lc, const VarDecl* v, const Expr* amount) {
    for (const auto& d : lc->decrements)
      if (d.first == v) return;
    lc->decrements.emplace_back(v, amount);
  }

  static bool expr_mentions_local(ExprRef e, const std::string& name) {
    if (e == nullptr) return false;
    if (e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_LOCAL && e->name == name) return true;
    return expr_mentions_local(e->a, name) || expr_mentions_local(e->b, name);
  }

  // A loop that reads NULL-terminated table elements: `V = Table[i]; if (V == NULL) break;`.
  // Called before the body is read; sets lc->walk_found, lc->walk_var and lc->walk.
  bool detect_table_walk(LoopCtx* lc, const Stmt* body) {
    const auto* comp = body != nullptr ? dyn_cast<CompoundStmt>(body) : nullptr;
    if (comp == nullptr) return false;
    const VarDecl* iv = lc->induction;
    for (const Stmt* st : comp->body()) {
      const auto* is = dyn_cast<IfStmt>(st);
      if (is == nullptr) continue;
      const Expr* c = is->getCond()->IgnoreParenImpCasts();
      const VarDecl* tested_var = nullptr;
      if (const auto* bo = dyn_cast<BinaryOperator>(c)) {
        if (bo->getOpcode() == BO_EQ) {
          const Expr* l = bo->getLHS()->IgnoreParenImpCasts();
          if (is_null_const(bo->getRHS()))
            if (const auto* dre = dyn_cast<DeclRefExpr>(l)) tested_var = dyn_cast<VarDecl>(dre->getDecl());
        }
      } else if (const auto* uo = dyn_cast<UnaryOperator>(c)) {
        if (uo->getOpcode() == UO_LNot)
          if (const auto* dre = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParenImpCasts())) tested_var = dyn_cast<VarDecl>(dre->getDecl());
      }
      if (tested_var == nullptr) continue;
      const Stmt* then = is->getThen();
      if (const auto* cs = dyn_cast<CompoundStmt>(then))
        if (cs->size() == 1) then = *cs->body_begin();
      if (!isa<BreakStmt>(then)) continue;
      // the element read: V = Base[index]
      const LocalInfo& li = locals_[tested_var];
      for (const DefInfo& d : li.defs) {
        if (d.rhs == nullptr || !in_range(d.loc, lc->range)) continue;
        const auto* as = dyn_cast<ArraySubscriptExpr>(d.rhs->IgnoreParenImpCasts());
        if (as == nullptr) continue;
        if (iv != nullptr) {
          const auto* idx = dyn_cast<DeclRefExpr>(as->getIdx()->IgnoreParenImpCasts());
          if (idx == nullptr || idx->getDecl() != iv) continue;
        }
        // the base: a global object, possibly through fields
        std::vector<std::string> fields, owners;
        std::vector<bool> consts;
        const Expr* b = as->getBase()->IgnoreParenImpCasts();
        const VarDecl* root = nullptr;
        for (;;) {
          if (const auto* me = dyn_cast<MemberExpr>(b)) {
            const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl());
            fields.insert(fields.begin(), me->getMemberDecl()->getNameAsString());
            owners.insert(owners.begin(), fl != nullptr ? field_type_name(fl) : std::string());
            consts.insert(consts.begin(), fl != nullptr && fl->getType().isConstQualified());
            b = me->getBase()->IgnoreParenImpCasts();
            continue;
          }
          if (const auto* dre = dyn_cast<DeclRefExpr>(b)) root = dyn_cast<VarDecl>(dre->getDecl());
          break;
        }
        if (root == nullptr || !root->hasGlobalStorage()) continue;
        Model::WalkSpec w;
        w.loop_key = m_.loops[lc->index].key;
        w.root_object = tu_.object_key(root);
        w.fields = fields;
        w.text = tu_.source_text(as->getBase()->getSourceRange());
        w.field_owner = owners;
        w.field_const = consts;
        w.root_const = root->getType().isConstant(ctx_);
        QualType et = tested_var->getType().getCanonicalType();
        if (et->isPointerType()) et = et->getPointeeType();
        if (const RecordDecl* erd = tu_.record_of(et)) w.elem_type = tu_.record_name(erd);
        lc->walk = w;
        lc->walk_found = true;
        lc->walk_var = tested_var;
        return true;
      }
    }
    return false;
  }

  void leave_loop(LoopCtx* lc, const Stmt* body) {
    end_loop(lc, body);
    inner_ctx_[lc->index] = InnerInfo{lc->induction, lc->body_guard_base};
    breakables_.pop_back();
    // fold the inner loop's findings into the outer loops
    LoopCtx* outer = loops_.size() >= 2 ? loops_[loops_.size() - 2].get() : nullptr;
    if (outer != nullptr) {
      outer->n_calls += lc->n_calls;
      outer->mod_names.insert(lc->mod_names.begin(), lc->mod_names.end());
      outer->callees.insert(lc->callees.begin(), lc->callees.end());
      for (const auto& kv : lc->decrements) add_decrement(outer, kv.first, kv.second);
      outer->other_mods.insert(lc->other_mods.begin(), lc->other_mods.end());
      outer->shared_loads += lc->shared_loads;
      outer->shared_loads_locked += lc->shared_loads_locked;
      outer->volatile_load |= lc->volatile_load;
      outer->lock_calls += lc->lock_calls;
      outer->callees_locked.insert(lc->callees_locked.begin(), lc->callees_locked.end());
      outer->callees_unlocked.insert(lc->callees_unlocked.begin(), lc->callees_unlocked.end());
      if (outer->delay_call.empty()) outer->delay_call = lc->delay_call;
      if (outer->lock_call.empty()) outer->lock_call = lc->lock_call;
    }
    loops_.pop_back();
  }

  void visit_while(const WhileStmt* s) {
    LoopCtx* lc = begin_loop(s, s->getCond(), false, true);
    detect_table_walk(lc, s->getBody());
    if (ext_) note_param_truth(s->getCond(), 0);
    visit(s->getCond());
    guards_.push_back({s->getCond(), true});
    lc->body_guard_base = guards_.size();
    visit(s->getBody());
    guards_.pop_back();
    leave_loop(lc, s->getBody());
  }
  void visit_do(const DoStmt* s) {
    LoopCtx* lc = begin_loop(s, s->getCond(), false, true);
    detect_table_walk(lc, s->getBody());
    lc->body_guard_base = guards_.size();
    visit(s->getBody());
    if (ext_) note_param_truth(s->getCond(), 0);
    visit(s->getCond());
    leave_loop(lc, s->getBody());
  }
  void visit_for(const ForStmt* s) {
    visit(s->getInit());
    LoopCtx* lc = begin_loop(s, s->getCond(), true, true);
    recognise_counted(lc, s);
    detect_table_walk(lc, s->getBody());
    if (ext_) note_param_truth(s->getCond(), 0);
    if (s->getCond() != nullptr) visit(s->getCond());
    if (s->getCond() != nullptr) guards_.push_back({s->getCond(), true});
    lc->body_guard_base = guards_.size();
    visit(s->getBody());
    // The step of a counted loop modifies its induction variable by definition; only a
    // modification in the body (or in an inner loop) makes the trip count unknown.
    const bool body_modifies = lc->induction != nullptr && lc->other_mods.count(lc->induction) != 0;
    visit(s->getInc());
    if (lc->induction != nullptr && !body_modifies) lc->other_mods.erase(lc->induction);
    if (s->getCond() != nullptr) guards_.pop_back();
    leave_loop(lc, s->getBody());
  }

  void recognise_counted(LoopCtx* lc, const ForStmt* s) {
    const VarDecl* iv = nullptr;
    int64_t c0 = 0;
    // init
    if (const auto* ds = dyn_cast_or_null<DeclStmt>(s->getInit())) {
      if (ds->isSingleDecl())
        if (const auto* vd = dyn_cast<VarDecl>(ds->getSingleDecl()))
          if (vd->getInit() != nullptr && tu_.eval_int(vd->getInit(), &c0)) iv = vd;
    } else if (const auto* bo = dyn_cast_or_null<BinaryOperator>(s->getInit())) {
      if (bo->getOpcode() == BO_Assign)
        if (const auto* dre = dyn_cast<DeclRefExpr>(bo->getLHS()->IgnoreParenImpCasts()))
          if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl()))
            if (tu_.eval_int(bo->getRHS(), &c0)) iv = vd;
    }
    if (iv == nullptr || s->getCond() == nullptr || s->getInc() == nullptr) return;
    // cond
    const auto* cb = dyn_cast<BinaryOperator>(s->getCond()->IgnoreParenImpCasts());
    if (cb == nullptr || !cb->isComparisonOp()) return;
    int64_t bound = 0;
    BinaryOperatorKind cmp = cb->getOpcode();
    const Expr* l = cb->getLHS()->IgnoreParenImpCasts();
    const Expr* r = cb->getRHS();
    const Expr* bound_expr = nullptr;
    const auto* ld = dyn_cast<DeclRefExpr>(l);
    if (ld != nullptr && ld->getDecl() == iv && tu_.eval_int(r, &bound)) {
      bound_expr = r;
    } else if (const auto* rd = dyn_cast<DeclRefExpr>(r->IgnoreParenImpCasts())) {
      if (rd->getDecl() != iv || !tu_.eval_int(cb->getLHS(), &bound)) return;
      bound_expr = cb->getLHS();
      switch (cmp) {  // the variable is on the right: mirror the comparison
        case BO_LT: cmp = BO_GT; break;
        case BO_LE: cmp = BO_GE; break;
        case BO_GT: cmp = BO_LT; break;
        case BO_GE: cmp = BO_LE; break;
        default: break;
      }
    } else {
      return;
    }
    // inc
    int64_t step = 0;
    const Expr* inc = s->getInc();
    if (const auto* uo = dyn_cast<UnaryOperator>(inc->IgnoreParens())) {
      const auto* dre = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParenImpCasts());
      if (dre == nullptr || dre->getDecl() != iv) return;
      if (uo->isIncrementOp()) step = 1;
      else if (uo->isDecrementOp()) step = -1;
      else return;
    } else if (const auto* ca = dyn_cast<BinaryOperator>(inc->IgnoreParens())) {
      const auto* dre = dyn_cast<DeclRefExpr>(ca->getLHS()->IgnoreParenImpCasts());
      if (dre == nullptr || dre->getDecl() != iv) return;
      int64_t k = 0;
      if (ca->getOpcode() == BO_AddAssign && tu_.eval_int(ca->getRHS(), &k)) step = k;
      else if (ca->getOpcode() == BO_SubAssign && tu_.eval_int(ca->getRHS(), &k)) step = -k;
      else return;
    } else {
      return;
    }
    lc->counted = true;
    lc->induction = iv;
    lc->c0 = c0;
    lc->bound = bound;
    lc->step = step;
    lc->cmp = cmp;
    lc->bound_text = tu_.source_text(bound_expr->getSourceRange());
  }

  void note_exit(LoopCtx* lc, const Stmt* at, uint8_t kind) {
    ++lc->n_exits;
    lc->exits.push_back(guard_from(lc->guard_base));
    // what decides this exit: the innermost guard inside the loop (none: an unconditional exit)
    lc->triggers.push_back(guards_.size() > lc->body_guard_base ? cond_expr(guards_.back().cond, guards_.back().polarity) : nullptr);
    lc->exit_pos.push_back(tu_.pos_of(at->getBeginLoc()));
    lc->exit_kind.push_back(kind);
  }
  void visit_break(const BreakStmt* s) {
    if (breakables_.empty() || breakables_.back() == nullptr) return;  // leaves a switch, not a loop
    note_exit(breakables_.back(), s, KZ_LOOPEXIT_KIND_BREAK);
  }
  void visit_return(const ReturnStmt* s) {
    for (auto& lc : loops_) note_exit(lc.get(), s, KZ_LOOPEXIT_KIND_RETURN);
    if (ext_ && s->getRetValue() != nullptr) returns_.push_back(PendingReturn{s, guard_from(0), loops_.empty() ? std::string() : m_.loops[loops_.back()->index].key});
    if (ext_ && s->getRetValue() != nullptr) {
      std::set<const VarDecl*> seen;
      const Src src = trace_source(s->getRetValue(), &seen);
      if (src.src == KZ_ARGSRC_PARAM) add_param_use(src.param, KZ_PFLOW_KIND_RETURN, 0, s, to_expr(s->getRetValue()));
    }
    if (const Expr* rv = s->getRetValue()) {
      visit(rv);
      if (is_fp_type(fd_->getReturnType())) add_flow_to("R:" + fkey_, rv);
      check_address_use(rv, "returned");
    }
  }
  void visit_goto(const Stmt* s) {
    for (auto& lc : loops_) note_exit(lc.get(), s, KZ_LOOPEXIT_KIND_GOTO);
  }

  // ---- fp-flow helpers ----
  std::string var_node(const VarDecl* vd) {
    if (const auto* pv = dyn_cast<ParmVarDecl>(vd)) return "P:" + fkey_ + "#" + std::to_string(pv->getFunctionScopeIndex());
    if (vd->hasGlobalStorage()) return "G:" + tu_.object_key(vd);
    return "L:" + fkey_ + "/" + vd->getNameAsString();
  }
  std::vector<std::string> fp_sources(const Expr* e) {
    std::vector<std::string> out;
    if (e == nullptr) return out;
    e = e->IgnoreParenCasts();
    if (is_null_const(e)) return out;
    if (const auto* dre = dyn_cast<DeclRefExpr>(e)) {
      if (const auto* fd = dyn_cast<FunctionDecl>(dre->getDecl())) {
        out.push_back("F:" + tu_.ensure_func(fd).key);
        return out;
      }
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
        out.push_back(var_node(vd));
        return out;
      }
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(e)) {
      if (uo->getOpcode() == UO_AddrOf) return fp_sources(uo->getSubExpr());
      if (uo->getOpcode() == UO_Deref) return fp_sources(uo->getSubExpr());
    }
    if (const auto* me = dyn_cast<MemberExpr>(e)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) {
        out.push_back(tu_.record_name(fl->getParent()) + "." + fl->getNameAsString());
        return out;
      }
    }
    if (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) return fp_sources(as->getBase());
    if (const auto* co = dyn_cast<ConditionalOperator>(e)) {
      auto a = fp_sources(co->getTrueExpr());
      auto b = fp_sources(co->getFalseExpr());
      a.insert(a.end(), b.begin(), b.end());
      return a;
    }
    if (const auto* ce = dyn_cast<CallExpr>(e)) {
      if (const FunctionDecl* callee = ce->getDirectCallee()) {
        out.push_back("R:" + tu_.ensure_func(callee).key);
        return out;
      }
      out.push_back("?:result of an indirect call");
      return out;
    }
    out.push_back("?:" + std::string(e->getStmtClassName()));
    return out;
  }
  void add_flow_to(const std::string& dst, const Expr* rhs) {
    for (const std::string& src : fp_sources(rhs)) m_.flow.push_back(FlowEdge{src, dst});
  }
  // The node an assignment to this lvalue feeds ("" if the target is not followed).
  std::string lvalue_node(const Expr* lv) {
    lv = lv->IgnoreParenImpCasts();
    if (const auto* dre = dyn_cast<DeclRefExpr>(lv))
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) return var_node(vd);
    if (const auto* me = dyn_cast<MemberExpr>(lv))
      if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) return tu_.record_name(fl->getParent()) + "." + fl->getNameAsString();
    if (const auto* as = dyn_cast<ArraySubscriptExpr>(lv)) return lvalue_node(as->getBase());
    // `*p = f`: the node of the pointer stands for what it points at (see the call arguments below)
    if (const auto* uo = dyn_cast<UnaryOperator>(lv))
      if (uo->getOpcode() == UO_Deref)
        if (const auto* dre = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParenImpCasts()))
          if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl()))
            if (is_fp_ptr_type(vd->getType())) return var_node(vd);
    return "";
  }

  // ---- call facts ----
  struct CallContext {
    bool used = false, tested = false;
  };
  CallContext call_context(const CallExpr* ce) {
    CallContext cc;
    const Stmt* top = chain_top(ce);
    const Stmt* p = parent_stmt(top);
    if (p == nullptr) {
      if (const VarDecl* vd = parent_var(top)) {
        cc.used = true;
        cc.tested = tested_.count(vd) != 0;
      }
      return cc;
    }
    if (const auto* cs = dyn_cast<CStyleCastExpr>(p)) {
      if (cs->getCastKind() == CK_ToVoid) return cc;  // explicitly discarded
    }
    if (isa<Expr>(p)) {
      cc.used = true;
      if (const auto* bo = dyn_cast<BinaryOperator>(p)) {
        if (bo->isComparisonOp()) cc.tested = true;
        if (bo->getOpcode() == BO_Assign) {
          if (const auto* dre = dyn_cast<DeclRefExpr>(bo->getLHS()->IgnoreParenImpCasts()))
            if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) cc.tested = tested_.count(vd) != 0;
        }
        if (bo->getOpcode() == BO_LAnd || bo->getOpcode() == BO_LOr) cc.tested = true;
      }
      if (const auto* uo = dyn_cast<UnaryOperator>(p))
        if (uo->getOpcode() == UO_LNot) cc.tested = true;
      if (isa<ConditionalOperator>(p)) cc.tested = true;
      return cc;
    }
    cc.used = false;
    if (const auto* is = dyn_cast<IfStmt>(p)) cc.used = cc.tested = (is->getCond() == top);
    else if (const auto* ws = dyn_cast<WhileStmt>(p)) cc.used = cc.tested = (ws->getCond() == top);
    else if (const auto* ds = dyn_cast<DoStmt>(p)) cc.used = cc.tested = (ds->getCond() == top);
    else if (const auto* fs = dyn_cast<ForStmt>(p)) cc.used = cc.tested = (fs->getCond() == top);
    else if (const auto* ss = dyn_cast<SwitchStmt>(p)) cc.used = cc.tested = (ss->getCond() == top);
    else if (isa<ReturnStmt>(p)) cc.used = true;
    return cc;
  }

  ArgFact classify_arg(const Expr* arg) {
    ArgFact a;
    const Expr* s = arg->IgnoreParenCasts();
    int64_t c = 0;
    a.symbol = tu_.source_text(arg->getSourceRange(), 80);
    if (tu_.eval_int(s, &c) && !isa<StringLiteral>(s)) {
      a.kind = KZ_ARG_KIND_CONST;
      a.value = c;
      return a;
    }
    if (const auto* sl = dyn_cast<StringLiteral>(s)) {
      a.kind = KZ_ARG_KIND_STRING;
      a.symbol = sl->getString().str();
      return a;
    }
    if (const auto* dre = dyn_cast<DeclRefExpr>(s)) {
      if (const auto* fd = dyn_cast<FunctionDecl>(dre->getDecl())) {
        a.kind = KZ_ARG_KIND_FUNC;
        a.fn_key = tu_.ensure_func(fd).key;
        a.symbol = fd->getNameAsString();
        return a;
      }
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
        if (vd->hasGlobalStorage()) {
          a.kind = KZ_ARG_KIND_SYMBOLIC;
          a.symbol = vd->getNameAsString();
          return a;
        }
      }
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(s)) {
      if (uo->getOpcode() == UO_AddrOf) {
        const Expr* sub = uo->getSubExpr()->IgnoreParens();
        while (const auto* as = dyn_cast<ArraySubscriptExpr>(sub)) sub = as->getBase()->IgnoreParenImpCasts();
        if (const auto* me = dyn_cast<MemberExpr>(sub)) {
          if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) {
            a.kind = KZ_ARG_KIND_ADDR_FIELD;
            a.loc_type = field_type_name(fl);
            a.loc_field = fl->getNameAsString();
            return a;
          }
        }
        if (const auto* dre2 = dyn_cast<DeclRefExpr>(sub)) {
          if (const auto* fd = dyn_cast<FunctionDecl>(dre2->getDecl())) {
            a.kind = KZ_ARG_KIND_FUNC;
            a.fn_key = tu_.ensure_func(fd).key;
            a.symbol = fd->getNameAsString();
            return a;
          }
          if (const auto* vd = dyn_cast<VarDecl>(dre2->getDecl())) {
            a.kind = KZ_ARG_KIND_ADDR_OBJECT;
            a.symbol = vd->getNameAsString();
            return a;
          }
        }
      }
    }
    if (const auto* me = dyn_cast<MemberExpr>(s)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) {
        a.kind = KZ_ARG_KIND_SYMBOLIC;
        a.loc_type = field_type_name(fl);
        a.loc_field = fl->getNameAsString();
        a.symbol = a.loc_type + "." + a.loc_field;
        return a;
      }
    }
    a.kind = KZ_ARG_KIND_TOP;
    return a;
  }

  // A copy chain from one shared location (SPEC 1.4 T-DERIVE).
  struct DeriveResult {
    uint8_t status = KZ_DERIVES_STATUS_NOT_DERIVED;
    uint8_t reason = KZ_DERIVES_REASON_NONE;
    std::string loc_type, loc_field;
    Pos capture;
    uint32_t hops = 0;
    ExprRef chain = nullptr;
  };
  DeriveResult derive(const Expr* e, int depth) {
    DeriveResult r;
    if (e == nullptr || depth > 8) {
      r.reason = KZ_DERIVES_REASON_NOT_COPY;
      return r;
    }
    int64_t c = 0;
    if (!isa<CallExpr>(e->IgnoreParenCasts()) && tu_.eval_int(e, &c)) {  // the value after the conversions
      r.status = KZ_DERIVES_STATUS_CONSTANT;
      r.chain = pool().constant(c);
      return r;
    }
    // A copy keeps the value: parentheses, lvalue-to-rvalue and no-op conversions, and integer
    // conversions that cannot lose information (to a type at least as wide).  A narrowing
    // conversion, a conversion to bool and every other cast is not a copy.
    const Expr* s = e;
    for (;;) {
      s = s->IgnoreParens();
      const auto* ce = dyn_cast<CastExpr>(s);
      if (ce == nullptr) break;
      const CastKind k = ce->getCastKind();
      if (k == CK_LValueToRValue || k == CK_NoOp) {
        s = ce->getSubExpr();
        continue;
      }
      if (k == CK_IntegralCast) {
        const QualType from = ce->getSubExpr()->getType(), to = ce->getType();
        if (!from->isIntegerType() || !to->isIntegerType() || ctx_.getIntWidth(to) < ctx_.getIntWidth(from)) {
          r.reason = KZ_DERIVES_REASON_NOT_COPY;
          return r;
        }
        s = ce->getSubExpr();
        continue;
      }
      r.reason = KZ_DERIVES_REASON_NOT_COPY;
      return r;
    }
    const Expr* lv = s;
    while (const auto* as = dyn_cast<ArraySubscriptExpr>(lv)) lv = as->getBase()->IgnoreParens()->IgnoreImpCasts();
    if (const auto* me = dyn_cast<MemberExpr>(lv)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) {
        r.status = KZ_DERIVES_STATUS_DERIVED;
        r.loc_type = field_type_name(fl);
        r.loc_field = fl->getNameAsString();
        r.capture = tu_.pos_of(me->getExprLoc());
        r.chain = pool().load(KZ_EXPR_REF_FIELD, r.loc_type + "." + r.loc_field);
        return r;
      }
    }
    // a global object, or an element of a global array (the array is the location)
    if (const auto* gre = dyn_cast<DeclRefExpr>(lv)) {
      if (const auto* gvd = dyn_cast<VarDecl>(gre->getDecl()); gvd != nullptr && gvd->hasGlobalStorage()) {
        r.status = KZ_DERIVES_STATUS_DERIVED;
        r.loc_type = "<global>";
        r.loc_field = gvd->getNameAsString();
        r.capture = tu_.pos_of(gre->getLocation());
        r.chain = pool().load(KZ_EXPR_REF_GLOBAL, gvd->getNameAsString());
        return r;
      }
    }
    if (const auto* dre = dyn_cast<DeclRefExpr>(s)) {
      if (isa<ParmVarDecl>(dre->getDecl())) {
        r.reason = KZ_DERIVES_REASON_FROM_PARAM;
        return r;
      }
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
        auto it = locals_.find(vd);
        if (it == locals_.end() || it->second.defs.empty()) {
          r.reason = KZ_DERIVES_REASON_NO_DEF;
          return r;
        }
        if (it->second.addr_taken) {
          r.reason = KZ_DERIVES_REASON_ADDRESS_TAKEN;
          return r;
        }
        if (it->second.defs.size() > 1) {
          r.reason = KZ_DERIVES_REASON_MULTI_DEF;
          return r;
        }
        const DefInfo& d = it->second.defs.front();
        if (!d.copy || d.rhs == nullptr) {
          r.reason = KZ_DERIVES_REASON_NOT_COPY;
          return r;
        }
        DeriveResult inner = derive(d.rhs, depth + 1);
        if (inner.status == KZ_DERIVES_STATUS_NOT_DERIVED) return inner;
        inner.hops += 1;
        inner.chain = pool().make(KZ_EXPR_OP_COPY, 0, 0, lid(vd), inner.chain, nullptr);
        return inner;
      }
    }
    if (isa<CallExpr>(s)) {
      r.reason = KZ_DERIVES_REASON_FROM_CALL;
      return r;
    }
    if (isa<UnaryOperator>(s) || isa<BinaryOperator>(s) || isa<ConditionalOperator>(s)) {
      r.reason = KZ_DERIVES_REASON_NOT_COPY;
      return r;
    }
    r.reason = KZ_DERIVES_REASON_NOT_SHARED;
    return r;
  }

  std::string callee_text(const CallExpr* ce) {
    const FunctionDecl* c = ce->getDirectCallee();
    return c != nullptr ? c->getNameAsString() : std::string("<indirect>");
  }

  bool is_data_call(const FunctionDecl* callee) { return callee != nullptr && spec_.data_fns.count(callee->getNameAsString()) != 0; }

  void visit_call(const CallExpr* ce) {
    const FunctionDecl* callee = ce->getDirectCallee();
    CallFact cf;
    cf.pos = tu_.pos_of(ce->getBeginLoc());
    const std::string ckey = tu_.next_key(cf.pos, fkey_, &cf.ordinal);
    if (ext_) {
      ext_call_keys_[ce] = ckey;
      std::vector<LockId>& ids = m_.call_arg_idents[ckey];
      for (const Expr* a : ce->arguments()) {
        std::set<const VarDecl*> seen;
        ids.push_back(ident_value(a, &seen));
      }
    }
    cf.tu = tu_.main_file();
    cf.caller_key = fkey_;
    cf.caller_name = fname_;
    ++m_.stats.calls;
    if (callee != nullptr) {
      const FuncFact& ff = tu_.ensure_func(callee);
      cf.callee_key = ff.key;
      cf.callee_name = ff.name;
      cf.kind = (callee->getBuiltinID() != 0 && ff.name.compare(0, 10, "__builtin_") == 0) ? KZ_CALL_KIND_BUILTIN : KZ_CALL_KIND_DIRECT;
      if (callee->getDefinition() != nullptr) cf.flags |= KZ_CALL_FLAG_INTRA_TU;
    } else {
      cf.kind = KZ_CALL_KIND_INDIRECT;
      cf.indirect_node = fp_callee_node(ce->getCallee());
      m_.indirect_calls.push_back(IndirectCall{ckey, cf.indirect_node});
    }
    if (ce->getBeginLoc().isMacroID()) cf.flags |= KZ_CALL_FLAG_IN_MACRO;
    const CallContext ctxr = call_context(ce);
    if (ctxr.used) cf.flags |= KZ_CALL_FLAG_RESULT_USED;
    if (ctxr.tested) cf.flags |= KZ_CALL_FLAG_RESULT_TESTED;
    if (!loops_.empty()) cf.flags |= KZ_CALL_FLAG_IN_LOOP;
    cf.loop_depth = static_cast<uint16_t>(loops_.size());
    if (!loops_.empty()) cf.loop_key = m_.loops[loops_.back()->index].key;
    const std::string name = cf.callee_name;
    // the profile's operations by name; the profile-free mode's by function key (empty otherwise)
    const std::string& ckk = cf.callee_key;
    auto by_key = [&](const std::set<std::string>& keys) { return !ckk.empty() && keys.count(ckk) != 0; };
    const bool is_delay = spec_.delay_ops.count(name) != 0 || by_key(spec_.delay_keys);
    const bool is_terminal = spec_.terminal_ops.count(name) != 0 || by_key(spec_.terminal_keys);
    const bool is_spawn = spec_.spawn_ops.count(name) != 0 || by_key(spec_.spawn_keys);
    const bool is_lock = spec_.lock_ops.count(name) != 0 || by_key(spec_.lock_keys);
    const bool is_unlock = spec_.unlock_ops.count(name) != 0 || by_key(spec_.unlock_keys);
    if (spec_.collect_wrapper_inputs) {
      call_exprs_[ce] = ckey;
      if (callee != nullptr) {
        std::vector<FpArg> fa;
        for (unsigned i = 0; i < ce->getNumArgs() && i < callee->getNumParams(); ++i) {
          if (!is_fp_type(callee->getParamDecl(i)->getType())) continue;
          FpArg a;
          a.index = i;
          a.nodes = fp_sources(ce->getArg(i));
          a.from_data = fp_from_data(ce->getArg(i));
          fa.push_back(a);
        }
        if (!fa.empty()) m_.call_fp_args[ckey] = fa;
      }
    }
    if (!spec_.guard_calls.empty() && spec_.guard_calls.count(ckey) != 0) m_.call_guards[ckey] = guard_from(0);
    const bool is_data = is_data_call(callee);
    if (is_delay) cf.flags |= KZ_CALL_FLAG_DELAY_OP;
    if (is_terminal) cf.flags |= KZ_CALL_FLAG_TERMINAL_OP;
    if (is_spawn) cf.flags |= KZ_CALL_FLAG_SPAWN_OP;
    if (is_lock) cf.flags |= KZ_CALL_FLAG_LOCK_OP;
    if (is_unlock) cf.flags |= KZ_CALL_FLAG_UNLOCK_OP;
    if (is_data) cf.flags |= KZ_CALL_FLAG_DATA_FN;

    // arguments and their roles
    const OpInfo* op = nullptr;
    auto oit = spec_.ops.find(name);
    if (oit != spec_.ops.end()) op = &oit->second;
    for (unsigned i = 0; i < ce->getNumArgs(); ++i) {
      ArgFact a = classify_arg(ce->getArg(i));
      if (op != nullptr && i < op->roles.size()) a.role = op->roles[i];
      cf.args.push_back(a);
    }

    // F4 (ARGFLOW): where every argument comes from
    if (ext_) note_argflows(ce, callee, ckey);

    // loop bookkeeping
    for (auto& lc : loops_) {
      ++lc->n_calls;
      lc->callees.insert(name.empty() ? "<indirect>" : name);
      (lock_depth_ > 0 ? lc->callees_locked : lc->callees_unlocked).insert(name.empty() ? "<indirect>" : name);
      if (is_delay && lc->delay_call.empty()) lc->delay_call = ckey;
      if (is_lock) {
        if (lc->lock_call.empty()) lc->lock_call = ckey;
        ++lc->lock_calls;
      }
    }
    if (is_delay && !loops_.empty()) loops_.back()->delay_direct = true;
    if (is_lock) ++lock_depth_;
    if (is_unlock && lock_depth_ > 0) --lock_depth_;

    // table walks around the call: its guards and the arguments that read the element, in
    // element form, for the per-iteration evaluation in finalize
    for (auto& lcp : loops_) {
      LoopCtx* lc = lcp.get();
      if (!lc->walk_found || lc->walk_var == nullptr) continue;
      WalkUse wu;
      wu.loop_key = m_.loops[lc->index].key;
      wu.call_key = ckey;
      elem_var_ = lc->walk_var;
      wu.guard = guard_from(lc->body_guard_base);
      if (callee == nullptr) {
        const Expr* cexpr = ce->getCallee()->IgnoreParenImpCasts();
        if (const auto* um = dyn_cast<UnaryOperator>(cexpr); um != nullptr && um->getOpcode() == UO_Deref) cexpr = um->getSubExpr()->IgnoreParenImpCasts();
        if (const auto* cme = dyn_cast<MemberExpr>(cexpr); cme != nullptr && is_elem_member(cme)) wu.callee_field = cme->getMemberDecl()->getNameAsString();
      }
      for (unsigned i = 0; i < ce->getNumArgs(); ++i)
        if (mentions_var(ce->getArg(i), lc->walk_var)) wu.args.push_back({i, to_expr(ce->getArg(i))});
      elem_var_ = nullptr;
      m_.walk_uses.push_back(wu);
    }

    // derives rows for the arguments with a role
    for (unsigned i = 0; i < cf.args.size(); ++i) {
      if (cf.args[i].role == KZ_ARG_ROLE_NONE) continue;
      DeriveResult dr = derive(ce->getArg(i), 0);
      DerivesFact d;
      d.call_key = ckey;
      d.arg_index = i;
      d.callee = name;
      d.status = dr.status;
      d.reason = dr.reason;
      d.loc_type = dr.loc_type;
      d.loc_field = dr.loc_field;
      d.capture = dr.capture;
      d.hops = dr.hops;
      d.chain = dr.chain;
      m_.derives.push_back(d);
    }

    // function-pointer arguments flow into the callee's parameters
    if (callee != nullptr) {
      for (unsigned i = 0; i < ce->getNumArgs() && i < callee->getNumParams(); ++i) {
        const std::string param = "P:" + cf.callee_key + "#" + std::to_string(i);
        if (is_fp_type(callee->getParamDecl(i)->getType())) add_flow_to(param, ce->getArg(i));
        // The address of a function pointer goes in; what the callee stores through it comes out.
        // One node stands for the pointer and for what it points at, so both directions are edges.
        if (is_fp_ptr_type(callee->getParamDecl(i)->getType()))
          for (const std::string& src : fp_sources(ce->getArg(i))) {
            m_.flow.push_back(FlowEdge{src, param});
            m_.flow.push_back(FlowEdge{param, src});
          }
      }
    } else {
      for (const Expr* a : ce->arguments())
        for (const std::string& src : fp_sources(a))
          if (src.compare(0, 2, "F:") == 0) m_.flow.push_back(FlowEdge{src, "?:passed to an indirect call"});
    }

    // spawn operations: which argument carries the entry function (a derived operation of the
    // profile-free mode names it; else the first function-pointer parameter)
    if (is_spawn && callee != nullptr) {
      m_.spawn_calls.push_back(ckey);
      std::vector<std::string> nodes;
      auto named = spec_.spawn_entry_param.find(ckk);
      if (named != spec_.spawn_entry_param.end()) {
        if (named->second < ce->getNumArgs()) {
          nodes = fp_sources(ce->getArg(named->second));
          m_.spawn_entry_arg[ckey] = named->second;
        }
      } else {
        for (unsigned i = 0; i < ce->getNumArgs() && i < callee->getNumParams(); ++i) {
          if (!callee->getParamDecl(i)->getType().getCanonicalType()->isFunctionPointerType()) continue;
          nodes = fp_sources(ce->getArg(i));
          m_.spawn_entry_arg[ckey] = i;
          break;
        }
      }
      m_.spawn_entry_nodes[ckey] = nodes;
    }

    // terminal operations
    if (is_terminal) {
      TerminalFact t;
      t.call_key = ckey;
      t.pos = cf.pos;
      t.tu = cf.tu;
      t.func_key = fkey_;
      t.op = name;
      t.guard = guard_from(0);
      t.depth = static_cast<uint32_t>(guards_.size());
      if ((oit != spec_.ops.end() && oit->second.process_terminal) || by_key(spec_.process_terminal_keys)) t.flags |= KZ_TERMINAL_FLAG_PROCESS_TERMINAL;
      if (by_key(spec_.conditional_terminal_keys)) t.flags |= KZ_TERMINAL_FLAG_CONDITIONAL;
      if (!loops_.empty()) t.flags |= KZ_TERMINAL_FLAG_IN_LOOP;
      if (guards_.empty()) t.flags |= KZ_TERMINAL_FLAG_UNGUARDED;
      m_.terminals.push_back(t);
    }

    // data functions: typed writers (memset family)
    if (is_data) handle_data_call(ce, callee, ckey, cf.pos);

    // calls with the address of a target field
    for (unsigned i = 0; i < ce->getNumArgs(); ++i) {
      const ArgFact& a = cf.args[i];
      if (a.kind != KZ_ARG_KIND_ADDR_FIELD) continue;
      for (size_t t = 0; t < spec_.fields.size(); ++t) {
        if (spec_.fields[t].field.field != a.loc_field || !type_name_matches(a.loc_type, spec_.fields[t].field.type)) continue;
        OutCallFact oc;
        oc.call_key = ckey;
        oc.func_key = fkey_;
        oc.pos = cf.pos;
        oc.ordinal = cf.ordinal;
        oc.tu = cf.tu;
        oc.arg_index = i;
        oc.loc_type = spec_.fields[t].field.type;
        oc.loc_field = a.loc_field;
        oc.callee_key = cf.callee_key;
        oc.callee_name = name;
        oc.summary = is_data ? KZ_OUTPTR_SUMMARY_DATA_FN : (callee == nullptr ? KZ_OUTPTR_SUMMARY_UNSUMMARIZED : KZ_OUTPTR_SUMMARY_NONE);
        m_.outcalls.push_back(oc);
      }
    }
    // pointers that reach a target, for the unknown-effect check of finalize (SPEC 4.2 steps 2-3):
    // the destination and the source of a data function are its summary, not an unknown effect
    for (unsigned i = 0; i < ce->getNumArgs(); ++i) {
      if (is_data) continue;  // the summary of a data function says what it writes (its destination)
      std::set<size_t> reached = targets_reached(ce->getArg(i)->IgnoreParenImpCasts()->getType());
      const std::set<size_t> before_casts = targets_reached(ce->getArg(i)->IgnoreParenCasts()->getType());
      reached.insert(before_casts.begin(), before_casts.end());
      if (reached.empty()) continue;
      Model::ReachPass rp;
      rp.call_key = ckey;
      rp.func_key = fkey_;
      rp.tu = cf.tu;
      rp.pos = cf.pos;
      rp.ordinal = cf.ordinal;
      rp.arg_index = i;
      rp.callee_key = cf.callee_key;
      rp.callee_name = name;
      rp.variadic_position = callee != nullptr && i >= callee->getNumParams();
      std::set<std::string> types;
      for (size_t t : reached) types.insert(spec_.fields[t].field.type);
      rp.types.assign(types.begin(), types.end());
      rp.text = tu_.source_text(ce->getArg(i)->getSourceRange(), 60);
      m_.reach_passes.push_back(rp);
    }
    m_.calls.push_back(cf);

    // the callee expression of an indirect call is itself evaluated; arguments are visited
    if (callee == nullptr) visit(ce->getCallee());
    for (const Expr* a : ce->arguments()) visit(a);
  }

  bool type_name_matches(const std::string& display, const std::string& spec_type) { return display == spec_type; }

  // The targets whose owner type a pointer of type `t` reaches: the records it points at (through
  // any number of pointer levels), the records they hold by value, and, transitively, the records
  // their pointer fields point at.  A value that is not a pointer (or an array) reaches nothing:
  // the callee gets a copy.  void and char pointees reach nothing typed (the conversion to them
  // is a cast hit of its own).
  std::set<size_t> targets_reached(QualType t) {
    std::set<size_t> out;
    if (t.isNull()) return out;
    t = t.getCanonicalType();
    if (const ArrayType* at = ctx_.getAsArrayType(t)) t = at->getElementType().getCanonicalType();
    else if (t->isPointerType()) t = t->getPointeeType().getCanonicalType();
    else return out;
    const RecordDecl* start = pointee_record(t);
    if (start == nullptr) return out;
    auto cit = reach_cache_.find(start);
    if (cit != reach_cache_.end()) return cit->second;
    std::set<const RecordDecl*> seen;
    std::vector<const RecordDecl*> work{start};
    while (!work.empty()) {
      const RecordDecl* rd = work.back();
      work.pop_back();
      if (rd == nullptr || !seen.insert(rd).second) continue;
      for (size_t i = 0; i < spec_.fields.size(); ++i)
        if (tu_.is_owner_record(rd, i)) out.insert(i);
      const RecordDecl* def = rd->getDefinition();
      if (def == nullptr) continue;
      for (const FieldDecl* fd : def->fields()) work.push_back(pointee_record(fd->getType()));
    }
    reach_cache_[start] = out;
    return out;
  }
  // The record a type is or points at, through arrays and any number of pointer levels (not
  // through function pointers); null for anything else.
  const RecordDecl* pointee_record(QualType t) {
    t = t.getCanonicalType();
    for (int guard = 0; guard < 16; ++guard) {
      if (const ArrayType* at = ctx_.getAsArrayType(t)) {
        t = at->getElementType().getCanonicalType();
        continue;
      }
      if (t->isPointerType() && !t->isFunctionPointerType()) {
        t = t->getPointeeType().getCanonicalType();
        continue;
      }
      break;
    }
    return tu_.record_of(t);
  }

  // Inline assembly (SPEC 4.2 step 3: unknown effect).  An operand that is, holds or reaches a
  // target, and every target when the statement clobbers memory (the assembly may then store
  // to any object, by name or through any address).
  void visit_asm(const AsmStmt* s) {
    std::set<size_t> hit;
    bool memory = false;
    for (unsigned i = 0; i < s->getNumClobbers(); ++i)
      if (s->getClobber(i) == "memory") memory = true;
    auto operand = [&](const Expr* e) {
      if (e == nullptr) return;
      const Expr* x = e->IgnoreParenImpCasts();
      for (size_t t : targets_reached(x->getType())) hit.insert(t);
      for (size_t t : tu_.owners_contained_in(x->getType())) hit.insert(t);
      std::set<size_t> on_chain;
      chain_targets(x, &on_chain);
      hit.insert(on_chain.begin(), on_chain.end());
      if (const auto* me = dyn_cast<MemberExpr>(x->IgnoreParens()))
        if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl()))
          for (size_t t : tu_.targets_of_field(fl)) hit.insert(t);
    };
    for (unsigned i = 0; i < s->getNumOutputs(); ++i) operand(s->getOutputExpr(i));
    for (unsigned i = 0; i < s->getNumInputs(); ++i) operand(s->getInputExpr(i));
    for (size_t t = 0; t < spec_.fields.size(); ++t) {
      if (!memory && hit.count(t) == 0) continue;
      add_atype(KZ_ATYPE_KIND_UNKNOWN_EFFECT, t, s, memory ? "inline assembly that clobbers memory" : "inline assembly with an operand that reaches the type");
    }
    for (const Stmt* c : s->children()) visit(c);
  }

  std::string fp_callee_node(const Expr* callee_expr) {
    std::vector<std::string> src = fp_sources(callee_expr);
    // fp_sources of a callee expression: the node(s) that carry the function value
    if (src.empty()) return "?:null";
    if (src.size() == 1) return src.front();
    return "?:several";
  }

  // The size of a type in bytes, or -1 when it has none (incomplete, void, a function).
  int64_t type_size(QualType t) {
    if (t.isNull() || t->isIncompleteType() || t->isFunctionType() || t->isVoidType() || t->isDependentType()) return -1;
    return static_cast<int64_t>(ctx_.getTypeSizeInChars(t).getQuantity());
  }
  // The parameter of a data function that bounds the write: the first size_t parameter other
  // than the destination and the source; -1 when there is none (strcpy, sprintf: unbounded).
  int size_param(const FunctionDecl* callee, const DataFn& d) {
    const QualType st = ctx_.getSizeType().getCanonicalType();
    for (unsigned i = 0; i < callee->getNumParams(); ++i) {
      if (static_cast<int>(i) == d.dest || static_cast<int>(i) == d.src) continue;
      if (callee->getParamDecl(i)->getType().getCanonicalType() == st) return static_cast<int>(i);
    }
    return -1;
  }
  // The target fields whose storage holds the lvalue `lv` as a proper part: the members on the
  // chain of `.` accesses and subscripts above it, up to and including the first `->` access.
  void chain_targets(const Expr* lv, std::set<size_t>* out) {
    const Expr* e = lv->IgnoreParens();
    bool first = true;
    for (;;) {
      if (const auto* me = dyn_cast<MemberExpr>(e)) {
        if (!first)
          if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl()))
            for (size_t t : tu_.targets_of_field(fl)) out->insert(t);
        if (me->isArrow()) return;
        e = me->getBase()->IgnoreParens();
      } else if (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) {
        e = as->getBase()->IgnoreParenImpCasts();
      } else {
        return;
      }
      first = false;
    }
  }
  // The object a write starting at `lv` stays inside when it stays inside any object: the
  // outermost lvalue through `.` members and subscripts of arrays; through `->` (or `*p`) the
  // record the pointer points at.  `offset` is where `lv` starts in it (-1 when an index is not
  // constant); `fields` are the target fields on the way (they contain `lv`).
  struct Enclosing {
    QualType type;
    int64_t offset = 0;
    std::string text;
    std::set<size_t> fields;
  };
  Enclosing enclosing_object(const Expr* lv) {
    Enclosing r;
    const Expr* e = lv->IgnoreParens();
    for (;;) {
      if (const auto* me = dyn_cast<MemberExpr>(e)) {
        const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl());
        if (fl == nullptr) break;
        if (r.offset >= 0) r.offset += static_cast<int64_t>(ctx_.getFieldOffset(fl) / ctx_.getCharWidth());
        for (size_t t : tu_.targets_of_field(fl)) r.fields.insert(t);
        if (me->isArrow()) {
          r.type = me->getBase()->getType()->getPointeeType();
          r.text = "the record " + tu_.source_text(me->getBase()->getSourceRange(), 40) + " points at";
          return r;
        }
        e = me->getBase()->IgnoreParens();
        continue;
      }
      if (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) {
        const Expr* base = as->getBase()->IgnoreParenImpCasts();
        if (!base->getType()->isArrayType()) {  // a subscript of a pointer: the pointee array is not known
          r.type = e->getType();
          r.offset = 0;
          r.text = "an element of " + tu_.source_text(base->getSourceRange(), 40);
          return r;
        }
        int64_t idx = 0;
        const int64_t es = type_size(e->getType());
        if (r.offset >= 0 && es >= 0 && tu_.eval_int(as->getIdx(), &idx)) r.offset += idx * es;
        else r.offset = -1;
        e = base->IgnoreParens();
        continue;
      }
      if (const auto* uo = dyn_cast<UnaryOperator>(e); uo != nullptr && uo->getOpcode() == UO_Deref) {
        r.type = e->getType();
        r.text = "the object " + tu_.source_text(uo->getSubExpr()->getSourceRange(), 40) + " points at";
        return r;
      }
      break;
    }
    r.type = e->getType();
    r.text = tu_.source_text(e->getSourceRange(), 40);
    return r;
  }

  // memset/memcpy family: a typed write of every field of the destination's pointee type.
  void handle_data_call(const CallExpr* ce, const FunctionDecl* callee, const std::string& ckey, const Pos& cpos) {
    (void)cpos;
    const DataFn& d = spec_.data_fns.at(callee->getNameAsString());
    if (static_cast<unsigned>(d.dest) >= ce->getNumArgs()) return;
    ++m_.data_fn_calls;
    const Expr* dest = ce->getArg(d.dest);
    const Expr* core = dest->IgnoreParenCasts();
    ValueInfo vi;
    vi.kind = KZ_ARG_KIND_TOP;
    if (d.writer_kind == KZ_WRITER_KIND_MEMSET) {
      int64_t fill = 0;
      if (d.zero_fill) {
        vi.kind = KZ_ARG_KIND_CONST;
        vi.value = 0;
        vi.text = "zero fill";
      } else if (d.fill >= 0 && static_cast<unsigned>(d.fill) < ce->getNumArgs() && tu_.eval_int(ce->getArg(d.fill), &fill)) {
        if (fill == 0) {
          vi.kind = KZ_ARG_KIND_CONST;
          vi.value = 0;
          vi.text = "fill 0";
        } else {
          vi.text = "fill " + std::to_string(fill);
        }
      }
    } else if (d.src >= 0 && static_cast<unsigned>(d.src) < ce->getNumArgs()) {
      vi.text = "copy of " + tu_.source_text(ce->getArg(d.src)->getSourceRange(), 60);
    } else {
      vi.text = "formatted output";
    }
    const std::string key = tu_.source_text(dest->getSourceRange(), 60);
    std::set<size_t> exact, contained;
    bool untyped = false;
    // the region the destination denotes
    if (const auto* uo = dyn_cast<UnaryOperator>(core); uo != nullptr && uo->getOpcode() == UO_AddrOf) {
      const Expr* lv = uo->getSubExpr()->IgnoreParens();
      if (const auto* me = dyn_cast<MemberExpr>(lv))
        if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl()))
          for (size_t t : tu_.targets_of_field(fl)) exact.insert(t);
      for (size_t t : tu_.owners_contained_in(lv->getType())) contained.insert(t);
    } else if (core->getType()->isPointerType()) {
      const QualType pt = core->getType()->getPointeeType();
      if (tu_.record_of(pt) != nullptr) {
        for (size_t t : tu_.owners_contained_in(pt)) contained.insert(t);
      } else if (is_untyped_pointee(pt)) {
        untyped = true;
      }
    } else if (const auto* me2 = dyn_cast<MemberExpr>(core)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me2->getMemberDecl()))
        for (size_t t : tu_.targets_of_field(fl)) exact.insert(t);
      for (size_t t : tu_.owners_contained_in(me2->getType())) contained.insert(t);
    } else if (core->getType()->isArrayType()) {
      for (size_t t : tu_.owners_contained_in(core->getType())) contained.insert(t);
    }
    // The destination as an lvalue (when it is one) and the type of the region it starts.
    const Expr* region_lv = nullptr;
    QualType region_t;
    bool via_pointer = false;
    if (const auto* uo = dyn_cast<UnaryOperator>(core); uo != nullptr && uo->getOpcode() == UO_AddrOf) {
      region_lv = uo->getSubExpr()->IgnoreParens();
      region_t = region_lv->getType();
    } else if (core->getType()->isArrayType() && (isa<MemberExpr>(core) || isa<DeclRefExpr>(core) || isa<ArraySubscriptExpr>(core))) {
      // an array lvalue that decays: the array itself is the region (a pointer-valued member or
      // variable is a pointer: the next case)
      region_lv = core;
      region_t = core->getType();
    } else if (core->getType()->isPointerType()) {
      region_t = core->getType()->getPointeeType();
      via_pointer = true;
    }
    // A store into a part of a target field (`&x.f.g`, `&x.f[2]`): a partial write of the field.
    std::set<size_t> partial;
    if (region_lv != nullptr) chain_targets(region_lv, &partial);
    for (size_t t : exact) partial.erase(t);
    // The extent of the write.  "Every field of the destination's type" (SPEC 4.2 step 2) is the
    // whole write only when the size argument is at most the size of that type.  Otherwise the
    // write is taken to cover the enclosing object (the outermost lvalue reached through `.`
    // members and subscripts, or the record an `->` access points into); a constant size that
    // goes past the end of that object, or a size that is not a whole number of elements of a
    // pointer's type, writes memory the types do not name: an unknown-effect hit for every target.
    std::string extent_note;
    if (!untyped && !region_t.isNull()) {
      const int size_arg = size_param(callee, d);
      int64_t n = 0;
      const bool n_known = size_arg >= 0 && static_cast<unsigned>(size_arg) < ce->getNumArgs() && tu_.eval_int(ce->getArg(size_arg), &n);
      const int64_t rsize = type_size(region_t);
      const bool inside = n_known && rsize >= 0 && n <= rsize;
      if (!inside && via_pointer) {
        if (n_known && rsize > 0 && n % rsize != 0) {
          for (size_t t = 0; t < spec_.fields.size(); ++t)
            add_atype(KZ_ATYPE_KIND_UNKNOWN_EFFECT, t, ce, callee->getNameAsString() + " writes " + std::to_string(n) + " bytes through a pointer to a type of " + std::to_string(rsize) + " bytes: " + key);
        } else if (!n_known) {
          extent_note = "; size not constant: taken to stay inside an array of the pointee type";
        }
      } else if (!inside && region_lv != nullptr) {
        const Enclosing enc = enclosing_object(region_lv);
        if (!enc.type.isNull()) {
          for (size_t t : tu_.owners_contained_in(enc.type)) contained.insert(t);
          for (size_t t : enc.fields) partial.insert(t);
          note_record_write(enc.type, ce);
          const int64_t osize = type_size(enc.type);
          if (n_known && enc.offset >= 0 && osize >= 0 && enc.offset + n > osize) {
            for (size_t t = 0; t < spec_.fields.size(); ++t)
              add_atype(KZ_ATYPE_KIND_UNKNOWN_EFFECT, t, ce, callee->getNameAsString() + " writes past the end of " + enc.text + ": " + key);
          } else {
            extent_note = n_known ? "; the size goes beyond the destination: covers " + enc.text
                                  : "; size not constant: taken to stay inside " + enc.text;
          }
        }
        for (size_t t : exact) partial.erase(t);
      }
    }
    if (!extent_note.empty()) vi.text += extent_note;
    // FUNC_WRITES: the region the destination denotes (a pointer to another scalar type writes
    // the object whose address was taken elsewhere: that use is an OUTARG row of its own)
    if (untyped) {
      note_write(KZ_FWRITE_KIND_UNTYPED, "", "", ce);
    } else if (const auto* uo = dyn_cast<UnaryOperator>(core); uo != nullptr && uo->getOpcode() == UO_AddrOf) {
      const Expr* lv = uo->getSubExpr()->IgnoreParens();
      while (const auto* as = dyn_cast<ArraySubscriptExpr>(lv)) lv = as->getBase()->IgnoreParenImpCasts();
      if (const auto* me = dyn_cast<MemberExpr>(lv)) {
        if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) note_write(KZ_FWRITE_KIND_FIELD, field_type_name(fl), fl->getNameAsString(), ce);
      } else if (const auto* gd = dyn_cast<DeclRefExpr>(lv)) {
        note_global_write(gd, ce);
      }
      note_record_write(uo->getSubExpr()->getType(), ce);
    } else if (core->getType()->isPointerType() && tu_.record_of(core->getType()->getPointeeType()) != nullptr) {
      note_record_write(core->getType()->getPointeeType(), ce);
    } else if (const auto* me3 = dyn_cast<MemberExpr>(core)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me3->getMemberDecl())) note_write(KZ_FWRITE_KIND_FIELD, field_type_name(fl), fl->getNameAsString(), ce);
      note_record_write(me3->getType(), ce);
    } else if (const auto* gd2 = dyn_cast<DeclRefExpr>(core)) {
      note_global_write(gd2, ce);
    }
    // a conversion of a typed region to an untyped pointer at this call is the summary itself,
    // not a cast hit: handled by is_data_arg in visit_cast.
    for (size_t t : contained) {
      const bool is_exact = exact.count(t) != 0;
      add_writer(t, ce, d.writer_kind, vi, key, is_exact ? 0u : static_cast<uint32_t>(KZ_WRITER_FLAG_COVERS_MORE));
    }
    for (size_t t : exact)
      if (contained.count(t) == 0) add_writer(t, ce, d.writer_kind, vi, key, 0u);
    for (size_t t : partial)
      if (contained.count(t) == 0 && exact.count(t) == 0) {
        ValueInfo pv;
        pv.kind = KZ_ARG_KIND_TOP;
        pv.text = vi.text;
        add_writer(t, ce, d.writer_kind, pv, key, KZ_WRITER_FLAG_PARTIAL);
      }
    if (untyped) {
      Model::UntypedWrite u;
      u.pos = tu_.pos_of(ce->getBeginLoc());
      u.func_key = fkey_;
      u.tu = tu_.main_file();
      u.call_key = ckey;
      u.detail = callee->getNameAsString() + " through an untyped pointer: " + key;
      tu_.next_key(u.pos, fkey_ + "#u", &u.ordinal);
      m_.untyped_writes.push_back(u);
    }
  }

  // ---- assignments and increments ----
  // `a[i]`, `a[i][j]`: the array variable, when the subscripts are of an array (not a pointer).
  static const DeclRefExpr* subscript_root(const Expr* lv) {
    const Expr* e = lv->IgnoreParenImpCasts();
    bool any = false;
    while (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) {
      const Expr* base = as->getBase()->IgnoreParenImpCasts();
      if (!base->getType()->isArrayType()) return nullptr;
      e = base->IgnoreParens();
      any = true;
    }
    return any ? dyn_cast<DeclRefExpr>(e) : nullptr;
  }
  void note_mod(const Expr* lv) {
    if (loops_.empty()) return;
    lv = lv->IgnoreParenImpCasts();
    std::string name;
    const VarDecl* var = nullptr;
    if (const auto* dre = dyn_cast<DeclRefExpr>(lv)) {
      if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
        name = vd->hasGlobalStorage() ? "global:" + vd->getNameAsString() : "local:" + lid(vd);
        var = vd;
      }
    } else if (const auto* me = dyn_cast<MemberExpr>(lv)) {
      if (const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl())) name = "field:" + tu_.record_name(fl->getParent()) + "." + fl->getNameAsString();
    } else if (const auto* as = dyn_cast<ArraySubscriptExpr>(lv)) {
      note_mod(as->getBase());
      return;
    } else {
      name = "local:<through a pointer>";
    }
    if (name.empty()) name = "local:<other>";
    for (auto& lc : loops_) {
      lc->mod_names.insert(name);
      if (var != nullptr) lc->other_mods.insert(var);
    }
  }

  // A definition of a local variable inside a loop (counter and cursor patterns).
  void note_local_event(const VarDecl* vd, LocalEvent::Kind kind, const Expr* rhs, SourceLocation loc) {
    if (loops_.empty() || vd == nullptr || vd->hasGlobalStorage()) return;
    LocalEvent ev;
    ev.kind = kind;
    ev.var = vd;
    for (const auto& lc : loops_) ev.loops.push_back(lc->index);
    ev.guard_depth = guards_.size();
    ev.guard_inner = guard_from(loops_.back()->body_guard_base);
    ev.rhs = rhs;
    ev.pos = tu_.pos_of(loc);
    local_events_.push_back(ev);
  }
  bool is_const_value(const Expr* e, int64_t v) {
    int64_t c = 0;
    return e != nullptr && tu_.eval_int(e->IgnoreParenCasts(), &c) && c == v;
  }

  void visit_binary(const BinaryOperator* bo) {
    const BinaryOperatorKind k = bo->getOpcode();
    if (k == BO_LAnd || k == BO_LOr) {
      if (ext_) {
        note_param_truth(bo->getLHS(), 0);
        note_param_truth(bo->getRHS(), 0);
      }
      visit(bo->getLHS());
      guards_.push_back({bo->getLHS(), k == BO_LAnd});
      visit(bo->getRHS());
      guards_.pop_back();
      return;
    }
    if (bo->isAssignmentOp()) {
      const Expr* lhs = bo->getLHS();
      if (const auto* ldre = dyn_cast<DeclRefExpr>(lhs->IgnoreParenImpCasts())) {
        if (const auto* lvd = dyn_cast<VarDecl>(ldre->getDecl())) {
          if (ext_) note_def(bo);
          if (k == BO_Assign) note_local_event(lvd, is_const_value(bo->getRHS(), 0) ? LocalEvent::Reset0 : LocalEvent::Other, bo->getRHS(), bo->getOperatorLoc());
          else if (k == BO_AddAssign && is_const_value(bo->getRHS(), 1)) note_local_event(lvd, LocalEvent::Inc1, nullptr, bo->getOperatorLoc());
          else note_local_event(lvd, LocalEvent::Other, nullptr, bo->getOperatorLoc());
        }
        note_global_write(ldre, bo);
      } else if (const DeclRefExpr* adre = subscript_root(lhs)) {
        note_global_write(adre, bo);  // an element of a global array
      }
      // loop bookkeeping: decrements are budget candidates; everything else modifies the variable
      if (!loops_.empty()) {
        const auto* dre = dyn_cast<DeclRefExpr>(lhs->IgnoreParenImpCasts());
        const VarDecl* vd = dre != nullptr ? dyn_cast<VarDecl>(dre->getDecl()) : nullptr;
        if (vd != nullptr && k == BO_SubAssign) {
          for (auto& lc : loops_) add_decrement(lc.get(), vd, bo->getRHS());
          for (auto& lc : loops_) lc->mod_names.insert(vd->hasGlobalStorage() ? "global:" + vd->getNameAsString() : "local:" + lid(vd));
        } else {
          note_mod(lhs);
        }
      }
      handle_assign_flow(bo);
      if (ext_ && k == BO_Assign) note_flow_store(bo);
      if (QualType lt = lhs->getType(); lt->isRecordType() || lt->isUnionType()) {
        note_record_write(lt, bo);
        for (size_t t : tu_.owners_contained_in(lt)) {
          ValueInfo v;
          v.kind = KZ_ARG_KIND_TOP;
          v.text = tu_.source_text(bo->getRHS()->getSourceRange(), 60);
          add_writer(t, bo, KZ_WRITER_KIND_AGGREGATE, v, tu_.source_text(lhs->getSourceRange(), 60), KZ_WRITER_FLAG_COVERS_MORE);
        }
      }
      note_untyped_store(lhs, bo);
    }
    if (ext_ && bo->isComparisonOp()) note_param_compare(bo);
    visit(bo->getLHS());
    visit(bo->getRHS());
  }

  void handle_assign_flow(const BinaryOperator* bo) {
    if (bo->getOpcode() != BO_Assign) return;
    const Expr* lhs = bo->getLHS();
    if (is_fp_type(lhs->getType())) {
      const std::string dst = lvalue_node(lhs);
      if (!dst.empty()) add_flow_to(dst, bo->getRHS());
    }
    check_address_use(bo->getRHS(), "assigned");
  }

  // `p = &obj.field` and friends: the address of a target field leaves the call-argument form.
  void check_address_use(const Expr* e, const char* how) {
    const Expr* s = e->IgnoreParenCasts();
    const auto* uo = dyn_cast<UnaryOperator>(s);
    if (uo == nullptr || uo->getOpcode() != UO_AddrOf) return;
    const Expr* sub = uo->getSubExpr()->IgnoreParens();
    while (const auto* as = dyn_cast<ArraySubscriptExpr>(sub)) sub = as->getBase()->IgnoreParenImpCasts();
    const auto* me = dyn_cast<MemberExpr>(sub);
    if (me == nullptr) return;
    const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl());
    if (fl == nullptr) return;
    for (size_t t : tu_.targets_of_field(fl)) add_atype(KZ_ATYPE_KIND_ADDRESS_ESCAPE, t, uo, std::string("the address of the field is ") + how, "", spec_.fields[t].field.field);
  }

  void note_untyped_store(const Expr* lhs, const Stmt* at) {
    const Expr* lv = lhs->IgnoreParenImpCasts();
    const Expr* base = nullptr;
    if (const auto* uo = dyn_cast<UnaryOperator>(lv)) {
      if (uo->getOpcode() == UO_Deref) base = uo->getSubExpr()->IgnoreParenImpCasts();
    } else if (const auto* as = dyn_cast<ArraySubscriptExpr>(lv)) {
      base = as->getBase()->IgnoreParenImpCasts();
    }
    if (base == nullptr) return;
    if (!base->getType()->isPointerType()) return;
    if (!is_untyped_pointee(base->getType()->getPointeeType())) return;
    note_write(KZ_FWRITE_KIND_UNTYPED, "", "", at);
    Model::UntypedWrite u;
    u.pos = tu_.pos_of(at->getBeginLoc());
    u.func_key = fkey_;
    u.tu = tu_.main_file();
    u.detail = "store through an untyped pointer: " + tu_.source_text(lhs->getSourceRange(), 60);
    tu_.next_key(u.pos, fkey_ + "#u", &u.ordinal);
    m_.untyped_writes.push_back(u);
  }

  void visit_unary(const UnaryOperator* uo) {
    if (uo->isIncrementDecrementOp()) {
      const Expr* sub = uo->getSubExpr();
      if (const auto* sdre = dyn_cast<DeclRefExpr>(sub->IgnoreParenImpCasts())) {
        if (const auto* svd = dyn_cast<VarDecl>(sdre->getDecl())) {
          note_local_event(svd, uo->isIncrementOp() ? LocalEvent::Inc1 : LocalEvent::Other, nullptr, uo->getOperatorLoc());
          if (ext_) note_def(uo);
        }
        note_global_write(sdre, uo);
      } else if (const DeclRefExpr* adre = subscript_root(sub)) {
        note_global_write(adre, uo);  // an element of a global array
      }
      if (!loops_.empty()) {
        const auto* dre = dyn_cast<DeclRefExpr>(sub->IgnoreParenImpCasts());
        const VarDecl* vd = dre != nullptr ? dyn_cast<VarDecl>(dre->getDecl()) : nullptr;
        if (vd != nullptr && uo->isDecrementOp()) {
          for (auto& lc : loops_) add_decrement(lc.get(), vd, nullptr);
          for (auto& lc : loops_) lc->mod_names.insert(vd->hasGlobalStorage() ? "global:" + vd->getNameAsString() : "local:" + lid(vd));
        } else {
          note_mod(sub);
        }
      }
    } else if (uo->getOpcode() == UO_LNot && ext_) {
      note_param_truth(uo->getSubExpr(), 0);
    } else if (uo->getOpcode() == UO_AddrOf) {
      // the address of a global object leaves the expression: whoever receives it may write it
      if (const auto* adre = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParens()))
        if (const auto* avd = dyn_cast<VarDecl>(adre->getDecl()); avd != nullptr && avd->hasGlobalStorage())
          note_write(KZ_FWRITE_KIND_OUTARG, "<global>", avd->getNameAsString(), uo);
    }
    visit(uo->getSubExpr());
  }

  // ---- member expressions: writers, address use, shared loads ----
  struct LvalueUse {
    enum Kind { Read, Assign, IncDec, AddrOf } kind = Read;
    const Stmt* stmt = nullptr;  // the assignment, the increment or the & expression
    const Expr* rhs = nullptr;
    bool compound = false;
    bool partial = false;  // the use is of a part of the lvalue (a member of it, an element of it)
  };
  // How the lvalue `me` is used: climbs over parentheses, `.` member accesses and subscripts of
  // which it is the base (a store to `x.f.g` or `x.f[i]` writes part of `x.f`), and stops at an
  // assignment, an increment or an `&`.  An `->` access through it reads it.
  LvalueUse classify_lvalue_use(const Expr* me) {
    LvalueUse u;
    const Stmt* cur = me;
    for (;;) {
      const Stmt* p = parent_stmt(cur);
      if (p == nullptr) return u;
      if (isa<ParenExpr>(p)) {
        cur = p;
        continue;
      }
      if (const auto* as = dyn_cast<ArraySubscriptExpr>(p)) {
        if (as->getBase()->IgnoreParenImpCasts() == cur || as->getBase() == cur) {
          u.partial = true;
          cur = p;
          continue;
        }
        return u;
      }
      if (const auto* pm = dyn_cast<MemberExpr>(p)) {
        if (!pm->isArrow() && (pm->getBase() == cur || pm->getBase()->IgnoreParens() == cur)) {
          u.partial = true;
          cur = p;
          continue;
        }
        return u;
      }
      if (const auto* bo = dyn_cast<BinaryOperator>(p)) {
        if (bo->isAssignmentOp() && (bo->getLHS() == cur || bo->getLHS()->IgnoreParens() == cur)) {
          u.kind = LvalueUse::Assign;
          u.stmt = bo;
          u.rhs = bo->getRHS();
          u.compound = bo->getOpcode() != BO_Assign;
        }
        return u;
      }
      if (const auto* uo = dyn_cast<UnaryOperator>(p)) {
        if (uo->isIncrementDecrementOp()) {
          u.kind = LvalueUse::IncDec;
          u.stmt = uo;
        } else if (uo->getOpcode() == UO_AddrOf) {
          u.kind = LvalueUse::AddrOf;
          u.stmt = uo;
        }
        return u;
      }
      return u;
    }
  }

  void visit_member(const MemberExpr* me) {
    const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl());
    if (fl != nullptr) {
      const LvalueUse use = classify_lvalue_use(me);
      const std::vector<size_t> tgts = tu_.targets_of_field(fl);
      // base object kind: a local aggregate accessed with '.'
      uint32_t base_flags = 0;
      if (!me->isArrow()) {
        const Expr* b = me->getBase()->IgnoreParenImpCasts();
        while (true) {
          if (const auto* bm = dyn_cast<MemberExpr>(b)) {
            if (bm->isArrow()) break;
            b = bm->getBase()->IgnoreParenImpCasts();
            continue;
          }
          if (const auto* as = dyn_cast<ArraySubscriptExpr>(b)) {
            b = as->getBase()->IgnoreParenImpCasts();
            continue;
          }
          break;
        }
        if (const auto* dre = dyn_cast<DeclRefExpr>(b))
          if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl()))
            if (vd->isLocalVarDecl() && !vd->isStaticLocal()) base_flags |= KZ_WRITER_FLAG_BASE_LOCAL;
      }
      const std::string key = tu_.source_text(me->getBase()->getSourceRange(), 60);
      if (use.kind == LvalueUse::Assign || use.kind == LvalueUse::IncDec) {
        note_write(KZ_FWRITE_KIND_FIELD, field_type_name(fl), fl->getNameAsString(), use.stmt);
        if (use.kind == LvalueUse::Assign && !use.partial && (fl->getType()->isRecordType() || fl->getType()->isArrayType())) note_record_write(fl->getType(), use.stmt);
      } else if (use.kind == LvalueUse::AddrOf) {
        note_write(KZ_FWRITE_KIND_OUTARG, field_type_name(fl), fl->getNameAsString(), use.stmt);
      }
      // a store to a part of the field (a member or an element of it) changes the field: a writer
      // whose value is not the value stored
      const uint32_t part_flags = use.partial ? static_cast<uint32_t>(KZ_WRITER_FLAG_PARTIAL) : 0u;
      for (size_t t : tgts) {
        switch (use.kind) {
          case LvalueUse::Assign: {
            ValueInfo v = (use.compound || use.partial) ? ValueInfo() : tu_.classify_value(use.rhs);
            if (use.compound || use.partial) v.text = tu_.source_text(use.stmt->getSourceRange(), 60);
            add_writer(t, use.stmt, KZ_WRITER_KIND_ASSIGN, v, key, base_flags | part_flags);
            break;
          }
          case LvalueUse::IncDec: {
            ValueInfo v;
            v.text = tu_.source_text(use.stmt->getSourceRange(), 60);
            add_writer(t, use.stmt, KZ_WRITER_KIND_INCDEC, v, key, base_flags | part_flags);
            break;
          }
          case LvalueUse::AddrOf: {
            // The address of the field as a call argument is an out-pointer call (recorded in
            // visit_call).  The address of a part of it is one only as the destination of a data
            // function (handle_data_call writes the field) or its source (a read).  Anything else
            // escapes.
            const Stmt* top = chain_top(use.stmt);
            const Stmt* p = parent_stmt(top);
            const auto* call = p != nullptr ? dyn_cast<CallExpr>(p) : nullptr;
            int arg_at = -1;
            if (call != nullptr)
              for (unsigned i = 0; i < call->getNumArgs(); ++i)
                if (call->getArg(i) == top || call->getArg(i)->IgnoreParenImpCasts() == use.stmt) arg_at = static_cast<int>(i);
            bool covered = arg_at >= 0 && !use.partial;
            if (arg_at >= 0 && use.partial && is_data_call(call->getDirectCallee())) {
              const DataFn& d = spec_.data_fns.at(call->getDirectCallee()->getNameAsString());
              covered = arg_at == d.dest || arg_at == d.src;
            }
            if (!covered) {
              std::string how = "other use";
              if (p != nullptr) how = std::string("used by ") + p->getStmtClassName();
              if (const VarDecl* vd = parent_var(top)) how = "initializer of " + vd->getNameAsString();
              if (arg_at >= 0) how = "passed to " + callee_text(call);
              add_atype(KZ_ATYPE_KIND_ADDRESS_ESCAPE, t, use.stmt, std::string(use.partial ? "the address of a part of the field: " : "the address of the field: ") + how, "",
                        spec_.fields[t].field.field);
            }
            break;
          }
          case LvalueUse::Read: break;
        }
      }
      // union member written: another member of the union may be the support type
      if (fl->getParent()->isUnion() && (use.kind == LvalueUse::Assign || use.kind == LvalueUse::IncDec)) {
        std::set<size_t> exposed, by_name;
        for (const FieldDecl* other : fl->getParent()->fields()) {
          for (size_t t : tu_.owners_contained_in(other->getType())) exposed.insert(t);
          // a sibling member of the target field shares its storage
          if (other != fl)
            for (size_t t = 0; t < spec_.fields.size(); ++t)
              if (tu_.is_owner_record(fl->getParent(), t) && other->getName() == spec_.fields[t].field.field) by_name.insert(t);
        }
        // the same hits as before (one per target); the cause is the field only for a target that is a member
        // of the union itself and whose type no member holds as a whole
        const std::set<size_t> by_type = exposed;
        for (size_t t : by_name) exposed.insert(t);
        for (size_t t : exposed)
          add_atype(KZ_ATYPE_KIND_UNION, t, use.stmt, "a store through a member of a union that also holds the type", "",
                    by_type.count(t) == 0 ? spec_.fields[t].field.field : std::string());
      }
      // shared loads of loops: a read of the field's value (an lvalue-to-rvalue conversion);
      // `x.f` as the base of another access, or an array that decays, reads nothing
      if (use.kind == LvalueUse::Read && !loops_.empty() && is_rvalue_load(me)) note_shared_load(me->getType());
    }
    visit(me->getBase());
  }

  // Is the lvalue `e` read, i.e. converted to an rvalue (through parentheses)?
  bool is_rvalue_load(const Expr* e) {
    const Stmt* cur = e;
    for (;;) {
      const Stmt* p = parent_stmt(cur);
      if (p == nullptr) return false;
      if (isa<ParenExpr>(p)) {
        cur = p;
        continue;
      }
      const auto* ic = dyn_cast<ImplicitCastExpr>(p);
      return ic != nullptr && ic->getCastKind() == CK_LValueToRValue;
    }
  }
  // A read that is one atomic access: an _Atomic object, or a volatile object of the C type
  // sig_atomic_t (C11 7.14), whose reads are single accesses.
  static bool is_atomic_read_type(QualType t) {
    if (t->isAtomicType()) return true;
    if (!t.isVolatileQualified()) return false;
    for (QualType u = t;;) {
      const auto* tt = u->getAs<TypedefType>();
      if (tt == nullptr) return false;
      if (tt->getDecl()->getName() == "sig_atomic_t") return true;
      u = tt->getDecl()->getUnderlyingType();
    }
  }
  void note_shared_load(QualType t) {
    for (auto& lc : loops_) {
      ++lc->shared_loads;
      if (lock_depth_ > 0) ++lc->shared_loads_locked;
      if (is_atomic_read_type(t)) lc->volatile_load = true;
    }
  }

  // ---- casts ----
  bool is_data_arg(const CastExpr* ce) {
    const Stmt* p = parent_stmt(ce);
    while (p != nullptr && (isa<ImplicitCastExpr>(p) || isa<ParenExpr>(p))) p = parent_stmt(p);
    const auto* call = p != nullptr ? dyn_cast<CallExpr>(p) : nullptr;
    if (call == nullptr) return false;
    const FunctionDecl* callee = call->getDirectCallee();
    if (!is_data_call(callee)) return false;
    const DataFn& d = spec_.data_fns.at(callee->getNameAsString());
    for (unsigned i = 0; i < call->getNumArgs(); ++i) {
      const Expr* a = call->getArg(i)->IgnoreParens();
      if (a == ce || call->getArg(i) == ce) return static_cast<int>(i) == d.dest || static_cast<int>(i) == d.src;
    }
    return true;  // part of the argument expression of a data function
  }

  void visit_cast(const CastExpr* ce) {
    if (ext_) note_read(ce);
    const CastKind k = ce->getCastKind();
    const Expr* sub = ce->getSubExpr();
    const QualType src = sub->getType(), dst = ce->getType();
    // The null pointer constant converts to every pointer type; it refers to no object.
    if ((k == CK_BitCast || k == CK_PointerToIntegral || k == CK_IntegralToPointer) && !is_null_const(sub) && !is_data_arg(ce)) {
      QualType sp, dp;
      if (src->isPointerType()) sp = src->getPointeeType();
      if (dst->isPointerType()) dp = dst->getPointeeType();
      for (size_t t = 0; t < spec_.fields.size(); ++t) {
        const bool s_has = !sp.isNull() && tu_.type_contains_target(sp, t);
        const bool d_has = !dp.isNull() && tu_.type_contains_target(dp, t);
        if (s_has && !d_has) {
          // A const view is not exempt: C lets a later cast drop the const again (to `char *`, say),
          // and that cast no longer mentions the type.
          add_atype(KZ_ATYPE_KIND_CAST, t, ce,
                    k == CK_PointerToIntegral ? "a pointer to the type is converted to an integer"
                    : (!dp.isNull() && dp.isConstQualified()) ? "a pointer to the type is converted to a const pointer to another type"
                                                              : "a pointer to the type is converted to a pointer to another type");
        } else if (d_has && !s_has) {
          add_atype(KZ_ATYPE_KIND_CAST, t, ce,
                    k == CK_IntegralToPointer ? "an integer is converted to a pointer to the type"
                                              : "a pointer to another type is converted to a pointer to the type");
        } else if (s_has && d_has && tu_.record_of(sp) != tu_.record_of(dp)) {
          add_atype(KZ_ATYPE_KIND_CAST, t, ce, "a pointer to a type that holds the type is converted to a pointer to another such type");
        }
      }
    }
    visit(sub);
  }

  // ---- references to functions and parameters ----
  void visit_declref(const DeclRefExpr* dre) {
    if (const auto* fd = dyn_cast<FunctionDecl>(dre->getDecl())) {
      tu_.ensure_func(fd).flags |= KZ_FUNC_FLAG_ADDRESS_TAKEN;
      return;
    }
    if (const auto* pv = dyn_cast<ParmVarDecl>(dre->getDecl())) {
      if (pv->getType()->isPointerType() && pv->getFunctionScopeIndex() < summary_.params.size()) param_use(dre, pv);
      return;
    }
    if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
      if (vd->hasGlobalStorage() && !loops_.empty() && is_rvalue_load(dre)) note_shared_load(dre->getType());
    }
  }

  // `p->f` or `(*p).f` for a pointer parameter p: a store to the member (or to a part of it) is a
  // store to the pointee, whose value is not the value stored; the member's address escapes; a
  // read is nothing.  (A field that is itself an extraction target also gets its writer from
  // visit_member; this is the summary the call sites instantiate when they pass the address of
  // a record-typed target.)
  template <typename Escape>
  void pointee_member_use(ParamSummary& ps, const MemberExpr* me, const DeclRefExpr* dre, Escape& escape) {
    const LvalueUse use = classify_lvalue_use(me);
    if (use.kind == LvalueUse::Assign || use.kind == LvalueUse::IncDec) {
      OutStore st;
      st.pos = tu_.pos_of(use.stmt->getBeginLoc());
      tu_.next_key(st.pos, fkey_ + "#o", &st.ordinal);
      st.value_kind = KZ_ARG_KIND_TOP;
      st.value_text = "store to a member of the pointee: " + tu_.source_text(use.stmt->getSourceRange(), 60);
      st.guard = guard_from(0);
      ps.stores.push_back(st);
      return;
    }
    if (use.kind == LvalueUse::AddrOf) escape("the address of a member of the pointee is taken");
    (void)dre;
  }

  // A use of a pointer parameter, for the out-pointer summary of the function (SPEC 4.2 step 2).
  void param_use(const DeclRefExpr* dre, const ParmVarDecl* pv) {
    ParamSummary& ps = summary_.params[pv->getFunctionScopeIndex()];
    const Stmt* top = chain_top(dre);
    // pointer-to-bool conversions in conditions are tests
    for (const Stmt* s = dre; s != top;) {
      const Stmt* p = parent_stmt(s);
      if (const auto* ic = dyn_cast_or_null<ImplicitCastExpr>(p))
        if (ic->getCastKind() == CK_PointerToBoolean) return;
      s = p;
    }
    const Stmt* p = parent_stmt(top);
    auto escape = [&](const std::string& why) {
      ps.escapes = true;
      if (ps.escape_detail.empty()) ps.escape_detail = why + " at " + tu_.pos_of(dre->getBeginLoc()).str();
    };
    if (p == nullptr) {
      escape("unclassified use");
      return;
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(p)) {
      if (uo->getOpcode() == UO_LNot) return;
      if (uo->getOpcode() == UO_Deref) {
        const Stmt* g = chain_top(uo);
        const Stmt* gp = parent_stmt(g);
        if (const auto* bo = dyn_cast_or_null<BinaryOperator>(gp)) {
          if (bo->isAssignmentOp() && (bo->getLHS() == g || bo->getLHS()->IgnoreParens() == g)) {
            OutStore st;
            st.pos = tu_.pos_of(bo->getBeginLoc());
            tu_.next_key(st.pos, fkey_ + "#o", &st.ordinal);
            ValueInfo v = bo->getOpcode() == BO_Assign ? tu_.classify_value(bo->getRHS()) : ValueInfo();
            st.value_kind = v.kind;
            st.value = v.value;
            st.value_text = v.text;
            st.guard = guard_from(0);
            ps.stores.push_back(st);
            return;
          }
        }
        if (const auto* uo2 = dyn_cast_or_null<UnaryOperator>(gp)) {
          if (uo2->isIncrementDecrementOp()) {
            OutStore st;
            st.pos = tu_.pos_of(uo2->getBeginLoc());
            tu_.next_key(st.pos, fkey_ + "#o", &st.ordinal);
            st.value_kind = KZ_ARG_KIND_TOP;
            st.value_text = tu_.source_text(uo2->getSourceRange(), 60);
            st.guard = guard_from(0);
            ps.stores.push_back(st);
            return;
          }
          if (uo2->getOpcode() == UO_AddrOf) {
            escape("the address of the pointee is taken");
            return;
          }
        }
        // `(*p).f`: a use of a member of the pointee
        if (const auto* gm = dyn_cast_or_null<MemberExpr>(gp); gm != nullptr && !gm->isArrow()) {
          pointee_member_use(ps, gm, dre, escape);
          return;
        }
        return;  // a read through the pointer
      }
      escape(std::string("used by a unary operator"));
      return;
    }
    if (const auto* me = dyn_cast<MemberExpr>(p)) {
      // `p->f`: a use of a member of the pointee
      if (me->isArrow()) {
        pointee_member_use(ps, me, dre, escape);
        return;
      }
      escape("member access");
      return;
    }
    if (const auto* as = dyn_cast<ArraySubscriptExpr>(p)) {
      const Stmt* g = chain_top(as);
      const Stmt* gp = parent_stmt(g);
      if (const auto* bo = dyn_cast_or_null<BinaryOperator>(gp)) {
        if (bo->isAssignmentOp() && (bo->getLHS() == g || bo->getLHS()->IgnoreParens() == g)) {
          OutStore st;
          st.pos = tu_.pos_of(bo->getBeginLoc());
          tu_.next_key(st.pos, fkey_ + "#o", &st.ordinal);
          st.value_kind = KZ_ARG_KIND_TOP;
          st.value_text = "element store: " + tu_.source_text(bo->getSourceRange(), 60);
          st.guard = guard_from(0);
          ps.stores.push_back(st);
          return;
        }
      }
      // `p[i].f`: a use of a member of an element of the pointee
      if (const auto* gm = dyn_cast_or_null<MemberExpr>(gp); gm != nullptr && !gm->isArrow()) pointee_member_use(ps, gm, dre, escape);
      return;
    }
    if (const auto* bo = dyn_cast<BinaryOperator>(p)) {
      if ((bo->getOpcode() == BO_EQ || bo->getOpcode() == BO_NE) && (is_null_const(bo->getLHS()) || is_null_const(bo->getRHS()))) return;
      if (bo->getOpcode() == BO_LAnd || bo->getOpcode() == BO_LOr) return;
      escape("binary operator");
      return;
    }
    if (isa<IfStmt>(p) || isa<WhileStmt>(p) || isa<ForStmt>(p) || isa<DoStmt>(p) || isa<ConditionalOperator>(p)) return;
    if (const auto* call = dyn_cast<CallExpr>(p)) {
      for (unsigned i = 0; i < call->getNumArgs(); ++i) {
        if (call->getArg(i) == top || call->getArg(i)->IgnoreParenImpCasts() == dre) {
          ParamPass pp;
          pp.pos = tu_.pos_of(call->getBeginLoc());
          pp.arg_index = i;
          if (const FunctionDecl* callee = call->getDirectCallee()) {
            pp.callee_key = tu_.ensure_func(callee).key;
            pp.callee_name = callee->getNameAsString();
            pp.data_fn = is_data_call(callee);
          }
          if (pp.callee_key.empty()) escape("passed to an indirect call");
          else ps.passes.push_back(pp);
          return;
        }
      }
      escape("callee expression");
      return;
    }
    escape(std::string("used by ") + p->getStmtClassName());
  }

  // ---- declarations ----
  void visit_declstmt(const DeclStmt* ds) {
    for (const Decl* d : ds->decls()) {
      const auto* vd = dyn_cast<VarDecl>(d);
      if (vd == nullptr) continue;
      if (!vd->isStaticLocal()) {
        const Expr* init0 = vd->getInit();
        note_local_event(vd, init0 != nullptr && is_const_value(init0, 0) ? LocalEvent::Reset0 : LocalEvent::Other, init0, vd->getLocation());
      }
      if (const Expr* init = vd->getInit()) {
        if (ext_) note_def(vd);
        if (is_fp_type(vd->getType())) {
          const std::string dst = var_node(vd);
          add_flow_to(dst, init);
          if (const auto* il = dyn_cast<InitListExpr>(init->IgnoreParens()))
            for (const Expr* e : il->inits()) add_flow_to(dst, e);
        }
        check_address_use(init, "used to initialize a variable");
        visit(init);
      }
    }
  }

  void visit_initlist(const InitListExpr* il) {
    const QualType t = il->getType();
    if (const RecordType* rt = t.getCanonicalType()->getAs<RecordType>()) {
      const RecordDecl* rd = rt->getDecl()->getDefinition();
      if (rd != nullptr) {
        unsigned idx = 0;
        for (const FieldDecl* fd : rd->fields()) {
          if (fd->isUnnamedBitField()) continue;
          const Expr* e = idx < il->getNumInits() ? il->getInit(idx) : nullptr;
          ++idx;
          if (is_fp_type(fd->getType()) && e != nullptr) add_flow_to(tu_.record_name(rd) + "." + fd->getNameAsString(), e);
          for (size_t tg : tu_.targets_of_field(fd)) {
            ValueInfo v = e != nullptr && !isa<ImplicitValueInitExpr>(e) ? tu_.classify_value(e) : ValueInfo();
            if (e == nullptr || isa<ImplicitValueInitExpr>(e)) {
              v.kind = KZ_ARG_KIND_CONST;
              v.value = 0;
              v.text = "implicit zero";
            }
            add_writer(tg, il, KZ_WRITER_KIND_ASSIGN, v, "initializer", KZ_WRITER_FLAG_BASE_LOCAL);
          }
        }
      }
    }
    for (const Stmt* c : il->children()) visit(c);
  }

  // ---- the extended facts F1-F3 (spec.extended_facts; kz_facts.h revision 6) ---------------------------

  // A definition of a variable (an assignment to it, an increment, a declaration with an initializer):
  // the conditions it executes under, for the RETURNS rows.
  void note_def(const void* key) {
    def_guards_[key] = guard_from(0);
    def_in_loop_[key] = !loops_.empty();
  }
  bool before(SourceLocation a, SourceLocation b) const { return sm_.isBeforeInTranslationUnit(sm_.getExpansionLoc(a), sm_.getExpansionLoc(b)); }
  static bool is_const_zero(ExprRef e) { return e != nullptr && e->op == KZ_EXPR_OP_CONST && e->a == nullptr && e->b == nullptr && e->value == 0; }
  ExprRef negate(ExprRef g) {
    if (g == nullptr) return pool().constant(0);
    if (g->op == KZ_EXPR_OP_NOT && g->a != nullptr && g->b == nullptr) return g->a;
    return pool().unary(KZ_EXPR_OP_NOT, g);
  }
  // The conjunction of the terms, each split at its `and`s, without repeated conjuncts (the trees are
  // hash-consed, so a repeated conjunct is the same node); nullptr terms are `true`.
  void conjuncts(ExprRef e, std::vector<ExprRef>* out) {
    if (e == nullptr) return;
    if (e->op == KZ_EXPR_OP_AND) {
      conjuncts(e->a, out);
      conjuncts(e->b, out);
      return;
    }
    out->push_back(e);
  }
  ExprRef and_unique(const std::vector<ExprRef>& terms) {
    std::vector<ExprRef> parts, keep;
    for (ExprRef t : terms) conjuncts(t, &parts);
    std::set<ExprRef> seen;  // lookup only
    for (ExprRef p : parts)
      if (seen.insert(p).second) keep.push_back(p);
    return pool().conj(KZ_EXPR_OP_AND, keep);
  }

  // Replaces each local that has a single copy definition (the table of the derives rows: one
  // definition, a copy, its address not taken, not reading itself) placed before `limit` in the
  // source by the value of that definition, in turn unfolded before the definition.
  ExprRef unfold(ExprRef e, SourceLocation limit, std::set<std::string>* active, bool* changed) {
    if (e == nullptr) return nullptr;
    if (e->op == KZ_EXPR_OP_LOAD && e->ref == KZ_EXPR_REF_LOCAL && e->a == nullptr) {
      auto vit = local_by_id_.find(e->name);
      if (vit == local_by_id_.end() || active->count(e->name) != 0) return e;
      auto lit = locals_.find(vit->second);
      if (lit == locals_.end() || lit->second.addr_taken || lit->second.defs.size() != 1) return e;
      const DefInfo& d = lit->second.defs.front();
      if (!d.copy || d.rhs == nullptr || isa<InitListExpr>(d.rhs->IgnoreParens()) || !before(d.loc, limit) || mentions_var(d.rhs, vit->second)) return e;
      active->insert(e->name);
      ExprRef v = unfold(to_expr(d.rhs), d.loc, active, changed);
      active->erase(e->name);
      *changed = true;
      return v;
    }
    ExprRef a = unfold(e->a, limit, active, changed), b = unfold(e->b, limit, active, changed);
    if (a == e->a && b == e->b) return e;
    return pool().make(e->op, e->ref, e->value, e->name, a, b);
  }
  ExprRef unfold(ExprRef e, SourceLocation limit, bool* changed) {
    std::set<std::string> active;
    return unfold(e, limit, &active, changed);
  }

  static uint8_t value_kind(ExprRef v) {
    if (v == nullptr) return KZ_RET_VALUE_EXPR;
    switch (v->op) {
      case KZ_EXPR_OP_CONST: return v->a == nullptr ? KZ_RET_VALUE_CONST : KZ_RET_VALUE_EXPR;
      case KZ_EXPR_OP_CALL_RESULT: return KZ_RET_VALUE_CALL;
      case KZ_EXPR_OP_ADDR: return KZ_RET_VALUE_ADDR;
      case KZ_EXPR_OP_LOAD:
        switch (v->ref) {
          case KZ_EXPR_REF_FIELD: return KZ_RET_VALUE_FIELD;
          case KZ_EXPR_REF_GLOBAL: return KZ_RET_VALUE_GLOBAL;
          case KZ_EXPR_REF_PARAM: return KZ_RET_VALUE_PARAM;
          case KZ_EXPR_REF_LOCAL: return KZ_RET_VALUE_LOCAL;
          case KZ_EXPR_REF_STRING: return KZ_RET_VALUE_STRING;
          case KZ_EXPR_REF_FUNC: return KZ_RET_VALUE_ADDR;
          default: return KZ_RET_VALUE_EXPR;
        }
      default: return KZ_RET_VALUE_EXPR;
    }
  }
  std::string call_key_of(const Expr* e) {
    if (e == nullptr) return "";
    const auto* ce = dyn_cast<CallExpr>(e->IgnoreParenCasts());
    if (ce == nullptr) return "";
    auto it = ext_call_keys_.find(ce);
    return it == ext_call_keys_.end() ? std::string() : it->second;
  }

  // ---- F1 READS ----
  // A read is an lvalue-to-rvalue conversion (or the decay of an array: its contents are read through
  // the pointer) of a member access, of a global variable or of an element of a global array.  The
  // decay of an array that is the base of a subscript is a step of a longer path, not a read.
  void note_read(const CastExpr* ce) {
    if (!isa<ImplicitCastExpr>(ce)) return;
    const CastKind k = ce->getCastKind();
    bool decay = false;
    if (k == CK_ArrayToPointerDecay) {
      const Stmt* p = parent_stmt(ce);
      const Stmt* cur = ce;
      while (p != nullptr && isa<ParenExpr>(p)) {
        cur = p;
        p = parent_stmt(p);
      }
      if (const auto* as = p != nullptr ? dyn_cast<ArraySubscriptExpr>(p) : nullptr; as != nullptr && as->getBase() == cur) return;
      decay = true;
    } else if (k != CK_LValueToRValue) {
      return;
    }
    const Expr* lv = ce->getSubExpr()->IgnoreParens();
    // the location the path names: the subscripts of arrays at its end are stripped (a subscript of a
    // pointer names no location of its own; the pointer's own read is a row)
    std::vector<const Expr*> idx;  // leaf first
    const Expr* named = lv;
    while (const auto* as = dyn_cast<ArraySubscriptExpr>(named)) {
      const auto* bc = dyn_cast<ImplicitCastExpr>(as->getBase()->IgnoreParens());
      if (bc == nullptr || bc->getCastKind() != CK_ArrayToPointerDecay) return;
      idx.push_back(as->getIdx());
      named = bc->getSubExpr()->IgnoreParens();
    }
    ReadFact r;
    if (const auto* me = dyn_cast<MemberExpr>(named)) {
      const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl());
      if (fl == nullptr) return;
      r.kind = KZ_READ_KIND_FIELD;
      r.type = field_type_name(fl);
      r.field = fl->getNameAsString();
      if (m_.record_fields.count(r.type) == 0) {
        std::vector<std::string>& names = m_.record_fields[r.type];
        for (const FieldDecl* f : fl->getParent()->fields())
          if (!f->getName().empty()) names.push_back(f->getNameAsString());
      }
      // the root of the access path, and the subscripts on the way
      const Expr* cur = me;
      for (;;) {
        cur = cur->IgnoreParens();
        if (const auto* m = dyn_cast<MemberExpr>(cur)) {
          if (m->isArrow()) {
            r.flags |= KZ_READ_FLAG_THROUGH_POINTER;
            cur = m->getBase()->IgnoreParenImpCasts();
          } else {
            cur = m->getBase();
          }
          continue;
        }
        if (const auto* as = dyn_cast<ArraySubscriptExpr>(cur)) {
          idx.push_back(as->getIdx());
          const Expr* base = as->getBase()->IgnoreParens();
          const auto* bc = dyn_cast<ImplicitCastExpr>(base);
          if (bc != nullptr && bc->getCastKind() == CK_ArrayToPointerDecay) {
            cur = bc->getSubExpr();
          } else {
            r.flags |= KZ_READ_FLAG_THROUGH_POINTER;
            cur = base->IgnoreParenImpCasts();
          }
          continue;
        }
        if (const auto* uo = dyn_cast<UnaryOperator>(cur)) {
          if (uo->getOpcode() == UO_Deref) {
            r.flags |= KZ_READ_FLAG_THROUGH_POINTER;
            cur = uo->getSubExpr()->IgnoreParenImpCasts();
            continue;
          }
          if (uo->getOpcode() == UO_AddrOf) {
            cur = uo->getSubExpr();
            continue;
          }
        }
        if (const auto* c = dyn_cast<CastExpr>(cur)) {
          cur = c->getSubExpr();
          continue;
        }
        if (const auto* d = dyn_cast<DeclRefExpr>(cur)) {
          if (const auto* pv = dyn_cast<ParmVarDecl>(d->getDecl())) {
            r.root_kind = KZ_READ_ROOT_PARAM;
            r.root = pv->getNameAsString();
            r.root_param = pv->getFunctionScopeIndex();
          } else if (const auto* vd = dyn_cast<VarDecl>(d->getDecl())) {
            r.root_kind = vd->hasGlobalStorage() ? KZ_READ_ROOT_GLOBAL : KZ_READ_ROOT_LOCAL;
            r.root = vd->hasGlobalStorage() ? vd->getNameAsString() : lid(vd);
          } else {
            r.root_kind = KZ_READ_ROOT_OTHER;
            r.root = d->getDecl()->getNameAsString();
          }
          break;
        }
        if (const auto* call = dyn_cast<CallExpr>(cur)) {
          r.root_kind = KZ_READ_ROOT_CALL;
          r.root = callee_text(call);
          break;
        }
        r.root_kind = KZ_READ_ROOT_OTHER;
        r.root = cur->getStmtClassName();
        break;
      }
    } else if (const auto* dre = dyn_cast<DeclRefExpr>(named)) {
      const auto* vd = dyn_cast<VarDecl>(dre->getDecl());
      if (vd == nullptr || !vd->hasGlobalStorage()) return;
      r.kind = KZ_READ_KIND_GLOBAL;
      r.type = "<global>";
      r.field = vd->getNameAsString();
      r.root_kind = KZ_READ_ROOT_GLOBAL;
      r.root = r.field;
    } else {
      return;
    }
    if (decay) r.flags |= KZ_READ_FLAG_ARRAY_CONTENTS;
    else if (lv->getType()->isRecordType()) r.flags |= KZ_READ_FLAG_RECORD;
    if (ce->getBeginLoc().isMacroID()) r.flags |= KZ_READ_FLAG_IN_MACRO;
    if (!idx.empty()) {
      std::vector<ExprRef> parts;
      for (size_t i = idx.size(); i > 0; --i) parts.push_back(to_expr(idx[i - 1]));  // root first
      // a subscript that is a local with a single copy definition before the read: its value (as in RETURNS)
      bool changed = false;
      r.index = unfold(pool().list(KZ_EXPR_OP_SEQ, parts), ce->getBeginLoc(), &changed);
      if (changed) r.flags |= KZ_READ_FLAG_INDEX_UNFOLDED;
    }
    // the use: every context the value passes through on its way up to its statement
    const Stmt* cur = ce;
    for (;;) {
      const Stmt* p = parent_stmt(cur);
      if (p == nullptr) {
        if (const VarDecl* vd = parent_var(cur)) {
          r.use |= KZ_READ_USE_STORE;
          if (r.ctx == nullptr) r.ctx = vd->hasGlobalStorage() ? pool().load(KZ_EXPR_REF_GLOBAL, vd->getNameAsString()) : pool().load(KZ_EXPR_REF_LOCAL, lid(vd));
        }
        break;
      }
      if (isa<ParenExpr>(p) || isa<ImplicitCastExpr>(p)) {
        cur = p;
        continue;
      }
      if (const auto* cs = dyn_cast<CStyleCastExpr>(p)) {
        if (cs->getCastKind() == CK_ToVoid) break;
        cur = p;
        continue;
      }
      if (const auto* uo = dyn_cast<UnaryOperator>(p)) {
        if (uo->getOpcode() == UO_Deref) r.use |= KZ_READ_USE_DEREF;
        if (uo->getOpcode() == UO_Deref || uo->getOpcode() == UO_AddrOf || uo->isIncrementDecrementOp()) break;
        cur = p;  // !, -, ~, +: the value goes on
        continue;
      }
      if (const auto* bo = dyn_cast<BinaryOperator>(p)) {
        if (bo->getOpcode() == BO_LAnd || bo->getOpcode() == BO_LOr) {
          r.use |= KZ_READ_USE_BRANCH;
          if (r.ctx == nullptr) r.ctx = cond_expr(cast<Expr>(cur), true);
          cur = p;
          continue;
        }
        if (bo->isAssignmentOp()) {
          if (bo->getRHS() == cur) {
            r.use |= is_out_store(bo->getLHS()) ? KZ_READ_USE_OUT_STORE : KZ_READ_USE_STORE;
            if (r.ctx == nullptr) r.ctx = to_expr(bo->getLHS());
          }
          break;
        }
        if (bo->getOpcode() == BO_Comma && bo->getLHS() == cur) break;
        cur = p;
        continue;
      }
      if (const auto* co = dyn_cast<ConditionalOperator>(p)) {
        if (co->getCond() == cur) {
          r.use |= KZ_READ_USE_BRANCH;
          if (r.ctx == nullptr) r.ctx = cond_expr(co->getCond(), true);
        }
        cur = p;
        continue;
      }
      if (const auto* call = dyn_cast<CallExpr>(p)) {
        if (call->getCallee() == cur) {
          r.use |= KZ_READ_USE_CALLEE;
          break;
        }
        for (unsigned i = 0; i < call->getNumArgs(); ++i)
          if (call->getArg(i) == cur) {
            r.use |= KZ_READ_USE_CALL_ARG;
            if (r.call_key.empty()) {
              auto it = ext_call_keys_.find(call);
              if (it != ext_call_keys_.end()) {
                r.call_key = it->second;
                r.arg_index = i;
              }
            }
          }
        cur = p;
        continue;
      }
      if (const auto* as = dyn_cast<ArraySubscriptExpr>(p)) {
        r.use |= as->getIdx() == cur ? KZ_READ_USE_SUBSCRIPT : KZ_READ_USE_DEREF;
        break;
      }
      if (isa<MemberExpr>(p)) {  // the base of `->`
        r.use |= KZ_READ_USE_DEREF;
        break;
      }
      if (isa<ReturnStmt>(p)) {
        r.use |= KZ_READ_USE_RETURN;
        break;
      }
      const Expr* cond = nullptr;
      if (const auto* is = dyn_cast<IfStmt>(p)) cond = is->getCond();
      else if (const auto* ws = dyn_cast<WhileStmt>(p)) cond = ws->getCond();
      else if (const auto* ds = dyn_cast<DoStmt>(p)) cond = ds->getCond();
      else if (const auto* fs = dyn_cast<ForStmt>(p)) cond = fs->getCond();
      else if (const auto* ss = dyn_cast<SwitchStmt>(p)) cond = ss->getCond();
      if (cond != nullptr && cond == cur) {
        r.use |= KZ_READ_USE_BRANCH;
        if (r.ctx == nullptr) r.ctx = cond_expr(cond, true);
        break;
      }
      if (isa<InitListExpr>(p)) {
        r.use |= KZ_READ_USE_STORE;
        break;
      }
      if (isa<Expr>(p)) {
        cur = p;
        continue;
      }
      break;  // a statement: the value is not used
    }
    if (r.use == 0) r.use = KZ_READ_USE_OTHER;
    r.func_key = fkey_;
    r.pos = tu_.pos_of(named->getExprLoc());
    tu_.next_key(r.pos, fkey_ + "#r", &r.ordinal);
    r.text = tu_.source_text(lv->getSourceRange(), 80);
    r.guard = guard_from(0);
    if (!loops_.empty()) r.loop_key = m_.loops[loops_.back()->index].key;
    // F4 (PTR_FIELDS): the pointer field the path goes through nearest to its root, and the key of the row
    {
      std::set<const VarDecl*> seen;
      const PathInfo pi = analyze_path(named, &seen);
      r.ptr_steps = pi.ptr_steps;
      if (pi.base == KZ_ARGBASE_GLOBAL || pi.base == KZ_ARGBASE_GLOBAL_ELEMENT || pi.base == KZ_ARGBASE_FIELD_POINTER) r.root_object = pi.object;
      if (pi.base == KZ_ARGBASE_FIELD_POINTER && !(pi.flags & KZ_ARGFLOW_FLAG_BASE_VIA_LOCAL)) {
        r.ptr_type = pi.ptr_type;
        r.ptr_field = pi.ptr_field;
        r.ptr_rest = pi.rest;
        if (pi.ptr_index != nullptr) {
          r.ptr_index = to_expr(pi.ptr_index);
          int64_t c = 0;
          if (tu_.eval_int(pi.ptr_index->IgnoreParenCasts(), &c)) {
            r.ptr_index_is_const = true;
            r.ptr_index_const = c;
          } else if (const auto* d = dyn_cast<DeclRefExpr>(pi.ptr_index->IgnoreParenImpCasts())) {
            if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && vd->isLocalVarDecl() && !vd->isStaticLocal()) r.ptr_index_local = lid(vd);
          }
        }
      }
    }
    read_keys_[ce] = r.key();
    m_.reads.push_back(r);
  }
  // `*p = v`, `p->f = v`, `p[i] = v` (also `p->f.g`, `p->a[i]`) for a pointer parameter p
  static bool is_out_store(const Expr* lhs) {
    const Expr* e = lhs->IgnoreParenImpCasts();
    for (;;) {
      if (const auto* m = dyn_cast<MemberExpr>(e)) {
        if (m->isArrow()) {
          const auto* d = dyn_cast<DeclRefExpr>(m->getBase()->IgnoreParenImpCasts());
          return d != nullptr && isa<ParmVarDecl>(d->getDecl());
        }
        e = m->getBase()->IgnoreParenImpCasts();
        continue;
      }
      if (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) {
        const Expr* b = as->getBase()->IgnoreParenImpCasts();
        if (const auto* d = dyn_cast<DeclRefExpr>(b); d != nullptr && isa<ParmVarDecl>(d->getDecl())) return true;
        e = b;
        continue;
      }
      if (const auto* uo = dyn_cast<UnaryOperator>(e); uo != nullptr && uo->getOpcode() == UO_Deref) {
        const auto* d = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParenImpCasts());
        return d != nullptr && isa<ParmVarDecl>(d->getDecl());
      }
      return false;
    }
  }

  // ---- F2 RETURNS ----
  CfgIndex* ext_cfg() {
    if (ext_cfg_tried_) return ext_cfg_.get();
    ext_cfg_tried_ = true;
    CFG::BuildOptions bo;
    bo.setAllAlwaysAdd();
    std::unique_ptr<CFG> cfg = CFG::buildCFG(fd_, const_cast<Stmt*>(fd_->getBody()), &ctx_, bo);
    if (!cfg) return nullptr;
    auto ix = std::make_unique<CfgIndex>();
    ix->blocks.assign(cfg->getNumBlockIDs(), nullptr);
    for (const CFGBlock* b : *cfg) {
      if (b == nullptr || b->getBlockID() >= ix->blocks.size()) continue;
      ix->blocks[b->getBlockID()] = b;
      unsigned i = 0;
      for (const CFGElement& el : *b) {
        if (const std::optional<CFGStmt> cs = el.getAs<CFGStmt>()) ix->at.emplace(cs->getStmt(), std::make_pair(b->getBlockID(), i));
        ++i;
      }
    }
    ix->cfg = std::move(cfg);
    ext_cfg_ = std::move(ix);
    return ext_cfg_.get();
  }
  // A definition of `v` in a CFG element: its key (the assignment or increment; the VarDecl of a
  // declaration with an initializer), or null.
  static const void* def_of(const CFGElement& el, const VarDecl* v) {
    const std::optional<CFGStmt> cs = el.getAs<CFGStmt>();
    if (!cs) return nullptr;
    const Stmt* st = cs->getStmt();
    if (const auto* bo = dyn_cast<BinaryOperator>(st)) {
      if (!bo->isAssignmentOp()) return nullptr;
      const auto* d = dyn_cast<DeclRefExpr>(bo->getLHS()->IgnoreParenImpCasts());
      return d != nullptr && d->getDecl() == v ? bo : nullptr;
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(st)) {
      if (!uo->isIncrementDecrementOp()) return nullptr;
      const auto* d = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParenImpCasts());
      return d != nullptr && d->getDecl() == v ? uo : nullptr;
    }
    if (const auto* ds = dyn_cast<DeclStmt>(st)) {
      for (const Decl* d : ds->decls())
        if (d == v && v->getInit() != nullptr) return v;
    }
    return nullptr;
  }
  struct DefAt {
    const void* key = nullptr;
    unsigned block = 0, index = 0;
  };
  // The definitions of `v` that reach the element (block, index) (backward from it until a definition
  // on every path); `undef` when a path from the entry reaches it without one.
  std::vector<DefAt> reaching_defs(const CfgIndex& ix, const VarDecl* v, unsigned block, unsigned index, bool* undef) {
    std::vector<DefAt> out;
    std::set<const void*> found;
    std::set<unsigned> visited;
    std::vector<std::pair<unsigned, unsigned>> work{{block, index}};  // scan the block backwards from `index` (exclusive)
    *undef = false;
    while (!work.empty()) {
      const auto [b, from] = work.back();
      work.pop_back();
      const CFGBlock* blk = b < ix.blocks.size() ? ix.blocks[b] : nullptr;
      if (blk == nullptr) continue;
      bool hit = false;
      for (unsigned k = from; k > 0 && !hit; --k) {
        const void* d = def_of((*blk)[k - 1], v);
        if (d == nullptr) continue;
        hit = true;
        if (found.insert(d).second) out.push_back(DefAt{d, b, k - 1});
      }
      if (hit) continue;
      if (blk == &ix.cfg->getEntry()) {
        *undef = true;
        continue;
      }
      bool any_pred = false;
      for (const CFGBlock::AdjacentBlock& a : blk->preds()) {
        const CFGBlock* pb = a.getReachableBlock();
        if (pb == nullptr) continue;
        any_pred = true;
        if (visited.insert(pb->getBlockID()).second) work.push_back({pb->getBlockID(), static_cast<unsigned>(pb->size())});
      }
      (void)any_pred;  // a block without a reachable predecessor is not reached from the entry
    }
    return out;
  }
  // Is (to) reachable from just after (from) on the CFG?
  bool reaches(const CfgIndex& ix, const DefAt& from, const DefAt& to) {
    if (from.block == to.block && to.index > from.index) return true;
    std::set<unsigned> visited;
    std::vector<unsigned> work;
    auto push_succ = [&](unsigned b) {
      const CFGBlock* blk = b < ix.blocks.size() ? ix.blocks[b] : nullptr;
      if (blk == nullptr) return;
      for (const CFGBlock::AdjacentBlock& a : blk->succs())
        if (const CFGBlock* sb = a.getReachableBlock())
          if (visited.insert(sb->getBlockID()).second) work.push_back(sb->getBlockID());
    };
    push_succ(from.block);
    while (!work.empty()) {
      const unsigned b = work.back();
      work.pop_back();
      if (b == to.block) return true;
      push_succ(b);
    }
    return false;
  }
  // The value a definition stores (null for a compound assignment or an increment).
  static const Expr* def_value(const void* key, const VarDecl* v) {
    if (key == v) return v->getInit();
    if (const auto* bo = dyn_cast<BinaryOperator>(static_cast<const Stmt*>(key))) return bo->getOpcode() == BO_Assign ? bo->getRHS() : nullptr;
    return nullptr;
  }
  // The definition as written.
  SourceRange def_range(const void* key, const VarDecl* v) const {
    if (key == v) return v->getSourceRange();
    return static_cast<const Stmt*>(key)->getSourceRange();
  }
  SourceLocation def_loc(const void* key, const VarDecl* v) const {
    if (key == v) return v->getLocation();
    const Stmt* st = static_cast<const Stmt*>(key);
    if (const auto* bo = dyn_cast<BinaryOperator>(st)) return bo->getOperatorLoc();
    if (const auto* uo = dyn_cast<UnaryOperator>(st)) return uo->getOperatorLoc();
    return st->getBeginLoc();
  }

  // After the walk: one RETURNS row per (return statement with a value, alternative).
  void emit_returns() {
    for (const PendingReturn& pr : returns_) {
      const ReturnStmt* s = pr.stmt;
      const Expr* rv = s->getRetValue();
      if (rv == nullptr) continue;
      ReturnFact base;
      base.func_key = fkey_;
      base.pos = tu_.pos_of(s->getBeginLoc());
      tu_.next_key(base.pos, fkey_ + "#ret", &base.ordinal);
      base.guard = pr.guard;
      base.loop_key = pr.loop_key;
      if (!pr.loop_key.empty()) base.flags |= KZ_RET_FLAG_IN_LOOP;
      if (s->getBeginLoc().isMacroID()) base.flags |= KZ_RET_FLAG_IN_MACRO;
      const SourceLocation limit = s->getBeginLoc();
      std::vector<ReturnFact> alts;
      // a returned local with definitions of its own: the definitions that reach the return
      const VarDecl* v = nullptr;
      if (const auto* d = dyn_cast<DeclRefExpr>(rv->IgnoreParenCasts()))
        if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && vd->isLocalVarDecl() && !vd->isStaticLocal()) v = vd;
      auto lit = v != nullptr ? locals_.find(v) : locals_.end();
      const bool resolvable = lit != locals_.end() && !lit->second.addr_taken && !lit->second.defs.empty();
      bool no_cfg = false;
      CfgIndex* ix = resolvable ? ext_cfg() : nullptr;
      if (resolvable && (ix == nullptr || ix->at.count(s) == 0)) no_cfg = true;
      if (resolvable && !no_cfg) {
        const std::pair<unsigned, unsigned> at = ix->at.at(s);
        {
          bool undef = false;
          std::vector<DefAt> defs = reaching_defs(*ix, v, at.first, at.second, &undef);
          std::sort(defs.begin(), defs.end(), [&](const DefAt& a, const DefAt& b) {
            const SourceLocation la = sm_.getExpansionLoc(def_loc(a.key, v)), lb = sm_.getExpansionLoc(def_loc(b.key, v));
            if (la != lb) return la.getRawEncoding() < lb.getRawEncoding();
            return std::tie(a.block, a.index) < std::tie(b.block, b.index);
          });
          for (const DefAt& d : defs) {
            ReturnFact r = base;
            r.flags |= KZ_RET_FLAG_FROM_DEF;
            if (undef) r.flags |= KZ_RET_FLAG_UNDEF_PATH;
            auto il = def_in_loop_.find(d.key);
            if (il != def_in_loop_.end() && il->second) r.flags |= KZ_RET_FLAG_IN_LOOP;
            r.local = lid(v);
            const SourceLocation dl = def_loc(d.key, v);
            r.def = tu_.pos_of(dl);
            auto g = def_guards_.find(d.key);
            r.def_guard = g != def_guards_.end() ? g->second : nullptr;
            const Expr* rhs = def_value(d.key, v);
            bool changed = false;
            if (rhs != nullptr && !isa<InitListExpr>(rhs->IgnoreParens())) {
              r.value = unfold(to_expr(rhs), dl, &changed);
              r.text = tu_.source_text(rhs->getSourceRange(), 80);
              r.call_key = call_key_of(rhs);
            } else {
              r.value = pool().top("not a copy");
              r.text = tu_.source_text(def_range(d.key, v), 80);
            }
            std::vector<ExprRef> terms{r.guard, r.def_guard};
            for (const DefAt& o : defs)
              if (o.key != d.key && reaches(*ix, d, o)) {
                r.flags |= KZ_RET_FLAG_REDEFINED;
                auto ol = def_in_loop_.find(o.key);
                if (ol != def_in_loop_.end() && ol->second) r.flags |= KZ_RET_FLAG_IN_LOOP;  // its guard holds per iteration
                auto og = def_guards_.find(o.key);
                terms.push_back(negate(og != def_guards_.end() ? og->second : nullptr));
              }
            r.cond = unfold(and_unique(terms), limit, &changed);
            if (changed) r.flags |= KZ_RET_FLAG_UNFOLDED;
            alts.push_back(r);
          }
          if (defs.empty()) {  // no definition reaches it
            ReturnFact r = base;
            r.flags |= KZ_RET_FLAG_UNDEF_PATH;
            r.value = to_expr(rv);
            r.text = tu_.source_text(rv->getSourceRange(), 80);
            bool changed = false;
            r.cond = unfold(and_unique({r.guard}), limit, &changed);
            if (changed) r.flags |= KZ_RET_FLAG_UNFOLDED;
            alts.push_back(r);
          }
        }
      }
      if (alts.empty()) {
        ReturnFact r = base;
        if (no_cfg) r.flags |= KZ_RET_FLAG_NO_CFG;
        bool changed = false;
        r.value = unfold(to_expr(rv), limit, &changed);
        r.text = tu_.source_text(rv->getSourceRange(), 80);
        r.call_key = call_key_of(rv);
        r.cond = unfold(and_unique({r.guard}), limit, &changed);
        if (changed) r.flags |= KZ_RET_FLAG_UNFOLDED;
        alts.push_back(r);
      }
      for (size_t i = 0; i < alts.size(); ++i) {
        ReturnFact& r = alts[i];
        r.alt = static_cast<uint32_t>(i);
        r.n_alts = static_cast<uint32_t>(alts.size());
        r.vkind = value_kind(r.value);
        if (r.vkind == KZ_RET_VALUE_CONST) r.constant = r.value->value;
        m_.returns.push_back(r);
      }
    }
  }

  // ---- the extended facts F4 (spec.extended_facts; kz_facts.h revision 7) --------------------------------

  // The access path of an lvalue, from the location back to its root: the storage of the object at its
  // base, and the pointer step nearest to the root (the pointee of that pointer is the base object).
  struct PathInfo {
    uint8_t base = KZ_ARGBASE_NONE;
    std::string root, object;        // the root as named; the global object key of the base object
    uint32_t param = kNone;          // a parameter base
    uint32_t flags = 0;              // KZ_ARGFLOW_FLAG
    std::vector<const Expr*> idx;    // the subscripts, leaf first
    uint32_t ptr_steps = 0;
    const Expr* ptr_expr = nullptr;  // the pointer of the step nearest to the root
    const Expr* ptr_index = nullptr; // its subscript (null for `->` and `*`)
    std::string rest;                // the path from its pointee to the location (".f[2].g")
    std::string ptr_type, ptr_field; // a pointer field (or an array field of pointers)
  };
  // What a value is a copy of (ARGFLOW; the copy rule of the derives rows, extended to pointer conversions).
  struct Src {
    uint8_t src = KZ_ARGSRC_EXPR, base = KZ_ARGBASE_NONE, reason = KZ_DERIVES_REASON_NONE;
    uint32_t flags = 0, param = kNone, hops = 0;
    int64_t value = 0;
    std::string type, field, root, object;
    ExprRef chain = nullptr, index = nullptr;
    const Expr* read = nullptr;  // the conversion that reads the source (a READS row)
  };
  std::string step_text(const Expr* index) {
    int64_t c = 0;
    if (index != nullptr && tu_.eval_int(index->IgnoreParenCasts(), &c)) return "[" + std::to_string(c) + "]";
    return "[*]";
  }
  PathInfo analyze_path(const Expr* lv, std::set<const VarDecl*>* seen) {
    PathInfo pi;
    std::vector<std::string> steps;  // leaf first
    bool element = false;            // an array element between the root and the first pointer step
    auto pointer_step = [&](const Expr* p, const Expr* index) {
      ++pi.ptr_steps;
      pi.flags |= KZ_ARGFLOW_FLAG_THROUGH_POINTER;
      pi.ptr_expr = p->IgnoreParenImpCasts();
      pi.ptr_index = index;
      pi.rest.clear();
      for (size_t i = steps.size(); i > 0; --i) pi.rest += steps[i - 1];
      element = false;
    };
    const Expr* cur = lv;
    for (;;) {
      cur = cur->IgnoreParens();
      if (const auto* m = dyn_cast<MemberExpr>(cur)) {
        steps.push_back("." + m->getMemberDecl()->getNameAsString());
        if (m->isArrow()) {
          pointer_step(m->getBase(), nullptr);
          cur = m->getBase()->IgnoreParenImpCasts();
        } else {
          cur = m->getBase();
        }
        continue;
      }
      if (const auto* as = dyn_cast<ArraySubscriptExpr>(cur)) {
        pi.idx.push_back(as->getIdx());
        const Expr* b = as->getBase()->IgnoreParens();
        const auto* bc = dyn_cast<ImplicitCastExpr>(b);
        if (bc != nullptr && bc->getCastKind() == CK_ArrayToPointerDecay) {
          steps.push_back(step_text(as->getIdx()));
          element = true;
          cur = bc->getSubExpr();
        } else {
          pointer_step(b, as->getIdx());
          cur = b->IgnoreParenImpCasts();
        }
        continue;
      }
      if (const auto* uo = dyn_cast<UnaryOperator>(cur)) {
        if (uo->getOpcode() == UO_Deref) {
          pointer_step(uo->getSubExpr(), nullptr);
          cur = uo->getSubExpr()->IgnoreParenImpCasts();
          continue;
        }
        if (uo->getOpcode() == UO_AddrOf) {
          cur = uo->getSubExpr();
          continue;
        }
      }
      if (const auto* c = dyn_cast<CastExpr>(cur)) {
        cur = c->getSubExpr();
        continue;
      }
      break;
    }
    // the root
    bool root_global = false, root_local = false, root_call = false;
    if (const auto* d = dyn_cast<DeclRefExpr>(cur)) {
      if (const auto* pv = dyn_cast<ParmVarDecl>(d->getDecl())) {
        pi.root = pv->getNameAsString();
        root_local = true;  // a parameter object itself is the function's own copy
      } else if (const auto* vd = dyn_cast<VarDecl>(d->getDecl())) {
        root_global = vd->hasGlobalStorage();
        root_local = !root_global;
        pi.root = root_global ? vd->getNameAsString() : lid(vd);
        if (root_global) pi.object = tu_.object_key(vd);
      } else {
        pi.root = d->getDecl()->getNameAsString();
      }
    } else if (const auto* call = dyn_cast<CallExpr>(cur)) {
      pi.root = callee_text(call);
      root_call = true;
    } else {
      pi.root = cur->getStmtClassName();
    }
    if (pi.ptr_steps == 0) {
      pi.base = root_global ? (element ? KZ_ARGBASE_GLOBAL_ELEMENT : KZ_ARGBASE_GLOBAL) : root_local ? KZ_ARGBASE_LOCAL : root_call ? KZ_ARGBASE_CALL_RESULT : KZ_ARGBASE_OTHER;
      if (!root_global) pi.object.clear();
      return pi;
    }
    if (pi.ptr_steps > 1) pi.flags |= KZ_ARGFLOW_FLAG_MULTI_LEVEL;
    // the pointer nearest to the root decides the base: its pointee is the base object
    const Expr* p = pi.ptr_expr;
    if (const auto* as = dyn_cast<ArraySubscriptExpr>(p)) {  // an element of an array of pointers
      const auto* bc = dyn_cast<ImplicitCastExpr>(as->getBase()->IgnoreParens());
      if (bc != nullptr && bc->getCastKind() == CK_ArrayToPointerDecay) p = bc->getSubExpr()->IgnoreParens();
    }
    if (const auto* d = dyn_cast<DeclRefExpr>(p)) {
      if (const auto* pv = dyn_cast<ParmVarDecl>(d->getDecl())) {
        pi.base = KZ_ARGBASE_PARAM;
        pi.param = pv->getFunctionScopeIndex();
        pi.root = pv->getNameAsString();
        pi.object.clear();
        return pi;
      }
      if (const auto* vd = dyn_cast<VarDecl>(d->getDecl())) {
        if (vd->hasGlobalStorage()) {
          pi.base = KZ_ARGBASE_GLOBAL_POINTER;
          pi.root = vd->getNameAsString();
          pi.object = tu_.object_key(vd);
          return pi;
        }
        pi.base = KZ_ARGBASE_LOCAL_POINTER;
        pi.root = lid(vd);
        pi.object.clear();
        // a local pointer with a single copy definition: what it points at
        auto it = locals_.find(vd);
        if (it != locals_.end() && !it->second.addr_taken && it->second.defs.size() == 1 && it->second.defs.front().copy && it->second.defs.front().rhs != nullptr &&
            seen->insert(vd).second) {
          const Src v = trace_source(it->second.defs.front().rhs, seen);
          bool via = true;
          if (v.src == KZ_ARGSRC_ADDRESS && (v.base == KZ_ARGBASE_GLOBAL || v.base == KZ_ARGBASE_GLOBAL_ELEMENT)) {
            pi.base = v.base;
            pi.object = v.object;
            pi.root = v.root;
          } else if (v.src == KZ_ARGSRC_GLOBAL && (v.flags & KZ_ARGFLOW_FLAG_ARRAY_CONTENTS)) {
            pi.base = KZ_ARGBASE_GLOBAL_ELEMENT;
            pi.object = v.object;
            pi.root = v.root;
          } else if (v.src == KZ_ARGSRC_FIELD) {
            pi.base = KZ_ARGBASE_FIELD_POINTER;
            pi.ptr_type = v.type;
            pi.ptr_field = v.field;
            pi.object = v.object;
            pi.root = v.type + "." + v.field;
          } else if (v.src == KZ_ARGSRC_PARAM) {
            pi.base = KZ_ARGBASE_PARAM;
            pi.param = v.param;
            pi.root = v.root;
          } else if (v.src == KZ_ARGSRC_CALL) {
            pi.base = KZ_ARGBASE_CALL_RESULT;
            pi.root = v.root;
          } else {
            via = false;
          }
          if (via) pi.flags |= KZ_ARGFLOW_FLAG_BASE_VIA_LOCAL;
        }
        return pi;
      }
    }
    if (const auto* m = dyn_cast<MemberExpr>(p)) {
      if (const auto* fl = dyn_cast<FieldDecl>(m->getMemberDecl())) {
        pi.base = KZ_ARGBASE_FIELD_POINTER;
        pi.ptr_type = field_type_name(fl);
        pi.ptr_field = fl->getNameAsString();
        if (!root_global) pi.object.clear();
        return pi;
      }
    }
    if (const auto* call = dyn_cast<CallExpr>(p)) {
      pi.base = KZ_ARGBASE_CALL_RESULT;
      pi.root = callee_text(call);
      pi.object.clear();
      return pi;
    }
    pi.base = KZ_ARGBASE_OTHER;
    pi.object.clear();
    return pi;
  }
  // The subscripts of a path as a SEQ, root first, the single-definition locals before `at` unfolded.
  ExprRef path_index(const PathInfo& pi, SourceLocation at, uint32_t* flags) {
    if (pi.idx.empty()) return nullptr;
    std::vector<ExprRef> parts;
    for (size_t i = pi.idx.size(); i > 0; --i) parts.push_back(to_expr(pi.idx[i - 1]));
    bool changed = false;
    ExprRef e = unfold(pool().list(KZ_EXPR_OP_SEQ, parts), at, &changed);
    if (changed) *flags |= KZ_ARGFLOW_FLAG_INDEX_UNFOLDED;
    return e;
  }
  // The location an lvalue names, as the writers name it.
  void location_of(const Expr* lv, std::string* type, std::string* field) {
    const Expr* e = lv->IgnoreParens();
    while (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) {
      const auto* bc = dyn_cast<ImplicitCastExpr>(as->getBase()->IgnoreParens());
      if (bc == nullptr || bc->getCastKind() != CK_ArrayToPointerDecay) break;
      e = bc->getSubExpr()->IgnoreParens();
    }
    if (const auto* m = dyn_cast<MemberExpr>(e)) {
      if (const auto* fl = dyn_cast<FieldDecl>(m->getMemberDecl())) {
        *type = field_type_name(fl);
        *field = fl->getNameAsString();
      }
      return;
    }
    if (const auto* d = dyn_cast<DeclRefExpr>(e)) {
      if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && vd->hasGlobalStorage()) {
        *type = "<global>";
        *field = vd->getNameAsString();
      }
    }
  }
  Src trace_source(const Expr* e, std::set<const VarDecl*>* seen) {
    Src r;
    if (e == nullptr) {
      r.reason = KZ_DERIVES_REASON_NOT_COPY;
      return r;
    }
    const Expr* bare = e->IgnoreParenCasts();
    if (is_null_const(e)) {
      r.src = KZ_ARGSRC_CONST;
      r.flags |= KZ_ARGFLOW_FLAG_NULLPTR;
      r.chain = pool().constant(0);
      return r;
    }
    if (const auto* sl = dyn_cast<StringLiteral>(bare)) {
      r.src = KZ_ARGSRC_STRING;
      r.root = sl->getString().str();
      r.chain = pool().load(KZ_EXPR_REF_STRING, r.root);
      return r;
    }
    int64_t c = 0;
    if (!isa<CallExpr>(bare) && tu_.eval_int(e, &c)) {
      r.src = KZ_ARGSRC_CONST;
      r.value = c;
      r.chain = pool().constant(c);
      return r;
    }
    // the conversions that keep the value
    const Expr* s = e;
    for (;;) {
      s = s->IgnoreParens();
      const auto* ce = dyn_cast<CastExpr>(s);
      if (ce == nullptr) break;
      const CastKind k = ce->getCastKind();
      const Expr* sub = ce->getSubExpr();
      if (k == CK_LValueToRValue) {
        if (r.read == nullptr) r.read = ce;
        s = sub;
        continue;
      }
      if (k == CK_ArrayToPointerDecay) {
        r.flags |= KZ_ARGFLOW_FLAG_ARRAY_CONTENTS;
        if (r.read == nullptr) r.read = ce;
        s = sub;
        continue;
      }
      if (k == CK_NoOp || k == CK_FunctionToPointerDecay) {
        s = sub;
        continue;
      }
      if (k == CK_BitCast && ce->getType()->isPointerType() && sub->getType()->isPointerType()) {
        r.flags |= KZ_ARGFLOW_FLAG_POINTER_CONVERSION;
        s = sub;
        continue;
      }
      if (k == CK_IntegralCast && sub->getType()->isIntegerType() && ce->getType()->isIntegerType() && ctx_.getIntWidth(ce->getType()) >= ctx_.getIntWidth(sub->getType())) {
        s = sub;
        continue;
      }
      r.src = KZ_ARGSRC_EXPR;
      r.reason = KZ_DERIVES_REASON_NOT_COPY;
      r.chain = to_expr(e);
      return r;
    }
    if (const auto* d = dyn_cast<DeclRefExpr>(s)) {
      if (const auto* fd = dyn_cast<FunctionDecl>(d->getDecl())) {
        r.src = KZ_ARGSRC_FUNCTION;
        r.root = fd->getNameAsString();
        r.chain = pool().load(KZ_EXPR_REF_FUNC, r.root);
        return r;
      }
      if (const auto* pv = dyn_cast<ParmVarDecl>(d->getDecl())) {
        r.src = KZ_ARGSRC_PARAM;
        r.param = pv->getFunctionScopeIndex();
        r.root = pv->getNameAsString();
        r.chain = pool().load(KZ_EXPR_REF_PARAM, r.root, r.param);
        return r;
      }
      if (const auto* vd = dyn_cast<VarDecl>(d->getDecl())) {
        if (vd->hasGlobalStorage()) {
          r.src = KZ_ARGSRC_GLOBAL;
          r.base = KZ_ARGBASE_GLOBAL;
          r.type = "<global>";
          r.field = r.root = vd->getNameAsString();
          r.object = tu_.object_key(vd);
          r.chain = pool().load(KZ_EXPR_REF_GLOBAL, r.root);
          return r;
        }
        r.src = KZ_ARGSRC_LOCAL;
        r.base = KZ_ARGBASE_LOCAL;
        r.root = lid(vd);
        r.chain = pool().load(KZ_EXPR_REF_LOCAL, r.root);
        if (r.flags & KZ_ARGFLOW_FLAG_ARRAY_CONTENTS) return r;  // a local array: its own storage
        auto it = locals_.find(vd);
        if (it == locals_.end() || it->second.defs.empty()) r.reason = KZ_DERIVES_REASON_NO_DEF;
        else if (it->second.addr_taken) r.reason = KZ_DERIVES_REASON_ADDRESS_TAKEN;
        else if (it->second.defs.size() > 1) r.reason = KZ_DERIVES_REASON_MULTI_DEF;
        else if (!it->second.defs.front().copy || it->second.defs.front().rhs == nullptr || isa<InitListExpr>(it->second.defs.front().rhs->IgnoreParens()))
          r.reason = KZ_DERIVES_REASON_NOT_COPY;
        else if (!seen->insert(vd).second) r.reason = KZ_DERIVES_REASON_NOT_COPY;  // a copy of itself
        if (r.reason != KZ_DERIVES_REASON_NONE) return r;
        Src inner = trace_source(it->second.defs.front().rhs, seen);
        inner.hops += 1;
        inner.flags |= r.flags & ~static_cast<uint32_t>(KZ_ARGFLOW_FLAG_ARRAY_CONTENTS);
        inner.chain = pool().make(KZ_EXPR_OP_COPY, 0, 0, lid(vd), inner.chain, nullptr);
        return inner;
      }
    }
    if (isa<MemberExpr>(s) || isa<ArraySubscriptExpr>(s) || (isa<UnaryOperator>(s) && cast<UnaryOperator>(s)->getOpcode() == UO_Deref)) {
      const PathInfo pi = analyze_path(s, seen);
      if (const auto* m = dyn_cast<MemberExpr>(s)) {
        r.src = KZ_ARGSRC_FIELD;
        if (const auto* fl = dyn_cast<FieldDecl>(m->getMemberDecl())) {
          r.type = field_type_name(fl);
          r.field = fl->getNameAsString();
        }
      } else if (isa<ArraySubscriptExpr>(s) && pi.ptr_steps == 0 && (pi.base == KZ_ARGBASE_GLOBAL || pi.base == KZ_ARGBASE_GLOBAL_ELEMENT)) {
        r.src = KZ_ARGSRC_ELEMENT;
        location_of(s, &r.type, &r.field);
      } else {
        r.src = KZ_ARGSRC_DEREF;
        location_of(s, &r.type, &r.field);
      }
      r.base = pi.base;
      r.param = pi.param;
      r.flags |= pi.flags;
      r.object = pi.object;
      r.root = pi.base == KZ_ARGBASE_FIELD_POINTER ? pi.ptr_type + "." + pi.ptr_field : pi.root;
      r.index = path_index(pi, s->getBeginLoc(), &r.flags);
      r.chain = to_expr(s);
      return r;
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(s); uo != nullptr && uo->getOpcode() == UO_AddrOf) {
      const Expr* lv = uo->getSubExpr()->IgnoreParens();
      if (const auto* d = dyn_cast<DeclRefExpr>(lv); d != nullptr && isa<FunctionDecl>(d->getDecl())) {
        r.src = KZ_ARGSRC_FUNCTION;
        r.root = d->getDecl()->getNameAsString();
        r.chain = pool().load(KZ_EXPR_REF_FUNC, r.root);
        return r;
      }
      const PathInfo pi = analyze_path(lv, seen);
      r.src = KZ_ARGSRC_ADDRESS;
      location_of(lv, &r.type, &r.field);
      r.base = pi.base;
      r.param = pi.param;
      r.flags |= pi.flags;
      r.object = pi.object;
      r.root = pi.base == KZ_ARGBASE_FIELD_POINTER ? pi.ptr_type + "." + pi.ptr_field : pi.root;
      r.index = path_index(pi, s->getBeginLoc(), &r.flags);
      r.chain = to_expr(s);
      r.read = nullptr;  // an address: nothing is read at the location
      return r;
    }
    if (const auto* call = dyn_cast<CallExpr>(s)) {
      r.src = KZ_ARGSRC_CALL;
      r.root = callee_text(call);
      r.reason = KZ_DERIVES_REASON_FROM_CALL;
      r.chain = to_expr(s);
      return r;
    }
    r.src = KZ_ARGSRC_EXPR;
    r.reason = (isa<UnaryOperator>(s) || isa<BinaryOperator>(s) || isa<ConditionalOperator>(s)) ? KZ_DERIVES_REASON_NOT_COPY : KZ_DERIVES_REASON_NOT_SHARED;
    r.chain = to_expr(s);
    return r;
  }

  // ARGFLOW: one row per argument of the call.
  void note_argflows(const CallExpr* ce, const FunctionDecl* callee, const std::string& ckey) {
    const ExprRef guard = guard_from(0);
    for (unsigned i = 0; i < ce->getNumArgs(); ++i) {
      std::set<const VarDecl*> seen;
      const Expr* arg = ce->getArg(i);
      const Src src = trace_source(arg, &seen);
      ArgFlowFact a;
      a.call_key = ckey;
      a.func_key = fkey_;
      a.arg_index = i;
      a.src = src.src;
      a.base = src.base;
      a.reason = src.reason;
      a.flags = src.flags;
      if (callee != nullptr && i >= callee->getNumParams()) a.flags |= KZ_ARGFLOW_FLAG_VARIADIC;
      if (arg->getBeginLoc().isMacroID()) a.flags |= KZ_ARGFLOW_FLAG_IN_MACRO;
      a.param = src.param;
      a.hops = src.hops;
      a.value = src.value;
      a.type = src.type;
      a.field = src.field;
      a.root = src.root;
      a.object = src.object;
      a.text = tu_.source_text(arg->getSourceRange(), 80);
      a.chain = src.chain;
      a.index = src.index;
      a.guard = guard;
      if (src.read != nullptr) pending_reads_.push_back({m_.argflows.size(), src.read});
      m_.argflows.push_back(a);
    }
  }
  // PARAM_FLOW uses of depth 0 (the call arguments are the ARGFLOW rows of a PARAM source).
  void add_param_use(uint32_t param, uint8_t kind, uint16_t flags, const Stmt* at, ExprRef expr) {
    ParamUse u;
    u.func_key = fkey_;
    u.param = param;
    u.kind = kind;
    u.flags = flags;
    if (!loops_.empty()) u.flags |= KZ_PFLOW_FLAG_IN_LOOP;
    u.pos = tu_.pos_of(at->getBeginLoc());
    tu_.next_key(u.pos, fkey_ + "#pu", &u.ordinal);
    u.expr = expr;
    u.guard = guard_from(0);
    u.text = tu_.source_text(at->getSourceRange(), 80);
    m_.param_uses.push_back(u);
  }
  void note_param_compare(const BinaryOperator* bo) {
    std::set<uint32_t> done;
    for (const Expr* side : {bo->getLHS(), bo->getRHS()}) {
      std::set<const VarDecl*> seen;
      const Src src = trace_source(side, &seen);
      if (src.src == KZ_ARGSRC_PARAM && done.insert(src.param).second) add_param_use(src.param, KZ_PFLOW_KIND_COMPARE, 0, bo, to_expr(bo));
    }
  }
  // A condition that is a copy of a parameter (not a comparison: those are rows of their own).
  void note_param_truth(const Expr* c, uint16_t flags) {
    if (c == nullptr) return;
    const Expr* b = c->IgnoreParenImpCasts();
    if (const auto* bo = dyn_cast<BinaryOperator>(b); bo != nullptr && (bo->isComparisonOp() || bo->isLogicalOp())) return;
    if (const auto* uo = dyn_cast<UnaryOperator>(b); uo != nullptr && uo->getOpcode() == UO_LNot) return;
    std::set<const VarDecl*> seen;
    const Src src = trace_source(c, &seen);
    if (src.src != KZ_ARGSRC_PARAM) return;
    if (flags & KZ_PFLOW_FLAG_SWITCH) add_param_use(src.param, KZ_PFLOW_KIND_COMPARE, flags, c, to_expr(c));
    else add_param_use(src.param, KZ_PFLOW_KIND_COMPARE, flags | KZ_PFLOW_FLAG_TRUTH_TEST, c, cond_expr(c, true));
  }
  void note_param_subscript(const ArraySubscriptExpr* as) {
    std::set<const VarDecl*> seen;
    const Src src = trace_source(as->getIdx(), &seen);
    if (src.src == KZ_ARGSRC_PARAM) add_param_use(src.param, KZ_PFLOW_KIND_SUBSCRIPT, 0, as, to_expr(as));
  }
  // `lhs = rhs`: a parameter stored to a location (not to a local: a copy that the tracing follows), and
  // the address of a global stored to a pointer field (PTR_FIELDS).
  void note_flow_store(const BinaryOperator* bo) {
    const Expr* lhs = bo->getLHS()->IgnoreParenImpCasts();
    std::set<const VarDecl*> seen;
    const Src rs = trace_source(bo->getRHS(), &seen);
    bool local_dest = false;
    if (const auto* d = dyn_cast<DeclRefExpr>(lhs))
      if (const auto* vd = dyn_cast<VarDecl>(d->getDecl())) local_dest = !vd->hasGlobalStorage();
    if (rs.src == KZ_ARGSRC_PARAM && !local_dest) add_param_use(rs.param, KZ_PFLOW_KIND_STORE, is_out_store(bo->getLHS()) ? KZ_PFLOW_FLAG_THROUGH_POINTER_PARAM : 0, bo, to_expr(bo->getLHS()));
    const auto* me = dyn_cast<MemberExpr>(lhs);
    const auto* fl = me != nullptr ? dyn_cast<FieldDecl>(me->getMemberDecl()) : nullptr;
    if (fl == nullptr || !fl->getType()->isPointerType() || is_fp_type(fl->getType())) return;
    // the value: the address of a global object, of an element of a global array, or a global array that decays
    std::string object;
    int64_t elem = 0;
    bool ok = false;
    if (rs.src == KZ_ARGSRC_GLOBAL && (rs.flags & KZ_ARGFLOW_FLAG_ARRAY_CONTENTS)) {
      object = rs.object;
      ok = true;
    } else if (rs.src == KZ_ARGSRC_ADDRESS && rs.base == KZ_ARGBASE_GLOBAL && rs.type == "<global>") {
      object = rs.object;
      ok = true;
    } else if (rs.src == KZ_ARGSRC_ADDRESS && rs.base == KZ_ARGBASE_GLOBAL_ELEMENT && rs.type == "<global>" && rs.index != nullptr && rs.index->op == KZ_EXPR_OP_SEQ &&
               rs.index->b == nullptr && rs.index->a != nullptr && rs.index->a->op == KZ_EXPR_OP_CONST && rs.index->a->a == nullptr) {
      object = rs.object;  // &G[c]
      elem = rs.index->a->value;
      ok = true;
    }
    if (!ok || object.empty()) return;
    std::set<const VarDecl*> seen2;
    const PathInfo wp = analyze_path(me, &seen2);
    PtrWriteFact w;
    w.kind = KZ_PTRF_KIND_ADDRESS_WRITE;
    w.func_key = fkey_;
    w.type = field_type_name(fl);
    w.field = fl->getNameAsString();
    w.root_global = wp.ptr_steps == 0 && (wp.base == KZ_ARGBASE_GLOBAL || wp.base == KZ_ARGBASE_GLOBAL_ELEMENT);
    w.root = w.root_global ? wp.object : wp.root;
    w.object = object;
    w.elem = elem;
    w.text = tu_.source_text(bo->getRHS()->getSourceRange(), 80);
    w.in_loop = !loops_.empty();
    w.pos = tu_.pos_of(bo->getBeginLoc());
    tu_.next_key(w.pos, fkey_ + "#pw", &w.ordinal);
    w.guard = guard_from(0);
    ext_ptr_writes_.push_back(PtrWriteAt{m_.ptr_writes.size(), bo, w.type, w.field, w.in_loop});
    m_.ptr_writes.push_back(w);
  }

  // The dominators of the blocks of a body's control-flow graph (blocks not reached from the entry
  // dominate nothing and are dominated by nothing).
  struct Dominators {
    std::vector<std::vector<bool>> dom;  // dom[b][a]: a dominates b
    std::vector<bool> reach;
  };
  Dominators dominators(const CfgIndex& ix) {
    Dominators d;
    const size_t n = ix.blocks.size();
    d.reach.assign(n, false);
    const unsigned entry = ix.cfg->getEntry().getBlockID();
    std::vector<unsigned> work{entry};
    if (entry < n) d.reach[entry] = true;
    while (!work.empty()) {
      const unsigned b = work.back();
      work.pop_back();
      const CFGBlock* blk = b < n ? ix.blocks[b] : nullptr;
      if (blk == nullptr) continue;
      for (const CFGBlock::AdjacentBlock& a : blk->succs())
        if (const CFGBlock* sb = a.getReachableBlock())
          if (sb->getBlockID() < n && !d.reach[sb->getBlockID()]) {
            d.reach[sb->getBlockID()] = true;
            work.push_back(sb->getBlockID());
          }
    }
    d.dom.assign(n, std::vector<bool>(n, true));
    if (entry < n) {
      d.dom[entry].assign(n, false);
      d.dom[entry][entry] = true;
    }
    for (bool changed = true; changed;) {
      changed = false;
      for (unsigned b = 0; b < n; ++b) {
        if (b == entry || !d.reach[b] || ix.blocks[b] == nullptr) continue;
        std::vector<bool> nd(n, true);
        bool any = false;
        for (const CFGBlock::AdjacentBlock& a : ix.blocks[b]->preds()) {
          const CFGBlock* pb = a.getReachableBlock();
          if (pb == nullptr || pb->getBlockID() >= n || !d.reach[pb->getBlockID()]) continue;
          any = true;
          const std::vector<bool>& pd = d.dom[pb->getBlockID()];
          for (size_t k = 0; k < n; ++k) nd[k] = nd[k] && pd[k];
        }
        if (!any) nd.assign(n, false);
        nd[b] = true;
        if (nd != d.dom[b]) {
          d.dom[b] = std::move(nd);
          changed = true;
        }
      }
    }
    return d;
  }
  static bool dominates(const Dominators& d, std::pair<unsigned, unsigned> a, std::pair<unsigned, unsigned> b) {
    if (a.first >= d.reach.size() || b.first >= d.reach.size() || !d.reach[a.first] || !d.reach[b.first]) return false;
    if (a.first == b.first) return a.second < b.second;
    return d.dom[b.first][a.first];
  }

  // After the walk: the READS rows of the ARGFLOW sources, and the order of each address write.
  void finish_flow_facts() {
    for (const auto& pr : pending_reads_) {
      auto it = read_keys_.find(pr.second);
      if (it != read_keys_.end()) m_.argflows[pr.first].read_key = it->second;
    }
    if (ext_ptr_writes_.empty()) return;
    CfgIndex* ix = ext_cfg();
    if (ix == nullptr) return;  // no control-flow graph: nothing is known to come after the writes
    const Dominators dom = dominators(*ix);
    for (const PtrWriteAt& pw : ext_ptr_writes_) {
      auto wit = ix->at.find(pw.at);
      if (wit == ix->at.end()) continue;
      PtrWriteFact& f = m_.ptr_writes[pw.row];
      for (const auto& kv : ext_call_keys_) {
        auto cit = ix->at.find(kv.first);
        if (cit != ix->at.end() && dominates(dom, wit->second, cit->second)) f.after_calls.insert(kv.second);
      }
      for (const auto& kv : read_keys_) {
        auto rit = ix->at.find(kv.first);
        if (rit != ix->at.end() && dominates(dom, wit->second, rit->second)) f.after_reads.insert(kv.second);
      }
      if (pw.in_loop) continue;  // a later iteration repeats the earlier writes
      for (const ExtWrite& w : ext_writes_) {
        if (w.at == pw.at) continue;
        const bool same = (w.kind == KZ_FWRITE_KIND_FIELD || w.kind == KZ_FWRITE_KIND_OUTARG) ? (w.type == pw.type && w.field == pw.field)
                                                                                              : (w.kind == KZ_FWRITE_KIND_RECORD && w.type == pw.type);
        if (!same) continue;
        auto oit = ix->at.find(w.at);
        if (oit != ix->at.end() && dominates(dom, oit->second, wit->second)) f.overwritten.insert(w.pos.str());
      }
    }
  }


  // ---- F5 (spec.extended_facts): the identity of a call argument as a lock (model.hpp LockIdNode) ----
  static LockId lnode(uint8_t k, int64_t v, const std::string& name, LockId a = nullptr, LockId b = nullptr) {
    auto n = std::make_shared<LockIdNode>();
    n->k = k;
    n->v = v;
    n->name = name;
    n->a = std::move(a);
    n->b = std::move(b);
    return n;
  }
  static LockId lunknown(const std::string& why) { return lnode(LockIdNode::UNKNOWN, 0, why); }
  // An unknown part stays a node of its own, so that the rest of the expression (the field the mutex is,
  // the table it is an element of) is kept.
  static LockId laddr(LockId lv) {
    if (lv->k == LockIdNode::DEREF) return lv->a;  // &*p is p
    return lnode(LockIdNode::ADDR, 0, "", lv);
  }
  static LockId lderef(LockId v) {
    if (v->k == LockIdNode::ADDR) return v->a;  // *&x is x
    return lnode(LockIdNode::DEREF, 0, "", v);
  }
  // A parameter that the body never assigns and whose address it never takes.
  bool param_unmodified(const ParmVarDecl* pv) const {
    auto it = locals_.find(pv);
    return it == locals_.end() || (it->second.defs.empty() && !it->second.addr_taken);
  }
  // The value of an expression: copies through locals with a single copy definition are followed (the
  // rule of the derives rows); conversions keep the value or the address.
  LockId ident_value(const Expr* e, std::set<const VarDecl*>* seen) {
    if (e == nullptr) return lunknown("no expression");
    e = e->IgnoreParens();
    int64_t c = 0;
    if (!isa<CallExpr>(e) && !e->getType()->isPointerType() && tu_.eval_int(e->IgnoreParenCasts(), &c)) return lnode(LockIdNode::CONST, c, "");
    if (const auto* ic = dyn_cast<ImplicitCastExpr>(e)) {
      const Expr* sub = ic->getSubExpr();
      switch (ic->getCastKind()) {
        case CK_LValueToRValue: {
          const Expr* x = sub->IgnoreParens();
          if (const auto* d = dyn_cast<DeclRefExpr>(x)) {
            if (const auto* pv = dyn_cast<ParmVarDecl>(d->getDecl()))
              return param_unmodified(pv) ? lnode(LockIdNode::PARAM, pv->getFunctionScopeIndex(), "") : lunknown("an assigned parameter");
            if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && !vd->hasGlobalStorage()) {
              auto it = locals_.find(vd);
              if (it == locals_.end() || it->second.addr_taken || it->second.defs.size() != 1 || !it->second.defs.front().copy ||
                  it->second.defs.front().rhs == nullptr || !seen->insert(vd).second)
                return lunknown("the local " + lid(vd));
              return ident_value(it->second.defs.front().rhs, seen);
            }
          }
          const LockId lv = ident_lvalue(x, seen);
          return lv->k == LockIdNode::UNKNOWN ? lv : lnode(LockIdNode::LOAD, 0, "", lv);
        }
        case CK_ArrayToPointerDecay: return laddr(lnode(LockIdNode::INDEX, 0, "", ident_lvalue(sub, seen), lnode(LockIdNode::CONST, 0, "")));
        case CK_FunctionToPointerDecay: return lunknown("a function");
        default: return ident_value(sub, seen);
      }
    }
    if (const auto* cc = dyn_cast<CStyleCastExpr>(e)) return ident_value(cc->getSubExpr(), seen);
    if (const auto* uo = dyn_cast<UnaryOperator>(e); uo != nullptr && uo->getOpcode() == UO_AddrOf) return laddr(ident_lvalue(uo->getSubExpr(), seen));
    if (const auto* call = dyn_cast<CallExpr>(e)) return lunknown("the result of " + callee_text(call));
    return lunknown("an expression");
  }
  LockId ident_lvalue(const Expr* e, std::set<const VarDecl*>* seen) {
    e = e->IgnoreParens();
    if (const auto* ic = dyn_cast<ImplicitCastExpr>(e); ic != nullptr && ic->getCastKind() == CK_NoOp) return ident_lvalue(ic->getSubExpr(), seen);
    if (const auto* d = dyn_cast<DeclRefExpr>(e)) {
      if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && vd->hasGlobalStorage()) return lnode(LockIdNode::GLOBAL, 0, tu_.object_key(vd));
      return lunknown("a local object");
    }
    if (const auto* me = dyn_cast<MemberExpr>(e)) {
      const auto* fl = dyn_cast<FieldDecl>(me->getMemberDecl());
      if (fl == nullptr) return lunknown("a member");
      const LockId base = me->isArrow() ? lderef(ident_value(me->getBase(), seen)) : ident_lvalue(me->getBase(), seen);
      return lnode(LockIdNode::FIELD, 0, field_type_name(fl) + "." + fl->getNameAsString(), base);
    }
    if (const auto* as = dyn_cast<ArraySubscriptExpr>(e)) {
      const LockId bv = ident_value(as->getBase(), seen);
      const LockId iv = ident_value(as->getIdx(), seen);
      if (bv->k == LockIdNode::ADDR && bv->a->k == LockIdNode::INDEX && bv->a->b->k == LockIdNode::CONST && bv->a->b->v == 0)
        return lnode(LockIdNode::INDEX, 0, "", bv->a->a, iv);
      return lnode(LockIdNode::INDEX, 0, "", lderef(bv), iv);
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(e); uo != nullptr && uo->getOpcode() == UO_Deref) return lderef(ident_value(uo->getSubExpr(), seen));
    return lunknown("an lvalue");
  }

  // ---- F5: the lock-set skeleton of the body (model.hpp LockCfg) ----
  // The test a branch makes of a local or of a call result against a constant; `invert` for `!`.
  LockTest lock_test(const Expr* e, bool invert) {
    LockTest t;
    if (e == nullptr) return t;
    e = e->IgnoreParenCasts();
    if (const auto* uo = dyn_cast<UnaryOperator>(e); uo != nullptr && uo->getOpcode() == UO_LNot) return lock_test(uo->getSubExpr(), !invert);
    auto subject = [&](const Expr* x, LockTest* out) {
      x = x->IgnoreParenCasts();
      if (const auto* d = dyn_cast<DeclRefExpr>(x))
        if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && !vd->hasGlobalStorage()) {
          out->subject = isa<ParmVarDecl>(vd) ? "param:" + vd->getNameAsString() : lid(vd);
          return true;
        }
      if (const auto* call = dyn_cast<CallExpr>(x)) {
        auto it = ext_call_keys_.find(call);
        if (it == ext_call_keys_.end()) return false;
        out->on_call = true;
        out->subject = it->second;
        return true;
      }
      return false;
    };
    if (const auto* bo = dyn_cast<BinaryOperator>(e); bo != nullptr && (bo->getOpcode() == BO_EQ || bo->getOpcode() == BO_NE)) {
      int64_t c = 0;
      const Expr* other = nullptr;
      if (tu_.eval_int(bo->getRHS()->IgnoreParenCasts(), &c)) other = bo->getLHS();
      else if (tu_.eval_int(bo->getLHS()->IgnoreParenCasts(), &c)) other = bo->getRHS();
      if (other == nullptr || !subject(other, &t)) return LockTest();
      t.value = c;
      t.op = (bo->getOpcode() == BO_EQ) != invert ? 1 : 2;
      return t;
    }
    if (!subject(e, &t)) return LockTest();
    t.value = 0;
    t.op = invert ? 1 : 2;  // `x` is x != 0, `!x` is x == 0
    return t;
  }
  // A definition of a local in a CFG element: (local id, value kind, value, call key).
  bool lock_def(const Stmt* st, LockEvent* ev) {
    auto value_of = [&](const Expr* rhs) {
      if (rhs == nullptr) return;
      const Expr* x = rhs->IgnoreParenCasts();
      int64_t c = 0;
      if (const auto* call = dyn_cast<CallExpr>(x)) {
        auto it = ext_call_keys_.find(call);
        if (it != ext_call_keys_.end()) {
          ev->vkind = 2;
          ev->call = it->second;
        }
      } else if (tu_.eval_int(x, &c)) {
        ev->vkind = 1;
        ev->value = c;
      }
    };
    ev->kind = LockEvent::DEF;
    if (const auto* bo = dyn_cast<BinaryOperator>(st)) {
      if (!bo->isAssignmentOp()) return false;
      const auto* d = dyn_cast<DeclRefExpr>(bo->getLHS()->IgnoreParenImpCasts());
      const auto* vd = d != nullptr ? dyn_cast<VarDecl>(d->getDecl()) : nullptr;
      if (vd == nullptr || vd->hasGlobalStorage()) return false;
      ev->key = isa<ParmVarDecl>(vd) ? "param:" + vd->getNameAsString() : lid(vd);
      if (bo->getOpcode() == BO_Assign) value_of(bo->getRHS());
      return true;
    }
    if (const auto* uo = dyn_cast<UnaryOperator>(st)) {
      if (!uo->isIncrementDecrementOp()) return false;
      const auto* d = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParenImpCasts());
      const auto* vd = d != nullptr ? dyn_cast<VarDecl>(d->getDecl()) : nullptr;
      if (vd == nullptr || vd->hasGlobalStorage()) return false;
      ev->key = isa<ParmVarDecl>(vd) ? "param:" + vd->getNameAsString() : lid(vd);
      return true;
    }
    if (const auto* ds = dyn_cast<DeclStmt>(st)) {
      for (const Decl* dd : ds->decls()) {
        const auto* vd = dyn_cast<VarDecl>(dd);
        if (vd == nullptr || vd->hasGlobalStorage() || vd->getInit() == nullptr) continue;
        ev->key = lid(vd);
        value_of(vd->getInit());
        return true;
      }
    }
    return false;
  }
  static LockEvent lock_event(uint8_t kind, const std::string& key) {
    LockEvent ev;
    ev.kind = kind;
    ev.key = key;
    return ev;
  }
  void record_lock_cfg() {
    CfgIndex* ix = ext_cfg();
    if (ix == nullptr) return;  // no control-flow graph: LOCKSET reports the body as such
    std::map<const Stmt*, std::vector<uint32_t>> writes_at;
    for (const ExtWrite& w : ext_writes_) writes_at[w.at].push_back(w.site);
    // a write or a read whose statement is no element of its own is placed at the nearest enclosing element
    std::map<const Stmt*, std::vector<std::pair<uint8_t, std::string>>> extra;  // element -> (kind, key) of such events
    std::map<const Stmt*, std::vector<uint32_t>> extra_writes;
    auto element_of = [&](const Stmt* s) -> const Stmt* {
      for (int guard = 0; s != nullptr && guard < 64; ++guard) {
        if (ix->at.count(s) != 0) return s;
        s = parent_stmt(s);
      }
      return nullptr;
    };
    for (const auto& kv : read_keys_)
      if (ix->at.count(kv.first) == 0)
        if (const Stmt* el = element_of(kv.first)) extra[el].push_back({LockEvent::READ, kv.second});
    for (const auto& kv : writes_at)
      if (ix->at.count(kv.first) == 0)
        if (const Stmt* el = element_of(kv.first)) extra_writes[el].insert(extra_writes[el].end(), kv.second.begin(), kv.second.end());
    LockCfg g;
    g.blocks.resize(ix->blocks.size());
    g.entry = ix->cfg->getEntry().getBlockID();
    g.exit = ix->cfg->getExit().getBlockID();
    for (size_t b = 0; b < ix->blocks.size(); ++b) {
      const CFGBlock* blk = ix->blocks[b];
      if (blk == nullptr) continue;
      LockBlock& out = g.blocks[b];
      out.noreturn = blk->hasNoReturnElement();
      for (const CFGElement& el : *blk) {
        const std::optional<CFGStmt> cs = el.getAs<CFGStmt>();
        if (!cs) continue;
        const Stmt* st = cs->getStmt();
        auto eit = extra.find(st);
        if (eit != extra.end())
          for (const auto& x : eit->second) out.events.push_back(lock_event(x.first, x.second));
        if (const auto* call = dyn_cast<CallExpr>(st)) {
          auto it = ext_call_keys_.find(call);
          if (it != ext_call_keys_.end()) out.events.push_back(lock_event(LockEvent::CALL, it->second));
        }
        if (const auto* ce = dyn_cast<Expr>(st)) {
          auto it = read_keys_.find(ce);
          if (it != read_keys_.end()) out.events.push_back(lock_event(LockEvent::READ, it->second));
        }
        auto wit = writes_at.find(st);
        if (wit != writes_at.end())
          for (uint32_t w : wit->second) {
            LockEvent ev;
            ev.kind = LockEvent::WRITE;
            ev.index = w;
            out.events.push_back(ev);
          }
        auto xit = extra_writes.find(st);
        if (xit != extra_writes.end())
          for (uint32_t w : xit->second) {
            LockEvent ev;
            ev.kind = LockEvent::WRITE;
            ev.index = w;
            out.events.push_back(ev);
          }
        LockEvent def;
        if (lock_def(st, &def)) out.events.push_back(def);
        if (const auto* rs = dyn_cast<ReturnStmt>(st)) {
          LockEvent ev;
          ev.kind = LockEvent::RET;
          const Expr* v = rs->getRetValue();
          if (v == nullptr) {
            ev.vkind = 4;
          } else {
            const Expr* x = v->IgnoreParenCasts();
            int64_t c = 0;
            if (const auto* call = dyn_cast<CallExpr>(x)) {
              auto it = ext_call_keys_.find(call);
              if (it != ext_call_keys_.end()) {
                ev.vkind = 2;
                ev.call = it->second;
              }
            } else if (tu_.eval_int(x, &c)) {
              ev.vkind = 1;
              ev.value = c;
            } else if (const auto* d = dyn_cast<DeclRefExpr>(x)) {
              if (const auto* vd = dyn_cast<VarDecl>(d->getDecl()); vd != nullptr && !vd->hasGlobalStorage()) {
                ev.vkind = 3;
                ev.key = isa<ParmVarDecl>(vd) ? "param:" + vd->getNameAsString() : lid(vd);
              }
            }
          }
          out.events.push_back(ev);
        }
      }
      for (const CFGBlock::AdjacentBlock& a : blk->succs()) {
        const CFGBlock* r = a.getReachableBlock();
        out.succ.push_back(r != nullptr ? r->getBlockID() : kNone);
      }
      const Stmt* term = blk->getTerminatorStmt();
      if (out.succ.size() == 2 && term != nullptr && !isa<SwitchStmt>(term)) {
        out.branch = true;
        out.test = lock_test(blk->getLastCondition(), false);
      }
    }
    m_.lock_cfgs[fkey_] = std::move(g);
  }

  struct Guard {
    const Expr* cond;
    bool polarity;
  };

  TuReader& tu_;
  Model& m_;
  const ExtractSpec& spec_;
  ASTContext& ctx_;
  const FunctionDecl* fd_;
  const SourceManager& sm_;
  std::string fkey_, fname_;
  FuncSummary summary_;
  std::map<const VarDecl*, LocalInfo> locals_;
  std::set<const VarDecl*> tested_;
  std::vector<Guard> guards_;
  std::vector<std::unique_ptr<LoopCtx>> loops_;
  std::vector<LoopCtx*> breakables_;
  std::map<std::tuple<const Expr*, bool, const VarDecl*>, ExprRef> cond_cache_;  // lookup only
  int lock_depth_ = 0;
  const VarDecl* elem_var_ = nullptr;  // set while an expression is built in element form
  std::vector<LocalEvent> local_events_;
  std::map<size_t, InnerInfo> inner_ctx_;  // Model::loops index -> what the finished loop left
  std::map<const VarDecl*, std::string> local_id_;  // lookup only, never iterated to choose
  std::map<std::string, const VarDecl*> local_by_id_;
  std::map<const RecordDecl*, std::set<size_t>> reach_cache_;  // lookup only
  std::map<const CallExpr*, std::string> call_exprs_;  // the profile-free mode: call -> key, lookup only
  // the extended facts F1-F3 (spec.extended_facts)
  struct PendingReturn {
    const ReturnStmt* stmt = nullptr;
    ExprRef guard = nullptr;
    std::string loop_key;
  };
  bool ext_ = false;
  std::map<const CallExpr*, std::string> ext_call_keys_;  // every call of the body -> its key, lookup only
  std::map<const void*, ExprRef> def_guards_;             // a definition of a variable -> its guard, lookup only
  std::map<const void*, bool> def_in_loop_;               // lookup only
  std::vector<PendingReturn> returns_;                    // in walk order
  std::unique_ptr<CfgIndex> ext_cfg_;
  bool ext_cfg_tried_ = false;
  // the extended facts F4 (spec.extended_facts)
  struct ExtWrite {
    const Stmt* at = nullptr;
    std::string type, field;
    uint8_t kind = KZ_FWRITE_KIND_NONE;
    Pos pos;
    uint32_t site = 0;  // in Model::write_sites
  };
  struct PtrWriteAt {
    size_t row = 0;  // in Model::ptr_writes
    const Stmt* at = nullptr;
    std::string type, field;
    bool in_loop = false;
  };
  std::map<const Expr*, std::string> read_keys_;                // the conversion of a READS row -> its key, lookup only
  std::vector<std::pair<size_t, const Expr*>> pending_reads_;   // an ARGFLOW row (Model::argflows) and its source read
  std::vector<ExtWrite> ext_writes_;                             // the FUNC_WRITES events of the body, in walk order
  std::vector<PtrWriteAt> ext_ptr_writes_;
};

// ---- static initializers: function values stored in tables -------------------------------------------

void flow_from_init(TuReader& tu, const std::string& object, QualType t, const Expr* init, int depth) {
  if (init == nullptr || depth > 6) return;
  Model& m = tu.model();
  init = init->IgnoreParens();
  t = t.getCanonicalType();
  const auto* il = dyn_cast<InitListExpr>(init);
  if (const ArrayType* at = tu.ctx().getAsArrayType(t)) {
    const QualType et = at->getElementType();
    if (il == nullptr) return;
    for (unsigned i = 0; i < il->getNumInits(); ++i) {
      if (is_fp_type(et)) {
        const Expr* e = il->getInit(i)->IgnoreParenCasts();
        if (const auto* dre = dyn_cast<DeclRefExpr>(e))
          if (const auto* fd = dyn_cast<FunctionDecl>(dre->getDecl())) m.flow.push_back(FlowEdge{"F:" + tu.ensure_func(fd).key, "G:" + object});
      } else {
        flow_from_init(tu, object, et, il->getInit(i), depth + 1);
      }
    }
    return;
  }
  if (const RecordType* rt = t->getAs<RecordType>()) {
    const RecordDecl* rd = rt->getDecl()->getDefinition();
    if (rd == nullptr || il == nullptr) return;
    unsigned idx = 0;
    for (const FieldDecl* fd : rd->fields()) {
      if (fd->isUnnamedBitField()) continue;
      const Expr* e = idx < il->getNumInits() ? il->getInit(idx) : nullptr;
      ++idx;
      if (e == nullptr) continue;
      if (is_fp_type(fd->getType())) {
        const Expr* s = e->IgnoreParenCasts();
        if (const auto* dre = dyn_cast<DeclRefExpr>(s)) {
          if (const auto* fn = dyn_cast<FunctionDecl>(dre->getDecl()))
            m.flow.push_back(FlowEdge{"F:" + tu.ensure_func(fn).key, tu.record_name(rd) + "." + fd->getNameAsString()});
        } else if (const auto* uo = dyn_cast<UnaryOperator>(s)) {
          if (uo->getOpcode() == UO_AddrOf)
            if (const auto* dre2 = dyn_cast<DeclRefExpr>(uo->getSubExpr()->IgnoreParens()))
              if (const auto* fn2 = dyn_cast<FunctionDecl>(dre2->getDecl()))
                m.flow.push_back(FlowEdge{"F:" + tu.ensure_func(fn2).key, tu.record_name(rd) + "." + fd->getNameAsString()});
        }
      } else {
        flow_from_init(tu, object, fd->getType(), e, depth + 1);
      }
    }
  }
}

}  // namespace

bool read_function_body(TuReader& tu, const FunctionDecl* fd) {
  const FunctionDecl* def = fd->getDefinition();
  if (def == nullptr) def = fd;
  const std::string key = tu.func_key(def);
  Model& m = tu.model();
  tu.ensure_func(def);
  if (m.processed_functions.count(key) != 0) return false;
  m.processed_functions.insert(key);
  ++m.stats.functions_read;
  FnWalker w(tu, def);
  w.run();
  return true;
}

void collect_global_fp_flows(TuReader& tu, const VarDecl* vd) {
  const std::string object = tu.object_key(vd);
  // whether the object is const (the table-walk check of finalize): the definition with an
  // initializer decides; a tentative definition only fills a gap
  {
    const bool is_const = vd->getType().isConstant(tu.ctx());
    std::map<std::string, bool>& oc = tu.model().object_const;
    if (vd->hasInit()) oc[object] = is_const;
    else oc.emplace(object, is_const);
  }
  if (!vd->hasInit()) return;
  if (is_fp_type(vd->getType())) {
    const Expr* s = vd->getInit()->IgnoreParenCasts();
    if (const auto* dre = dyn_cast<DeclRefExpr>(s))
      if (const auto* fn = dyn_cast<FunctionDecl>(dre->getDecl())) tu.model().flow.push_back(FlowEdge{"F:" + tu.ensure_func(fn).key, "G:" + object});
    return;
  }
  flow_from_init(tu, object, vd->getType(), vd->getInit(), 0);
}

}  // namespace kzs
