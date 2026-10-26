// kz_instr_clang.cpp -- kz-instr-clang, the Clang AST source-transformation connector of the instrumented
// family (a candidate of stage C; SPEC rev 1.4 section 4.1 "Clang as a source transformer", section 5.12;
// review IR section 5.2).  docs/instr_candidate_clang.md describes the rules and the C1/R2 fitness check.
//
//   kz-instr-clang --sites <logical.kzs> --db <compile_commands.json> [--db ...] [--exclude <text>]...
//                  --map <from-prefix>=<to-prefix> [--map ...] --module <id>=<path-part>|* [--module ...]
//                  [--resource-dir <dir>] [--backend <name>] --binds-out <file> --report-out <file>
//   kz-instr-clang ... --fixed <directory> -- <compiler arguments without the input file>
//                  (the units are the tu= paths, resolved against <directory>; a compile database is not needed)
//
// What it does.  Every translation unit that a logical site names (site tu=, matched as a path suffix of a
// recorded command's file) is parsed with the command the reference build recorded, with analysis-only
// adaptations (-Wno-unknown-warning-option, -w, -resource-dir).  The main file's text is edited
// (clang::Rewriter) and the edited copy is written at the mapped path of a SEPARATE source tree; the reference
// sources are only read.  gcc then builds the separate tree with the reference configuration and flags.  A
// unit recorded by several commands is transformed with each and the texts must agree.
//
// Binding forms (chosen from the site, never from the callee's name):
//   call-site wrapper   site with callee=, the callee has a prototype and is not variadic: the callee's name at
//                       the call is replaced by a static inline wrapper with the callee's parameter types,
//                       defined before the enclosing function.  The arguments are evaluated once, as the
//                       wrapper's arguments, in the order C leaves unspecified (nothing is hoisted); the before
//                       record is made at the sequence point before the call, the after record right after the
//                       return (out<k>: the pointee read by the same thread just after the return, only when
//                       the pointer is not null).
//   call-site taps      site with callee= and a variadic callee: each captured fixed argument E becomes
//                       kz_t<b>_<k>(&kz_l<b>, (E)) (it records when that argument is evaluated, one record per
//                       argument, relative order unspecified as in C), and the call becomes
//                       kz_r<b>(&kz_l<b>, (call)) for the after record (link = the last captured argument's
//                       record).  out<k> is not available in this form.
//   callee entry        site without callee=, phase=before: a statement after the body's '{' records the
//                       parameters (reading a parameter has no side effect).
//   callee returns      site without callee=, phase=after|exit: every `return E;` of the body whose text is in
//                       the file becomes `{ T kz_r = (E); record; return kz_r; }` (the conversion of E to T is the
//                       one `return` makes); a void function also records before the body's closing brace.  A
//                       return inside a macro expansion is not rewritten and is listed as an uninstrumented path.
//                       Parameters used as out<k> must not be assigned in the body.
//   store tap           site without callee=, phase=effect, value from=operand0, line=: the right operand R of
//                       each simple assignment at that line becomes kz_s<b>((R)) (recorded after R is
//                       computed and before the store; the conversion to the left type is the assignment's).
// Text in a macro body, a macro argument or a skipped preprocessor region is never edited; such sites are
// written as status=unsupported with the reason.  Inserted text never adds a line before original text
// without a following #line directive, so __LINE__ and diagnostics keep the original numbers; __func__ is
// unchanged because no function is renamed.  The hooks are those of kairos/runtime/include/kz_rt.h (hook ABI 1);
// site and bind ids are the manifest's constants.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Lex/Lexer.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CompilationDatabase.h"
#include "clang/Tooling/JSONCompilationDatabase.h"
#include "clang/Tooling/Tooling.h"
#include "kz_rt_manifest.hpp"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

namespace {

// ---- options and inputs ---------------------------------------------------------------------------------

struct Options {
  std::string sites, binds_out, report_out, resource_dir = KZI_DEFAULT_RESOURCE_DIR, backend = "clang-src-0.1", fixed_dir;
  std::vector<std::string> dbs, excludes, fixed_args;
  std::vector<std::pair<std::string, std::string>> maps;
  std::vector<std::pair<uint32_t, std::string>> modules;
};

enum class Form { CallSite, Callee, Store };

// A group: the sites one binding serves (a call's before and after, a function's entry and return).
struct Group {
  Form form = Form::CallSite;
  std::string tu, func, callee;
  uint32_t line = 0;
  const kzrt::Site* before = nullptr;
  const kzrt::Site* after = nullptr;
  const kzrt::Site* effect = nullptr;
};

struct BindLine {
  uint32_t id = 0, site = 0, module = 0, vals = 0;
  std::string at, status, reason;
};

struct Unit {
  std::string tu;
  std::vector<tooling::CompileCommand> cmds;
};

std::string hex(uint32_t v) {
  char b[16];
  std::snprintf(b, sizeof b, "0x%x", v);
  return b;
}

bool ends_with_path(const std::string& file, const std::string& tu) {
  if (file == tu) return true;
  return file.size() > tu.size() && file.compare(file.size() - tu.size(), tu.size(), tu) == 0 && file[file.size() - tu.size() - 1] == '/';
}

std::string abs_file(const tooling::CompileCommand& c) {
  llvm::SmallString<256> p(c.Filename);
  if (!llvm::sys::path::is_absolute(p)) {
    llvm::SmallString<256> d(c.Directory);
    llvm::sys::path::append(d, p);
    p = d;
  }
  llvm::sys::path::remove_dots(p, true);
  return std::string(p.str());
}

// ---- generated C text ---------------------------------------------------------------------------------

unsigned type_bits(const std::string& t) {
  if (t == "ptr") return 64;
  return static_cast<unsigned>(std::atoi(t.c_str() + 1));
}

// The 64-bit hook operand of a value of manifest type `t` (zero- or sign-extended per the manifest).
std::string u64_of(const std::string& e, const std::string& t) {
  if (t == "ptr") return "(uint64_t)(uintptr_t)(" + e + ")";
  const std::string bits = std::to_string(type_bits(t));
  if (t[0] == 's') return "(uint64_t)(int64_t)(int" + bits + "_t)(" + e + ")";
  return "(uint64_t)(uint" + bits + "_t)(" + e + ")";
}

struct Cap {
  uint32_t slot = 0;
  std::string expr;  // C expression of the value (already uint64_t)
  std::string cond;  // non-empty: the value is bound only when this holds (valid bit at run time)
  bool str = false;
  uint32_t bound = 0;
};

std::string record_call(uint32_t site, uint32_t bind, const std::string& link, const std::vector<Cap>& caps) {
  uint32_t fixed = 0;
  std::string cond_bits;
  std::string v[4] = {"0u", "0u", "0u", "0u"};
  const Cap* s = nullptr;
  for (const Cap& c : caps) {
    v[c.slot] = c.expr;
    if (c.cond.empty()) fixed |= 1u << c.slot;
    else cond_bits += " | ((" + c.cond + ") ? " + hex(1u << c.slot) + "u : 0u)";
    if (c.str) s = &c;
  }
  std::string valid = cond_bits.empty() ? hex(fixed) + "u" : "(" + hex(fixed) + "u" + cond_bits + ")";
  std::string out = s != nullptr ? "kz_rt_obs_str(" : "kz_rt_obs(";
  out += std::to_string(site) + "u, " + std::to_string(bind) + "u, " + link + ", " + valid;
  for (const std::string& x : v) out += ", " + x;
  if (s != nullptr) out += ", " + std::to_string(s->slot) + "u, " + std::to_string(s->bound) + "u";
  return out + ")";
}

// ---- the per-unit transform -------------------------------------------------------------------------------

struct UnitResult {
  std::string text;  // the edited main file (empty: no edit)
  std::string main_path;
  std::vector<BindLine> binds;
  std::vector<std::string> report;
  bool error = false;
};

class Transform {
 public:
  Transform(ASTContext& ctx, Rewriter& rw, const std::vector<const Group*>& groups, const std::vector<SourceRange>& skipped,
            uint32_t module, uint32_t* counter, const std::string& backend, UnitResult* out)
      : ctx_(ctx), sm_(ctx.getSourceManager()), lo_(ctx.getLangOpts()), rw_(rw), pp_(ctx.getLangOpts()), groups_(groups),
        skipped_(skipped), module_(module), counter_(counter), backend_(backend), out_(out) {
    pp_.SuppressTagKeyword = false;
  }

