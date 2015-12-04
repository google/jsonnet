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

#include <cstdlib>
#include <cassert>
#include <cmath>

#include <list>
#include <set>
#include <sstream>
#include <string>
#include <iomanip>


#include "ast.h"
#include "desugaring.h"
#include "lexer.h"
#include "parser.h"
#include "static_error.h"


// TODO(dcunnin): Preserve:
// additional parens
// whitespace
// comments

namespace {

    // Cache some immutable stuff in global variables.
    const int APPLY_PRECEDENCE = 2;  // Function calls and indexing.
    const int UNARY_PRECEDENCE = 4;  // Logical and bitwise negation, unary + -
    const int BEFORE_ELSE_PRECEDENCE = 15; // True branch of an if.
    const int MAX_PRECEDENCE = 16; // Local, If, Import, Function, Error

    /** These are the binary operator precedences, unary precedence is given by
     * UNARY_PRECEDENCE.
     */
    std::map<BinaryOp, int> build_precedence_map(void)
    {
        std::map<BinaryOp, int> r;

        r[BOP_MULT] = 5;
        r[BOP_DIV] = 5;
        r[BOP_PERCENT] = 5;

        r[BOP_PLUS] = 6;
        r[BOP_MINUS] = 6;

        r[BOP_SHIFT_L] = 7;
        r[BOP_SHIFT_R] = 7;

        r[BOP_GREATER] = 8;
        r[BOP_GREATER_EQ] = 8;
        r[BOP_LESS] = 8;
        r[BOP_LESS_EQ] = 8;

        r[BOP_MANIFEST_EQUAL] = 9;
        r[BOP_MANIFEST_UNEQUAL] = 9;

        r[BOP_BITWISE_AND] = 10;

        r[BOP_BITWISE_XOR] = 11;

        r[BOP_BITWISE_OR] = 12;

        r[BOP_AND] = 13;

        r[BOP_OR] = 14;

        return r;
    }

    std::map<std::string, UnaryOp> build_unary_map(void)
    {
        std::map<std::string, UnaryOp> r;
        r["!"] = UOP_NOT;
        r["~"] = UOP_BITWISE_NOT;
        r["+"] = UOP_PLUS;
        r["-"] = UOP_MINUS;
        return r;
    }

    std::map<std::string, BinaryOp> build_binary_map(void)
    {
        std::map<std::string, BinaryOp> r;

        r["*"] = BOP_MULT;
        r["/"] = BOP_DIV;
        r["%"] = BOP_PERCENT;

        r["+"] = BOP_PLUS;
        r["-"] = BOP_MINUS;

        r["<<"] = BOP_SHIFT_L;
        r[">>"] = BOP_SHIFT_R;

        r[">"] = BOP_GREATER;
        r[">="] = BOP_GREATER_EQ;
        r["<"] = BOP_LESS;
        r["<="] = BOP_LESS_EQ;

        r["=="] = BOP_MANIFEST_EQUAL;
        r["!="] = BOP_MANIFEST_UNEQUAL;

        r["&"] = BOP_BITWISE_AND;
        r["^"] = BOP_BITWISE_XOR;
        r["|"] = BOP_BITWISE_OR;

        r["&&"] = BOP_AND;
        r["||"] = BOP_OR;
        return r;
    }

    auto precedence_map = build_precedence_map();
    auto unary_map = build_unary_map();
    auto binary_map = build_binary_map();
}


String jsonnet_unparse_escape(const String &str)
{
    StringStream ss;
    ss << U'\"';
    for (std::size_t i=0 ; i<str.length() ; ++i) {
        char32_t c = str[i];
        switch (c) {
            case U'"': ss << U"\\\""; break;
            case U'\\': ss << U"\\\\"; break;
            case U'\b': ss << U"\\b"; break;
            case U'\f': ss << U"\\f"; break;
            case U'\n': ss << U"\\n"; break;
            case U'\r': ss << U"\\r"; break;
            case U'\t': ss << U"\\t"; break;
            case U'\0': ss << U"\\u0000"; break;
            default: {
                if (c < 0x20 || (c >= 0x7f && c <= 0x9f)) {
                    //Unprintable, use \u
                    std::stringstream ss8;
                    ss8 << "\\u" << std::hex << std::setfill('0') << std::setw(4)
                       << (unsigned long)(c);
                    ss << decode_utf8(ss8.str());
                } else {
                    // Printable, write verbatim
                    ss << c;
                }
            }
        }
    }
    ss << U'\"';
    return ss.str();
}

std::string jsonnet_unparse_number(double v)
{
    std::stringstream ss;
    if (v == floor(v)) {
        ss << std::fixed << std::setprecision(0) << v;
    } else {
        // See "What Every Computer Scientist Should Know About Floating-Point Arithmetic"
        // Theorem 15
        // http://docs.oracle.com/cd/E19957-01/806-3568/ncg_goldberg.html
        ss << std::setprecision(17);
        ss << v;
    }
    return ss.str();
}

