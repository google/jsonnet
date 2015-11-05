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

#ifndef JSONNET_AST_H
#define JSONNET_AST_H

#include <cstdlib>
#include <cassert>

#include <iostream>
#include <string>
#include <map>
#include <vector>

#include "lexer.h"
#include "unicode.h"

enum ASTType {
    AST_APPLY,
    AST_ARRAY,
    AST_ARRAY_COMPREHENSION,
    AST_ARRAY_COMPREHENSION_SIMPLE,
    AST_ASSERT,
    AST_BINARY,
    AST_BUILTIN_FUNCTION,
    AST_CONDITIONAL,
    AST_DOLLAR,
    AST_ERROR,
    AST_FUNCTION,
    AST_IMPORT,
    AST_IMPORTSTR,
    AST_INDEX,
    AST_LOCAL,
    AST_LITERAL_BOOLEAN,
    AST_LITERAL_NULL,
    AST_LITERAL_NUMBER,
    AST_LITERAL_STRING,
    AST_OBJECT,
    AST_DESUGARED_OBJECT,
    AST_OBJECT_COMPREHENSION,
    AST_OBJECT_COMPREHENSION_SIMPLE,
    AST_SELF,
    AST_SUPER_INDEX,
    AST_UNARY,
    AST_VAR
};

/** Represents a variable / parameter / field name. */
struct Identifier {
    String name;
    Identifier(const String &name)
      : name(name)
    { }
};

static inline std::ostream &operator<<(std::ostream &o, const Identifier *id)
{
    o << encode_utf8(id->name);
    return o;
}

typedef std::vector<const Identifier *> Identifiers;


/** All AST nodes are subtypes of this class.
 */
struct AST {
    LocationRange location;
    ASTType type;
    Identifiers freeVariables;
    AST(const LocationRange &location, ASTType type)
      : location(location), type(type)
    {
    }
    virtual ~AST(void)
    {
    }
};

typedef std::vector<AST*> ASTs;

/** Used in Object & Array Comprehensions. */
struct ComprehensionSpec {
    enum Kind {
        FOR,
        IF
    };
    Kind kind;
    const Identifier *var;  // Null when kind != SPEC_FOR.
    AST *expr;
    ComprehensionSpec(Kind kind, const Identifier *var, AST *expr)
      : kind(kind), var(var), expr(expr)
    { }
};


/** Represents function calls. */
struct Apply : public AST {
    AST *target;
    ASTs arguments;
    bool trailingComma;
    bool tailstrict;
    Apply(const LocationRange &lr, AST *target, const ASTs &arguments, bool trailing_comma,
          bool tailstrict)
      : AST(lr, AST_APPLY), target(target), arguments(arguments), trailingComma(trailing_comma),
        tailstrict(tailstrict)
    { }
};

/** Represents e { }.  Desugared to e + { }. */
struct ApplyBrace : public AST {
    AST *left;
    AST *right;  // This is always an object or object comprehension.
    ApplyBrace(const LocationRange &lr, AST *left, AST *right)
      : AST(lr, AST_BINARY), left(left), right(right)
    { }
};

/** Represents array constructors [1, 2, 3]. */
struct Array : public AST {
    ASTs elements;
    bool trailingComma;
    Array(const LocationRange &lr, const ASTs &elements, bool trailing_comma)
      : AST(lr, AST_ARRAY), elements(elements), trailingComma(trailing_comma)
    { }
};

/** Represents array comprehensions (which are like Python list comprehensions). */
struct ArrayComprehension : public AST {
    AST* body;
    bool trailingComma;
    std::vector<ComprehensionSpec> specs;
    ArrayComprehension(const LocationRange &lr, AST *body,
                       bool trailing_comma,
                       const std::vector<ComprehensionSpec> &specs)
      : AST(lr, AST_ARRAY_COMPREHENSION), body(body), trailingComma(trailing_comma), specs(specs)
    { }
};

/** Represents an assert expression (not an object-level assert).
 * 
 * After parsing, message can be nullptr indicating that no message was specified. This AST is
 * elimiated by desugaring.
 */
struct Assert : public AST {
    AST *cond;
    AST *message;
    AST *rest;
    Assert(const LocationRange &lr, AST *cond, AST *message, AST *rest)
      : AST(lr, AST_ASSERT), cond(cond), message(message), rest(rest)
    { }
};

enum BinaryOp {
    BOP_MULT,
    BOP_DIV,
    BOP_PERCENT,

    BOP_PLUS,
    BOP_MINUS,

    BOP_SHIFT_L,
    BOP_SHIFT_R,