  void run() {
    collect_definitions();
    for (const Group* g : groups_) {
      switch (g->form) {
        case Form::CallSite: call_site(*g); break;
        case Form::Callee: callee(*g); break;
        case Form::Store: store(*g); break;
      }
    }
    emit_pre_texts();
    // Replacements first, then insertions in the order they were made: a replacement's size is then never
    // measured across text inserted at the same offset, and an insertion before a replaced token stays before it.
    for (const Ed& e : eds_)
      if (e.kind == 0) rw_.ReplaceText(e.range, e.text);
    for (const Ed& e : eds_) {
      if (e.kind == 1) rw_.InsertTextBefore(e.loc, e.text);
      if (e.kind == 2) rw_.InsertTextAfter(e.loc, e.text);
    }
  }

 private:
  struct Ed {
    int kind = 0;  // 0 replace, 1 insert before, 2 insert after
    CharSourceRange range;
    SourceLocation loc;
    std::string text;
  };
  std::vector<Ed> eds_;
  void ed_replace(CharSourceRange r, const std::string& t) { eds_.push_back(Ed{0, r, SourceLocation(), t}); }
  void ed_before(SourceLocation l, const std::string& t) { eds_.push_back(Ed{1, CharSourceRange(), l, t}); }
  void ed_after(SourceLocation l, const std::string& t) { eds_.push_back(Ed{2, CharSourceRange(), l, t}); }
  // ---- locations
  std::string where(SourceLocation loc) {
    SourceLocation e = sm_.getExpansionLoc(loc);
    PresumedLoc p = sm_.getPresumedLoc(e);
    if (p.isInvalid()) return "<invalid>";
    std::string f = p.getFilename();
    const size_t slash = f.rfind('/');
    if (slash != std::string::npos) f = f.substr(slash + 1);
    return f + ":" + std::to_string(p.getLine()) + ":" + std::to_string(p.getColumn());
  }
  unsigned line_of(SourceLocation loc) { return sm_.getExpansionLineNumber(loc); }
  // The macros a location is expanded through, outermost first ("A via B").
  std::string macro_chain(SourceLocation l) {
    std::vector<std::string> names;
    while (l.isMacroID()) {
      names.push_back(Lexer::getImmediateMacroName(l, sm_, lo_).str());
      l = sm_.getImmediateMacroCallerLoc(l);
    }
    std::string s;
    for (size_t i = names.size(); i-- > 0;) s += (s.empty() ? "" : " via ") + names[i];
    return s;
  }
  // "" when the token at `l` may be edited: file text, or the first (begin) / last (end) token of a macro use
  // written in the file (the whole use is then inside the edited range); else what prevents the edit.
  std::string loc_problem(SourceLocation l, bool begin) {
    if (l.isFileID()) return "";
    if (sm_.isMacroArgExpansion(l)) return "macro argument (" + macro_chain(l) + ")";
    SourceLocation edge;
    const bool at = begin ? Lexer::isAtStartOfMacroExpansion(l, sm_, lo_, &edge) : Lexer::isAtEndOfMacroExpansion(l, sm_, lo_, &edge);
    if (at && edge.isFileID()) return "";
    return "macro body (" + macro_chain(l) + ")";
  }
  std::string macro_context(SourceRange r) {
    std::string p = loc_problem(r.getBegin(), true);
    if (p.empty()) p = loc_problem(r.getEnd(), false);
    return p;
  }
  bool file_range(SourceRange r, CharSourceRange* out) {
    if (!macro_context(r).empty()) return false;
    CharSourceRange c = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(r), sm_, lo_);
    if (c.isInvalid() || !sm_.isInMainFile(c.getBegin())) return false;
    *out = c;
    return true;
  }
  // The ';' that follows file offset `end` (white space and comments skipped).
  bool semi_after(SourceLocation end, SourceLocation* semi) {
    const std::pair<FileID, unsigned> d = sm_.getDecomposedLoc(end);
    StringRef buf = sm_.getBufferData(d.first);
    size_t i = d.second;
    while (i < buf.size()) {
      const char ch = buf[i];
      if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') { ++i; continue; }
      if (buf.substr(i).starts_with("/*")) {
        const size_t e = buf.find("*/", i + 2);
        if (e == StringRef::npos) return false;
        i = e + 2;
        continue;
      }
      if (buf.substr(i).starts_with("//")) {
        const size_t e = buf.find('\n', i);
        if (e == StringRef::npos) return false;
        i = e + 1;
        continue;
      }
      if (ch != ';') return false;
      *semi = sm_.getComposedLoc(d.first, static_cast<unsigned>(i));
      return true;
    }
    return false;
  }
  std::string type_text(QualType t, const std::string& name) {
    std::string s;
    llvm::raw_string_ostream os(s);
    t.print(os, pp_, name);
    return os.str();
  }
  bool check_type(QualType t, const kzrt::Value& v, std::string* why) {
    QualType c = t.getCanonicalType();
    if (v.type == "ptr") {
      if (!c->isPointerType()) { *why = "type=ptr but the C type is " + t.getAsString(pp_); return false; }
      return true;
    }
    if (!c->isIntegralOrEnumerationType()) { *why = "the C type " + t.getAsString(pp_) + " is not an integer type (non-scalar values are not supported)"; return false; }
    const uint64_t w = ctx_.getTypeSize(c);
    if (w > type_bits(v.type)) { *why = "type=" + v.type + " is narrower than the C type " + t.getAsString(pp_); return false; }
    const bool sgn = c->isSignedIntegerOrEnumerationType();
    if (sgn != (v.type[0] == 's')) { *why = "type=" + v.type + " and the C type " + t.getAsString(pp_) + " differ in signedness"; return false; }
    return true;
  }
  uint32_t next_bind() { return (module_ << 16) | ++*counter_; }

  void add_bind(const kzrt::Site* s, uint32_t id, const std::string& at, const std::string& status, const std::string& reason, uint32_t vals) {
    BindLine b;
    b.id = id;
    b.site = s->id;
    b.module = module_;
    b.at = at;
    b.status = status;
    b.reason = reason;
    b.vals = vals;
    out_->binds.push_back(b);
  }
  void unsupported(const Group& g, const std::string& at, const std::string& reason) {
    for (const kzrt::Site* s : {g.before, g.after, g.effect})
      if (s != nullptr) add_bind(s, next_bind(), at, "unsupported", reason, 0);
    note("  unsupported " + at + ": " + reason);
  }
  void note(const std::string& s) { out_->report.push_back(s); }

