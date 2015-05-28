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


#include "static_error.h"
#include "ast.h"
#include "parser.h"
#include "lexer.h"


namespace {

    // Cache some immutable stuff in global variables.
    const int APPLY_PRECEDENCE = 2;  // Function calls and indexing.
    const int UNARY_PRECEDENCE = 4;  // Logical and bitwise negation, unary + -
    const int PERCENT_PRECEDENCE = 5; // Modulo and string formatting
    const int MAX_PRECEDENCE = 15; // Local, If, Import, Function, Error

    /** These are the binary operator precedences, unary precedence is given by
     * UNARY_PRECEDENCE.
     */
    std::map<BinaryOp, int> build_precedence_map(void)
    {
        std::map<BinaryOp, int> r;

        r[BOP_MULT] = 5;
        r[BOP_DIV] = 5;

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
            tokens->pop_front();
            return tok;
        }

        Token peek(void)
        {
            Token tok = tokens->front();
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

        std::list<Token> *tokens;
        Allocator *alloc;

        public:

        Parser(std::list<Token> *tokens, Allocator *alloc)
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
                             unsigned obj_level)
        {
            bool got_comma = true;
            do {
                Token next = peek();
                if (!got_comma) {
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
                if (!got_comma) {
                    std::stringstream ss;
                    ss << "Expected a comma before next " << element_kind <<  ".";
                    throw StaticError(next.location, ss.str());
                }
                exprs.push_back(parse(MAX_PRECEDENCE, obj_level));
                got_comma = false;
            } while (true);
        }

        std::vector<const Identifier*> parseIdentifierList(const std::string &element_kind,
                                                           unsigned obj_level)
        {
            std::vector<AST*> exprs;
            parseCommaList(exprs, Token::PAREN_R, element_kind, obj_level);

            // Check they're all identifiers
            std::vector<const Identifier*> ret;
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

        void parseBind(Local::Binds &binds, unsigned obj_level)
        {
            Token var_id = popExpect(Token::IDENTIFIER);
            auto *id = alloc->makeIdentifier(var_id.data);
            if (binds.find(id) != binds.end()) {
                throw StaticError(var_id.location,
                                  "Duplicate local var: " + var_id.data);
            }
            AST *init;
            if (peek().kind == Token::PAREN_L) {
                pop();
                auto params = parseIdentifierList("function parameter", obj_level);
                popExpect(Token::OPERATOR, "=");
                AST *body = parse(MAX_PRECEDENCE, obj_level);
                init = alloc->make<Function>(span(var_id, body), params, body);
            } else {
                popExpect(Token::OPERATOR, "=");
                init = parse(MAX_PRECEDENCE, obj_level);
            }
            binds[id] = init;
        }


        Token parseObjectRemainder(AST *&obj, const Token &tok, unsigned obj_level)
        {
            std::set<std::string> literal_fields;
            Object::Fields fields;
            std::map<const Identifier*, AST*> let_binds;

            // Hidden variable to allow outer/top binding.
            if (obj_level == 0) {
                const Identifier *hidden_var = alloc->makeIdentifier("$");
                let_binds[hidden_var] = alloc->make<Self>(LocationRange());
            }

            bool got_comma = true;

            // This is used to prevent { [x]: x, local foo = 3 for x in [1,2,3] }
            bool last_was_local = false;
            do {

                Token next = pop();
                if (!got_comma) {
                    if (next.kind == Token::COMMA) {
                        next = pop();
                        got_comma = true;
                    }
                }
                if (next.kind == Token::BRACE_R) {
                    // got_comma can be true or false here.
                    Object::Fields r;
                    if (let_binds.size() == 0) {
                        r = fields;
                    } else {
                        for (const auto &f : fields) {
                            AST *body = alloc->make<Local>(f.body->location, let_binds, f.body);
                            r.emplace_back(f.name, f.hide, body);
                        }
                    }
                    obj = alloc->make<Object>(span(tok, next), r);
                    return next;
                } else if (next.kind == Token::FOR) {
                    if (fields.size() != 1) {
                        auto msg = "Object composition can only have one field/value pair.";
                        throw StaticError(next.location, msg);
                    }
                    if (last_was_local) {
                        auto msg = "Locals must appear first in an object comprehension.";
                        throw StaticError(next.location, msg);
                    }
                    AST *field = fields.front().name;
                    Object::Field::Hide field_hide = fields.front().hide;
                    AST *value = fields.front().body;
                    if (let_binds.size() > 0) {
                        value = alloc->make<Local>(value->location, let_binds, value);
                    }
                    if (field_hide != Object::Field::INHERIT) {
                        auto msg = "Object comprehensions cannot have hidden fields.";
                        throw StaticError(next.location, msg);
                    }
                    if (got_comma) {
                        throw StaticError(next.location, "Unexpected comma before for.");
                    }
                    Token id_tok = popExpect(Token::IDENTIFIER);
                    const Identifier *id = alloc->makeIdentifier(id_tok.data);
                    popExpect(Token::IN);
                    AST *array = parse(MAX_PRECEDENCE, obj_level);
                    Token last = popExpect(Token::BRACE_R);
                    obj = alloc->make<ObjectComposition>(span(tok, last), field, value, id, array);
                    return last;
                }
                if (!got_comma)
                    throw StaticError(next.location, "Expected a comma before next field.");

                switch (next.kind) {
                    case Token::IDENTIFIER: case Token::STRING: {
                        last_was_local = false;
                        bool is_method = false;
                        std::vector<const Identifier *> params;
                        if (peek().kind == Token::PAREN_L) {
                            pop();
                            params = parseIdentifierList("method parameter", obj_level);
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
                            throw StaticError(next.location, "Cannot use +: syntax sugar in a method: "+next.data);
                        }

                        popExpect(Token::COLON);
                        Object::Field::Hide field_hide = Object::Field::INHERIT;
                        if (peek().kind == Token::COLON) {
                            pop();
                            field_hide = Object::Field::HIDDEN;
                            if (peek().kind == Token::COLON) {
                                pop();
                                field_hide = Object::Field::VISIBLE;
                            }
                        }
                        if (!literal_fields.insert(next.data).second) {
                            throw StaticError(next.location, "Duplicate field: "+next.data);
                        }
                        AST *field_expr = alloc->make<LiteralString>(next.location, next.data);

                        AST *body = parse(MAX_PRECEDENCE, obj_level+1);
                        if (is_method) {
                            body = alloc->make<Function>(body->location, params, body);
                        }
                        if (plus_sugar) {
                            AST *f = alloc->make<LiteralString>(plus_loc, next.data);
                            AST *super_f = alloc->make<Index>(plus_loc, alloc->make<Super>(LocationRange()), f);
                            body = alloc->make<Binary>(body->location, super_f, BOP_PLUS, body);
                        }
                        fields.emplace_back(field_expr, field_hide, body);
                    }
                    break;

                    case Token::LOCAL: {
                        last_was_local = true;
                        parseBind(let_binds, obj_level);
                    }
                    break;

                    case Token::BRACKET_L: {
                        last_was_local = false;
                        AST *field_expr = parse(MAX_PRECEDENCE, obj_level);
                        popExpect(Token::BRACKET_R);
                        popExpect(Token::COLON);
                        Object::Field::Hide field_hide = Object::Field::INHERIT;
                        if (peek().kind == Token::COLON) {
                            pop();
                            field_hide = Object::Field::HIDDEN;
                            if (peek().kind == Token::COLON) {
                                pop();
                                field_hide = Object::Field::VISIBLE;
                            }
                        }
                        fields.emplace_back(field_expr, field_hide,
                                            parse(MAX_PRECEDENCE, obj_level+1));
                    }
                    break;

                    default:
                    throw unexpected(next, "parsing field definition");
                }

                got_comma = false;
            } while (true);
        }

        AST *parseTerminal(unsigned obj_level)
        {
            Token tok = pop();
            switch (tok.kind) {
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
                    parseObjectRemainder(obj, tok, obj_level);
                    return obj;
                }

                case Token::BRACKET_L: {
                    Token next = peek();
                    if (next.kind == Token::BRACKET_R) {
                        pop();
                        return alloc->make<Array>(span(tok, next), std::vector<AST*>{});
                    }
                    AST *first = parse(MAX_PRECEDENCE, obj_level);
                    next = peek();
                    if (next.kind == Token::FOR) {
                        LocationRange l;
                        pop();
                        Token id_token = popExpect(Token::IDENTIFIER);
                        const Identifier *id = alloc->makeIdentifier(id_token.data);
                        std::vector<const Identifier*> params = {id};
                        AST *std = alloc->make<Var>(l, alloc->makeIdentifier("std"));
                        AST *map_func = alloc->make<Function>(first->location, params, first);
                        popExpect(Token::IN);
                        AST *arr = parse(MAX_PRECEDENCE, obj_level);
                        Token maybe_if = pop();
                        if (maybe_if.kind == Token::BRACKET_R) {
                            AST *map_str = alloc->make<LiteralString>(l, "map");
                            AST *map = alloc->make<Index>(l, std, map_str);
                            std::vector<AST*> args = {map_func, arr};
                            return alloc->make<Apply>(span(tok, maybe_if), map, args, false);
                        } else if (maybe_if.kind == Token::IF) {
                            AST *cond = parse(MAX_PRECEDENCE, obj_level);
                            Token last = popExpect(Token::BRACKET_R);
                            AST *filter_func = alloc->make<Function>(cond->location, params, cond);
                            AST *fmap_str = alloc->make<LiteralString>(l, "filterMap");
                            AST *fmap = alloc->make<Index>(l, std, fmap_str);
                            std::vector<AST*> args = {filter_func, map_func, arr};
                            return alloc->make<Apply>(span(tok, last), fmap, args, false);
                        } else {
                            std::stringstream ss;
                            ss << "Expected if or ] after for clause, got: " << maybe_if;
                            throw StaticError(next.location, ss.str());
                        }
                    } else {
                        std::vector<AST*> elements;
                        elements.push_back(first);
                        do {
                            next = peek();
                            bool got_comma = false;
                            if (next.kind == Token::COMMA) {
                                pop();
                                next = peek();
                                got_comma = true;
                            }
                            if (next.kind == Token::BRACKET_R) {
                                pop();
                                break;
                            }
                            if (!got_comma) {
                                std::stringstream ss;
                                ss << "Expected a comma before next array element.";
                                throw StaticError(next.location, ss.str());
                            }
                            elements.push_back(parse(MAX_PRECEDENCE, obj_level));
                        } while (true);
                        return alloc->make<Array>(span(tok, next), elements);
                    }
                }

                case Token::PAREN_L: {
                    auto *inner = parse(MAX_PRECEDENCE, obj_level);
                    popExpect(Token::PAREN_R);
                    return inner;
                }


                // Literals
                case Token::NUMBER:
                return alloc->make<LiteralNumber>(span(tok), strtod(tok.data.c_str(), nullptr));

                case Token::STRING:
                return alloc->make<LiteralString>(span(tok), tok.data);

                case Token::FALSE:
                return alloc->make<LiteralBoolean>(span(tok), false);

                case Token::TRUE:
                return alloc->make<LiteralBoolean>(span(tok), true);

                case Token::NULL_LIT:
                return alloc->make<LiteralNull>(span(tok));

                // Import
                case Token::IMPORT: {
                    Token file = popExpect(Token::STRING);
                    return alloc->make<Import>(span(tok, file), file.data);
                }

                case Token::IMPORTSTR: {
                    Token file = popExpect(Token::STRING);
                    return alloc->make<Importstr>(span(tok, file), file.data);
                }


                // Variables
                case Token::DOLLAR:
                if (obj_level == 0) {
                    throw StaticError(tok.location, "No top-level object found.");
                }
                return alloc->make<Var>(span(tok), alloc->makeIdentifier("$"));

                case Token::IDENTIFIER:
                return alloc->make<Var>(span(tok), alloc->makeIdentifier(tok.data));

                case Token::SELF:
                return alloc->make<Self>(span(tok));

                case Token::SUPER:
                return alloc->make<Super>(span(tok));
            }

            std::cerr << "INTERNAL ERROR: Unknown tok kind: " << tok.kind << std::endl;
            std::abort();
            return nullptr;  // Quiet, compiler.
        }

        AST *parse(int precedence, unsigned obj_level)
        {
            Token begin = peek();

            switch (begin.kind) {

                // These cases have effectively MAX_PRECEDENCE as the first
                // call to parse will parse them.
                case Token::ERROR: {
                    pop();
                    AST *expr = parse(MAX_PRECEDENCE, obj_level);
                    return alloc->make<Error>(span(begin, expr), expr);
                }

                case Token::IF: {
                    pop();
                    AST *cond = parse(MAX_PRECEDENCE, obj_level);
                    popExpect(Token::THEN);
                    AST *branch_true = parse(MAX_PRECEDENCE, obj_level);
                    AST *branch_false;
                    if (peek().kind == Token::ELSE) {
                        pop();
                        branch_false = parse(MAX_PRECEDENCE, obj_level);
                    } else {
                        branch_false = alloc->make<LiteralNull>(span(begin, branch_true));
                    }
                    return alloc->make<Conditional>(span(begin, branch_false),
                                                   cond, branch_true, branch_false);
                }

                case Token::FUNCTION: {
                    pop();
                    Token next = pop();
                    if (next.kind == Token::PAREN_L) {
                        std::vector<AST*> params_asts;
                        parseCommaList(params_asts, Token::PAREN_R,
                                       "function parameter", obj_level);
                        AST *body = parse(MAX_PRECEDENCE, obj_level);
                        std::vector<const Identifier*> params;
                        for (AST *p_ast : params_asts) {
                            auto *p = dynamic_cast<Var*>(p_ast);
                            if (p == nullptr) {
                                std::stringstream ss;
                                ss << "Not an identifier: " << p_ast;
                                throw StaticError(p_ast->location, ss.str());
                            }
                            params.push_back(p->id);
                        }
                        return alloc->make<Function>(span(begin, body), params, body);
                    } else {
                        std::stringstream ss;
                        ss << "Expected ( but got " << next;
                        throw StaticError(next.location, ss.str());
                    }
                }

                case Token::LOCAL: {
                    pop();
                    Local::Binds binds;
                    do {
                        parseBind(binds, obj_level);
                        Token delim = pop();
                        if (delim.kind != Token::SEMICOLON && delim.kind != Token::COMMA) {
                            std::stringstream ss;
                            ss << "Expected , or ; but got " << delim;
                            throw StaticError(delim.location, ss.str());
                        }
                        if (delim.kind == Token::SEMICOLON) break;
                    } while (true);
                    AST *body = parse(MAX_PRECEDENCE, obj_level);
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
                        AST *expr = parse(precedence, obj_level);
                        return alloc->make<Unary>(span(op, expr), uop, expr);
                    }
                }

                // Base case
                if (precedence == 0) return parseTerminal(obj_level);

                AST *lhs = parse(precedence - 1, obj_level);

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
                        if (peek().data == "%") {
                            if (PERCENT_PRECEDENCE != precedence) return lhs;
                        } else {
                            if (!op_is_binary(peek().data, bop)) {
                                std::stringstream ss;
                                ss << "Not a binary operator: " << peek().data;
                                throw StaticError(peek().location, ss.str());
                            }
                            if (precedence_map[bop] != precedence) return lhs;
                        }
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
                        AST *index = parse(MAX_PRECEDENCE, obj_level);
                        Token end = popExpect(Token::BRACKET_R);
                        lhs = alloc->make<Index>(span(begin, end), lhs, index);

                    } else if (op.kind == Token::DOT) {
                        Token field = popExpect(Token::IDENTIFIER);
                        AST *index = alloc->make<LiteralString>(span(field), field.data);
                        lhs = alloc->make<Index>(span(begin, field), lhs, index);

                    } else if (op.kind == Token::PAREN_L) {
                        std::vector<AST*> args;
                        Token end = parseCommaList(args, Token::PAREN_R,
                                                   "function argument", obj_level);
                        bool tailstrict = false;
                        if (peek().kind == Token::TAILSTRICT) {
                            pop();
                            tailstrict = true;
                        }
                        lhs = alloc->make<Apply>(span(begin, end), lhs, args, tailstrict);

                    } else if (op.kind == Token::BRACE_L) {
                        AST *obj;
                        Token end = parseObjectRemainder(obj, op, obj_level);
                        lhs = alloc->make<Binary>(span(begin, end), lhs, BOP_PLUS, obj);

                    } else if (op.data == "%") {
                        AST *rhs = parse(precedence - 1, obj_level);
                        LocationRange l;
                        AST *std = alloc->make<Var>(l, alloc->makeIdentifier("std"));
                        AST *mod_str = alloc->make<LiteralString>(l, "mod");
                        AST *f_mod = alloc->make<Index>(l, std, mod_str);
                        std::vector<AST*> args = {lhs, rhs};
                        lhs = alloc->make<Apply>(span(begin, rhs), f_mod, args, false);

                    } else {
                        // Logical / arithmetic binary operator.
                        AST *rhs = parse(precedence - 1, obj_level);
                        bool invert = false;
                        if (bop == BOP_MANIFEST_UNEQUAL) {
                            bop = BOP_MANIFEST_EQUAL;
                            invert = true;
                        }
                        lhs = alloc->make<Binary>(span(begin, rhs), lhs, bop, rhs);
                        if (invert) {
                            lhs = alloc->make<Unary>(lhs->location, UOP_NOT, lhs);
                        }
                    }
                }
            }
        }

    };
}