    BOP_GREATER,
    BOP_GREATER_EQ,
    BOP_LESS,
    BOP_LESS_EQ,

    BOP_MANIFEST_EQUAL,
    BOP_MANIFEST_UNEQUAL,

    BOP_BITWISE_AND,
    BOP_BITWISE_XOR,
    BOP_BITWISE_OR,

    BOP_AND,
    BOP_OR
};

static inline std::string bop_string (BinaryOp bop)
{
    switch (bop) {
        case BOP_MULT: return "*";
        case BOP_DIV: return "/";
        case BOP_PERCENT: return "%";

        case BOP_PLUS: return "+";
        case BOP_MINUS: return "-";

        case BOP_SHIFT_L: return "<<";
        case BOP_SHIFT_R: return ">>";

        case BOP_GREATER: return ">";
        case BOP_GREATER_EQ: return ">=";
        case BOP_LESS: return "<";
        case BOP_LESS_EQ: return "<=";

        case BOP_MANIFEST_EQUAL: return "==";
        case BOP_MANIFEST_UNEQUAL: return "!=";

        case BOP_BITWISE_AND: return "&";
        case BOP_BITWISE_XOR: return "^";
        case BOP_BITWISE_OR: return "|";

        case BOP_AND: return "&&";
        case BOP_OR: return "||";

        default:
        std::cerr << "INTERNAL ERROR: Unrecognised binary operator: " << bop << std::endl;
        std::abort();
    }
}

/** Represents binary operators. */
struct Binary : public AST {
    AST *left;
    BinaryOp op;
    AST *right;
    Binary(const LocationRange &lr, AST *left, BinaryOp op, AST *right)
      : AST(lr, AST_BINARY), left(left), op(op), right(right)
    { }
};

/** Represents built-in functions.
 *
 * There is no parse rule to build this AST.  Instead, it is used to build the std object in the
 * interpreter.
 */
struct BuiltinFunction : public AST {
    unsigned long id;
    Identifiers params;
    BuiltinFunction(const LocationRange &lr, unsigned long id,
                    const Identifiers &params)
      : AST(lr, AST_BUILTIN_FUNCTION), id(id), params(params)
    { }
};

/** Represents if then else.
 * 
 * After parsing, branchFalse can be nullptr indicating that no else branch was specified.  The
 * desugarer fills this in with a LiteralNull.
 */
struct Conditional : public AST {
    AST *cond;
    AST *branchTrue;
    AST *branchFalse;
    Conditional(const LocationRange &lr, AST *cond, AST *branchTrue, AST *branchFalse)
      : AST(lr, AST_CONDITIONAL), cond(cond), branchTrue(branchTrue), branchFalse(branchFalse)
    { }
};

/** Represents the $ keyword. */
struct Dollar : public AST {
    Dollar(const LocationRange &lr)
      : AST(lr, AST_DOLLAR)
    { }
};

/** Represents error e. */
struct Error : public AST {
    AST *expr;
    Error(const LocationRange &lr, AST *expr)
      : AST(lr, AST_ERROR), expr(expr)
    { }
};

/** Represents function calls. */
struct Function : public AST {
    Identifiers parameters;
    bool trailingComma;
    AST *body;
    Function(const LocationRange &lr, const Identifiers &parameters, bool trailing_comma, AST *body)
      : AST(lr, AST_FUNCTION), parameters(parameters), trailingComma(trailing_comma), body(body)
    { }
};

/** Represents import "file". */
struct Import : public AST {
    String file;
    Import(const LocationRange &lr, const String &file)
      : AST(lr, AST_IMPORT), file(file)
    { }
};

/** Represents importstr "file". */
struct Importstr : public AST {
    String file;
    Importstr(const LocationRange &lr, const String &file)
      : AST(lr, AST_IMPORTSTR), file(file)
    { }
};

/** Represents both e[e] and the syntax sugar e.f.
 *
 * One of index and id will be nullptr before desugaring.  After desugaring id will be nullptr.
 */
struct Index : public AST {
    AST *target;
    AST *index;
    const Identifier *id;
    Index(const LocationRange &lr, AST *target, AST *index, const Identifier *id)
      : AST(lr, AST_INDEX), target(target), index(index), id(id)
    { }
};

