#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define SINGLE_QUOTED_CASES                             \
            "squoted, only text",                       \
            "squoted, with double quotes",              \
            "squoted, with single quotes",              \
            "squoted, with single and double quotes",   \
            "squoted, with escapes",                    \
            "squoted, all",                             \
            "squoted, empty",                           \
            "squoted, blank",                           \
            "squoted, numbers",                         \
            "squoted, trailing space",                  \
            "squoted, leading space",                   \
            "squoted, trailing and leading space",      \
            "squoted, 1 squote",                        \
            "squoted, 2 squotes",                       \
            "squoted, 3 squotes",                       \
            "squoted, 4 squotes",                       \
            "squoted, 5 squotes"

CASE_GROUP(SINGLE_QUOTED)
{
    APPEND_CASES(

C("squoted, only text",
R"('Some text without any quotes.'
)",
  L{N("Some text without any quotes.")}
),

C("squoted, with double quotes",
R"('Some "text" "with double quotes"')",
  L{N("Some \"text\" \"with double quotes\"")}
),

C("squoted, with single quotes",
R"('Some text ''with single quotes''')",
  L{N("Some text 'with single quotes'")}
),

C("squoted, with single and double quotes",
R"('Some text ''with single quotes'' "and double quotes".')",
  L{N("Some text 'with single quotes' \"and double quotes\".")}
),

C("squoted, with escapes",
R"('Some text with escapes \n \r \t')",
  L{N("Some text with escapes \\n \\r \\t")}
),

C("squoted, all",
R"('Several lines of text,
containing ''single quotes'' and "double quotes". Escapes (like \n) don''t do anything.

Newlines can be added by leaving a blank line.
            Leading whitespace on lines is ignored.'
)",
  L{N("Several lines of text, containing 'single quotes' and \"double quotes\". Escapes (like \\n) don't do anything.\nNewlines can be added by leaving a blank line. Leading whitespace on lines is ignored.")}
),

C("squoted, empty",
R"('')",
  L{N("")}
),

C("squoted, blank",
R"(
- ''
- ' '
- '  '
- '   '
- '    '
)",
  L{N(""), N(" "), N("  "), N("   "), N("    ")}
),

C("squoted, numbers", // these should not be quoted when emitting
R"(
- -1
- -1.0
- +1.0
- 1e-2
- 1e+2
)",
  L{N("-1"), N("-1.0"), N("+1.0"), N("1e-2"), N("1e+2")}
),

C("squoted, trailing space",
R"('a aaaa  ')",
  L{N("a aaaa  ")}
),

C("squoted, leading space",
R"('  a aaaa')",
  L{N("  a aaaa")}
),

C("squoted, trailing and leading space",
R"('  012345  ')",
  L{N("  012345  ")}
),

C("squoted, 1 squote",
R"('''')",
  L{N("'")}
),

C("squoted, 2 squotes",
R"('''''')",
  L{N("''")}
),

C("squoted, 3 squotes",
R"('''''''')",
  L{N("'''")}
),

C("squoted, 4 squotes",
R"('''''''''')",
  L{N("''''")}
),

C("squoted, 5 squotes",
R"('''''''''''')",
  L{N("'''''")}
),

/*
C("squoted, example 2",
R"('This is a key

that has multiple lines

': and this is its value
)",
  L{N("This is a key\nthat has multiple lines\n", "and this is its value")}
),
*/
    )
}

INSTANTIATE_GROUP(SINGLE_QUOTED)

} // namespace yml
} // namespace c4
