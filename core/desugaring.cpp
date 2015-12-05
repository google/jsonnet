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

#include <cassert>

#include "ast.h"
#include "lexer.h"
#include "parser.h"

static LocationRange E;  // Empty.
    
static unsigned long max_builtin = 24;
BuiltinDecl jsonnet_builtin_decl(unsigned long builtin)
{
    switch (builtin) {
        case 0: return {U"makeArray", {U"sz", U"func"}};
        case 1: return {U"pow", {U"x", U"n"}};
        case 2: return {U"floor", {U"x"}};
        case 3: return {U"ceil", {U"x"}};
        case 4: return {U"sqrt", {U"x"}};
        case 5: return {U"sin", {U"x"}};
        case 6: return {U"cos", {U"x"}};
        case 7: return {U"tan", {U"x"}};
        case 8: return {U"asin", {U"x"}};
        case 9: return {U"acos", {U"x"}};
        case 10: return {U"atan", {U"x"}};
        case 11: return {U"type", {U"x"}};
        case 12: return {U"filter", {U"func", U"arr"}};
        case 13: return {U"objectHasEx", {U"obj", U"f", U"inc_hidden"}};
        case 14: return {U"length", {U"x"}};
        case 15: return {U"objectFieldsEx", {U"obj", U"inc_hidden"}};
        case 16: return {U"codepoint", {U"str"}};
        case 17: return {U"char", {U"n"}};
        case 18: return {U"log", {U"n"}};
        case 19: return {U"exp", {U"n"}};
        case 20: return {U"mantissa", {U"n"}};
        case 21: return {U"exponent", {U"n"}};
        case 22: return {U"modulo", {U"a", U"b"}};
        case 23: return {U"extVar", {U"x"}};
        case 24: return {U"primitiveEquals", {U"a", U"b"}};
        default:
        std::cerr << "INTERNAL ERROR: Unrecognized builtin function: " << builtin << std::endl;
        std::abort();
    }
    // Quiet, compiler.
    return BuiltinDecl();
}

static constexpr char STD_CODE[] = {
    #include "std.jsonnet.h"
};

static String desugar_string(const LocationRange &loc, const String &s)
{
    String r;
    const char32_t *s_ptr = s.c_str();
    for (const char32_t *c = s_ptr; *c != U'\0' ; ++c) {
        switch (*c) {
            case '\\':
            switch (*(++c)) {
                case '"':
                r += *c;
                break;

                case '\\':
                r += *c;
                break;

                case '/':
                r += *c;
                break;

                case 'b':
                r += '\b';
                break;

                case 'f':
                r += '\f';
                break;

                case 'n':
                r += '\n';
                break;

                case 'r':
                r += '\r';
                break;

                case 't':
                r += '\t';
                break;

                case 'u': {
                    ++c;  // Consume the 'u'.
                    unsigned long codepoint = 0;
                    // Expect 4 hex digits.
                    for (unsigned i=0 ; i<4 ; ++i) {
                        auto x = (unsigned char)(c[i]);
                        unsigned digit;
                        if (x == '\0') {
                            auto msg = "Truncated unicode escape sequence in string literal.";
                            throw StaticError(loc, msg);
                        } else if (x >= '0' && x <= '9') {
                            digit = x - '0';
                        } else if (x >= 'a' && x <= 'f') {
                            digit = x - 'a' + 10;
                        } else if (x >= 'A' && x <= 'F') {
                            digit = x - 'A' + 10;
                        } else {
                            std::stringstream ss;
                            ss << "Malformed unicode escape character, "
                               << "should be hex: '" << x << "'";
                            throw StaticError(loc, ss.str());
                        }
                        codepoint *= 16;
                        codepoint += digit;
                    }

                    r += codepoint;

                    // Leave us on the last char, ready for the ++c at
                    // the outer for loop.
                    c += 3;
                }
                break;

                case '\0': {
                    auto msg = "Truncated escape sequence in string literal.";
                    throw StaticError(loc, msg);
                }

                default: {
                    std::stringstream ss;
                    std::string utf8;
                    encode_utf8(*c, utf8);
                    ss << "Unknown escape sequence in string literal: '" << utf8 << "'";
                    throw StaticError(loc, ss.str());
                }
            }
            break;

            default:
            // Just a regular letter.
            r += *c;
        }
    }
    return r;
}


