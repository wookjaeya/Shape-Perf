// kz_lint -- the repository lints of Kairos.
//
//   kz_lint <rule> <root> --data <dir>        check the tree under <root>
//   kz_lint --selftest --data <dir>           check that every rule rejects what it must and
//                                             accepts what it must, on trees written on the fly
//
// Rules:
//   case-ids       no case identifier (data/case_identifiers.txt) in include/, lib/ and tools/, whether it
//                  stands alone or is a component of a longer name (check_R2, r2_x, R2Cert,
//                  CFE_ES_Main, a file name, a string or a comment); components are split at '_'
//                  and at camel-case boundaries and compared without regard to case
//   retired-term  no identifier containing "contract" in code, build files and profiles
//                  (comments and string literals are not identifiers; data/contract_allow.txt
//                  lists the few exact tokens that are allowed, e.g. the provenance tier name)
//   banned-files   no Python and no JSON files in the tool core
//   no-llvm        libkz-core, tools/kz, lib/proofcheck, include/ and the core tests do not
//                  use LLVM or Clang (headers, CMake names and paths, in any case); kz_use_llvm
//                  appears only in lib/llvm, tools/kz-static, instrument/clang (the Clang source-transform
//                  connector, SPEC rev 1.4 section 4.1) and its definition in
//                  cmake/KzSettings.cmake
//   no-generated   no generated code and no code that generates code (markers, templates,
//                  configure_file, file(WRITE|APPEND|CONFIGURE|GENERATE) of a source file,
//                  add_custom_command and execute_process that produce a source file)
//
// Not implemented: the "no bare constants" lint of Architecture/08_roadmap.md 3 (an experiment
// constant in the engine must come from a profile or a basis record with a file:line and hash).
// It needs the KZ_REF(value, "file:line") discipline of the components that use constants, which
// does not exist yet.  It is an open item of gate S0-R2 (README.md, status table).
//
// Exit status: 0 = clean, 1 = violations (printed as path:line: message), 2 = usage error.
//
// The selftest trees are written to a scratch directory at run time so that the repository
// never contains the files the lints forbid.

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

