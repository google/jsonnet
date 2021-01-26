#include <vector>
#include <sstream>

#include <c4/bitmask.hpp>
#include <c4/std/vector.hpp>

#include <c4/test.hpp>

#include "./test_enum_common.hpp"

template<typename Enum>
void cmp_enum(Enum lhs, Enum rhs)
{
    using I = typename std::underlying_type<Enum>::type;
    CHECK_EQ(static_cast<I>(lhs), static_cast<I>(rhs));
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


TEST_CASE("str2bm.simple_bitmask")
{
    using namespace c4;
    std::vector<char> str;

    CHECK_EQ(BM_NONE,              str2bm<MyBitmask>("BM_NONE"));
    CHECK_EQ(BM_NONE,              str2bm<MyBitmask>("NONE"));
    CHECK_EQ(BM_NONE,              str2bm<MyBitmask>("0"));
    CHECK_EQ(BM_NONE,              str2bm<MyBitmask>("0x0"));

    CHECK_EQ(BM_FOO,               str2bm<MyBitmask>("BM_FOO"));
    CHECK_EQ(BM_FOO,               str2bm<MyBitmask>("FOO"));
    CHECK_EQ(BM_FOO,               str2bm<MyBitmask>("1"));
    CHECK_EQ(BM_FOO,               str2bm<MyBitmask>("0x1"));
    CHECK_EQ(BM_FOO,               str2bm<MyBitmask>("BM_NONE|0x1"));

    CHECK_EQ(BM_BAR,               str2bm<MyBitmask>("BM_BAR"));
    CHECK_EQ(BM_BAR,               str2bm<MyBitmask>("BAR"));
    CHECK_EQ(BM_BAR,               str2bm<MyBitmask>("2"));
    CHECK_EQ(BM_BAR,               str2bm<MyBitmask>("0x2"));
    CHECK_EQ(BM_BAR,               str2bm<MyBitmask>("BM_NONE|0x2"));

    CHECK_EQ(BM_BAZ,               str2bm<MyBitmask>("BM_BAZ"));
    CHECK_EQ(BM_BAZ,               str2bm<MyBitmask>("BAZ"));
    CHECK_EQ(BM_BAZ,               str2bm<MyBitmask>("4"));
    CHECK_EQ(BM_BAZ,               str2bm<MyBitmask>("0x4"));

    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("BM_FOO|BM_BAR"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("BM_FOO|BAR"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("FOO|BM_BAR"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("BM_FOO_BAR"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("FOO_BAR"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("FOO|BAR"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("0x1|0x2"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("1|2"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("0x3"));
    CHECK_EQ(BM_FOO_BAR,           str2bm<MyBitmask>("3"));

    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("BM_FOO|BM_BAR|BM_BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("BM_FOO|BM_BAR|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("BM_FOO|BAR|BM_BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO|BM_BAR|BM_BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO|BM_BAR|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO|BAR|BM_BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO|BAR|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO_BAR|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("BM_FOO_BAR|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("0x1|BAR|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO|0x2|BAZ"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("FOO|BAR|0x4"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("0x1|0x2|0x4"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("0x7"));
    CHECK_EQ(BM_FOO_BAR_BAZ,       str2bm<MyBitmask>("7"));
}

TEST_CASE("str2bm.scoped_bitmask")
{
    using namespace c4;
    std::vector<char> str;
    using bmt = MyBitmaskClass;

    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("MyBitmaskClass::BM_NONE"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_BAR"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO_BAR"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO_BAR_BAZ"));
    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("BM_NONE"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("BM_FOO"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("BM_BAR"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("BM_FOO_BAR"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("BM_FOO_BAR_BAZ"));
    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("NONE"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("FOO"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("BAR"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("BAZ"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("FOO_BAR"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO_BAR_BAZ"));

    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("NONE"));
    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("BM_NONE"));
    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("MyBitmaskClass::BM_NONE"));
    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("0"));
    cmp_enum(bmt::BM_NONE,              (bmt)str2bm<bmt>("0x0"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("FOO"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("BM_FOO"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("1"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("0x1"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("NONE|0x1"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("BM_NONE|0x1"));
    cmp_enum(bmt::BM_FOO,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_NONE|0x1"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("BAR"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("BM_BAR"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_BAR"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("2"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("0x2"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("BAZ"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("BM_BAZ"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_BAZ"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("BM_NONE|0x2"));
    cmp_enum(bmt::BM_BAR,               (bmt)str2bm<bmt>("MyBitmaskClass::BM_NONE|0x2"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("4"));
    cmp_enum(bmt::BM_BAZ,               (bmt)str2bm<bmt>("0x4"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("BM_FOO|BM_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO|MyBitmaskClass::BM_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("BM_FOO|BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO|BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("FOO|BM_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("FOO|MyBitmaskClass::BM_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("BM_FOO_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("FOO_BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("FOO|BAR"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("0x1|0x2"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("1|2"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("0x3"));
    cmp_enum(bmt::BM_FOO_BAR,           (bmt)str2bm<bmt>("3"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("BM_FOO|BM_BAR|BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO|MyBitmaskClass::BM_BAR|MyBitmaskClass::BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("BM_FOO|BM_BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO|MyBitmaskClass::BM_BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("BM_FOO|BAR|BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("BM_FOO|BAR|MyBitmaskClass::BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|BM_BAR|BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|MyBitmaskClass::BM_BAR|MyBitmaskClass::BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|BM_BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|MyBitmaskClass::BM_BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|BAR|BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|BAR|MyBitmaskClass::BM_BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO_BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("MyBitmaskClass::BM_FOO_BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("0x1|BAR|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|0x2|BAZ"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("FOO|BAR|0x4"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("0x1|0x2|0x4"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("0x7"));
    cmp_enum(bmt::BM_FOO_BAR_BAZ,       (bmt)str2bm<bmt>("0x7"));
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template<class Enum>
const char* do_bm2str(Enum e, std::vector<char> *s, c4::EnumOffsetType which)
{
    size_t len = c4::bm2str<Enum>(e, nullptr, 0, which);
    C4_CHECK(len > 0);
    s->resize(len);
    C4_CHECK(s->data() != nullptr);
    c4::bm2str<Enum>(e, s->data(), s->size(), which);
    return s->data();
}

TEST_CASE("bm2str.simple_bitmask")
{
    using namespace c4;
    std::vector<char> str;

    CHECK_STREQ(do_bm2str(BM_NONE,        &str, EOFFS_NONE), "BM_NONE");
    CHECK_STREQ(do_bm2str(BM_FOO,         &str, EOFFS_NONE), "BM_FOO");
    CHECK_STREQ(do_bm2str(BM_BAR,         &str, EOFFS_NONE), "BM_BAR");
    CHECK_STREQ(do_bm2str(BM_BAZ,         &str, EOFFS_NONE), "BM_BAZ");
    CHECK_STREQ(do_bm2str(BM_FOO_BAR,     &str, EOFFS_NONE), "BM_FOO_BAR");
    CHECK_STREQ(do_bm2str(BM_FOO_BAR_BAZ, &str, EOFFS_NONE), "BM_FOO_BAR_BAZ");
    CHECK_STREQ(do_bm2str(BM_NONE,        &str, EOFFS_CLS ), "BM_NONE");
    CHECK_STREQ(do_bm2str(BM_FOO,         &str, EOFFS_CLS ), "BM_FOO");
    CHECK_STREQ(do_bm2str(BM_BAR,         &str, EOFFS_CLS ), "BM_BAR");
    CHECK_STREQ(do_bm2str(BM_BAZ,         &str, EOFFS_CLS ), "BM_BAZ");
    CHECK_STREQ(do_bm2str(BM_FOO_BAR,     &str, EOFFS_CLS ), "BM_FOO_BAR");
    CHECK_STREQ(do_bm2str(BM_FOO_BAR_BAZ, &str, EOFFS_CLS ), "BM_FOO_BAR_BAZ");
    CHECK_STREQ(do_bm2str(BM_NONE,        &str, EOFFS_PFX ), "NONE");
    CHECK_STREQ(do_bm2str(BM_FOO,         &str, EOFFS_PFX ), "FOO");
    CHECK_STREQ(do_bm2str(BM_BAR,         &str, EOFFS_PFX ), "BAR");
    CHECK_STREQ(do_bm2str(BM_BAZ,         &str, EOFFS_PFX ), "BAZ");
    CHECK_STREQ(do_bm2str(BM_FOO_BAR,     &str, EOFFS_PFX ), "FOO_BAR");
    CHECK_STREQ(do_bm2str(BM_FOO_BAR_BAZ, &str, EOFFS_PFX ), "FOO_BAR_BAZ");
}

TEST_CASE("bm2str.scoped_bitmask")
{
    using namespace c4;
    std::vector<char> str;

    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_NONE,        &str, EOFFS_NONE), "MyBitmaskClass::BM_NONE");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO,         &str, EOFFS_NONE), "MyBitmaskClass::BM_FOO");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_BAR,         &str, EOFFS_NONE), "MyBitmaskClass::BM_BAR");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_BAZ,         &str, EOFFS_NONE), "MyBitmaskClass::BM_BAZ");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO_BAR,     &str, EOFFS_NONE), "MyBitmaskClass::BM_FOO_BAR");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO_BAR_BAZ, &str, EOFFS_NONE), "MyBitmaskClass::BM_FOO_BAR_BAZ");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_NONE,        &str, EOFFS_CLS ), "BM_NONE");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO,         &str, EOFFS_CLS ), "BM_FOO");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_BAR,         &str, EOFFS_CLS ), "BM_BAR");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_BAZ,         &str, EOFFS_CLS ), "BM_BAZ");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO_BAR,     &str, EOFFS_CLS ), "BM_FOO_BAR");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO_BAR_BAZ, &str, EOFFS_CLS ), "BM_FOO_BAR_BAZ");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_NONE,        &str, EOFFS_PFX ), "NONE");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO,         &str, EOFFS_PFX ), "FOO");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_BAR,         &str, EOFFS_PFX ), "BAR");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_BAZ,         &str, EOFFS_PFX ), "BAZ");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO_BAR,     &str, EOFFS_PFX ), "FOO_BAR");
    CHECK_STREQ(do_bm2str(MyBitmaskClass::BM_FOO_BAR_BAZ, &str, EOFFS_PFX ), "FOO_BAR_BAZ");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------

template<class Enum>
std::string do_bm2stream(Enum e, c4::EnumOffsetType which)
{
    std::stringstream ss;
    c4::bm2stream<Enum>(ss, e, which);
    return ss.str();
}

TEST_CASE("bm2stream.simple_bitmask")
{
    using namespace c4;

    CHECK_EQ(do_bm2stream(BM_NONE,        EOFFS_NONE), "BM_NONE");
    CHECK_EQ(do_bm2stream(BM_FOO,         EOFFS_NONE), "BM_FOO");
    CHECK_EQ(do_bm2stream(BM_BAR,         EOFFS_NONE), "BM_BAR");
    CHECK_EQ(do_bm2stream(BM_BAZ,         EOFFS_NONE), "BM_BAZ");
    CHECK_EQ(do_bm2stream(BM_FOO_BAR,     EOFFS_NONE), "BM_FOO_BAR");
    CHECK_EQ(do_bm2stream(BM_FOO_BAR_BAZ, EOFFS_NONE), "BM_FOO_BAR_BAZ");
    CHECK_EQ(do_bm2stream(BM_NONE,        EOFFS_CLS ), "BM_NONE");
    CHECK_EQ(do_bm2stream(BM_FOO,         EOFFS_CLS ), "BM_FOO");
    CHECK_EQ(do_bm2stream(BM_BAR,         EOFFS_CLS ), "BM_BAR");
    CHECK_EQ(do_bm2stream(BM_BAZ,         EOFFS_CLS ), "BM_BAZ");
    CHECK_EQ(do_bm2stream(BM_FOO_BAR,     EOFFS_CLS ), "BM_FOO_BAR");
    CHECK_EQ(do_bm2stream(BM_FOO_BAR_BAZ, EOFFS_CLS ), "BM_FOO_BAR_BAZ");
    CHECK_EQ(do_bm2stream(BM_NONE,        EOFFS_PFX ), "NONE");
    CHECK_EQ(do_bm2stream(BM_FOO,         EOFFS_PFX ), "FOO");
    CHECK_EQ(do_bm2stream(BM_BAR,         EOFFS_PFX ), "BAR");
    CHECK_EQ(do_bm2stream(BM_BAZ,         EOFFS_PFX ), "BAZ");
    CHECK_EQ(do_bm2stream(BM_FOO_BAR,     EOFFS_PFX ), "FOO_BAR");
    CHECK_EQ(do_bm2stream(BM_FOO_BAR_BAZ, EOFFS_PFX ), "FOO_BAR_BAZ");
}

TEST_CASE("bm2stream.scoped_bitmask")
{
    using namespace c4;

    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_NONE,        EOFFS_NONE), "MyBitmaskClass::BM_NONE");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO,         EOFFS_NONE), "MyBitmaskClass::BM_FOO");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_BAR,         EOFFS_NONE), "MyBitmaskClass::BM_BAR");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_BAZ,         EOFFS_NONE), "MyBitmaskClass::BM_BAZ");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO_BAR,     EOFFS_NONE), "MyBitmaskClass::BM_FOO_BAR");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO_BAR_BAZ, EOFFS_NONE), "MyBitmaskClass::BM_FOO_BAR_BAZ");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_NONE,        EOFFS_CLS ), "BM_NONE");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO,         EOFFS_CLS ), "BM_FOO");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_BAR,         EOFFS_CLS ), "BM_BAR");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_BAZ,         EOFFS_CLS ), "BM_BAZ");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO_BAR,     EOFFS_CLS ), "BM_FOO_BAR");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO_BAR_BAZ, EOFFS_CLS ), "BM_FOO_BAR_BAZ");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_NONE,        EOFFS_PFX ), "NONE");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO,         EOFFS_PFX ), "FOO");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_BAR,         EOFFS_PFX ), "BAR");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_BAZ,         EOFFS_PFX ), "BAZ");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO_BAR,     EOFFS_PFX ), "FOO_BAR");
    CHECK_EQ(do_bm2stream(MyBitmaskClass::BM_FOO_BAR_BAZ, EOFFS_PFX ), "FOO_BAR_BAZ");
}

