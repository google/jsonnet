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

#include "pass.h"

void CompilerPass::fodder(Fodder &fodder)
{
    for (auto &f : fodder)
        fodderElement(f);
}

void CompilerPass::specs(std::vector<ComprehensionSpec> &specs)
{
    for (auto &spec : specs) {
        fodder(spec.openFodder);
        switch (spec.kind) {
            case ComprehensionSpec::FOR:
            fodder(spec.varFodder);
            fodder(spec.inFodder);
            expr(spec.expr);
            break;
            case ComprehensionSpec::IF:
            expr(spec.expr);
            break;
        }
    }
}

void CompilerPass::params(Fodder &fodder_l, ArgParams &params, Fodder &fodder_r)
{
    fodder(fodder_l);
    for (auto &param : params) {
        fodder(param.idFodder);
        if (param.expr) {
            fodder(param.eqFodder);
            expr(param.expr);
        }
        fodder(param.commaFodder);
    }
    fodder(fodder_r);
}

void CompilerPass::fieldParams(ObjectField &field)
{
    if (field.methodSugar) {
        params(field.fodderL, field.params, field.fodderR);
    }
}

void CompilerPass::fields(ObjectFields &fields)
{
    for (auto &field : fields) {

        switch (field.kind) {
            case ObjectField::LOCAL: {
                fodder(field.fodder1);
                fodder(field.fodder2);
                fieldParams(field);
                fodder(field.opFodder);
                expr(field.expr2);
            } break;

            case ObjectField::FIELD_ID:
            case ObjectField::FIELD_STR:
            case ObjectField::FIELD_EXPR: {
                if (field.kind == ObjectField::FIELD_ID) {
                    fodder(field.fodder1);

                } else if (field.kind == ObjectField::FIELD_STR) {
                    expr(field.expr1);

                } else if (field.kind == ObjectField::FIELD_EXPR) {
                    fodder(field.fodder1);
                    expr(field.expr1);
                    fodder(field.fodder2);
                }
                fieldParams(field);
                fodder(field.opFodder);
                expr(field.expr2);

            } break;

            case ObjectField::ASSERT: {
                fodder(field.fodder1);
                expr(field.expr2);
                if (field.expr3 != nullptr) {
                    fodder(field.opFodder);
                    expr(field.expr3);
                }
            } break;
        }

        fodder(field.commaFodder);
    }
}

void CompilerPass::expr(AST *&ast_)
{
    fodder(ast_->openFodder);
    visitExpr(ast_);
}

void CompilerPass::visit(Apply *ast)
{
    expr(ast->target);
    fodder(ast->fodderL);
    for (auto &arg : ast->args) {
        expr(arg.expr);
        fodder(arg.commaFodder);
    }
    fodder(ast->fodderR);
    if (ast->tailstrict) {
        fodder(ast->tailstrictFodder);
    }
}

void CompilerPass::visit(ApplyBrace *ast)
{
    expr(ast->left);
    expr(ast->right);
}

void CompilerPass::visit(Array *ast)
{
    for (auto &element : ast->elements) {
        expr(element.expr);
        fodder(element.commaFodder);
    }
    fodder(ast->closeFodder);
}

void CompilerPass::visit(ArrayComprehension *ast)
{
    expr(ast->body);
    fodder(ast->commaFodder);
    specs(ast->specs);
    fodder(ast->closeFodder);
}

void CompilerPass::visit(Assert *ast)
{
    expr(ast->cond);
    if (ast->message != nullptr) {
        fodder(ast->colonFodder);
        expr(ast->message);
    }
    fodder(ast->semicolonFodder);
    expr(ast->rest);
}

void CompilerPass::visit(Binary *ast)
{
    expr(ast->left);
    fodder(ast->opFodder);
    expr(ast->right);
}

void CompilerPass::visit(Conditional *ast)
{
    expr(ast->cond);
    fodder(ast->thenFodder);
    if (ast->branchFalse != nullptr) {
        expr(ast->branchTrue);
        fodder(ast->elseFodder);
        expr(ast->branchFalse);
    } else {
        expr(ast->branchTrue);
    }
}

void CompilerPass::visit(Error *ast)
{
    expr(ast->expr);
}

void CompilerPass::visit(Function *ast)
{
    params(ast->parenLeftFodder, ast->params, ast->parenRightFodder);
    expr(ast->body);
}