static unsigned long max_builtin = 23;
BuiltinDecl jsonnet_builtin_decl(unsigned long builtin)
{
    switch (builtin) {
        case 0: return {"makeArray", {"sz", "func"}};
        case 1: return {"pow", {"x", "n"}};
        case 2: return {"floor", {"x"}};
        case 3: return {"ceil", {"x"}};
        case 4: return {"sqrt", {"x"}};
        case 5: return {"sin", {"x"}};
        case 6: return {"cos", {"x"}};
        case 7: return {"tan", {"x"}};
        case 8: return {"asin", {"x"}};
        case 9: return {"acos", {"x"}};
        case 10: return {"atan", {"x"}};
        case 11: return {"type", {"x"}};
        case 12: return {"filter", {"func", "arr"}};
        case 13: return {"objectHas", {"obj", "f"}};
        case 14: return {"length", {"x"}};
        case 15: return {"objectFields", {"obj"}};
        case 16: return {"codepoint", {"str"}};
        case 17: return {"char", {"n"}};
        case 18: return {"log", {"n"}};
        case 19: return {"exp", {"n"}};
        case 20: return {"mantissa", {"n"}};
        case 21: return {"exponent", {"n"}};
        case 22: return {"modulo", {"a", "b"}};
        case 23: return {"extVar", {"x"}};
        default:
        std::cerr << "INTERNAL ERROR: Unrecognized builtin function: " << builtin << std::endl;
        std::abort();
    }
    // Quiet, compiler.
    return BuiltinDecl();
}