/** Desugar Jsonnet expressions to reduce the number of constructs the rest of the implementation
 * needs to understand.
 *
 * Desugaring should happen immediately after parsing, i.e. before static analysis and execution.
 * Temporary variables introduced here should be prefixed with $ to ensure they do not clash with
 * variables used in user code.
 */
class Desugarer {

    Allocator *alloc;

    template <class T, class... Args> T* make(Args&&... args)
    {
        return alloc->make<T>(std::forward<Args>(args)...);
    }

    const Identifier *id(const String &s)
    { return alloc->makeIdentifier(s); }

    LiteralString *str(const String &s)
    { return make<LiteralString>(E, s, LiteralString::DOUBLE, ""); }

    Var *var(const Identifier *ident)
    { return make<Var>(E, ident); }

    Var *std(void)
    { return var(id(U"std")); }

    Array *singleton(AST *body)
    { return make<Array>(body->location, std::vector<AST*>{body}, false); }

    Apply *length(AST *v)
    {
        return make<Apply>(
            v->location, 
            make<Index>(
                E,
                std(),
                str(U"length"),
                nullptr),
            std::vector<AST*>{v},
            false,  // trailingComma
            true  // tailstrict
        );
    }

    public:
    Desugarer(Allocator *alloc)
      : alloc(alloc)
    { }

    void desugarFields(AST *ast, ObjectFields &fields, unsigned obj_level)
    {
        // Desugar children
        for (auto &field : fields) {
            if (field.expr1 != nullptr) desugar(field.expr1, obj_level);
            desugar(field.expr2, obj_level + 1);
            if (field.expr3 != nullptr) desugar(field.expr3, obj_level + 1);
        }

        // Simplify asserts
        for (auto &field : fields) {
            if (field.kind != ObjectField::ASSERT) continue;
            AST *msg = field.expr3;
            field.expr3 = nullptr;
            if (msg == nullptr) {
                auto msg_str = U"Object assertion failed.";
                msg = alloc->make<LiteralString>(field.expr2->location, msg_str,
                                                 LiteralString::DOUBLE, "");
            }

            // if expr2 then true else error msg
            field.expr2 = alloc->make<Conditional>(
                ast->location,
                field.expr2,
                alloc->make<LiteralBoolean>(E, true),
                alloc->make<Error>(msg->location, msg));
        }

        // Remove methods
        for (auto &field : fields) {
            if (!field.methodSugar) continue;
            field.expr2 = alloc->make<Function>(
                field.expr2->location, field.ids, false, field.expr2);
            field.methodSugar = false;
            field.ids.clear();
        }


        // Remove object-level locals
        auto copy = fields;
        fields.clear();
        Local::Binds binds;
        for (auto &local : copy) {
            if (local.kind != ObjectField::LOCAL) continue;
            binds.emplace_back(local.id, local.expr2);
        }
        for (auto &field : copy) {
            if (field.kind == ObjectField::LOCAL) continue;
            if (!binds.empty())
                field.expr2 = alloc->make<Local>(ast->location, binds, field.expr2);
            fields.push_back(field);
        }

        // Change all to FIELD_EXPR
        for (auto &field : fields) {
            switch (field.kind) {
                case ObjectField::ASSERT:
                // Nothing to do.
                break;

                case ObjectField::FIELD_ID:
                field.expr1 = str(field.id->name);
                field.kind = ObjectField::FIELD_EXPR;
                break;

                case ObjectField::FIELD_EXPR:
                // Nothing to do.
                break;

                case ObjectField::FIELD_STR:
                // Just set the flag.
                field.kind = ObjectField::FIELD_EXPR;
                break;

                case ObjectField::LOCAL:
                std::cerr << "Locals should be removed by now." << std::endl;
                abort();
            }
        }

        // Remove +:
        for (auto &field : fields) {
            if (!field.superSugar) continue;
            AST *super_f = alloc->make<SuperIndex>(field.expr1->location, field.expr1, nullptr);
            field.expr2 = alloc->make<Binary>(ast->location, super_f, BOP_PLUS, field.expr2);
            field.superSugar = false;
        }
    }

