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

#include <typeinfo>

#include "lexer.h"
#include "formatter.h"
#include "string_utils.h"
#include "unicode.h"

static std::string unparse_id(const Identifier *id)
{
    return encode_utf8(id->name);
}

/** If left recursive, return the left hand side, else return nullptr. */
static AST *left_recursive(AST *ast_)
{
    if (auto *ast = dynamic_cast<Apply*>(ast_))
        return ast->target;
    if (auto *ast = dynamic_cast<ApplyBrace*>(ast_))
        return ast->left;
    if (auto *ast = dynamic_cast<Binary*>(ast_))
        return ast->left;
    if (auto *ast = dynamic_cast<Index*>(ast_))
        return ast->target;
    return nullptr;
}
static const AST *left_recursive(const AST *ast_)
{
    return left_recursive(const_cast<AST*>(ast_));
}

/** Pretty-print fodder.
 *
 * \param fodder The fodder to print
 * \param space_before Whether a space should be printed before any other output.
 * \param separate_token If a space should separate the token from any interstitials.
 */
void fodder_fill(std::ostream &o, const Fodder &fodder, bool space_before, bool separate_token)
{
    unsigned last_indent = 0;
    for (const auto &fod : fodder) {
        switch (fod.kind) {
            case FodderElement::LINE_END:
            if (fod.comment.size() > 0)
                o << "  " << fod.comment[0];
            o << '\n';
            o << std::string(fod.blanks, '\n');
            o << std::string(fod.indent, ' ');
            last_indent = fod.indent;
            space_before = false;
            break;

            case FodderElement::INTERSTITIAL:
            if (space_before)
                o << ' ';
            o << fod.comment[0];
            space_before = true;
            break;

            case FodderElement::PARAGRAPH: {
                bool first = true;
                for (const std::string &l : fod.comment) {
                    // Do not indent empty lines (note: first line is never empty).
                    if (l.length() > 0) {
                        // First line is already indented by previous fod.
                        if (!first)
                            o << std::string(last_indent, ' ');
                        o << l;
                    }
                    o << '\n';
                    first = false;
                }
                o << std::string(fod.blanks, '\n');
                o << std::string(fod.indent, ' ');
                last_indent = fod.indent;
                space_before = false;
            } break;
        }
    }
    if (separate_token && space_before)
        o << ' ';
}

/** A model of fodder_fill that just keeps track of the column counter. */
static void fodder_count(unsigned &column, const Fodder &fodder, bool space_before,
                         bool separate_token)
{
    for (const auto &fod : fodder) {
        switch (fod.kind) {
            case FodderElement::PARAGRAPH:
            case FodderElement::LINE_END:
            column = fod.indent;
            space_before = false;
            break;

            case FodderElement::INTERSTITIAL:
            if (space_before)
                column++;
            column += fod.comment[0].length();
            space_before = true;
            break;
        }
    }
    if (separate_token && space_before)
        column++;
}

class Unparser {
    public:

    private:
    std::ostream &o;
    FmtOpts opts;

    public:
    Unparser(std::ostream &o, const FmtOpts &opts)
      : o(o), opts(opts)
    { }

    void unparseSpecs(const std::vector<ComprehensionSpec> &specs)
    {
        for (const auto &spec : specs) {
            fill(spec.openFodder, true, true);
            switch (spec.kind) {
                case ComprehensionSpec::FOR:
                o << "for";
                fill(spec.varFodder, true, true);
                o << unparse_id(spec.var);
                fill(spec.inFodder, true, true);
                o << "in";
                unparse(spec.expr, true);
                break;
                case ComprehensionSpec::IF:
                o << "if";
                unparse(spec.expr, true);
                break;
            }
        }
    }

    void fill(const Fodder &fodder, bool space_before, bool separate_token)
    {
        fodder_fill(o, fodder, space_before, separate_token);
    }

    void unparseParams(const Fodder &fodder_l, const ArgParams &params, bool trailing_comma,
                       const Fodder &fodder_r)
    {
        fill(fodder_l, false, false);
        o << "(";
        bool first = true;
        for (const auto &param : params) {
            if (!first) o << ",";
            fill(param.idFodder, !first, true);
            o << unparse_id(param.id);
            if (param.expr != nullptr) {
                // default arg, no spacing: x=e
                fill(param.eqFodder, false, false);
                o << "=";
                unparse(param.expr, false);
            }
            fill(param.commaFodder, false, false);
            first = false;
        }
        if (trailing_comma)
            o << ",";
        fill(fodder_r, false, false);
        o << ")";
    }

    void unparseFieldParams(const ObjectField &field)
    {
        if (field.methodSugar) {
            unparseParams(field.fodderL, field.params, field.trailingComma, field.fodderR);
        }
    }

    void unparseFields(const ObjectFields &fields, bool space_before)
    {
        bool first = true;
        for (const auto &field : fields) {

            if (!first) o << ',';

            switch (field.kind) {
                case ObjectField::LOCAL: {
                    fill(field.fodder1, !first || space_before, true);
                    o << "local";
                    fill(field.fodder2, true, true);
                    o << unparse_id(field.id);
                    unparseFieldParams(field);
                    fill(field.opFodder, true, true);
                    o << "=";
                    unparse(field.expr2, true);
                } break;

                case ObjectField::FIELD_ID:
                case ObjectField::FIELD_STR:
                case ObjectField::FIELD_EXPR: {


                    if (field.kind == ObjectField::FIELD_ID) {
                        fill(field.fodder1, !first || space_before, true);
                        o << unparse_id(field.id);

                    } else if (field.kind == ObjectField::FIELD_STR) {
                        unparse(field.expr1, !first || space_before);

                    } else if (field.kind == ObjectField::FIELD_EXPR) {
                        fill(field.fodder1, !first || space_before, true);
                        o << "[";
                        unparse(field.expr1, false);
                        fill(field.fodder2, false, false);
                        o << "]";
                    }
                    unparseFieldParams(field);

                    fill(field.opFodder, false, false);

                    if (field.superSugar) o << "+";
                    switch (field.hide) {
                        case ObjectField::INHERIT: o << ":"; break;
                        case ObjectField::HIDDEN: o << "::"; break;
                        case ObjectField::VISIBLE: o << ":::"; break;
                    }
                    unparse(field.expr2, true);

                } break;

                case ObjectField::ASSERT: {
                    fill(field.fodder1, !first || space_before, true);
                    o << "assert";
                    unparse(field.expr2, true);
                    if (field.expr3 != nullptr) {
                        fill(field.opFodder, true, true);
                        o << ":";
                        unparse(field.expr3, true);
                    }
                } break;
            }

            first = false;
            fill(field.commaFodder, false, false);
        }
    }

