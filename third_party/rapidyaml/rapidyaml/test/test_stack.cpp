#include "c4/yml/detail/stack.hpp"
#include <gtest/gtest.h>

//-------------------------------------------

namespace c4 {
namespace yml {

namespace detail {

template<size_t N>
using istack = stack<int, N>;
using ip = int const*;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<size_t N>
void test_stack_small_vs_large()
{
    istack<N> s;
    for(size_t i = 0; i < N; ++i)
    {
        s.push(static_cast<int>(i));
        EXPECT_EQ(s.size(), i+1);
    }
    EXPECT_EQ(s.size(), N);
    EXPECT_EQ(s.m_stack, s.m_buf);
    for(size_t i = 0; i < N; ++i)
    {
        EXPECT_EQ(s.top(N-1-i), static_cast<int>(i));
    }
    s.push(N);
    EXPECT_NE(s.m_stack, s.m_buf);
    EXPECT_EQ(s.top(), static_cast<int>(N));
    EXPECT_EQ(s.pop(), static_cast<int>(N));
    EXPECT_NE(s.m_stack, s.m_buf);
    for(size_t i = 0; i < N; ++i)
    {
        EXPECT_EQ(s.top(N-1-i), static_cast<int>(i));
    }
}

TEST(stack, small_vs_large)
{
    test_stack_small_vs_large<8>();
    test_stack_small_vs_large<16>();
    test_stack_small_vs_large<32>();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<size_t N>
void test_copy_ctor()
{
    istack<N> src;

    // small
    for(size_t i = 0; i < N; ++i)
    {
        src.push((int)i);
    }
    EXPECT_EQ(src.m_stack, src.m_buf);
    ip b = src.begin();
    {
        istack<N> dst(src);
        EXPECT_EQ(dst.size(), src.size());
        EXPECT_EQ(dst.m_stack, dst.m_buf);
        EXPECT_EQ((ip)src.begin(), b);
        EXPECT_NE((ip)dst.begin(), (ip)src.begin());
    }

    // large
    for(size_t i = 0; i < 2*N; ++i)
    {
        src.push((int)i); // large
    }
    EXPECT_NE(src.m_stack, src.m_buf);
    b = src.begin();
    {
        istack<N> dst(src);
        EXPECT_EQ(dst.size(), src.size());
        EXPECT_NE(dst.m_stack, dst.m_buf);
        EXPECT_EQ((ip)src.begin(), b);
        EXPECT_NE((ip)dst.begin(), (ip)src.begin());
    }
}

TEST(stack, copy_ctor)
{
    test_copy_ctor<4>();
    test_copy_ctor<8>();
    test_copy_ctor<64>();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<size_t N>
void test_move_ctor()
{
    istack<N> src;

    // small
    for(size_t i = 0; i < N; ++i)
    {
        src.push((int)i);
    }
    EXPECT_EQ(src.m_stack, src.m_buf);
    ip b = src.begin();
    size_t sz = src.size();
    {
        istack<N> dst(std::move(src));
        EXPECT_EQ(dst.size(), sz);
        EXPECT_EQ(dst.m_stack, dst.m_buf);
        EXPECT_NE(dst.m_stack, b);
        EXPECT_EQ(src.size(), size_t(0));
        EXPECT_EQ((ip)src.begin(), src.m_buf);
        EXPECT_NE((ip)dst.begin(), b);
    }
    EXPECT_EQ(src.size(), size_t(0));
    EXPECT_EQ(src.capacity(), N);
    EXPECT_EQ(src.m_stack, src.m_buf);

    // redo
    for(size_t i = 0; i < N; ++i)
    {
        src.push((int)i);
    }
    EXPECT_EQ(src.size(), N);
    EXPECT_EQ(src.capacity(), N);
    EXPECT_EQ(src.m_stack, src.m_buf);
    // large
    for(size_t i = 0; i < 2*N; ++i)
    {
        src.push((int)i); // large
    }
    EXPECT_EQ(src.size(), 3*N);
    EXPECT_NE(src.m_stack, src.m_buf);
    b = src.begin();
    sz = src.size();
    {
        istack<N> dst(std::move(src));
        EXPECT_EQ(dst.size(), sz);
        EXPECT_NE(dst.m_stack, dst.m_buf);
        EXPECT_EQ(dst.m_stack, b);
        EXPECT_EQ(src.capacity(), N);
        EXPECT_EQ(src.size(), size_t(0));
        EXPECT_EQ((ip)src.begin(), src.m_buf);
        EXPECT_EQ((ip)dst.begin(), b);
    }
}

TEST(stack, move_ctor)
{
    test_move_ctor<4>();
    test_move_ctor<8>();
    test_move_ctor<64>();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<size_t N>
void test_copy_assign()
{
    istack<N> srcs;
    istack<N> srcl;
    istack<N> dst;

    for(size_t i = 0; i < N; ++i)
    {
        srcs.push((int)i); // small
        srcl.push((int)i); // large
    }
    for(size_t i = 0; i < 2*N; ++i)
    {
        srcl.push((int)i); // large
    }
    EXPECT_EQ(srcs.m_stack, srcs.m_buf);
    EXPECT_NE(srcl.m_stack, srcl.m_buf);

    ip bs = srcs.begin(), bl = srcl.begin();

    {
        dst = srcs;
        EXPECT_EQ(dst.size(), srcs.size());
        EXPECT_EQ(dst.m_stack, dst.m_buf);
        EXPECT_EQ((ip)srcs.begin(), bs);
        EXPECT_NE((ip)dst.begin(), (ip)srcs.begin());
    }

    {
        dst = srcl;
        EXPECT_EQ(dst.size(), srcl.size());
        EXPECT_NE(dst.m_stack, dst.m_buf);
        EXPECT_EQ((ip)srcl.begin(), bl);
        EXPECT_NE((ip)dst.begin(), (ip)srcl.begin());
    }

    {
        dst = srcs;
        EXPECT_EQ(dst.size(), srcs.size());
        EXPECT_NE(dst.m_stack, dst.m_buf); // it stays in long mode (it's no trimmed when assigned from a short-mode stack)
        EXPECT_EQ((ip)srcs.begin(), bs);
        EXPECT_NE((ip)dst.begin(), (ip)srcs.begin());
    }
}

TEST(stack, copy_assign)
{
    test_copy_assign<4>();
    test_copy_assign<8>();
    test_copy_assign<64>();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<size_t N>
void test_move_assign()
{
    istack<N> srcs, srcl, dst;

    for(size_t i = 0; i < N; ++i)
    {
        srcs.push((int)i); // small
        srcl.push((int)i); // large
    }
    for(size_t i = 0; i < 2*N; ++i)
    {
        srcl.push((int)i); // large
    }
    EXPECT_EQ(srcs.m_stack, srcs.m_buf);
    EXPECT_NE(srcl.m_stack, srcl.m_buf);

    ip bs = srcs.begin()/*, bl = srcl.begin()*/;
    size_t szs = srcs.size(), szl = srcl.size();

    for(int i = 0; i < 10; ++i)
    {
        EXPECT_FALSE(srcs.empty());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.m_stack, dst.m_buf);
        EXPECT_EQ(srcs.m_stack, srcs.m_buf);

        dst = std::move(srcs);
        EXPECT_TRUE(srcs.empty());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(srcs.size(), size_t(0));
        EXPECT_EQ(srcs.capacity(), N);
        EXPECT_EQ(dst.size(), szs);
        EXPECT_EQ(dst.m_stack, dst.m_buf);
        EXPECT_EQ(srcs.m_stack, srcs.m_buf);
        EXPECT_EQ((ip)srcs.begin(), bs);
        EXPECT_NE((ip)dst.begin(), (ip)srcs.begin());

        srcs = std::move(dst);
    }

    for(int i = 0; i < 10; ++i)
    {
        EXPECT_EQ(srcl.size(), 3*N);
        EXPECT_FALSE(srcl.empty());
        EXPECT_TRUE(dst.empty());
        EXPECT_EQ(dst.m_stack, dst.m_buf);
        EXPECT_NE(srcl.m_stack, srcl.m_buf);

        dst = std::move(srcl);
        EXPECT_TRUE(srcl.empty());
        EXPECT_FALSE(dst.empty());
        EXPECT_EQ(srcl.size(), size_t(0));
        EXPECT_EQ(srcl.capacity(), N);
        EXPECT_EQ(dst.size(), szl);
        EXPECT_NE(dst.m_stack, dst.m_buf);
        EXPECT_EQ(srcl.m_stack, srcl.m_buf);
        EXPECT_EQ((ip)srcl.begin(), srcl.m_buf);
        EXPECT_NE((ip)dst.begin(), (ip)srcl.begin());

        srcl = std::move(dst);
    }
}

TEST(stack, move_assign)
{
    test_move_assign<4>();
    test_move_assign<8>();
    test_move_assign<64>();
}

} // namespace detail
} // namespace yml
} // namespace c4


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// this is needed to use the test case library

#include "c4/substr.hpp"
#include "c4/yml/common.hpp"

namespace c4 {
namespace yml {
struct Case;
Case const* get_case(csubstr /*name*/)
{
    return nullptr;
}

} // namespace yml
} // namespace c4