static std::string unparse(const Identifier *id)
{
    return encode_utf8(id->name);
}

static std::string unparse(const AST *ast_, int precedence);

static std::string unparse(const std::vector<ComprehensionSpec> &specs)
{
    std::stringstream ss;
    for (const auto &spec : specs) {
        switch (spec.kind) {
            case ComprehensionSpec::FOR:
            ss << " for " << unparse(spec.var) << " in " << unparse(spec.expr, MAX_PRECEDENCE);
            break;
            case ComprehensionSpec::IF:
            ss << " if " << unparse(spec.expr, MAX_PRECEDENCE);
            break;
        }
    }
    return ss.str();
}

static std::string unparse(const ObjectFields &fields)
{
    const char *prefix = "";
    std::stringstream ss;
    for (const auto &field : fields) {

        std::string meth;
        if (field.methodSugar) {
            std::stringstream ss2;
            ss2 << "(";
            const char *prefix = "";
            for (const Identifier *param : field.ids) {
                ss2 << prefix << unparse(param);
                prefix = ", ";
            }
            ss2 << ")";
            meth = ss2.str();
        }

        ss << prefix;

        switch (field.kind) {
            case ObjectField::LOCAL: {
                ss << "local " << unparse(field.id);
                ss << meth;
                ss << " = " << unparse(field.expr2, MAX_PRECEDENCE);
            } break;

            case ObjectField::FIELD_ID:
            case ObjectField::FIELD_STR:
            case ObjectField::FIELD_EXPR: {

                if (field.kind == ObjectField::FIELD_ID) {
                    ss << unparse(field.id);
                } else if (field.kind == ObjectField::FIELD_STR) {
                    ss << unparse(field.expr1, MAX_PRECEDENCE);
                } else if (field.kind == ObjectField::FIELD_EXPR) {
                    ss << "[" << unparse(field.expr1, MAX_PRECEDENCE) << "]";
                }
                ss << meth;

                if (field.superSugar) ss << "+";
                switch (field.hide) {
                    case ObjectField::INHERIT: ss << ": "; break;
                    case ObjectField::HIDDEN: ss << ":: "; break;
                    case ObjectField::VISIBLE: ss << "::: "; break;
                }
                ss << unparse(field.expr2, MAX_PRECEDENCE);

            } break;

            case ObjectField::ASSERT: {
                ss << "assert " << unparse(field.expr2, MAX_PRECEDENCE);
                if (field.expr3 != nullptr) {
                    ss << " : " << unparse(field.expr3, MAX_PRECEDENCE);
                }
            } break;
        }
        prefix = ", ";
    }
    return ss.str();
}

/** Unparse the given AST.
 *
 * \param precedence The precedence of the enclosing AST.  If this is greater than the current
 * precedence, parens are not needed.
 *
 * \returns The unparsed string.
 */