    /** Unparse the given AST.
     *
     * \param ast_ The AST to be unparsed.
     *
     * \param precedence The precedence of the enclosing AST.  If this is greater than the current
     * precedence, parens are not needed.
     */
    void unparse(const AST *ast_, bool space_before)
    {
        bool separate_token = !left_recursive(ast_);

        fill(ast_->openFodder, space_before, separate_token);

        if (auto *ast = dynamic_cast<const Apply*>(ast_)) {
            unparse(ast->target, space_before);
            fill(ast->fodderL, false, false);
            o << "(";
            bool first = true;
            for (const auto &arg : ast->args) {
                if (!first) o << ',';
                bool space = !first;
                if (arg.id != nullptr) {
                    fill(arg.idFodder, space, true);
                    o << unparse_id(arg.id);
                    space = false;
                    o << "=";
                }
                unparse(arg.expr, space);
                fill(arg.commaFodder, false, false);
                first = false;
            }
            if (ast->trailingComma) o << ",";
            fill(ast->fodderR, false, false);
            o << ")";
            if (ast->tailstrict) {
                fill(ast->tailstrictFodder, true, true);
                o << "tailstrict";
            }

        } else if (auto *ast = dynamic_cast<const ApplyBrace*>(ast_)) {
            unparse(ast->left, space_before);
            unparse(ast->right, true);

        } else if (auto *ast = dynamic_cast<const Array*>(ast_)) {
            o << "[";
            bool first = true;
            for (const auto &element : ast->elements) {
                if (!first) o << ',';
                unparse(element.expr, !first || opts.padArrays);
                fill(element.commaFodder, false, false);
                first = false;
            }
            if (ast->trailingComma) o << ",";
            fill(ast->closeFodder, ast->elements.size() > 0, opts.padArrays);
            o << "]";

        } else if (auto *ast = dynamic_cast<const ArrayComprehension*>(ast_)) {
            o << "[";
            unparse(ast->body, opts.padArrays);
            fill(ast->commaFodder, false, false);
            if (ast->trailingComma) o << ",";
            unparseSpecs(ast->specs);
            fill(ast->closeFodder, true, opts.padArrays);
            o << "]";

        } else if (auto *ast = dynamic_cast<const Assert*>(ast_)) {
            o << "assert";
            unparse(ast->cond, true);
            if (ast->message != nullptr) {
                fill(ast->colonFodder, true, true);
                o << ":";
                unparse(ast->message, true);
            }
            fill(ast->semicolonFodder, false, false);
            o << ";";
            unparse(ast->rest, true);

        } else if (auto *ast = dynamic_cast<const Binary*>(ast_)) {
            unparse(ast->left, space_before);
            fill(ast->opFodder, true, true);
            o << bop_string(ast->op);
            // The - 1 is for left associativity.
            unparse(ast->right, true);

        } else if (auto *ast = dynamic_cast<const BuiltinFunction*>(ast_)) {
            o << "/* builtin " << ast->name << " */ null";

        } else if (auto *ast = dynamic_cast<const Conditional*>(ast_)) {
            o << "if";
            unparse(ast->cond, true);
            fill(ast->thenFodder, true, true);
            o << "then";
            if (ast->branchFalse != nullptr) {
                unparse(ast->branchTrue, true);
                fill(ast->elseFodder, true, true);
                o << "else";
                unparse(ast->branchFalse, true);
            } else {
                unparse(ast->branchTrue, true);
            }

        } else if (dynamic_cast<const Dollar*>(ast_)) {
            o << "$";

        } else if (auto *ast = dynamic_cast<const Error*>(ast_)) {
            o << "error";
            unparse(ast->expr, true);

        } else if (auto *ast = dynamic_cast<const Function*>(ast_)) {
            o << "function";
            unparseParams(ast->parenLeftFodder, ast->params, ast->trailingComma,
                          ast->parenRightFodder);
            unparse(ast->body, true);

        } else if (auto *ast = dynamic_cast<const Import*>(ast_)) {
            o << "import";
            unparse(ast->file, true);

        } else if (auto *ast = dynamic_cast<const Importstr*>(ast_)) {
            o << "importstr";
            unparse(ast->file, true);

        } else if (auto *ast = dynamic_cast<const Index*>(ast_)) {
            unparse(ast->target, space_before);
            fill(ast->dotFodder, false, false);
            if (ast->id != nullptr) {
                o << ".";
                fill(ast->idFodder, false, false);
                o << unparse_id(ast->id);
            } else {
                o << "[";
                if (ast->isSlice) {
                    if (ast->index != nullptr) {
                        unparse(ast->index, false);
                    }
                    fill(ast->endColonFodder, false, false);
                    o << ":";
                    if (ast->end != nullptr) {
                        unparse(ast->end, false);
                    }
                    if (ast->step != nullptr || ast->stepColonFodder.size() > 0) {
                        fill(ast->stepColonFodder, false, false);
                        o << ":";
                        if (ast->step != nullptr) {
                            unparse(ast->step, false);
                        }
                    }
                } else {
                    unparse(ast->index, false);
                }
                fill(ast->idFodder, false, false);
                o << "]";
            }

        } else if (auto *ast = dynamic_cast<const Local*>(ast_)) {
            o << "local";
            assert(ast->binds.size() > 0);
            bool first = true;
            for (const auto &bind : ast->binds) {
                if (!first)
                    o << ",";
                first = false;
                fill(bind.varFodder, true, true);
                o << unparse_id(bind.var);
                if (bind.functionSugar) {
                    unparseParams(bind.parenLeftFodder, bind.params, bind.trailingComma,
                                  bind.parenRightFodder);
                } 
                fill(bind.opFodder, true, true);
                o << "=";
                unparse(bind.body, true);
                fill(bind.closeFodder, false, false);
            }
            o << ";";
            unparse(ast->body, true);

        } else if (auto *ast = dynamic_cast<const LiteralBoolean*>(ast_)) {
            o << (ast->value ? "true" : "false");

        } else if (auto *ast = dynamic_cast<const LiteralNumber*>(ast_)) {
            o << ast->originalString;

        } else if (auto *ast = dynamic_cast<const LiteralString*>(ast_)) {
            if (ast->tokenKind == LiteralString::DOUBLE) {
                o << "\"";
                o << encode_utf8(ast->value);
                o << "\"";
            } else if (ast->tokenKind == LiteralString::SINGLE) {
                o << "'";
                o << encode_utf8(ast->value);
                o << "'";
            } else if (ast->tokenKind == LiteralString::BLOCK) {
                o << "|||\n";
                if (ast->value.c_str()[0] != U'\n')
                    o << ast->blockIndent;
                for (const char32_t *cp = ast->value.c_str() ; *cp != U'\0' ; ++cp) {
                    std::string utf8;
                    encode_utf8(*cp, utf8);
                    o << utf8;
                    if (*cp == U'\n' && *(cp + 1) != U'\n' && *(cp + 1) != U'\0') {
                        o << ast->blockIndent;
                    }
                }
                o << ast->blockTermIndent << "|||";
            }

        } else if (dynamic_cast<const LiteralNull*>(ast_)) {
            o << "null";

        } else if (auto *ast = dynamic_cast<const Object*>(ast_)) {
            o << "{";
            unparseFields(ast->fields, opts.padObjects);
            if (ast->trailingComma) o << ",";
            fill(ast->closeFodder, ast->fields.size() > 0, opts.padObjects);
            o << "}";

        } else if (auto *ast = dynamic_cast<const DesugaredObject*>(ast_)) {
            o << "{";
            for (AST *assert : ast->asserts) {
                o << "assert";
                unparse(assert, true);
                o << ",";
            }
            for (auto &field : ast->fields) {
                o << "[";
                unparse(field.name, false);
                o << "]";
                switch (field.hide) {
                    case ObjectField::INHERIT: o << ":"; break;
                    case ObjectField::HIDDEN: o << "::"; break;
                    case ObjectField::VISIBLE: o << ":::"; break;
                }
                unparse(field.body, true);
                o << ",";
            }
            o << "}";

        } else if (auto *ast = dynamic_cast<const ObjectComprehension*>(ast_)) {
            o << "{";
            unparseFields(ast->fields, opts.padObjects);
            if (ast->trailingComma) o << ",";
            unparseSpecs(ast->specs);
            fill(ast->closeFodder, true, opts.padObjects);
            o << "}";

        } else if (auto *ast = dynamic_cast<const ObjectComprehensionSimple*>(ast_)) {
            o << "{[";
            unparse(ast->field, false);
            o << "]:";
            unparse(ast->value, true);
            o << " for " << unparse_id(ast->id) << " in";
            unparse(ast->array, true);
            o << "}";

        } else if (auto *ast = dynamic_cast<const Parens*>(ast_)) {
            o << "(";
            unparse(ast->expr, false);
            fill(ast->closeFodder, false, false);
            o << ")";

        } else if (dynamic_cast<const Self*>(ast_)) {
            o << "self";

        } else if (auto *ast = dynamic_cast<const SuperIndex*>(ast_)) {
            o << "super";
            fill(ast->dotFodder, false, false);
            if (ast->id != nullptr) {
                o << ".";
                fill(ast->idFodder, false, false);
                o << unparse_id(ast->id);
            } else {
                o << "[";
                unparse(ast->index, false);
                fill(ast->idFodder, false, false);
                o << "]";
            }

        } else if (auto *ast = dynamic_cast<const Unary*>(ast_)) {
            o << uop_string(ast->op);
            unparse(ast->expr, false);

        } else if (auto *ast = dynamic_cast<const Var*>(ast_)) {
            o << encode_utf8(ast->id->name);

        } else {
            std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
            std::abort();

        }
    }
};


