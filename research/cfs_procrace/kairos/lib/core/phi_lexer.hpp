// phi_lexer.hpp -- tokens of the profile language (internal to libkz-core).
//
// Lexical rules (docs/phi_reference.md section 2):
//   - '#' starts a comment that runs to the end of the line, outside strings;
//   - an identifier starts with a letter or '_' and continues with letters, digits, '_', '.', '-'; a
//     trailing '.' or '-' belongs to the next token (so `A->B` is A, ->, B and `x.y.` is x.y, .);
//   - an integer is decimal or 0x-hexadecimal, optionally with a leading '-', and fits in int64;
//   - a string is "..." with the escapes \\ \" \n \t, on one line;
//   - outside comments and strings only ASCII is accepted;
//   - columns count bytes.
// The lint of the repository reads profiles with the same rules (tests/lint/kz_lint.cpp).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "kz/core/phi_ast.hpp"

namespace kz {
namespace phi {

enum class Tok : uint8_t {
  End, Ident, Int, Str,
  LBrace, RBrace, LParen, RParen, LBracket, RBracket, Comma, Semi, Colon, Dot, Star,
  Eq, EqEq, Ne, Lt, Le, Gt, Ge, AndAnd, OrOr, Bang, FatArrow, Arrow, Assign
};

struct Token {
  Tok kind = Tok::End;
  SrcLoc loc;
  std::string text;   // Ident: the name; Str: the unescaped value; punctuation: its spelling
  int64_t ival = 0;   // Int
};

struct LexError {
  SrcLoc loc;
  std::string message;
};

// Splits `text` into tokens (the last one is End).  Stops at the first error.
bool tokenize(const std::string& text, uint32_t file, std::vector<Token>* out, LexError* err);

const char* tok_spelling(Tok t);  // for messages: "';'", "identifier", ...

}  // namespace phi
}  // namespace kz