  // ---- the unit's function definitions
  void collect_definitions() {
    for (Decl* d : ctx_.getTranslationUnitDecl()->decls()) {
      auto* fd = dyn_cast<FunctionDecl>(d);
      if (fd == nullptr || !fd->doesThisDeclarationHaveABody()) continue;
      if (!sm_.isInMainFile(sm_.getExpansionLoc(fd->getLocation()))) continue;
      defs_[fd->getNameAsString()] = fd;
    }
  }
  FunctionDecl* definition(const Group& g) {
    auto it = defs_.find(g.func);
    if (it != defs_.end()) return it->second;
    std::string why = "function " + g.func + " is not defined in this unit under this configuration";
    for (SourceRange r : skipped_) {
      bool inv = false;
      StringRef text = Lexer::getSourceText(CharSourceRange::getCharRange(r), sm_, lo_, &inv);
      if (!inv && text.contains(g.func)) why += "; its name appears in the skipped preprocessor region at " + where(r.getBegin());
    }
    unsupported(g, g.tu + " " + g.func, why);
    return nullptr;
  }

  // Text inserted before a function definition: at the start of its first line, followed by #line.
  bool add_pre_text(FunctionDecl* fd, const std::string& text) {
    SourceLocation b = fd->getBeginLoc();
    if (b.isMacroID()) {
      // e.g. `bool` from <stdbool.h>: the declaration starts with a whole macro use written in the file
      if (!loc_problem(b, true).empty()) return false;
      b = sm_.getExpansionLoc(b);
    }
    const FileID fid = sm_.getFileID(b);
    const unsigned off = sm_.getFileOffset(b);
    StringRef buf = sm_.getBufferData(fid);
    unsigned start = off;
    while (start > 0 && buf[start - 1] != '\n') --start;
    for (unsigned i = start; i < off; ++i)
      if (buf[i] != ' ' && buf[i] != '\t') return false;
    auto& p = pre_[fd];
    p.loc = sm_.getComposedLoc(fid, start);
    p.line = sm_.getLineNumber(fid, start);
    p.offset = start;
    p.text += text;
    return true;
  }
  // A declaration placed right after the body's '{' (same line).
  void add_body_head(FunctionDecl* fd, const std::string& text) { head_[fd] += text; }

  void emit_pre_texts() {
    if (pre_.empty() && head_.empty()) return;
    // the include goes before the first function that uses a hook
    const FunctionDecl* first = nullptr;
    unsigned first_off = ~0u;
    for (auto& kv : head_) {
      if (pre_.count(kv.first) == 0) add_pre_text(const_cast<FunctionDecl*>(kv.first), "");
    }
    for (auto& kv : pre_)
      if (kv.second.offset < first_off) { first_off = kv.second.offset; first = kv.first; }
    for (auto& kv : pre_) {
      std::string t;
      if (kv.first == first) t += "#include \"kz_rt.h\" /* kz-instr-clang: hook ABI 1 */\n";
      t += kv.second.text;
      if (t.empty()) continue;
      t += "#line " + std::to_string(kv.second.line) + "\n";
      ed_before(kv.second.loc, t);
    }
    for (auto& kv : head_) {
      const auto* body = dyn_cast<CompoundStmt>(kv.first->getBody());
      ed_after(body->getLBracLoc().getLocWithOffset(1), kv.second);
    }
  }

  // ---- values
  // An argument / parameter value: `e` is the C expression (a parameter name), `t` its type.
  bool value_cap(const kzrt::Value& v, const std::string& e, QualType t, Cap* c, std::string* why) {
    if (!check_type(t, v, why)) return false;
    c->slot = v.slot;
    c->expr = u64_of(e, v.type);
    if (v.type == "ptr" && v.string_mode == "validated") {
      c->str = true;
      c->bound = v.bound;
    }
    return true;
  }
  bool out_cap(const kzrt::Value& v, const std::string& p, QualType pt, Cap* c, std::string* why) {
    QualType pc = pt.getCanonicalType();
    if (!pc->isPointerType()) { *why = "from=" + v.from + " needs a pointer parameter"; return false; }
    if (v.string_mode == "validated") { *why = "a string through an out parameter is not supported"; return false; }
    if (!check_type(pc->getPointeeType(), v, why)) return false;
    c->slot = v.slot;
    c->expr = "(" + p + " != 0 ? " + u64_of("*" + p, v.type) + " : 0u)";
    c->cond = p + " != 0";
    return true;
  }
  static int index_of(const std::string& from, const char* prefix) {
    const size_t n = std::strlen(prefix);
    if (from.compare(0, n, prefix) != 0) return -1;
    return std::atoi(from.c_str() + n);
  }
  bool slots_ok(const kzrt::Site* s, std::string* why) {
    if (s == nullptr) return true;
    int strs = 0;
    for (const kzrt::Value& v : s->values) {
      if (v.slot > 3) { *why = "slot " + std::to_string(v.slot) + " > 3 (this connector passes v0..v3)"; return false; }
      if (v.string_mode == "validated") ++strs;
    }
    if (strs > 1) { *why = "more than one validated string in one record"; return false; }
    return true;
  }
  // The captures of a site from a parameter list (names p[k], types t[k]) and a result (name r, type rt).
  bool caps_of(const kzrt::Site* s, const std::vector<std::string>& p, const std::vector<QualType>& t, const std::string& r, QualType rt,
               bool after, std::vector<Cap>* caps, uint32_t* vals, std::string* why) {
    *vals = 0;
    if (s == nullptr) return true;
    if (!slots_ok(s, why)) return false;
    for (const kzrt::Value& v : s->values) {
      Cap c;
      int k;
      if ((k = index_of(v.from, "arg")) >= 0) {
        if (static_cast<size_t>(k) >= p.size()) { *why = v.from + ": the function has " + std::to_string(p.size()) + " parameters"; return false; }
        if (!value_cap(v, p[k], t[k], &c, why)) return false;
      } else if (v.from == "ret") {
        if (!after || r.empty()) { *why = "from=ret at a site that is not after a return"; return false; }
        if (!value_cap(v, r, rt, &c, why)) return false;
      } else if ((k = index_of(v.from, "out")) >= 0) {
        if (!after) { *why = "from=" + v.from + " needs phase=after"; return false; }
        if (static_cast<size_t>(k) >= p.size()) { *why = v.from + ": the function has " + std::to_string(p.size()) + " parameters"; return false; }
        if (!out_cap(v, p[k], t[k], &c, why)) return false;
      } else {
        *why = "from=" + v.from + " is not available at this binding form";
        return false;
      }
      *vals |= 1u << v.slot;
      caps->push_back(c);
    }
    return true;
  }

  // ---- call sites
  struct CallFinder : RecursiveASTVisitor<CallFinder> {
    std::string callee;
    std::vector<CallExpr*> calls;
    bool VisitCallExpr(CallExpr* c) {
      const FunctionDecl* f = c->getDirectCallee();
      if (f != nullptr && f->getNameAsString() == callee) calls.push_back(c);
      return true;
    }
  };