/********************************************************************************
 * The rest of this file contains transformations on the ASTs before unparsing. *
 ********************************************************************************/

/** As a + b but preserves constraints.
 *
 * Namely, a LINE_END is not allowed to follow a PARAGRAPH or a LINE_END.
 */
static Fodder concat_fodder(const Fodder &a, const Fodder &b)
{
    if (a.size() == 0) return b;
    if (b.size() == 0) return a;
    Fodder r = a;
    // Add the first element of b somehow.
    if (r[a.size() - 1].kind != FodderElement::INTERSTITIAL &&
        b[0].kind == FodderElement::LINE_END) {
        if (b[0].comment.size() > 0) {
            // The line end had a comment, so create a single line paragraph for it.
            r.emplace_back(FodderElement::PARAGRAPH, b[0].blanks, b[0].indent, b[0].comment);
        } else {
            // Merge it into the previous line end.
            r[r.size() - 1].indent = b[0].indent;
            r[r.size() - 1].blanks += b[0].blanks;
        }
    } else {
        r.push_back(b[0]);
    }
    // Add the rest of b.
    for (unsigned i = 1; i < b.size() ; ++i) {
        r.push_back(b[i]);
    }
    return r;
}

/** Move b to the front of a. */
static void fodder_move_front(Fodder &a, Fodder &b)
{
    a = concat_fodder(b, a);
    b.clear();
}

/** A generic Pass that does nothing but can be extended to easily define real passes.
 */
class Pass {
    public:

    protected:
    Allocator &alloc;
    FmtOpts opts;

    public:
    Pass(Allocator &alloc, const FmtOpts &opts)
     : alloc(alloc), opts(opts) { }

    virtual void fodderElement(FodderElement &f)
    {
        (void) f;
    }

    virtual void fodder(Fodder &fodder)
    {
        for (auto &f : fodder)
            fodderElement(f);
    }

    virtual void specs(std::vector<ComprehensionSpec> &specs)
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

    virtual void params(Fodder &fodder_l, ArgParams &params, Fodder &fodder_r)
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

    virtual void fieldParams(ObjectField &field)
    {
        if (field.methodSugar) {
            params(field.fodderL, field.params, field.fodderR);
        }
    }

    virtual void fields(ObjectFields &fields)
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

    virtual void expr(AST *&ast_)
    {
        fodder(ast_->openFodder);
        visitExpr(ast_);
    }

    virtual void visit(Apply *ast)
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

    virtual void visit(ApplyBrace *ast)
    {
        expr(ast->left);
        expr(ast->right);
    }

    virtual void visit(Array *ast)
    {
        for (auto &element : ast->elements) {
            expr(element.expr);
            fodder(element.commaFodder);
        }
        fodder(ast->closeFodder);
    }

    virtual void visit(ArrayComprehension *ast)
    {
        expr(ast->body);
        fodder(ast->commaFodder);
        specs(ast->specs);
        fodder(ast->closeFodder);
    }

    virtual void visit(Assert *ast)
    {
        expr(ast->cond);
        if (ast->message != nullptr) {
            fodder(ast->colonFodder);
            expr(ast->message);
        }
        fodder(ast->semicolonFodder);
        expr(ast->rest);
    }

    virtual void visit(Binary *ast)
    {
        expr(ast->left);
        fodder(ast->opFodder);
        expr(ast->right);
    }

    virtual void visit(BuiltinFunction *) { }

    virtual void visit(Conditional *ast)
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

    virtual void visit(Dollar *) { }

    virtual void visit(Error *ast)
    {
        expr(ast->expr);
    }

    virtual void visit(Function *ast)
    {
        params(ast->parenLeftFodder, ast->params, ast->parenRightFodder);
        expr(ast->body);
    }

    virtual void visit(Import *ast)
    {
        visit(ast->file);
    }

    virtual void visit(Importstr *ast)
    {
        visit(ast->file);
    }

    virtual void visit(Index *ast)
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

    virtual void visit(Local *ast)
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

    virtual void visit(LiteralBoolean *) { }

    virtual void visit(LiteralNumber *) { }

    virtual void visit(LiteralString *) { }

    virtual void visit(LiteralNull *) { }

    virtual void visit(Object *ast)
    {
        fields(ast->fields);
        fodder(ast->closeFodder);
    }

    virtual void visit(DesugaredObject *ast)
    {
        for (AST *assert : ast->asserts) {
            expr(assert);
        }
        for (auto &field : ast->fields) {
            expr(field.name);
            expr(field.body);
        }
    }

    virtual void visit(ObjectComprehension *ast)
    {
        fields(ast->fields);
        specs(ast->specs);
        fodder(ast->closeFodder);
    }

    virtual void visit(ObjectComprehensionSimple *ast)
    {
        expr(ast->field);
        expr(ast->value);
        expr(ast->array);
    }

