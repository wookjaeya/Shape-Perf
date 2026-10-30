// ast_basis.cpp -- see basis_clang.hpp.
#include <filesystem>
#include <fstream>
#include <sstream>

#include "ast_reader.hpp"
#include "basis_clang.hpp"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/ASTUnit.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/ADT/SmallString.h"

namespace kzs {

using namespace clang;
namespace fs = std::filesystem;

kz::Status load_compile_db(const std::string& path, std::vector<CdbEntry>* out) {
  std::string err;
  auto db = tooling::JSONCompilationDatabase::loadFromFile(path, err, tooling::JSONCommandLineSyntax::AutoDetect);
  if (!db) return kz::Status::error(kz::Err::Io, "cannot read the compile database " + path + ": " + err);
  out->clear();
  for (const tooling::CompileCommand& c : db->getAllCompileCommands()) {
    CdbEntry e;
    e.directory = c.Directory;
    e.file = c.Filename;
    e.output = c.Output;
    e.command_line = c.CommandLine;
    out->push_back(std::move(e));
  }
  return kz::Status::success();
}

std::string clang_version_text() { return getClangFullVersion(); }

namespace {

class ErrorCollector : public DiagnosticConsumer {
 public:
  void HandleDiagnostic(DiagnosticsEngine::Level level, const Diagnostic& info) override {
    DiagnosticConsumer::HandleDiagnostic(level, info);
    if (level >= DiagnosticsEngine::Error) {
      llvm::SmallString<256> msg;
      info.FormatDiagnostic(msg);
      std::string where = info.hasSourceManager() ? info.getLocation().printToString(info.getSourceManager()) : std::string();
      if (errors.size() < 5) errors.push_back(where + ": " + std::string(msg.str()));
    }
  }
  std::vector<std::string> errors;
};

std::string read_text_file(const std::string& path, bool* ok) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    *ok = false;
    return "";
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  *ok = true;
  return ss.str();
}

std::string rel(const std::string& path, const std::string& root) {
  std::string s = fs::path(path).lexically_normal().generic_string();
  if (!root.empty()) {
    std::string r = root;
    if (r.back() != '/') r += '/';
    if (s.compare(0, r.size(), r) == 0) s = s.substr(r.size());
  }
  return s;
}

}  // namespace

kz::Status evaluate_constants(const UnitCommand& unit, const ReadOptions& options, const std::vector<ConstQuery>& queries, std::vector<ConstResult>* out) {
  out->clear();
  bool ok = false;
  std::string code = read_text_file(unit.source, &ok);
  if (!ok) return kz::Status::error(kz::Err::Io, "cannot read " + unit.source);
  code += "\n";
  for (size_t i = 0; i < queries.size(); ++i) {
    // The probe declarations come after the whole unit, so every macro and every declaration of
    // the unit is in scope, as at its end.
    if (queries[i].is_string) code += "static const char *const kz_eval_" + std::to_string(i) + " = (" + queries[i].name + ");\n";
    else code += "static const long long kz_eval_" + std::to_string(i) + " = (long long)(" + queries[i].name + ");\n";
  }
  std::vector<std::string> args = flag_list(front_end_command(unit.command_line));
  for (const char* a : {"-Wno-unknown-warning-option", "-Wno-error", "-w"}) args.push_back(a);
  if (!options.resource_dir.empty()) args.push_back("-resource-dir=" + options.resource_dir);
  args.insert(args.end(), options.extra_args.begin(), options.extra_args.end());
  args.insert(args.end(), unit.analysis_args.begin(), unit.analysis_args.end());
  // relative paths of the command (e.g. -I../../inc of a classic make build) as in the directory the command ran in, as
  // the extraction reads the unit (ClangTool with the command's directory).  The driver applies -working-directory to
  // the real file system, which changes the process's directory; it is restored after the parse (as ClangTool does), so
  // the caller's relative paths keep their meaning.
  if (!unit.directory.empty()) args.push_back("-working-directory=" + unit.directory);
  std::error_code cwd_ec;
  const fs::path cwd = fs::current_path(cwd_ec);
  ErrorCollector diags;
  std::unique_ptr<ASTUnit> ast = tooling::buildASTFromCodeWithArgs(code, args, unit.source, "kz-static", std::make_shared<PCHContainerOperations>(),
                                                                   tooling::getClangStripDependencyFileAdjuster(), tooling::FileContentMappings(), &diags);
  if (!cwd_ec) fs::current_path(cwd, cwd_ec);
  if (!ast) return kz::Status::error(kz::Err::Invalid, "cannot parse " + unit.source);
  ASTContext& ctx = ast->getASTContext();
  Preprocessor& pp = ast->getPreprocessor();
  const SourceManager& sm = ast->getSourceManager();
  std::map<std::string, const VarDecl*> probes;
  for (Decl* d : ctx.getTranslationUnitDecl()->decls())
    if (const auto* vd = dyn_cast<VarDecl>(d))
      if (vd->getName().starts_with("kz_eval_")) probes[vd->getName().str()] = vd;
  for (size_t i = 0; i < queries.size(); ++i) {
    ConstResult r;
    r.name = queries[i].name;
    r.is_string = queries[i].is_string;
    auto it = probes.find("kz_eval_" + std::to_string(i));
    if (it == probes.end() || !it->second->hasInit()) {
      r.error = "not evaluable in " + unit.source + (diags.errors.empty() ? std::string() : ": " + diags.errors.front());
      out->push_back(r);
      continue;
    }
    const Expr* init = it->second->getInit();
    if (r.is_string) {
      const auto* sl = dyn_cast<StringLiteral>(init->IgnoreParenCasts());
      if (sl != nullptr) {
        r.ok = true;
        r.text = sl->getString().str();
      } else {
        r.error = "not a string literal";
      }
    } else {
      Expr::EvalResult ev;
      if (!init->isValueDependent() && init->EvaluateAsInt(ev, ctx)) {
        r.ok = true;
        r.value = ev.Val.getInt().getSExtValue();
      } else {
        r.error = "not an integer constant expression";
      }
    }
    // where is the name defined, if it is a macro
    if (IdentifierInfo* ii = pp.getIdentifierInfo(r.name)) {
      const MacroDefinition md = pp.getMacroDefinition(ii);
      if (const MacroInfo* mi = md.getMacroInfo()) {
        const SourceLocation dl = sm.getExpansionLoc(mi->getDefinitionLoc());
        if (auto fe = sm.getFileEntryRefForID(sm.getFileID(dl))) {
          std::string name(fe->getName());  // a header found through a relative -I is named relative to the command's directory
          if (!name.empty() && name[0] != '/' && !unit.directory.empty()) name = unit.directory + "/" + name;
          r.macro_file = rel(name, options.root);
        }
        r.macro_line = sm.getExpansionLineNumber(dl);
        std::string body;
        for (const Token& t : mi->tokens()) {
          if (!body.empty()) body += " ";
          body += pp.getSpelling(t);
        }
        r.macro_body = body;
      }
    }
    out->push_back(r);
  }
  return kz::Status::success();
}

kz::Status read_unit_images(const UnitCommand& unit, const ReadOptions& options, std::map<std::string, std::vector<ImageFact>>* images) {
  Model tmp;
  ExtractSpec spec;
  std::string err;
  if (!extract_units({unit}, spec, options, &tmp, &err)) return kz::Status::error(kz::Err::Invalid, err);
  *images = tmp.object_images;
  return kz::Status::success();
}

}  // namespace kzs
