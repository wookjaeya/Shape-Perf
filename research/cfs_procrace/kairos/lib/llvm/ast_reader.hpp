// ast_reader.hpp -- reading one translation unit: the shared state of the AST-level extraction.
//
// TuReader (ast_tu.cpp) owns what is common to a whole unit: path and type naming, the table of
// extraction targets and the record-containment relation, function and object registration, and
// the global variables (INIT image, static initializer images).  FnWalker (ast_fn.cpp) reads one
// function body.  Both write into a Model (model.hpp).
//
// Everything here is at the Clang AST level on purpose: field identity is explicit in a
// MemberExpr, and "(struct type, field)" is the abstract location of SPEC 4.2 step 1.
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/AST/Stmt.h"
#include "clang/Basic/SourceManager.h"
#include "extract.hpp"
#include "model.hpp"
#include "spec.hpp"

namespace kzs {


// A value as the extractor classifies it for a fact.
struct ValueInfo {
  uint8_t kind = KZ_ARG_KIND_TOP;
  int64_t value = 0;
  std::string text;  // the expression as written (whitespace collapsed, at most 120 bytes)
  bool is_param = false;
  std::string detail;  // "param:name#k", "call:f", "field:T.f", "local:name", "const"
};

class TuReader {
 public:
  TuReader(clang::ASTContext& ctx, const ExtractSpec& spec, const ReadOptions& opt, Model* model,
           const std::string& main_file, const std::string& directory);

  // Reads the unit: registers typedef names, then every function definition and global variable.
  void run();

  // ---- naming ----
  clang::ASTContext& ctx() { return ctx_; }
  const clang::SourceManager& sm() const { return ctx_.getSourceManager(); }
  const ExtractSpec& spec() const { return spec_; }
  Model& model() { return *model_; }
  const std::string& main_file() const { return main_file_; }

  Pos pos_of(clang::SourceLocation loc);
  std::string path_of(clang::FileID id);
  std::string source_text(clang::SourceRange range, size_t max = 120);
  // The name of a record type for output: a typedef name if there is one, else the tag.
  std::string record_name(const clang::RecordDecl* rd);
  // The record declaration a type denotes after stripping qualifiers, typedefs and arrays; null if none.
  const clang::RecordDecl* record_of(clang::QualType t);
  std::string func_key(const clang::FunctionDecl* fd);
  std::string object_key(const clang::VarDecl* vd);
  // An object of internal linkage named by a lock identity: noted when its definition is not in this unit's main
  // file (Model::internal_object_outside_main; each unit that defines it has its own object under one key).
  void note_lock_object(const clang::VarDecl* vd);
  FuncFact& ensure_func(const clang::FunctionDecl* fd);

  // ---- targets ----
  // Indices into spec.fields of the targets that `fd` is.
  std::vector<size_t> targets_of_field(const clang::FieldDecl* fd);
  // True if a value of type `t` (or an array of it) holds the owner type of target `index`, directly
  // or inside nested records and arrays (not through pointers).
  bool type_contains_target(clang::QualType t, size_t index);
  // True if `t` holds the owner type of any target.
  bool type_contains_any_target(clang::QualType t);
  // The owner record types of the targets seen in this unit.
  bool is_owner_record(const clang::RecordDecl* rd, size_t index);
  std::set<size_t> owners_contained_in(clang::QualType t);  // target indices whose owner type `t` holds
  bool is_union_type(clang::QualType t);

  // ---- values ----
  ValueInfo classify_value(const clang::Expr* e);
  bool eval_int(const clang::Expr* e, int64_t* out);
  std::string next_key(const Pos& pos, const std::string& func_key, uint32_t* ordinal);

  // ---- per-unit tables used by the walker ----
  std::map<const clang::RecordDecl*, std::set<std::string>> names;

  // Counters of what was skipped, for the report.
  uint32_t skipped_system_ = 0;

 private:
  void collect_typedefs();
  void read_function(const clang::FunctionDecl* fd);
  void read_global(const clang::VarDecl* vd);
  void read_init_for_targets(const clang::VarDecl* vd);
  void flatten_initializer(const std::string& object_key, const std::string& path, clang::QualType t,
                           const clang::Expr* init, size_t* budget, std::vector<ImageFact>* out, const Pos& pos);
  void init_leaf(const std::string& object_key, const std::string& path, const clang::Expr* init, ImageFact* f);

  clang::ASTContext& ctx_;
  const ExtractSpec& spec_;
  const ReadOptions& opt_;
  Model* model_;
  std::string main_file_, directory_;
  std::map<clang::FileID, std::string> path_cache_;
  std::map<std::pair<const clang::RecordDecl*, size_t>, int> contain_cache_;
  std::map<std::string, uint32_t> ordinal_counter_;

  friend class FnWalker;
};

// Reads the body of `fd` into the model.  Returns false if the function was already read.
bool read_function_body(TuReader& tu, const clang::FunctionDecl* fd);
// Function values stored by the static initializer of a global object: fp-flow edges.
void collect_global_fp_flows(TuReader& tu, const clang::VarDecl* vd);

}  // namespace kzs