    virtual void visit(Parens *ast)
    {
        expr(ast->expr);
        fodder(ast->closeFodder);
    }

    virtual void visit(Self *) { }

    virtual void visit(SuperIndex *ast)
    {
        if (ast->id != nullptr) {
        } else {
            expr(ast->index);
        }
    }

    virtual void visit(Unary *ast)
    {
        expr(ast->expr);
    }

    virtual void visit(Var *) { }

    virtual void visitExpr(AST *&ast_)
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

    virtual void file(AST *&body, Fodder &final_fodder)
    {
        expr(body);
        fodder(final_fodder);
    }
};


class EnforceStringStyle : public Pass {
    using Pass::visit;
    public:
    EnforceStringStyle(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    void visit(LiteralString *lit)
    {
        if (lit->tokenKind == LiteralString::BLOCK) return;
        String canonical = jsonnet_string_unescape(lit->location, lit->value);
        unsigned num_single = 0, num_double = 0;
        for (char32_t c : canonical) {
            if (c == '\'') num_single++;
            if (c == '"') num_double++;
        }
        if (num_single > 0 && num_double > 0) return;  // Don't change it.
        bool use_single = opts.stringStyle == 's';
        if (num_single > 0)
            use_single = false;
        if (num_double > 0)
            use_single = true;

        // Change it.
        lit->value = jsonnet_string_escape(canonical, use_single);
        lit->tokenKind = use_single ? LiteralString::SINGLE : LiteralString::DOUBLE;
    }
};

class EnforceCommentStyle : public Pass {
    public:
    bool firstFodder;
    EnforceCommentStyle(Allocator &alloc, const FmtOpts &opts)
      : Pass(alloc, opts), firstFodder(true)
    { }
    /** Change the comment to match the given style, but don't break she-bang.
     *
     * If preserve_hash is true, do not touch a comment that starts with #!.
     */
    void fixComment(std::string &s, bool preserve_hash)
    {
        if (opts.commentStyle == 'h' && s[0] == '/') {
            s = "#" + s.substr(2);
        }
        if (opts.commentStyle == 's' && s[0] == '#') {
            if (preserve_hash && s[1] == '!') return;
            s = "//" + s.substr(1);
        }
    }
    void fodder(Fodder &fodder)
    {
        for (auto &f : fodder) {
            switch (f.kind) {
                case FodderElement::LINE_END:
                case FodderElement::PARAGRAPH:
                if (f.comment.size() == 1) {
                    fixComment(f.comment[0], firstFodder);
                }
                break;

                case FodderElement::INTERSTITIAL:
                break;
            }
            firstFodder = false;
        }
    }
};

class EnforceMaximumBlankLines : public Pass {
    public:
    EnforceMaximumBlankLines(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    void fodderElement(FodderElement &f)
    {
        if (f.kind != FodderElement::INTERSTITIAL)
            if (f.blanks > 2) f.blanks = 2;
    }
};

class StripComments : public Pass {
    public:
    StripComments(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    void fodder(Fodder &fodder)
    {
        Fodder copy = fodder;
        fodder.clear();
        for (auto &f : copy) {
            if (f.kind == FodderElement::LINE_END)
                fodder.push_back(f);
        }
    }
};

class StripEverything : public Pass {
    public:
    StripEverything(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    void fodder(Fodder &fodder) { fodder.clear(); }
};

class StripAllButComments : public Pass {
    public:
    StripAllButComments(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    Fodder comments;
    void fodder(Fodder &fodder)
    {
        for (auto &f : fodder) {
            if (f.kind == FodderElement::PARAGRAPH) {
                comments.emplace_back(FodderElement::PARAGRAPH, 0, 0, f.comment);
            } else if (f.kind == FodderElement::INTERSTITIAL) {
                comments.push_back(f);
                comments.emplace_back(FodderElement::LINE_END, 0, 0, std::vector<std::string>{});
            }
        }
        fodder.clear();
    }
    virtual void file(AST *&body, Fodder &final_fodder)
    {
        expr(body);
        fodder(final_fodder);
        body = alloc.make<LiteralNull>(body->location, comments);
        final_fodder.clear();
    }
};

/** These cases are infix so we descend on the left to find the fodder. */
static Fodder &open_fodder(AST *ast_)
{
    AST *left = left_recursive(ast_);
    return left != nullptr ? open_fodder(left) : ast_->openFodder;
}
static const Fodder &open_fodder(const AST *ast_)
{
    return open_fodder(const_cast<AST*>(ast_));
}

/** Strip blank lines from the top of the file. */
void remove_initial_newlines(AST *ast)
{
    Fodder &f = open_fodder(ast);
    while (f.size() > 0 && f[0].kind == FodderElement::LINE_END)
        f.erase(f.begin());
}

bool contains_newline(const Fodder &fodder)
{
    for (const auto &f : fodder) {
        if (f.kind != FodderElement::INTERSTITIAL)
            return true;
    }
    return false;
}

/* Commas should appear at the end of an object/array only if the closing token is on a new line. */
class FixTrailingCommas : public Pass {
    using Pass::visit;
    public:
    FixTrailingCommas(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    Fodder comments;

    // Generalized fix that works across a range of ASTs.
    void fix_comma(Fodder &last_comma_fodder, bool &trailing_comma, Fodder &close_fodder)
    {
        bool need_comma = contains_newline(close_fodder) || contains_newline(last_comma_fodder);
        if (trailing_comma) {
            if (!need_comma) {
                // Remove it but keep fodder.
                trailing_comma = false;
                fodder_move_front(close_fodder, last_comma_fodder);
            } else if (contains_newline(last_comma_fodder)) {
                // The comma is needed but currently is separated by a newline.
                fodder_move_front(close_fodder, last_comma_fodder);
            }
        } else {
            if (need_comma) {
                // There was no comma, but there was a newline before the ] so add a comma.
                trailing_comma = true;
            }
        }
    }

    void remove_comma(Fodder &last_comma_fodder, bool &trailing_comma, Fodder &close_fodder)
    {
        if (trailing_comma) {
            // Remove it but keep fodder.
            trailing_comma = false;
            close_fodder = concat_fodder(last_comma_fodder, close_fodder);
            last_comma_fodder.clear();
        }
    }

    void visit(Array *expr)
    {
        if (expr->elements.size() == 0) {
            // No comma present and none can be added.
            return;
        }

        fix_comma(expr->elements.back().commaFodder, expr->trailingComma, expr->closeFodder);
        Pass::visit(expr);
    }

    void visit(ArrayComprehension *expr)
    {
        remove_comma(expr->commaFodder, expr->trailingComma, expr->specs[0].openFodder);
        Pass::visit(expr);
    }

    void visit(Object *expr)
    {
        if (expr->fields.size() == 0) {
            // No comma present and none can be added.
            return;
        }

        fix_comma(expr->fields.back().commaFodder, expr->trailingComma, expr->closeFodder);
        Pass::visit(expr);
    }

    void visit(ObjectComprehension *expr)
    {
        remove_comma(expr->fields.back().commaFodder, expr->trailingComma, expr->closeFodder);
        Pass::visit(expr);
    }

};


/* Remove nested parens. */
class FixParens : public Pass {
    using Pass::visit;
    public:
    FixParens(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    void visit(Parens *expr)
    {
        if (auto *body = dynamic_cast<Parens*>(expr->expr)) {
            // Deal with fodder.
            expr->expr = body->expr;
            fodder_move_front(open_fodder(body->expr), body->openFodder);
            fodder_move_front(expr->closeFodder, body->closeFodder);
        }
        Pass::visit(expr);
    }
};



/* Ensure ApplyBrace syntax sugar is used in the case of A + { }. */
class FixPlusObject : public Pass {
    using Pass::visit;
    public:
    FixPlusObject(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }
    void visitExpr(AST *&expr)
    {
        if (auto *bin_op = dynamic_cast<Binary*>(expr)) {
            // Could relax this to allow more ASTs on the LHS but this seems OK for now.
            if (dynamic_cast<Var*>(bin_op->left)
                || dynamic_cast<Index*>(bin_op->left)) {
                if (AST *rhs = dynamic_cast<Object*>(bin_op->right)) {
                    if (bin_op->op == BOP_PLUS) {
                        fodder_move_front(rhs->openFodder, bin_op->opFodder);
                        expr = alloc.make<ApplyBrace>(bin_op->location, bin_op->openFodder,
                                                      bin_op->left, rhs);
                    }
                }
            }
        }
        Pass::visitExpr(expr);
    }
};



/* Remove final colon in slices. */
class NoRedundantSliceColon : public Pass {
    using Pass::visit;
    public:
    NoRedundantSliceColon(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }

