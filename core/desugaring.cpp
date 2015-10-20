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

LocationRange E;  // Empty.
    
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

    Var *var(const Identifier *ident)
    { return make<Var>(E, ident); }

    Array *singleton(AST *body)
    { return make<Array>(body->location, std::vector<AST*>{body}); }

    Apply *length(AST *v)
    {
        return make<Apply>(
            v->location, 
            make<Index>(
                E,
                var(id(U"std")),
                make<LiteralString>(E, U"length")),
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
            for (ComprehensionSpec &spec : ast->specs)
                desugar(spec.expr);
            desugar(ast->body);

            int n = ast->specs.size();
            AST *zero = make<LiteralNumber>(E, 0.0);
            AST *one = make<LiteralNumber>(E, 1.0);
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
                                    make<Conditional>(
                                        ast->location, 
                                        make<Binary>(
                                            E, var(_i[i]), BOP_GREATER_EQ, length(var(_l))),
                                        out,
                                        make<Local>(
                                            ast->location,
                                            Local::Binds {{
                                                spec.var,
                                                make<Index>(E, var(_l), var(_i[i]))
                                            }},
                                            in)
                                    )
                                )}},
                                make<Apply>(
                                    E,
                                    var(_aux[i]),
                                    std::vector<AST*> {
                                        zero,
                                        i == 0 ? make<Array>(E, std::vector<AST*>{})
                                               : static_cast<AST*>(var(_r))
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
            for (ComprehensionSpec &spec : ast->specs)
                desugar(spec.expr);
            desugar(ast->field);
            desugar(ast->value);

            /*  {
                    [arr[0]]: local x = arr[1], y = arr[2], z = arr[3]; val_expr
                    for arr in [ [key_expr, x, y, z] for ...  ]
                }
            */
            auto *_arr = id(U"$arr");
            AST *zero = make<LiteralNumber>(E, 0.0);
            int counter = 1;
            Local::Binds binds;
            auto arr_e = std::vector<AST*> {ast->field};
            for (ComprehensionSpec &spec : ast->specs) {
                if (spec.kind == ComprehensionSpec::FOR) {
                    binds[spec.var] =
                        make<Index>(E, var(_arr), make<LiteralNumber>(E, double(counter++)));
                    arr_e.push_back(var(spec.var));
                }
            }
            AST *arr = make<ArrayComprehension>(
                ast->location,
                make<Array>(ast->location, arr_e),
                ast->specs);
            desugar(arr);
            ast_ = make<ObjectComprehensionSimple>(
                ast->location,
                make<Index>(E, var(_arr), zero),
                make<Local>(
                    ast->location,
                    binds,
                    ast->value),
                _arr,
                arr);

        } else if (auto *ast = dynamic_cast<ObjectComprehensionSimple*>(ast_)) {
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
