#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define SIMPLE_SEQ_CASES                                        \
"simple seq",                                                   \
"simple seq, explicit, single line",                            \
"simple seq, explicit, single line, trailcomma",                \
"simple seq, explicit, multiline, unindented",                  \
"simple seq, explicit, multiline, unindented, trailcomma",      \
"simple seq, explicit, multiline, comments inline",             \
"simple seq, explicit, multiline, comments prev line",          \
"simple seq, explicit, multiline, indented",                    \
"simple seq, comments inline",                                  \
"simple seq, comments prev line",                               \
"simple seq, scalars with special chars, comma",                \
"simple seq, scalars with special chars, semicolon",            \
"simple seq, scalars with special chars, cardinal",             \
"simple seq, scalars with special chars, dash",                 \
"simple seq, scalars with special chars, left-bracket",         \
"simple seq, scalars with special chars, right-bracket",        \
"simple seq, scalars with special chars, left-curly",           \
"simple seq, scalars with special chars, right-curly",          \
"simple seq expl, scalars with special chars, comma",           \
"simple seq expl, scalars with special chars, semicolon",       \
"simple seq expl, scalars with special chars, cardinal",        \
"simple seq expl, scalars with special chars, dash",            \
"simple seq expl, scalars with special chars, left-bracket",    \
"simple seq expl, scalars with special chars, right-bracket",   \
"simple seq expl, scalars with special chars, left-curly",      \
"simple seq expl, scalars with special chars, right-curly",     \
"simple seq, issue 28",                                         \
"simple seq, invalid character 1",                              \
"simple seq, invalid character 2",                              \
"simple seq, invalid character 3",                              \
"simple seq, invalid character 4"


CASE_GROUP(SIMPLE_SEQ)
{
    APPEND_CASES(

C("simple seq",
R"(- 0
- 1
- 2
- 3
)",
L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),


C("simple seq, explicit, single line",
"[0, 1, 2, 3]",
L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, explicit, single line, trailcomma",
"[0, 1, 2, 3,]",
L{N{"0"}, N{"1"}, N{"2"}, N{"3"},}
),

C("simple seq, explicit, multiline, unindented",
R"([
0,
1,
2,
3
])",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, explicit, multiline, unindented, trailcomma",
R"([
0,
1,
2,
3,
])",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, explicit, multiline, comments inline",
R"([
0,  # bla0
1,  # bla1
2,  # bla2
3   # bla3
])",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, explicit, multiline, comments prev line",
R"([
# bla0
0,
# bla1
1,
# bla2
2,
# bla3
3
])",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, explicit, multiline, indented",
R"([
  0,
  1,
  2,
  3
])",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, comments inline",
R"(
- 0   # this is a foo
- 1   # this is a bar
- 2   # this is a bar
- 3   # this is a bar
)",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, comments prev line",
R"(
# this is a foo
- 0
# this is a bar
- 1
# this is a baz
- 2
# this is a bat
- 3
)",
    L{N{"0"}, N{"1"}, N{"2"}, N{"3"}}
),

C("simple seq, scalars with special chars, comma",
R"(
- a,b
- c,d
- e,f
- a, b
- c, d
- e, f
- a , b
- c , d
- e , f
- a ,b
- c ,d
- e ,f
)",
L{N{"a,b"}, N{"c,d"}, N{"e,f"},
  N{"a, b"}, N{"c, d"}, N{"e, f"},
  N{"a , b"}, N{"c , d"}, N{"e , f"},
  N{"a ,b"}, N{"c ,d"}, N{"e ,f"},
    }
),

C("simple seq, scalars with special chars, semicolon",
R"(
- a:b
- "c:d"
- 'e:f'
- a :b
- "c :d"
- 'e :f'
- a : b    # THIS IS A KEY-VAL!!!
- "c : d"
- 'e : f'
- a: b     # THIS IS A KEY-VAL!!!
- "c: d"
- 'e: f'
)",
L{
  N("a:b"), N("c:d"), N("e:f"),
  N("a :b"), N("c :d"), N("e :f"),
  N(L{N("a", "b")}), N("c : d"), N("e : f"),
  N(L{N("a", "b")}), N("c: d"), N("e: f"),
    }
),

C("simple seq, scalars with special chars, cardinal",
R"(
- a#b
- "a#b"
- 'a#b'
- a# b
- "a# b"
- 'a# b'
- a # b
- "a # b"
- 'a # b'
- a #b
- "a #b"
- 'a #b'
)",
L{
  N{"a#b"}, N{"a#b"}, N{"a#b"},
  N{"a# b"}, N{"a# b"}, N{"a# b"},
  N{"a"}, N{"a # b"}, N{"a # b"},
  N{"a"}, N{"a #b"}, N{"a #b"},
    }
),