static std::string unparse(const AST *ast_, int precedence)
{
    std::stringstream ss;
    int current_precedence = 0;  // Default is for atoms.

    if (auto *ast = dynamic_cast<const Apply*>(ast_)) {
        current_precedence = APPLY_PRECEDENCE;
        ss << unparse(ast->target, APPLY_PRECEDENCE);
        ss << "(";
        const char *prefix = "";
        for (auto arg : ast->arguments) {
            ss << prefix << unparse(arg, MAX_PRECEDENCE);
            prefix = ", ";
        }
        if (ast->trailingComma) ss << ",";
        ss << ")";
        if (ast->tailstrict) ss << " tailstrict";

    } else if (auto *ast = dynamic_cast<const ApplyBrace*>(ast_)) {
        current_precedence = APPLY_PRECEDENCE;
        ss << unparse(ast->left, APPLY_PRECEDENCE) << unparse(ast->right, precedence);

    } else if (auto *ast = dynamic_cast<const Array*>(ast_)) {
        ss << "[";
        const char *prefix = "";
        for (const auto &element : ast->elements) {
            ss << prefix << unparse(element, MAX_PRECEDENCE);
            prefix = ", ";
        }
        if (ast->trailingComma) ss << ",";
        ss << "]";

    } else if (auto *ast = dynamic_cast<const ArrayComprehension*>(ast_)) {
        ss << "[";
        ss << unparse(ast->body, MAX_PRECEDENCE);
        if (ast->trailingComma) ss << ",";
        ss << unparse(ast->specs);
        ss << "]";

    } else if (auto *ast = dynamic_cast<const Assert*>(ast_)) {
        current_precedence = MAX_PRECEDENCE;
        ss << "assert " << unparse(ast->cond, MAX_PRECEDENCE);
        if (ast->message != nullptr)
            ss << " : " << unparse(ast->message, MAX_PRECEDENCE);
        ss << "; " << unparse(ast->rest, precedence);

    } else if (auto *ast = dynamic_cast<const Binary*>(ast_)) {
        current_precedence = precedence_map[ast->op];
        ss << unparse(ast->left, current_precedence) << " " << bop_string(ast->op) << " "
           << unparse(ast->right, current_precedence - 1);  // The - 1 is for left associativity.

    } else if (auto *ast = dynamic_cast<const BuiltinFunction*>(ast_)) {
        ss << "/* builtin " << ast->id << " */ null";

    } else if (auto *ast = dynamic_cast<const Conditional*>(ast_)) {
        current_precedence = MAX_PRECEDENCE;
        ss << "if " << unparse(ast->cond, MAX_PRECEDENCE) << " then ";
        if (ast->branchFalse != nullptr) {
            ss << unparse(ast->branchTrue, BEFORE_ELSE_PRECEDENCE)
               << " else " << unparse(ast->branchFalse, precedence);
        } else {
            ss << unparse(ast->branchTrue, precedence);
        }

    } else if (dynamic_cast<const Dollar*>(ast_)) {
        ss << "$";

    } else if (auto *ast = dynamic_cast<const Error*>(ast_)) {
        current_precedence = MAX_PRECEDENCE;
        ss << "error " << unparse(ast->expr, precedence);

    } else if (auto *ast = dynamic_cast<const Function*>(ast_)) {
        current_precedence = MAX_PRECEDENCE;
        ss << "function (";
        const char *prefix = "";
        for (const Identifier *arg : ast->parameters) {
            ss << prefix << unparse(arg);
            prefix = ", ";
        }
        if (ast->trailingComma) ss << ",";
        ss << ") " << unparse(ast->body, precedence);

    } else if (auto *ast = dynamic_cast<const Import*>(ast_)) {
        ss << "import " << encode_utf8(jsonnet_unparse_escape(ast->file));

    } else if (auto *ast = dynamic_cast<const Importstr*>(ast_)) {
        ss << "importstr " << encode_utf8(jsonnet_unparse_escape(ast->file));

    } else if (auto *ast = dynamic_cast<const Index*>(ast_)) {
        current_precedence = APPLY_PRECEDENCE;
        ss << unparse(ast->target, current_precedence);
        if (ast->id != nullptr) {
            ss << "." << unparse(ast->id);
        } else {
            ss << "[" << unparse(ast->index, MAX_PRECEDENCE) << "]";
        }

    } else if (auto *ast = dynamic_cast<const Local*>(ast_)) {
        current_precedence = MAX_PRECEDENCE;
        const char *prefix = "local ";
        assert(ast->binds.size() > 0);
        for (const auto &bind : ast->binds) {
            ss << prefix << unparse(bind.var);
            if (bind.functionSugar) {
                ss << "(";
                const char *prefix = "";
                for (const Identifier *id : bind.params) {
                    ss << prefix << unparse(id);
                    prefix = ", ";
                }
                if (bind.trailingComma) ss << ",";
                ss << ")";
            }
            ss << " = " << unparse(bind.body, MAX_PRECEDENCE);
            prefix = ", ";
        }
        ss << "; " << unparse(ast->body, precedence);

    } else if (auto *ast = dynamic_cast<const LiteralBoolean*>(ast_)) {
        ss << (ast->value ? "true" : "false");

    } else if (auto *ast = dynamic_cast<const LiteralNumber*>(ast_)) {
        ss << ast->originalString;

    } else if (auto *ast = dynamic_cast<const LiteralString*>(ast_)) {
        // TODO(dcunnin): Unparse string using ast->tokenKind etc
        ss << encode_utf8(jsonnet_unparse_escape(ast->value));

    } else if (dynamic_cast<const LiteralNull*>(ast_)) {
        ss << "null";

    } else if (auto *ast = dynamic_cast<const Object*>(ast_)) {
        ss << "{";
        ss << unparse(ast->fields);
        if (ast->trailingComma) ss << ",";
        ss << "}";

    } else if (auto *ast = dynamic_cast<const DesugaredObject*>(ast_)) {
        ss << "{";
        for (AST *assert : ast->asserts) {
            ss << "assert " << unparse(assert, MAX_PRECEDENCE) << ",";
        }
        for (auto &field : ast->fields) {
            ss << "[" << unparse(field.name, MAX_PRECEDENCE) << "]";
            switch (field.hide) {
                case ObjectField::INHERIT: ss << ": "; break;
                case ObjectField::HIDDEN: ss << ":: "; break;
                case ObjectField::VISIBLE: ss << "::: "; break;
            }
            ss << unparse(field.body, MAX_PRECEDENCE) << ",";
        }
        ss << "}";

    } else if (auto *ast = dynamic_cast<const ObjectComprehension*>(ast_)) {
        ss << "{";
        ss << unparse(ast->fields);
        if (ast->trailingComma) ss << ",";
        ss << unparse(ast->specs);
        ss << "}";

    } else if (auto *ast = dynamic_cast<const ObjectComprehensionSimple*>(ast_)) {
        ss << "{[" << unparse(ast->field, MAX_PRECEDENCE) << "]: "
           << unparse(ast->value, MAX_PRECEDENCE)
           << " for " << unparse(ast->id) << " in " << unparse(ast->array, MAX_PRECEDENCE)
           << "}";

    } else if (dynamic_cast<const Self*>(ast_)) {
        ss << "self";

    } else if (auto *ast = dynamic_cast<const SuperIndex*>(ast_)) {
        current_precedence = APPLY_PRECEDENCE;
        if (ast->id != nullptr) {
            ss << "super." << unparse(ast->id);
        } else {
            ss << "super[" << unparse(ast->index, MAX_PRECEDENCE) << "]";
        }

    } else if (auto *ast = dynamic_cast<const Unary*>(ast_)) {
        current_precedence = APPLY_PRECEDENCE;
        ss << uop_string(ast->op) << unparse(ast->expr, current_precedence);

    } else if (auto *ast = dynamic_cast<const Var*>(ast_)) {
        ss << encode_utf8(ast->id->name);

    } else {
        std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
        std::abort();

    }

    if (precedence < current_precedence)
        return "(" + ss.str() + ")";

    return ss.str();
}