    void visit(Index *expr)
    {
        if (expr->isSlice) {
            if (expr->step == nullptr) {
                if (expr->stepColonFodder.size() > 0) {
                    fodder_move_front(expr->idFodder, expr->stepColonFodder);
                }
            }
        }
        Pass::visit(expr);
    }
};

/* Ensure syntax sugar is used where possible. */
class PrettyFieldNames : public Pass {
    using Pass::visit;
    public:
    PrettyFieldNames(Allocator &alloc, const FmtOpts &opts) : Pass(alloc, opts) { }

    bool isIdentifier(const String &str) {
        bool first = true;
        for (char32_t c : str) {
            if (!first && c >= '0' && c <= '9')
                continue;
            first = false;
            if ((c >= 'A' && c <= 'Z')
                || (c >= 'a' && c <= 'z')
                || (c == '_'))
                continue;
            return false;
        }
        // Filter out keywords.
        if (lex_get_keyword_kind(encode_utf8(str)) != Token::IDENTIFIER)
            return false;
        return true;
    }

    void visit(Index *expr)
    {
        if (!expr->isSlice && expr->index != nullptr) {
            // Maybe we can use an id instead.
            if (auto *lit = dynamic_cast<LiteralString*>(expr->index)) {
                if (isIdentifier(lit->value)) {
                    expr->id = alloc.makeIdentifier(lit->value);
                    expr->idFodder = lit->openFodder;
                    expr->index = nullptr;
                }
            }
        }
        Pass::visit(expr);
    }

    void visit(Object *expr)
    {
        for (auto &field : expr->fields) {
            // First try ["foo"] -> "foo".
            if (field.kind == ObjectField::FIELD_EXPR) {
                if (auto *field_expr = dynamic_cast<LiteralString*>(field.expr1)) {
                    field.kind = ObjectField::FIELD_STR;
                    fodder_move_front(field_expr->openFodder, field.fodder1);
                    if (field.methodSugar) {
                        fodder_move_front(field.fodderL, field.fodder2);
                    } else {
                        fodder_move_front(field.opFodder, field.fodder2);
                    }
                }
            }
            // Then try "foo" -> foo.
            if (field.kind == ObjectField::FIELD_STR) {
                if (auto *lit = dynamic_cast<LiteralString*>(field.expr1)) {
                    if (isIdentifier(lit->value)) {
                        field.kind = ObjectField::FIELD_ID;
                        field.id = alloc.makeIdentifier(lit->value);
                        field.fodder1 = lit->openFodder;
                        field.expr1 = nullptr;
                    }
                }
            }
        }
        Pass::visit(expr);
    }
};

class FixIndentation {

    FmtOpts opts;
    unsigned column;

    public:
    FixIndentation(const FmtOpts &opts) : opts(opts) { }

    void fill(Fodder &fodder, bool space_before, bool separate_token,
              unsigned all_but_last_indent, unsigned last_indent)
    {
        setIndents(fodder, all_but_last_indent, last_indent);
        fodder_count(column, fodder, space_before, separate_token);
    }

    void fill(Fodder &fodder, bool space_before, bool separate_token, unsigned indent)
    {
        fill(fodder, space_before, separate_token, indent, indent);
    }

    struct Indent {
        unsigned base;
        unsigned lineUp;
        Indent(unsigned base, unsigned line_up) : base(base), lineUp(line_up) { }
    };

    /** Calculate the indentation of sub-expressions.
     *
     * If the first sub-expression is on the same line as the current node, then subsequent
     * ones will be lined up, otherwise subsequent ones will be on the next line indented
     * by 'indent'.
     */
    Indent newIndent(const Fodder &first_fodder, const Indent &old, unsigned line_up)
    {
        if (first_fodder.size() == 0 || first_fodder[0].kind == FodderElement::INTERSTITIAL) {
            return Indent(old.base, line_up);
        } else {
            // Reset
            return Indent(old.base + opts.indent, old.base + opts.indent);
        }
    }

    /** Calculate the indentation of sub-expressions.
     *
     * If the first sub-expression is on the same line as the current node, then subsequent
     * ones will be lined up and further indentations in their subexpressions will be based from
     * this column.
     * by 'indent'.
     */
    Indent newIndentStrong(const Fodder &first_fodder, const Indent &old, unsigned line_up)
    {
        if (first_fodder.size() == 0 || first_fodder[0].kind == FodderElement::INTERSTITIAL) {
            return Indent(line_up, line_up);
        } else {
            // Reset
            return Indent(old.base + opts.indent, old.base + opts.indent);
        }
    }

    Indent align(const Fodder &first_fodder, const Indent &old, unsigned line_up)
    {
        if (first_fodder.size() == 0 || first_fodder[0].kind == FodderElement::INTERSTITIAL) {
            return Indent(old.base, line_up);
        } else {
            // Reset
            return old;
        }
    }

    void setIndents(Fodder &fodder, unsigned all_but_last_indent, unsigned last_indent)
    {
        // First count how many there are.
        unsigned count = 0;
        for (const auto &f : fodder) {
            if (f.kind != FodderElement::INTERSTITIAL)
                count++;
        }
        // Now set the indent on the right ones.
        unsigned i = 0;
        for (auto &f : fodder) {
            if (f.kind != FodderElement::INTERSTITIAL) {
                if (i + 1 < count) {
                    f.indent = all_but_last_indent;
                } else {
                    assert(i == count - 1);
                    f.indent = last_indent;
                }
                i++;
            }
        }
    }