static AST *do_parse(Allocator *alloc, const std::string &file, const char *input)
{
    // Lex the input.
    auto token_list = jsonnet_lex(file, input);

    // Parse the input.
    Parser parser(&token_list, alloc);
    AST *expr = parser.parse(MAX_PRECEDENCE, 0);
    if (token_list.front().kind != Token::END_OF_FILE) {
        std::stringstream ss;
        ss << "Did not expect: " << token_list.front();
        throw StaticError(token_list.front().location, ss.str());
    }

    return expr;
}

static constexpr char STD_CODE[] = {
    #include "std.jsonnet.h"
};

AST *jsonnet_parse(Allocator *alloc, const std::string &file, const char *input)
{
    // Parse the actual file.
    AST *expr = do_parse(alloc, file, input);

    // Now, implement the std library by wrapping in a local construct.
    auto *std_obj = static_cast<Object*>(do_parse(alloc, "std.jsonnet", STD_CODE));

    // For generated ASTs, use a bogus location.
    const LocationRange l;

    // Bind 'std' builtins that are implemented natively.
    Object::Fields &fields = std_obj->fields;
    for (unsigned long c=0 ; c <= max_builtin ; ++c) {
        const auto &decl = jsonnet_builtin_decl(c);
        std::vector<const Identifier*> params;
        for (const auto &p : decl.params)
            params.push_back(alloc->makeIdentifier(p));
        fields.emplace_back(alloc->make<LiteralString>(l, decl.name), Object::Field::HIDDEN,
                            alloc->make<BuiltinFunction>(l, c, params));
    }

    Local::Binds std_binds;
    std_binds[alloc->makeIdentifier("std")] = std_obj;
    AST *wrapped = alloc->make<Local>(expr->location, std_binds, expr);
    return wrapped;
}