std::string jsonnet_unparse_jsonnet(const AST *ast) 
{
    return unparse(ast, MAX_PRECEDENCE);
}


namespace {

    bool op_is_unary(const std::string &op, UnaryOp &uop)
    {
        auto it = unary_map.find(op);
        if (it == unary_map.end()) return false;
        uop = it->second;
        return true;
    }

    bool op_is_binary(const std::string &op, BinaryOp &bop)
    {
        auto it = binary_map.find(op);
        if (it == binary_map.end()) return false;
        bop = it->second;
        return true;
    }

    LocationRange span(const Token &begin)
    {
        return LocationRange(begin.location.file, begin.location.begin, begin.location.end);
    }

    LocationRange span(const Token &begin, const Token &end)
    {
        return LocationRange(begin.location.file, begin.location.begin, end.location.end);
    }

    LocationRange span(const Token &begin, AST *end)
    {
        return LocationRange(begin.location.file, begin.location.begin, end->location.end);
    }

    /** Holds state while parsing a given token list.
     */
    class Parser {

        // The private member functions are utilities for dealing with the token stream.

        StaticError unexpected(const Token &tok, const std::string &while_)
        {
            std::stringstream ss;
            ss << "Unexpected: " << tok.kind << " while " << while_;
            return StaticError(tok.location, ss.str());
        }

        Token pop(void)
        {
            Token tok = peek();
            tokens.pop_front();
            return tok;
        }

        Token peek(void)
        {
            Token tok = tokens.front();
            return tok;
        }

        Token popExpect(Token::Kind k, const char *data=nullptr)
        {
            Token tok = pop();
            if (tok.kind != k) {
                std::stringstream ss;
                ss << "Expected token " << k << " but got " << tok;
                throw StaticError(tok.location, ss.str());
            }
            if (data != nullptr && tok.data != data) {
                std::stringstream ss;
                ss << "Expected operator " << data << " but got " << tok.data;
                throw StaticError(tok.location, ss.str());
            }
            return tok;
        }

        std::list<Token> &tokens;
        Allocator *alloc;

        public:

        Parser(Tokens &tokens, Allocator *alloc)
          : tokens(tokens), alloc(alloc)
        { }

        /** Parse a comma-separated list of expressions.
         *
         * Allows an optional ending comma.
         * \param exprs Expressions added here.
         * \param end The token that ends the list (e.g. ] or )).
         * \param element_kind Used in error messages when a comma was not found.
         * \returns The last token (the one that matched parameter end).
         */
        Token parseCommaList(std::vector<AST*> &exprs,
                             Token::Kind end,
                             const std::string &element_kind,
                             bool &got_comma)
        {
            got_comma = false;
            bool first = true;
            do {
                Token next = peek();
                if (!first && !got_comma) {
                    if (next.kind == Token::COMMA) {
                        pop();
                        next = peek();
                        got_comma = true;
                    }
                }
                if (next.kind == end) {
                    // got_comma can be true or false here.
                    return pop();
                }
                if (!first && !got_comma) {
                    std::stringstream ss;
                    ss << "Expected a comma before next " << element_kind <<  ".";
                    throw StaticError(next.location, ss.str());
                }
                exprs.push_back(parse(MAX_PRECEDENCE));
                got_comma = false;
                first = false;
            } while (true);
        }

        Identifiers parseIdentifierList(const std::string &element_kind, bool &got_comma)
        {
            std::vector<AST*> exprs;
            parseCommaList(exprs, Token::PAREN_R, element_kind, got_comma);

            // Check they're all identifiers
            Identifiers ret;
            for (AST *p_ast : exprs) {
                auto *p = dynamic_cast<Var*>(p_ast);
                if (p == nullptr) {
                    std::stringstream ss;
                    ss << "Not an identifier: " << p_ast;
                    throw StaticError(p_ast->location, ss.str());
                }
                ret.push_back(p->id);
            }

            return ret;
        }

