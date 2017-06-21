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
#include <cassert>

#include <iostream>
#include <string>
#include <list>
#include <sstream>
#include <vector>

#include "unicode.h"
#include "static_error.h"

/** Whitespace and comments.
 *
 * "Fodder" (as in cannon fodder) implies this data is expendable.
 */
struct FodderElement {
    enum Kind {
        LINE_END,
        INTERSTITIAL,
        PARAGRAPH,
    };
    Kind kind;
    unsigned blanks;
    unsigned indent;
    std::vector<std::string> comment;
    FodderElement(Kind kind, unsigned blanks, unsigned indent,
                  const std::vector<std::string> &comment)
      : kind(kind), blanks(blanks), indent(indent), comment(comment)
    {
        assert(kind != LINE_END || comment.size() <= 1);
        assert(kind != INTERSTITIAL || (blanks == 0 && indent == 0 && comment.size() == 1));
        assert(kind != PARAGRAPH || comment.size() >= 1);
    }
};
static inline std::ostream &operator<<(std::ostream &o, const FodderElement &f)
{
    switch (f.kind) {
        case FodderElement::LINE_END:
        o << "END(" << f.blanks << ", " << f.indent << ", " << f.comment[0] << ")";
        break;
        case FodderElement::INTERSTITIAL:
        o << "INT(" << f.blanks << ", " << f.indent << ", " << f.comment[0] << ")";
        break;
        case FodderElement::PARAGRAPH:
        o << "PAR(" << f.blanks << ", " << f.indent << ", " << f.comment[0] << "...)";
        break;
    }
    return o;
}
typedef std::vector<FodderElement> Fodder;

static inline std::ostream &operator<<(std::ostream &o, const Fodder &fodder)
{
    bool first = true;
    for (const auto &f : fodder) {
        o << (first ? "[" : ", ");
        first = false;
        o << f;
    }
    o << (first ? "[]" : "]");
    return o;
}

struct Token {
    enum Kind {
        // Symbols
        BRACE_L,
        BRACE_R,
        BRACKET_L,
        BRACKET_R,
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
        STRING_DOUBLE,
        STRING_SINGLE,
        STRING_BLOCK,
        VERBATIM_STRING_SINGLE,
        VERBATIM_STRING_DOUBLE,

        // Keywords
        ASSERT,
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

    /** Fodder before this token. */
    Fodder fodder;

    /** Content of the token if it wasn't a keyword. */
    std::string data;

    /** If kind == STRING_BLOCK then stores the sequence of whitespace that indented the block. */
    std::string stringBlockIndent;

    /** If kind == STRING_BLOCK then stores the sequence of whitespace that indented the end of
     * the block.
     *
     * This is always fewer whitespace characters than in stringBlockIndent.
     */
    std::string stringBlockTermIndent;

    UString data32(void) { return decode_utf8(data); }

    LocationRange location;

    Token(Kind kind, const Fodder &fodder, const std::string &data,
          const std::string &string_block_indent, const std::string &string_block_term_indent,
          const LocationRange &location)
      : kind(kind), fodder(fodder), data(data), stringBlockIndent(string_block_indent),
        stringBlockTermIndent(string_block_term_indent), location(location)
    { }

    Token(Kind kind, const std::string &data="")
        : kind(kind), data(data) { }

    static const char *toString(Kind v)
    {
        switch (v) {
            case BRACE_L: return "\"{\"";
            case BRACE_R: return "\"}\"";
            case BRACKET_L: return "\"[\"";
            case BRACKET_R: return "\"]\"";
            case COMMA: return "\",\"";
            case DOLLAR: return "\"$\"";
            case DOT: return "\".\"";

            case PAREN_L: return "\"(\"";
            case PAREN_R: return "\")\"";
            case SEMICOLON: return "\";\"";

            case IDENTIFIER: return "IDENTIFIER";
            case NUMBER: return "NUMBER";
            case OPERATOR: return "OPERATOR";
            case STRING_SINGLE: return "STRING_SINGLE";
            case STRING_DOUBLE: return "STRING_DOUBLE";
            case VERBATIM_STRING_SINGLE: return "VERBATIM_STRING_SINGLE";
            case VERBATIM_STRING_DOUBLE: return "VERBATIM_STRING_DOUBLE";
            case STRING_BLOCK: return "STRING_BLOCK";

            case ASSERT: return "assert";
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

/** The result of lexing.
 *
 * Because of the EOF token, this will always contain at least one token.  So element 0 can be used
 * to get the filename.
 */
typedef std::list<Token> Tokens;

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

/** IF the given identifier is a keyword, return its kind, otherwise return IDENTIFIER. */
Token::Kind lex_get_keyword_kind(const std::string &identifier);

Tokens jsonnet_lex(const std::string &filename, const char *input);

std::string jsonnet_unlex(const Tokens &tokens);

#endif  // JSONNET_LEXER_H
