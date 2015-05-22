/*
Copyright 2015 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef JSONNET_LEXER_H
#define JSONNET_LEXER_H

#include <cstdlib>
#include <iostream>
#include <string>
#include <list>
#include <sstream>

#include "static_error.h"

struct Token {
    enum Kind {
        // Symbols
        BRACE_L,
        BRACE_R,
        BRACKET_L,
        BRACKET_R,
        COLON,
        COMMA,
        DOLLAR,
        DOT,
        PAREN_L,
        PAREN_R,
        SEMICOLON,

        // Arbitrary length lexemes
        IDENTIFIER,
        NUMBER,
        OPERATOR,
        STRING,

        // Keywords
        ELSE,
        ERROR,
        FALSE,
        FOR,
        FUNCTION,
        IF,
        IMPORT,
        IMPORTSTR,
        IN,
        LOCAL,
        NULL_LIT,
        TAILSTRICT,
        THEN,
        SELF,
        SUPER,
        TRUE,

        // A special token that holds line/column information about the end of the file.
        END_OF_FILE
    } kind;

    std::string data;

    LocationRange location;

    Token(Kind kind, const std::string &data, const LocationRange &location)
      : kind(kind), data(data), location(location)
    { }

    Token(Kind kind, const std::string &data="") : kind(kind), data(data) { }

    static const char *toString(Kind v)
    {
        switch (v) {
            case BRACE_L: return "\"{\"";
            case BRACE_R: return "\"}\"";
            case BRACKET_L: return "\"[\"";
            case BRACKET_R: return "\"]\"";
            case COLON: return "\":\"";
            case COMMA: return "\",\"";
            case DOLLAR: return "\"$\"";
            case DOT: return "\".\"";

            case PAREN_L: return "\"(\"";
            case PAREN_R: return "\")\"";
            case SEMICOLON: return "\";\"";

            case IDENTIFIER: return "IDENTIFIER";
            case NUMBER: return "NUMBER";
            case OPERATOR: return "OPERATOR";
            case STRING: return "STRING";

            case ELSE: return "else";
            case ERROR: return "error";
            case FALSE: return "false";
            case FOR: return "for";
            case FUNCTION: return "function";
            case IF: return "if";
            case IMPORT: return "import";
            case IMPORTSTR: return "importstr";
            case IN: return "in";
            case LOCAL: return "local";
            case NULL_LIT: return "null";
            case SELF: return "self";
            case SUPER: return "super";
            case TAILSTRICT: return "tailstrict";
            case THEN: return "then";
            case TRUE: return "true";

            case END_OF_FILE: return "end of file";
            default:
            std::cerr << "INTERNAL ERROR: Unknown token kind: " << v << std::endl;
            std::abort();
        }
    }
};

static inline bool operator==(const Token &a, const Token &b)
{
    if (a.kind != b.kind) return false;
    if (a.data != b.data) return false;
    return true;
}

static inline std::ostream &operator<<(std::ostream &o, Token::Kind v)
{
    o << Token::toString(v);
    return o;
}

static inline std::ostream &operator<<(std::ostream &o, const Token &v)
{
    if (v.data == "") {
        o << Token::toString(v.kind);
    } else if (v.kind == Token::OPERATOR) {
            o << "\"" << v.data << "\"";
    } else {
        o << "(" << Token::toString(v.kind) << ", \"" << v.data << "\")";
    }
    return o;
}

std::list<Token> jsonnet_lex(const std::string &filename, const char *input);

#endif