namespace {

struct Violation {
  std::string path;
  int line;
  std::string message;
};

using Violations = std::vector<Violation>;

// ---- file system helpers ---------------------------------------------------------------------

bool read_file(const fs::path& p, std::string* out) {
  std::ifstream f(p, std::ios::binary);
  if (!f) return false;
  std::ostringstream s;
  s << f.rdbuf();
  *out = s.str();
  return true;
}

bool looks_binary(const std::string& text) {
  size_t n = std::min<size_t>(text.size(), 4096);
  for (size_t i = 0; i < n; ++i)
    if (text[i] == '\0') return true;
  return false;
}

std::string rel(const fs::path& root, const fs::path& p) {
  std::error_code ec;
  return fs::relative(p, root, ec).generic_string();
}

bool skipped_dir(const std::string& relative) {
  // .git and build trees are never source; the repository holds no lint fixtures (they are
  // written to a scratch directory), but a stray `fixtures` directory of the lint is skipped.
  for (const char* s : {".git/", "build/"})
    if (relative.rfind(s, 0) == 0) return true;
  return relative.rfind("tests/lint/fixtures/", 0) == 0;
}

// Regular files under root/sub (or root itself when sub is empty), sorted by path.
std::vector<fs::path> files_under(const fs::path& root, const std::vector<std::string>& subs) {
  std::vector<fs::path> out;
  std::error_code ec;
  for (const std::string& sub : subs) {
    const fs::path base = sub.empty() ? root : root / sub;
    if (!fs::exists(base, ec)) continue;
    if (fs::is_regular_file(base, ec)) {
      out.push_back(base);
      continue;
    }
    for (fs::recursive_directory_iterator it(base, fs::directory_options::skip_permission_denied, ec), end; it != end; it.increment(ec)) {
      if (ec) break;
      const std::string r = rel(root, it->path());
      if (it->is_directory(ec)) {
        if (skipped_dir(r + "/")) it.disable_recursion_pending();
        continue;
      }
      if (it->is_regular_file(ec) && !skipped_dir(r)) out.push_back(it->path());
    }
  }
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

std::string extension_of(const fs::path& p) { return p.extension().string(); }

bool is_c_like(const fs::path& p) {
  static const std::set<std::string> e = {".h", ".hpp", ".hh", ".c", ".cc", ".cpp", ".cxx"};
  return e.count(extension_of(p)) != 0;
}

bool is_cmake(const fs::path& p) { return p.filename() == "CMakeLists.txt" || extension_of(p) == ".cmake"; }
bool is_profile(const fs::path& p) { return extension_of(p) == ".kzp"; }

bool word_char(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_'; }

std::string lower(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

// ---- lexers: identifiers outside comments and string literals --------------------------------

struct Token {
  std::string text;
  int line;
};

enum class Lang { CLike, CMake, Profile };

// True if the quote at s[i] is a C++14 digit separator inside a numeric literal (1'000,
// 0xFF'FF), not the start of a character literal (L'a', 'a', f('a')).
bool digit_separator(const std::string& s, size_t i) {
  if (i == 0 || i + 1 >= s.size() || std::isalnum(static_cast<unsigned char>(s[i + 1])) == 0) return false;
  size_t j = i;
  while (j > 0 && (std::isalnum(static_cast<unsigned char>(s[j - 1])) != 0 || s[j - 1] == '_' || s[j - 1] == '.' || s[j - 1] == '\'')) --j;
  return j < i && std::isdigit(static_cast<unsigned char>(s[j])) != 0;
}

std::vector<Token> identifiers(const std::string& s, Lang lang) {
  std::vector<Token> out;
  int line = 1;
  size_t i = 0;
  auto ident_start = [](char c) { return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_'; };
  auto ident_cont = [&](char c) { return word_char(c) || (lang == Lang::Profile && (c == '-' || c == '.')); };
  while (i < s.size()) {
    char c = s[i];
    if (c == '\n') {
      ++line;
      ++i;
      continue;
    }
    if (lang == Lang::CLike && c == '/' && i + 1 < s.size() && s[i + 1] == '/') {  // line comment
      while (i < s.size() && s[i] != '\n') ++i;
      continue;
    }
    if (lang == Lang::CLike && c == '/' && i + 1 < s.size() && s[i + 1] == '*') {  // block comment
      i += 2;
      while (i + 1 < s.size() && !(s[i] == '*' && s[i + 1] == '/')) {
        if (s[i] == '\n') ++line;
        ++i;
      }
      i += 2;
      continue;
    }
    if (lang != Lang::CLike && c == '#') {  // CMake and profile comments
      while (i < s.size() && s[i] != '\n') ++i;
      continue;
    }
    if (lang == Lang::CLike && c == '\'' && digit_separator(s, i)) {
      ++i;
      continue;
    }
    if (c == '"' || (lang == Lang::CLike && c == '\'')) {  // string or character literal
      const char quote = c;
      ++i;
      while (i < s.size() && s[i] != quote) {
        if (s[i] == '\\' && lang == Lang::CLike) ++i;
        if (i < s.size() && s[i] == '\n') ++line;
        ++i;
      }
      ++i;
      continue;
    }
    if (ident_start(c)) {
      size_t j = i;
      while (j < s.size() && ident_cont(s[j])) ++j;
      std::string t = s.substr(i, j - i);
      // a trailing '.' or '-' of a profile token is punctuation, not part of the name
      while (lang == Lang::Profile && !t.empty() && (t.back() == '.' || t.back() == '-')) t.pop_back();
      out.push_back(Token{t, line});
      i = j;
      continue;
    }
    ++i;
  }
  return out;
}

// ---- data files ------------------------------------------------------------------------------

std::vector<std::string> read_lines(const fs::path& p, bool* ok) {
  std::vector<std::string> v;
  std::string text;
  *ok = read_file(p, &text);
  std::istringstream in(text);
  std::string line;
  while (std::getline(in, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    if (line.empty() || line[0] == '#') continue;
    v.push_back(line);
  }
  return v;
}

// Splits a run of [A-Za-z0-9_] into lower-case name components.  '_' separates components.  A
// boundary also lies between a lower-case letter and an upper-case one (getApp), between the
// last capital of an acronym and the capital of the next word (HTTPServer), and between a digit
// and a capitalised word (R2Cert).  There is none between a letter and a digit (r2, sha256) or
// before a capital that is followed by a digit or by nothing (R2D2, cFS), so R2D2 and C10 are
// single names while check_R2, r2_x and R2Cert contain the component r2.  A piece that starts
// with a digit is a number (2663, 0xC1) and is not cut.
std::vector<std::string> components(const std::string& run) {
  std::vector<std::string> out;
  auto upper = [](char c) { return std::isupper(static_cast<unsigned char>(c)) != 0; };
  auto lowerc = [](char c) { return std::islower(static_cast<unsigned char>(c)) != 0; };
  auto digit = [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; };
  size_t start = 0;
  while (start <= run.size()) {
    size_t end = run.find('_', start);
    if (end == std::string::npos) end = run.size();
    const std::string seg = run.substr(start, end - start);
    start = end + 1;
    if (seg.empty()) continue;
    if (digit(seg[0])) {  // a number (2663, 0xC1, 1e5): one component, never cut
      out.push_back(lower(seg));
      continue;
    }
    std::string cur;
    for (size_t i = 0; i < seg.size(); ++i) {
      if (!cur.empty()) {
        const char p = seg[i - 1];
        const char c = seg[i];
        const char n = i + 1 < seg.size() ? seg[i + 1] : '\0';
        if ((lowerc(p) && upper(c)) || (digit(p) && upper(c) && lowerc(n)) || (upper(p) && upper(c) && lowerc(n))) {
          out.push_back(cur);
          cur.clear();
        }
      }
      cur.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(seg[i]))));
    }
    if (!cur.empty()) out.push_back(cur);
  }
  return out;
}

struct CasePattern {
  bool is_regex;
  std::string text;
  std::vector<std::string> phrase;  // word entries: the components of the phrase
  std::regex re;
  size_t index;
};

bool load_case_patterns(const fs::path& data, std::vector<CasePattern>* out) {
  bool ok = false;
  std::vector<std::string> lines = read_lines(data / "case_identifiers.txt", &ok);
  if (!ok) return false;
  size_t n = 0;
  for (const std::string& l : lines) {
    ++n;
    CasePattern p;
    p.index = n;
    if (l.rfind("word ", 0) == 0) {
      p.is_regex = false;
      p.text = l.substr(5);
      p.phrase = components(p.text);
      if (p.phrase.empty()) {
        std::fprintf(stderr, "case_identifiers.txt: empty word entry: %s\n", l.c_str());
        return false;
      }
    } else if (l.rfind("regex ", 0) == 0) {
      p.is_regex = true;
      p.text = l.substr(6);
      p.re = std::regex(p.text, std::regex::ECMAScript);
    } else {
      std::fprintf(stderr, "case_identifiers.txt: bad line: %s\n", l.c_str());
      return false;
    }
    out->push_back(std::move(p));
  }
  return true;
}

// Does `phrase` occur as consecutive components of one run of the line?
bool has_phrase(const std::string& line, const std::vector<std::string>& phrase) {
  size_t i = 0;
  while (i < line.size()) {
    if (!word_char(line[i])) {
      ++i;
      continue;
    }
    size_t j = i;
    while (j < line.size() && word_char(line[j])) ++j;
    const std::vector<std::string> parts = components(line.substr(i, j - i));
    if (parts.size() >= phrase.size())
      for (size_t k = 0; k + phrase.size() <= parts.size(); ++k)
        if (std::equal(phrase.begin(), phrase.end(), parts.begin() + static_cast<std::ptrdiff_t>(k))) return true;
    i = j;
  }
  return false;
}

// ---- rules -----------------------------------------------------------------------------------

Violations rule_case_ids(const fs::path& root, const fs::path& data, bool* usage_ok) {
  Violations v;
  std::vector<CasePattern> patterns;
  if (!load_case_patterns(data, &patterns)) {
    *usage_ok = false;
    return v;
  }
  for (const fs::path& f : files_under(root, {"include", "lib", "tools"})) {
    std::string text;
    if (!read_file(f, &text) || looks_binary(text)) continue;
    const std::string r = rel(root, f);
    std::istringstream in(text);
    std::string line;
    int no = 0;
    // The path is checked as one line so that a file name cannot carry a case identifier either.
    std::vector<std::pair<int, std::string>> lines = {{0, r}};
    while (std::getline(in, line)) lines.emplace_back(++no, line);
    for (const auto& entry : lines) {
      for (const CasePattern& p : patterns) {
        bool hit = false;
        std::string what = p.text;
        if (p.is_regex) {
          std::smatch m;
          if (std::regex_search(entry.second, m, p.re)) {
            hit = true;
            what = m.str();
          }
        } else {
          hit = has_phrase(entry.second, p.phrase);
        }
        if (hit)
          v.push_back(Violation{r, entry.first, "case identifier '" + what + "' (case_identifiers.txt entry " + std::to_string(p.index) +
                                                    "): cFS and case knowledge belongs in profiles/, not in the engine"});
      }
    }
  }
  return v;
}

Violations rule_retired_word(const fs::path& root, const fs::path& data, bool* usage_ok) {
  Violations v;
  bool ok = false;
  std::set<std::string> allow;
  for (const std::string& l : read_lines(data / "contract_allow.txt", &ok)) allow.insert(l);
  if (!ok) {
    *usage_ok = false;
    return v;
  }
  const std::vector<std::string> subs = {"include", "lib", "tools", "probe", "gate", "tests", "profiles", "cmake", "CMakeLists.txt"};
  for (const fs::path& f : files_under(root, subs)) {
    Lang lang;
    if (is_c_like(f))
      lang = Lang::CLike;
    else if (is_cmake(f))
      lang = Lang::CMake;
    else if (is_profile(f))
      lang = Lang::Profile;
    else
      continue;
    std::string text;
    if (!read_file(f, &text)) continue;
    for (const Token& t : identifiers(text, lang)) {
      if (lower(t.text).find("contract") == std::string::npos || allow.count(t.text) != 0) continue;
      v.push_back(Violation{rel(root, f), t.line,
                            "identifier '" + t.text + "' contains \"contract\": the judged object is a readiness specification (use rspec / rprecondition)"});
    }
  }
  return v;
}

Violations rule_banned_files(const fs::path& root) {
  Violations v;
  static const std::set<std::string> banned = {".py", ".pyc", ".pyo", ".pyi", ".pyw", ".ipynb", ".json", ".jsonl"};
  for (const fs::path& f : files_under(root, {""})) {
    const std::string r = rel(root, f);
    if (banned.count(lower(extension_of(f))) != 0) {
      v.push_back(Violation{r, 0, "Python and JSON files are not allowed in the tool core"});
      continue;
    }
    std::string text;
    if (!read_file(f, &text) || text.rfind("#!", 0) != 0) continue;
    std::string first = text.substr(0, text.find('\n'));
    if (lower(first).find("python") != std::string::npos) v.push_back(Violation{r, 1, "a Python script (shebang) is not allowed in the tool core"});
  }
  return v;
}

// CMake text without comments (line comments and bracket comments); quoted arguments keep
// their text, and the line structure is kept so that line numbers stay right.
std::string strip_cmake_comments(const std::string& s) {
  std::string out;
  size_t i = 0;
  bool quoted = false;
  while (i < s.size()) {
    const char c = s[i];
    if (quoted) {
      out.push_back(c);
      if (c == '\\' && i + 1 < s.size()) out.push_back(s[++i]);
      else if (c == '"') quoted = false;
      ++i;
      continue;
    }
    if (c == '"') {
      quoted = true;
      out.push_back(c);
      ++i;
      continue;
    }
    if (c == '#') {
      size_t j = i + 1;
      if (j < s.size() && s[j] == '[') {  // bracket comment #[==[ ... ]==]
        size_t eq = j + 1;
        while (eq < s.size() && s[eq] == '=') ++eq;
        if (eq < s.size() && s[eq] == '[') {
          const std::string close = "]" + std::string(eq - j - 1, '=') + "]";
          const size_t end = s.find(close, eq + 1);
          const size_t stop = end == std::string::npos ? s.size() : end + close.size();
          for (size_t k = i; k < stop; ++k)
            if (s[k] == '\n') out.push_back('\n');
          i = stop;
          continue;
        }
      }
      while (i < s.size() && s[i] != '\n') ++i;
      continue;
    }
    out.push_back(c);
    ++i;
  }
  return out;
}

bool contains_dir(const std::string& relative, const std::string& dir) { return relative.rfind(dir + "/", 0) == 0; }

Violations dedupe(Violations v) {
  std::sort(v.begin(), v.end(), [](const Violation& a, const Violation& b) {
    if (a.path != b.path) return a.path < b.path;
    return a.line < b.line;
  });
  v.erase(std::unique(v.begin(), v.end(), [](const Violation& a, const Violation& b) { return a.path == b.path && a.line == b.line; }), v.end());
  return v;
}

Violations rule_no_llvm(const fs::path& root) {
  Violations v;
  static const std::regex include_re("^\\s*#\\s*include\\s*[<\"](llvm|llvm-c|clang|clang-c|mlir)/");
  // Names and paths of LLVM, Clang and MLIR in build files, in any case; the compiler driver
  // `clang` (find_program, a command) and the compiler id "Clang" are not dependencies.
  static const std::vector<std::regex> cmake_re = {
      std::regex("llvm", std::regex::icase),
      std::regex("mlir", std::regex::icase),
      std::regex("find_package\\s*\\(\\s*clang", std::regex::icase),
      std::regex("\\bClang::"),
      std::regex("\\bCLANG_[A-Z_]*(DIR|DIRS|DEFINITIONS|LIBS|LIBRARIES)\\b"),
      std::regex("\\bclang[A-Z][A-Za-z0-9_]*"),
      std::regex("libclang"),
      std::regex("clang-c"),
      std::regex("\\bclang/"),
  };
  const std::vector<std::string> subs = {"lib/core", "lib/proofcheck", "tools/kz", "tools/kz-dump", "include", "tests/unit", "tests/golden"};
  for (const fs::path& f : files_under(root, subs)) {
    std::string text;
    if (!read_file(f, &text)) continue;
    const std::string r = rel(root, f);
    if (is_c_like(f)) {
      std::istringstream in(text);
      std::string line;
      int no = 0;
      while (std::getline(in, line)) {
        ++no;
        if (std::regex_search(line, include_re)) v.push_back(Violation{r, no, "LLVM/Clang headers are for the analysis tools only"});
      }
    } else if (is_cmake(f)) {
      std::istringstream in(strip_cmake_comments(text));
      std::string line;
      int no = 0;
      while (std::getline(in, line)) {
        ++no;
        for (const std::regex& re : cmake_re) {
          std::smatch m;
          if (std::regex_search(line, m, re)) {
            v.push_back(Violation{r, no, "'" + m.str() + "': LLVM is not a dependency of libkz-core, tools/kz or lib/proofcheck"});
            break;
          }
        }
      }
    }
  }
  // kz_use_llvm is the only way LLVM reaches a target, and it is used only by the analysis tools.
  for (const fs::path& f : files_under(root, {""})) {
    if (!is_cmake(f)) continue;
    const std::string r = rel(root, f);
    if (contains_dir(r, "lib/llvm") || contains_dir(r, "tools/kz-static") || contains_dir(r, "instrument/clang") || r == "cmake/KzSettings.cmake") continue;
    std::string text;
    if (!read_file(f, &text)) continue;
    for (const Token& t : identifiers(text, Lang::CMake))
      if (t.text == "kz_use_llvm") v.push_back(Violation{r, t.line, "kz_use_llvm is allowed only in lib/llvm, tools/kz-static and instrument/clang"});
  }
  return dedupe(v);
}

Violations rule_no_generated(const fs::path& root) {
  Violations v;
  static const std::vector<std::string> markers = {"@generated", "auto-generated", "autogenerated", "automatically generated", "do not edit",
                                                   "generated by", "code generator"};
  // Files that hold or produce source text: C, C++, includes, X-macro tables, TableGen, assembly.
  static const std::string src_ext = "(c|cc|cpp|cxx|h|hh|hpp|hxx|ipp|tcc|inc|def|td|s|asm)";
  static const std::regex custom_command("add_custom_command\\s*\\(([^)]*)\\)", std::regex::icase);
  static const std::regex source_name("\\." + src_ext + "$", std::regex::icase);
  static const std::regex file_write("\\bfile\\s*\\(\\s*(WRITE|APPEND)\\s+(\"[^\"]*\"|[^\\s)]+)", std::regex::icase);
  static const std::regex file_configure("\\bfile\\s*\\(\\s*CONFIGURE\\s+OUTPUT\\s+(\"[^\"]*\"|[^\\s)]+)", std::regex::icase);
  static const std::regex process_output("\\bOUTPUT_FILE\\s+(\"[^\"]*\"|[^\\s)]+)", std::regex::icase);
  // The OUTPUT and BYPRODUCTS lists of an add_custom_command, up to the next keyword: a source
  // file named in COMMAND or DEPENDS (an input) is not an output.
  auto produces_source = [&](const std::string& args) {
    static const std::set<std::string> keywords = {"COMMAND", "ARGS", "MAIN_DEPENDENCY", "DEPENDS", "IMPLICIT_DEPENDS", "WORKING_DIRECTORY", "COMMENT",
                                                   "DEPFILE", "JOB_POOL", "VERBATIM", "APPEND", "USES_TERMINAL", "COMMAND_EXPAND_LISTS", "TARGET", "POST_BUILD",
                                                   "PRE_BUILD", "PRE_LINK", "OUTPUT", "BYPRODUCTS"};
    std::istringstream words(args);
    std::string w;
    bool outputs = false;
    while (words >> w) {
      std::string key = w;
      for (char& c : key) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      if (keywords.count(key) != 0) {
        outputs = key == "OUTPUT" || key == "BYPRODUCTS";
        continue;
      }
      std::string a = w;
      while (!a.empty() && a.back() == '"') a.pop_back();
      if (outputs && std::regex_search(a, source_name)) return true;
    }
    return false;
  };
  auto unquote = [](std::string a) {
    while (!a.empty() && (a.back() == ')' || a.back() == '"')) a.pop_back();
    if (!a.empty() && a.front() == '"') a.erase(a.begin());
    return a;
  };
  // The text of an argument that writes a file: flagged if it names a source file, or, outside
  // tests/, if it is built from a variable and so cannot be checked.
  auto writes_source = [&](const std::string& arg, bool in_tests) {
    return std::regex_search(arg, source_name) || (!in_tests && arg.find("${") != std::string::npos);
  };
  for (const fs::path& f : files_under(root, {""})) {
    const std::string r = rel(root, f);
    const std::string name = f.filename().string();
    // Templates that produce source files.
    static const std::regex template_re("\\." + src_ext + "\\.in$", std::regex::icase);
    if (std::regex_search(name, template_re)) v.push_back(Violation{r, 0, "a source template (.in) means generated code"});
    if (!(is_c_like(f) || is_cmake(f) || extension_of(f) == ".sh")) continue;
    if (r.rfind("tests/lint/", 0) == 0) continue;  // the lint names the markers it looks for
    std::string text;
    if (!read_file(f, &text) || looks_binary(text)) continue;
    std::istringstream in(text);
    std::string line;
    int no = 0;
    while (std::getline(in, line)) {
      ++no;
      const std::string l = lower(line);
      for (const std::string& m : markers)
        if (l.find(m) != std::string::npos) v.push_back(Violation{r, no, "marker \"" + m + "\": no generated code and no code that generates code"});
    }
    if (is_cmake(f)) {
      for (const Token& t : identifiers(text, Lang::CMake))
        if (lower(t.text) == "configure_file") v.push_back(Violation{r, t.line, "configure_file generates a file at configure time"});
      const std::string stripped = strip_cmake_comments(text);
      const bool in_tests = contains_dir(r, "tests");
      if (std::regex_search(stripped, std::regex("file\\s*\\(\\s*GENERATE", std::regex::icase)))
        v.push_back(Violation{r, 0, "file(GENERATE ...) generates a file at generate time"});
      for (std::sregex_iterator it(stripped.begin(), stripped.end(), custom_command), end; it != end; ++it)
        if (produces_source((*it)[1].str())) v.push_back(Violation{r, 0, "add_custom_command produces a source file"});
      for (std::sregex_iterator it(stripped.begin(), stripped.end(), file_write), end; it != end; ++it)
        if (writes_source(unquote((*it)[2].str()), in_tests)) v.push_back(Violation{r, 0, "file(" + (*it)[1].str() + ") writes a source file or a file whose name is not fixed"});
      for (std::sregex_iterator it(stripped.begin(), stripped.end(), file_configure), end; it != end; ++it)
        if (writes_source(unquote((*it)[1].str()), in_tests)) v.push_back(Violation{r, 0, "file(CONFIGURE) writes a source file or a file whose name is not fixed"});
      for (std::sregex_iterator it(stripped.begin(), stripped.end(), process_output), end; it != end; ++it)
        if (writes_source(unquote((*it)[1].str()), in_tests)) v.push_back(Violation{r, 0, "execute_process writes a source file or a file whose name is not fixed"});
    }
  }
  return v;
}

// ---- running ---------------------------------------------------------------------------------

int report(const char* rule, const Violations& v) {
  for (const Violation& x : v) std::fprintf(stderr, "%s:%d: [%s] %s\n", x.path.c_str(), x.line, rule, x.message.c_str());
  if (v.empty()) std::printf("lint %s: clean\n", rule);
  else std::printf("lint %s: %zu violation(s)\n", rule, v.size());
  return v.empty() ? 0 : 1;
}

Violations run_rule(const std::string& rule, const fs::path& root, const fs::path& data, bool* known, bool* usage_ok) {
  *known = true;
  if (rule == "case-ids") return rule_case_ids(root, data, usage_ok);
  if (rule == "retired-term") return rule_retired_word(root, data, usage_ok);
  if (rule == "banned-files") return rule_banned_files(root);
  if (rule == "no-llvm") return rule_no_llvm(root);
  if (rule == "no-generated") return rule_no_generated(root);
  *known = false;
  return {};
}

// ---- selftest --------------------------------------------------------------------------------

struct Tree {
  std::string name;
  std::vector<std::pair<std::string, std::string>> files;  // relative path, content
};

struct Expectation {
  const char* rule;
  bool must_flag;
  Tree tree;
};

int selftest(const fs::path& data) {
  const std::vector<Expectation> cases = {
      // ---- case identifiers: whole words only, comments and paths included, profiles and tests exempt
      {"case-ids", true, {"case id R2 in code", {{"lib/a.cpp", "int x; // R2 certificate\n"}}}},
      {"case-ids", true, {"case id in a header string", {{"include/kz/a.h", "const char* s = \"C3\";\n"}}}},
      {"case-ids", true, {"issue number", {{"lib/a.cpp", "// see #2663\n"}}}},
      {"case-ids", true, {"issue number as a word", {{"lib/a.cpp", "int n = 2663;\n"}}}},
      {"case-ids", true, {"framework symbol", {{"lib/a.cpp", "void f() { CFE_ES_Main(); }\n"}}}},
      {"case-ids", true, {"case symbol", {{"include/a.h", "extern int CFE_SB_Global;\n"}}}},
      {"case-ids", true, {"case symbol in a tool", {{"tools/kz/cmd_a.cpp", "// the threads that cFS creates\n"}}}},
      {"case-ids", true, {"reference commit", {{"lib/a.cpp", "// fixed in 833fdbb\n"}}}},
      {"case-ids", true, {"case id in a file name", {{"lib/C1.cpp", "int x;\n"}}}},
      // a case id inside a longer name, in any case, and the framework's own names
      {"case-ids", true, {"case id as an underscore component", {{"lib/a.cpp", "void check_R2() {}\n"}}}},
      {"case-ids", true, {"lower-case case id component", {{"lib/a.cpp", "void r2_fixed_order() {}\n"}}}},
      {"case-ids", true, {"case id before a capital", {{"include/a.h", "struct R2Cert {};\n"}}}},
      {"case-ids", true, {"case id after a lower-case word", {{"lib/a.cpp", "int kzC1Rule;\n"}}}},
      {"case-ids", true, {"case id in the middle of a name", {{"lib/a.cpp", "int x_c3_y;\n"}}}},
      {"case-ids", true, {"case id in a macro name", {{"include/a.h", "#define KZ_R2_LIMIT 3\n"}}}},
      {"case-ids", true, {"case id in a file name with an underscore", {{"lib/r2_helpers.cpp", "int x;\n"}}}},
      {"case-ids", true, {"framework component EVS", {{"lib/a.cpp", "int EVS_APP_ID = 3;\n"}}}},
      {"case-ids", true, {"framework component cfe in a source name", {{"lib/a.cpp", "const char* f = \"cfe_sb_task.c:138\";\n"}}}},
      {"case-ids", true, {"framework component osal", {{"include/a.h", "int osal_state;\n"}}}},
      {"case-ids", true, {"framework component PSP", {{"lib/a.cpp", "// the PSP restarts\n"}}}},
      {"case-ids", true, {"framework name cFS", {{"lib/a.cpp", "int cFS;\n"}}}},
      {"case-ids", true, {"framework name CFS", {{"lib/a.cpp", "int CFS_ROOT;\n"}}}},
      {"case-ids", true, {"function name of the bundle", {{"lib/a.cpp", "void MainTaskSyncDelay();\n"}}}},
      {"case-ids", true, {"function name GetAppID", {{"lib/a.cpp", "int GetAppID();\n"}}}},
      {"case-ids", true, {"field name AppId", {{"lib/a.cpp", "int AppId;\n"}}}},
      {"case-ids", true, {"field name app_id", {{"lib/a.cpp", "int app_id;\n"}}}},
      {"case-ids", false, {"similar but different words", {{"lib/a.cpp", "int CRC32 = 0xC1; int R2D2; int sha256; int KZ_OS_X; int C10; int usr2; int SIGUSR2;\n"}}}},
      {"case-ids", false, {"words that only contain a case id as letters", {{"lib/a.cpp", "int pace1 = 1; int arc2 = 2; int revs = 3; int appendix = 4; int cfei;\n"}}}},
      {"case-ids", false, {"profiles and tests are allowed", {{"profiles/p.kzp", "R2 C1 CFE_SB_Global\n"}, {"tests/t.cpp", "// C3 R2\n"}}}},
      {"case-ids", false, {"clean engine code", {{"lib/a.cpp", "int main() { return 0; }\n"}, {"include/a.h", "#pragma once\n"}}}},
      // ---- the word "contract" in identifiers
      {"retired-term", true, {"identifier in C++", {{"lib/a.hpp", "struct kz_contract {};\n"}}}},
      {"retired-term", true, {"macro name", {{"include/a.h", "#define KZ_CONTRACT_MAX 4\n"}}}},
      {"retired-term", true, {"capitalised", {{"lib/a.cpp", "int Contract;\n"}}}},
      {"retired-term", true, {"profile keyword", {{"profiles/p.kzp", "contract K-ID { }\n"}}}},
      {"retired-term", true, {"cmake target", {{"CMakeLists.txt", "add_library(kz-contracts a.cpp)\n"}}}},
      {"retired-term", true, {"after a C++14 digit separator", {{"lib/a.cpp", "int n = 1'000'000; struct kz_contract {};\n"}}}},
      {"retired-term", true, {"after a hex literal with a separator", {{"lib/a.cpp", "unsigned m = 0xFF'FF; int Contract;\n"}}}},
      {"retired-term", false, {"a character literal is not a digit separator", {{"lib/a.cpp", "char c = 'a'; wchar_t w = L'b'; /* contract */ int rspec;\n"}}}},
      {"retired-term", false,
       {"comments and strings are not identifiers",
        {{"lib/a.cpp", "// API contract (documented semantics)\n/* contract */ const char* t = \"contract-backed\"; int rspec;\n"}}}},
      {"retired-term", false, {"the provenance tier name in a profile", {{"profiles/p.kzp", "tier contract-backed;  # a comment about a contract\n"}}}},
      {"retired-term", false, {"markdown is not scanned", {{"README.md", "contract contract\n"}}}},
      // ---- Python and JSON
      {"banned-files", true, {"python file", {{"lib/tool.py", "print(1)\n"}}}},
      {"banned-files", true, {"json file", {{"profiles/data.json", "{}\n"}}}},
      {"banned-files", true, {"jupyter notebook", {{"x.ipynb", "{}\n"}}}},
      {"banned-files", true, {"python shebang", {{"tools/run", "#!/usr/bin/env python3\nprint(1)\n"}}}},
      {"banned-files", false, {"shell and C++ are fine", {{"tools/run.sh", "#!/bin/sh\necho ok\n"}, {"lib/a.cpp", "int x;\n"}}}},
      // ---- LLVM in the core
      {"no-llvm", true, {"LLVM header in libkz-core", {{"lib/core/a.cpp", "#include <llvm/Support/Casting.h>\n"}}}},
      {"no-llvm", true, {"Clang header in a public header", {{"include/kz/a.h", "#include \"clang/AST/AST.h\"\n"}}}},
      {"no-llvm", true, {"find_package(LLVM) in the core", {{"lib/core/CMakeLists.txt", "find_package(LLVM REQUIRED)\n"}}}},
      {"no-llvm", true, {"kz_use_llvm on a core target", {{"tools/kz/CMakeLists.txt", "kz_use_llvm(kz)\n"}}}},
      {"no-llvm", true, {"clang library in a core test", {{"tests/unit/CMakeLists.txt", "target_link_libraries(t PRIVATE clangTooling)\n"}}}},
      {"no-llvm", true, {"LLVM C API header", {{"lib/core/a.cpp", "#include <llvm-c/Core.h>\n"}}}},
      {"no-llvm", true, {"Clang C API header", {{"tools/kz/a.cpp", "#include \"clang-c/Index.h\"\n"}}}},
      {"no-llvm", true, {"MLIR header", {{"lib/proofcheck/a.cpp", "#  include <mlir/IR/Operation.h>\n"}}}},
      {"no-llvm", true, {"LLVM include path, unquoted", {{"lib/core/CMakeLists.txt", "target_include_directories(kz-core PRIVATE /home/user/work/llvm-project/build/include)\n"}}}},
      {"no-llvm", true, {"LLVM include path, quoted", {{"lib/core/CMakeLists.txt", "target_include_directories(kz-core PRIVATE \"/home/user/work/llvm-project/build/include\")\n"}}}},
      {"no-llvm", true, {"lower-case llvm command", {{"lib/core/CMakeLists.txt", "llvm_map_components_to_libnames(L support)\n"}}}},
      {"no-llvm", true, {"mixed-case LLVM variable", {{"tools/kz/CMakeLists.txt", "target_link_libraries(kz PRIVATE ${Llvm_LIBS})\n"}}}},
      {"no-llvm", true, {"lower-case mlir", {{"tools/kz-dump/CMakeLists.txt", "find_package(mlir REQUIRED)\n"}}}},
      {"no-llvm", true, {"find_package(Clang)", {{"lib/proofcheck/CMakeLists.txt", "find_package(Clang REQUIRED CONFIG)\n"}}}},
      {"no-llvm", true, {"Clang target namespace", {{"tools/kz/CMakeLists.txt", "target_link_libraries(kz PRIVATE Clang::Tooling)\n"}}}},
      {"no-llvm", true, {"after a bracket comment", {{"lib/core/CMakeLists.txt", "#[[ comment ]]\nfind_package(LLVM)\n"}}}},
      {"no-llvm", true, {"kz_use_llvm in another component", {{"lib/static/CMakeLists.txt", "kz_use_llvm(kz-static-lib)\n"}}}},
      {"no-llvm", true, {"kz_use_llvm in a probe", {{"probe/x/CMakeLists.txt", "kz_use_llvm(probe)\n"}}}},
      {"no-llvm", false, {"LLVM in the analysis tools is fine", {{"lib/llvm/a.cpp", "#include <llvm/Support/Casting.h>\n"}, {"tools/kz-static/CMakeLists.txt", "kz_use_llvm(kz-static)\n"}, {"lib/llvm/CMakeLists.txt", "kz_use_llvm(kz-llvm)\n"}}}},
      {"no-llvm", false, {"the definition of kz_use_llvm", {{"cmake/KzSettings.cmake", "function(kz_use_llvm target)\nendfunction()\n"}}}},
      {"no-llvm", false, {"the Clang source-transform connector", {{"instrument/clang/CMakeLists.txt", "kz_use_llvm(kz-instr-clang)\n"}}}},
      {"no-llvm", false, {"the compiler id and a comment", {{"lib/core/CMakeLists.txt", "if(CMAKE_CXX_COMPILER_ID STREQUAL \"Clang\")\nendif()\n# no LLVM, no clangTooling, no mlir\n"}}}},
      {"no-llvm", false, {"a bpf compiler variable", {{"tests/unit/CMakeLists.txt", "find_program(KZ_BPF_CLANG clang)\nadd_test(NAME t COMMAND ${KZ_BPF_CLANG} -target bpf -c x.c)\n"}}}},
      {"no-llvm", false, {"a clang compiler is not a dependency", {{"tests/unit/CMakeLists.txt", "find_program(KZ_BPF_CLANG clang)\n# LLVM is not used here\n"}}}},
      // ---- generated code
      {"no-generated", true, {"generated marker", {{"lib/a.cpp", "// auto-generated by a script\nint x;\n"}}}},
      {"no-generated", true, {"do not edit", {{"include/a.h", "/* DO NOT EDIT */\n"}}}},
      {"no-generated", true, {"configure_file", {{"CMakeLists.txt", "configure_file(a.h.in a.h)\n"}}}},
      {"no-generated", true, {"file(GENERATE)", {{"CMakeLists.txt", "file(GENERATE OUTPUT a.h CONTENT x)\n"}}}},
      {"no-generated", true, {"custom command producing a source", {{"CMakeLists.txt", "add_custom_command(OUTPUT gen.cpp COMMAND tool)\n"}}}},
      {"no-generated", true, {"source template", {{"lib/a.h.in", "x\n"}}}},
      {"no-generated", true, {"custom command producing an include file", {{"CMakeLists.txt", "add_custom_command(OUTPUT x.inc COMMAND gen)\n"}}}},
      {"no-generated", true, {"custom command producing a def file", {{"lib/CMakeLists.txt", "add_custom_command(\n  OUTPUT tables.def\n  COMMAND gen > tables.def)\n"}}}},
      {"no-generated", true, {"custom command producing a TableGen file", {{"CMakeLists.txt", "add_custom_command(OUTPUT a.td COMMAND gen)\n"}}}},
      {"no-generated", true, {"custom command with a source byproduct", {{"CMakeLists.txt", "add_custom_command(OUTPUT stamp BYPRODUCTS gen.hpp COMMAND gen)\n"}}}},
      {"no-generated", true, {"file(WRITE) of a source file", {{"CMakeLists.txt", "file(WRITE gen.cpp \"int x;\")\n"}}}},
      {"no-generated", true, {"file(WRITE) of a quoted source file", {{"lib/CMakeLists.txt", "file(WRITE \"${CMAKE_CURRENT_BINARY_DIR}/gen.h\" \"int x;\")\n"}}}},
      {"no-generated", true, {"file(APPEND) of a source file", {{"CMakeLists.txt", "file(APPEND gen.c \"int y;\")\n"}}}},
      {"no-generated", true, {"file(CONFIGURE) of a source file", {{"CMakeLists.txt", "file(CONFIGURE OUTPUT gen.hpp CONTENT \"x\")\n"}}}},
      {"no-generated", true, {"file(WRITE) to a variable outside tests", {{"lib/CMakeLists.txt", "file(WRITE ${out} \"int x;\")\n"}}}},
      {"no-generated", true, {"execute_process writing a source file", {{"CMakeLists.txt", "execute_process(COMMAND gen OUTPUT_FILE gen.cpp)\n"}}}},
      {"no-generated", true, {"file(WRITE) of a source file in tests", {{"tests/CMakeLists.txt", "file(WRITE gen.cpp \"int x;\")\n"}}}},
      {"no-generated", false, {"a custom command producing an object", {{"CMakeLists.txt", "add_custom_command(OUTPUT probe.o COMMAND cc)\n"}}}},
      {"no-generated", false, {"a source file as an input of a custom command", {{"CMakeLists.txt", "add_custom_command(OUTPUT probe.o COMMAND cc -c probe.c -o probe.o DEPENDS probe.c)\n"}}}},
      {"no-generated", false, {"file(WRITE) of a data file in tests", {{"tests/CMakeLists.txt", "file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/input.txt \"x\")\n"}}}},
      {"no-generated", false, {"a commented generator", {{"CMakeLists.txt", "# file(WRITE gen.cpp \"x\")\n# add_custom_command(OUTPUT gen.inc COMMAND gen)\n"}}}},
      {"no-generated", false, {"clean", {{"lib/a.cpp", "// generated tables would be a violation; this is not one\nint x;\n"}}}},
  };

  const fs::path scratch = fs::current_path() / ("kz_lint_selftest_" + std::to_string(static_cast<long>(getpid())));
  int failures = 0;
  size_t index = 0;
  for (const Expectation& e : cases) {
    const fs::path root = scratch / std::to_string(index++);
    std::error_code ec;
    fs::remove_all(root, ec);
    for (const auto& file : e.tree.files) {
      fs::create_directories((root / file.first).parent_path(), ec);
      std::ofstream out(root / file.first, std::ios::binary);
      out << file.second;
    }
    bool known = false, usage_ok = true;
    Violations v = run_rule(e.rule, root, data, &known, &usage_ok);
    const bool flagged = !v.empty();
    if (!known || !usage_ok || flagged != e.must_flag) {
      ++failures;
      std::fprintf(stderr, "selftest FAILED: rule %s, tree \"%s\": expected %s, got %s\n", e.rule, e.tree.name.c_str(),
                   e.must_flag ? "a violation" : "no violation", flagged ? "a violation" : "no violation");
      for (const Violation& x : v) std::fprintf(stderr, "    %s:%d: %s\n", x.path.c_str(), x.line, x.message.c_str());
    }
  }
  std::error_code ec;
  fs::remove_all(scratch, ec);
  std::printf("lint selftest: %zu cases, %d failed\n", cases.size(), failures);
  return failures == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  std::vector<std::string> args(argv + 1, argv + argc);
  fs::path data;
  std::vector<std::string> rest;
  for (size_t i = 0; i < args.size(); ++i) {
    if (args[i] == "--data" && i + 1 < args.size()) data = args[++i];
    else rest.push_back(args[i]);
  }
  if (data.empty()) {
    std::fprintf(stderr, "usage: kz_lint <rule> <root> --data <dir> | --selftest --data <dir>\n");
    return 2;
  }
  if (rest.size() == 1 && rest[0] == "--selftest") return selftest(data);
  if (rest.size() != 2) {
    std::fprintf(stderr, "usage: kz_lint <rule> <root> --data <dir> | --selftest --data <dir>\n");
    return 2;
  }
  bool known = false, usage_ok = true;
  Violations v = run_rule(rest[0], rest[1], data, &known, &usage_ok);
  if (!known) {
    std::fprintf(stderr, "unknown rule %s\n", rest[0].c_str());
    return 2;
  }
  if (!usage_ok) {
    std::fprintf(stderr, "cannot read the data files in %s\n", data.string().c_str());
    return 2;
  }
  return report(rest[0].c_str(), v);
}
