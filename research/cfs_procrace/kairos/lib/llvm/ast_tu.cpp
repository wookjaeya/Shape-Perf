// ast_tu.cpp -- see ast_reader.hpp: the unit-level part of the extraction.
#include <algorithm>
#include <filesystem>

#include "ast_reader.hpp"
#include "clang/AST/APValue.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecordLayout.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/Support/Casting.h"

namespace kzs {

using namespace clang;
namespace fs = std::filesystem;

namespace {

const RecordDecl* canonical_record(const RecordDecl* rd) {
  if (rd == nullptr) return nullptr;
  const RecordDecl* def = rd->getDefinition();
  return def != nullptr ? def : cast<RecordDecl>(rd->getCanonicalDecl());
}

std::string collapse_ws(llvm::StringRef text, size_t max) {
  std::string out;
  bool space = false;
  for (char c : text) {
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\\') {
      if (c == '\\') continue;  // line continuations of macro bodies
      space = !out.empty();
      continue;
    }
    if (space) out.push_back(' ');
    space = false;
    out.push_back(c);
    if (out.size() >= max) {
      out += "...";
      break;
    }
  }
  return out;
}

}  // namespace

TuReader::TuReader(ASTContext& ctx, const ExtractSpec& spec, const ReadOptions& opt, Model* model,
                   const std::string& main_file, const std::string& directory)
    : ctx_(ctx), spec_(spec), opt_(opt), model_(model), main_file_(main_file), directory_(directory) {}

// ---- naming ---------------------------------------------------------------------------------

std::string TuReader::path_of(FileID id) {
  auto it = path_cache_.find(id);
  if (it != path_cache_.end()) return it->second;
  std::string out;
  if (id.isInvalid()) {
    out = "<invalid>";
  } else if (auto entry = sm().getFileEntryRefForID(id)) {
    fs::path p(std::string(entry->getName()));
    if (p.is_relative() && !directory_.empty()) p = fs::path(directory_) / p;
    p = p.lexically_normal();
    std::string s = p.generic_string();
    if (!opt_.root.empty()) {
      std::string root = opt_.root;
      if (root.back() != '/') root += '/';
      if (s.compare(0, root.size(), root) == 0) s = s.substr(root.size());
    }
    out = s;
  } else {
    out = "<builtin>";
  }
  path_cache_[id] = out;
  return out;
}

Pos TuReader::pos_of(SourceLocation loc) {
  Pos p;
  if (loc.isInvalid()) {
    p.file = "<invalid>";
    return p;
  }
  const SourceLocation e = sm().getExpansionLoc(loc);
  p.file = path_of(sm().getFileID(e));
  p.line = sm().getExpansionLineNumber(e);
  p.col = sm().getExpansionColumnNumber(e);
  return p;
}

std::string TuReader::source_text(SourceRange range, size_t max) {
  if (range.isInvalid()) return "";
  const CharSourceRange cr = sm().getExpansionRange(range);
  bool invalid = false;
  llvm::StringRef text = Lexer::getSourceText(cr, sm(), ctx_.getLangOpts(), &invalid);
  if (invalid) return "";
  return collapse_ws(text, max);
}

const RecordDecl* TuReader::record_of(QualType t) {
  t = t.getCanonicalType();
  while (const ArrayType* at = ctx_.getAsArrayType(t)) t = at->getElementType().getCanonicalType();
  if (const RecordType* rt = t->getAs<RecordType>()) return canonical_record(rt->getDecl());
  return nullptr;
}

std::string TuReader::record_name(const RecordDecl* rd) {
  rd = canonical_record(rd);
  if (rd == nullptr) return "<none>";
  auto it = names.find(rd);
  if (it != names.end() && !it->second.empty()) {
    // Typedef names are stored with a leading '\x01' so that they sort before tags.
    for (const std::string& n : it->second)
      if (!n.empty() && n[0] == '\x01') return n.substr(1);
    return *it->second.begin();
  }
  return "<anonymous>";
}

