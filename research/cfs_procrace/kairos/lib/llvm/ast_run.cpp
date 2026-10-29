// ast_run.cpp -- running Clang over the recorded translation units (SPEC 4.1 steps 1-2).
//
// Each unit is parsed with the command the build recorded plus the analysis-only adaptations
// of SPEC 4.1 step 2: -Wno-unknown-warning-option and -Wno-error (the recorded command was
// written for another compiler) and -resource-dir.  Warnings are silenced because they carry
// no meaning here; an error stops the run, so a closed program that does not parse is never
// analysed silently.
#include <filesystem>

#include "ast_reader.hpp"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/ADT/SmallString.h"

namespace kzs {

using namespace clang;
namespace fs = std::filesystem;

namespace {

std::string record_path(const std::string& path, const std::string& directory, const std::string& root) {
  fs::path p(path);
  if (p.is_relative() && !directory.empty()) p = fs::path(directory) / p;
  std::string s = p.lexically_normal().generic_string();
  if (!root.empty()) {
    std::string r = root;
    if (r.back() != '/') r += '/';
    if (s.compare(0, r.size(), r) == 0) s = s.substr(r.size());
  }
  return s;
}

class SingleUnitDb : public tooling::CompilationDatabase {
 public:
  explicit SingleUnitDb(tooling::CompileCommand cmd) : cmd_(std::move(cmd)) {}
  std::vector<tooling::CompileCommand> getCompileCommands(llvm::StringRef file) const override {
    if (file == cmd_.Filename) return {cmd_};
    return {};
  }
  std::vector<std::string> getAllFiles() const override { return {cmd_.Filename}; }
  std::vector<tooling::CompileCommand> getAllCompileCommands() const override { return {cmd_}; }

 private:
  tooling::CompileCommand cmd_;
};

class ErrorCounter : public DiagnosticConsumer {
 public:
  void HandleDiagnostic(DiagnosticsEngine::Level level, const Diagnostic& info) override {
    DiagnosticConsumer::HandleDiagnostic(level, info);
    if (level >= DiagnosticsEngine::Error) {
      llvm::SmallString<256> msg;
      info.FormatDiagnostic(msg);
      std::string where = info.hasSourceManager() ? info.getLocation().printToString(info.getSourceManager()) : std::string("<no location>");
      if (errors.size() < 10) errors.push_back(where + ": " + std::string(msg.str()));
    }
  }
  std::vector<std::string> errors;
};

class Consumer : public ASTConsumer {
 public:
  Consumer(const ExtractSpec& spec, const ReadOptions& opt, Model* model, std::string main_file, std::string directory)
      : spec_(spec), opt_(opt), model_(model), main_file_(std::move(main_file)), directory_(std::move(directory)) {}
  void HandleTranslationUnit(ASTContext& ctx) override {
    TuReader reader(ctx, spec_, opt_, model_, main_file_, directory_);
    reader.run();
  }

 private:
  const ExtractSpec& spec_;
  const ReadOptions& opt_;
  Model* model_;
  std::string main_file_, directory_;
};

class Action : public ASTFrontendAction {
 public:
  Action(const ExtractSpec& spec, const ReadOptions& opt, Model* model, std::string main_file, std::string directory)
      : spec_(spec), opt_(opt), model_(model), main_file_(std::move(main_file)), directory_(std::move(directory)) {}
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance&, llvm::StringRef) override {
    return std::make_unique<Consumer>(spec_, opt_, model_, main_file_, directory_);
  }

 private:
  const ExtractSpec& spec_;
  const ReadOptions& opt_;
  Model* model_;
  std::string main_file_, directory_;
};

class Factory : public tooling::FrontendActionFactory {
 public:
  Factory(const ExtractSpec& spec, const ReadOptions& opt, Model* model, std::string main_file, std::string directory)
      : spec_(spec), opt_(opt), model_(model), main_file_(std::move(main_file)), directory_(std::move(directory)) {}
  std::unique_ptr<FrontendAction> create() override { return std::make_unique<Action>(spec_, opt_, model_, main_file_, directory_); }