        void parseBind(Local::Binds &binds)
        {
            Token var_id = popExpect(Token::IDENTIFIER);
            auto *id = alloc->makeIdentifier(var_id.data32());
            for (const auto &bind : binds) {
                if (bind.var == id)
                    throw StaticError(var_id.location,
                                      "Duplicate local var: " + var_id.data);
            }
            if (peek().kind == Token::PAREN_L) {
                pop();
                bool got_comma;
                auto params = parseIdentifierList("function parameter", got_comma);
                popExpect(Token::OPERATOR, "=");
                AST *body = parse(MAX_PRECEDENCE);
                binds.emplace_back(id, body, true, params, got_comma);
            } else {
                popExpect(Token::OPERATOR, "=");
                binds.emplace_back(id, parse(MAX_PRECEDENCE));
            }
        }


        Token parseObjectRemainder(AST *&obj, const Token &tok)
        {
            ObjectFields fields;
            std::set<std::string> literal_fields;  // For duplicate fields detection.
            std::set<const Identifier *> binds;  // For duplicate locals detection.

            bool got_comma = false;
            bool first = true;

            do {

                Token next = pop();
                if (!got_comma && !first) {
                    if (next.kind == Token::COMMA) {
                        next = pop();
                        got_comma = true;
                    }
                }
                if (next.kind == Token::BRACE_R) {
                    obj = alloc->make<Object>(span(tok, next), fields, got_comma);
                    return next;

                } else if (next.kind == Token::FOR) {
                    // It's a comprehension
                    unsigned num_fields = 0;
                    unsigned num_asserts = 0;
                    const ObjectField *field_ptr = nullptr;
                    for (const auto &field : fields) {
                        if (field.kind == ObjectField::LOCAL) continue;
                        if (field.kind == ObjectField::ASSERT) {
                            num_asserts++;
                            continue;
                        }
                        field_ptr = &field;
                        num_fields++;
                    }
                    if (num_asserts > 0) {
                        auto msg = "Object comprehension cannot have asserts.";
                        throw StaticError(next.location, msg);
                    }
                    if (num_fields != 1) {
                        auto msg = "Object comprehension can only have one field.";
                        throw StaticError(next.location, msg);
                    }
                    const ObjectField &field = *field_ptr;

                    if (field.hide != ObjectField::INHERIT) {
                        auto msg = "Object comprehensions cannot have hidden fields.";
                        throw StaticError(next.location, msg);
                    }

                    if (field.kind != ObjectField::FIELD_EXPR) {
                        auto msg = "Object comprehensions can only have [e] fields.";
                        throw StaticError(next.location, msg);
                    }

                    std::vector<ComprehensionSpec> specs;
                    Token last = parseComprehensionSpecs(Token::BRACE_R, specs);
                    obj = alloc->make<ObjectComprehension>(
                        span(tok, last), fields, got_comma, specs);

                    return last;
                }

                if (!got_comma && !first)
                    throw StaticError(next.location, "Expected a comma before next field.");

                first = false;

                switch (next.kind) {
                    case Token::BRACKET_L: case Token::IDENTIFIER: case Token::STRING_DOUBLE: {
                        ObjectField::Kind kind;
                        AST *expr1 = nullptr;
                        const Identifier *id = nullptr;
                        if (next.kind == Token::IDENTIFIER) {
                            kind = ObjectField::FIELD_ID;
                            id = alloc->makeIdentifier(next.data32());
                        } else if (next.kind == Token::STRING_DOUBLE) {
                            kind = ObjectField::FIELD_STR;
                            expr1 = alloc->make<LiteralString>(
                                next.location, next.data32(), LiteralString::DOUBLE, "");
                        } else {
                            kind = ObjectField::FIELD_EXPR;
                            expr1 = parse(MAX_PRECEDENCE);
                            popExpect(Token::BRACKET_R);
                        }

                        bool is_method = false;
                        bool meth_comma = false;
                        Identifiers params;
                        if (peek().kind == Token::PAREN_L) {
                            pop();
                            params = parseIdentifierList("method parameter", meth_comma);
                            is_method = true;
                        }

                        bool plus_sugar = false;
                        LocationRange plus_loc;
                        if (peek().kind == Token::OPERATOR && peek().data == "+") {
                            plus_loc = peek().location;
                            plus_sugar = true;
                            pop();
                        }

                        if (is_method && plus_sugar) {
                            throw StaticError(
                                next.location,
                                "Cannot use +: syntax sugar in a method: " + next.data);
                        }

                        popExpect(Token::COLON);
                        ObjectField::Hide field_hide = ObjectField::INHERIT;
                        if (peek().kind == Token::COLON) {
                            pop();
                            field_hide = ObjectField::HIDDEN;
                            if (peek().kind == Token::COLON) {
                                pop();
                                field_hide = ObjectField::VISIBLE;
                            }
                        }
                        if (kind != ObjectField::FIELD_EXPR) {
                            if (!literal_fields.insert(next.data).second) {
                                throw StaticError(next.location, "Duplicate field: "+next.data);
                            }
                        }


                        AST *body = parse(MAX_PRECEDENCE);

                        fields.emplace_back(kind, field_hide, plus_sugar, is_method,
                                            expr1, id, params, meth_comma, body, nullptr);

                    }
                    break;

                    case Token::LOCAL: {
                        Token var_id = popExpect(Token::IDENTIFIER);
                        auto *id = alloc->makeIdentifier(var_id.data32());

                        if (binds.find(id) != binds.end()) {
                            throw StaticError(var_id.location,
                                              "Duplicate local var: " + var_id.data);
                        }
                        bool is_method = false;
                        bool func_comma = false;
                        Identifiers params;
                        if (peek().kind == Token::PAREN_L) {
                            pop();
                            is_method = true;
                            params = parseIdentifierList("function parameter", func_comma);
                        }
                        popExpect(Token::OPERATOR, "=");
                        AST *body = parse(MAX_PRECEDENCE);
                        binds.insert(id);

                        fields.emplace_back(
                            ObjectField::Local(is_method, id, params, func_comma, body));

                    }
                    break;

                    case Token::ASSERT: {
                        AST *cond = parse(MAX_PRECEDENCE);
                        AST *msg = nullptr;
                        if (peek().kind == Token::COLON) {
                            pop();
                            msg = parse(MAX_PRECEDENCE);
                        }
                        fields.push_back(ObjectField::Assert(cond, msg));
                    }
                    break;

                    default:
                    throw unexpected(next, "parsing field definition");
                }

                got_comma = false;
            } while (true);
        }