void TuReader::collect_typedefs() {
  for (Decl* d : ctx_.getTranslationUnitDecl()->decls()) {
    if (auto* td = dyn_cast<TypedefDecl>(d)) {
      QualType u = td->getUnderlyingType().getCanonicalType();
      while (const ArrayType* at = ctx_.getAsArrayType(u)) u = at->getElementType().getCanonicalType();
      if (const RecordType* rt = u->getAs<RecordType>()) names[canonical_record(rt->getDecl())].insert("\x01" + td->getName().str());
    } else if (auto* rd = dyn_cast<RecordDecl>(d)) {
      if (rd->getIdentifier() != nullptr) names[canonical_record(rd)].insert(rd->getName().str());
    }
  }
}

std::string TuReader::func_key(const FunctionDecl* fd) {
  const FunctionDecl* def = nullptr;
  fd->hasBody(def);
  const FunctionDecl* where = def != nullptr ? def : fd->getCanonicalDecl();
  if (fd->isExternallyVisible()) return fd->getNameAsString();
  const Pos p = pos_of(where->getLocation());
  return fd->getNameAsString() + "@" + p.file + ":" + std::to_string(p.line);
}

std::string TuReader::object_key(const VarDecl* vd) {
  if (vd->isExternallyVisible()) return vd->getNameAsString();
  const VarDecl* def = vd->getDefinition();
  const Pos p = pos_of((def != nullptr ? def : vd)->getLocation());
  return vd->getNameAsString() + "@" + p.file + ":" + std::to_string(p.line);
}

FuncFact& TuReader::ensure_func(const FunctionDecl* fd) {
  const std::string key = func_key(fd);
  FuncFact& f = model_->funcs[key];
  const FunctionDecl* def = nullptr;
  fd->hasBody(def);
  const bool has_def = def != nullptr;
  if (f.key.empty()) {
    f.key = key;
    f.name = fd->getNameAsString();
    f.nparams = fd->getNumParams();
    f.pos = pos_of((has_def ? def : fd->getCanonicalDecl())->getLocation());
    if (!fd->isExternallyVisible()) f.flags |= KZ_FUNC_FLAG_STATIC;
    if (fd->isVariadic()) f.flags |= KZ_FUNC_FLAG_VARIADIC;
    if (fd->getBuiltinID() != 0) f.flags |= KZ_FUNC_FLAG_BUILTIN;
    if (fd->isInlineSpecified()) f.flags |= KZ_FUNC_FLAG_INLINE;
  }
  if (has_def && (f.flags & KZ_FUNC_FLAG_DEFINED) == 0) {
    f.flags |= KZ_FUNC_FLAG_DEFINED;
    f.pos = pos_of(def->getLocation());
    f.end_line = pos_of(def->getEndLoc()).line;
    f.tu = main_file_;
    f.nparams = def->getNumParams();
  }
  return f;
}

std::string TuReader::next_key(const Pos& pos, const std::string& func_key, uint32_t* ordinal) {
  const std::string base = pos.str();
  const uint32_t n = ordinal_counter_[func_key + "|" + base]++;
  *ordinal = n;
  return base + "#" + std::to_string(n);
}

// ---- targets and containment ----------------------------------------------------------------

std::vector<size_t> TuReader::targets_of_field(const FieldDecl* fd) {
  std::vector<size_t> out;
  const RecordDecl* rd = fd->getParent();
  // A field of an anonymous record inside a named record belongs to the named one.
  while (rd != nullptr && rd->getIdentifier() == nullptr && rd->getTypedefNameForAnonDecl() == nullptr) {
    const RecordDecl* outer = dyn_cast_or_null<RecordDecl>(rd->getDeclContext());
    if (outer == nullptr) break;
    rd = outer;
  }
  rd = canonical_record(rd);
  auto it = names.find(rd);
  if (it == names.end()) return out;
  const std::string fname = fd->getNameAsString();
  for (size_t i = 0; i < spec_.fields.size(); ++i) {
    const FieldRef& f = spec_.fields[i].field;
    if (f.field != fname) continue;
    if (it->second.count(f.type) != 0 || it->second.count("\x01" + f.type) != 0) out.push_back(i);
  }
  return out;
}

