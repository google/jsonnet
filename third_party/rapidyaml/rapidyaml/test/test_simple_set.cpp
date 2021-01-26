#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define SIMPLE_SET_CASES                     \
"doc as set, implicit",                      \
"doc as set",\
"sets 2XXW",                       \
"sets 2XXW, indented",             \
"sets 2XXW, no set",               \
"sets 2XXW, no doc",               \
"sets 2XXW, no doc, no set"


TEST(simple_set, emit)
{
    const char yaml[] = R"(!!set
? aa
? bb
? cc
)";
    std::string expected = R"(!!set
aa: ~
bb: ~
cc: ~
)";
    Tree t = parse(yaml);
    auto s = emitrs<std::string>(t);
    EXPECT_EQ(expected, s);
}


TEST(simple_set, emit_doc)
{
    const char yaml[] = R"(--- !!set
? aa
? bb
? cc
)";
    std::string expected = R"(--- !!set
aa: ~
bb: ~
cc: ~
)";
    Tree t = parse(yaml);
    auto s = emitrs<std::string>(t);
    EXPECT_EQ(expected, s);
}


CASE_GROUP(SIMPLE_SET)
{
    APPEND_CASES(

C("doc as set, implicit",
R"(!!set
? a
? b
)",
N(TL("!!set", L{N(KEYVAL, "a", /*"~"*/{}), N(KEYVAL, "b", /*"~"*/{})}))
),

C("doc as set",
R"(--- !!set
? aa
? bb
? cc
)",
N(STREAM, L{N(DOCMAP, TL("!!set", L{N(KEYVAL, "aa", /*"~"*/{}), N(KEYVAL, "bb", /*"~"*/{}), N(KEYVAL, "cc", /*"~"*/{})}))})
),

C("sets 2XXW",
R"(
--- !!set
? Mark McGwire
? Sammy Sosa
? Ken Griff
)",
N(STREAM, L{N(DOCMAP, TL("!!set", L{
  N(KEYVAL, "Mark McGwire", /*"~"*/{}),
  N(KEYVAL, "Sammy Sosa", /*"~"*/{}),
  N(KEYVAL, "Ken Griff", /*"~"*/{}),})
)})),

C("sets 2XXW, indented",
R"(
    --- !!set
    ? Mark McGwire
    ? Sammy Sosa
    ? Ken Griff
)",
N(STREAM, L{N(DOCMAP, TL("!!set", L{
  N(KEYVAL, "Mark McGwire", /*"~"*/{}),
  N(KEYVAL, "Sammy Sosa", /*"~"*/{}),
  N(KEYVAL, "Ken Griff", /*"~"*/{}),})
)})),

C("sets 2XXW, no set",
R"(
---
? Mark McGwire
? Sammy Sosa
? Ken Griff
)",
N(STREAM, L{N(DOCMAP, L{
  N(KEYVAL, "Mark McGwire", /*"~"*/{}),
  N(KEYVAL, "Sammy Sosa", /*"~"*/{}),
  N(KEYVAL, "Ken Griff", /*"~"*/{}),}
)})),

C("sets 2XXW, no doc",
R"(!!set
? Mark McGwire
? Sammy Sosa
? Ken Griff
)",
TL("!!set", L{
  N(KEYVAL, "Mark McGwire", /*"~"*/{}),
  N(KEYVAL, "Sammy Sosa", /*"~"*/{}),
  N(KEYVAL, "Ken Griff", /*"~"*/{}),
})),

C("sets 2XXW, no doc, no set",
R"(
? Mark McGwire
? Sammy Sosa
? Ken Griff
)",
L{
  N(KEYVAL, "Mark McGwire", /*"~"*/{}),
  N(KEYVAL, "Sammy Sosa", /*"~"*/{}),
  N(KEYVAL, "Ken Griff", /*"~"*/{}),
}),

    )
}

INSTANTIATE_GROUP(SIMPLE_SET)

} // namespace yml
} // namespace c4
