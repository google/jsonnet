#include "c4/test.hpp"
#include "c4/std/string.hpp"

namespace c4 {

TEST_CASE("std_string.to_substr")
{
    std::string s("barnabe");
    substr ss = to_substr(s);
    CHECK_EQ(ss.str, s.data());
    CHECK_EQ(ss.len, s.size());
    s[0] = 'B';
    CHECK_EQ(ss[0], 'B');
    ss[0] = 'b';
    CHECK_EQ(s[0], 'b');
}

TEST_CASE("std_string.to_csubstr")
{
    std::string s("barnabe");
    csubstr ss = to_csubstr(s);
    CHECK_EQ(ss.str, s.data());
    CHECK_EQ(ss.len, s.size());
    s[0] = 'B';
    CHECK_EQ(ss[0], 'B');
}

} // namespace c4
