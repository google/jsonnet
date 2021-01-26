#include <gtest/gtest.h>
#include <c4/yml/std/std.hpp>
#include <c4/yml/yml.hpp>
#include <initializer_list>
#include <string>
#include <iostream>

#include "./test_case.hpp"

namespace c4 {
namespace yml {

// The other test executables are written to contain the declarative-style
// YmlTestCases. This executable does not have any but the build setup
// assumes it does, and links with the test lib, which requires an existing
// get_case() function. So this is here to act as placeholder until (if?)
// proper test cases are added here. This was detected in #47 (thanks
// @cburgard).
Case const* get_case(csubstr)
{
    return nullptr;
}


void test_merge(std::initializer_list<csubstr> li, csubstr expected)
{
    Tree loaded, merged, ref;

    parse(expected, &ref);

    // make sure the arena in the loaded tree is never resized
    size_t arena_dim = 2;
    for(csubstr src : li)
    {
        arena_dim += src.len;
    }
    loaded.reserve_arena(arena_dim);

    for(csubstr src : li)
    {
        loaded.clear(); // do not clear the arena of the loaded tree
        parse(src, &loaded);
        merged.merge_with(&loaded);
    }

    auto buf_result = emitrs<std::string>(merged);
    auto buf_expected = emitrs<std::string>(ref);

    EXPECT_EQ(buf_result, buf_expected);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST(merge, seq_no_overlap_explicit)
{
    test_merge(
        {"[0, 1, 2]", "[3, 4, 5]", "[6, 7, 8]"},
        "[0, 1, 2, 3, 4, 5, 6, 7, 8]"
    );
}


TEST(merge, seq_no_overlap_implicit)
{
    test_merge(
        {"0, 1, 2", "3, 4, 5", "6, 7, 8"},
        "0, 1, 2, 3, 4, 5, 6, 7, 8"
    );
}


TEST(merge, seq_overlap_explicit)
{
    test_merge(
        {"[0, 1, 2]", "[1, 2, 3]", "[2, 3, 4]"},
        "[0, 1, 2, 1, 2, 3, 2, 3, 4]"
        // or this? "[0, 1, 2, 3, 4]"
    );
}


TEST(merge, seq_overlap_implicit)
{
    // now a bit more difficult
    test_merge(
        {"0, 1, 2", "1, 2, 3", "2, 3, 4"},
        "0, 1, 2, 1, 2, 3, 2, 3, 4"
        // or this? "0, 1, 2, 3, 4"
    );
}


TEST(merge, map_orthogonal)
{
    test_merge(
        {"a: 0", "b: 1", "c: 2"},
        "{a: 0, b: 1, c: 2}"
    );
}


TEST(merge, map_overriding)
{
    test_merge(
        {
            "a: 0",
            "{a: 1, b: 1}",
            "c: 2"
        },
        "{a: 1, b: 1, c: 2}"
    );
}

TEST(merge, map_overriding_multiple)
{
    test_merge(
        {
            "a: 0",
            "{a: 1, b: 1}",
            "c: 2",
            "a: 2",
            "a: 3",
            "c: 4",
            "c: 5",
            "a: 4",
        },
        "{a: 4, b: 1, c: 5}"
    );
}


TEST(merge, seq_nested_in_map)
{
    test_merge(
        {
            "{a: 0, seq: [a, b, c], d: 2}",
            "{a: 1, seq: [d, e, f], d: 3, c: 3}"
        },
        "{a: 1, seq: [a, b, c, d, e, f], d: 3, c: 3}"
    );
}


TEST(merge, seq_nested_in_map_override_with_map)
{
    test_merge(
        {
            "{a: 0, ovr: [a, b, c], d: 2}",
            "{a: 1, ovr: {d: 0, b: 1, c: 2}, d: 3, c: 3}"
        },
        "{a: 1, ovr: {d: 0, b: 1, c: 2}, d: 3, c: 3}"
    );
}


TEST(merge, seq_nested_in_map_override_with_keyval)
{
    test_merge(
        {
            "{a: 0, ovr: [a, b, c], d: 2}",
            "{a: 1, ovr: foo, d: 3, c: 3}"
        },
        "{a: 1, ovr: foo, d: 3, c: 3}"
    );
}


} // namespace yml
} // namespace c4