bool TuReader::is_owner_record(const RecordDecl* rd, size_t index) {
  rd = canonical_record(rd);
  auto it = names.find(rd);
  if (it == names.end()) return false;
  const FieldRef& f = spec_.fields[index].field;
  if (it->second.count(f.type) == 0 && it->second.count("\x01" + f.type) == 0) return false;
  for (const FieldDecl* fd : rd->fields())
    if (fd->getName() == f.field) return true;
  return false;
}

bool TuReader::type_contains_target(QualType t, size_t index) {
  const RecordDecl* rd = record_of(t);
  if (rd == nullptr) return false;
  const auto key = std::make_pair(rd, index);
  auto it = contain_cache_.find(key);
  if (it != contain_cache_.end()) return it->second > 0;
  contain_cache_[key] = 0;  // cycle guard: a record cannot contain itself by value
  bool result = is_owner_record(rd, index);
  if (!result)
    for (const FieldDecl* fd : rd->fields()) {
      if (type_contains_target(fd->getType(), index)) {
        result = true;
        break;
      }
    }
  contain_cache_[key] = result ? 1 : 0;
  return result;
}

bool TuReader::type_contains_any_target(QualType t) {
  for (size_t i = 0; i < spec_.fields.size(); ++i)
    if (type_contains_target(t, i)) return true;
  return false;
}

std::set<size_t> TuReader::owners_contained_in(QualType t) {
  std::set<size_t> out;
  for (size_t i = 0; i < spec_.fields.size(); ++i)
    if (type_contains_target(t, i)) out.insert(i);
  return out;
}

bool TuReader::is_union_type(QualType t) {
  const RecordDecl* rd = record_of(t);
  return rd != nullptr && rd->isUnion();
}

// ---- values ---------------------------------------------------------------------------------

bool TuReader::eval_int(const Expr* e, int64_t* out) {
  if (e == nullptr || e->isValueDependent() || e->containsErrors()) return false;
  Expr::EvalResult r;
  if (!e->EvaluateAsInt(r, ctx_, Expr::SE_NoSideEffects)) return false;
  *out = r.Val.getInt().isSigned() ? r.Val.getInt().getSExtValue() : static_cast<int64_t>(r.Val.getInt().getZExtValue());
  return true;
}

ValueInfo TuReader::classify_value(const Expr* e) {
  ValueInfo v;
  if (e == nullptr) return v;
  v.text = source_text(e->getSourceRange());
  const Expr* s = e->IgnoreParenCasts();
  int64_t c = 0;
  if (eval_int(s, &c)) {
    v.kind = KZ_ARG_KIND_CONST;
    v.value = c;
    v.detail = "const";
    return v;
  }
  if (const auto* dre = dyn_cast<DeclRefExpr>(s)) {
    if (const auto* pv = dyn_cast<ParmVarDecl>(dre->getDecl())) {
      v.kind = KZ_ARG_KIND_SYMBOLIC;
      v.is_param = true;
      v.value = pv->getFunctionScopeIndex();
      v.detail = "param:" + pv->getNameAsString() + "#" + std::to_string(pv->getFunctionScopeIndex());
      return v;
    }
    if (const auto* vd = dyn_cast<VarDecl>(dre->getDecl())) {
      v.kind = KZ_ARG_KIND_SYMBOLIC;
      v.detail = (vd->hasGlobalStorage() ? "global:" : "local:") + vd->getNameAsString();
      return v;
    }
    if (const auto* fd = dyn_cast<FunctionDecl>(dre->getDecl())) {
      v.kind = KZ_ARG_KIND_FUNC;
      v.detail = "function:" + fd->getNameAsString();
      return v;
    }
  }
  if (const auto* me = dyn_cast<MemberExpr>(s)) {
    if (const auto* fd = dyn_cast<FieldDecl>(me->getMemberDecl())) {
      v.kind = KZ_ARG_KIND_SYMBOLIC;
      v.detail = "field:" + record_name(fd->getParent()) + "." + fd->getNameAsString();
      return v;
    }
  }
  if (isa<CallExpr>(s)) {
    v.kind = KZ_ARG_KIND_SYMBOLIC;
    const auto* ce = cast<CallExpr>(s);
    const FunctionDecl* callee = ce->getDirectCallee();
    v.detail = "call:" + (callee != nullptr ? callee->getNameAsString() : std::string("<indirect>"));
    return v;
  }
  v.kind = KZ_ARG_KIND_TOP;
  return v;
}

