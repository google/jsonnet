#include "c4/szconv.hpp"

#include "c4/libtest/supprwarn_push.hpp"
#include "c4/test.hpp"

#define C4_EXPECT_NARROWER(yes_or_no, ty_out, ty_in) \
    CHECK_UNARY(( yes_or_no (is_narrower_size<ty_out C4_COMMA ty_in>::value)));

namespace c4 {

TEST_CASE("is_narrower_size.signed_types")
{
    C4_EXPECT_NARROWER( ! ,  int8_t ,  int8_t );
    C4_EXPECT_NARROWER(   ,  int8_t ,  int16_t);
    C4_EXPECT_NARROWER(   ,  int8_t ,  int32_t);
    C4_EXPECT_NARROWER(   ,  int8_t ,  int64_t);
    C4_EXPECT_NARROWER(   ,  int8_t , uint8_t );
    C4_EXPECT_NARROWER(   ,  int8_t , uint16_t);
    C4_EXPECT_NARROWER(   ,  int8_t , uint32_t);
    C4_EXPECT_NARROWER(   ,  int8_t , uint64_t);
    C4_EXPECT_NARROWER( ! , int16_t ,  int8_t );
    C4_EXPECT_NARROWER( ! , int16_t ,  int16_t);
    C4_EXPECT_NARROWER(   , int16_t ,  int32_t);
    C4_EXPECT_NARROWER(   , int16_t ,  int64_t);
    C4_EXPECT_NARROWER( ! , int16_t , uint8_t );
    C4_EXPECT_NARROWER(   , int16_t , uint16_t);
    C4_EXPECT_NARROWER(   , int16_t , uint32_t);
    C4_EXPECT_NARROWER(   , int16_t , uint64_t);
    C4_EXPECT_NARROWER( ! , int32_t ,  int8_t );
    C4_EXPECT_NARROWER( ! , int32_t ,  int16_t);
    C4_EXPECT_NARROWER( ! , int32_t ,  int32_t);
    C4_EXPECT_NARROWER(   , int32_t ,  int64_t);
    C4_EXPECT_NARROWER( ! , int32_t , uint8_t );
    C4_EXPECT_NARROWER( ! , int32_t , uint16_t);
    C4_EXPECT_NARROWER(   , int32_t , uint32_t);
    C4_EXPECT_NARROWER(   , int32_t , uint64_t);
    C4_EXPECT_NARROWER( ! , int64_t ,  int8_t );
    C4_EXPECT_NARROWER( ! , int64_t ,  int16_t);
    C4_EXPECT_NARROWER( ! , int64_t ,  int32_t);
    C4_EXPECT_NARROWER( ! , int64_t ,  int64_t);
    C4_EXPECT_NARROWER( ! , int64_t , uint8_t );
    C4_EXPECT_NARROWER( ! , int64_t , uint16_t);
    C4_EXPECT_NARROWER( ! , int64_t , uint32_t);
    C4_EXPECT_NARROWER(   , int64_t , uint64_t);
}

TEST_CASE("is_narrower_size.unsigned_types")
{
    C4_EXPECT_NARROWER( ! ,  uint8_t ,  int8_t );
    C4_EXPECT_NARROWER(   ,  uint8_t ,  int16_t);
    C4_EXPECT_NARROWER(   ,  uint8_t ,  int32_t);
    C4_EXPECT_NARROWER(   ,  uint8_t ,  int64_t);
    C4_EXPECT_NARROWER( ! ,  uint8_t , uint8_t );
    C4_EXPECT_NARROWER(   ,  uint8_t , uint16_t);
    C4_EXPECT_NARROWER(   ,  uint8_t , uint32_t);
    C4_EXPECT_NARROWER(   ,  uint8_t , uint64_t);
    C4_EXPECT_NARROWER( ! , uint16_t ,  int8_t );
    C4_EXPECT_NARROWER( ! , uint16_t ,  int16_t);
    C4_EXPECT_NARROWER(   , uint16_t ,  int32_t);
    C4_EXPECT_NARROWER(   , uint16_t ,  int64_t);
    C4_EXPECT_NARROWER( ! , uint16_t , uint8_t );
    C4_EXPECT_NARROWER( ! , uint16_t , uint16_t);
    C4_EXPECT_NARROWER(   , uint16_t , uint32_t);
    C4_EXPECT_NARROWER(   , uint16_t , uint64_t);
    C4_EXPECT_NARROWER( ! , uint32_t ,  int8_t );
    C4_EXPECT_NARROWER( ! , uint32_t ,  int16_t);
    C4_EXPECT_NARROWER( ! , uint32_t ,  int32_t);
    C4_EXPECT_NARROWER(   , uint32_t ,  int64_t);
    C4_EXPECT_NARROWER( ! , uint32_t , uint8_t );
    C4_EXPECT_NARROWER( ! , uint32_t , uint16_t);
    C4_EXPECT_NARROWER( ! , uint32_t , uint32_t);
    C4_EXPECT_NARROWER(   , uint32_t , uint64_t);
    C4_EXPECT_NARROWER( ! , uint64_t ,  int8_t );
    C4_EXPECT_NARROWER( ! , uint64_t ,  int16_t);
    C4_EXPECT_NARROWER( ! , uint64_t ,  int32_t);
    C4_EXPECT_NARROWER( ! , uint64_t ,  int64_t);
    C4_EXPECT_NARROWER( ! , uint64_t , uint8_t );
    C4_EXPECT_NARROWER( ! , uint64_t , uint16_t);
    C4_EXPECT_NARROWER( ! , uint64_t , uint32_t);
    C4_EXPECT_NARROWER( ! , uint64_t , uint64_t);
}

template<typename I, typename O>
typename std::enable_if<std::is_same<I, O>::value, void>::type
test_szconv()
{
  // nothing to do here
}

template<typename I, typename O>
typename std::enable_if< ! std::is_same<I, O>::value, void>::type
test_szconv()
{
    C4_STATIC_ASSERT(std::is_integral<I>::value);
    C4_STATIC_ASSERT(std::is_integral<O>::value);

    const I imax = std::numeric_limits<I>::max();
    const I imin = std::numeric_limits<I>::min();
    const O omax = std::numeric_limits<O>::max();
    const O omin = std::numeric_limits<O>::min();

    CHECK_EQ(szconv<O>(I(0)), O(0));
    CHECK_EQ(szconv<I>(I(0)), I(0));

#if C4_USE_XASSERT
    if((uint64_t)omax < (uint64_t)imax)
    {
        C4_EXPECT_ERROR_OCCURS();
        O out = szconv<O>(imax);
    }
    else if((uint64_t)omax > (uint64_t)imax)
    {
        C4_EXPECT_ERROR_OCCURS();
        I out = szconv<I>(omax);
    }
#endif
}

#define DO_TEST_SZCONV(ty)                      \
    TEST_CASE("szconv." #ty "_to_int8")         \
    {                                           \
        test_szconv<ty##_t, int8_t>();          \
    }                                           \
    TEST_CASE("zconv." #ty "_to_uint8")         \
    {                                           \
        test_szconv<ty##_t, uint8_t>();         \
    }                                           \
    TEST_CASE("zconv." #ty "_to_int16")         \
    {                                           \
        test_szconv<ty##_t, int16_t>();         \
    }                                           \
    TEST_CASE("zconv." #ty "_to_uint16")        \
    {                                           \
        test_szconv<ty##_t, uint16_t>();        \
    }                                           \
    TEST_CASE("zconv." #ty "_to_int32")         \
    {                                           \
        test_szconv<ty##_t, int32_t>();         \
    }                                           \
    TEST_CASE("zconv." #ty "_to_uint32")        \
    {                                           \
        test_szconv<ty##_t, uint32_t>();        \
    }                                           \
    TEST_CASE("zconv." #ty "_to_int64")         \
    {                                           \
        test_szconv<ty##_t, int64_t>();         \
    }                                           \
    TEST_CASE("szconv." #ty "_to_uint64")       \
    {                                           \
        test_szconv<ty##_t, uint64_t>();        \
    }

DO_TEST_SZCONV(int8)
DO_TEST_SZCONV(uint8)
DO_TEST_SZCONV(int16)
DO_TEST_SZCONV(uint16)
DO_TEST_SZCONV(int32)
DO_TEST_SZCONV(uint32)
DO_TEST_SZCONV(int64)
DO_TEST_SZCONV(uint64)

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