  void census(const std::string& callee) {
    if (!censused_.insert(callee).second) return;
    CallFinder all;
    all.callee = callee;
    all.TraverseDecl(ctx_.getTranslationUnitDecl());
    unsigned in_file = 0, in_arg = 0, in_body = 0;
    std::set<std::string> macros;
    for (CallExpr* c : all.calls) {
      if (!sm_.isInMainFile(sm_.getExpansionLoc(c->getBeginLoc()))) continue;
      const SourceLocation name = c->getCallee()->IgnoreParenImpCasts()->getBeginLoc();
      if (name.isFileID()) ++in_file;
      else if (sm_.isMacroArgExpansion(name)) { ++in_arg; macros.insert(macro_chain(name)); }
      else { ++in_body; macros.insert(macro_chain(name)); }
    }
    std::string ms;
    for (const std::string& m : macros) ms += (ms.empty() ? "" : "; ") + m;
    note("  census " + callee + " in this unit: " + std::to_string(in_file + in_arg + in_body) + " call(s) whose callee name is " + std::to_string(in_file) +
         " file text, " + std::to_string(in_arg) + " in a macro argument, " + std::to_string(in_body) + " in a macro body" +
         (ms.empty() ? "" : " (macros: " + ms + ")"));
    unsigned inactive = 0;
    for (SourceRange r : skipped_) {
      bool inv = false;
      StringRef text = Lexer::getSourceText(CharSourceRange::getCharRange(r), sm_, lo_, &inv);
      if (!inv && text.contains(callee + "(")) {
        ++inactive;
        note("  inactive: " + callee + " is named in the skipped preprocessor region at " + where(r.getBegin()) + " (not instrumented in this configuration)");
      }
    }
    (void)inactive;
  }

  void call_site(const Group& g) {
    note("group call-site " + g.func + " -> " + g.callee + (g.line != 0 ? " line " + std::to_string(g.line) : "") + " (sites" +
         (g.before ? " " + std::to_string(g.before->id) : "") + (g.after ? " " + std::to_string(g.after->id) : "") + ")");
    FunctionDecl* fd = definition(g);
    if (fd == nullptr) return;
    census(g.callee);
    CallFinder f;
    f.callee = g.callee;
    f.TraverseStmt(fd->getBody());
    std::vector<CallExpr*> calls;
    for (CallExpr* c : f.calls)
      if (g.line == 0 || line_of(c->getBeginLoc()) == g.line) calls.push_back(c);
    if (calls.empty()) {
      unsupported(g, g.tu + " " + g.func, "no call of " + g.callee + " in " + g.func + (g.line ? " at line " + std::to_string(g.line) : ""));
      return;
    }
    for (CallExpr* c : calls) one_call(g, fd, c);
  }

  void one_call(const Group& g, FunctionDecl* fd, CallExpr* c) {
    const std::string at = where(c->getBeginLoc());
    const FunctionDecl* callee = c->getDirectCallee();
    const SourceLocation name_loc = c->getCallee()->IgnoreParenImpCasts()->getBeginLoc();
    if (!name_loc.isFileID()) {
      unsupported(g, at, std::string("the callee name is in a ") + (sm_.isMacroArgExpansion(name_loc) ? "macro argument" : "macro body") + " (" +
                             macro_chain(name_loc) + "; macro text is not edited)");
      return;
    }
    const auto* proto = callee->getType()->getAs<FunctionProtoType>();
    if (proto == nullptr) { unsupported(g, at, "the callee has no prototype"); return; }
    // the callee must be declared before the enclosing function (the helpers are defined there)
    if (!sm_.isBeforeInTranslationUnit(callee->getFirstDecl()->getLocation(), fd->getBeginLoc())) {
      unsupported(g, at, "the callee is first declared inside or after " + g.func);
      return;
    }
    std::vector<std::string> pn;
    std::vector<QualType> pt;
    for (unsigned i = 0; i < proto->getNumParams(); ++i) {
      pn.push_back("kz_p" + std::to_string(i));
      pt.push_back(proto->getParamType(i));
    }
    const QualType rt = proto->getReturnType();
    const bool is_void = rt->isVoidType();
    if (proto->isVariadic()) { tap_call(g, fd, c, pn, pt, rt, at); return; }

    const uint32_t bb = g.before ? next_bind() : 0, ab = g.after ? next_bind() : 0;
    std::vector<Cap> bc, ac;
    uint32_t bv = 0, av = 0;
    std::string why;
    if (!caps_of(g.before, pn, pt, "", rt, false, &bc, &bv, &why) || !caps_of(g.after, pn, pt, is_void ? "" : "kz_r", rt, true, &ac, &av, &why)) {
      *counter_ -= (bb ? 1 : 0) + (ab ? 1 : 0);
      unsupported(g, at, why);
      return;
    }
    const auto* callee_ref = dyn_cast<DeclRefExpr>(c->getCallee()->IgnoreParenImpCasts());
    CharSourceRange name_range;
    if (callee_ref == nullptr || !file_range(callee_ref->getSourceRange(), &name_range)) {
      *counter_ -= (bb ? 1 : 0) + (ab ? 1 : 0);
      unsupported(g, at, "the callee is not named by a plain identifier in the file text");
      return;
    }
    const std::string w = "kz_w" + std::to_string(bb ? bb : ab);
    std::string params, args;
    for (size_t i = 0; i < pn.size(); ++i) {
      params += (i ? ", " : "") + type_text(pt[i], pn[i]);
      args += (i ? ", " : "") + pn[i];
    }
    if (params.empty()) params = "void";
    std::string t = "/* kz-instr-clang call-site wrapper: " + g.callee + " called by " + g.func + " at " + at + "; binds " +
                    (bb ? std::to_string(bb) : "-") + "/" + (ab ? std::to_string(ab) : "-") + " */\n";
    t += "static inline " + type_text(rt, w + "(" + params + ")") + "\n{\n";
    const std::string link = bb ? "kz_e" : "0u";
    if (bb) t += "    uint32_t kz_e = " + record_call(g.before->id, bb, "0u", bc) + ";\n";
    if (is_void) {
      t += "    " + g.callee + "(" + args + ");\n";
      if (ab) t += "    (void)" + record_call(g.after->id, ab, link, ac) + ";\n";
      else if (bb) t += "    (void)kz_e;\n";
    } else {
      t += "    " + type_text(rt.getUnqualifiedType(), "kz_r") + " = " + g.callee + "(" + args + ");\n";
      if (ab) t += "    (void)" + record_call(g.after->id, ab, link, ac) + ";\n";
      else if (bb) t += "    (void)kz_e;\n";
      t += "    return kz_r;\n";
    }
    t += "}\n";
    if (!add_pre_text(fd, t)) {
      *counter_ -= (bb ? 1 : 0) + (ab ? 1 : 0);
      unsupported(g, at, "the enclosing function does not start a line");
      return;
    }
    ed_replace(name_range, w);
    if (bb) add_bind(g.before, bb, at + " " + g.func + "->" + g.callee + " call-site-wrapper before", "bound", "", bv);
    if (ab) add_bind(g.after, ab, at + " " + g.func + "->" + g.callee + " call-site-wrapper after", "bound", "", av);
    note("  bound " + at + " call-site wrapper " + w + " binds " + (bb ? std::to_string(bb) : "-") + "/" + (ab ? std::to_string(ab) : "-"));
  }