void CompilerPass::visit(Import *ast)
{
    visit(ast->file);
}

void CompilerPass::visit(Importstr *ast)
{
    visit(ast->file);
}

void CompilerPass::visit(InSuper *ast)
{
    expr(ast->element);
}

void CompilerPass::visit(Index *ast)
{
    expr(ast->target);
    if (ast->id != nullptr) {
    } else {
        if (ast->isSlice) {
            if (ast->index != nullptr)
                expr(ast->index);
            if (ast->end != nullptr)
                expr(ast->end);
            if (ast->step != nullptr)
                expr(ast->step);
        } else {
            expr(ast->index);
        }
    }
}

void CompilerPass::visit(Local *ast)
{
    assert(ast->binds.size() > 0);
    for (auto &bind : ast->binds) {
        fodder(bind.varFodder);
        if (bind.functionSugar) {
            params(bind.parenLeftFodder, bind.params, bind.parenRightFodder);
        } 
        fodder(bind.opFodder);
        expr(bind.body);
        fodder(bind.closeFodder);
    }
    expr(ast->body);
}

void CompilerPass::visit(Object *ast)
{
    fields(ast->fields);
    fodder(ast->closeFodder);
}

void CompilerPass::visit(DesugaredObject *ast)
{
    for (AST *assert : ast->asserts) {
        expr(assert);
    }
    for (auto &field : ast->fields) {
        expr(field.name);
        expr(field.body);
    }
}

void CompilerPass::visit(ObjectComprehension *ast)
{
    fields(ast->fields);
    specs(ast->specs);
    fodder(ast->closeFodder);
}

void CompilerPass::visit(ObjectComprehensionSimple *ast)
{
    expr(ast->field);
    expr(ast->value);
    expr(ast->array);
}

void CompilerPass::visit(Parens *ast)
{
    expr(ast->expr);
    fodder(ast->closeFodder);
}

void CompilerPass::visit(SuperIndex *ast)
{
    if (ast->id != nullptr) {
    } else {
        expr(ast->index);
    }
}

void CompilerPass::visit(Unary *ast)
{
    expr(ast->expr);
}

void CompilerPass::visitExpr(AST *&ast_)
{
    if (auto *ast = dynamic_cast<Apply*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<ApplyBrace*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Array*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<ArrayComprehension*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Assert*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Binary*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<BuiltinFunction*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Conditional*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Dollar*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Error*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Function*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Import*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Importstr*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<InSuper*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Index*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Local*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<LiteralBoolean*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<LiteralNumber*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<LiteralString*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<LiteralNull*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Object*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<DesugaredObject*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<ObjectComprehension*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<ObjectComprehensionSimple*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Parens*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Self*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<SuperIndex*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Unary*>(ast_)) {
        visit(ast);
    } else if (auto *ast = dynamic_cast<Var*>(ast_)) {
        visit(ast);

    } else {
        std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
        std::abort();

    }
}

void CompilerPass::file(AST *&body, Fodder &final_fodder)
{
    expr(body);
    fodder(final_fodder);
}

/** A pass that clones the AST it is given. */
class ClonePass : public CompilerPass {
    public:
    ClonePass(Allocator &alloc) : CompilerPass(alloc) { }
    virtual void expr(AST *&ast);
};

void ClonePass::expr(AST *&ast_)
{
    if (auto *ast = dynamic_cast<Apply*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<ApplyBrace*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Array*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<ArrayComprehension*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Assert*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Binary*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<BuiltinFunction*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Conditional*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Dollar*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Error*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Function*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Import*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Importstr*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<InSuper*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Index*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Local*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<LiteralBoolean*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<LiteralNumber*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<LiteralString*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<LiteralNull*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Object*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<DesugaredObject*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<ObjectComprehension*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<ObjectComprehensionSimple*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Parens*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Self*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<SuperIndex*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Unary*>(ast_)) {
        ast_ = alloc.clone(ast);
    } else if (auto *ast = dynamic_cast<Var*>(ast_)) {
        ast_ = alloc.clone(ast);

    } else {
        std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
        std::abort();

    }

    CompilerPass::expr(ast_);
}

AST *clone_ast(Allocator &alloc, AST *ast)
{
    AST *r = ast;
    ClonePass(alloc).expr(r);
    return r;
}
