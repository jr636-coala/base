#pragma once
#include <cassert>
#include "base.hpp"

// BASE_TEST_REGEX

namespace Base {
#ifdef BASE_TEST_REGEX
#define TOKEN_TYPE(_) \
  _(Null, "") \
  _(Type, "type") \
  _(Macro, "macro") \
  _(OpenMacro, "openmacro") \
  _(Is, "is") \
  _(IsNot, "isnot") \
  _(Fn, "fn") \
  _(Return, "return") \
  _(If, "if") \
  _(Else, "else") \
  _(While, "while") \
  _(Identifier, "[_$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ][_$abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789]*") \
  _(Whitespace, "[ \t\n]+") \
  _(String, "\"[^\"]*\"") \
  _(Equal, "==") _(NotEqual, "!=") \
  _(Template, "\\\\<") \
  _(Backslash, "\\\\") \
  _(DoubleColon, "::") \
  _(Hash, "#") \
  _(Pipe, "\\|") \
  _(Colon, ":") \
  _(Semicolon, ";") \
  _(LeftParenthesis, "\\(") _(RightParenthesis, "\\)") \
  _(Comma, ",") \
  _(LeftCurly, "{") _(RightCurly, "}") \
  _(Assign, "=") \
  _(LeftSquare, "\\[") _(RightSquare, "\\]") \
  _(LessThan, "<") _(GreaterThan, ">") \
  _(Plus, "\\+") _(Minus, "-") _(Star, "\\*") _(Slash, "/") \
  _(Dot, ".") \
  _(And, "&")

enum class TokenType {
#define _(t, r) t,
TOKEN_TYPE(_)
#undef _
};

using RL = RegexLexer<TokenType, TokenType::Null, [](TokenType a, TokenType b){
  if (a == TokenType::Null) return b;
  switch (b) {
    case TokenType::Null: case TokenType::Identifier: return a;
    default: return b;
  }
}>;

auto tokenTypeToString(TokenType t) {
  switch (t) {
#define _(t, r) case TokenType::t: return #t;
TOKEN_TYPE(_)
#undef _
  }
}
auto regexsize(const RL& regex) {
  auto size = 0;
  for (const auto& m : regex.transitions) {
    for (const auto& _ : m) ++size;
  }
  return size;
}
#endif

void basetest() {
#ifdef BASE_TEST_REGEX

  RL regex;
#define _(t, r) regex.alter(RL::parse(r, TokenType::t));
TOKEN_TYPE(_);
#undef _

  regex.dfa();
  
#define print printf("states: %lu transitions: %u\n", regex.states.size(), regexsize(regex))

  //print;
  regex.minimise();
  //print;

  /*
  printf("start: %d accept:", regex.start);
  for (const auto& x : regex.accept) printf(" %d", x);
  printf("\n");
  for (auto i = 0; i < regex.states.size(); ++i) {
    for (const auto& [c, v] : regex.transitions[i]) {
      //printf("%d %d %c %s\n", i, v.val, c, tokenTypeToString(regex.states[v.val]).c_str());
    }
  }
  */

  assert(std::find(regex.alphabet.begin(), regex.alphabet.end(), 0) == regex.alphabet.end());
  assert(regex.match("") == TokenType::Null);
  assert(regex.match("!=") == TokenType::NotEqual);
  assert(regex.match("==") == TokenType::Equal);
  assert(regex.match("if") == TokenType::If);
  assert(regex.match("ife") == TokenType::Identifier);
  assert(regex.match("     ") == TokenType::Whitespace);
  assert(regex.match("\t\n ") == TokenType::Whitespace);
  assert(regex.match("\"Hello World\\n\"") == TokenType::String);

#endif
}
}