  void tap_call(const Group& g, FunctionDecl* fd, CallExpr* c, const std::vector<std::string>& pn, const std::vector<QualType>& pt, QualType rt,
                const std::string& at) {
    const uint32_t b = next_bind();
    const uint32_t ab = g.after ? next_bind() : 0;
    auto fail = [&](const std::string& why) {
      *counter_ -= 1 + (ab ? 1 : 0);
      unsupported(g, at, why + " (variadic callee: argument taps)");
    };
    CharSourceRange call_range;
    if (!file_range(c->getSourceRange(), &call_range)) return fail("the call's text is not plain file text");
    const std::string l = "kz_l" + std::to_string(b);
    std::string t = "/* kz-instr-clang call-site taps (variadic callee): " + g.callee + " called by " + g.func + " at " + at + "; binds " +
                    std::to_string(b) + "/" + (ab ? std::to_string(ab) : "-") + " */\n";
    struct Wrap { CharSourceRange r; std::string pre, post; };
    std::vector<Wrap> eds;
    uint32_t bv = 0;
    if (g.before != nullptr) {
      std::string why;
      if (!slots_ok(g.before, &why)) return fail(why);
      for (const kzrt::Value& v : g.before->values) {
        const int k = index_of(v.from, "arg");
        if (k < 0) return fail("from=" + v.from + " at a tapped call");
        if (static_cast<size_t>(k) >= pn.size()) return fail(v.from + " is not a fixed parameter");
        if (static_cast<unsigned>(k) >= c->getNumArgs()) return fail(v.from + " has no argument");
        Cap cap;
        if (!value_cap(v, "kz_v", pt[k], &cap, &why)) return fail(why);
        CharSourceRange ar;
        if (!file_range(c->getArg(k)->getSourceRange(), &ar)) return fail("argument " + std::to_string(k) + " is not plain file text");
        const std::string fn = "kz_t" + std::to_string(b) + "_" + std::to_string(k);
        const QualType vt = pt[k].getUnqualifiedType();
        t += "static inline " + type_text(vt, fn + "(uint32_t *kz_l, " + type_text(vt, "kz_v") + ")") + "\n{\n    *kz_l = " +
             record_call(g.before->id, b, "0u", {cap}) + ";\n    return kz_v;\n}\n";
        eds.push_back({ar, fn + "(&" + l + ", (", "))"});
        bv |= 1u << v.slot;
      }
    }
    uint32_t av = 0;
    if (g.after != nullptr) {
      if (rt->isVoidType()) return fail("a void variadic callee has no result to tap");
      std::vector<Cap> ac;
      std::string why;
      if (!slots_ok(g.after, &why)) return fail(why);
      for (const kzrt::Value& v : g.after->values) {
        if (v.from != "ret") return fail("from=" + v.from + " after a tapped call (only ret)");
        Cap cap;
        if (!value_cap(v, "kz_v", rt, &cap, &why)) return fail(why);
        ac.push_back(cap);
        av |= 1u << v.slot;
      }
      const std::string fn = "kz_r" + std::to_string(ab);
      const QualType vt = rt.getUnqualifiedType();
      t += "static inline " + type_text(vt, fn + "(const uint32_t *kz_l, " + type_text(vt, "kz_v") + ")") + "\n{\n    (void)" +
           record_call(g.after->id, ab, "*kz_l", ac) + ";\n    return kz_v;\n}\n";
      eds.push_back({call_range, fn + "(&" + l + ", (", "))"});
    }
    if (!add_pre_text(fd, t)) return fail("the enclosing function does not start a line");
    add_body_head(fd, " uint32_t " + l + " = 0u;");
    for (const Wrap& e : eds) {
      ed_before(e.r.getBegin(), e.pre);
      ed_after(e.r.getEnd(), e.post);
    }
    if (g.before != nullptr) add_bind(g.before, b, at + " " + g.func + "->" + g.callee + " call-site-taps before", "bound", "", bv);
    if (ab) add_bind(g.after, ab, at + " " + g.func + "->" + g.callee + " call-site-taps after", "bound", "", av);
    note("  bound " + at + " call-site taps binds " + (g.before ? std::to_string(b) : "-") + "/" + (ab ? std::to_string(ab) : "-") +
         " (one before record per captured argument)");
  }

  // ---- callee bodies
  struct ReturnFinder : RecursiveASTVisitor<ReturnFinder> {
    std::vector<ReturnStmt*> rets;
    std::set<const ParmVarDecl*> assigned;
    bool VisitReturnStmt(ReturnStmt* r) { rets.push_back(r); return true; }
    bool VisitBinaryOperator(BinaryOperator* b) {
      if (b->isAssignmentOp()) mark(b->getLHS());
      return true;
    }
    bool VisitUnaryOperator(UnaryOperator* u) {
      if (u->isIncrementDecrementOp() || u->getOpcode() == UO_AddrOf) mark(u->getSubExpr());
      return true;
    }
    void mark(Expr* e) {
      if (auto* d = dyn_cast<DeclRefExpr>(e->IgnoreParenImpCasts()))
        if (auto* p = dyn_cast<ParmVarDecl>(d->getDecl())) assigned.insert(p);
    }
    bool TraverseLambdaExpr(LambdaExpr*) { return true; }
  };

