#include "c4/char_traits.hpp"

#include "c4/test.hpp"

namespace c4 {

TEST_CASE("num_needed_chars.char")
{
    CHECK_EQ(num_needed_chars<char>(0), 0);
    CHECK_EQ(num_needed_chars<char>(1), 1);
    CHECK_EQ(num_needed_chars<char>(2), 2);
    CHECK_EQ(num_needed_chars<char>(3), 3);
    CHECK_EQ(num_needed_chars<char>(4), 4);
    for(int i = 0; i < 100; ++i)
    {
        CHECK_EQ(num_needed_chars<char>(i), i);
    }
}

TEST_CASE("num_needed_chars.wchar_t")
{
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4127) // C4127: conditional expression is constant
#endif
    if(sizeof(wchar_t) == 2)
    {
        CHECK_EQ(num_needed_chars<wchar_t>(  0),  0);
        CHECK_EQ(num_needed_chars<wchar_t>(  1),  1);
        CHECK_EQ(num_needed_chars<wchar_t>(  2),  1);
        CHECK_EQ(num_needed_chars<wchar_t>(  3),  2);
        CHECK_EQ(num_needed_chars<wchar_t>(  4),  2);
        CHECK_EQ(num_needed_chars<wchar_t>( 97), 49);
        CHECK_EQ(num_needed_chars<wchar_t>( 98), 49);
        CHECK_EQ(num_needed_chars<wchar_t>( 99), 50);
        CHECK_EQ(num_needed_chars<wchar_t>(100), 50);
        CHECK_EQ(num_needed_chars<wchar_t>(101), 51);
    }
    else if(sizeof(wchar_t) == 4)
    {
        CHECK_EQ(num_needed_chars<wchar_t>(  0),  0);
        CHECK_EQ(num_needed_chars<wchar_t>(  1),  1);
        CHECK_EQ(num_needed_chars<wchar_t>(  2),  1);
        CHECK_EQ(num_needed_chars<wchar_t>(  3),  1);
        CHECK_EQ(num_needed_chars<wchar_t>(  4),  1);
        CHECK_EQ(num_needed_chars<wchar_t>(  5),  2);
        CHECK_EQ(num_needed_chars<wchar_t>(  6),  2);
        CHECK_EQ(num_needed_chars<wchar_t>(  7),  2);
        CHECK_EQ(num_needed_chars<wchar_t>(  8),  2);
        CHECK_EQ(num_needed_chars<wchar_t>( 93), 24);
        CHECK_EQ(num_needed_chars<wchar_t>( 94), 24);
        CHECK_EQ(num_needed_chars<wchar_t>( 95), 24);
        CHECK_EQ(num_needed_chars<wchar_t>( 96), 24);
        CHECK_EQ(num_needed_chars<wchar_t>( 97), 25);
        CHECK_EQ(num_needed_chars<wchar_t>( 98), 25);
        CHECK_EQ(num_needed_chars<wchar_t>( 99), 25);
        CHECK_EQ(num_needed_chars<wchar_t>(100), 25);
        CHECK_EQ(num_needed_chars<wchar_t>(101), 26);
    }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
}

} // namespace c4