    void desugar(AST *&ast_, unsigned obj_level)
    {
        if (auto *ast = dynamic_cast<Apply*>(ast_)) {
            desugar(ast->target, obj_level);
            for (AST *&arg : ast->arguments)
                desugar(arg, obj_level);

        } else if (auto *ast = dynamic_cast<ApplyBrace*>(ast_)) {
            desugar(ast->left, obj_level);
            desugar(ast->right, obj_level);
            ast_ = alloc->make<Binary>(ast->location, ast->left, BOP_PLUS, ast->right);

        } else if (auto *ast = dynamic_cast<Array*>(ast_)) {
            for (AST *&el : ast->elements)
                desugar(el, obj_level);

        } else if (auto *ast = dynamic_cast<ArrayComprehension*>(ast_)) {
            for (ComprehensionSpec &spec : ast->specs)
                desugar(spec.expr, obj_level);
            desugar(ast->body, obj_level + 1);

            int n = ast->specs.size();
            AST *zero = make<LiteralNumber>(E, "0.0");
            AST *one = make<LiteralNumber>(E, "1.0");
            auto *_r = id(U"$r");
            auto *_l = id(U"$l");
            std::vector<const Identifier*> _i(n);
            for (int i = 0; i < n ; ++i) {
                StringStream ss;
                ss << U"$i_" << i;
                _i[i] = id(ss.str());
            }
            std::vector<const Identifier*> _aux(n);
            for (int i = 0; i < n ; ++i) {
                StringStream ss;
                ss << U"$aux_" << i;
                _aux[i] = id(ss.str());
            }

            // Build it from the inside out.  We keep wrapping 'in' with more ASTs.
            assert(ast->specs[0].kind == ComprehensionSpec::FOR);

            int last_for = n - 1;
            while (ast->specs[last_for].kind != ComprehensionSpec::FOR)
                last_for--;
            // $aux_{last_for}($i_{last_for} + 1, $r + [body])
            AST *in = make<Apply>(
                ast->body->location,
                var(_aux[last_for]),
                std::vector<AST*> {
                    make<Binary>(E, var(_i[last_for]), BOP_PLUS, one),
                    make<Binary>(E, var(_r), BOP_PLUS, singleton(ast->body))
                },
                false,  // trailingComma
                true  // tailstrict
            );
            for (int i = n - 1; i >= 0 ; --i) {
                const ComprehensionSpec &spec = ast->specs[i];
                AST *out;
                if (i > 0) {
                    int prev_for = i - 1;
                    while (ast->specs[prev_for].kind != ComprehensionSpec::FOR)
                        prev_for--;

                    // aux_{prev_for}($i_{prev_for} + 1, $r)
                    out = make<Apply>(  // False branch.
                        E,
                        var(_aux[prev_for]), 
                        std::vector<AST*> {
                            make<Binary>(E, var(_i[prev_for]), BOP_PLUS, one), var(_r)},
                        false, // trailingComma
                        true  // tailstrict
                    );
                } else {
                    out = var(_r);
                }
                switch (spec.kind) {
                    case ComprehensionSpec::IF: {
                        /*
                            if [[[...cond...]]] then
                                [[[...in...]]]
                            else
                                [[[...out...]]]
                        */
                        in = make<Conditional>(
                            ast->location,
                            spec.expr,
                            in,  // True branch.
                            out);  // False branch.
                    } break;
                    case ComprehensionSpec::FOR: {
                        /*
                            local $l = [[[...array...]]];
                            local aux_{i}(i_{i}, r) =
                                if i_{i} >= std.length(l) then
                                    [[[...out...]]]
                                else
                                    local [[[...var...]]] = l[i_{i}];
                                    [[[...in...]]]
                            aux_{i}(0, r) tailstrict;
                        */
                        in = make<Local>(
                            ast->location,
                            Local::Binds {
                                {_l, spec.expr},
                                {_aux[i], make<Function>(
                                    ast->location,
                                    std::vector<const Identifier*>{_i[i], _r},
                                    false,  // trailingComma
                                    make<Conditional>(
                                        ast->location, 
                                        make<Binary>(
                                            E, var(_i[i]), BOP_GREATER_EQ, length(var(_l))),
                                        out,
                                        make<Local>(
                                            ast->location,
                                            Local::Binds {{
                                                spec.var,
                                                make<Index>(E, var(_l), var(_i[i]), nullptr),
                                            }},
                                            in)
                                    )
                                )}},
                                make<Apply>(
                                    E,
                                    var(_aux[i]),
                                    std::vector<AST*> {
                                        zero,
                                        i == 0 ? make<Array>(E, std::vector<AST*>{}, false)
                                               : static_cast<AST*>(var(_r))
                                    },
                                    false,  // trailingComma
                                    true));  // tailstrict
                    } break;
                }
            }
                    
            ast_ = in;

        } else if (auto *ast = dynamic_cast<Assert*>(ast_)) {
            desugar(ast->cond, obj_level);
            if (ast->message == nullptr) {
                ast->message = str(U"Assertion failed.");
            }
            desugar(ast->message, obj_level);
            desugar(ast->rest, obj_level);

            // if cond then rest else error msg
            AST *branch_false = alloc->make<Error>(ast->location, ast->message);
            ast_ = alloc->make<Conditional>(ast->location,
                                            ast->cond, ast->rest, branch_false);

        } else if (auto *ast = dynamic_cast<Binary*>(ast_)) {
            desugar(ast->left, obj_level);
            desugar(ast->right, obj_level);

            bool invert = false;

            switch (ast->op) {
                case BOP_PERCENT: {
                    AST *f_mod = alloc->make<Index>(E, std(), str(U"mod"), nullptr);
                    std::vector<AST*> args = {ast->left, ast->right};
                    ast_ = alloc->make<Apply>(ast->location, f_mod, args, false, false);
                } break;

                case BOP_MANIFEST_UNEQUAL:
                invert = true;
                case BOP_MANIFEST_EQUAL: {
                    AST *f_equals = alloc->make<Index>(E, std(), str(U"equals"), nullptr);
                    std::vector<AST*> args = {ast->left, ast->right};
                    ast_ = alloc->make<Apply>(ast->location, f_equals, args, false, false);
                    if (invert)
                        ast_ = alloc->make<Unary>(ast->location, UOP_NOT, ast_);
                }
                break;

                default:;
                // Otherwise don't change it.
            }

        } else if (dynamic_cast<const BuiltinFunction*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<Conditional*>(ast_)) {
            desugar(ast->cond, obj_level);
            desugar(ast->branchTrue, obj_level);
            if (ast->branchFalse == nullptr)
                ast->branchFalse = alloc->make<LiteralNull>(LocationRange());
            desugar(ast->branchFalse, obj_level);

        } else if (auto *ast = dynamic_cast<Dollar*>(ast_)) {
            if (obj_level == 0) {
                throw StaticError(ast->location, "No top-level object found.");
            }
            ast_ = alloc->make<Var>(ast->location, alloc->makeIdentifier(U"$"));

        } else if (auto *ast = dynamic_cast<Error*>(ast_)) {
            desugar(ast->expr, obj_level);

        } else if (auto *ast = dynamic_cast<Function*>(ast_)) {
            desugar(ast->body, obj_level);

        } else if (dynamic_cast<const Import*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const Importstr*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<Index*>(ast_)) {
            desugar(ast->target, obj_level);
            if (ast->id != nullptr) {
                assert(ast->index == nullptr);
                ast->index = str(ast->id->name);
                ast->id = nullptr;
            }
            desugar(ast->index, obj_level);

        } else if (auto *ast = dynamic_cast<Local*>(ast_)) {
            for (auto &bind: ast->binds)
                desugar(bind.body, obj_level);
            desugar(ast->body, obj_level);

            for (auto &bind: ast->binds) {
                if (bind.functionSugar) {
                    bind.body = alloc->make<Function>(ast->location, bind.params, false, bind.body);
                    bind.functionSugar = false;
                    bind.params.clear();
                }
            }

        } else if (dynamic_cast<const LiteralBoolean*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const LiteralNumber*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<LiteralString*>(ast_)) {
            ast->value = desugar_string(ast->location, ast->value);
            ast->tokenKind = LiteralString::DOUBLE;
            ast->blockIndent.clear();

        } else if (dynamic_cast<const LiteralNull*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<DesugaredObject*>(ast_)) {
            for (auto &field : ast->fields) {
                desugar(field.name, obj_level);
                desugar(field.body, obj_level + 1);
            }
            for (AST *assert : ast->asserts) {
                desugar(assert, obj_level + 1);
            }

        } else if (auto *ast = dynamic_cast<Object*>(ast_)) {
            // Hidden variable to allow outer/top binding.
            if (obj_level == 0) {
                const Identifier *hidden_var = alloc->makeIdentifier(U"$");
                auto *body = alloc->make<Self>(E);
                ast->fields.push_back(ObjectField::Local(hidden_var, body));
            }

            desugarFields(ast, ast->fields, obj_level);

            DesugaredObject::Fields new_fields;
            ASTs new_asserts;
            for (const ObjectField &field : ast->fields) {
                if (field.kind == ObjectField::ASSERT) {
                    new_asserts.push_back(field.expr2);
                } else if (field.kind == ObjectField::FIELD_EXPR) {
                    new_fields.emplace_back(field.hide, field.expr1, field.expr2);
                } else {
                    std::cerr << "INTERNAL ERROR: field should have been desugared: "
                              << field.kind << std::endl;
                }
            }
            ast_ = alloc->make<DesugaredObject>(ast->location, new_asserts, new_fields);

        } else if (auto *ast = dynamic_cast<ObjectComprehension*>(ast_)) {
            // Hidden variable to allow outer/top binding.
            if (obj_level == 0) {
                const Identifier *hidden_var = alloc->makeIdentifier(U"$");
                auto *body = alloc->make<Self>(E);
                ast->fields.push_back(ObjectField::Local(hidden_var, body));
            }

            desugarFields(ast, ast->fields, obj_level);

            for (ComprehensionSpec &spec : ast->specs)
                desugar(spec.expr, obj_level);

            AST *field = ast->fields.front().expr1;
            AST *value = ast->fields.front().expr2;

            /*  {
                    [arr[0]]: local x = arr[1], y = arr[2], z = arr[3]; val_expr
                    for arr in [ [key_expr, x, y, z] for ...  ]
                }
            */
            auto *_arr = id(U"$arr");
            AST *zero = make<LiteralNumber>(E, "0.0");
            int counter = 1;
            Local::Binds binds;
            auto arr_e = std::vector<AST*> {field};
            for (ComprehensionSpec &spec : ast->specs) {
                if (spec.kind == ComprehensionSpec::FOR) {
                    std::stringstream num;
                    num << counter++;
                    binds.emplace_back(
                        spec.var,
                        make<Index>(E, var(_arr), make<LiteralNumber>(E, num.str()), nullptr));
                    arr_e.push_back(var(spec.var));
                }
            }
            AST *arr = make<ArrayComprehension>(
                ast->location,
                make<Array>(ast->location, arr_e, false),
                false,
                ast->specs);
            desugar(arr, obj_level);
            ast_ = make<ObjectComprehensionSimple>(
                ast->location,
                make<Index>(E, var(_arr), zero, nullptr),
                make<Local>(
                    ast->location,
                    binds,
                    value),
                _arr,
                arr);

        } else if (auto *ast = dynamic_cast<ObjectComprehensionSimple*>(ast_)) {
            desugar(ast->field, obj_level);
            desugar(ast->value, obj_level + 1);
            desugar(ast->array, obj_level);

        } else if (dynamic_cast<const Self*>(ast_)) {
            // Nothing to do.

        } else if (auto * ast = dynamic_cast<SuperIndex*>(ast_)) {
            if (ast->id != nullptr) {
                assert(ast->index == nullptr);
                ast->index = str(ast->id->name);
                ast->id = nullptr;
            }
            desugar(ast->index, obj_level);

        } else if (auto *ast = dynamic_cast<Unary*>(ast_)) {
            desugar(ast->expr, obj_level);

        } else if (dynamic_cast<const Var*>(ast_)) {
            // Nothing to do.

        } else {
            std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
            std::abort();

        }
    }