C("simple seq, scalars with special chars, dash",
R"(
- a-b
- "a-b"
- 'a-b'
- a- b
- "a- b"
- 'a- b'
- a - b
- "a - b"
- 'a - b'
- a -b
- "a -b"
- 'a -b'
)",
L{
  N{"a-b"}, N{"a-b"}, N{"a-b"},
  N{"a- b"}, N{"a- b"}, N{"a- b"},
  N{"a - b"}, N{"a - b"}, N{"a - b"},
  N{"a -b"}, N{"a -b"}, N{"a -b"},
    }
),

C("simple seq, scalars with special chars, left-curly",
R"(
- a{b
- "a{b"
- 'a{b'
- a{ b
- "a{ b"
- 'a{ b'
- a { b
- "a { b"
- 'a { b'
- a {b
- "a {b"
- 'a {b'
)",
L{
  N{"a{b"}, N{"a{b"}, N{"a{b"},
  N{"a{ b"}, N{"a{ b"}, N{"a{ b"},
  N{"a { b"}, N{"a { b"}, N{"a { b"},
  N{"a {b"}, N{"a {b"}, N{"a {b"},
    }
),

C("simple seq, scalars with special chars, right-curly",
R"(
- a}b
- "a}b"
- 'a}b'
- a} b
- "a} b"
- 'a} b'
- a } b
- "a } b"
- 'a } b'
- a }b
- "a }b"
- 'a }b'
)",
L{
  N{"a}b"}, N{"a}b"}, N{"a}b"},
  N{"a} b"}, N{"a} b"}, N{"a} b"},
  N{"a } b"}, N{"a } b"}, N{"a } b"},
  N{"a }b"}, N{"a }b"}, N{"a }b"},
    }
),

C("simple seq, scalars with special chars, left-bracket",
R"(
- a[b
- "a[b"
- 'a[b'
- a[ b
- "a[ b"
- 'a[ b'
- a [ b
- "a [ b"
- 'a [ b'
- a [b
- "a [b"
- 'a [b'
)",
L{
  N{"a[b"}, N{"a[b"}, N{"a[b"},
  N{"a[ b"}, N{"a[ b"}, N{"a[ b"},
  N{"a [ b"}, N{"a [ b"}, N{"a [ b"},
  N{"a [b"}, N{"a [b"}, N{"a [b"},
    }
),

C("simple seq, scalars with special chars, right-bracket",
R"(
- a]b
- "a]b"
- 'a]b'
- a] b
- "a] b"
- 'a] b'
- a ] b
- "a ] b"
- 'a ] b'
- a ]b
- "a ]b"
- 'a ]b'
)",
L{
  N{"a]b"}, N{"a]b"}, N{"a]b"},
  N{"a] b"}, N{"a] b"}, N{"a] b"},
  N{"a ] b"}, N{"a ] b"}, N{"a ] b"},
  N{"a ]b"}, N{"a ]b"}, N{"a ]b"},
    }
),

C("simple seq expl, scalars with special chars, comma",
R"([
 a,b,  "c,d",   'e,f',
 a, b,  "c, d",   'e, f',
 a , b,  "c , d",   'e , f',
 a ,b,  "c ,d",   'e ,f',
])",
L{
  N{"a"}, N("b"), N("c,d"), N("e,f"),
  N{"a"}, N("b"), N("c, d"), N("e, f"),
  N{"a"}, N("b"), N("c , d"), N("e , f"),
  N{"a"}, N("b"), N("c ,d"), N("e ,f"),
    }
),

C("simple seq expl, scalars with special chars, semicolon",
R"([
# a:b,  # not legal
  "c:d",   'e:f',
# a: b,  # not legal
  "c: d",   'e: f',
# a : b,  # not legal
  "c : d",   'e : f',
# a :b,  # not legal
  "c :d",   'e :f',
])",
L{/*...not legal...*/
  /*N{"a"}, N("b"),*/ N("c:d"), N("e:f"),
  /*N{"a"}, N("b"),*/ N("c: d"), N("e: f"),
  /*N{"a"}, N("b"),*/ N("c : d"), N("e : f"),
  /*N{"a"}, N("b"),*/ N("c :d"), N("e :f"),
    }
),

C("simple seq expl, scalars with special chars, cardinal",
R"([
 a#b, "c#d",   'e#f',
 a# b, "c# d",   'e# f',
 a # b, "c # d",   'e # f',
, # this is needed because of the comment above
 a #b, "c #d",   'e #f',
])",
L{
  N{"a#b"}, N("c#d"), N("e#f"),
  N{"a# b"}, N("c# d"), N("e# f"),
  N{"a"},
  N{"a"},
    }
),

C("simple seq expl, scalars with special chars, dash",
R"([
 a-b, "c-d",   'e-f',
 a- b, "c- d",   'e- f',
 a - b, "c - d",   'e - f',
 a -b, "c -d",   'e -f',
])",
L{
  N{"a-b"}, N("c-d"), N("e-f"),
  N{"a- b"}, N("c- d"), N("e- f"),
  N{"a - b"}, N("c - d"), N("e - f"),
  N{"a -b"}, N("c -d"), N("e -f"),
    }
),