// ---- the unit -------------------------------------------------------------------------------

void TuReader::read_function(const FunctionDecl* fd) {
  if (sm().isInSystemHeader(fd->getLocation())) {
    ++skipped_system_;
    ++model_->stats.functions_skipped_system;
    return;
  }
  if (!read_function_body(*this, fd)) ++model_->stats.functions_dedup;
}

void TuReader::run() {
  collect_typedefs();
  std::map<std::string, const VarDecl*> best;  // object key -> the declaration that defines it
  for (Decl* d : ctx_.getTranslationUnitDecl()->decls()) {
    if (const auto* fd = dyn_cast<FunctionDecl>(d)) {
      if (fd->doesThisDeclarationHaveABody()) read_function(fd);
    } else if (const auto* vd = dyn_cast<VarDecl>(d)) {
      if (vd->isLocalVarDeclOrParm() || !vd->hasGlobalStorage()) continue;
      if (sm().isInSystemHeader(vd->getLocation())) continue;
      if (vd->isThisDeclarationADefinition() == VarDecl::DeclarationOnly) continue;
      const std::string key = object_key(vd);
      auto it = best.find(key);
      // A declaration with an initializer wins over a tentative definition.
      if (it == best.end() || (vd->hasInit() && !it->second->hasInit())) best[key] = vd;
    }
  }
  for (const auto& kv : best) read_global(kv.second);
}