    void desugarFile(AST *&ast)
    {
        desugar(ast, 0);

        // Now, implement the std library by wrapping in a local construct.
        Tokens tokens = jsonnet_lex("std.jsonnet", STD_CODE);
        AST *std_ast = jsonnet_parse(alloc, tokens);
        desugar(std_ast, 0);
        auto *std_obj = dynamic_cast<DesugaredObject*>(std_ast);
        if (std_obj == nullptr) {
            std::cerr << "INTERNAL ERROR: std.jsonnet not an object." << std::endl;
            std::abort();
        }

        // Bind 'std' builtins that are implemented natively.
        DesugaredObject::Fields &fields = std_obj->fields;
        for (unsigned long c=0 ; c <= max_builtin ; ++c) {
            const auto &decl = jsonnet_builtin_decl(c);
            Identifiers params;
            for (const auto &p : decl.params)
                params.push_back(alloc->makeIdentifier(p));
            fields.emplace_back(
                ObjectField::HIDDEN,
                str(decl.name),
                alloc->make<BuiltinFunction>(E, c, params));
        }
        fields.emplace_back(
            ObjectField::HIDDEN,
            str(U"thisFile"),
            str(decode_utf8(ast->location.file)));

        Local::Binds binds {Local::Bind(id(U"std"), std_obj)};
        ast = alloc->make<Local>(ast->location, binds, ast);
    }
};

void jsonnet_desugar(Allocator *alloc, AST *&ast)
{
    Desugarer desugarer(alloc);
    desugarer.desugarFile(ast);
}