/** Represents local x = e; e.  After desugaring, functionSugar is false. */
struct Local : public AST {
    struct Bind {
        const Identifier *var;
        AST *body;
        bool functionSugar;
        Identifiers params;  // If functionSugar == true
        bool trailingComma;
        Bind(const Identifier *var, AST *body)
          : var(var), body(body), functionSugar(false)
        { }
        Bind(const Identifier *var, AST *body, bool function_sugar, const Identifiers &params,
             bool trailing_comma)
          : var(var), body(body), functionSugar(function_sugar), params(params), trailingComma(trailing_comma)
        { }
    };
    typedef std::vector<Bind> Binds;
    Binds binds;
    AST *body;
    Local(const LocationRange &lr, const Binds &binds, AST *body)
      : AST(lr, AST_LOCAL), binds(binds), body(body)
    { }
};

/** Represents true and false. */
struct LiteralBoolean : public AST {
    bool value;
    LiteralBoolean(const LocationRange &lr, bool value)
      : AST(lr, AST_LITERAL_BOOLEAN), value(value)
    { }
};

/** Represents the null keyword. */
struct LiteralNull : public AST {
    LiteralNull(const LocationRange &lr)
      : AST(lr, AST_LITERAL_NULL)
    { }
};

/** Represents JSON numbers. */
struct LiteralNumber : public AST {
    double value;
    std::string originalString;
    LiteralNumber(const LocationRange &lr, const std::string &str)
      : AST(lr, AST_LITERAL_NUMBER), value(strtod(str.c_str(), nullptr)), originalString(str)
    { }
};

/** Represents JSON strings. */
struct LiteralString : public AST {
    String value;
    enum TokenKind { SINGLE, DOUBLE, BLOCK };
    TokenKind tokenKind;
    std::string blockIndent;
    LiteralString(const LocationRange &lr, const String &value, TokenKind token_kind,
                  const std::string &block_indent)
      : AST(lr, AST_LITERAL_STRING), value(value), tokenKind(token_kind), blockIndent(block_indent)
    { }
};


struct ObjectField {
    enum Kind {
        ASSERT,  // assert expr2 [: expr3]  where expr3 can be nullptr
        FIELD_ID,  // id:[:[:]] expr2
        FIELD_EXPR,  // '['expr1']':[:[:]] expr2
        FIELD_STR,  // expr1:[:[:]] expr2
        LOCAL  // local id = expr2
    };
    enum Hide {
        HIDDEN,  // f:: e
        INHERIT,  // f: e
        VISIBLE,  // f::: e
    };
    enum Kind kind;
    enum Hide hide;  // (ignore if kind != FIELD_something
    bool superSugar;  // +:  (ignore if kind != FIELD_something)
    bool methodSugar;  // f(x, y, z): ...  (ignore if kind  == ASSERT)
    AST *expr1;  // Not in scope of the object
    const Identifier *id;
    Identifiers ids;  // If methodSugar == true then holds the params.
    bool trailingComma;  // If methodSugar == true then remembers the trailing comma
    AST *expr2, *expr3;  // In scope fo the object (can see self).

    ObjectField(enum Kind kind, enum Hide hide, bool super_sugar, bool method_sugar, AST *expr1,
                const Identifier *id, const Identifiers &ids, bool trailing_comma, AST *expr2,
                AST *expr3)
        : kind(kind), hide(hide), superSugar(super_sugar), methodSugar(method_sugar),
          expr1(expr1), id(id), ids(ids), trailingComma(trailing_comma), expr2(expr2), expr3(expr3)
    { }
    static ObjectField Local(bool method_sugar, const Identifier *id, const Identifiers &ids,
                             bool trailingComma, AST *body)
    {
        return ObjectField(
            LOCAL, VISIBLE, false, method_sugar, nullptr, id, ids, trailingComma, body, nullptr);
    }
    static ObjectField Local(const Identifier *id, AST *body)
    {
        return ObjectField(
            LOCAL, VISIBLE, false, false, nullptr, id, Identifiers{}, false, body, nullptr);
    }
    static ObjectField LocalMethod(const Identifier *id, const Identifiers &ids, bool trailingComma,
                                   AST *body)
    {
        return ObjectField(
            LOCAL, VISIBLE, false, true, nullptr, id, ids, trailingComma, body, nullptr);
    }
    static ObjectField Assert(AST *body, AST *msg)
    {
        return ObjectField(
            ASSERT, VISIBLE, false, false, nullptr, nullptr, Identifiers{}, false, body, msg);
    }
};
typedef std::vector<ObjectField> ObjectFields;

/** Represents object constructors { f: e ... }.
 *
 * The trailing comma is only allowed if fields.size() > 0.  Converted to DesugaredObject during
 * desugaring.
 */
struct Object : public AST {
    ObjectFields fields;
    bool trailingComma;
    Object(const LocationRange &lr, const ObjectFields &fields, bool trailing_comma)
      : AST(lr, AST_OBJECT), fields(fields), trailingComma(trailing_comma)
    {
        assert(fields.size() > 0 || !trailing_comma);
    }
};

