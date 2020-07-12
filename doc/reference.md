# Language Reference

This page explains Jsonnet in detail. We assume basic familiarity with the language, so if you are just starting out, please go through [the tutorial](/learning/tutorial.html) first. On the other hand, we do not try to cover 100% of the language here. If you need a more complete and precise description, please refer to [the specification](/ref/spec.html) and [the standard library documentation](/ref/stdlib.html). 

## The Scope of the Jsonnet Project

Jsonnet is designed primarily for configuring complex systems. The standard use case is integrating multiple services which do not know about each other. Writing the configuration for each independently would result in massive duplication and most likely would be difficult to maintain. Jsonnet allows you to specify the configuration on your terms and programmatically set up all individual services.

If you are an application author, Jsonnet may free you from having to design your custom configuration format. The applications can simply consume JSON or other structured format and the users can specify their configuration in Jsonnet – a generic configuration language.

Jsonnet is a full programming language, so it can be used to implement arbitrary logic. However, it does not allow unrestricted IO (see [Independence from the Environment](#independence_from_the_environment)), so it is practical for cases where precise control over input and output is a good thing. There are a couple of potential applications, other than configuration:
* Static site generation
* Embedded expression language
* Ad hoc JSON transformations – it makes sense to use Jsonnet for that if you already know it. Otherwise [jq](https://stedolan.github.io/jq/) is arguably a better option (Jsonnet prioritizes clarity over terseness and convenience, which can be a disadvantage for one-off usage).
* Teaching – due to simplicity and principled approach.

## What It Is, Not What It Does

Jsonnet is a purely functional language (with OO features). Programmers used to mostly imperative languages such as Python, C++, Java or Go will need to change their mindset a bit.

One important difference is that you define things in terms of *what they are* rather than *what they do*. Consider a simple implementation of a `map` function:
```
local map(func, arr) =
  if std.length(arr) == 0 then
    [func(arr[0])] + map(func, arr[1:])
  else
    []
  ;
  // Example usage:
  map(function(i) i * 2, [1, 2, 3, 4, 5])
```

If you try to read it out loud, you get something like:
> The result of mapping function `func` over array `arr` is either
>   1) an empty array if array `arr` is empty
>   2) function `func` applied to the first element of `arr` followed by the mapping of the rest of the array is array `arr` is not empty.

Compare it to a traditional, imperative implementation which would read more like:
> In order to map a function `func` over array `arr`:
> 1) Create a copy of an array `arr`, called `newArr`.
> 2) For each index `i` in array  `newArr`
>   2a) Replace `newArr[i]` with `func` applied to `newArr[i]`.
> 3) Return `newArr` as the result.

Jsonnet definitions are similar to definitions in mathematics. A great advantage of such style is that it frees you from constantly thinking about the state and the order of operations.

## Expressions