 private:
  const ExtractSpec& spec_;
  const ReadOptions& opt_;
  Model* model_;
  std::string main_file_, directory_;
};

}  // namespace

std::vector<std::string> flag_list(const std::vector<std::string>& cl) {
  std::vector<std::string> out;
  bool skip_next = false, take_next = false;
  for (size_t i = 1; i < cl.size(); ++i) {
    const std::string& a = cl[i];
    if (skip_next) {
      skip_next = false;
      continue;
    }
    if (take_next) {
      take_next = false;
      out.push_back(a);
      continue;
    }
    if (a == "-o" || a == "-MF" || a == "-MT" || a == "-MQ") {
      skip_next = true;
      continue;
    }
    if (a == "-c" || a == "-MD" || a == "-MMD" || a == "-MP") continue;
    if (!a.empty() && a[0] == '-') {
      out.push_back(a);
      if (a == "-I" || a == "-D" || a == "-U" || a == "-include" || a == "-isystem" || a == "-iquote" || a == "-x") take_next = true;
      continue;
    }
    // a bare word that is not an option value: the input file
  }
  return out;
}

bool is_compiler_plugin_option(const std::string& a) { return a.rfind("-fplugin=", 0) == 0 || a.rfind("-fplugin-arg-", 0) == 0; }

std::vector<std::string> front_end_command(const std::vector<std::string>& cl) {
  std::vector<std::string> out;
  out.reserve(cl.size());
  for (size_t i = 0; i < cl.size(); ++i)
    if (i == 0 || !is_compiler_plugin_option(cl[i])) out.push_back(cl[i]);
  return out;
}

std::string normalized_flags(const std::vector<std::string>& cl) {
  std::string out;
  for (const std::string& f : flag_list(cl)) out += (out.empty() ? "" : " ") + f;
  return out;
}

bool extract_units(const std::vector<UnitCommand>& units, const ExtractSpec& spec, const ReadOptions& options, Model* model, std::string* error) {
  std::vector<std::string> adapt = {"-Wno-unknown-warning-option", "-Wno-error", "-w"};
  if (!options.resource_dir.empty()) adapt.push_back("-resource-dir=" + options.resource_dir);
  adapt.insert(adapt.end(), options.extra_args.begin(), options.extra_args.end());
  for (const UnitCommand& u : units) {
    tooling::CompileCommand cmd(u.directory, u.source, front_end_command(u.command_line), u.object);
    SingleUnitDb db(cmd);
    tooling::ClangTool tool(db, std::vector<std::string>{u.source});
    std::vector<std::string> unit_adapt = adapt;  // and this unit's own analysis arguments (UnitCommand::analysis_args)
    unit_adapt.insert(unit_adapt.end(), u.analysis_args.begin(), u.analysis_args.end());
    tool.appendArgumentsAdjuster(tooling::getInsertArgumentAdjuster(unit_adapt, tooling::ArgumentInsertPosition::END));
    tool.setPrintErrorMessage(false);
    ErrorCounter diags;
    tool.setDiagnosticConsumer(&diags);
    const std::string main_rel = record_path(u.source, u.directory, options.root);
    model->main_files.push_back(main_rel);
    ++model->stats.tus;
    Factory factory(spec, options, model, main_rel, u.directory);
    const int rc = tool.run(&factory);
    if (rc != 0 || !diags.errors.empty()) {
      ++model->stats.parse_errors;
      *error = "cannot parse " + u.source + " (status " + std::to_string(rc) + ")";
      for (const std::string& e : diags.errors) *error += "\n  " + e;
      return false;
    }
    if (options.verbose) fprintf(stderr, "kz-static: read %s\n", main_rel.c_str());
  }
  return true;
}

}  // namespace kzs
