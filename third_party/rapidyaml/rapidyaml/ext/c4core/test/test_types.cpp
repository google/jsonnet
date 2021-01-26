#include "c4/config.hpp"
#include <string>

#include "c4/libtest/supprwarn_push.hpp"
#include "c4/test.hpp"

namespace c4 {

C4_STATIC_ASSERT((std::is_same< fastcref< char >, char >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< i8   >, i8   >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< u8   >, u8   >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< i16  >, i16  >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< u16  >, u16  >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< i32  >, i32  >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< u32  >, u32  >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< i64  >, i64  >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< u64  >, u64  >::value));

using carr64 = char[64];
C4_STATIC_ASSERT((std::is_same< fastcref< carr64      >, carr64 const& >::value));
C4_STATIC_ASSERT((std::is_same< fastcref< std::string >, std::string const& >::value));

//-----------------------------------------------------------------------------

C4_BEGIN_HIDDEN_NAMESPACE
template< class T > struct ufonix { T a; };
using F = ufonix< uint32_t >;
C4_END_HIDDEN_NAMESPACE

TEST_CASE("TestSizeStructs.min_remainder")
{
    CHECK_EQ(min_remainder(4, 6), 2);
    CHECK_EQ(min_remainder(6, 6), 0);
    CHECK_EQ(min_remainder(8, 6), 0);
}

TEST_CASE("TestSizeStructs.mult_remainder")
{
    CHECK_EQ(mult_remainder(6, 1), 0);
    CHECK_EQ(mult_remainder(6, 2), 0);
    CHECK_EQ(mult_remainder(6, 3), 0);
    CHECK_EQ(mult_remainder(6, 4), 2);
    CHECK_EQ(mult_remainder(6, 5), 4);
    CHECK_EQ(mult_remainder(6, 6), 0);
    CHECK_EQ(mult_remainder(6, 7), 1);
}
TEST_CASE("TestSizeStructs.Padded")
{
    CHECK_EQ(sizeof(F), sizeof(uint32_t));
    CHECK_EQ((sizeof(Padded< F, 0 >)), sizeof(F));
    CHECK_EQ((sizeof(Padded< F, 1 >)), sizeof(F)+1);
    CHECK_EQ((sizeof(Padded< F, 2 >)), sizeof(F)+2);
    CHECK_EQ((sizeof(Padded< F, 3 >)), sizeof(F)+3);
}
TEST_CASE("TestSizeStructs.MinSized")
{
    CHECK_EQ((sizeof(MinSized< F, 14 >)), 14);
    CHECK_EQ((sizeof(MinSized< F, 15 >)), 15);
    CHECK_EQ((sizeof(MinSized< F, 16 >)), 16);
    CHECK_EQ((sizeof(MinSized< F, 17 >)), 17);
}
TEST_CASE("TestSizeStructs.MultSized")
{
    using G = ufonix< char[8] >;
    CHECK_EQ((sizeof(MultSized< G, 7 >)), 14);
    CHECK_EQ((sizeof(MultSized< G, 6 >)), 12);
    CHECK_EQ((sizeof(MultSized< G, 5 >)), 10);
    CHECK_EQ((sizeof(MultSized< G, 4 >)), 8);
}
TEST_CASE("TestSizeStructs.UbufSized")
{
    CHECK_EQ((sizeof(UbufSized<ufonix<char[63]>>)), 64);
    CHECK_EQ((sizeof(UbufSized<ufonix<char[64]>>)), 64);
    CHECK_EQ((sizeof(UbufSized<ufonix<char[65]>>)), 80);
}

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
