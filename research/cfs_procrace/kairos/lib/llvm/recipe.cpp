// recipe.cpp -- see recipe.hpp.
#include "recipe.hpp"

#include <cctype>
#include <cstdint>

namespace kzs {

namespace {

struct Tok {
  enum Kind { End, Word, String, Number, Punct } kind = End;
  std::string text;
  uint32_t line = 0;
};

class Lexer {
 public:
  explicit Lexer(const std::string& s) : s_(s) {}
  Tok next() {
    skip();
    Tok t;
    t.line = line_;
    if (i_ >= s_.size()) return t;
    const char c = s_[i_];
    if (c == '"') {
      ++i_;
      t.kind = Tok::String;
      while (i_ < s_.size() && s_[i_] != '"') {
        if (s_[i_] == '\n') ++line_;
        if (s_[i_] == '\\' && i_ + 1 < s_.size()) ++i_;
        t.text.push_back(s_[i_++]);
      }
      if (i_ < s_.size()) ++i_;
      else t.kind = Tok::End;  // unterminated
      return t;
    }
    if (c == '{' || c == '}' || c == ';') {
      ++i_;
      t.kind = Tok::Punct;
      t.text = std::string(1, c);
      return t;
    }
    t.kind = std::isdigit(static_cast<unsigned char>(c)) ? Tok::Number : Tok::Word;
    while (i_ < s_.size() && !std::isspace(static_cast<unsigned char>(s_[i_])) && s_[i_] != ';' && s_[i_] != '{' && s_[i_] != '}' && s_[i_] != '"') t.text.push_back(s_[i_++]);
    if (t.kind == Tok::Number)
      for (char ch : t.text)
        if (!std::isdigit(static_cast<unsigned char>(ch))) t.kind = Tok::Word;
    return t;
  }

 private:
  void skip() {
    while (i_ < s_.size()) {
      if (s_[i_] == '\n') {
        ++line_;
        ++i_;
      } else if (std::isspace(static_cast<unsigned char>(s_[i_]))) {
        ++i_;
      } else if (s_[i_] == '#') {
        while (i_ < s_.size() && s_[i_] != '\n') ++i_;
      } else {
        break;
      }
    }
  }
  const std::string& s_;
  size_t i_ = 0;
  uint32_t line_ = 1;
};

class Parser {
 public:
  explicit Parser(const std::string& text) : lex_(text) { advance(); }

  kz::Status run(Recipe* r) {
    if (!word("kzb") || cur_.kind != Tok::Number || cur_.text != "1") return fail("a recipe starts with `kzb 1;`");
    advance();
    if (!semi()) return err_;
    while (cur_.kind != Tok::End) {
      if (cur_.kind != Tok::Word) return fail("a statement starts with a keyword");
      const std::string kw = cur_.text;
      advance();
      if (kw == "name") {
        if (!str(&r->name) || !semi()) return err_;
      } else if (kw == "link") {
        if (!str(&r->link) || !semi()) return err_;
      } else if (kw == "binary") {
        if (!str(&r->binary) || !semi()) return err_;
      } else if (kw == "generated") {
        Recipe::Generated g;
        if (!str(&g.role) || !str(&g.path) || !semi()) return err_;
        r->generated.push_back(g);
      } else if (kw == "table") {
        Recipe::Table t;
        if (!str(&t.symbol) || !word("in") || !str(&t.file) || !semi()) return err_;
        r->tables.push_back(t);
      } else if (kw == "declared") {
        Recipe::Declared d;
        if (!str(&d.name) || !str(&d.value) || !word("authored") || !str(&d.authored) || !semi()) return err_;
        r->declared.push_back(d);
      } else if (kw == "startup_script") {
        if (!startup(r)) return err_;
      } else {
        return fail("unknown statement `" + kw + "`");
      }
    }
    if (r->link.empty()) return fail("the recipe has no `link` statement");
    return kz::Status::success();
  }

 private:
  void advance() { cur_ = lex_.next(); }
  kz::Status fail(const std::string& m) {
    err_ = kz::Status::error(kz::Err::Invalid, "recipe line " + std::to_string(cur_.line) + ": " + m);
    return err_;
  }
  bool word(const char* w) {
    if (cur_.kind == Tok::Word && cur_.text == w) {
      advance();
      return true;
    }
    fail(std::string("expected `") + w + "`");
    return false;
  }
  bool ident(std::string* out) {
    if (cur_.kind != Tok::Word) {
      fail("expected a word");
      return false;
    }
    *out = cur_.text;
    advance();
    return true;
  }
  bool str(std::string* out) {
    if (cur_.kind != Tok::String) {
      fail("expected a quoted string");
      return false;
    }
    *out = cur_.text;
    advance();
    return true;
  }
  bool num(uint32_t* out) {
    if (cur_.kind != Tok::Number) {
      fail("expected a number");
      return false;
    }
    *out = static_cast<uint32_t>(std::stoul(cur_.text));
    advance();
    return true;
  }
  bool semi() {
    if (cur_.kind == Tok::Punct && cur_.text == ";") {
      advance();
      return true;
    }
    fail("expected `;`");
    return false;
  }
  bool startup(Recipe* r) {
    if (!(cur_.kind == Tok::Punct && cur_.text == "{")) {
      fail("expected `{`");
      return false;
    }
    advance();
    r->startup.present = true;
    while (!(cur_.kind == Tok::Punct && cur_.text == "}")) {
      if (cur_.kind != Tok::Word) {
        fail("expected a statement or `}`");
        return false;
      }
      const std::string kw = cur_.text;
      advance();
      if (kw == "reset") {
        if (!ident(&r->startup.reset) || !semi()) return false;
      } else if (kw == "rule") {
        Recipe::Startup::Rule rule;
        if (!ident(&rule.reset)) return false;
        while (cur_.kind == Tok::Word) {
          rule.order.push_back(cur_.text);
          advance();
        }
        if (!semi()) return false;
        r->startup.rules.push_back(rule);
      } else if (kw == "file") {
        Recipe::Startup::File f;
        if (!ident(&f.role) || !word("const") || !str(&f.constant) || !word("in") || !str(&f.source) || !semi()) return false;
        r->startup.files.push_back(f);
      } else if (kw == "mount") {
        Recipe::Startup::Mount m;
        if (!str(&m.point) || !str(&m.dir) || !semi()) return false;
        r->startup.mounts.push_back(m);
      } else if (kw == "entry") {
        std::string role, w;
        if (!ident(&role) || !str(&w) || !semi()) return false;
        if (role == "library") r->startup.library_word = w;
        else if (role == "app") r->startup.app_word = w;
        else {
          fail("entry role must be `library` or `app`");
          return false;
        }
      } else if (kw == "cite") {
        Recipe::Startup::Cite c;
        if (!str(&c.file) || !num(&c.from) || !num(&c.to)) return false;
        if (cur_.kind == Tok::Word && cur_.text == "note") {
          advance();
          if (!str(&c.note)) return false;
        }
        if (!semi()) return false;
        r->startup.cites.push_back(c);
      } else {
        fail("unknown statement `" + kw + "` in startup_script");
        return false;
      }
    }
    advance();  // '}'
    return true;
  }

  Lexer lex_;
  Tok cur_;
  kz::Status err_;
};

}  // namespace

kz::Status parse_recipe(const std::string& text, Recipe* out) {
  *out = Recipe();
  Parser p(text);
  return p.run(out);
}

}  // namespace kzs