C("simple seq expl, scalars with special chars, left-bracket",
R"([
# a[b,
   "c[d",   'e[f',
# a[ b,
   "c[ d",   'e[ f',
# a [ b,
   "c [ d",   'e [ f',
# a [b,
   "c [d",   'e [f',
])",
L{
  /*N{"a[b"}, */N("c[d"), N("e[f"),
  /*N{"a[ b"}, */N("c[ d"), N("e[ f"),
  /*N{"a [ b"},*/ N("c [ d"), N("e [ f"),
  /*N{"a [b"}, */N("c [d"), N("e [f"),
    }
),

C("simple seq expl, scalars with special chars, right-bracket",
R"([
# a]b,
   "c]d",   'e]f',
# a] b,
   "c] d",   'e] f',
# a ] b,
   "c ] d",   'e ] f',
# a ]b,
   "c ]d",   'e ]f',
])",
L{
  /*N{"a]b"}, */N("c]d"), N("e]f"),
  /*N{"a] b"}, */N("c] d"), N("e] f"),
  /*N{"a ] b"},*/ N("c ] d"), N("e ] f"),
  /*N{"a ]b"}, */N("c ]d"), N("e ]f"),
    }
),

C("simple seq expl, scalars with special chars, left-curly",
R"([
# a{b,
   "c{d",   'e{f',
# a{ b,
   "c{ d",   'e{ f',
# a { b,
   "c { d",   'e { f',
# a {b,
   "c {d",   'e {f',
])",
L{
  /*N{"a{b"}, */N("c{d"), N("e{f"),
  /*N{"a{ b"}, */N("c{ d"), N("e{ f"),
  /*N{"a { b"},*/ N("c { d"), N("e { f"),
  /*N{"a {b"}, */N("c {d"), N("e {f"),
    }
),

C("simple seq expl, scalars with special chars, right-curly",
R"([
# a}b,
   "c}d",   'e}f',
# a} b,
   "c} d",   'e} f',
# a } b,
   "c } d",   'e } f',
# a }b,
   "c }d",   'e }f',
])",
L{
  /*N{"a}b"}, */N("c}d"), N("e}f"),
  /*N{"a} b"}, */N("c} d"), N("e} f"),
  /*N{"a } b"},*/ N("c } d"), N("e } f"),
  /*N{"a }b"}, */N("c }d"), N("e }f"),
    }
),

C("simple seq, issue 28",
R"(# was failing on https://github.com/biojppm/rapidyaml/issues/28
enemy:
- actors:
  - {name: Enemy_Bokoblin_Junior, value: 4.0}
  - {name: Enemy_Bokoblin_Middle, value: 16.0}
  - {name: Enemy_Bokoblin_Senior, value: 32.0}
  - {name: Enemy_Bokoblin_Dark, value: 48.0}
  species: BokoblinSeries
enemy2:
- actors:
    - {name: Enemy_Bokoblin_Junior, value: 4.0}
    - {name: Enemy_Bokoblin_Middle, value: 16.0}
    - {name: Enemy_Bokoblin_Senior, value: 32.0}
    - {name: Enemy_Bokoblin_Dark, value: 48.0}
  species: BokoblinSeries
)",
L{
   N("enemy", L{N(L{
     N("actors", L{
       N(L{N("name", "Enemy_Bokoblin_Junior"), N("value", "4.0"),}),
       N(L{N("name", "Enemy_Bokoblin_Middle"), N("value", "16.0"),}),
       N(L{N("name", "Enemy_Bokoblin_Senior"), N("value", "32.0"),}),
       N(L{N("name", "Enemy_Bokoblin_Dark"), N("value", "48.0"),}),
     }),
     N("species", "BokoblinSeries"),
     })
   }),
   N("enemy2", L{N(L{
     N("actors", L{
       N(L{N("name", "Enemy_Bokoblin_Junior"), N("value", "4.0"),}),
       N(L{N("name", "Enemy_Bokoblin_Middle"), N("value", "16.0"),}),
       N(L{N("name", "Enemy_Bokoblin_Senior"), N("value", "32.0"),}),
       N(L{N("name", "Enemy_Bokoblin_Dark"), N("value", "48.0"),}),
     }),
     N("species", "BokoblinSeries"),
     })
   }),
}),

C("simple seq, invalid character 1", HAS_PARSE_ERROR,
R"(- 0   # this is a foo
}
)",
  LineCol(2, 1)
),

C("simple seq, invalid character 2", HAS_PARSE_ERROR,
R"(- 0   # this is a foo
]
)",
  LineCol(2, 1)
),

C("simple seq, invalid character 3", HAS_PARSE_ERROR,
R"(- 0   # this is a foo
:
)",
  LineCol(2, 1)
),

C("simple seq, invalid character 4", HAS_PARSE_ERROR,
R"(- 0   # this is a foo
abcdef!
)",
  LineCol(2, 1)
),


    )
}

INSTANTIATE_GROUP(SIMPLE_SEQ)

} // namespace yml
} // namespace c4
