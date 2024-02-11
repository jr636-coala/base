#include <string>
#include "base.hpp"

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
  _(Whitespace, "( |\t|\n)+") \
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

std::string tokenTypeToString(TokenType t) {
  switch (t) {
#define _(t, r) case TokenType::t: return #t;
TOKEN_TYPE(_)
#undef _
  }
}

using RL = RegexLexer<TokenType, TokenType::Null, [](TokenType a, TokenType b){
  if (a == TokenType::Null) return b;
  switch (b) {
    case TokenType::Null: case TokenType::Identifier: return a;
    default: return b;
  }
}>;

int main(int argc, char** argv) {
  for (auto i = 0; i < 100; ++i) {
  RL regex;
#define _(t, r) regex.alter(RL::parse(r, TokenType::t));
TOKEN_TYPE(_);
#undef _
  regex.dfa();
  const auto match = regex.match("");
  if (match != TokenType::Null) printf("%s\n", tokenTypeToString(match).c_str());
  }
  return 0;
}