Jsonnet programs are composed entirely out of [expressions](https://en.wikipedia.org/wiki/Expression_(computer_science)). There are no statements or special top-level declarations present in most other languages. For instance, imports, conditionals, functions,  objects, and locals are all expressions.

Any kind of expression can be a Jsonnet program, it is not necessary to have a top-level object. For example `2+2`, `"foo"` or `local bar = 21; bar * 2` are all valid Jsonnet programs.

Expressions can be *evaluated* to produce a *value*. The evaluation does not have any side effects.

The value to which an expression evaluates depends on its *environment* – values of variables, to which the expression refers. For example expression `x * x` depends on the value of `x`. Such expression is only valid in contexts, where variable `x` is available, e.g. `local x = 2; x * x` or `function(x); x * x`.

Jsonnet variables follow the rules of *lexical scoping*. The environment mentioned before and consequently the meaning of variables in a given expression is statically determined:

```
local a = local x = 'a'; x;
local foo = local x = 'b';  a;
foo
```

A special case of that is creating [closures](https://en.wikipedia.org/wiki/Closure_(computer_programming)), such as the following:
```
local addNumber(number) = function(x) number + x;
local add2 = addNumber(2);
local add3 = addNumber(3);
[
  add2(2),
  add3(5)
]
```

## Values

Jsonnet has only seven types of values:
* `null` – only one value, namely `null`.
* `boolean` – two values, `true` and `false`. 
* `string` – Unicode strings (sequences of Unicode codepoints).
* `number` – IEEE754 64-bit floating point number.
* `function`, pure functions which take values as arguments and return a value.
* `array`, a finite-length array of values. 
* `object`, a superset of JSON objects with support for inheritance.

All Jsonnet values are immutable. It is not possible to change a value of a field of an object or an element of an array – you can only create a new one with the desired change applied. 

You can check the type of any value using `std.type`.

### Equivalence and Equality

We say that values `a` and `b` are equal if `a == b` evaluates to `true` and that they are unequal if `a == b` evaluates to `false`. Some pairs of values are neither equal or unequal, because functions cannot be checked for equality (and consequently some complex values containing functions). 

Values of different types are never considered equal – there is no implicit casting (unlike, for example, in JavaScript).

We say that values `a` and `b` are *the same value*, or *equivalent* if it is impossible to tell these values apart in any way in Jsonnet. More formally `a` and `b` are equivalent if there does not exist a Jsonnet function `f` such that exactly one of `f(a)` and `f(b)` results in an error.

In general, equivalent values may have different representations, and that may have performance implications, but does not affect the result.

Equivalent values of course cannot be unequal, but equal values may not be equivalent (e.g. `{ a: 1, b: 1}` and `{a: 1, b: self.a}`). 

### Null

Null is the simplest type in Jsonnet. It has only one value –  `null`.

There is no special handling of `null`, it is a value like any other.
In particular arrays can have null elements and objects can have null fields. The null value is equal only to the null value.

### Boolean 

Boolean has two values `true` and `false`. They are the only values which can be used in `if` conditions.

### String

Strings in Jsonnet are sequences of Unicode codepoints.

In most contexts they can be treated as an array of one codepoint strings (e.g. `std.length` and the `[]` operator work like that), but they are not equivalent (for example `std.type("foo") != std.type(["f", "o", "o"])`).

Unlike arrays, strings are strict, meaning that evaluating a string requires calculating all its contents.

Strings can be constructed as literals, slices of existing strings, concatenations of existing strings or converted from array of Unicode codepoint numbers.

### Number

Jsonnet numbers are 64-bit floating point numbers as defined in IEEE754 excluding `nan` and `inf` values. Operation resulting in infinity or not a number is considered an error. 

Integers can be precisely represented as a Jsonnet number in the range [-2^53,2^53]. This is [a direct consequence of IEEE754 spec](https://en.wikipedia.org/wiki/Double-precision_floating-point_format).

### Function

Functions in Jsonnet are functions in the mathematical sense. Each function has parameters and a body expression. The result of calling a function is equivalent to the result of evaluating its body with arguments introduced to the environment.

The basic way to define a Jsonnet function, is using a *function literal*:
```
local func = function(x) x * 2;
func(21)
```

Jsonnet has syntax sugar for defining functions, which allows shorter, equivalent version:
```
local func(x) = x * 2;
func(21)
```

The arguments are **not** evaluated when a function is called. They are passed lazily and evaluated only when actually used. This allows expressing things, which in other languages require built-in functionality or macros, such as the short-circuiting boolean operations:
```
local and3(a, b, c) = a && b && c;
and3(true, false, error "this one is never evaluated")
```

It is worth noting at this point that functions in Jsonnet are [referentially transparent](Referential_transparency), meaning that any function call can be replaced with its definition, without changing the meaning of the program. Therefore, in some sense, functions in Jsonnet are "hygienic macros". For example consider the following snippet:

```
local pow2(n) = if n = 0 then 1 else 2 * pow2(n);
pow2(17)
```

The call to function pow2 can be replaced with its definition as follows:

```
local pow2(n) = if n = 0 then 1 else 2 * pow2(n);
local n = 17;
if n = 0 then 1 else 2 * pow2(n)
```

#### Function Parameters

The parameters can be either required or optional. In the definition, required and optional parameters can be mixed in any order. Optional parameters require specifying the default argument.

When calling a function, each argument can be passed either as named or as positional, but all positional arguments need to go before the named arguments.

For example the following program is valid: 

```
local foo(x, y=1) = x + y;
[
  foo(1),
  foo(1, 1),
  foo(x=1, y=1),
  foo(y=1, x=1),
  foo(x=1),
]
```

It is recommended to always pass optional arguments as named (for readability). We also recommend to pass arguments as positional when all parameters are required, because the function author probably did not consider the names of the function names as part of the stable interface.

(Sidenote: We are looking for a way to have explicit named-only or positional-only parameters in function signatures, without breaking backwards compatibility.)

### Array

Arrays in Jsonnet are finite-length sequences of arbitrary values.
Values of different types can be mixed in an array. Individual array elements are lazy, meaning that evaluating an array does not cause evaluation of all arguments.

There is no separate tuple type in Jsonnet. Arrays are used in contexts where tuples would be natural in other languages, for example for returning multiple values from a function.

The simplest way of creating an array is *an array literal*. It is simply a comma separated list of elements, such as `[1, 2, "foo", 2 + 2]`.

The most flexible way of creating an array is `std.makeArray(sz, func)`. It takes the size of the array to construct and a function which takes an index `i` and returns `i`-th element. It is possible to build all other array functionality using this function. In practice, using more specialized functions is usually (but not always) more handy and results in a more efficient program.

Jsonnet also offers *array comprehensions* which provide an elegant and easy to use syntax for mapping, filtering and taking Cartesian products of arrays. Array comprehensions can be "mechanically" translated to a series of `std.flatMap` calls and simple conditionals. For example, the following programs are equivalent:

```
[ 
  [x * 2, y]
  for x in [1, 2, 3, 4, 5]
  for y in [1, 2, 3]
  if x % 2 == 0
]
```

```
std.flatMap(
  function(x) std.flatMap(
    function(y) if x % 2 == 0 then [[x * 2, y]] else [],
    [1, 2, 3]
  ),
  [1, 2, 3, 4, 5]
)
```

Arrays can be concatenated using the operator `+`. Arrays `a` and `b` are equal if they have equal length and for all indexes `i`, `a[i] == b[i]`. The comparison of arrays is "lexicographic", so array `a` is smaller than `b` if `a[i] < b[i]` for some `i` and for all `j < i`, `a[i] == b[i]`. 

### Object

In the simplest case Jsonnet object is a mapping from string keys to arbitrary values.

```
{
  "foo": 1,
  "bar": {
    "arr": [1, 2, 3],
    "number": 10 + 7,
  }
}
```

Jsonnet objects can be indexed either using `.` and an identifier or `[]` with an arbitrary expression.

```
local obj = {
  "foo": 1,
  "bar": {
    "arr": [1, 2, 3],
    "number": 10 + 7,
  }
};
[
  obj.foo,
  obj["foo"],
  obj["f" + "oo"]
]
```

### Inheritance

Jsonnet objects allow inheritance in the OOP sense, even though there are no classes or declarations. The inheritance is realized as an operation `+` which can be applied to any two objects. This might be surprising, because in mainstream languages the inheritance hierarchy is static.

For objects which are simple key-value mappings, inheritance is the same as replacing the respective fields from the first object with fields from the second object.

```
{
  a: 1
  b: 2,
}
+
{
  a: 3
}
```

It is possible for one field to refer to other ones using `self` and it is possible to refer to the fields of the base object using `super`.

```
local obj = {
  name: "Alice",
  greeting: "Hello, " + self.name,
};
[
  obj,
  obj + { name: "Bob" },
  obj + { greeting: super.greeting + "!"}
]
```

In general, it is useful to think about Jsonnet object as a stack of *layers*. A layer consists of fields. An object literal or an object comprehension is a single-layer object. Object inheritance `A` + `B` results in an object with `B` layers put on top of `A` layers. 

Reference to a field through `self` corresponds to looking for the field starting from the top of the stack and going towards the bottom until the field is found. Reference through `super` searches for the field starting from the layer below the current one.

It is easy to emulate functions using objects. It goes without saying that it is not a good style to use in the real code. 

```
local add = {
  params: {
    a: error "please provide argument a",
    b: error "please provide argument b",
  },
  result: self.params.a + self.params.b
};
(add + { params: { a: 1, b: 2} }).result
```

#### Properties

Let `D`, `E`, `F` range over arbitrary expressions evaluating to objects. Let ≡ mean equivalence.

* **Associativity** always holds:
  ```
  (D + E) + F   ≡   D + (E + F)
  ```
* **Identity** always holds:
  ```
  D + { }   ≡   D
  { } + D   ≡   D
  ```
* **Idempotence** holds when `D` does not contain `super`:
  ```
  D + D ≡ D
  ```
* **Commutativity** holds when `D` and `E` do not contain super and have no common fields:
  ```
  D + E   ≡   E + D
  ```

### Self-Referencing Objects

Objects can be self-referencing even without OOP features, simply because variable definitions can be recursive:

```
local obj = {
  name: "Alice",
  greeting: "Hello, " + obj.name,
}; obj
```

Referencing `obj` like that is **different** from using `self`. Here `obj` is a fixed object with a fixed set of fields and their values, and consequently `obj.name` is fixed. On the other hand, `self` is the "current" object, which may be a result of inheritance.

```
[
  local obj = {
    name: "Alice",
    greeting: "Hello, " + obj.name + "!",
  }; obj + {name: "Bob"},
  {
    name: "Alice",
    greeting: "Hello, " + self.name + "!",
  } + {name: "Bob"},
]
```

Both kinds of behavior are useful. You need to make a decision whether to refer to the fields of the objects as they are currently defined or to allow overriding.

### Visibilities

Jsonnet objects have a concept of visibility which affects manifestation (printing out objects) and equality checks. This concept has nothing to do with the notion of private/public fields from other languages.

There are three kinds of visibility that an object field may have:
* `:` – Default, visible unless parent's field with the same name is hidden.
* `::` – Hidden.
* `:::` – Forced Visible.

Example:
```
{
  default: "foo",
  default_then_hidden: "foo",
  hidden:: "foo",
  hidden_then_default:: "foo",
  hidden_then_visible:: "foo",
  visible::: "foo",
  visible_then_hidden::: "foo",
}
+
{
  default_then_hidden:: "foo",
  hidden_then_default: "foo",
  hidden_then_visible::: "foo",
  visible_then_hidden:: "foo",
}
```

The value of a field is irrelevant for determining its visibility.

It is possible to check field's visibility using `std.objectHas` and `std.objectHasAll` standard library functions. The first checks if an object has a visible field with a specified name and the second checks has a field regardless of its visibility.

### Objects Equality

Two objects are equal when their respective *visible* fields are equal. Hidden fields are ignored, which allows ignoring the "helper" parts of the object when evaluating equality.

Checking equality of some objects is not allowed, when the result depends on fields which have values which cannot be checked for equality. For example:

```
{
  a: function() 42
} == {
  a: function() 42
}
```

### Object Locals

It is possible to declare a `local` inside an object, which will be available to all fields.

```
{
  local foo = 1,
  aaa: foo,
  bbb: foo,
}
```

Object locals end with a comma, instead of semicolon (like fields). They are not independent expressions, but parts of an object literal expression. Formally, they are equivalent to declaring the locals in each field separately but, the interpreters are likely to implement object locals more efficiently.

Object locals can access `self` and `super` – they are "inside the object". As a consequence of this, **object locals are not available in Field Name Expressions**, because (in general) they depend on the object being already created, which requires the field names to be already known.

### Field Name Expressions

Field names of Jsonnet objects can be arbitrary strings and can be calculated dynamically *when the object is created*. 

```
{
  a: 1,
  "a a": 2,
  "ąę": 3,
  ["aaa" + "bbb"]: 4,
}
```

When an object is evaluated all the field names need to be evaluated. This means that they cannot refer to object locals, `self` or `super` – in other words their scope is "outside of the object".

## Independence from the Environment (Hermeticity)

Jsonnet programs are *pure computations*, which have no side-effects and which depend only on the values which were explicitly passed.
In particular, the behavior is independent from the setup of the system on which it runs (operating system, environment variables, filesystem, ...).
The semantics are [defined almost entirely in mathematical terms](/ref/spec.html), with a few exceptions, where Jsonnet depends on well-established portable standards (such as [IEEE754](https://en.wikipedia.org/wiki/IEEE_754) and [Unicode](https://en.wikipedia.org/wiki/Unicode)). 

It is possible to pass data from the environment, but only explicitly, using abstractions provided by Jsonnet. There ale multiple benefits coming from that:
* Fewer surprises – The behavior will not change when you change something about your system. In other languages any part of the program can depend on anything. In Jsonnet you need to worry only about what is explicitly passed.
* Easier to run on other machines (e.g. development or CI) – You can pass any values to the program, which allows generating any configuration on any system. 
* Longevity – Jsonnet programs don't need to change as other technologies go out of date.
* Portability – it is reasonably easy to create an implementation of Jsonnet for any platform.

## Passing Data to Jsonnet

### Consider Self-Contained Programs

Before using any of the methods described below, it is worth considering if a fully self-contained setup is viable.

In this style, the configuration is a set of `.jsonnet` and `.libsonnet` files. Every output file corresponds to a `.jsonnet` file and all shared setup is in `.libsonnet` files. Any raw data can be placed in additional files and imported using `importstr`. Usually, all code and data is committed to a repository.
Sometimes the generated configuration is also checked in, which makes it easy to spot unintended changes.

Sometimes it is not practical, though. For example if the produced configuration needs to contain secrets, which you do not want to commit alongside code, it is necessary to pass them from outside.

### Top-Level Arguments (TLAs)

The preferred way of passing data to Jsonnet is through Top-level Arguments. If you have a program which evaluates to a function, It allows passing arguments to a Jsonnet program which evaluates to a function.

Consider the following simple program `add.jsonnet`:
```
function(a, b) a + b
```

It can be called `jsonnet add.jsonnet --tla-code a=1 b=2`. The result of the evaluation will be `3`.

Functions are the canonical way of parameterizing values, so this method fits well with the rest of the language. The programs intended for use with TLAs can also be imported from other Jsonnet files and called as normal functions, such as in the following example:

```
local add = import 'add.jsonnet';
add(1, 2)
```

### External Variables (ExtVars)

Alternatively, you can use ExtVars. ExtVars are available globally in the program, including in any imported library. **Contrary to their name, they are not variables in the same sense as variables introduced by `local`**. They have a separate namespace and can be accessed through `std.extVar` function.

Consider the following program `foo.jsonnet`:
```
std.extVar("foo")
```
It can be called with `jsonnet foo.jsonnet --ext-str foo=bar`.

Referring to an undeclared External Variable is an error. There is no way to check if an ExtVar exists dynamically. There is no way to set External Variables from within a Jsonnet program – they need to be set before execution.

Since ExtVars are global, they create composability problems. It is easy to imagine two libraries depending on the same ExtVar, but with a different meaning.  **For this reason we strongly discourage authors of generic libraries from using ExtVars**. If a global configuration for a library is desired, you can make your library a function which has parameter for any necessary configuration. This is less of a problem in "final" setups – in individual configuration or in opinionated frameworks, which enforce the structure anyway.

<!--
TODO: Some parts of this page are still incomplete:
* JsonPath heritage of $ (?)
* Scoping rules of local
* numbers, floating point rounding error, int64, bitwise on float
* different pes of string literals, escapes, strings that span lines |||, line endings
* Precise meaning of $
* scope of self within an object, including locals, asserts and comprehensions
* general form of comprehensions
* general form of slices
* precise semantics of default argument expressions
* tailstrict
* Recursion in all its forms - functions, objects, modules, locals
* "Looping in Jsonnet" – recursion and higher-order functions
* More object orientation: mixins, diamond problem, definition of +:, 
* laziness in depth
* Imports in depth: relative paths, jpaths, etc.
* Native callbacks and import callbacks
* operator precedence
* Debugging techniques
* Explaining syntax sugar for methods
* "Plain objects" (equivalent to the mapping). 
* Thunks
-->