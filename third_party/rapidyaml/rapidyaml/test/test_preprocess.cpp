#include <c4/yml/std/string.hpp>
#include <c4/yml/preprocess.hpp>
#include <gtest/gtest.h>
#include "./test_case.hpp"

namespace c4 {
namespace yml {

#define _test(val, expected)                                \
    EXPECT_EQ(preprocess_json<std::string>(val), expected)


TEST(preprocess_json, basic)
{
    _test("", "");
    _test("{}", "{}");
    _test(R"("a":"b")",
          R"("a": "b")");
    _test(R"('a':'b')",
          R"('a': 'b')");
    _test(R"({'a':'b'})",
          R"({'a': 'b'})");
    _test(R"({"a":"b"})",
          R"({"a": "b"})");

    _test(R"({"a":{"a":"b"}})",
          R"({"a": {"a": "b"}})");
    _test(R"({'a':{'a':'b'}})",
          R"({'a': {'a': 'b'}})");
}

TEST(preprocess_json, github52)
{
    _test(R"({"a": "b","c": 42,"d": "e"})",
          R"({"a": "b","c": 42,"d": "e"})");
    _test(R"({"aaaa": "bbbb","cccc": 424242,"dddddd": "eeeeeee"})",
          R"({"aaaa": "bbbb","cccc": 424242,"dddddd": "eeeeeee"})");

    _test(R"({"a":"b","c":42,"d":"e"})",
          R"({"a": "b","c": 42,"d": "e"})");
    _test(R"({"aaaaa":"bbbbb","ccccc":424242,"ddddd":"eeeee"})",
          R"({"aaaaa": "bbbbb","ccccc": 424242,"ddddd": "eeeee"})");
    _test(R"({"a":"b","c":{},"d":"e"})",
          R"({"a": "b","c": {},"d": "e"})");
    _test(R"({"aaaaa":"bbbbb","ccccc":{    },"ddddd":"eeeee"})",
          R"({"aaaaa": "bbbbb","ccccc": {    },"ddddd": "eeeee"})");
    _test(R"({"a":"b","c":true,"d":"e"})",
          R"({"a": "b","c": true,"d": "e"})");
    _test(R"({"a":"b","c":false,"d":"e"})",
          R"({"a": "b","c": false,"d": "e"})");
    _test(R"({"a":"b","c":true,"d":"e"})",
          R"({"a": "b","c": true,"d": "e"})");
    _test(R"({"a":"b","c":null,"d":"e"})",
          R"({"a": "b","c": null,"d": "e"})");
    _test(R"({"aaaaa":"bbbbb","ccccc":false,"ddddd":"eeeee"})",
          R"({"aaaaa": "bbbbb","ccccc": false,"ddddd": "eeeee"})");
    _test(R"({"a":"b","c":false,"d":"e"})",
          R"({"a": "b","c": false,"d": "e"})");
    _test(R"({"aaaaa":"bbbbb","ccccc":true,"ddddd":"eeeee"})",
          R"({"aaaaa": "bbbbb","ccccc": true,"ddddd": "eeeee"})");
}

TEST(preprocess_json, nested)
{
    _test(R"({"a":"b","c":{"a":"b","c":{},"d":"e"},"d":"e"})",
          R"({"a": "b","c": {"a": "b","c": {},"d": "e"},"d": "e"})");
    _test(R"({"a":"b","c":{"a":"b","c":{"a":"b","c":{},"d":"e"},"d":"e"},"d":"e"})",
          R"({"a": "b","c": {"a": "b","c": {"a": "b","c": {},"d": "e"},"d": "e"},"d": "e"})");
    _test(R"({"a":"b","c":{"a":"b","c":{"a":"b","c":{"a":"b","c":{},"d":"e"},"d":"e"},"d":"e"},"d":"e"})",
          R"({"a": "b","c": {"a": "b","c": {"a": "b","c": {"a": "b","c": {},"d": "e"},"d": "e"},"d": "e"},"d": "e"})");
    _test(R"({"a":"b","c":{"a":"b","c":{"a":"b","c":{"a":"b","c":{"a":"b","c":{},"d":"e"},"d":"e"},"d":"e"},"d":"e"},"d":"e"})",
          R"({"a": "b","c": {"a": "b","c": {"a": "b","c": {"a": "b","c": {"a": "b","c": {},"d": "e"},"d": "e"},"d": "e"},"d": "e"},"d": "e"})");

    _test(R"({"a":"b","c":["a","c","d","e"],"d":"e"})",
          R"({"a": "b","c": ["a","c","d","e"],"d": "e"})");
}

TEST(preprocess_json, nested_end)
{
    _test(R"({"a":"b","d":"e","c":{"a":"b","d":"e","c":{}}})",
          R"({"a": "b","d": "e","c": {"a": "b","d": "e","c": {}}})");
    _test(R"({"a":"b","d":"e","c":{"a":"b","d":"e","c":{"a":"b","d":"e","c":{}}}})",
          R"({"a": "b","d": "e","c": {"a": "b","d": "e","c": {"a": "b","d": "e","c": {}}}})");
    _test(R"({"a":"b","d":"e","c":{"a":"b","d":"e","c":{"a":"b","d":"e","c":{"a":"b","d":"e","c":{}}}}})",
          R"({"a": "b","d": "e","c": {"a": "b","d": "e","c": {"a": "b","d": "e","c": {"a": "b","d": "e","c": {}}}}})");
    _test(R"({"a":"b","d":"e","c":{"a":"b","d":"e","c":{"a":"b","d":"e","c":{"a":"b","d":"e","c":{"a":"b","d":"e","c":{}}}}}})",
          R"({"a": "b","d": "e","c": {"a": "b","d": "e","c": {"a": "b","d": "e","c": {"a": "b","d": "e","c": {"a": "b","d": "e","c": {}}}}}})");
}

#undef _test


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST(preprocess, rxmap_basic)
{
    #define _test(val, expected)                                \
        EXPECT_EQ(preprocess_rxmap<std::string>(val), expected)

    _test("{}", "{}");
    _test("a", "{a: 1}");
    _test("{a}", "{a: 1}");
    _test("a, b, c", "{a: 1, b: 1, c: 1}");
    _test("a,b,c", "{a,b,c: 1}");
    _test("a a a a, b, c", "{a a a a: 1, b: 1, c: 1}");
    _test(",", "{,}");

    _test("a: [b, c, d]", "{a: [b, c, d]}");
    _test("a:b: [b, c, d]", "{a:b: [b, c, d]}");
    _test("a,b: [b, c, d]", "{a,b: [b, c, d]}");
    _test("a: {b, c, d}", "{a: {b, c, d}}");
    _test("a: {b: {f, g}, c: {h, i}, d: {j, k}}",
          "{a: {b: {f, g}, c: {h, i}, d: {j, k}}}");
    _test("a: {b: {f g}, c: {f g}, d: {j, k}}",
          "{a: {b: {f g}, c: {f g}, d: {j, k}}}");

    #undef _test
}



// The other test executables are written to contain the declarative-style
// YmlTestCases. This executable does not have any but the build setup
// assumes it does, and links with the test lib, which requires an existing
// get_case() function. So this is here to act as placeholder until (if?)
// proper test cases are added here.
Case const* get_case(csubstr)
{
    return nullptr;
}

} // namespace yml
} // namespace c4
