// Copyright 2016 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "lexer.h"

#include <list>
#include "gtest/gtest.h"

namespace {

void TestLex(const char* name,
             const char* input,
             const std::list<Token>& tokens,
             const std::string& error) {
  std::list<Token> test_tokens(tokens);
  test_tokens.push_back(Token(Token::Kind::END_OF_FILE, ""));

  try {
    std::list<Token> lexed_tokens = jsonnet_lex(name, input);
    ASSERT_EQ(test_tokens, lexed_tokens)
        << "Test failed: " << name << std::endl;
  } catch (StaticError& e) {
    ASSERT_EQ(error, e.toString());
  }
}

TEST(Lexer, TestWhitespace) {
  TestLex("empty", "", {}, "");
  TestLex("whitespace", "  \t\n\r\r\n", {}, "");
}

TEST(Lexer, TestOperators) {
  TestLex("brace L", "{", {Token(Token::Kind::BRACE_L, "")}, "");
  TestLex("brace R", "}", {Token(Token::Kind::BRACE_R, "")}, "");
  TestLex("bracket L", "[", {Token(Token::Kind::BRACKET_L, "")}, "");
  TestLex("bracket R", "]", {Token(Token::Kind::BRACKET_R, "")}, "");
  TestLex("colon ", ":", {Token(Token::Kind::OPERATOR, ":")}, "");
  TestLex("colon 2", "::", {Token(Token::Kind::OPERATOR, "::")}, "");
  TestLex("colon 2", ":::", {Token(Token::Kind::OPERATOR, ":::")}, "");
  TestLex("arrow right", "->", {Token(Token::Kind::OPERATOR, "->")}, "");
  TestLex("less than minus", "<-",
          {Token(Token::Kind::OPERATOR, "<"),
           Token(Token::Kind::OPERATOR, "-")}, "");
  TestLex("comma", ",", {Token(Token::Kind::COMMA, "")}, "");
  TestLex("dollar", "$", {Token(Token::Kind::DOLLAR, "")}, "");
  TestLex("dot", ".", {Token(Token::Kind::DOT, "")}, "");
  TestLex("paren L", "(", {Token(Token::Kind::PAREN_L, "")}, "");
  TestLex("paren R", ")", {Token(Token::Kind::PAREN_R, "")}, "");
  TestLex("semicolon", ";", {Token(Token::Kind::SEMICOLON, "")}, "");

  TestLex("not 1", "!", {Token(Token::Kind::OPERATOR, "!")}, "");
  TestLex("not 2", "! ", {Token(Token::Kind::OPERATOR, "!")}, "");
  TestLex("not equal", "!=", {Token(Token::Kind::OPERATOR, "!=")}, "");
  TestLex("tilde", "~", {Token(Token::Kind::OPERATOR, "~")}, "");
  TestLex("plus", "+", {Token(Token::Kind::OPERATOR, "+")}, "");
  TestLex("minus", "-", {Token(Token::Kind::OPERATOR, "-")}, "");
}

TEST(Lexer, TestMiscOperators) {
  TestLex("op *", "*", {Token(Token::Kind::OPERATOR, "*")}, "");
  TestLex("op /", "/", {Token(Token::Kind::OPERATOR, "/")}, "");
  TestLex("op %", "%", {Token(Token::Kind::OPERATOR, "%")}, "");
  TestLex("op &", "&", {Token(Token::Kind::OPERATOR, "&")}, "");
  TestLex("op |", "|", {Token(Token::Kind::OPERATOR, "|")}, "");
  TestLex("op ^", "^", {Token(Token::Kind::OPERATOR, "^")}, "");
  TestLex("op =", "=", {Token(Token::Kind::OPERATOR, "=")}, "");
  TestLex("op <", "<", {Token(Token::Kind::OPERATOR, "<")}, "");
  TestLex("op >", ">", {Token(Token::Kind::OPERATOR, ">")}, "");
  TestLex("op >==|", ">==|", {Token(Token::Kind::OPERATOR, ">==|")}, "");
}

TEST(Lexer, TestNumbers) {
  TestLex("number 0", "0", {Token(Token::Kind::NUMBER, "0")}, "");
  TestLex("number 1", "1", {Token(Token::Kind::NUMBER, "1")}, "");
  TestLex("number 1.0", "1.0", {Token(Token::Kind::NUMBER, "1.0")}, "");
  TestLex("number 0.10", "0.10", {Token(Token::Kind::NUMBER, "0.10")}, "");
  TestLex("number 0e100", "0e100", {Token(Token::Kind::NUMBER, "0e100")}, "");
  TestLex("number 1e100", "1e100", {Token(Token::Kind::NUMBER, "1e100")}, "");
  TestLex("number 1.1e100", "1.1e100",
          {Token(Token::Kind::NUMBER, "1.1e100")}, "");
  TestLex("number 1.1e-100", "1.1e-100",
          {Token(Token::Kind::NUMBER, "1.1e-100")}, "");
  TestLex("number 1.1e+100", "1.1e+100",
          {Token(Token::Kind::NUMBER, "1.1e+100")}, "");
  TestLex("number 0100", "0100",
          {Token(Token::Kind::NUMBER, "0"), Token(Token::Kind::NUMBER, "100")},
          "");
  TestLex("number 10+10", "10+10",
          {Token(Token::Kind::NUMBER, "10"),
           Token(Token::Kind::OPERATOR, "+"),
           Token(Token::Kind::NUMBER, "10")}, "");
  TestLex("number 1.+3", "1.+3", {},
          "number 1.+3:1:1: Couldn't lex number, junk after decimal point: +");
  TestLex("number 1e!", "1e!", {},
          "number 1e!:1:1: Couldn't lex number, junk after 'E': !");
  TestLex("number 1e+!", "1e+!", {},
          "number 1e+!:1:1: Couldn't lex number, junk after exponent sign: !");
}

TEST(Lexer, TestDoubleStrings) {
  TestLex("double string \"hi\"",
          "\"hi\"", {Token(Token::Kind::STRING_DOUBLE, "hi")}, "");
  TestLex("double string \"hi nl\"",
          "\"hi\n\"", {Token(Token::Kind::STRING_DOUBLE, "hi\n")}, "");
  TestLex("double string \"hi\\\"\"",
          "\"hi\\\"\"", {Token(Token::Kind::STRING_DOUBLE, "hi\\\"")}, "");
  TestLex("double string \"hi\\nl\"",
          "\"hi\\\n\"", {Token(Token::Kind::STRING_DOUBLE, "hi\\\n")}, "");
  TestLex("double string \"hi",
          "\"hi", {}, "double string \"hi:1:1: Unterminated string");
}

TEST(Lexer, TestSingleStrings) {
  TestLex("single string 'hi'",
          "'hi'", {Token(Token::Kind::STRING_SINGLE, "hi")}, "");
  TestLex("single string 'hi nl'",
          "'hi\n'", {Token(Token::Kind::STRING_SINGLE, "hi\n")}, "");
  TestLex("single string 'hi\\''",
          "'hi\\''", {Token(Token::Kind::STRING_SINGLE, "hi\\'")}, "");
  TestLex("single string 'hi\\nl'",
          "'hi\\\n'", {Token(Token::Kind::STRING_SINGLE, "hi\\\n")}, "");
  TestLex("single string 'hi",
          "'hi", {}, "single string 'hi:1:1: Unterminated string");
}

TEST(Lexer, TestBlockStringSpaces) {
  const char str[] =
      "|||\n"
      "  test\n"
      "    more\n"
      "  |||\n"
      "    foo\n"
      "|||";
  const Token token = Token(
      Token::Kind::STRING_BLOCK,
      {},
      "test\n  more\n|||\n  foo\n",
      "  ",
      "",
      {});
  TestLex("block string spaces", str, {token}, "");
}

TEST(Lexer, TestBlockStringTabs) {
  const char str[] =
      "|||\n"
      "\ttest\n"
      "\t  more\n"
      "\t|||\n"
      "\t  foo\n"
      "|||";
  const Token token = Token(
      Token::Kind::STRING_BLOCK,
      {},
      "test\n  more\n|||\n  foo\n",
      "\t",
      "",
      {});
  TestLex("block string tabs", str, {token}, "");
}

TEST(Lexer, TestBlockStringsMixed) {
  const char str[] =
      "|||\n"
      "\t  \ttest\n"
      "\t  \t  more\n"
      "\t  \t|||\n"
      "\t  \t  foo\n"
      "|||";
  const Token token = Token(
      Token::Kind::STRING_BLOCK,
      {},
      "test\n  more\n|||\n  foo\n",
      "\t  \t",
      "",
      {});
  TestLex("block string mixed", str, {token}, "");
}

TEST(Lexer, TestBlockStringBlanks) {
  const char str[] =
      "|||\n\n"
      "  test\n\n\n"
      "    more\n"
      "  |||\n"
      "    foo\n"
      "|||";
  const Token token = Token(
      Token::Kind::STRING_BLOCK,
      {},
      "\ntest\n\n\n  more\n|||\n  foo\n",
      "  ",
      "",
      {});
  TestLex("block string blanks", str, {token}, "");
}

TEST(Lexer, TestBlockStringBadIndent) {
  const char str[] =
      "|||\n"
      "  test\n"
      " foo\n"
      "|||";
  TestLex("block string bad indent", str, {},
          "block string bad indent:1:1: Text block not terminated with |||");
}

TEST(Lexer, TestBlockStringEof) {
  const char str[] =
      "|||\n"
      "  test";
  TestLex("block string eof", str, {}, "block string eof:1:1: Unexpected EOF");
}

TEST(Lexer, TestBlockStringNotTerm) {
  const char str[] =
      "|||\n"
      "  test\n";
  TestLex("block string not term", str, {},
          "block string not term:1:1: Text block not terminated with |||");
}

TEST(Lexer, TestBlockStringNoWs) {
  const char str[] =
      "|||\n"
      "test\n"
      "|||";
  TestLex("block string no ws", str, {},
          "block string no ws:1:1: Text block's first line must start with"
          " whitespace.");
}

TEST(Lexer, TestKeywords) {
  TestLex("assert", "assert", {Token(Token::Kind::ASSERT, "assert")}, "");
  TestLex("else", "else", {Token(Token::Kind::ELSE, "else")}, "");
  TestLex("error", "error", {Token(Token::Kind::ERROR, "error")}, "");
  TestLex("false", "false", {Token(Token::Kind::FALSE, "false")}, "");
  TestLex("for", "for", {Token(Token::Kind::FOR, "for")}, "");
  TestLex("function", "function",
          {Token(Token::Kind::FUNCTION, "function")}, "");
  TestLex("if", "if", {Token(Token::Kind::IF, "if")}, "");
  TestLex("import", "import", {Token(Token::Kind::IMPORT, "import")}, "");
  TestLex("importstr", "importstr",
          {Token(Token::Kind::IMPORTSTR, "importstr")}, "");
  TestLex("in", "in", {Token(Token::Kind::IN, "in")}, "");
  TestLex("local", "local", {Token(Token::Kind::LOCAL, "local")}, "");
  TestLex("null", "null", {Token(Token::Kind::NULL_LIT, "null")}, "");
  TestLex("self", "self", {Token(Token::Kind::SELF, "self")}, "");
  TestLex("super", "super", {Token(Token::Kind::SUPER, "super")}, "");
  TestLex("tailstrict", "tailstrict",
          {Token(Token::Kind::TAILSTRICT, "tailstrict")}, "");
  TestLex("then", "then", {Token(Token::Kind::THEN, "then")}, "");
  TestLex("true", "true", {Token(Token::Kind::TRUE, "true")}, "");
}

TEST(Lexer, TestIdentifier) {
  TestLex("identifier", "foobar123",
          {Token(Token::Kind::IDENTIFIER, "foobar123")}, "");
  TestLex("identifier", "foo bar123",
          {Token(Token::Kind::IDENTIFIER, "foo"),
           Token(Token::Kind::IDENTIFIER, "bar123")}, "");
}

TEST(Lexer, TestComments) {
  // TODO(dzc): Test does not look at fodder yet.
  TestLex("c++ comment", "// hi", {}, "");
  TestLex("hash comment", "# hi", {}, "");
  TestLex("c comment", "/* hi */", {}, "");
  TestLex("c comment no term", "/* hi", {},
          "c comment no term:1:1: Multi-line comment has no terminating */.");
}

}  // namespace
