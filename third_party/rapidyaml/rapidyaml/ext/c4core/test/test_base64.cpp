#include "c4/test.hpp"
#include "c4/std/string.hpp"
#include "c4/std/vector.hpp"
#include "c4/format.hpp"
#include "c4/base64.hpp"

#include "c4/libtest/supprwarn_push.hpp"

namespace c4 {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<class T, class U>
void test_base64_str(T const& val, csubstr expected, U *ws)
{
    char buf_[512];
    substr buf(buf_);

    csubstr encoded = to_chars_sub(buf, fmt::base64(val));
    CHECK(base64_valid(encoded));
    CHECK_EQ(encoded, expected);
    CHECK_EQ(encoded.len % 4, 0);

    auto req = fmt::base64(*ws);
    size_t written = from_chars(encoded, &req);
    CHECK_EQ(ws->first(written), val);
}

template<class T>
void test_base64(T const& val, csubstr expected, T *ws)
{
    char buf_[512];
    substr buf(buf_);

    csubstr encoded = to_chars_sub(buf, fmt::base64(val));
    CHECK(base64_valid(encoded));
    CHECK_EQ(encoded, expected);
    CHECK_EQ(encoded.len % 4, 0);

    auto req = fmt::base64(*ws);
    size_t written = from_chars(encoded, &req);
    CHECK_EQ(written, sizeof(T));
    CHECK_EQ(*ws, val);
}

template<class T>
struct base64_test_pair
{
    T val;
    csubstr encoded;
};


base64_test_pair<csubstr> base64_str_pairs[] = {
#define __(val, expected) {csubstr(val), csubstr(expected)}
    __(""                    , ""                            ),
    __("0"                   , "MA=="                        ),
    __("1"                   , "MQ=="                        ),
    __("2"                   , "Mg=="                        ),
    __("3"                   , "Mw=="                        ),
    __("4"                   , "NA=="                        ),
    __("5"                   , "NQ=="                        ),
    __("6"                   , "Ng=="                        ),
    __("7"                   , "Nw=="                        ),
    __("8"                   , "OA=="                        ),
    __("9"                   , "OQ=="                        ),
    __("10"                  , "MTA="                        ),
    __("123"                 , "MTIz"                        ),
    __("1234"                , "MTIzNA=="                    ),
    __("1235"                , "MTIzNQ=="                    ),
    __("Man"                 , "TWFu"                        ),
    __("Ma"                  , "TWE="                        ),
    __("M"                   , "TQ=="                        ),
    __("any carnal pleasure.", "YW55IGNhcm5hbCBwbGVhc3VyZS4="),
    __("any carnal pleasure" , "YW55IGNhcm5hbCBwbGVhc3VyZQ=="),
    __("any carnal pleasur"  , "YW55IGNhcm5hbCBwbGVhc3Vy"    ),
    __("any carnal pleasu"   , "YW55IGNhcm5hbCBwbGVhc3U="    ),
    __("any carnal pleas"    , "YW55IGNhcm5hbCBwbGVhcw=="    ),
    __("pleasure."           , "cGxlYXN1cmUu"                ),
    __( "leasure."           , "bGVhc3VyZS4="                ),
    __(  "easure."           , "ZWFzdXJlLg=="                ),
    __(   "asure."           , "YXN1cmUu"                    ),
    __(    "sure."           , "c3VyZS4="                    ),
#undef __
};

base64_test_pair<int> base64_int_pairs[] = {
#define __(val, expected) {val, csubstr(expected)}
    __(   0, "AAAAAA=="),
    __(   1, "AQAAAA=="),
    __(   2, "AgAAAA=="),
    __(   3, "AwAAAA=="),
    __(   4, "BAAAAA=="),
    __(   5, "BQAAAA=="),
    __(   6, "BgAAAA=="),
    __(   7, "BwAAAA=="),
    __(   8, "CAAAAA=="),
    __(   9, "CQAAAA=="),
    __(  10, "CgAAAA=="),
    __(1234, "0gQAAA=="),
#undef __
};

TEST_CASE("base64.str")
{
    char buf_[512];
    substr buf(buf_);

    for(auto p : base64_str_pairs)
    {
        INFO(p.val);
        test_base64_str(p.val, p.encoded, &buf);
    }
}

TEST_CASE("base64.int")
{
    int val;

    for(auto p : base64_int_pairs)
    {
        INFO(p.val);
        test_base64(p.val, p.encoded, &val);
    }
}

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