void TuReader::init_leaf(const std::string& object_key_, const std::string& path, const Expr* init, ImageFact* f) {
  f->object = object_key_;
  f->path = path;
  f->tu = main_file_;
  if (init == nullptr || isa<ImplicitValueInitExpr>(init)) {
    f->kind = KZ_IMAGE_KIND_INT;
    f->value = 0;
    f->text = "zero";
    return;
  }
  f->pos = pos_of(init->getBeginLoc());
  f->text = source_text(init->getSourceRange(), 80);
  if (spec_.extended_facts && init->getBeginLoc().isMacroID() && !sm().isInSystemHeader(sm().getSpellingLoc(init->getBeginLoc()))) {
    // F6: the macro written at the initializer (the outermost one) and where the first token of the value is
    // spelled (a macro of a system header, NULL say, is not recorded)
    SourceLocation loc = init->getBeginLoc();
    std::string name;
    while (loc.isMacroID()) {
      const llvm::StringRef n = Lexer::getImmediateMacroName(loc, sm(), ctx_.getLangOpts());
      if (!n.empty()) name = n.str();
      loc = sm().getImmediateMacroCallerLoc(loc);
    }
    f->macro = name;
    const SourceLocation sp = sm().getSpellingLoc(init->getBeginLoc());
    f->spell.file = path_of(sm().getFileID(sp));
    f->spell.line = sm().getSpellingLineNumber(sp);
    f->spell.col = sm().getSpellingColumnNumber(sp);
  }
  const Expr* s = init->IgnoreParenCasts();
  if (const auto* sl = dyn_cast<StringLiteral>(s)) {
    f->kind = KZ_IMAGE_KIND_STRING;
    f->text = sl->getString().str();
    return;
  }
  // The conversions stay: evaluating `table` (an array) as a pointer gives the address, while the
  // array itself would evaluate to its elements.
  const Expr* whole = init->IgnoreParens();
  Expr::EvalResult r;
  if (!whole->isValueDependent() && whole->EvaluateAsRValue(r, ctx_)) {
    const APValue& av = r.Val;
    if (av.isInt()) {
      f->kind = KZ_IMAGE_KIND_INT;
      f->value = av.getInt().isSigned() ? av.getInt().getSExtValue() : static_cast<int64_t>(av.getInt().getZExtValue());
      return;
    }
    if (av.isLValue()) {
      if (av.isNullPointer() || (av.getLValueBase().isNull() && av.getLValueOffset().isZero())) {
        f->kind = KZ_IMAGE_KIND_NULLPTR;
        f->text = "NULL";
        return;
      }
      if (const auto* vd = av.getLValueBase().dyn_cast<const ValueDecl*>()) {
        if (const auto* fd = dyn_cast<FunctionDecl>(vd)) {
          f->kind = KZ_IMAGE_KIND_FUNC;
          f->text = fd->getNameAsString();
          f->fn_key = ensure_func(fd).key;
          return;
        }
        if (const auto* var = dyn_cast<VarDecl>(vd)) {
          f->kind = KZ_IMAGE_KIND_ADDR;
          f->text = object_key(var);
          if (!av.getLValueOffset().isZero()) f->text += "+" + std::to_string(av.getLValueOffset().getQuantity());
          return;
        }
      }
      if (const auto* base = av.getLValueBase().dyn_cast<const Expr*>()) {
        if (const auto* sl2 = dyn_cast<StringLiteral>(base)) {
          f->kind = KZ_IMAGE_KIND_STRING;
          f->text = sl2->getString().str();
          return;
        }
      }
    }
  }
  f->kind = KZ_IMAGE_KIND_NONCONST;
}

