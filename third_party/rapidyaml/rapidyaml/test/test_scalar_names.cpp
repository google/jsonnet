#include "./test_group.hpp"

#if defined(_MSC_VER)
#   pragma warning(push)
//#   pragma warning(disable: 4127/*conditional expression is constant*/)
//#   pragma warning(disable: 4389/*'==': signed/unsigned mismatch*/)
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#endif

namespace c4 {
namespace yml {


#define SCALAR_NAMES_CASES \
"funny names, seq",\
"funny names, seq expl",\
"funny names, map",\
"funny names, map expl"\

#define _(name) N(#name) // makes it simpler
#define __(name) N(#name, #name) // makes it simpler

CASE_GROUP(SCALAR_NAMES)
{
    APPEND_CASES(

C("funny names, seq",
R"(
- a
- b:b
- c{c
- cc{
- c}c
- cc}
- c!c
- cc!
- .foo
- .
- -a
- +b
- /b
- :c
- $g
)",
L{_(a), _(b:b), _(c{c), _(cc{), _(c}c), _(cc}), _(c!c), _(cc!), _(.foo), _(.), _(-a), _(+b), _(/b), _(:c), _($g)}
),

C("funny names, seq expl", IGNORE_LIBYAML_PARSE_FAIL,
R"([a, b, c, .foo, ., -a, +b, /b, :c, $g])",
L{_(a), _(b), _(c), _(.foo), _(.), _(-a), _(+b), _(/b), _(:c), _($g)}
),

C("funny names, map",
R"(
a: a
b: b
c: c
.foo: .foo
.: .
-a: -a
+b: +b
/b: /b
:c: :c
$g: $g
)",
L{__(a), __(b), __(c), __(.foo), __(.), __(-a), __(+b), __(/b), __(:c), __($g)}
),

C("funny names, map expl", IGNORE_LIBYAML_PARSE_FAIL,
R"({a: a, b: b, c: c, .foo: .foo, .: ., -a: -a, +b: +b, /b: /b, :c: :c, $g: $g})",
L{__(a), __(b), __(c), __(.foo), __(.), __(-a), __(+b), __(/b), __(:c), __($g)}
),
    )
}

INSTANTIATE_GROUP(SCALAR_NAMES)

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
