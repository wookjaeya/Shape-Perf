#include "phi_lexer.hpp"

#include <cstdio>

namespace kz {
namespace phi {

namespace {

bool is_alpha(char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_'; }
bool is_digit(char c) { return c >= '0' && c <= '9'; }
bool is_hex(char c) { return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
bool is_ident_cont(char c) { return is_alpha(c) || is_digit(c) || c == '.' || c == '-'; }

int hex_value(char c) {
  if (is_digit(c)) return c - '0';
  if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
  return 10 + (c - 'A');
}

std::string byte_text(unsigned char c) {
  char buf[16];
  std::snprintf(buf, sizeof buf, "0x%02X", static_cast<unsigned>(c));
  return buf;
}

}  // namespace

const char* tok_spelling(Tok t) {
  switch (t) {
    case Tok::End: return "end of file";
    case Tok::Ident: return "identifier";
    case Tok::Int: return "integer";
    case Tok::Str: return "string";
    case Tok::LBrace: return "'{'";
    case Tok::RBrace: return "'}'";
    case Tok::LParen: return "'('";
    case Tok::RParen: return "')'";
    case Tok::LBracket: return "'['";
    case Tok::RBracket: return "']'";
    case Tok::Comma: return "','";
    case Tok::Semi: return "';'";
    case Tok::Colon: return "':'";
    case Tok::Dot: return "'.'";
    case Tok::Star: return "'*'";
    case Tok::Eq: return "'='";
    case Tok::EqEq: return "'=='";
    case Tok::Ne: return "'!='";
    case Tok::Lt: return "'<'";
    case Tok::Le: return "'<='";
    case Tok::Gt: return "'>'";
    case Tok::Ge: return "'>='";
    case Tok::AndAnd: return "'&&'";
    case Tok::OrOr: return "'||'";
    case Tok::Bang: return "'!'";
    case Tok::FatArrow: return "'=>'";
    case Tok::Arrow: return "'->'";
    case Tok::Assign: return "':='";
  }
  return "?";
}

bool tokenize(const std::string& text, uint32_t file, std::vector<Token>* out, LexError* err) {
  out->clear();
  const size_t n = text.size();
  size_t i = 0;
  uint32_t line = 1;
  size_t line_start = 0;  // index of the first byte of the current line
  auto here = [&](size_t pos) {
    SrcLoc l;
    l.file = file;
    l.line = line;
    l.col = static_cast<uint32_t>(pos - line_start + 1);
    return l;
  };
  auto fail = [&](size_t pos, const std::string& msg) {
    err->loc = here(pos);
    err->message = msg;
    return false;
  };
  auto push = [&](Tok k, size_t start, size_t len) {
    Token t;
    t.kind = k;
    t.loc = here(start);
    t.text = text.substr(start, len);
    out->push_back(std::move(t));
  };

  while (i < n) {
    const char c = text[i];
    if (c == '\n') {
      ++i;
      ++line;
      line_start = i;
      continue;
    }
    if (c == ' ' || c == '\t' || c == '\r') {
      ++i;
      continue;
    }
    if (c == '#') {
      while (i < n && text[i] != '\n') ++i;
      continue;
    }
    const size_t start = i;
    if (is_alpha(c)) {
      size_t j = i + 1;
      while (j < n && is_ident_cont(text[j])) ++j;
      while (j > i + 1 && (text[j - 1] == '.' || text[j - 1] == '-')) --j;  // punctuation, not part of the name
      push(Tok::Ident, start, j - i);
      i = j;
      continue;
    }
    if (is_digit(c) || (c == '-' && i + 1 < n && is_digit(text[i + 1]))) {
      const bool neg = c == '-';
      size_t j = neg ? i + 1 : i;
      uint64_t v = 0;
      bool overflow = false;
      if (text[j] == '0' && j + 1 < n && (text[j + 1] == 'x' || text[j + 1] == 'X')) {
        j += 2;
        const size_t digits = j;
        while (j < n && is_hex(text[j])) {
          if (v > (UINT64_MAX >> 4)) overflow = true;
          v = (v << 4) | static_cast<uint64_t>(hex_value(text[j]));
          ++j;
        }
        if (j == digits) return fail(start, "hexadecimal literal without digits");
      } else {
        while (j < n && is_digit(text[j])) {
          const uint64_t d = static_cast<uint64_t>(text[j] - '0');
          if (v > (UINT64_MAX - d) / 10) overflow = true;
          v = v * 10 + d;
          ++j;
        }
      }
      if (j < n && (is_alpha(text[j]))) return fail(start, "invalid number: a letter follows the digits");
      const uint64_t limit = neg ? (static_cast<uint64_t>(INT64_MAX) + 1) : static_cast<uint64_t>(INT64_MAX);
      if (overflow || v > limit) return fail(start, "integer literal does not fit in 64 bits");
      Token t;
      t.kind = Tok::Int;
      t.loc = here(start);
      t.text = text.substr(start, j - start);
      t.ival = neg ? static_cast<int64_t>(0 - v) : static_cast<int64_t>(v);
      out->push_back(std::move(t));
      i = j;
      continue;
    }
    if (c == '"') {
      std::string value;
      size_t j = i + 1;
      bool closed = false;
      while (j < n) {
        const unsigned char u = static_cast<unsigned char>(text[j]);
        if (u == '"') {
          closed = true;
          ++j;
          break;
        }
        if (u == '\n') return fail(start, "unterminated string (strings end on the line they start)");
        if (u == '\\') {
          if (j + 1 >= n) return fail(start, "unterminated string");
          const char e = text[j + 1];
          if (e == '\\') value.push_back('\\');
          else if (e == '"') value.push_back('"');
          else if (e == 'n') value.push_back('\n');
          else if (e == 't') value.push_back('\t');
          else return fail(j, std::string("unknown escape '\\") + e + "' in string");
          j += 2;
          continue;
        }
        if (u < 0x20 && u != '\t') return fail(j, "control character " + byte_text(u) + " in string");
        value.push_back(static_cast<char>(u));
        ++j;
      }
      if (!closed) return fail(start, "unterminated string");
      Token t;
      t.kind = Tok::Str;
      t.loc = here(start);
      t.text = std::move(value);
      out->push_back(std::move(t));
      i = j;
      continue;
    }
    const char d = i + 1 < n ? text[i + 1] : '\0';
    auto two = [&](Tok k) {
      push(k, start, 2);
      i += 2;
    };
    auto one = [&](Tok k) {
      push(k, start, 1);
      i += 1;
    };
    switch (c) {
      case '{': one(Tok::LBrace); continue;
      case '}': one(Tok::RBrace); continue;
      case '(': one(Tok::LParen); continue;
      case ')': one(Tok::RParen); continue;
      case '[': one(Tok::LBracket); continue;
      case ']': one(Tok::RBracket); continue;
      case ',': one(Tok::Comma); continue;
      case ';': one(Tok::Semi); continue;
      case '.': one(Tok::Dot); continue;
      case '*': one(Tok::Star); continue;
      case ':':
        if (d == '=') two(Tok::Assign);
        else one(Tok::Colon);
        continue;
      case '=':
        if (d == '=') two(Tok::EqEq);
        else if (d == '>') two(Tok::FatArrow);
        else one(Tok::Eq);
        continue;
      case '!':
        if (d == '=') two(Tok::Ne);
        else one(Tok::Bang);
        continue;
      case '<':
        if (d == '=') two(Tok::Le);
        else one(Tok::Lt);
        continue;
      case '>':
        if (d == '=') two(Tok::Ge);
        else one(Tok::Gt);
        continue;
      case '-':
        if (d == '>') {
          two(Tok::Arrow);
          continue;
        }
        break;
      case '&':
        if (d == '&') {
          two(Tok::AndAnd);
          continue;
        }
        break;
      case '|':
        if (d == '|') {
          two(Tok::OrOr);
          continue;
        }
        break;
      default: break;
    }
    const unsigned char u = static_cast<unsigned char>(c);
    if (u >= 0x80) return fail(start, "non-ASCII byte " + byte_text(u) + " outside a string or comment");
    return fail(start, std::string("unexpected character '") + c + "'");
  }
  Token end;
  end.kind = Tok::End;
  end.loc = here(n);
  out->push_back(std::move(end));
  return true;
}

}  // namespace phi
}  // namespace kz