void TuReader::flatten_initializer(const std::string& object_key_, const std::string& path, QualType t, const Expr* init,
                                   size_t* budget, std::vector<ImageFact>* out, const Pos& pos) {
  if (*budget == 0) return;
  t = t.getCanonicalType();
  if (init != nullptr) init = init->IgnoreParens();
  const auto* il = init != nullptr ? dyn_cast<InitListExpr>(init) : nullptr;
  if (const ArrayType* at = ctx_.getAsArrayType(t)) {
    // A string literal can initialize a char array.
    if (init != nullptr && isa<StringLiteral>(init->IgnoreParenCasts())) {
      ImageFact f;
      f.pos = pos;
      init_leaf(object_key_, path, init, &f);
      out->push_back(f);
      --*budget;
      return;
    }
    if (il == nullptr) return;  // zero-filled
    const QualType et = at->getElementType();
    const auto* cat = dyn_cast<ConstantArrayType>(at);
    const uint64_t n = cat != nullptr ? cat->getSize().getZExtValue() : il->getNumInits();
    const uint64_t explicit_n = il->getNumInits();
    for (uint64_t i = 0; i < std::min<uint64_t>(n, explicit_n); ++i) {
      if (*budget == 0) {
        ImageFact f;
        f.object = object_key_;
        f.path = path + "[...]";
        f.kind = KZ_IMAGE_KIND_NONCONST;
        f.text = "<image truncated>";
        f.pos = pos;
        f.tu = main_file_;
        out->push_back(f);
        return;
      }
      flatten_initializer(object_key_, path + "[" + std::to_string(i) + "]", et, il->getInit(static_cast<unsigned>(i)), budget, out, pos);
    }
    if (n > explicit_n) {
      ImageFact f;
      f.object = object_key_;
      f.path = path + "[" + std::to_string(explicit_n) + ".." + std::to_string(n - 1) + "]";
      f.kind = KZ_IMAGE_KIND_ZERO_FILL;
      f.text = il->hasArrayFiller() ? "array filler" : "zero";
      f.pos = pos;
      f.tu = main_file_;
      out->push_back(f);
    }
    return;
  }
  if (const RecordType* rt = t->getAs<RecordType>()) {
    const RecordDecl* rd = rt->getDecl()->getDefinition();
    if (rd == nullptr) return;
    if (il == nullptr) {
      // `= other` or a missing initializer: nothing is known field by field.
      if (init != nullptr) {
        ImageFact f;
        f.pos = pos;
        init_leaf(object_key_, path, init, &f);
        f.kind = KZ_IMAGE_KIND_NONCONST;
        out->push_back(f);
        --*budget;
      }
      return;
    }
    unsigned idx = 0;
    for (const FieldDecl* fd : rd->fields()) {
      if (fd->isUnnamedBitField()) continue;
      if (rd->isUnion()) {
        const FieldDecl* active = il->getInitializedFieldInUnion();
        if (active != fd) continue;
        flatten_initializer(object_key_, path + "." + fd->getNameAsString(), fd->getType(), il->getNumInits() > 0 ? il->getInit(0) : nullptr, budget, out, pos);
        break;
      }
      const Expr* sub = idx < il->getNumInits() ? il->getInit(idx) : nullptr;
      ++idx;
      const size_t before = out->size();
      flatten_initializer(object_key_, path + "." + fd->getNameAsString(), fd->getType(), sub, budget, out, pos);
      // F6: the record field a leaf is (a scalar, a pointer or a string field: one leaf right here)
      if (spec_.extended_facts && out->size() == before + 1 && (*out)[before].path == path + "." + fd->getNameAsString()) {
        (*out)[before].owner = record_name(rd);
        (*out)[before].field = fd->getNameAsString();
      }
    }
    return;
  }
  // A scalar or a pointer.
  ImageFact f;
  f.pos = pos;
  init_leaf(object_key_, path, init, &f);
  out->push_back(f);
  --*budget;
}

namespace {

struct Leaf {
  bool constant = false;
  bool explicit_init = false;
  int64_t value = 0;
};

// The nodes the FACT_INIT walk of one object's initializer visits before it stops (a tool parameter no reference
// fixes: unspecified_by_reference; the stops are counted, count.walk_bounds_hit, and an object whose walk stopped
// gets a row "not a constant" besides the values read).
constexpr size_t kInitBudget = 4096;

}  // namespace