        /** parses for x in expr for y in expr if expr for z in expr ... */
        Token parseComprehensionSpecs(Token::Kind end,
                                      std::vector<ComprehensionSpec> &specs)
        {
            while (true) {
                LocationRange l;
                Token id_token = popExpect(Token::IDENTIFIER);
                const Identifier *id = alloc->makeIdentifier(id_token.data32());
                popExpect(Token::IN);
                AST *arr = parse(MAX_PRECEDENCE);
                specs.emplace_back(ComprehensionSpec::FOR, id, arr);

                Token maybe_if = pop();
                for (; maybe_if.kind == Token::IF; maybe_if = pop()) {
                    AST *cond = parse(MAX_PRECEDENCE);
                    specs.emplace_back(ComprehensionSpec::IF, nullptr, cond);
                }
                if (maybe_if.kind == end) {
                    return maybe_if;
                }
                if (maybe_if.kind != Token::FOR) {
                    std::stringstream ss;
                    ss << "Expected for, if or " << end << " after for clause, got: " << maybe_if;
                    throw StaticError(maybe_if.location, ss.str());
                }
            } 
        }

        AST *parseTerminal(void)
        {
            Token tok = pop();
            switch (tok.kind) {
                case Token::ASSERT:
                case Token::BRACE_R:
                case Token::BRACKET_R:
                case Token::COLON:
                case Token::COMMA:
                case Token::DOT:
                case Token::ELSE:
                case Token::ERROR:
                case Token::FOR:
                case Token::FUNCTION:
                case Token::IF:
                case Token::IN:
                case Token::IMPORT:
                case Token::IMPORTSTR:
                case Token::LOCAL:
                case Token::OPERATOR:
                case Token::PAREN_R:
                case Token::SEMICOLON:
                case Token::TAILSTRICT:
                case Token::THEN:
                throw unexpected(tok, "parsing terminal");

                case Token::END_OF_FILE:
                throw StaticError(tok.location, "Unexpected end of file.");

                case Token::BRACE_L: {
                    AST *obj;
                    parseObjectRemainder(obj, tok);
                    return obj;
                }

                case Token::BRACKET_L: {
                    Token next = peek();
                    if (next.kind == Token::BRACKET_R) {
                        pop();
                        return alloc->make<Array>(span(tok, next), std::vector<AST*>{}, false);
                    }
                    AST *first = parse(MAX_PRECEDENCE);
                    bool got_comma = false;
                    next = peek();
                    if (!got_comma && next.kind == Token::COMMA) {
                        pop();
                        next = peek();
                        got_comma = true;
                    }
 
                    if (next.kind == Token::FOR) {
                        // It's a comprehension
                        pop();
                        std::vector<ComprehensionSpec> specs;
                        Token last = parseComprehensionSpecs(Token::BRACKET_R, specs);
                        return alloc->make<ArrayComprehension>(
                            span(tok, last), first, got_comma, specs);
                    }

                    // Not a comprehension: It can have more elements.
                    std::vector<AST*> elements;
                    elements.push_back(first);
                    do {
                        if (next.kind == Token::BRACKET_R) {
                            // TODO(dcunnin): SYNTAX SUGAR HERE (preserve comma)
                            pop();
                            break;
                        }
                        if (!got_comma) {
                            std::stringstream ss;
                            ss << "Expected a comma before next array element.";
                            throw StaticError(next.location, ss.str());
                        }
                        elements.push_back(parse(MAX_PRECEDENCE));
                        next = peek();
                        if (next.kind == Token::COMMA) {
                            pop();
                            next = peek();
                            got_comma = true;
                        }
                    } while (true);
                    return alloc->make<Array>(span(tok, next), elements, got_comma);
                    
                }

                case Token::PAREN_L: {
                    auto *inner = parse(MAX_PRECEDENCE);
                    popExpect(Token::PAREN_R);
                    return inner;
                }


                // Literals
                case Token::NUMBER:
                return alloc->make<LiteralNumber>(span(tok), tok.data);

                case Token::STRING_SINGLE:
                return alloc->make<LiteralString>(
                    span(tok), tok.data32(), LiteralString::SINGLE, "");
                case Token::STRING_DOUBLE:
                return alloc->make<LiteralString>(
                    span(tok), tok.data32(), LiteralString::DOUBLE, "");
                case Token::STRING_BLOCK:
                return alloc->make<LiteralString>(
                    span(tok), tok.data32(), LiteralString::BLOCK, tok.stringBlockIndent);

                case Token::FALSE:
                return alloc->make<LiteralBoolean>(span(tok), false);

                case Token::TRUE:
                return alloc->make<LiteralBoolean>(span(tok), true);

                case Token::NULL_LIT:
                return alloc->make<LiteralNull>(span(tok));

                // Variables
                case Token::DOLLAR:
                return alloc->make<Dollar>(span(tok));

                case Token::IDENTIFIER:
                return alloc->make<Var>(span(tok), alloc->makeIdentifier(tok.data32()));

                case Token::SELF:
                return alloc->make<Self>(span(tok));

                case Token::SUPER: {
                    Token next = pop();
                    AST *index = nullptr;
                    const Identifier *id = nullptr;
                    switch (next.kind) {
                        case Token::DOT: {
                            Token field_id = popExpect(Token::IDENTIFIER);
                            id = alloc->makeIdentifier(field_id.data32());
                        } break;
                        case Token::BRACKET_L: {
                            index = parse(MAX_PRECEDENCE);
                            popExpect(Token::BRACKET_R);
                        } break;
                        default:
                        throw StaticError(tok.location, "Expected . or [ after super.");
                    }
                    return alloc->make<SuperIndex>(span(tok), index, id);
                }
            }

            std::cerr << "INTERNAL ERROR: Unknown tok kind: " << tok.kind << std::endl;
            std::abort();
            return nullptr;  // Quiet, compiler.
        }