    /** Indent comprehension specs.
     * \param indent The indentation level.
     */
    void specs(std::vector<ComprehensionSpec> &specs, const Indent &indent)
    {
        for (auto &spec : specs) {
            fill(spec.openFodder, true, true, indent.lineUp);
            switch (spec.kind) {
                case ComprehensionSpec::FOR: {
                    column += 3;  // for
                    fill(spec.varFodder, true, true, indent.lineUp);
                    column += spec.var->name.length();
                    fill(spec.inFodder, true, true, indent.lineUp);
                    column += 2;  // in
                    Indent new_indent = newIndent(open_fodder(spec.expr), indent, column);
                    expr(spec.expr, new_indent, true);
                }
                break;

                case ComprehensionSpec::IF: {
                    column += 2;  // if
                    Indent new_indent = newIndent(open_fodder(spec.expr), indent, column);
                    expr(spec.expr, new_indent, true);
                }
                break;
            }
        }
    }

    void params(Fodder &fodder_l, ArgParams &params, bool trailing_comma, Fodder &fodder_r,
                const Indent &indent)
    {
        fill(fodder_l, false, false, indent.lineUp, indent.lineUp);
        column++;  // (
        const Fodder &first_inside = params.size() == 0 ? fodder_r : params[0].idFodder;

        Indent new_indent = newIndent(first_inside, indent, column);
        bool first = true;
        for (auto &param : params) {
            if (!first) column++;  // ','
            fill(param.idFodder, !first, true, new_indent.lineUp);
            column += param.id->name.length();
            if (param.expr != nullptr) {
                // default arg, no spacing: x=e
                fill(param.eqFodder, false, false, new_indent.lineUp);
                column++;
                expr(param.expr, new_indent, false);
            }
            fill(param.commaFodder, false, false, new_indent.lineUp);
            first = false;
        }
        if (trailing_comma)
            column++;
        fill(fodder_r, false, false, new_indent.lineUp, indent.lineUp);
        column++;  // )
    }

    void fieldParams(ObjectField &field, const Indent &indent)
    {
        if (field.methodSugar) {
            params(field.fodderL, field.params, field.trailingComma, field.fodderR, indent);
        }
    }

    /** Indent fields within an object.
     *
     * \params fields
     * \param indent Indent of the first field.
     * \param space_before
     */
    void fields(ObjectFields &fields, const Indent &indent, bool space_before)
    {
        unsigned new_indent = indent.lineUp;
        bool first = true;
        for (auto &field : fields) {
            if (!first) column++;  // ','

            switch (field.kind) {
                case ObjectField::LOCAL: {

                    fill(field.fodder1, !first || space_before, true, indent.lineUp);
                    column += 5;  // local
                    fill(field.fodder2, true, true, indent.lineUp);
                    column += field.id->name.length();
                    fieldParams(field, indent);
                    fill(field.opFodder, true, true, indent.lineUp);
                    column++;  // =
                    Indent new_indent2 = newIndent(open_fodder(field.expr2), indent, column);
                    expr(field.expr2, new_indent2, true);
                } break;

                case ObjectField::FIELD_ID:
                case ObjectField::FIELD_STR:
                case ObjectField::FIELD_EXPR: {

                    if (field.kind == ObjectField::FIELD_ID) {
                        fill(field.fodder1, !first || space_before, true, new_indent);
                        column += field.id->name.length();

                    } else if (field.kind == ObjectField::FIELD_STR) {
                        expr(field.expr1, indent, !first || space_before);

                    } else if (field.kind == ObjectField::FIELD_EXPR) {
                        fill(field.fodder1, !first || space_before, true, new_indent);
                        column++;  // [
                        expr(field.expr1, indent, false);
                        fill(field.fodder2, false, false, new_indent);
                        column++;  // ]
                    }

                    fieldParams(field, indent);

                    fill(field.opFodder, false, false, new_indent);

                    if (field.superSugar) column++;
                    switch (field.hide) {
                        case ObjectField::INHERIT: column+=1; break;
                        case ObjectField::HIDDEN: column+=2; break;
                        case ObjectField::VISIBLE: column+=3; break;
                    }
                    Indent new_indent2 = newIndent(open_fodder(field.expr2), indent, column);
                    expr(field.expr2, new_indent2, true);

                } break;

                case ObjectField::ASSERT: {

                    fill(field.fodder1, !first || space_before, true, new_indent);
                    column += 6;  // assert
                    expr(field.expr2, indent, true);
                    if (field.expr3 != nullptr) {
                        fill(field.opFodder, true, true, new_indent);
                        column++;  // ":"
                        expr(field.expr3, indent, true);
                    }
                } break;
            }

            first = false;
            fill(field.commaFodder, false, false, new_indent);
        }
    }

    bool hasNewLines(const Fodder &fodder)
    {
        for (const auto &f : fodder) {
            if (f.kind != FodderElement::INTERSTITIAL)
                return true;
        }
        return false;
    }

    bool hasNewLines(const AST *expr)
    {
        return hasNewLines(open_fodder(expr));
    }

