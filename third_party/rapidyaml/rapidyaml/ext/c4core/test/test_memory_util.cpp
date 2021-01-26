#include "c4/memory_util.hpp"

#include "c4/libtest/supprwarn_push.hpp"

#include <c4/test.hpp>

namespace c4 {

TEST_CASE("mem_repeatT.one_repetition")
{
    char buf[120] = {0};

    mem_repeat(buf, "123", 2, 1);
    CHECK_EQ(strcmp(buf, "12"), 0);

    mem_repeat(buf, "123", 3, 1);
    CHECK_EQ(strcmp(buf, "123"), 0);
}

TEST_CASE("mem_repeatT.basic")
{
    char buf[120] = {0};

    mem_zero(buf);

    mem_repeat(buf, "123", 2, 2);
    CHECK_EQ(strcmp(buf, "1212"), 0);
    CHECK_EQ(buf[4], '\0');

    mem_zero(buf);

    mem_repeat(buf, "123", 3, 2);
    CHECK_EQ(strcmp(buf, "123123"), 0);
    CHECK_EQ(buf[6], '\0');

    mem_zero(buf);

    mem_repeat(buf, "123", 2, 3);
    CHECK_EQ(strcmp(buf, "121212"), 0);
    CHECK_EQ(buf[6], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 3, 3);
    CHECK_EQ(strcmp(buf, "123123123"), 0);
    CHECK_EQ(buf[9], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 2, 4);
    CHECK_EQ(strcmp(buf, "12121212"), 0);
    CHECK_EQ(buf[8], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 3, 4);
    CHECK_EQ(strcmp(buf, "123123123123"), 0);
    CHECK_EQ(buf[12], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 2, 5);
    CHECK_EQ(strcmp(buf, "1212121212"), 0);
    CHECK_EQ(buf[10], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 3, 5);
    CHECK_EQ(strcmp(buf, "123123123123123"), 0);
    CHECK_EQ(buf[15], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 2, 6);
    CHECK_EQ(strcmp(buf, "121212121212"), 0);
    CHECK_EQ(buf[12], '\0');

    mem_zero(buf, sizeof(buf));

    mem_repeat(buf, "123", 3, 6);
    CHECK_EQ(strcmp(buf, "123123123123123123"), 0);
    CHECK_EQ(buf[18], '\0');
}


//-----------------------------------------------------------------------------

TEST_CASE("is_aligned.basic")
{
    CHECK(is_aligned<int>((int*)0x0));
    CHECK_FALSE(is_aligned<int>((int*)0x1));
    CHECK_FALSE(is_aligned<int>((int*)0x2));
    CHECK_FALSE(is_aligned<int>((int*)0x3));
    CHECK_FALSE(is_aligned<int>((int*)0x3));
    CHECK(is_aligned<int>((int*)0x4));
}


//-----------------------------------------------------------------------------

TEST_CASE("lsb.basic")
{
    CHECK_EQ(lsb( 0), 0);
    CHECK_EQ(lsb( 1), 0);
    CHECK_EQ(lsb( 2), 1);
    CHECK_EQ(lsb( 3), 0);
    CHECK_EQ(lsb( 4), 2);
    CHECK_EQ(lsb( 5), 0);
    CHECK_EQ(lsb( 6), 1);
    CHECK_EQ(lsb( 7), 0);
    CHECK_EQ(lsb( 8), 3);
    CHECK_EQ(lsb( 9), 0);
    CHECK_EQ(lsb(10), 1);
    CHECK_EQ(lsb(11), 0);
    CHECK_EQ(lsb(12), 2);
    CHECK_EQ(lsb(13), 0);
    CHECK_EQ(lsb(14), 1);
    CHECK_EQ(lsb(15), 0);
    CHECK_EQ(lsb(16), 4);
}

TEST_CASE("lsb11.basic")
{
    //CHECK_EQ((lsb11<int, 0>::value), 0);
    CHECK_EQ((lsb11<int, 1>::value), 0);
    CHECK_EQ((lsb11<int, 2>::value), 1);
    CHECK_EQ((lsb11<int, 3>::value), 0);
    CHECK_EQ((lsb11<int, 4>::value), 2);
    CHECK_EQ((lsb11<int, 5>::value), 0);
    CHECK_EQ((lsb11<int, 6>::value), 1);
    CHECK_EQ((lsb11<int, 7>::value), 0);
    CHECK_EQ((lsb11<int, 8>::value), 3);
    CHECK_EQ((lsb11<int, 9>::value), 0);
    CHECK_EQ((lsb11<int,10>::value), 1);
    CHECK_EQ((lsb11<int,11>::value), 0);
    CHECK_EQ((lsb11<int,12>::value), 2);
    CHECK_EQ((lsb11<int,13>::value), 0);
    CHECK_EQ((lsb11<int,14>::value), 1);
    CHECK_EQ((lsb11<int,15>::value), 0);
    CHECK_EQ((lsb11<int,16>::value), 4);
}


//-----------------------------------------------------------------------------

TEST_CASE("msb.basic")
{
    CHECK_EQ(msb( 0), -1);
    CHECK_EQ(msb( 1),  0);
    CHECK_EQ(msb( 2),  1);
    CHECK_EQ(msb( 3),  1);
    CHECK_EQ(msb( 4),  2);
    CHECK_EQ(msb( 5),  2);
    CHECK_EQ(msb( 6),  2);
    CHECK_EQ(msb( 7),  2);
    CHECK_EQ(msb( 8),  3);
    CHECK_EQ(msb( 9),  3);
    CHECK_EQ(msb(10),  3);
    CHECK_EQ(msb(11),  3);
    CHECK_EQ(msb(12),  3);
    CHECK_EQ(msb(13),  3);
    CHECK_EQ(msb(14),  3);
    CHECK_EQ(msb(15),  3);
    CHECK_EQ(msb(16),  4);
}

TEST_CASE("msb11.basic")
{
    CHECK_EQ((msb11<int, 0>::value), -1);
    CHECK_EQ((msb11<int, 1>::value),  0);
    CHECK_EQ((msb11<int, 2>::value),  1);
    CHECK_EQ((msb11<int, 3>::value),  1);
    CHECK_EQ((msb11<int, 4>::value),  2);
    CHECK_EQ((msb11<int, 5>::value),  2);
    CHECK_EQ((msb11<int, 6>::value),  2);
    CHECK_EQ((msb11<int, 7>::value),  2);
    CHECK_EQ((msb11<int, 8>::value),  3);
    CHECK_EQ((msb11<int, 9>::value),  3);
    CHECK_EQ((msb11<int,10>::value),  3);
    CHECK_EQ((msb11<int,11>::value),  3);
    CHECK_EQ((msb11<int,12>::value),  3);
    CHECK_EQ((msb11<int,13>::value),  3);
    CHECK_EQ((msb11<int,14>::value),  3);
    CHECK_EQ((msb11<int,15>::value),  3);
    CHECK_EQ((msb11<int,16>::value),  4);
}


//-----------------------------------------------------------------------------
// tight pair

TEST_CASE("contiguous_mask.basic")
{
    CHECK_EQ(contiguous_mask(0, 0), 0);
    CHECK_EQ(contiguous_mask(0, 1), 1);
    CHECK_EQ(contiguous_mask(0, 2), 3);
    CHECK_EQ(contiguous_mask(0, 3), 7);
    CHECK_EQ(contiguous_mask(0, 4), 15);
    CHECK_EQ(contiguous_mask(1, 4), 14);
    CHECK_EQ(contiguous_mask(2, 4), 12);
    CHECK_EQ(contiguous_mask(3, 4), 8);
    CHECK_EQ(contiguous_mask(4, 4), 0);
}

TEST_CASE("contiguous_mask11.basic")
{
    CHECK_EQ((contiguous_mask11<int, 0, 0>::value), 0);
    CHECK_EQ((contiguous_mask11<int, 0, 1>::value), 1);
    CHECK_EQ((contiguous_mask11<int, 0, 2>::value), 3);
    CHECK_EQ((contiguous_mask11<int, 0, 3>::value), 7);
    CHECK_EQ((contiguous_mask11<int, 0, 4>::value), 15);
    CHECK_EQ((contiguous_mask11<int, 1, 4>::value), 14);
    CHECK_EQ((contiguous_mask11<int, 2, 4>::value), 12);
    CHECK_EQ((contiguous_mask11<int, 3, 4>::value), 8);
    CHECK_EQ((contiguous_mask11<int, 4, 4>::value), 0);
}


//-----------------------------------------------------------------------------


template<size_t N> struct sz    { char buf[N]; };
template<        > struct sz<0> {              };
template<size_t F, size_t S> void check_tp()
{
    #if defined(__clang__)
    #   pragma clang diagnostic push
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic push
    #   if __GNUC__ >= 7
    #       pragma GCC diagnostic ignored "-Wduplicated-branches"
    #   endif
    #endif
    size_t expected;
    if(F != 0 && S != 0) expected = F+S;
    else if(F == 0 && S != 0) expected = S;
    else if(F != 0 && S == 0) expected = F;   // -Wduplicated-branches: false positive here
    else /* F == 0 && S == 0)*/expected = 1;
    #if defined(__clang__)
    #   pragma clang diagnostic pop
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic pop
    #endif
    INFO("F=" << F << "  S=" << S);
    CHECK_EQ(sizeof(tight_pair<sz<F>, sz<S>>), expected);
}


TEST_CASE("tight_pair.basic")
{
    check_tp<0,0>();
    check_tp<0,1>();
    check_tp<0,2>();
    check_tp<0,3>();
    check_tp<0,4>();

    check_tp<0,0>();
    check_tp<1,0>();
    check_tp<2,0>();
    check_tp<3,0>();
    check_tp<4,0>();

    check_tp<0,0>();
    check_tp<1,1>();
    check_tp<2,2>();
    check_tp<3,3>();
    check_tp<4,4>();
}

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