void TuReader::read_init_for_targets(const VarDecl* vd) {
  const QualType vt = vd->getType();
  for (size_t i = 0; i < spec_.fields.size(); ++i) {
    if (!type_contains_target(vt, i)) continue;
    // Walk the type with the initializer and collect the initial value(s) of the target field.
    std::vector<Leaf> leaves;
    std::vector<std::pair<QualType, const Expr*>> stack;
    stack.push_back({vt, vd->hasInit() ? vd->getInit() : nullptr});
    size_t budget = kInitBudget;
    while (!stack.empty() && budget > 0) {
      auto [t, init] = stack.back();
      stack.pop_back();
      --budget;
      t = t.getCanonicalType();
      if (init != nullptr) init = init->IgnoreParens();
      const auto* il = init != nullptr ? dyn_cast<InitListExpr>(init) : nullptr;
      if (const ArrayType* at = ctx_.getAsArrayType(t)) {
        if (!type_contains_target(at->getElementType(), i)) continue;
        if (il == nullptr) {
          stack.push_back({at->getElementType(), nullptr});
        } else {
          for (unsigned k = 0; k < il->getNumInits(); ++k) stack.push_back({at->getElementType(), il->getInit(k)});
          const auto* cat = dyn_cast<ConstantArrayType>(at);
          if (cat == nullptr || cat->getSize().getZExtValue() > il->getNumInits()) stack.push_back({at->getElementType(), nullptr});
        }
        continue;
      }
      const RecordDecl* rd = record_of(t);
      if (rd == nullptr) continue;
      rd = rd->getDefinition();
      if (rd == nullptr) continue;
      unsigned idx = 0;
      for (const FieldDecl* fd : rd->fields()) {
        if (fd->isUnnamedBitField()) continue;
        const Expr* sub = (il != nullptr && idx < il->getNumInits()) ? il->getInit(idx) : nullptr;
        ++idx;
        if (is_owner_record(rd, i) && fd->getName() == spec_.fields[i].field.field) {
          Leaf l;
          l.explicit_init = sub != nullptr && !isa<ImplicitValueInitExpr>(sub);
          if (!l.explicit_init) {
            l.constant = true;
            l.value = 0;
          } else {
            int64_t c = 0;
            l.constant = eval_int(sub, &c);
            l.value = c;
          }
          leaves.push_back(l);
        } else if (type_contains_target(fd->getType(), i)) {
          stack.push_back({fd->getType(), sub});
        }
      }
    }
    InitFact f;
    f.loc_type = spec_.fields[i].field.type;
    f.loc_field = spec_.fields[i].field.field;
    f.object = vd->getNameAsString();
    f.pos = pos_of(vd->getLocation());
    f.tu = main_file_;
    // One fact per distinct initial value of the field in the object (an array of structs can
    // start with several), and one for a value that is not a constant.  A value that every
    // element gets from the implicit zero initialization is `zero-initialized`.
    std::map<int64_t, bool> values;  // value -> some leaf with this value has an explicit initializer
    bool nonconst = false;
    for (const Leaf& l : leaves) {
      if (!l.constant) {
        nonconst = true;
        continue;
      }
      bool& explicit_seen = values[l.value];
      explicit_seen |= l.explicit_init;
    }
    // the walk stopped at its bound with parts not read: the values are not all known (a row "not a constant" says so)
    const bool cut = !stack.empty();
    if (cut) {
      ++model_->stats.init_budget_out;
      if (leaves.empty()) ++model_->stats.init_budget_out_no_leaf;
      nonconst = true;
    }
    if (leaves.empty() && !cut) values[0] = false;
    for (const auto& kv : values) {
      InitFact one = f;
      one.value = kv.first;
      one.source = kv.second ? KZ_INIT_SOURCE_STATIC_INIT : KZ_INIT_SOURCE_ZERO;
      model_->inits.push_back(one);
    }
    if (nonconst) {
      InitFact one = f;
      one.source = KZ_INIT_SOURCE_NONCONST;
      model_->inits.push_back(one);
    }
  }
}

void TuReader::read_global(const VarDecl* vd) {
  const std::string key = object_key(vd);
  const Pos pos = pos_of(vd->getLocation());
  model_->object_pos.emplace(key, pos);
  model_->object_tu.emplace(key, main_file_);
  if (vd->hasInit() && model_->object_images.count(key) == 0) {
    const QualType t = vd->getType().getCanonicalType();
    if (record_of(t) != nullptr || ctx_.getAsArrayType(t) != nullptr) {
      std::vector<ImageFact> img;
      size_t budget = opt_.image_budget;
      flatten_initializer(key, "", t, vd->getInit(), &budget, &img, pos);
      model_->object_images[key] = std::move(img);
    } else if (spec_.extended_facts && model_->object_scalar_images.count(key) == 0) {
      // F6: a scalar global's initializer as one leaf
      ImageFact f;
      f.pos = pos;
      init_leaf(key, "", vd->getInit(), &f);
      model_->object_scalar_images[key] = f;
    }
  }
  read_init_for_targets(vd);
  collect_global_fp_flows(*this, vd);
}

}  // namespace kzs