    /** Reindent an expression.
     *
     * \param ast_ The ast to reindent.
     * \param indent Beginning of the line.
     * \param line_up Line up to here.
     * \param space_before As defined in the pretty-printer.
     */
    void expr(AST *ast_, const Indent &indent, bool space_before)
    {
        fill(ast_->openFodder, space_before, !left_recursive(ast_), indent.lineUp);

        if (auto *ast = dynamic_cast<Apply*>(ast_)) {
            const Fodder &init_fodder = open_fodder(ast->target);
            Indent new_indent = align(init_fodder, indent,
                                      column + (space_before ? 1 : 0));
            expr(ast->target, new_indent, space_before);
            fill(ast->fodderL, false, false, new_indent.lineUp);
            column++;  // (
            const Fodder &first_fodder = ast->args.size() == 0
                                         ? ast->fodderR
                                         : open_fodder(ast->args[0].expr);
            bool strong_indent = false;
            // Need to use strong indent if there are not newlines before any of the sub-expressions
            bool first = true;
            for (auto &arg : ast->args) {
                if (first) {
                    first = false;
                    continue;
                }
                if (hasNewLines(arg.expr)) strong_indent = true;
            }

            Indent arg_indent = strong_indent
                                ? newIndentStrong(first_fodder, indent, column)
                                : newIndent(first_fodder, indent, column);
            first = true;
            for (auto &arg : ast->args) {
                if (!first) column++;  // ","

                bool space = !first;
                if (arg.id != nullptr) {
                    fill(arg.idFodder, space, false, arg_indent.lineUp);
                    column += arg.id->name.length();
                    space = false;
                    column++;  // "="
                }
                expr(arg.expr, arg_indent, space);
                fill(arg.commaFodder, false, false, arg_indent.lineUp);
                first = false;
            }
            if (ast->trailingComma) column++;  // ","
            fill(ast->fodderR, false, false, arg_indent.lineUp, indent.base);
            column++;  // )
            if (ast->tailstrict) {
                fill(ast->tailstrictFodder, true, true, indent.base);
                column += 10;  // tailstrict
            }

        } else if (auto *ast = dynamic_cast<ApplyBrace*>(ast_)) {
            const Fodder &init_fodder = open_fodder(ast->left);
            Indent new_indent = align(init_fodder, indent,
                                      column + (space_before ? 1 : 0));
            expr(ast->left, new_indent, space_before);
            expr(ast->right, new_indent, true);

        } else if (auto *ast = dynamic_cast<Array*>(ast_)) {
            column++;  // '['
            // First fodder element exists and is a newline
            const Fodder &first_fodder = ast->elements.size() > 0
                                         ? open_fodder(ast->elements[0].expr)
                                         : ast->closeFodder;
            unsigned new_column = column + (opts.padArrays ? 1 : 0);
            bool strong_indent = false;
            // Need to use strong indent if there are not newlines before any of the sub-expressions
            bool first = true;
            for (auto &el : ast->elements) {
                if (first) {
                    first = false;
                    continue;
                }
                if (hasNewLines(el.expr)) strong_indent = true;
            }

            Indent new_indent = strong_indent
                                ? newIndentStrong(first_fodder, indent, new_column)
                                : newIndent(first_fodder, indent, new_column);
                
            first = true;
            for (auto &element : ast->elements) {
                if (!first) column++;
                expr(element.expr, new_indent, !first || opts.padArrays);
                fill(element.commaFodder, false, false, new_indent.lineUp, new_indent.lineUp);
                first = false;
            }
            if (ast->trailingComma) column++;

            // Handle penultimate newlines from expr.close_fodder if there are any.
            fill(ast->closeFodder, ast->elements.size() > 0, opts.padArrays, new_indent.lineUp,
                 indent.base);
            column++;  // ']'

        } else if (auto *ast = dynamic_cast<ArrayComprehension*>(ast_)) {
            column++;  // [
            Indent new_indent = newIndent(open_fodder(ast->body), indent,
                                          column + (opts.padArrays ? 1 : 0));
            expr(ast->body, new_indent, opts.padArrays);
            fill(ast->commaFodder, false, false, new_indent.lineUp);
            if (ast->trailingComma) column++;  // ','
            specs(ast->specs, new_indent);
            fill(ast->closeFodder, true, opts.padArrays, new_indent.lineUp, indent.base);
            column++;  // ]

        } else if (auto *ast = dynamic_cast<Assert*>(ast_)) {
            column += 6;  // assert
            // + 1 for the space after the assert
            Indent new_indent = newIndent(open_fodder(ast->cond), indent, column + 1);
            expr(ast->cond, new_indent, true);
            if (ast->message != nullptr) {
                fill(ast->colonFodder, true, true, new_indent.lineUp);
                column++;  // ":"
                expr(ast->message, indent, true);
            }
            fill(ast->semicolonFodder, false, false, new_indent.lineUp);
            column++;  // ";"
            expr(ast->rest, indent, true);

        } else if (auto *ast = dynamic_cast<Binary*>(ast_)) {
            const Fodder &first_fodder = open_fodder(ast->left);
            Indent new_indent = align(first_fodder, indent,
                                      column + (space_before ? 1 : 0));
            expr(ast->left, new_indent, space_before);
            fill(ast->opFodder, true, true, new_indent.lineUp);
            column += bop_string(ast->op).length();
            // Don't calculate a new indent for here, because we like being able to do:
            // true &&
            // true &&
            // true
            expr(ast->right, new_indent, true);

        } else if (auto *ast = dynamic_cast<BuiltinFunction*>(ast_)) {
            column += 11;  // "/* builtin "
            column += ast->name.length();
            column += 8;  // " */ null"

        } else if (auto *ast = dynamic_cast<Conditional*>(ast_)) {
            column += 2;  // if
            Indent cond_indent = newIndent(open_fodder(ast->cond), indent, column + 1);
            expr(ast->cond, cond_indent, true);
            fill(ast->thenFodder, true, true, indent.base);
            column += 4;  // then
            Indent true_indent = newIndent(open_fodder(ast->branchTrue), indent, column + 1);
            expr(ast->branchTrue, true_indent, true);
            if (ast->branchFalse != nullptr) {
                fill(ast->elseFodder, true, true, indent.base);
                column += 4;  // else
                Indent false_indent = newIndent(open_fodder(ast->branchFalse), indent, column + 1);
                expr(ast->branchFalse, false_indent, true);
            }

        } else if (dynamic_cast<Dollar*>(ast_)) {
            column++;  // $

        } else if (auto *ast = dynamic_cast<Error*>(ast_)) {
            column += 5;  // error
            Indent new_indent = newIndent(open_fodder(ast->expr), indent, column + 1);
            expr(ast->expr, new_indent, true);

        } else if (auto *ast = dynamic_cast<Function*>(ast_)) {
            column += 8;  // function
            params(ast->parenLeftFodder, ast->params, ast->trailingComma,
                   ast->parenRightFodder, indent);
            Indent new_indent = newIndent(open_fodder(ast->body), indent, column + 1);
            expr(ast->body, new_indent, true);

        } else if (auto *ast = dynamic_cast<Import*>(ast_)) {
            column += 6;  // import
            Indent new_indent = newIndent(open_fodder(ast->file), indent, column + 1);
            expr(ast->file, new_indent, true);

        } else if (auto *ast = dynamic_cast<Importstr*>(ast_)) {
            column += 9;  // importstr
            Indent new_indent = newIndent(open_fodder(ast->file), indent, column + 1);
            expr(ast->file, new_indent, true);

        } else if (auto *ast = dynamic_cast<Index*>(ast_)) {
            expr(ast->target, indent, space_before);
            fill(ast->dotFodder, false, false, indent.lineUp);
            if (ast->id != nullptr) {
                Indent new_indent = newIndent(ast->idFodder, indent, column);
                column++;  // ".";
                fill(ast->idFodder, false, false, new_indent.lineUp);
                column += ast->id->name.length();
            } else {
                column++;  // "[";
                Indent new_indent = newIndent(open_fodder(ast->index), indent, column);
                expr(ast->index, new_indent, false);
                fill(ast->idFodder, false, false, new_indent.lineUp, indent.base);
                column++;  // "]";
            }

        } else if (auto *ast = dynamic_cast<Local*>(ast_)) {
            column += 5;  // local
            assert(ast->binds.size() > 0);
            bool first = true;
            Indent new_indent = newIndent(ast->binds[0].varFodder, indent, column + 1);
            for (auto &bind : ast->binds) {
                if (!first)
                    column++;  // ','
                first = false;
                fill(bind.varFodder, true, true, new_indent.lineUp);
                column += bind.var->name.length();
                if (bind.functionSugar) {
                    params(bind.parenLeftFodder, bind.params, bind.trailingComma,
                           bind.parenRightFodder, new_indent);
                } 
                fill(bind.opFodder, true, true, new_indent.lineUp);
                column++;  // '='
                Indent new_indent2 = newIndent(open_fodder(bind.body), new_indent, column + 1);
                expr(bind.body, new_indent2, true);
                fill(bind.closeFodder, false, false, new_indent2.lineUp, indent.base);
            }
            column++;  // ';'
            expr(ast->body, indent, true);

        } else if (auto *ast = dynamic_cast<LiteralBoolean*>(ast_)) {
            column += (ast->value ? 4 : 5);

        } else if (auto *ast = dynamic_cast<LiteralNumber*>(ast_)) {
            column += ast->originalString.length();

        } else if (auto *ast = dynamic_cast<LiteralString*>(ast_)) {
            if (ast->tokenKind == LiteralString::DOUBLE) {
                column += 2 + ast->value.length();  // Include quotes
            } else if (ast->tokenKind == LiteralString::SINGLE) {
                column += 2 + ast->value.length();  // Include quotes
            } else if (ast->tokenKind == LiteralString::BLOCK) {
                ast->blockIndent = std::string(indent.base + opts.indent, ' ');
                ast->blockTermIndent = std::string(indent.base, ' ');
                column = indent.base;  // blockTermIndent
                column += 3;  // "|||"
            }

        } else if (dynamic_cast<LiteralNull*>(ast_)) {
            column += 4;  // null

        } else if (auto *ast = dynamic_cast<Object*>(ast_)) {
            column++;  // '{'
            const Fodder &first_fodder = ast->fields.size() == 0
                                         ? ast->closeFodder
                                         : ast->fields[0].kind == ObjectField::FIELD_STR
                                           ? open_fodder(ast->fields[0].expr1)
                                           : ast->fields[0].fodder1;
            Indent new_indent = newIndent(first_fodder, indent,
                                          column + (opts.padObjects ? 1 : 0));

            fields(ast->fields, new_indent, opts.padObjects);
            if (ast->trailingComma) column++;
            fill(ast->closeFodder, ast->fields.size() > 0, opts.padObjects,
                 new_indent.lineUp, indent.base);
            column++;  // '}'

        } else if (auto *ast = dynamic_cast<DesugaredObject*>(ast_)) {
            // No fodder but need to recurse and maintain column counter.
            column++;  // '{'
            for (AST *assert : ast->asserts) {
                column += 6;  // assert
                expr(assert, indent, true);
                column++;  // ','
            }
            for (auto &field : ast->fields) {
                column++;  // '['
                expr(field.name, indent, false);
                column++;  // ']'
                switch (field.hide) {
                    case ObjectField::INHERIT: column += 1; break;
                    case ObjectField::HIDDEN: column += 2; break;
                    case ObjectField::VISIBLE: column += 3; break;
                }
                expr(field.body, indent, true);
            }
            column++;  // '}'

        } else if (auto *ast = dynamic_cast<ObjectComprehension*>(ast_)) {
            column++;  // '{'
            unsigned start_column = column;
            const Fodder &first_fodder = ast->fields.size() == 0
                                         ? ast->closeFodder
                                         : ast->fields[0].kind == ObjectField::FIELD_STR
                                           ? open_fodder(ast->fields[0].expr1)
                                           : ast->fields[0].fodder1;
            Indent new_indent = newIndent(first_fodder, indent,
                                          start_column + (opts.padObjects ? 1 : 0));

            fields(ast->fields, new_indent, opts.padObjects);
            if (ast->trailingComma) column++;  // ','
            specs(ast->specs, new_indent);
            fill(ast->closeFodder, true, opts.padObjects, new_indent.lineUp, indent.base);
            column++;  // '}'

        } else if (auto *ast = dynamic_cast<ObjectComprehensionSimple*>(ast_)) {
            column++;  // '{'
            column++;  // '['
            expr(ast->field, indent, false);
            column++;  // ']'
            column++;  // ':'
            expr(ast->value, indent, true);
            column += 5;  // " for "
            column += ast->id->name.length();
            column += 3;  // " in"
            expr(ast->array, indent, true);
            column++;  // '}'

        } else if (auto *ast = dynamic_cast<Parens*>(ast_)) {
            column++;  // (
            Indent new_indent = newIndent(open_fodder(ast->expr), indent, column);
            expr(ast->expr, new_indent, false);
            fill(ast->closeFodder, false, false, new_indent.lineUp, indent.base);
            column++;  // )

        } else if (dynamic_cast<const Self*>(ast_)) {
            column += 4;  // self

        } else if (auto *ast = dynamic_cast<SuperIndex*>(ast_)) {
            column += 5;  // super
            fill(ast->dotFodder, false, false, indent.lineUp);
            if (ast->id != nullptr) {
                column++;  // ".";
                Indent new_indent = newIndent(ast->idFodder, indent, column);
                fill(ast->idFodder, false, false, new_indent.lineUp);
                column += ast->id->name.length();
            } else {
                column++;  // "[";
                Indent new_indent = newIndent(open_fodder(ast->index), indent, column);
                expr(ast->index, new_indent, false);
                fill(ast->idFodder, false, false, new_indent.lineUp, indent.base);
                column++;  // "]";
            }

        } else if (auto *ast = dynamic_cast<Unary*>(ast_)) {
            column += uop_string(ast->op).length();
            Indent new_indent = newIndent(open_fodder(ast->expr), indent, column);
            expr(ast->expr, new_indent, false);

        } else if (auto *ast = dynamic_cast<Var*>(ast_)) {
            column += ast->id->name.length();

        } else {
            std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_ << std::endl;
            std::abort();

        }
    }
    virtual void file(AST *body, Fodder &final_fodder)
    {
        expr(body, Indent(0, 0), false);
        setIndents(final_fodder, 0, 0);
    }
};



// TODO(dcunnin): Add pass to alphabeticize top level imports.


std::string jsonnet_fmt(AST *ast, Fodder &final_fodder, const FmtOpts &opts)
{
    Allocator alloc;

    // Passes to enforce style on the AST.
    remove_initial_newlines(ast);
    if (opts.maxBlankLines > 0)
        EnforceMaximumBlankLines(alloc, opts).file(ast, final_fodder);
    FixTrailingCommas(alloc, opts).file(ast, final_fodder);
    FixParens(alloc, opts).file(ast, final_fodder);
    FixPlusObject(alloc, opts).file(ast, final_fodder);
    NoRedundantSliceColon(alloc, opts).file(ast, final_fodder);
    if (opts.stripComments)
        StripComments(alloc, opts).file(ast, final_fodder);
    else if (opts.stripAllButComments)
        StripAllButComments(alloc, opts).file(ast, final_fodder);
    else if (opts.stripEverything)
        StripEverything(alloc, opts).file(ast, final_fodder);
    if (opts.prettyFieldNames)
        PrettyFieldNames(alloc, opts).file(ast, final_fodder);
    if (opts.stringStyle != 'l')
        EnforceStringStyle(alloc, opts).file(ast, final_fodder);
    if (opts.commentStyle != 'l')
        EnforceCommentStyle(alloc, opts).file(ast, final_fodder);
    if (opts.indent > 0)
        FixIndentation(opts).file(ast, final_fodder);

    std::stringstream ss;
    Unparser unparser(ss, opts);
    unparser.unparse(ast, false);
    unparser.fill(final_fodder, true, false);
    return ss.str();
}
