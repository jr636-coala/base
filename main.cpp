#include <cstdio>
#include "base.hpp"

#define TOKEN_TYPE(_) \
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
  _(Identifier, "[abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ][abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789]*") \
  _(Whitespace, "( |\t|\n)+") \
  _(Equal, "==")

enum class TokenType {
#define _(t, r) t,
TOKEN_TYPE(_)
#undef _
};

std::string tokenTypeToString(TokenType t) {
  switch (t) {
#define _(t, r) case TokenType::t: return #t;
TOKEN_TYPE(_)
#undef _
  }
}

using RL = RegexLexer<TokenType>;

int main(int argc, char** argv) {
  for (auto i = 0; i < 500; ++i) {
    RL regex;
#define _(t, r) regex.alter(RL::parse(r, TokenType::t));
TOKEN_TYPE(_);
#undef _
    regex.dfa();

    auto match = regex.match("");
    for (const auto x : match) printf("%s\n", tokenTypeToString(x).c_str());
  }
  return 0;
}
