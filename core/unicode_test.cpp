/*
Copyright 2025 Google Inc. All rights reserved.

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

#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include "unicode.h"
#include "gtest/gtest.h"

namespace jsonnet::internal {
namespace {

void testEncodeDecode(char32_t codepoint, const std::string &expect_utf8) {
    std::string buffer;
    size_t len = encode_utf8(codepoint, buffer);
    EXPECT_EQ(len, expect_utf8.size());
    EXPECT_EQ(buffer, expect_utf8);

    size_t at = 0;
    char32_t decoded = decode_utf8(expect_utf8, at);
    EXPECT_EQ(decoded, codepoint);
    EXPECT_EQ(at, expect_utf8.size() - 1);
}

TEST(Unicode, TestUTF8)
{
    // ASCII encodes as itself.
    testEncodeDecode(0x00, std::string("\x00", 1));
    testEncodeDecode(0x41, "A");
    testEncodeDecode(0x7f, "\x7f");

    testEncodeDecode(0x80, "\xc2\x80");
    testEncodeDecode(0x100, "\xc4\x80");
    testEncodeDecode(0x7ff, "\xdf\xbf");

    testEncodeDecode(0x800, "\xe0\xa0\x80");
    testEncodeDecode(0x1482, "\xe1\x92\x82");
    testEncodeDecode(0xffff, "\xef\xbf\xbf");

    testEncodeDecode(0x010000, "\xf0\x90\x80\x80");
    testEncodeDecode(0x01f600, "\xf0\x9f\x98\x80"); // U+1F600 "Grinning Face"
    testEncodeDecode(0x0f057e, "\xf3\xb0\x95\xbe"); // U+F057E Private use area character
    testEncodeDecode(0x10ffff, "\xf4\x8f\xbf\xbf");
}

TEST(Unicode, TestUTF8RejectBad)
{
    const auto test_cases = std::array<const char*, 20>{
        "\x80",  // Continuation byte without leading byte
        "\xa0",  // Continuation byte without leading byte
        "\xbf",  // Continuation byte without leading byte
        "\xc0",  // Leading byte for 2-byte sequence (missing tail)
        "\xe0",  // Leading byte for 3-byte sequence (missing tail)
        "\xf0",  // Leading byte for 4-byte sequence (missing tail)
        "\xf8\x83\x83\x83",  // Invalid leading byte
        "\xe0\x80",  // Leading byte for 3-byte sequence (missing tail)
        "\xf0\x80",  // Leading byte for 4-byte sequence (missing tail)
        "\xf0\x80\x80",  // Leading byte for 4-byte sequence (missing tail)
        "\xc0\xcf",  // Leading byte for 2-byte sequence (incorrect tail)
        "\xe0\xcf",  // Leading byte for 3-byte sequence (incorrect tail)
        "\xf0\xcf",  // Leading byte for 4-byte sequence (incorrect tail)
        "\xe0\xcf\x80",  // Leading byte for 3-byte sequence (incorrect tail)
        "\xf0\xcf\x80",  // Leading byte for 4-byte sequence (incorrect tail)
        "\xe0\x80\xcf",  // Leading byte for 3-byte sequence (incorrect tail)
        "\xf0\x80\xcf",  // Leading byte for 4-byte sequence (incorrect tail)
        "\xf0\x80\x80\xcf",  // Leading byte for 4-byte sequence (incorrect tail)
        "\xf0\x80\xcf\x80",  // Leading byte for 4-byte sequence (incorrect tail)
        "\xf0\xcf\x80\x80",  // Leading byte for 4-byte sequence (incorrect tail)
    };
    for (size_t i = 0; i < test_cases.size(); ++i) {
        const auto str = test_cases[i];
        size_t at = 0;
        char32_t c = decode_utf8(str, at);

        EXPECT_EQ(c, JSONNET_CODEPOINT_ERROR) << "expect decode to reject. case " << i << std::endl;
    }
}

TEST(Unicode, TestUTF8RoundTripExhaustive)
{
    // Encode every Unicode code-point as UTF-8 and verify that
    // it decodes to the same value.
    std::string buffer;
    for (int x = 0; x < JSONNET_CODEPOINT_MAX; ++x) {
        if (x == JSONNET_CODEPOINT_ERROR) {
            continue;
        }
        buffer.clear();
        encode_utf8(x, buffer);

        size_t at = 0;
        char32_t y = decode_utf8(buffer, at);
        EXPECT_NE(y, JSONNET_CODEPOINT_ERROR) << "UTF-8 roundtrip failed for codepoint " << x << " decode rejects" << std::endl;
        EXPECT_EQ(x, y) << "UTF-8 roundtrip failed for codepoint " << x << " converts to " << y << std::endl;
        EXPECT_EQ(at, buffer.size() - 1) << "UTF-8 roundtrip failed for codepoint " << x << " decodes incorrect length" << std::endl;
    }
}

}  // namespace
}  // namespace jsonnet::internal
