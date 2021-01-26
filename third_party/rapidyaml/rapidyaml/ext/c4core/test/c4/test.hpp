#ifndef _C4_TEST_HPP_
#define _C4_TEST_HPP_

#include "c4/config.hpp"
#include "c4/memory_resource.hpp"
#include "c4/allocator.hpp"
#include "c4/substr.hpp"
#include <cstdio>
#include <iostream>

// FIXME - these are just dumb placeholders
#define C4_LOGF_ERR(...) fprintf(stderr, __VA_ARGS__)
#define C4_LOGF_WARN(...) fprintf(stderr, __VA_ARGS__)
#define C4_LOGP(msg, ...) printf(msg)

#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include <doctest/doctest.h>

#define CHECK_STREQ(lhs, rhs) CHECK_EQ(c4::to_csubstr(lhs), c4::to_csubstr(rhs))
#define CHECK_FLOAT_EQ(lhs, rhs) CHECK((double)(lhs) == doctest::Approx((double)(rhs)))


namespace c4 {

template<class C>
inline std::ostream& operator<< (std::ostream& stream, c4::basic_substring<C> s)
{
    stream.write(s.str, std::streamsize(s.len));
    return stream;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** RAII class that tests whether an error occurs inside a scope. */
struct TestErrorOccurs
{
    TestErrorOccurs(size_t num_expected_errors = 1)
      :
      expected_errors(num_expected_errors),
      tmp_settings(c4::ON_ERROR_CALLBACK, &TestErrorOccurs::error_callback)
    {
        num_errors = 0;
    }
    ~TestErrorOccurs()
    {
        CHECK_EQ(num_errors, expected_errors);
        num_errors = 0;
    }

    size_t expected_errors;
    static size_t num_errors;
    ScopedErrorSettings tmp_settings;
    static void error_callback(const char* /*msg*/, size_t /*msg_size*/)
    {
        ++num_errors;
    }
};

#define C4_EXPECT_ERROR_OCCURS(...) \
  auto _testerroroccurs##__LINE__ = TestErrorOccurs(__VA_ARGS__)

#if C4_USE_ASSERT
#   define C4_EXPECT_ASSERT_TRIGGERS(...) C4_EXPECT_ERROR_OCCURS(__VA_ARGS__)
#else
#   define C4_EXPECT_ASSERT_TRIGGERS(...)
#endif

#if C4_USE_XASSERT
#   define C4_EXPECT_XASSERT_TRIGGERS(...) C4_EXPECT_ERROR_OCCURS(__VA_ARGS__)
#else
#   define C4_EXPECT_XASSERT_TRIGGERS(...)
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** count constructors, destructors and assigns */
template< class T >
struct Counting
{
    using value_type = T;

    T obj;

    bool operator== (T const& that) const { return obj == that; }

    static bool log_ctors;
    static size_t num_ctors;
    template< class ...Args >
    Counting(Args && ...args);

    static bool log_dtors;
    static size_t num_dtors;
    ~Counting();

    static bool log_copy_ctors;
    static size_t num_copy_ctors;
    Counting(Counting const& that);

    static bool log_move_ctors;
    static size_t num_move_ctors;
    Counting(Counting && that);

    static bool log_copy_assigns;
    static size_t num_copy_assigns;
    Counting& operator= (Counting const& that);

    static bool log_move_assigns;
    static size_t num_move_assigns;
    Counting& operator= (Counting && that);


    struct check_num
    {
        char   const* name;
        size_t const& what;
        size_t const  initial;
        size_t const  must_be_num;
        check_num(char const* nm, size_t const& w, size_t n) : name(nm), what(w), initial(what), must_be_num(n) {}
        ~check_num()
        {
            size_t del = what - initial;
            INFO("# of " << name << " calls: expected " << must_be_num << ", but got " << del);
            CHECK_EQ(del, must_be_num);
        }
    };

    static check_num check_ctors(size_t n) { return check_num("ctor", num_ctors, n); }
    static check_num check_dtors(size_t n) { return check_num("dtor", num_dtors, n); }
    static check_num check_copy_ctors(size_t n) { return check_num("copy_ctor", num_copy_ctors, n); }
    static check_num check_move_ctors(size_t n) { return check_num("move_ctor", num_move_ctors, n); }
    static check_num check_copy_assigns(size_t n) { return check_num("copy_assign", num_copy_assigns, n); }
    static check_num check_move_assigns(size_t n) { return check_num("move_assign", num_move_assigns, n); }

    struct check_num_ctors_dtors
    {
        check_num ctors, dtors;
        check_num_ctors_dtors(size_t _ctors, size_t _dtors)
        :
            ctors(check_ctors(_ctors)),
            dtors(check_dtors(_dtors))
        {
        }
    };
    static check_num_ctors_dtors check_ctors_dtors(size_t _ctors, size_t _dtors)
    {
        return check_num_ctors_dtors(_ctors, _dtors);
    }

    struct check_num_all
    {
        check_num ctors, dtors, cp_ctors, mv_ctors, cp_assigns, mv_assigns;
        check_num_all(size_t _ctors, size_t _dtors, size_t _cp_ctors, size_t _mv_ctors, size_t _cp_assigns, size_t _mv_assigns)
        {
            ctors = check_ctors(_ctors);
            dtors = check_dtors(_dtors);
            cp_ctors = check_copy_ctors(_cp_ctors);
            mv_ctors = check_move_ctors(_mv_ctors);
            cp_assigns = check_copy_assigns(_cp_assigns);
            mv_assigns = check_move_assigns(_mv_assigns);
        }
    };
    static check_num_all check_all(size_t _ctors, size_t _dtors, size_t _cp_ctors, size_t _move_ctors, size_t _cp_assigns, size_t _mv_assigns)
    {
        return check_num_all(_ctors, _dtors, _cp_ctors, _move_ctors, _cp_assigns, _mv_assigns);
    }

    static void reset()
    {
        num_ctors = 0;
        num_dtors = 0;
        num_copy_ctors = 0;
        num_move_ctors = 0;
        num_copy_assigns = 0;
        num_move_assigns = 0;
    }
};

template< class T > size_t Counting< T >::num_ctors = 0;
template< class T > bool   Counting< T >::log_ctors = false;
template< class T > size_t Counting< T >::num_dtors = 0;
template< class T > bool   Counting< T >::log_dtors = false;
template< class T > size_t Counting< T >::num_copy_ctors = 0;
template< class T > bool   Counting< T >::log_copy_ctors = false;
template< class T > size_t Counting< T >::num_move_ctors = 0;
template< class T > bool   Counting< T >::log_move_ctors = false;
template< class T > size_t Counting< T >::num_copy_assigns = 0;
template< class T > bool   Counting< T >::log_copy_assigns = false;
template< class T > size_t Counting< T >::num_move_assigns = 0;
template< class T > bool   Counting< T >::log_move_assigns = false;

template< class T >
template< class ...Args >
Counting< T >::Counting(Args && ...args) : obj(std::forward< Args >(args)...)
{
    if(log_ctors) C4_LOGP("Counting[{}]::ctor #{}\n", (void*)this, num_ctors);
    ++num_ctors;
}

template< class T >
Counting< T >::~Counting()
{
    if(log_dtors) C4_LOGP("Counting[{}]::dtor #{}\n", (void*)this, num_dtors);
    ++num_dtors;
}

template< class T >
Counting< T >::Counting(Counting const& that) : obj(that.obj)
{
    if(log_copy_ctors) C4_LOGP("Counting[{}]::copy_ctor #{}\n", (void*)this, num_copy_ctors);
    ++num_copy_ctors;
}

template< class T >
Counting< T >::Counting(Counting && that) : obj(std::move(that.obj))
{
    if(log_move_ctors) C4_LOGP("Counting[{}]::move_ctor #{}\n", (void*)this, num_move_ctors);
    ++num_move_ctors;
}

template< class T >
Counting< T >& Counting< T >::operator= (Counting const& that)
{
    obj = that.obj;
    if(log_copy_assigns) C4_LOGP("Counting[{}]::copy_assign #{}\n", (void*)this, num_copy_assigns);
    ++num_copy_assigns;
    return *this;
}

template< class T >
Counting< T >& Counting< T >::operator= (Counting && that)
{
    obj = std::move(that.obj);
    if(log_move_assigns) C4_LOGP("Counting[{}]::move_assign #{}\n", (void*)this, num_move_assigns);
    ++num_move_assigns;
    return *this;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** @todo refactor to use RAII @see Counting */
struct AllocationCountsChecker : public ScopedMemoryResourceCounts
{
    AllocationCounts first;

public:

    AllocationCountsChecker()
    :
        ScopedMemoryResourceCounts(),
        first(mr.counts())
    {
    }

    AllocationCountsChecker(MemoryResource *mr_)
    :
        ScopedMemoryResourceCounts(mr_),
        first(mr.counts())
    {
    }

    void reset()
    {
        first = mr.counts();
    }

    /** check value of curr allocations and size */
    void check_curr(ssize_t expected_allocs, ssize_t expected_size) const
    {
        CHECK_EQ(mr.counts().curr.allocs, expected_allocs);
        CHECK_EQ(mr.counts().curr.size, expected_size);
    }
    /** check delta of curr allocations and size since construction or last reset() */
    void check_curr_delta(ssize_t expected_allocs, ssize_t expected_size) const
    {
        AllocationCounts delta = mr.counts() - first;
        CHECK_EQ(delta.curr.allocs, expected_allocs);
        CHECK_EQ(delta.curr.size, expected_size);
    }

    /** check value of total allocations and size */
    void check_total(ssize_t expected_allocs, ssize_t expected_size) const
    {
        CHECK_EQ(mr.counts().total.allocs, expected_allocs);
        CHECK_EQ(mr.counts().total.size, expected_size);
    }
    /** check delta of total allocations and size since construction or last reset() */
    void check_total_delta(ssize_t expected_allocs, ssize_t expected_size) const
    {
        AllocationCounts delta = mr.counts() - first;
        CHECK_EQ(delta.total.allocs, expected_allocs);
        CHECK_EQ(delta.total.size, expected_size);
    }

    /** check value of max allocations and size */
    void check_max(ssize_t expected_max_allocs, ssize_t expected_max_size) const
    {
        CHECK_EQ(mr.counts().max.allocs, expected_max_allocs);
        CHECK_EQ(mr.counts().max.size, expected_max_size);
    }

    /** check that since construction or the last reset():
     *    - num_allocs occcurred
     *    - totaling total_size
     *    - of which the largest is max_size */
    void check_all_delta(ssize_t num_allocs, ssize_t total_size, ssize_t max_size) const
    {
        check_curr_delta(num_allocs, total_size);
        check_total_delta(num_allocs, total_size);
        check_max(num_allocs > mr.counts().max.allocs ? num_allocs : mr.counts().max.allocs,
                  max_size   > mr.counts().max.size   ? max_size   : mr.counts().max.size);
    }
};

} // namespace c4

#endif // _C4_LIBTEST_TEST_HPP_
