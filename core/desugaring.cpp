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

#include "core/ast.h"

LocationRange E;  // Empty.

class Desugarer {

    Allocator *alloc;

    const Identifier *id(const String &s)
    { return alloc->makeIdentifier(s); }

    Var *var(const Identifier *ident)
    { return alloc->make<Var>(E, ident); }

    Array *singleton(AST *body)
    { return alloc->make<Array>(body->location, std::vector<AST*>{body}); }

    Apply *length(AST *v)
    {
        return alloc->make<Apply>(
            v->location, 
            alloc->make<Index>(
                E,
                var(id(U"std")),
                alloc->make<LiteralString>(E, U"length")),
            std::vector<AST*>{v},
            true  // tailstrict
        );
    }

    public:
    Desugarer(Allocator *alloc)
      : alloc(alloc)
    { }

    void desugar(AST *&ast_)
    {
        if (auto *ast = dynamic_cast<Apply*>(ast_)) {
                
            desugar(ast->target);
            for (AST *&arg : ast->arguments)
                desugar(arg);

        } else if (auto *ast = dynamic_cast<Array*>(ast_)) {
            for (AST *&el : ast->elements)
                desugar(el);

        } else if (auto *ast = dynamic_cast<ArrayComprehension*>(ast_)) {
            for (ArrayComprehension::Spec &spec : ast->specs)
                desugar(spec.expr);
            desugar(ast->body);

            int n = ast->specs.size();
            AST *zero = alloc->make<LiteralNumber>(E, 0.0);
            AST *one = alloc->make<LiteralNumber>(E, 1.0);
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
            assert(ast->specs[0].kind == ArrayComprehension::SPEC_FOR);

            int last_for = n - 1;
            while (ast->specs[last_for].kind != ArrayComprehension::SPEC_FOR)
                last_for--;
            // $aux_{last_for}($i_{last_for} + 1, $r + [body])
            AST *in = alloc->make<Apply>(
                ast->body->location,
                var(_aux[last_for]),
                std::vector<AST*> {
                    alloc->make<Binary>(E, var(_i[last_for]), BOP_PLUS, one),
                    alloc->make<Binary>(E, var(_r), BOP_PLUS, singleton(ast->body))
                },
                true  // tailstrict
            );
            for (int i = n - 1; i >= 0 ; --i) {
                const ArrayComprehension::Spec &spec = ast->specs[i];
                AST *out;
                if (i > 0) {
                    int prev_for = i - 1;
                    while (ast->specs[prev_for].kind != ArrayComprehension::SPEC_FOR)
                        prev_for--;

                    // aux_{prev_for}($i_{prev_for} + 1, $r)
                    out = alloc->make<Apply>(  // False branch.
                        E,
                        var(_aux[prev_for]), 
                        std::vector<AST*> { alloc->make<Binary>(E, var(_i[prev_for]), BOP_PLUS, one), var(_r) },
                        true  // tailstrict
                    );
                } else {
                    out = var(_r);
                }
                switch (spec.kind) {
                    case ArrayComprehension::SPEC_IF: {
                        /*
                            if [[[...cond...]]] then
                                [[[...in...]]]
                            else
                                [[[...out...]]]
                        */
                        in = alloc->make<Conditional>(
                            ast->location,
                            spec.expr,
                            in,  // True branch.
                            out);  // False branch.
                    } break;
                    case ArrayComprehension::SPEC_FOR: {
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
                        in = alloc->make<Local>(
                            ast->location,
                            Local::Binds {
                                {_l, spec.expr},
                                {_aux[i], alloc->make<Function>(
                                    ast->location,
                                    std::vector<const Identifier*>{_i[i], _r},
                                    alloc->make<Conditional>(
                                        ast->location, 
                                        alloc->make<Binary>(E, var(_i[i]), BOP_GREATER_EQ, length(var(_l))),
                                        out,
                                        alloc->make<Local>(
                                            ast->location,
                                            Local::Binds {{
                                                spec.var,
                                                alloc->make<Index>(E, var(_l), var(_i[i]))
                                            }},
                                            in)
                                    )
                                )}},
                                alloc->make<Apply>(
                                    E,
                                    var(_aux[i]),
                                    std::vector<AST*> {
                                        zero,
                                        i == 0 ? alloc->make<Array>(E, std::vector<AST*>{}) : static_cast<AST*>(var(_r))
                                    },
                                    true));  // tailstrict
                    } break;
                }
            }
                    

            ast_ = in;

        } else if (auto *ast = dynamic_cast<Binary*>(ast_)) {
            desugar(ast->left);
            desugar(ast->right);

        } else if (dynamic_cast<const BuiltinFunction*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<Conditional*>(ast_)) {
            desugar(ast->cond);
            desugar(ast->branchTrue);
            desugar(ast->branchFalse);

        } else if (auto *ast = dynamic_cast<Error*>(ast_)) {
            desugar(ast->expr);

        } else if (auto *ast = dynamic_cast<Function*>(ast_)) {
            desugar(ast->body);

        } else if (dynamic_cast<const Import*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const Importstr*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<Index*>(ast_)) {
            desugar(ast->target);
            desugar(ast->index);

        } else if (auto *ast = dynamic_cast<Local*>(ast_)) {
            for (auto &bind: ast->binds)
                desugar(bind.second);
            desugar(ast->body);

        } else if (dynamic_cast<const LiteralBoolean*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const LiteralNumber*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const LiteralString*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const LiteralNull*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<Object*>(ast_)) {
            for (auto &assert : ast->asserts) {
                desugar(assert);
            }
            for (auto &field : ast->fields) {
                desugar(field.name);
                desugar(field.body);
            }

        } else if (auto *ast = dynamic_cast<ObjectComprehension*>(ast_)) {
            desugar(ast->field);
            desugar(ast->value);
            desugar(ast->array);

        } else if (dynamic_cast<const Self*>(ast_)) {
            // Nothing to do.

        } else if (dynamic_cast<const Super*>(ast_)) {
            // Nothing to do.

        } else if (auto *ast = dynamic_cast<Unary*>(ast_)) {
            desugar(ast->expr);

        } else if (dynamic_cast<const Var*>(ast_)) {
            // Nothing to do.

        } else {
            std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
            std::abort();

        }
    }
};

void jsonnet_desugar(Allocator *alloc, AST *&ast)
{
    Desugarer desugarer(alloc);
    desugarer.desugar(ast);
}