  void callee(const Group& g) {
    note("group callee " + g.func + " (sites" + (g.before ? " " + std::to_string(g.before->id) : "") + (g.after ? " " + std::to_string(g.after->id) : "") + ")");
    FunctionDecl* fd = definition(g);
    if (fd == nullptr) return;
    const std::string at = where(fd->getLocation());
    std::vector<std::string> pn;
    std::vector<QualType> pt;
    for (unsigned i = 0; i < fd->getNumParams(); ++i) {
      const ParmVarDecl* p = fd->getParamDecl(i);
      if (p->getName().empty()) { unsupported(g, at, "parameter " + std::to_string(i) + " has no name"); return; }
      pn.push_back(p->getNameAsString());
      pt.push_back(p->getType());
    }
    const QualType rt = fd->getReturnType();
    const bool is_void = rt->isVoidType();
    auto* body = dyn_cast<CompoundStmt>(fd->getBody());
    if (body == nullptr || body->getLBracLoc().isMacroID() || body->getRBracLoc().isMacroID()) { unsupported(g, at, "the body's braces are not file text"); return; }
    const uint32_t eb = g.before ? next_bind() : 0, rb = g.after ? next_bind() : 0;
    std::vector<Cap> ec, rc;
    uint32_t ev = 0, rv = 0;
    std::string why;
    if (!caps_of(g.before, pn, pt, "", rt, false, &ec, &ev, &why) || !caps_of(g.after, pn, pt, is_void ? "" : "kz_r", rt, true, &rc, &rv, &why)) {
      *counter_ -= (eb ? 1 : 0) + (rb ? 1 : 0);
      unsupported(g, at, why);
      return;
    }
    ReturnFinder rf;
    rf.TraverseStmt(body);
    if (g.after != nullptr) {
      for (const kzrt::Value& v : g.after->values) {
        // a parameter read at a return (arg<k>, out<k>) must still hold the value the caller passed
        int k = index_of(v.from, "out");
        if (k < 0) k = index_of(v.from, "arg");
        if (k >= 0 && rf.assigned.count(fd->getParamDecl(static_cast<unsigned>(k))) != 0) {
          *counter_ -= (eb ? 1 : 0) + (rb ? 1 : 0);
          unsupported(g, at, "parameter " + pn[k] + " read at the return is assigned or has its address taken in the body");
          return;
        }
      }
    }
    const std::string e = "kz_e" + std::to_string(eb ? eb : rb);
    if (eb && rb) add_body_head(fd, " uint32_t " + e + " = " + record_call(g.before->id, eb, "0u", ec) + ";");
    else if (eb) add_body_head(fd, " (void)" + record_call(g.before->id, eb, "0u", ec) + ";");
    else add_body_head(fd, " const uint32_t " + e + " = 0u;");
    if (!add_pre_text(fd, "")) {
      *counter_ -= (eb ? 1 : 0) + (rb ? 1 : 0);
      unsupported(g, at, "the function does not start a line");
      head_.erase(fd);
      return;
    }
    unsigned rewritten = 0, skipped = 0;
    if (rb) {
      const std::string rec = "(void)" + record_call(g.after->id, rb, e, rc) + ";";
      for (ReturnStmt* r : rf.rets) {
        const SourceLocation kw = r->getReturnLoc();
        const std::string rat = where(kw);
        if (!kw.isFileID()) {
          ++skipped;
          note("  uninstrumented return path " + rat + ": the return is in a macro expansion (" + macro_chain(kw) + ")");
          continue;
        }
        // `return E;` -> `{ T kz_r = (E); record; return kz_r; }` by three local edits (the text of E is kept,
        // with any other edit inside it); `return;` -> `{ record; return; }`
        SourceLocation end = Lexer::getLocForEndOfToken(kw, 0, sm_, lo_);
        CharSourceRange er;
        if (r->getRetValue() != nullptr) {
          if (!file_range(r->getRetValue()->getSourceRange(), &er)) {
            ++skipped;
            note("  uninstrumented return path " + rat + ": the value's text is not file text (" + macro_context(r->getRetValue()->getSourceRange()) + ")");
            continue;
          }
          end = er.getEnd();
        }
        SourceLocation semi;
        if (!semi_after(end, &semi)) { ++skipped; note("  uninstrumented return path " + rat + ": no ';' in the file text after it"); continue; }
        // the ';' stays; the record and the closing brace go after it
        const SourceLocation after_semi = semi.getLocWithOffset(1);
        if (r->getRetValue() == nullptr) {
          ed_replace(CharSourceRange::getTokenRange(kw, kw), "{ " + rec + " return");
          ed_after(after_semi, " }");
        } else {
          ed_replace(CharSourceRange::getTokenRange(kw, kw), "{ " + type_text(rt.getUnqualifiedType(), "kz_r") + " = (");
          ed_after(er.getEnd(), ")");
          ed_after(after_semi, " " + rec + " return kz_r; }");
        }
        ++rewritten;
      }
      if (is_void) {
        ed_before(body->getRBracLoc(), rec + " ");
        ++rewritten;
      }
      note("  callee returns: " + std::to_string(rewritten) + " path(s) instrumented, " + std::to_string(skipped) + " uninstrumented");
    }
    if (eb) add_bind(g.before, eb, at + " " + g.func + " callee-entry", "bound", "", ev);
    if (rb) {
      std::string reason;
      if (skipped != 0) reason = std::to_string(skipped) + " return path(s) inside macro expansions are not instrumented";
      add_bind(g.after, rb, at + " " + g.func + " callee-returns " + std::to_string(rewritten) + "/" + std::to_string(rewritten + skipped), rewritten ? "bound" : "unsupported",
               rewritten ? reason : (reason.empty() ? "no return path could be instrumented" : reason), rv);
    }
    note("  bound " + at + " callee binds " + (eb ? std::to_string(eb) : "-") + "/" + (rb ? std::to_string(rb) : "-"));
  }

  // ---- store operands
  struct AssignFinder : RecursiveASTVisitor<AssignFinder> {
    std::vector<BinaryOperator*> as;
    bool VisitBinaryOperator(BinaryOperator* b) {
      if (b->getOpcode() == BO_Assign) as.push_back(b);
      return true;
    }
  };

  void store(const Group& g) {
    const kzrt::Site* s = g.effect;
    note("group store " + g.func + " line " + std::to_string(g.line) + " (site " + std::to_string(s->id) + ")");
    FunctionDecl* fd = definition(g);
    if (fd == nullptr) return;
    if (g.line == 0) { unsupported(g, g.func, "a store site needs line="); return; }
    AssignFinder f;
    f.TraverseStmt(fd->getBody());
    unsigned n = 0;
    for (BinaryOperator* a : f.as) {
      if (line_of(a->getOperatorLoc()) != g.line) continue;
      ++n;
      const std::string at = where(a->getOperatorLoc());
      const QualType lt = a->getLHS()->getType().getUnqualifiedType();
      if (s->values.size() != 1 || s->values[0].from != "operand0") { unsupported(g, at, "a store site has exactly one value from=operand0"); continue; }
      std::string why;
      Cap cap;
      if (!slots_ok(s, &why) || !value_cap(s->values[0], "kz_v", lt, &cap, &why)) { unsupported(g, at, why); continue; }
      CharSourceRange rr;
      if (!file_range(a->getRHS()->getSourceRange(), &rr)) { unsupported(g, at, "the stored operand is not plain file text"); continue; }
      const uint32_t b = next_bind();
      const std::string fn = "kz_s" + std::to_string(b);
      const std::string t = "/* kz-instr-clang store tap: " + g.func + " at " + at + "; bind " + std::to_string(b) + " */\nstatic inline " +
                            type_text(lt, fn + "(" + type_text(lt, "kz_v") + ")") + "\n{\n    (void)" + record_call(s->id, b, "0u", {cap}) +
                            ";\n    return kz_v;\n}\n";
      if (!add_pre_text(fd, t)) { --*counter_; unsupported(g, at, "the enclosing function does not start a line"); continue; }
      ed_before(rr.getBegin(), fn + "((");
      ed_after(rr.getEnd(), "))");
      add_bind(s, b, at + " " + g.func + " store-operand", "bound", "", 1u << s->values[0].slot);
      note("  bound " + at + " store tap " + fn);
    }
    if (n == 0) unsupported(g, g.func + ":" + std::to_string(g.line), "no simple assignment at that line");
  }

  struct Pre {
    SourceLocation loc;
    unsigned line = 0, offset = 0;
    std::string text;
  };
  ASTContext& ctx_;
  SourceManager& sm_;
  const LangOptions& lo_;
  Rewriter& rw_;
  PrintingPolicy pp_;
  const std::vector<const Group*>& groups_;
  const std::vector<SourceRange>& skipped_;
  uint32_t module_;
  uint32_t* counter_;
  std::string backend_;
  UnitResult* out_;
  std::map<std::string, FunctionDecl*> defs_;
  std::map<const FunctionDecl*, Pre> pre_;
  std::map<const FunctionDecl*, std::string> head_;
  std::set<std::string> censused_;
};

// ---- front-end plumbing -------------------------------------------------------------------------------

class Skipped : public PPCallbacks {
 public:
  Skipped(SourceManager& sm, std::vector<SourceRange>* out) : sm_(sm), out_(out) {}
  void SourceRangeSkipped(SourceRange r, SourceLocation) override {
    if (sm_.isInMainFile(r.getBegin())) out_->push_back(r);
  }

 private:
  SourceManager& sm_;
  std::vector<SourceRange>* out_;
};

struct Job {
  const std::vector<const Group*>* groups;
  uint32_t module = 0;
  uint32_t* counter = nullptr;
  std::string backend;
  UnitResult* out = nullptr;
};