std::string jsonnet_unparse_escape(const std::string &str)
{
    std::stringstream ss;
    ss << '\"';
    for (std::size_t i=0 ; i<str.length() ; ++i) {
        char c = str[i];
        switch (c) {
            case '"': ss << "\\\""; break;
            case '\\': ss << "\\\\"; break;
            case '\b': ss << "\\b"; break;
            case '\f': ss << "\\f"; break;
            case '\n': ss << "\\n"; break;
            case '\r': ss << "\\r"; break;
            case '\t': ss << "\\t"; break;
            case '\0': ss << "\\u0000"; break;
            default: {
                if (c < 0x20 || c > 0x7e) {
                    //Unprintable, use \u
                    ss << "\\u" << std::hex << std::setfill('0') << std::setw(4)
                       << unsigned((unsigned char)(c));
                } else {
                    // Printable, write verbatim
                    ss << c;
                }
            }
        }
    }
    ss << '\"';
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

static std::string unparse(const AST *ast_)
{
    std::stringstream ss;
    if (auto *ast = dynamic_cast<const Apply*>(ast_)) {
        ss << unparse(ast->target);
        if (ast->arguments.size() == 0) {
            ss << "()";
        } else {
            const char *prefix = "(";
            for (auto arg : ast->arguments) {
                ss << prefix << unparse(arg);
                prefix = ", ";
            }
            ss << ")";
        }

    } else if (auto *ast = dynamic_cast<const Array*>(ast_)) {
        if (ast->elements.size() == 0) {
            ss << "[ ]";
        } else {
            const char *prefix = "[";
            for (const auto &element : ast->elements) {
                ss << prefix;
                ss << unparse(element);
                prefix = ", ";
            }
            ss << "]";
        }

    } else if (auto *ast = dynamic_cast<const Binary*>(ast_)) {
        ss << unparse(ast->left) << " " << bop_string(ast->op)
           << " " << unparse(ast->right);

    } else if (dynamic_cast<const BuiltinFunction*>(ast_)) {
        std::cerr << "INTERNAL ERROR: Unparsing builtin function." << std::endl;
        std::abort();

    } else if (auto *ast = dynamic_cast<const Conditional*>(ast_)) {
        ss << "if " << unparse(ast->cond) << " then "
           << unparse(ast->branchTrue) << " else "
           << unparse(ast->branchFalse);

    } else if (auto *ast = dynamic_cast<const Error*>(ast_)) {
        ss << "error " << unparse(ast->expr);

    } else if (auto *ast = dynamic_cast<const Function*>(ast_)) {
        ss << "function ";
        const char *prefix = "(";
        for (const Identifier *arg : ast->parameters) {
            ss << prefix << arg->name;
            prefix = ", ";
        }
        ss << ") " << unparse(ast->body);

    } else if (auto *ast = dynamic_cast<const Import*>(ast_)) {
        ss << "import " << jsonnet_unparse_escape(ast->file);

    } else if (auto *ast = dynamic_cast<const Importstr*>(ast_)) {
        ss << "importstr " << jsonnet_unparse_escape(ast->file);

    } else if (auto *ast = dynamic_cast<const Index*>(ast_)) {
        ss << unparse(ast->target) << "["
           << unparse(ast->index) << "]";

    } else if (auto *ast = dynamic_cast<const Local*>(ast_)) {
        const char *prefix = "local ";
        for (const auto &bind : ast->binds) {
            ss << prefix << bind.first->name << " = " << unparse(bind.second);
            prefix = ", ";
        }
        ss << "; " << unparse(ast->body);

    } else if (auto *ast = dynamic_cast<const LiteralBoolean*>(ast_)) {
        ss << (ast->value ? "true" : "false");

    } else if (auto *ast = dynamic_cast<const LiteralNumber*>(ast_)) {
        ss << ast->value;

    } else if (auto *ast = dynamic_cast<const LiteralString*>(ast_)) {
        ss << jsonnet_unparse_escape(ast->value);

    } else if (dynamic_cast<const LiteralNull*>(ast_)) {
        ss << "null";

    } else if (auto *ast = dynamic_cast<const Object*>(ast_)) {
        if (ast->fields.size() == 0) {
            ss << "{ }";
        } else {
            const char *prefix = "{";
            for (const auto &f : ast->fields) {
                ss << prefix;
                const char *colons = nullptr;
                switch (f.hide) {
                    case Object::Field::INHERIT: colons = ":"; break;
                    case Object::Field::HIDDEN: colons = "::"; break;
                    case Object::Field::VISIBLE: colons = ":::"; break;
                    default:
                    std::cerr << "INTERNAL ERROR: Unknown FieldHide: " << f.hide << std::endl;
                }
                ss << "[" << unparse(f.name) << "]" << colons << " " << unparse(f.body);
                prefix = ", ";
            }
            ss << "}";
        }

    } else if (auto *ast = dynamic_cast<const ObjectComposition*>(ast_)) {
        ss << "{[" << unparse(ast->field) << "]: " << unparse(ast->value);
        ss << " for " << ast->id->name << " in " << unparse(ast->array);
        ss << "}";

    } else if (dynamic_cast<const Self*>(ast_)) {
        ss << "self";

    } else if (dynamic_cast<const Super*>(ast_)) {
        ss << "super";

    } else if (auto *ast = dynamic_cast<const Unary*>(ast_)) {
        ss << uop_string(ast->op) << unparse(ast->expr);

    } else if (auto *ast = dynamic_cast<const Var*>(ast_)) {
        ss << ast->id->name;

    } else {
        std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
        std::abort();

    }

    return "(" + ss.str() + ")";
}

std::string jsonnet_unparse_jsonnet(const AST *ast) 
{
    const auto *wrapper = dynamic_cast<const Local*>(ast);
    if (wrapper == nullptr) {
        std::cerr << "INTERNAL ERROR: Unparsing an AST that wasn't wrapped in a std local."
                  << std::endl;
        std::abort();
    }
    return unparse(wrapper->body);
}