TEST_CASE("bm2stream.simple_bitmask_without_null_symbol")
{
    using namespace c4;

    CHECK_EQ(do_bm2stream(BM_KABOOM, EOFFS_NONE), "KABOOM");
    CHECK_EQ(do_bm2stream<BmWithoutNull>((BmWithoutNull)0, EOFFS_NONE), "0");
}

TEST_CASE("bm2stream.bitmask_class_without_null_symbol")
{
    using namespace c4;

    CHECK_EQ(do_bm2stream(BmClassWithoutNull::BM_KABOOM, EOFFS_PFX), "KABOOM");
    CHECK_EQ(do_bm2stream<BmClassWithoutNull>((BmClassWithoutNull)0, EOFFS_NONE), "0");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// TODO
template<typename Enum>
void test_bm2str()
{
    using namespace c4;
    using I = typename std::underlying_type<Enum>::type;
    int combination_depth = 4;
    auto syms = esyms<Enum>();

    std::vector<int> indices;
    std::string str;
    std::vector<char> ws;
    I val = 0, res;
    size_t len;

    for(int k = 1; k <= combination_depth; ++k)
    {
        indices.clear();
        indices.resize(static_cast<size_t>(k));
        while(1)
        {
            str.clear();
            val = 0;
            for(auto i : indices)
            {
                if(!str.empty()) str += '|';
                str += syms[i].name;
                val |= static_cast<I>(syms[i].value);
                //printf("%d", i);
            }
            //len = bm2str<Enum>(val); // needed length
            //ws.resize(len);
            //bm2str<Enum>(val, &ws[0], len);
            //printf(": %s (%zu) %s\n", str.c_str(), (uint64_t)val, ws.data());

            res = str2bm<Enum>(str.data());
            CHECK_EQ(res, val);

            len = bm2str<Enum>(res); // needed length
            ws.resize(len);
            bm2str<Enum>(val, &ws[0], len);
            res = str2bm<Enum>(ws.data());
            CHECK_EQ(res, val);

            // write a string with the bitmask as an int
            c4::catrs(&ws, val);
            res = str2bm<Enum>(str.data());
            CHECK_EQ(res, val);

            bool carry = true;
            for(size_t i = static_cast<size_t>(k-1); i != size_t(-1); --i)
            {
                if(indices[i] + 1 < syms.size())
                {
                    ++indices[i];
                    carry = false;
                    break;
                }
                else
                {
                    indices[i] = 0;
                }
            }
            if(carry)
            {
                break;
            }
        } // while(1)
    } // for k
}