/** Represents object constructors { f: e ... } after desugaring.
 *
 * The assertions either return true or raise an error.
 */
struct DesugaredObject : public AST {
    struct Field {
        enum ObjectField::Hide hide;
        AST *name;
        AST *body;
        Field(enum ObjectField::Hide hide, AST *name, AST *body)
          : hide(hide), name(name), body(body)
        { }
    };
    typedef std::vector<Field> Fields;
    ASTs asserts;
    Fields fields;
    DesugaredObject(const LocationRange &lr, const ASTs &asserts, const Fields &fields)
      : AST(lr, AST_DESUGARED_OBJECT), asserts(asserts), fields(fields)
    { }
};


/** Represents object comprehension { [e]: e for x in e for.. if... }. */
struct ObjectComprehension : public AST {
    ObjectFields fields;
    bool trailingComma;
    std::vector<ComprehensionSpec> specs;
    ObjectComprehension(const LocationRange &lr, const ObjectFields &fields, bool trailing_comma,
                        const std::vector<ComprehensionSpec> &specs)
                        
      : AST(lr, AST_OBJECT_COMPREHENSION), fields(fields), trailingComma(trailing_comma),
        specs(specs)
    { }
};

/** Represents post-desugaring object comprehension { [e]: e for x in e }. */
struct ObjectComprehensionSimple : public AST {
    AST *field;
    AST *value;
    const Identifier *id;
    AST *array;
    ObjectComprehensionSimple(const LocationRange &lr, AST *field, AST *value,
                              const Identifier *id, AST *array)
      : AST(lr, AST_OBJECT_COMPREHENSION_SIMPLE), field(field), value(value), id(id), array(array)
    { }
};

/** Represents the self keyword. */
struct Self : public AST {
    Self(const LocationRange &lr)
      : AST(lr, AST_SELF)
    { }
};

/** Represents the super[e] and super.f constructs.
 *
 * Either index or identifier will be set before desugaring.  After desugaring, id will be
 * nullptr.
 */
struct SuperIndex : public AST {
    AST *index;
    const Identifier *id;
    SuperIndex(const LocationRange &lr, AST *index, const Identifier *id)
      : AST(lr, AST_SUPER_INDEX), index(index), id(id)
    { }
};

enum UnaryOp {
    UOP_NOT,
    UOP_BITWISE_NOT,
    UOP_PLUS,
    UOP_MINUS
};

static inline std::string uop_string (UnaryOp uop)
{
    switch (uop) {
        case UOP_PLUS: return "+";
        case UOP_MINUS: return "-";
        case UOP_BITWISE_NOT: return "~";
        case UOP_NOT: return "!";

        default:
        std::cerr << "INTERNAL ERROR: Unrecognised unary operator: " << uop << std::endl;
        std::abort();
    }
}

/** Represents unary operators. */
struct Unary : public AST {
   UnaryOp op;
    AST *expr;
    Unary(const LocationRange &lr, UnaryOp op, AST *expr)
      : AST(lr, AST_UNARY), op(op), expr(expr)
    { }
};

/** Represents variables. */
struct Var : public AST {
    const Identifier *id;
    const Identifier *original;
    Var(const LocationRange &lr, const Identifier *id)
      : AST(lr, AST_VAR), id(id), original(id)
    { }
    Var(const LocationRange &lr, const Identifier *id, const Identifier *original)
      : AST(lr, AST_VAR), id(id), original(original)
    { }
};


/** Allocates ASTs on demand, frees them in its destructor.
 */
class Allocator {
    std::map<String, const Identifier*> internedIdentifiers;
    ASTs allocated;
    public:
    template <class T, class... Args> T* make(Args&&... args)
    {
        auto r = new T(std::forward<Args>(args)...);
        allocated.push_back(r);
        return r;
    }
    /** Returns interned identifiers.
     *
     * The location used in the Identifier AST is that of the first one parsed.
     */
    const Identifier *makeIdentifier(const String &name)
    {
        auto it = internedIdentifiers.find(name);
        if (it != internedIdentifiers.end()) {
            return it->second;
        }
        auto r = new Identifier(name);
        internedIdentifiers[name] = r;
        return r;
    }
    ~Allocator()
    {
        for (auto x : allocated) {
            delete x;
        }
        allocated.clear();
        for (auto x : internedIdentifiers) {
            delete x.second;
        }
        internedIdentifiers.clear();
    }
};

#endif  // JSONNET_AST_H