class Consumer : public ASTConsumer {
 public:
  Consumer(CompilerInstance& ci, Job job, std::vector<SourceRange>* skipped) : ci_(ci), job_(job), skipped_(skipped) {}
  void HandleTranslationUnit(ASTContext& ctx) override {
    Rewriter rw(ctx.getSourceManager(), ctx.getLangOpts());
    Transform t(ctx, rw, *job_.groups, *skipped_, job_.module, job_.counter, job_.backend, job_.out);
    t.run();
    const FileID main = ctx.getSourceManager().getMainFileID();
    if (auto fe = ctx.getSourceManager().getFileEntryRefForID(main)) job_.out->main_path = std::string(fe->getName());
    if (const auto* buf = rw.getRewriteBufferFor(main)) {
      std::string s;
      llvm::raw_string_ostream os(s);
      buf->write(os);
      job_.out->text = os.str();
    }
    if (ci_.getDiagnostics().hasErrorOccurred()) job_.out->error = true;
  }

 private:
  CompilerInstance& ci_;
  Job job_;
  std::vector<SourceRange>* skipped_;
};

class Action : public ASTFrontendAction {
 public:
  explicit Action(Job job) : job_(job) {}
  bool BeginSourceFileAction(CompilerInstance& ci) override {
    ci.getPreprocessor().addPPCallbacks(std::make_unique<Skipped>(ci.getSourceManager(), &skipped_));
    return true;
  }
  std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, llvm::StringRef) override {
    return std::make_unique<Consumer>(ci, job_, &skipped_);
  }

 private:
  Job job_;
  std::vector<SourceRange> skipped_;
};

class Factory : public tooling::FrontendActionFactory {
 public:
  explicit Factory(Job job) : job_(job) {}
  std::unique_ptr<FrontendAction> create() override { return std::make_unique<Action>(job_); }

 private:
  Job job_;
};

class OneCommandDb : public tooling::CompilationDatabase {
 public:
  explicit OneCommandDb(tooling::CompileCommand c) : c_(std::move(c)) {}
  std::vector<tooling::CompileCommand> getCompileCommands(llvm::StringRef f) const override {
    return f == c_.Filename ? std::vector<tooling::CompileCommand>{c_} : std::vector<tooling::CompileCommand>{};
  }
  std::vector<std::string> getAllFiles() const override { return {c_.Filename}; }
  std::vector<tooling::CompileCommand> getAllCompileCommands() const override { return {c_}; }

 private:
  tooling::CompileCommand c_;
};

bool run_unit(const tooling::CompileCommand& cmd, const Options& o, Job job, std::string* err) {
  tooling::CompileCommand c = cmd;
  c.Filename = abs_file(cmd);
  OneCommandDb db(c);
  tooling::ClangTool tool(db, std::vector<std::string>{c.Filename});
  std::vector<std::string> adapt = {"-Wno-unknown-warning-option", "-w"};
  if (!o.resource_dir.empty()) adapt.push_back("-resource-dir=" + o.resource_dir);
  tool.appendArgumentsAdjuster(tooling::getInsertArgumentAdjuster(adapt, tooling::ArgumentInsertPosition::END));
  Factory f(job);
  const int rc = tool.run(&f);
  if (rc != 0 || job.out->error) {
    *err = "cannot parse " + c.Filename + " (status " + std::to_string(rc) + ")";
    return false;
  }
  return true;
}

std::string map_path(const std::string& p, const Options& o) {
  size_t best = 0;
  std::string out;
  for (const auto& m : o.maps)
    if (p.compare(0, m.first.size(), m.first) == 0 && m.first.size() > best && (p.size() == m.first.size() || p[m.first.size()] == '/')) {
      best = m.first.size();
      out = m.second + p.substr(m.first.size());
    }
  return out;
}

bool read_text(const std::string& path, std::string* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream ss;
  ss << in.rdbuf();
  *out = ss.str();
  return true;
}

int usage() {
  std::fprintf(stderr,
               "usage: kz-instr-clang --sites <logical.kzs> (--db <compile_commands.json>... | --fixed <dir> -- <args>)\n"
               "                      --map <from>=<to>... --module <id>=<path-part>|*... [--exclude <text>]...\n"
               "                      [--resource-dir <dir>] [--backend <name>] --binds-out <file> --report-out <file>\n");
  return 2;
}

}  // namespace