        AST *parse(int precedence)
        {
            Token begin = peek();

            switch (begin.kind) {

                // These cases have effectively MAX_PRECEDENCE as the first
                // call to parse will parse them.
                case Token::ASSERT: {
                    pop();
                    AST *cond = parse(MAX_PRECEDENCE);
                    AST *msg = nullptr;
                    if (peek().kind == Token::COLON) {
                        pop();
                        msg = parse(MAX_PRECEDENCE);
                    }
                    popExpect(Token::SEMICOLON);
                    AST *rest = parse(MAX_PRECEDENCE);
                    return alloc->make<Assert>(span(begin, rest), cond, msg, rest);
                }

                case Token::ERROR: {
                    pop();
                    AST *expr = parse(MAX_PRECEDENCE);
                    return alloc->make<Error>(span(begin, expr), expr);
                }

                case Token::IF: {
                    pop();
                    AST *cond = parse(MAX_PRECEDENCE);
                    popExpect(Token::THEN);
                    AST *branch_true = parse(MAX_PRECEDENCE);
                    AST *branch_false = nullptr;
                    LocationRange lr = span(begin, branch_true);
                    if (peek().kind == Token::ELSE) {
                        pop();
                        branch_false = parse(MAX_PRECEDENCE);
                        lr = span(begin, branch_false);
                    }
                    return alloc->make<Conditional>(lr, cond, branch_true, branch_false);
                }

                case Token::FUNCTION: {
                    pop();
                    Token next = pop();
                    if (next.kind == Token::PAREN_L) {
                        std::vector<AST*> params_asts;
                        bool got_comma;
                        Identifiers params = parseIdentifierList(
                            "function parameter", got_comma);
                        AST *body = parse(MAX_PRECEDENCE);
                        return alloc->make<Function>(span(begin, body), params, got_comma, body);
                    } else {
                        std::stringstream ss;
                        ss << "Expected ( but got " << next;
                        throw StaticError(next.location, ss.str());
                    }
                }

                case Token::IMPORT: {
                    pop();
                    AST *body = parse(MAX_PRECEDENCE);
                    if (auto *lit = dynamic_cast<LiteralString*>(body)) {
                        return alloc->make<Import>(span(begin, body), lit->value);
                    } else {
                        std::stringstream ss;
                        ss << "Computed imports are not allowed.";
                        throw StaticError(body->location, ss.str());
                    }
                }

                case Token::IMPORTSTR: {
                    pop();
                    AST *body = parse(MAX_PRECEDENCE);
                    if (auto *lit = dynamic_cast<LiteralString*>(body)) {
                        return alloc->make<Importstr>(span(begin, body), lit->value);
                    } else {
                        std::stringstream ss;
                        ss << "Computed imports are not allowed.";
                        throw StaticError(body->location, ss.str());
                    }
                }

                case Token::LOCAL: {
                    pop();
                    Local::Binds binds;
                    do {
                        parseBind(binds);
                        Token delim = pop();
                        if (delim.kind != Token::SEMICOLON && delim.kind != Token::COMMA) {
                            std::stringstream ss;
                            ss << "Expected , or ; but got " << delim;
                            throw StaticError(delim.location, ss.str());
                        }
                        if (delim.kind == Token::SEMICOLON) break;
                    } while (true);
                    AST *body = parse(MAX_PRECEDENCE);
                    return alloc->make<Local>(span(begin, body), binds, body);
                }

                default:

                // Unary operator.
                if (begin.kind == Token::OPERATOR) {
                    UnaryOp uop;
                    if (!op_is_unary(begin.data, uop)) {
                        std::stringstream ss;
                        ss << "Not a unary operator: " << begin.data;
                        throw StaticError(begin.location, ss.str());
                    }
                    if (UNARY_PRECEDENCE == precedence) {
                        Token op = pop();
                        AST *expr = parse(precedence);
                        return alloc->make<Unary>(span(op, expr), uop, expr);
                    }
                }

                // Base case
                if (precedence == 0) return parseTerminal();

                AST *lhs = parse(precedence - 1);

                while (true) {

                    // Then next token must be a binary operator.

                    // The compiler can't figure out that this is never used uninitialized.
                    BinaryOp bop = BOP_PLUS;

                    // Check precedence is correct for this level.  If we're
                    // parsing operators with higher precedence, then return
                    // lhs and let lower levels deal with the operator.
                    switch (peek().kind) {
                        // Logical / arithmetic binary operator.
                        case Token::OPERATOR:
                        if (!op_is_binary(peek().data, bop)) {
                            std::stringstream ss;
                            ss << "Not a binary operator: " << peek().data;
                            throw StaticError(peek().location, ss.str());
                        }
                        if (precedence_map[bop] != precedence) return lhs;
                        break;

                        // Index, Apply
                        case Token::DOT: case Token::BRACKET_L:
                        case Token::PAREN_L: case Token::BRACE_L:
                        if (APPLY_PRECEDENCE != precedence) return lhs;
                        break;

                        default:
                        return lhs;
                    }

                    Token op = pop();
                    if (op.kind == Token::BRACKET_L) {
                        AST *index = parse(MAX_PRECEDENCE);
                        Token end = popExpect(Token::BRACKET_R);
                        lhs = alloc->make<Index>(span(begin, end), lhs, index, nullptr);

                    } else if (op.kind == Token::DOT) {
                        Token field_id = popExpect(Token::IDENTIFIER);
                        const Identifier *id = alloc->makeIdentifier(field_id.data32());
                        lhs = alloc->make<Index>(span(begin, field_id), lhs, nullptr, id);

                    } else if (op.kind == Token::PAREN_L) {
                        std::vector<AST*> args;
                        bool got_comma;
                        Token end = parseCommaList(args, Token::PAREN_R,
                                                   "function argument", got_comma);
                        bool tailstrict = false;
                        if (peek().kind == Token::TAILSTRICT) {
                            pop();
                            tailstrict = true;
                        }
                        lhs = alloc->make<Apply>(span(begin, end), lhs, args, got_comma,
                                                 tailstrict);

                    } else if (op.kind == Token::BRACE_L) {
                        AST *obj;
                        Token end = parseObjectRemainder(obj, op);
                        lhs = alloc->make<ApplyBrace>(span(begin, end), lhs, obj);

                    } else {
                        // Logical / arithmetic binary operator.
                        AST *rhs = parse(precedence - 1);
                        lhs = alloc->make<Binary>(span(begin, rhs), lhs, bop, rhs);
                    }
                }
            }
        }

    };
}

AST *jsonnet_parse(Allocator *alloc, Tokens &tokens)
{
    Parser parser(tokens, alloc);
    AST *expr = parser.parse(MAX_PRECEDENCE);
    if (tokens.front().kind != Token::END_OF_FILE) {
        std::stringstream ss;
        ss << "Did not expect: " << tokens.front();
        throw StaticError(tokens.front().location, ss.str());
    }

    return expr;
}