int main(int argc, char** argv) {
  Options o;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](std::string* out) {
      if (i + 1 >= argc) return false;
      *out = argv[++i];
      return true;
    };
    std::string v;
    if (a == "--") {
      for (++i; i < argc; ++i) o.fixed_args.push_back(argv[i]);
      break;
    }
    if (a == "--sites" && val(&o.sites)) continue;
    if (a == "--db" && val(&v)) { o.dbs.push_back(v); continue; }
    if (a == "--exclude" && val(&v)) { o.excludes.push_back(v); continue; }
    if (a == "--fixed" && val(&o.fixed_dir)) continue;
    if (a == "--resource-dir" && val(&o.resource_dir)) continue;
    if (a == "--backend" && val(&o.backend)) continue;
    if (a == "--binds-out" && val(&o.binds_out)) continue;
    if (a == "--report-out" && val(&o.report_out)) continue;
    if ((a == "--map" || a == "--module") && val(&v)) {
      const size_t eq = v.find('=');
      if (eq == std::string::npos || eq == 0) return usage();
      if (a == "--map") o.maps.emplace_back(v.substr(0, eq), v.substr(eq + 1));
      else o.modules.emplace_back(static_cast<uint32_t>(std::strtoul(v.substr(0, eq).c_str(), nullptr, 10)), v.substr(eq + 1));
      continue;
    }
    std::fprintf(stderr, "kz-instr-clang: unknown or incomplete argument %s\n", a.c_str());
    return usage();
  }
  if (o.sites.empty() || o.binds_out.empty() || o.report_out.empty() || o.maps.empty() || o.modules.empty() || (o.dbs.empty() == o.fixed_dir.empty()))
    return usage();

  // the logical sites
  kzrt::Manifest man;
  std::string text, err;
  if (!read_text(o.sites, &text)) { std::fprintf(stderr, "kz-instr-clang: cannot read %s\n", o.sites.c_str()); return 2; }
  if (!kzrt::parse_manifest(text, o.sites, &man, &err)) { std::fprintf(stderr, "kz-instr-clang: %s\n", err.c_str()); return 2; }
  for (const auto& m : o.modules)
    if (man.module(m.first) == nullptr) { std::fprintf(stderr, "kz-instr-clang: --module %u is not declared in %s\n", m.first, o.sites.c_str()); return 2; }

  // groups: a call's before/after, a function's entry/return, a store
  std::vector<std::unique_ptr<Group>> groups;
  std::map<std::string, Group*> by_key;
  std::vector<std::string> errors;
  for (const kzrt::Site& s : man.sites) {
    Group g;
    g.tu = s.tu;
    g.func = s.func;
    g.callee = s.callee;
    g.line = s.src_line;
    if (!s.callee.empty()) g.form = Form::CallSite;
    else if (s.phase == "effect") g.form = Form::Store;
    else g.form = Form::Callee;
    const std::string key = std::to_string(static_cast<int>(g.form)) + "|" + g.tu + "|" + g.func + "|" + g.callee + "|" + std::to_string(g.form == Form::Callee ? 0 : g.line) +
                            (g.form == Form::Store ? "|" + std::to_string(s.id) : "");
    Group*& slot = by_key[key];
    if (slot == nullptr) {
      groups.push_back(std::make_unique<Group>(g));
      slot = groups.back().get();
    }
    if (g.form == Form::Store) slot->effect = &s;
    else if (s.phase == "before") {
      if (slot->before) errors.push_back("two before sites for one binding: " + std::to_string(slot->before->id) + ", " + std::to_string(s.id));
      slot->before = &s;
    } else if (s.phase == "after" || s.phase == "exit") {
      if (slot->after) errors.push_back("two after sites for one binding: " + std::to_string(slot->after->id) + ", " + std::to_string(s.id));
      slot->after = &s;
    } else errors.push_back("site " + std::to_string(s.id) + ": phase " + s.phase + " is not bound by this connector");
  }
  for (const std::string& e : errors) std::fprintf(stderr, "kz-instr-clang: %s\n", e.c_str());
  if (!errors.empty()) return 2;

  // the recorded commands
  std::vector<tooling::CompileCommand> all;
  for (const std::string& d : o.dbs) {
    std::string e;
    auto db = tooling::JSONCompilationDatabase::loadFromFile(d, e, tooling::JSONCommandLineSyntax::AutoDetect);
    if (!db) { std::fprintf(stderr, "kz-instr-clang: %s: %s\n", d.c_str(), e.c_str()); return 2; }
    for (auto& c : db->getAllCompileCommands()) {
      bool ex = false;
      for (const std::string& x : o.excludes)
        if (c.Directory.find(x) != std::string::npos || c.Output.find(x) != std::string::npos) ex = true;
      if (!ex) all.push_back(c);
    }
  }
  std::map<std::string, Unit> units;
  for (const auto& g : groups) {
    Unit& u = units[g->tu];
    u.tu = g->tu;
  }
  for (auto& kv : units) {
    if (!o.fixed_dir.empty()) {
      std::vector<std::string> cl = {"cc"};
      cl.insert(cl.end(), o.fixed_args.begin(), o.fixed_args.end());
      llvm::SmallString<256> p(o.fixed_dir);
      llvm::sys::path::append(p, kv.first);
      cl.push_back(std::string(p.str()));
      kv.second.cmds.emplace_back(o.fixed_dir, std::string(p.str()), cl, "");
      continue;
    }
    for (const auto& c : all)
      if (ends_with_path(abs_file(c), kv.first)) kv.second.cmds.push_back(c);
  }

  std::map<uint32_t, uint32_t> counters;
  std::set<uint32_t> registered;
  std::vector<BindLine> binds;
  std::vector<std::string> report;
  report.push_back("kz-instr-clang report (backend " + o.backend + ")");
  report.push_back("sites " + o.sites);
  int status = 0;
  for (auto& kv : units) {
    Unit& u = kv.second;
    std::vector<const Group*> gs;
    for (const auto& g : groups)
      if (g->tu == u.tu) gs.push_back(g.get());
    report.push_back("");
    report.push_back("unit " + u.tu);
    if (u.cmds.empty()) {
      report.push_back("  no recorded command names this unit");
      for (const Group* g : gs)
        for (const kzrt::Site* s : {g->before, g->after, g->effect})
          if (s) binds.push_back(BindLine{0, s->id, o.modules.front().first, 0, u.tu, "unsupported", "translation unit not in the compile database"});
      status = 1;
      continue;
    }
    const std::string dir = u.cmds.front().Directory, file = abs_file(u.cmds.front());
    uint32_t module = 0;
    for (const auto& m : o.modules)
      if (m.second == "*" || dir.find(m.second) != std::string::npos || file.find(m.second) != std::string::npos) { module = m.first; break; }
    if (module == 0) { std::fprintf(stderr, "kz-instr-clang: no --module matches %s\n", file.c_str()); return 2; }
    uint32_t& counter = counters[module];
    const uint32_t start = counter;
    UnitResult first;
    for (size_t i = 0; i < u.cmds.size(); ++i) {
      UnitResult r;
      counter = start;
      Job job{&gs, module, &counter, o.backend, &r};
      if (!run_unit(u.cmds[i], o, job, &err)) { std::fprintf(stderr, "kz-instr-clang: %s\n", err.c_str()); return 1; }
      if (i == 0) {
        first = r;
        report.push_back("  command " + u.cmds[i].Directory + " -> " + u.cmds[i].Output + " (module " + std::to_string(module) + ")");
      } else if (r.text != first.text) {
        std::fprintf(stderr, "kz-instr-clang: %s: the transform differs under the command in %s\n", u.tu.c_str(), u.cmds[i].Directory.c_str());
        return 1;
      } else {
        report.push_back("  also " + u.cmds[i].Directory + " -> " + u.cmds[i].Output + ": same text");
      }
    }
    report.insert(report.end(), first.report.begin(), first.report.end());
    binds.insert(binds.end(), first.binds.begin(), first.binds.end());
    if (first.text.empty()) { report.push_back("  no edit"); continue; }
    if (registered.insert(module).second) {
      // the module tells the runtime it runs (kz_rt_module; a module never registered is "not loaded or not run")
      const std::string n = std::to_string(module);
      first.text += "\n/* kz-instr-clang: registration of module " + n + " */\nstatic void kz_module_" + n + "(void) __attribute__((constructor));\nstatic void kz_module_" + n +
                    "(void)\n{\n    kz_rt_module(" + n + "u, 0);\n}\n";
      report.push_back("  module " + n + " registers itself from this unit (a constructor at the end of the file)");
    }
    const std::string outp = map_path(first.main_path, o);
    if (outp.empty() || outp == first.main_path) { std::fprintf(stderr, "kz-instr-clang: no --map for %s\n", first.main_path.c_str()); return 2; }
    std::ofstream out(outp, std::ios::binary | std::ios::trunc);
    if (!out || !(out << first.text)) { std::fprintf(stderr, "kz-instr-clang: cannot write %s\n", outp.c_str()); return 1; }
    report.push_back("  wrote " + outp);
  }

  // the binding list (kzs-sites 1; read together with the logical sites)
  std::ofstream bo(o.binds_out, std::ios::trunc);
  bo << "kzs-sites 1\n# bindings of " << o.backend << " for the logical sites of " << o.sites << " (kz-instr-clang)\n";
  uint32_t fallback = 0x7fff0000u;
  for (const BindLine& b : binds) {
    bo << "bind " << (b.id != 0 ? b.id : ++fallback) << " site=" << b.site << " backend=" << o.backend << " module=" << b.module << " at=\"" << b.at << "\" status=" << b.status;
    if (b.vals != 0) bo << " vals=" << hex(b.vals);
    if (!b.reason.empty()) bo << " reason=\"" << b.reason << "\"";
    bo << "\n";
  }
  std::ofstream ro(o.report_out, std::ios::trunc);
  for (const std::string& l : report) ro << l << "\n";
  unsigned bound = 0, unsup = 0;
  for (const BindLine& b : binds) (b.status == "unsupported" ? unsup : bound)++;
  std::printf("kz-instr-clang: %zu unit(s), %u binding(s) bound, %u unsupported; binds %s, report %s\n", units.size(), bound, unsup, o.binds_out.c_str(), o.report_out.c_str());
  return status;
}
