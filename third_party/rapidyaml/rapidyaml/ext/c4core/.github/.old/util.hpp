#ifndef _C4_UTIL_HPP_
#define _C4_UTIL_HPP_

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// CONFIG

//#define C4_DEBUG
//#define C4_USE_XASSERT
//#define C4_NO_ALLOC_DEFAULTS
//#define C4_REDEFINE_CPPNEW
//#define C4_LOG_THREAD_SAFE
#ifndef C4_LOG_MAX_CHANNELS
#   define C4_LOG_MAX_CHANNELS 32
#endif
#ifndef C4_LOG_BUFFER_INITIAL_SIZE
#   define C4_LOG_BUFFER_INITIAL_SIZE 128
#endif
#ifndef C4_LOG_BUFFER_REF_SIZE
#   define C4_LOG_BUFFER_REF_SIZE 256
#endif

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// INCLUDES

#include <stddef.h>
#include <stdint.h> // put uint32_t et al into the :: namespace
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <stdarg.h>
#include <algorithm>
#include <assert.h>

#include <limits>

#include "c4/preprocessor.hpp"
#include "c4/platform.hpp"
#include "c4/cpu.hpp"

#include "c4/windows.hpp"

#if !defined(C4_WIN) && !defined(C4_POSIX)
#   include <chrono>
#elif defined(C4_POSIX)
#   include <time.h>
#endif

#ifdef C4_LOG_THREAD_SAFE
#   include <thread>
#   include <mutex>
#endif

#if !defined(C4_NO_ALLOC_DEFAULTS) && defined(C4_POSIX)
#   include <errno.h>
#endif

#include <memory>

#include "c4/language.hpp"


//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// error reporting

#define C4_STATIC_ASSERT(cond) static_assert(cond, "static assert failed")
#define C4_STATIC_ASSERT_MSG(cond, msg) static_assert(cond, msg)

C4_BEGIN_NAMESPACE(c4)

using error_callback_type = void (*)();

/** Defaults to abort() */
inline error_callback_type& get_error_callback()
{
    static error_callback_type cb = &::abort;
    return cb;
}
/** Set the function which is called when an error occurs. */
inline void set_error_callback(error_callback_type cb)
{
    get_error_callback() = cb;
}

void report_error(const char *file, int line, const char *func, const char *fmt, ...);
C4_END_NAMESPACE(c4)

/** Raise an error, and report a printf-formatted message.
 * If an error callback was set, it will be called.
 * @see set_error_callback() */
#define C4_ERROR(msg, ...)                                              \
    c4::report_error(__FILE__, __LINE__, C4_PRETTY_FUNC, msg, ## __VA_ARGS__)

/** Report a warning with a printf-formatted message. */
#define C4_WARNING(msg, ...)                                \
    {                                                       \
        C4_LOG_WARN("\n%s:%d: WARNING: " msg                \
                    "\n%s:%d: WARNING: %s\n",               \
                    __FILE__, __LINE__, ## __VA_ARGS__,     \
                    __FILE__, __LINE__, C4_PRETTY_FUNC);    \
    }

// error checking - always turned on
/** Check that a condition is true, or raise an error when not true. */
#define C4_CHECK(cond)                          \
    if(C4_UNLIKELY(!(cond)))                    \
    {                                           \
        C4_ERROR("check failed: " #cond);       \
    }

/** like C4_CHECK(), and additionally log a printf-style message.
 * @see C4_CHECK */
#define C4_CHECK_MSG(cond, fmt, ...)                                \
    if(C4_UNLIKELY(!(cond)))                                        \
    {                                                               \
        C4_ERROR("check failed: " #cond "\n" fmt, ## __VA_ARGS__);  \
    }

// assertions - only in debug builds
#ifdef NDEBUG // turn off assertions
#   define C4_ASSERT(cond)
#   define C4_ASSERT_MSG(cond, fmt, ...)
#else
#   define C4_ASSERT(cond) C4_CHECK(cond)
#   define C4_ASSERT_MSG(cond, fmt, ...) C4_CHECK_MSG(cond, fmt, ## __VA_ARGS__)
#endif

// Extreme assertion: can be switched off independently of the regular assertion.
// Use eg for bounds checking in hot code.
#ifdef C4_USE_XASSERT
#   define C4_XASSERT(cond) C4_CHECK(cond)
#   define C4_XASSERT_MSG(cond, fmt, ...) C4_CHECK_MSG(cond, fmt, ## __VA_ARGS__)
#else
#   define C4_XASSERT(cond)
#   define C4_XASSERT_MSG(cond, fmt, ...)
#endif

// Common error conditions
#define C4_NOT_IMPLEMENTED() C4_ERROR("NOT IMPLEMENTED")
#define C4_NOT_IMPLEMENTED_MSG(msg, ...) C4_ERROR("NOT IMPLEMENTED: " msg, ## __VA_ARGS__)

#define C4_NEVER_REACH() C4_UNREACHABLE(); C4_ERROR("never reach this point")
#define C4_NEVER_REACH_MSG(msg, ...) C4_UNREACHABLE(); C4_ERROR("never reach this point: " msg, ## __VA_ARGS__)

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// TIMING

C4_BEGIN_NAMESPACE(c4)

/** converts automatically from/to microseconds. */
class Time
{
public:

    C4_ALWAYS_INLINE Time() : m_microsecs(0) {}
    C4_ALWAYS_INLINE Time(double microsecs) : m_microsecs(microsecs) {}

    C4_ALWAYS_INLINE operator double () const { return m_microsecs; }

    C4_ALWAYS_INLINE void operator= (double t) { m_microsecs = t; }

    C4_ALWAYS_INLINE void m(double minutes) { m_microsecs = minutes * 60.e6; }
    C4_ALWAYS_INLINE double m() const { return m_microsecs / 60.e6; }

    C4_ALWAYS_INLINE void s(double seconds) { m_microsecs = seconds * 1.e6; }
    C4_ALWAYS_INLINE double s() const { return m_microsecs * 1.e-6; }

    C4_ALWAYS_INLINE void ms(double miliseconds) { m_microsecs = miliseconds * 1.e3; }
    C4_ALWAYS_INLINE double ms() const { return m_microsecs * 1.e-3; }

    C4_ALWAYS_INLINE void us(double microseconds) { m_microsecs = microseconds; }
    C4_ALWAYS_INLINE double us() const { return m_microsecs; }

    C4_ALWAYS_INLINE void ns(double nanoseconds) { m_microsecs = nanoseconds * 1.e-3; }
    C4_ALWAYS_INLINE double ns() const { return m_microsecs * 1.e3; }

private:

    double m_microsecs;

};

C4_BEGIN_NAMESPACE(time_suffixes)
Time operator"" _s(long double seconds) { Time t; t.s((double)seconds); return t; }
Time operator"" _ms(long double milliseconds) { Time t; t.ms((double)milliseconds); return t; }
Time operator"" _us(long double microseconds) { Time t; t.us((double)microseconds); return t; }
Time operator"" _ns(long double nanoseconds) { Time t; t.ns((double)nanoseconds); return t; }
C4_END_NAMESPACE(time_suffixes)

/** a general-use time stamp in microseconds (usecs).
 * Although this is timed precisely, there may be some issues.
 * Eg, concurrent or heavy use may cause penalties.
 * @see https://www.strchr.com/performance_measurements_with_rdtsc
 * @see https://msdn.microsoft.com/en-us/library/windows/desktop/ee417693(v=vs.85).aspx */
inline double currtime()
{
#ifdef C4_WIN
    static bool gotfreq = false;
    static double ifreq = 0.;
    if(C4_UNLIKELY(!gotfreq))
    {
        static LARGE_INTEGER freq = {};
        QueryPerformanceFrequency(&freq);
        ifreq = 1.e9 / double(freq.QuadPart);
        gotfreq = true;
    }
    LARGE_INTEGER ts;
    QueryPerformanceCounter(&ts);
    double usecs = ts.QuadPart * ifreq;
    return usecs;
#elif defined(C4_POSIX)
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    double usecs = 1.e6 * double(ts.tv_sec) + 1.e-3 * double(ts.tv_nsec);
    return usecs;
#else
    std::chrono::time_point< hrc_type, usec_type > tp = std::chrono::high_resolution_clock::now();
    double nsecs = tp.time_since_epoch().count();
    return 1.e-3 * nsecs;
#endif
}

/** execution time */
inline double exetime()
{
    static const double atstart = currtime();
    double now = currtime() - atstart;
    return now;
}

/** do a spin loop for at least the given time */
inline void busy_wait(double microsecs)
{
    double start = currtime();
    while(currtime() - start < microsecs)
    {
        C4_KEEP_EMPTY_LOOP;
    }
}

C4_END_NAMESPACE(c4)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Some memory utilities.

C4_BEGIN_NAMESPACE(c4)

C4_ALWAYS_INLINE void zero_mem(void* mem, size_t num_bytes)
{
    ::memset(mem, 0, num_bytes);
}
template< class T >
C4_ALWAYS_INLINE void zero_mem(T* mem, size_t num_elms)
{
    ::memset(mem, 0, sizeof(T) * num_elms);
}
template< class T >
C4_ALWAYS_INLINE void zero_mem(T* mem)
{
    ::memset(mem, 0, sizeof(T));
}


inline bool mem_overlaps(void const* a, void const* b, size_t sza, size_t szb)
{
    if(a < b)
    {
        if(size_t(a) + sza > size_t(b)) return true;
    }
    else if(a > b)
    {
        if(size_t(b) + szb > size_t(a)) return true;
    }
    else if(a == b)
    {
        if(sza != 0 && szb != 0) return true;
    }
    return false;
}

/** Fills 'dest' with the first 'pattern_size' bytes at 'pattern', 'num_times'. */
inline void mem_repeat(void* dest, void const* pattern, size_t pattern_size, size_t num_times)
{
    if(C4_UNLIKELY(num_times == 0)) return;
    C4_ASSERT(!mem_overlaps(dest, pattern, num_times*pattern_size, pattern_size));
    char *begin = (char*)dest;
    char *end   = begin + num_times * pattern_size;
    // copy the pattern once
    ::memcpy(begin, pattern, pattern_size);
    // now copy from dest to itself, doubling up every time
    size_t n = pattern_size;
    while(begin + 2*n < end)
    {
        ::memcpy(begin + n, begin, n);
        n <<= 1; // double n
    }
    // copy the missing part
    if(begin + n < end)
    {
        ::memcpy(begin + n, begin, end - (begin + n));
    }
}

C4_END_NAMESPACE(c4)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// some traits and type utility classes/macros.

C4_BEGIN_NAMESPACE(c4)

/** use this macro to enable a function overload based on a compile-time condition.
@code
// define an overload for a non-pod type
template< class T, C4_REQUIRE_R(std::is_pod< T >::value) >
void foo() { std::cout << "pod type\n"; }

// define an overload for a non-pod type
template< class T, C4_REQUIRE_R(!std::is_pod< T >::value) >
void foo() { std::cout << "nonpod type\n"; }

struct non_pod
{
    non_pod() : name("asdfkjhasdkjh") {}
    const char *name;
};

int main()
{
    foo< float >(); // prints "pod type"
    foo< non_pod >(); // prints "nonpod type"
}
@endcode */
#define C4_REQUIRE_T(cond) typename std::enable_if< cond, bool >::type* = nullptr

/** enable_if for a return type */
#define C4_REQUIRE_R(cond, type_) typename std::enable_if< cond, type_ >::type

//-----------------------------------------------------------------------------
/** declare a traits class telling whether a type provides a member typedef */
#define C4_DEFINE_HAS_TYPEDEF(member_typedef)                   \
template< typename T >                                          \
struct has_##stype                                              \
{                                                               \
private:                                                        \
                                                                \
    typedef char                      yes;                      \
    typedef struct { char array[2]; } no;                       \
                                                                \
    template< typename C >                                      \
    static yes _test(typename C::member_typedef*);              \
                                                                \
    template< typename C >                                      \
    static no  _test(...);                                      \
                                                                \
public:                                                         \
                                                                \
    enum { value = (sizeof(_test< T >(0)) == sizeof(yes)) };    \
                                                                \
}

//-----------------------------------------------------------------------------
/** declare a traits class telling whether a type provides a method */
#define C4_DEFINE_HAS_METHOD(ret_type, method_name, const_qualifier, ...) \
template< typename T >                                                  \
struct has_##method_name##_method                                       \
{                                                                       \
private:                                                                \
                                                                        \
    typedef char                      &yes;                             \
    typedef struct { char array[2]; } &no;                              \
                                                                        \
    template< typename C >                                              \
    static yes _test                                                    \
    (                                                                   \
        C const_qualifier* v,                                           \
        typename std::enable_if                                         \
        <                                                               \
            std::is_same< decltype(v->method_name(__VA_ARGS__)), ret_type >::value \
            ,                                                           \
            void /* this is defined only if the bool above is true. */  \
                 /* so when it fails, SFINAE is triggered */            \
        >                                                               \
        ::type*                                                         \
    );                                                                  \
                                                                        \
    template< typename C >                                              \
    static no _test(...);                                               \
                                                                        \
public:                                                                 \
                                                                        \
    enum { value = (sizeof(_test< T >((typename std::remove_reference< T >::type*)0, 0)) == sizeof(yes)) }; \
                                                                        \
};

//--------------------------------------------------

/** whether a value should be used in place of a const-reference in argument passing. */
template< class T >
struct cref_uses_val
{
    enum { value = (
    std::is_scalar< T >::value
    ||
    (std::is_pod< T >::value && sizeof(T) <= sizeof(size_t))) };
};
/** override the default behaviour for c4::fastcref< T >
 @see fastcref */
#define C4_CREF_USES_VAL(T) \
template<>                  \
struct cref_uses_val< T >   \
{                           \
    enum { value = true };  \
};

/** Whether to use pass-by-value or pass-by-const-reference in a function argument
 * or return type. */
template< class T >
using fastcref = typename std::conditional< c4::cref_uses_val< T >::value, T, T const& >::type;

//--------------------------------------------------

/** Just what its name says. Useful sometimes as a default empty policy class. */
struct EmptyStruct
{
    template< class... T > EmptyStruct(T && ...){}
};

/** Just what its name says. Useful sometimes as a default policy class to
 * be inherited from. */
struct EmptyStructVirtual
{
    virtual ~EmptyStructVirtual() = default;
    template< class... T > EmptyStructVirtual(T && ...){}
};


//--------------------------------------------------
// Utilities to make a class obey size restrictions (eg, min size or size multiple of).
// DirectX usually makes this restriction with uniform buffers.
// This is also useful for padding to prevent false-sharing.

/** force the following class to be tightly packed.
 * @see http://stackoverflow.com/questions/21092415/force-c-structure-to-pack-tightly */
#pragma pack(push, 1)
/** pad a class with more bytes at the end. */
template< class T, size_t BytesToPadAtEnd >
struct Padded : public T
{
    using T::T;
public:
    char ___c4padspace___[BytesToPadAtEnd];
};
#pragma pack(pop)
/** When the padding argument is 0, we cannot declare the char[] array. */
template< class T >
struct Padded< T, 0 > : public T
{
    using T::T;
};

/** how many bytes must be added to size such that the result is at least minsize? */
constexpr inline size_t min_remainder(size_t size, size_t minsize)
{
    return size < minsize ? minsize-size : 0;
}

/** how many bytes must be added to size such that the result is a multiple of multipleOf?  */
constexpr inline size_t mult_remainder(size_t size, size_t multipleof)
{
    return (((size % multipleof) != 0) ? (multipleof-(size % multipleof)) : 0);
}

/** make T have a size which is at least Min bytes */
template< class T, size_t Min >
using MinSized = Padded< T, min_remainder(sizeof(T), Min) >;

/** make T have a size which is a multiple of Mult bytes */
template< class T, size_t Mult >
using MultSized = Padded< T, mult_remainder(sizeof(T), Mult) >;

/** make T have a size which is simultaneously:
 *  -bigger or equal than Min
 *  -a multiple of Mult */
template< class T, size_t Min, size_t Mult >
using MinMultSized = MultSized< MinSized< T, Min >, Mult >;

/** make T be suitable for use as a uniform buffer. (at least with DirectX). */
template< class T >
using UbufSized = MinMultSized< T, 64, 16 >;

//-----------------------------------------------------------------------------
// construct

#define _C4REQUIRE(cond) \
C4_ALWAYS_INLINE typename std::enable_if< cond, void >::type


/** default-construct an object, trivial version */
template< class U > _C4REQUIRE(std::is_trivially_default_constructible< U >::value)
construct(U* ptr) noexcept
{
    memset(ptr, 0, sizeof(U));
}
/** default-construct an object, non-trivial version */
template< class U > _C4REQUIRE( ! std::is_trivially_default_constructible< U >::value)
construct(U* ptr) noexcept
{
    new (ptr) U();
}

/** default-construct n objects, trivial version */
template< class U, class I > _C4REQUIRE(std::is_trivially_default_constructible< U >::value)
construct_n(U* ptr, I n) noexcept
{
    memset(ptr, 0, n * sizeof(U));
}
/** default-construct n objects, non-trivial version */
template< class U, class I > _C4REQUIRE( ! std::is_trivially_default_constructible< U >::value)
construct_n(U* ptr, I n) noexcept
{
    for(I i = 0; i < n; ++i)
    {
        new (ptr+i) U();
    }
}

template< class U, class ...Args >
inline void construct(U* ptr, Args&&... args)
{
    new ((void*)ptr) U(std::forward< Args >(args)...);
}
template< class U, class I, class ...Args >
inline void construct_n(U* ptr, I n, Args&&... args)
{
    for(I i = 0; i < n; ++i)
    {
        new ((void*)(ptr + i)) U(std::forward< Args >(args)...);
    }
}

//-----------------------------------------------------------------------------
// copy-construct

template< class U > _C4REQUIRE(std::is_trivially_copy_constructible< U >::value)
copy_construct(U* dst, U const* src) noexcept
{
    memcpy(dst, src, sizeof(U));
}
template< class U > _C4REQUIRE( ! std::is_trivially_copy_constructible< U >::value)
copy_construct(U* dst, U const* src)
{
    new (dst) U(*src);
}
template< class U, class I > _C4REQUIRE(std::is_trivially_copy_constructible< U >::value)
copy_construct_n(U* dst, U const* src, I n) noexcept
{
    memcpy(dst, src, n * sizeof(U));
}
template< class U, class I > _C4REQUIRE( ! std::is_trivially_copy_constructible< U >::value)
copy_construct_n(U* dst, U const* src, I n)
{
    for(I i = 0; i < n; ++i)
    {
        new (dst + i) U(*(src + i));
    }
}

template< class U > _C4REQUIRE(std::is_scalar< U >::value)
copy_construct(U* dst, U src) noexcept // pass by value for scalar types
{
    *dst = src;
}
template< class U > _C4REQUIRE( ! std::is_scalar< U >::value)
copy_construct(U* dst, U const& src) // pass by reference for non-scalar types
{
    new (dst) U(src);
}
template< class U, class I > _C4REQUIRE(std::is_scalar< U >::value)
copy_construct_n(U* dst, U src, I n) noexcept // pass by value for scalar types
{
    for(I i = 0; i < n; ++i)
    {
        dst[i] = src;
    }
}
template< class U, class I > _C4REQUIRE( ! std::is_scalar< U >::value)
copy_construct_n(U* dst, U const& src, I n) // pass by reference for non-scalar types
{
    for(I i = 0; i < n; ++i)
    {
        new (dst + i) U(src);
    }
}

template< class U, size_t N >
C4_ALWAYS_INLINE void copy_construct(U (&dst)[N], U const (&src)[N]) noexcept
{
    copy_construct_n(dst, src, N);
}

//-----------------------------------------------------------------------------
// copy-assign

template< class U > _C4REQUIRE(std::is_trivially_copy_assignable< U >::value)
copy_assign(U* dst, U const* src) noexcept
{
    memcpy(dst, src, sizeof(U));
}
template< class U > _C4REQUIRE( ! std::is_trivially_copy_assignable< U >::value)
copy_assign(U* dst, U const* src) noexcept
{
    *dst = *src;
}
template< class U, class I > _C4REQUIRE(std::is_trivially_copy_assignable< U >::value)
copy_assign_n(U* dst, U const* src, I n) noexcept
{
    memcpy(dst, src, n * sizeof(U));
}
template< class U, class I > _C4REQUIRE( ! std::is_trivially_copy_assignable< U >::value)
copy_assign_n(U* dst, U const* src, I n) noexcept
{
    for(I i = 0; i < n; ++i)
    {
        dst[i] = src[i];
    }
}

template< class U > _C4REQUIRE(std::is_scalar< U >::value)
copy_assign(U* dst, U src) noexcept // pass by value for scalar types
{
    *dst = src;
}
template< class U > _C4REQUIRE( ! std::is_scalar< U >::value)
copy_assign(U* dst, U const& src) noexcept // pass by reference for non-scalar types
{
    *dst = src;
}
template< class U, class I > _C4REQUIRE(std::is_scalar< U >::value)
copy_assign_n(U* dst, U src, I n) noexcept // pass by value for scalar types
{
    for(I i = 0; i < n; ++i)
    {
        dst[i] = src;
    }
}
template< class U, class I > _C4REQUIRE( ! std::is_scalar< U >::value)
copy_assign_n(U* dst, U const& src, I n) noexcept // pass by reference for non-scalar types
{
    for(I i = 0; i < n; ++i)
    {
        dst[i] = src;
    }
}

template< class U, size_t N >
C4_ALWAYS_INLINE void copy_assign(U (&dst)[N], U const (&src)[N]) noexcept
{
    copy_assign_n(dst, src, N);
}

//-----------------------------------------------------------------------------
// move-construct

template< class U > _C4REQUIRE(std::is_trivially_move_constructible< U >::value)
move_construct(U* dst, U* src) noexcept
{
    memcpy(dst, src, sizeof(U));
}
template< class U > _C4REQUIRE( ! std::is_trivially_move_constructible< U >::value)
move_construct(U* dst, U* src) noexcept
{
    new (dst) U(std::move(*src));
}
template< class U, class I > _C4REQUIRE(std::is_trivially_move_constructible< U >::value)
move_construct_n(U* dst, U* src, I n) noexcept
{
    memcpy(dst, src, n * sizeof(U));
}
template< class U, class I > _C4REQUIRE( ! std::is_trivially_move_constructible< U >::value)
move_construct_n(U* dst, U* src, I n) noexcept
{
    for(I i = 0; i < n; ++i)
    {
        new (dst + i) U(std::move(src[i]));
    }
}

//-----------------------------------------------------------------------------
// move-assign

template< class U > _C4REQUIRE(std::is_trivially_move_assignable< U >::value)
move_assign(U* dst, U* src) noexcept
{
    memcpy(dst, src, sizeof(U));
}
template< class U > _C4REQUIRE( ! std::is_trivially_move_assignable< U >::value)
move_assign(U* dst, U* src) noexcept
{
    *dst = std::move(*src);
}
template< class U, class I > _C4REQUIRE(std::is_trivially_move_assignable< U >::value)
move_assign_n(U* dst, U* src, I n) noexcept
{
    memcpy(dst, src, n * sizeof(U));
}
template< class U, class I > _C4REQUIRE( ! std::is_trivially_move_assignable< U >::value)
move_assign_n(U* dst, U* src, I n) noexcept
{
    for(I i = 0; i < n; ++i)
    {
        *(dst + i) = std::move(*(src + i));
    }
}

//-----------------------------------------------------------------------------
// destroy

template< class U > _C4REQUIRE(std::is_trivially_destructible< U >::value)
destroy(U* ptr) noexcept
{
    C4_UNUSED(ptr); // nothing to do
}
template< class U > _C4REQUIRE( ! std::is_trivially_destructible< U >::value)
destroy(U* ptr) noexcept
{
    ptr->~U();
}
template< class U, class I > _C4REQUIRE(std::is_trivially_destructible< U >::value)
destroy_n(U* ptr, I n) noexcept
{
    C4_UNUSED(ptr);
    C4_UNUSED(n); // nothing to do
}
template< class U, class I > _C4REQUIRE( ! std::is_trivially_destructible< U >::value)
destroy_n(U* ptr, I n) noexcept
{
    for(I i = 0; i < n; ++i)
    {
        ptr[i].~U();
    }
}

#undef _C4REQUIRE

C4_END_NAMESPACE(c4)


//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// MEMORY ALLOCATION

C4_BEGIN_NAMESPACE(c4)

// c-style allocation ---------------------------------------------------------

// this API provides unaligned as well as aligned allocation functions.
// These functions forward the call to a modifiable function.

// aligned allocation. Thread-safe.
void* aalloc(size_t sz, size_t alignment);
void afree(void* ptr);
void* arealloc(void* ptr, size_t oldsz, size_t newsz, size_t alignment);

// classic, unaligned allocation. Thread-safe.
void* alloc(size_t sz);
void free(void* ptr);
void* realloc(void* ptr, size_t oldsz, size_t newsz);

// allocation setup facilities
using aalloc_type   = void* (*)(size_t size, size_t alignment);
using afree_type    = void  (*)(void *ptr);
using arealloc_type = void* (*)(void *ptr, size_t oldsz, size_t newsz, size_t alignment);

using alloc_type   = void* (*)(size_t size);
using free_type    = void  (*)(void *ptr);
using realloc_type = void* (*)(void *ptr, size_t oldsz, size_t newsz);

// set the function to be called
void set_aalloc(aalloc_type fn);
void set_afree(afree_type fn);
void set_arealloc(arealloc_type fn);

void set_alloc(alloc_type fn);
void set_free(free_type fn);
void set_realloc(realloc_type fn);

// get the function which will be called
alloc_type get_alloc();
free_type get_free();
realloc_type get_realloc();

aalloc_type get_aalloc();
free_type get_afree();
arealloc_type get_arealloc();

// c++-style allocation -------------------------------------------------------

/** C++17-style memory_resource. See http://en.cppreference.com/w/cpp/experimental/memory_resource */
struct MemoryResource
{
    const char *name = nullptr;
    virtual ~MemoryResource() {}

    void* allocate(size_t sz, size_t alignment = alignof(max_align_t))
    {
        void *mem = this->do_allocate(sz, alignment);
        C4_CHECK_MSG(mem != nullptr, "could not allocate %lu bytes", sz);
        return mem;
    }
    void* reallocate(void* ptr, size_t oldsz, size_t newsz, size_t alignment = alignof(max_align_t))
    {
        void *mem = this->do_reallocate(ptr, oldsz, newsz, alignment);
        C4_CHECK_MSG(mem != nullptr, "could not reallocate from %lu to %lu bytes", oldsz, newsz);
        return mem;
    }
    void deallocate(void* ptr, size_t sz, size_t alignment = alignof(max_align_t))
    {
        this->do_deallocate(ptr, sz, alignment);
    }

protected:

    virtual void* do_allocate(size_t sz, size_t alignment) = 0;
    virtual void* do_reallocate(void* ptr, size_t oldsz, size_t newsz, size_t alignment) = 0;
    virtual void  do_deallocate(void* ptr, size_t sz, size_t alignment) = 0;

};

/** A c4::aalloc-based memory resource. Thread-safe if the implementation called by
 * c4::aalloc() is safe. */
struct MemoryResourceMalloc : public MemoryResource
{

    MemoryResourceMalloc() { name = "malloc"; }
    virtual ~MemoryResourceMalloc() {}

protected:

    virtual void* do_allocate(size_t sz, size_t alignment) override
    {
        return c4::aalloc(sz, alignment);
    }

    virtual void  do_deallocate(void* ptr, size_t sz, size_t alignment) override
    {
        C4_UNUSED(sz);
        C4_UNUSED(alignment);
        c4::afree(ptr);
    }

    virtual void* do_reallocate(void* ptr, size_t oldsz, size_t newsz, size_t alignment) override
    {
        return c4::arealloc(ptr, oldsz, newsz, alignment);
    }

};


C4_ALWAYS_INLINE MemoryResource* get_memory_resource_malloc()
{
    static MemoryResourceMalloc mr;
    return &mr;
}

C4_BEGIN_NAMESPACE(detail)
C4_ALWAYS_INLINE MemoryResource* & get_memory_resource()
{
    static MemoryResource* mr = get_memory_resource_malloc();
    return mr;
}
C4_END_NAMESPACE(detail)

MemoryResource* get_memory_resource()
{
    return detail::get_memory_resource();
}
void set_memory_resource(MemoryResource* mr)
{
    C4_ASSERT(mr != nullptr);
    detail::get_memory_resource() = mr;
}


struct AllocationCounts
{
    size_t curr_allocs = 0;
    size_t curr_size = 0;
    size_t max_allocs = 0;
    size_t max_size = 0;
    size_t total_allocs = 0;
    size_t sum_size = 0;

    void clear_counts()
    {
        zero_mem(this);
    }

    void add_counts(void* ptr, size_t sz)
    {
        if(ptr == nullptr) return;
        ++curr_allocs;
        curr_size += sz;
        max_allocs = curr_allocs > max_allocs ? curr_allocs : max_allocs;
        max_size = curr_size > max_size ? curr_size : max_size;
        ++total_allocs;
        sum_size += sz;
    }
    void rem_counts(void *ptr, size_t sz)
    {
        if(ptr == nullptr) return;
        --curr_allocs;
        curr_size -= sz;
    }

    AllocationCounts operator- (AllocationCounts const& that)
    {
        AllocationCounts r(*this);
        r.curr_allocs -= that.curr_allocs;
        r.curr_size -= that.curr_size;
        r.max_allocs = max_allocs > that.max_allocs ? max_allocs : that.max_allocs;
        r.max_size = max_size > that.max_size ? max_size : that.max_size;
        r.total_allocs -= that.total_allocs;
        r.sum_size -= that.sum_size;
        return r;
    }
    AllocationCounts operator+ (AllocationCounts const& that)
    {
        AllocationCounts r(*this);
        r.curr_allocs += that.curr_allocs;
        r.curr_size += that.curr_size;
        r.max_allocs += max_allocs > that.max_allocs ? max_allocs : that.max_allocs;
        r.max_size += max_size > that.max_size ? max_size : that.max_size;
        r.total_allocs += that.total_allocs;
        r.sum_size += that.sum_size;
        return r;
    }

};


/** a MemoryResource which latches onto another MemoryResource
 * and counts allocations and sizes. */
class MemoryResourceCounts : public MemoryResource
{
public:

    MemoryResourceCounts() : m_resource(get_memory_resource()) { name = m_resource->name; }
    MemoryResourceCounts(MemoryResource *res) : m_resource(res) { name = m_resource->name; }

protected:

    MemoryResource *m_resource;

public:

    AllocationCounts counts;

protected:

    virtual void* do_allocate(size_t sz, size_t alignment) override
    {
        void *ptr = m_resource->allocate(sz, alignment);
        counts.add_counts(ptr, sz);
        return ptr;
    }

    virtual void  do_deallocate(void* ptr, size_t sz, size_t alignment) override
    {
        counts.rem_counts(ptr, sz);
        m_resource->deallocate(ptr, sz, alignment);
    }

    virtual void* do_reallocate(void* ptr, size_t oldsz, size_t newsz, size_t alignment) override
    {
        counts.rem_counts(ptr, oldsz);
        void* nptr = m_resource->reallocate(ptr, oldsz, newsz, alignment);
        counts.add_counts(nptr, newsz);
        return nptr;
    }

};

struct AllocatorBase
{
protected:

    MemoryResource *m_resource;

    AllocatorBase() : m_resource(get_memory_resource()) {}
    AllocatorBase(MemoryResource* mem) noexcept : m_resource(mem) {}

public:

    MemoryResource* resource() const { return m_resource; }

    /** for construct:
     * @see http://en.cppreference.com/w/cpp/experimental/polymorphic_allocator/construct */

/** SFINAE: enable the function with a void return type when a condition is verified */
#define _c4_void_if(cond) C4_ALWAYS_INLINE typename std::enable_if< cond, void >::type
/** @see http://en.cppreference.com/w/cpp/memory/uses_allocator */
#define _c4_uses_allocator(U) std::uses_allocator< U, MemoryResource* >::value
/** @see http://en.cppreference.com/w/cpp/types/is_constructible */
#define _c4_is_constructible(...) std::is_constructible< __VA_ARGS__ >::value

    // 1. types with no allocators
    template< class U, class... Args >
    _c4_void_if( ! _c4_uses_allocator(U) && _c4_is_constructible(U, Args...) )
    construct(U* ptr, Args&&... args)
    {
        c4::construct(ptr, std::forward< Args >(args)...);
    }
    template< class U, class I, class... Args >
    _c4_void_if( ! _c4_uses_allocator(U) && _c4_is_constructible(U, Args...) )
    construct_n(U* ptr, I n, Args&&... args)
    {
        c4::construct_n(ptr, n, std::forward< Args >(args)...);
    }

    // 2. types using allocators (ie, containers)

    // 2.1. can construct(std::allocator_arg_t, MemoryResource*, Args...)
    template< class U, class... Args >
    _c4_void_if(_c4_uses_allocator(U) && _c4_is_constructible(U, std::allocator_arg_t, MemoryResource*, Args...))
    construct(U* ptr, Args&&... args)
    {
        c4::construct(ptr, std::allocator_arg, m_resource, std::forward< Args >(args)...);
    }
    template< class U, class I, class... Args >
    _c4_void_if(_c4_uses_allocator(U) && _c4_is_constructible(U, std::allocator_arg_t, MemoryResource*, Args...))
    construct_n(U* ptr, I n, Args&&... args)
    {
        c4::construct_n(ptr, n, std::allocator_arg, m_resource, std::forward< Args >(args)...);
    }

    // 2.2. can construct(Args..., MemoryResource*)
    template< class U, class... Args >
    _c4_void_if(_c4_uses_allocator(U) && _c4_is_constructible(U, Args..., MemoryResource*))
    construct(U* ptr, Args&&... args)
    {
        c4::construct(ptr, std::forward< Args >(args)..., m_resource);
    }
    template< class U, class I, class... Args >
    _c4_void_if(_c4_uses_allocator(U) && _c4_is_constructible(U, Args..., MemoryResource*))
    construct_n(U* ptr, I n, Args&&... args)
    {
        c4::construct_n(ptr, n, std::forward< Args >(args)..., m_resource);
    }

    template< class U >
    static C4_ALWAYS_INLINE void destroy(U* ptr)
    {
        c4::destroy(ptr);
    }
    template< class U, class I >
    static C4_ALWAYS_INLINE void destroy_n(U* ptr, I n)
    {
        c4::destroy_n(ptr, n);
    }

#undef _c4_void_if
#undef _c4_is_constructible
#undef _c4_uses_allocator

};


/** A polymorphic allocator, acting as a proxy to a memory resource */
template< class T >
class Allocator : public AllocatorBase
{
public:

    template< class U > using rebind = Allocator< U >;
    template< class U > Allocator< U > rebound() { return Allocator< U >(*this); }

    Allocator() : AllocatorBase() {}
    Allocator(MemoryResource *r) : AllocatorBase(r) {}
    template< class U > Allocator(Allocator<U> const& that) : AllocatorBase(that.m_resource) {}

    Allocator(Allocator const&) = default;
    Allocator(Allocator     &&) = default;

    Allocator& operator= (Allocator const&) = delete; // WTF? why? @see http://en.cppreference.com/w/cpp/memory/polymorphic_allocator
    Allocator& operator= (Allocator     &&) = default;

    /** returns a default-constructed polymorphic allocator object
     * @see http://en.cppreference.com/w/cpp/memory/polymorphic_allocator/select_on_container_copy_construction      */
    Allocator select_on_container_copy_construct() const { return Allocator(*this); }

    T* allocate(size_t num_objs, size_t alignment = alignof(T))
    {
        C4_ASSERT(m_resource != nullptr);
        void* vmem = m_resource->allocate(num_objs * sizeof(T), alignment);
        T* mem = static_cast< T* >(vmem);
        return mem;
    }

    void deallocate(T * ptr, size_t num_objs, size_t alignment = alignof(T))
    {
        C4_ASSERT(m_resource != nullptr);
        m_resource->deallocate(ptr, num_objs * sizeof(T), alignment);
    }

    T* reallocate(T* ptr, size_t oldnum, size_t newnum, size_t alignment = alignof(T))
    {
        C4_ASSERT(m_resource != nullptr);
        void* vmem = m_resource->reallocate(ptr, oldnum * sizeof(T), newnum * sizeof(T), alignment);
        T* mem = static_cast< T* >(vmem);
        return mem;
    }

};


template< class T, size_t N = 16, size_t Alignment = alignof(T) >
class SmallAllocator : public AllocatorBase
{

    union {
        alignas(Alignment) char _m_arr[N * sizeof(T)];
        alignas(Alignment) T m_arr[N];
    };

public:

    template< class U > using rebind = SmallAllocator< U >;
    template< class U > SmallAllocator< U > rebound() { return SmallAllocator< U >(*this); }

    SmallAllocator() : AllocatorBase() {}
    SmallAllocator(MemoryResource *r) : AllocatorBase(r) {}
    template< class U > SmallAllocator(SmallAllocator<U> const& that) : AllocatorBase(that.m_resource) {}

    SmallAllocator(SmallAllocator const&) = default;
    SmallAllocator(SmallAllocator     &&) = default;

    SmallAllocator& operator= (SmallAllocator const&) = delete; // WTF? why? @see http://en.cppreference.com/w/cpp/memory/polymorphic_allocator
    SmallAllocator& operator= (SmallAllocator     &&) = default;

    /** returns a default-constructed polymorphic allocator object
     * @see http://en.cppreference.com/w/cpp/memory/polymorphic_allocator/select_on_container_copy_construction      */
    SmallAllocator select_on_container_copy_construct() const { return SmallAllocator(*this); }

    T* allocate(size_t num_objs, size_t alignment = alignof(T))
    {
        C4_ASSERT(m_resource != nullptr);
        if(num_objs <= N)
        {
            return m_arr;
        }
        void* vmem = m_resource->allocate(num_objs * sizeof(T), alignment);
        T* mem = static_cast< T* >(vmem);
        return mem;
    }

    void deallocate(T * ptr, size_t num_objs, size_t alignment = alignof(T))
    {
        if(ptr == &m_arr[0])
        {
            return;
        }
        C4_ASSERT(m_resource != nullptr);
        m_resource->deallocate(ptr, num_objs * sizeof(T), alignment);
    }

    T* reallocate(T* ptr, size_t oldnum, size_t newnum, size_t alignment = alignof(T))
    {
        C4_ASSERT(m_resource != nullptr);
        if(oldnum <= N && newnum <= N)
        {
            return m_arr;
        }
        else if(oldnum <= N && newnum > N)
        {
            return allocate(newnum, alignment);
        }
        else if(oldnum > N && newnum <= N)
        {
            deallocate(ptr, oldnum, alignment);
            return m_arr;
        }
        void* vmem = m_resource->reallocate(ptr, oldnum * sizeof(T), newnum * sizeof(T), alignment);
        T* mem = static_cast< T* >(vmem);
        return mem;
    }

private:

    MemoryResource* m_resource;

};

C4_END_NAMESPACE(c4)

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Storage facilities.


C4_BEGIN_NAMESPACE(c4)

//-----------------------------------------------------------------------------
#define _c4_DEFINE_ARRAY_TYPES(T, I) \
\
    using value_type = T;\
    using size_type = I;\
\
    using pointer = T*;\
    using const_pointer = T const*;\
\
    using reference = T&;\
    using const_reference = T const&;\
\
    using iterator = T*;\
    using const_iterator = T const*;\
\
    using difference_type = ptrdiff_t;

//-----------------------------------------------------------------------------
template< class T, class I = uint32_t >
class array_view
{
protected:

    T *m_ptr;
    I m_size;

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);

    C4_ALWAYS_INLINE operator array_view< const T, I > () const { return array_view< const T, I >((T const*)m_ptr, m_size); }

    array_view(array_view const&) = default;
    array_view(array_view     &&) = default;

    array_view& operator= (array_view const&) = default;
    array_view& operator= (array_view     &&) = default;

public:

    C4_ALWAYS_INLINE array_view() : m_ptr{}, m_size{0} {}
    C4_ALWAYS_INLINE array_view(T *p, I sz) : m_ptr{p}, m_size{sz} {}
    template< size_t N >
    C4_ALWAYS_INLINE array_view(T (&arr)[N]) : m_ptr{arr}, m_size{N} {}

    C4_ALWAYS_INLINE constexpr I type_size() const { return sizeof(T); }
    C4_ALWAYS_INLINE           I byte_size() const { return m_size*sizeof(T); }

    C4_ALWAYS_INLINE bool empty()    const noexcept { return m_size == 0; }
    C4_ALWAYS_INLINE I    size()     const noexcept { return m_size; }
    C4_ALWAYS_INLINE I    capacity() const noexcept { return m_size; }

    C4_ALWAYS_INLINE T      * data()       noexcept { return m_ptr; }
    C4_ALWAYS_INLINE T const* data() const noexcept { return m_ptr; }

    C4_ALWAYS_INLINE       iterator  begin()       noexcept { return m_ptr; }
    C4_ALWAYS_INLINE const_iterator  begin() const noexcept { return m_ptr; }
    C4_ALWAYS_INLINE const_iterator cbegin() const noexcept { return m_ptr; }

    C4_ALWAYS_INLINE       iterator  end()       noexcept { return m_ptr + m_size; }
    C4_ALWAYS_INLINE const_iterator  end() const noexcept { return m_ptr + m_size; }
    C4_ALWAYS_INLINE const_iterator cend() const noexcept { return m_ptr + m_size; }

    C4_ALWAYS_INLINE T&          front()       noexcept { C4_XASSERT(!empty()); return m_ptr[0]; }
    C4_ALWAYS_INLINE fastcref<T> front() const noexcept { C4_XASSERT(!empty()); return m_ptr[0]; }

    C4_ALWAYS_INLINE T&          back()       noexcept { C4_XASSERT(!empty()); return m_ptr[m_size - 1]; }
    C4_ALWAYS_INLINE fastcref<T> back() const noexcept { C4_XASSERT(!empty()); return m_ptr[m_size - 1]; }

    C4_ALWAYS_INLINE T&          operator[] (I i)       noexcept { C4_XASSERT(i >= 0 && i < m_size); return m_ptr[i]; }
    C4_ALWAYS_INLINE fastcref<T> operator[] (I i) const noexcept { C4_XASSERT(i >= 0 && i < m_size); return m_ptr[i]; }

    C4_ALWAYS_INLINE void clear() noexcept { m_size = 0; }

    C4_ALWAYS_INLINE void ltrim(I n) noexcept { C4_XASSERT(n >= 0 && n < m_size); m_ptr += n; }
    C4_ALWAYS_INLINE void rtrim(I n) noexcept { C4_XASSERT(n >= 0 && n < m_size); m_size -= n; }

    array_view view(I first = 0) const noexcept
    {
        C4_ASSERT(first >= 0 && first < m_size);
        return array_view(m_ptr + first, m_size - first);
    }
    array_view view(I first, I num) const noexcept
    {
        C4_ASSERT(first >= 0 && first < m_size);
        C4_ASSERT(first + num >= 0 && first + num < m_size);
        return array_view(m_ptr + first, num);
    }

};

C4_BEGIN_NAMESPACE(stg) // stg==SToraGe

struct growth_default;

//-----------------------------------------------------------------------------
/** a tag type for specifying the initial capacity of allocatable contiguous storage */
struct with_capacity_t {};
/** a tag type for initializing the containers with variadic arguments a la
 * initializer_list, minus the initializer_list problems.
 * @see */
struct aggregate_t {};

//-----------------------------------------------------------------------------
// Raw storage classes

/** @defgroup raw_storage_classes Raw storage classes
 *
 * These classes are a building block for the several flavours of
 * contiguous containers. They offer a convenient way to hold a
 * number of (almost-)contiguous objects (via the index-based [] operator).
 *
 *     - The memory used by these objects is NOT automatically allocated or\n
 *       freed. Use the protected methods _raw_reserve(I n), _raw_trim(I n)
 *     - The elements in the raw storage are NOT automatically constructed\n
 *       or destroyed.
 */

//-----------------------------------------------------------------------------
/** Type traits for raw storage classes.
 * @ingroup raw_storage_classes */
template< class T >
struct raw_storage_traits
{
    using storage_type = T;
    using value_type = typename T::value_type;
    using size_type = typename T::size_type;

    constexpr static const bool fixed = false;
    constexpr static const bool contiguous = false;
}

#define C4_DEFINE_STORAGE_TRAITS(type, is_fixed, is_contiguous, ...)    \
    template< __VA_ARGS__ >                                             \
    struct raw_storage_traits< type >                                   \
    {                                                                   \
        using storage_type = T;                                         \
        using value_type = typename T::value_type;                      \
        using size_type = typename T::size_type;                        \
                                                                        \
        constexpr static const bool fixed = is_fixed;                   \
        constexpr static const bool contiguous = is_contiguous;         \
    }

//-----------------------------------------------------------------------------
/** Utility class which uses SFINAE to dispatch construction/destruction
 * to the appropriate functions. This class is meant to be used by container
 * implementations to aid in object management.
 * @ingroup raw_storage_classes */
template< class T >
struct raw_storage_util : public raw_storage_traits< T >
{
    using raw_storage_traits< T >::contiguous;

#define _c4require(what) C4_ALWAYS_INLINE static typename std::enable_if< what, void >::type

    template< class ...Args >
    _c4require(contiguous) construct(T& dest, I first, I n, Args&&... args)
    {
        dest.m_allocator.construct_n(dest.data() + first, n, std::forward< Args >(args)...);
    }
    template< class ...Args >
    _c4require( ! contiguous) construct(T& dest, I first, I n, Args&&... args)
    {
        dest._raw_construct(first, n, std::forward< Args >(args)...);
    }

    _c4require(contiguous) destroy(T& dest, I first, I n)
    {
        dest.m_allocator.destroy_n(dest.data() + first, n);
    }
    _c4require( ! contiguous) destroy(T& dest, I first, I n)
    {
        dest._raw_destroy(first, n);
    }

    _c4require(contiguous) move_construct(T& dest, T const& src, I first, I n)
    {
        c4::move_construct_n(dest.data() + first, src.data() + first, n);
    }
    _c4require( ! contiguous) move_construct(T& dest, T const& src, I first, I n)
    {
        dest._raw_move_construct(src, first, n);
    }

    _c4require(contiguous) copy_construct(T& dest, T const& src, I first, I n)
    {
        c4::copy_construct_n(dest.data() + first, src.data() + first, n);
    }
    _c4require( ! contiguous) copy_construct(T& dest, T const& src, I first, I n)
    {
        dest._raw_copy_construct(src, first, n);
    }

    _c4require(contiguous) move_assign(T& dest, T const& src, I first, I n)
    {
        c4::move_assign_n(dest.data() + first, src.data() + first, n);
    }
    _c4require( ! contiguous) move_assign(T& dest, T const& src, I first, I n)
    {
        dest._raw_move_assign(src, first, n);
    }

    _c4require(contiguous) copy_assign(T& dest, T const& src, I first, I n)
    {
        c4::copy_assign_n(dest.data() + first, src.data() + first, n);
    }
    _c4require( ! contiguous) copy_assign(T& dest, T const& src, I first, I n)
    {
        dest._raw_copy_assign(src, first, n);
    }


#undef _c4require
};

//-----------------------------------------------------------------------------
/** @ingroup raw_storage_classes */
template< class T, size_t N, class I = uint32_t, I Alignment = alignof(T) >
struct raw_fixed
{

    union {
        alignas(Alignment) char _m_buf[N * sizeof(T)];
        alignas(Alignment) T m_ptr[N];
    };

    _c4_DEFINE_ARRAY_TYPES(T, I);

    C4_ALWAYS_INLINE T& operator[] (I i) { return m_ptr[i]; }
    C4_ALWAYS_INLINE constexpr fastcref<T> operator[] (I i) const { return m_ptr[i]; }

    C4_ALWAYS_INLINE T      * data()       noexcept { return m_ptr; }
    C4_ALWAYS_INLINE T const* data() const noexcept { return m_ptr; }

    C4_ALWAYS_INLINE constexpr I max_capacity() const noexcept { return N; }
    C4_ALWAYS_INLINE constexpr I capacity() const noexcept { return N; }
    C4_ALWAYS_INLINE constexpr I next_capacity(I cap) const noexcept { return N; }

    void _raw_reserve(I n) { C4_XASSERT(n <= N); }
    void _raw_trim(I n) { C4_XASSERT(n <= N); }
};

C4_DEFINE_STORAGE_TRAITS(raw_fixed< T, N, I, Alignment >,
                         true, // fixed
                         true, // contiguous
                         class T, size_t N, class I, I Alignment);

//-----------------------------------------------------------------------------

/** raw storage variable size: allocatable, contiguous
 * @ingroup raw_storage_classes */
template
<
    class T,
    class I = uint32_t,
    I Alignment = alignof(T),
    class Alloc = Allocator< T >,
    class GrowthType = growth_default
>
struct raw
{

    T*    m_ptr;
    I     m_capacity;
    Alloc m_allocator;

    _c4_DEFINE_ARRAY_TYPES(T, I);

    using allocator_type = Alloc;

    raw() : m_ptr(nullptr), m_capacity(0), m_allocator() {}
    raw(Alloc const& a) : m_ptr(nullptr), m_capacity(0), m_allocator(a) {}
    ~raw()
    {
        C4_ASSERT_MSG(m_ptr == nullptr, "the container using this did not free the storage");
    }

    /** @todo implement this */
    raw(raw const& that) = delete;
    raw(raw     && that) = default;

    /** @todo implement this */
    raw& operator=(raw const& that) = delete;
    raw& operator=(raw     && that) = default;

    C4_ALWAYS_INLINE T& operator[] (I i) { return m_ptr[i]; }
    C4_ALWAYS_INLINE constexpr fastcref<T> operator[] (I i) const { return m_ptr[i]; }

    C4_ALWAYS_INLINE T      * data()       noexcept { return m_ptr; }
    C4_ALWAYS_INLINE T const* data() const noexcept { return m_ptr; }

    C4_ALWAYS_INLINE I max_capacity() const noexcept { return std::numeric_limits< I >::max() - 1; }
    C4_ALWAYS_INLINE I capacity() const noexcept { return m_capacity; }
    C4_ALWAYS_INLINE I next_capacity(I desired) const
    {
        return GrowthType::next_size(m_capacity, desired);
    }

    void _raw_reserve(I cap)
    {

    }

    void _raw_trim(I cap)
    {

    }
};

C4_DEFINE_STORAGE_TRAITS(raw< T, I, Alignment, Alloc, GrowthType >,
                         false, // fixed
                         true,  // contiguous
                         class T, class I, I Alignment, class Alloc, class GrowthType);


//-----------------------------------------------------------------------------
/** raw storage: allocatable and paged. This is NOT a contiguous storage structure.
  * However, it does behave as such, offering the [] operator with contiguous
  * range indices. This is useful for minimizing allocations and data copies in
  * dynamic array-based containers as flat_list.
  *
  * @ingroup raw_storage_classes
  * @todo add a raw structure with a runtime-determined page size */
template
<
    class T,
    size_t PageSize,      //< The page size. Must be a power of two.
    class I = uint32_t,
    I Alignment = alignof(T),
    class Alloc = Allocator< T >
>
struct raw_paged
{
    static_assert(PageSize > 1, "PageSize must be > 1")
    static_assert(PageSize & (PageSize - 1) == 0, "PageSize must be a power of two")

    //! id mask: all the bits up to PageSize. Use to extract the position of an index within a page.
    constexpr static const I _raw_idmask = (I)PageSize - 1;

    //! page mask: bits complementary to PageSize. Use to extract the page of an index.
    constexpr static const I _raw_pgmask = ~ ((I)PageSize - 1); //< all the bits

    T    **m_pages;      //< array containing the pages
    I      m_num_pages;  //< number of current pages in the array
    Alloc  m_allocator;

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);
    using allocator_type = Alloc;

    raw_paged() : m_pages(nullptr), m_num_pages(0), m_allocator() {}
    raw_paged(Alloc const& a) : m_pages(nullptr), m_num_pages(0), m_allocator(a) {}

    raw_paged(raw_paged const& that) = delete;
    raw_paged(raw_paged     && that) = default;
    raw_paged& operator=(raw_paged const& that) = delete;
    raw_paged& operator=(raw_paged     && that) = default;

    C4_ALWAYS_INLINE T& operator[] (I i)
    {
        C4_XASSERT(i < capacity());
        I pg = i & _raw_pgmask;
        I id = i & _raw_idmask;
        C4_XASSERT(pg < m_num_pages);
        C4_XASSERT(id < PageSize);
        return m_pages[pg][id];
    }
    C4_ALWAYS_INLINE constexpr fastcref<T> operator[] (I i) const
    {
        C4_XASSERT(i < capacity());
        I pg = i & _raw_pgmask;
        I id = i & _raw_id;
        C4_XASSERT(pg < m_num_pages);
        C4_XASSERT(id < PageSize);
        return m_pages[pg][id];
    }

    C4_ALWAYS_INLINE I max_capacity() const noexcept { return std::numeric_limits< I >::max() - 1; }
    C4_ALWAYS_INLINE I capacity() const noexcept { return m_num_pages * PageSize; }
    C4_ALWAYS_INLINE I next_capacity(I desired) const
    {
        I cap = capacity();
        if(desired < cap) return cap;
        I np = desired / PageSize;
        I cap = np * PageSize;
        return cap;
    }

protected:

    void _raw_reserve(I cap)
    {
        if(cap <= capacity()) return;
        I np = cap / PageSize;
        C4_XASSERT(np * PageSize >= capacity());
        if(np > m_num_pages)
        {
            auto at = m_allocator.rebound< T* >();
            m_pages = at.reallocate(m_pages, m_num_pages, np);
            for(I i = m_num_pages; i < np; ++i)
            {
                m_pages[i] = m_allocator.allocate(PageSize, Alignment);
            }
        }
        m_num_pages = np;
    }
    void _raw_trim(I to)
    {
        if(m_pages == nullptr) return;
        I np = to / PageSize;
        if(np >= m_num_pages) return;
        for(I i = np; i < m_num_pages; ++i)
        {
            m_allocator.deallocate(m_pages[i], PageSize, Alignment);
        }
        auto at = m_allocator.rebound< T* >();
        if(to == 0)
        {
            at.deallocate(m_pages, m_num_pages);
            m_pages = nullptr;
        }
        else
        {
            m_pages = at.reallocate(m_pages, m_num_pages, np);
        }
        m_num_pages = np;
    }
    void _raw_copy(raw_paged const& that, I first, I num)
    {
        C4_NOT_IMPLEMENTED();
    }
};


C4_DEFINE_STORAGE_TRAITS(raw_paged< T, I, PageSize, Alignment, Alloc >,
                         false, // fixed
                         false, // contiguous
                         true, // paged
                         class T, class I, I PageSize, I Alignment, class Alloc);

//-----------------------------------------------------------------------------
/* CRTP bases for contiguous storage.
 * These serve as a development scaffold until the implementation is validated
 * and stable. Afterwards, the member functions in these base classes
 * should be copied and adapted into each of the contiguous storage classes. */


#define _c4this  static_cast< S* >(this)
#define _c4cthis static_cast< S const* >(this)

/** CRTP base for non-resizeable contiguous storage */
template< class T, class I, class S >
struct contiguous_base
{
protected:

    // prevent construction/destruction of an object of this type unless through derived types

    contiguous_base() {}
    ~contiguous_base() {}

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);

    using view_type = array_view< T, I >;
    using const_view_type = array_view< const T, I >;

    C4_ALWAYS_INLINE constexpr I type_size() const { return sizeof(T); }
    C4_ALWAYS_INLINE           I byte_size() const { return _c4this->size() * sizeof(T); }

public:

    C4_ALWAYS_INLINE operator view_type () { return view_type(_c4this->m_ptr, _c4this->size()); }
    C4_ALWAYS_INLINE operator const_view_type () const { return const_view_type(_c4this->m_ptr, _c4this->size()); }

    view_type view(I first = 0)
    {
        C4_ASSERT(first >= 0 && first < _c4this->size());
        return view_type(_c4this->m_ptr + first, _c4this->size() - first);
    }
    const_view_type view(I first = 0) const
    {
        C4_ASSERT(first >= 0 && first < _c4this->size());
        return view_type(_c4this->m_ptr + first, _c4this->size() - first);
    }
    view_type view(I first, I num)
    {
        C4_ASSERT(first >= 0 && first < _c4this->size());
        C4_ASSERT(first + num >= 0 && first + num < _c4this->size());
        return view_type(_c4this->m_ptr + first, num);
    }
    const_view_type view(I first, I num) const
    {
        C4_ASSERT(first >= 0 && first < _c4this->size());
        C4_ASSERT(first + num >= 0 && first + num < _c4this->size());
        return view_type(_c4this->m_ptr + first, num);
    }

    void fill(T const& v)
    {
        C4_ASSERT(_c4this->m_size > 0);
        copy_assign_n(_c4this->m_ptr, v, _c4this->m_size);
    }

    void assign(T const* v, I sz)
    {
        if(v == _c4this->m_ptr && sz == _c4this->size()) return;
        _c4this->resize(sz); // resize() for fixed-size storage just asserts whether the size is the same
        copy_assign_n(_c4this->m_ptr, v, sz);
    }

    void assign(array_view< T, I > v)
    {
        assign(v.data(), v.size());
    }

    void assign(aggregate_t, std::initializer_list< T > il)
    {
        assign(il.begin(), il.size());
    }

    C4_ALWAYS_INLINE bool is_valid_iterator(const_iterator it) const noexcept
    {
        return it >= _c4cthis->m_ptr && it <= _c4cthis->m_ptr + _c4cthis->size();
    }
    C4_ALWAYS_INLINE bool is_valid_index(I i) const noexcept
    {
        return i >= 0 && i < _c4cthis->size();
    }

};


/** CRTP base for resizeable contiguous storage */
template< class T, class I, class S >
struct contiguous_base_rs : public contiguous_base< T, I, S >
{
    using contiguous_base< T, I, S >::is_valid_iterator;

protected:

    // prevent construction/destruction of an object of this type unless through derived types

    contiguous_base_rs() {}
    ~contiguous_base_rs() {}

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);

public:

    // emplace
    template< class... Args >
    iterator emplace(const_iterator pos, Args&&... args)
    {
        C4_XASSERT(pos >= _c4this->m_ptr && pos <= _c4this->m_ptr + _c4this->m_size);
        I ipos = I(pos - _c4this->m_ptr);
        _c4this->_growto(_c4this->m_size + 1, pos);
        pos = _c4this->m_ptr + ipos;
        _c4this->_construct(pos, std::forward< Args >(args)...);
        ++_c4this->m_size;
        return (iterator)pos;
    }
    template< class... Args >
    void emplace_back(Args&& ...a)
    {
        _c4this->_growto(_c4this->m_size + 1);
        _c4this->_construct(_c4this->m_ptr + _c4this->m_size, std::forward< Args >(a)...);
        ++_c4this->m_size;
    }
    template< class... Args >
    void emplace_front(Args&& ...a)
    {
        _c4this->_growto(_c4this->m_size + 1);
        _c4this->_construct(_c4this->m_ptr, std::forward< Args >(a)...);
        ++_c4this->m_size;
    }

    // push

    void push_back(T const& val)
    {
        _c4this->_growto(_c4this->m_size + 1);
        _c4this->_construct(_c4this->m_ptr + _c4this->m_size, val);
        ++_c4this->m_size;
    }
    void push_back(T && val)
    {
        _c4this->_growto(_c4this->m_size + 1);
        _c4this->_construct(_c4this->m_ptr + _c4this->m_size, std::move(val));
        ++_c4this->m_size;
    }

    void push_front(T const& val)
    {
        _c4this->_growto(_c4this->m_size + 1, _c4this->m_ptr);
        _c4this->_construct(_c4this->m_ptr, val);
        ++_c4this->m_size;
    }
    void push_front(T && val)
    {
        _c4this->_growto(_c4this->m_size + 1, _c4this->m_ptr);
        _c4this->_construct(_c4this->m_ptr, std::move(val));
        ++_c4this->m_size;
    }

    // pop

    void pop_back()
    {
        C4_XASSERT(_c4this->m_size > 0);
        _c4this->_growto(_c4this->m_size - 1);
        --_c4this->m_size;
    }
    void pop_front()
    {
        C4_XASSERT(_c4this->m_size > 0);
        _c4this->_growto(_c4this->m_size - 1, _c4this->m_ptr);
        --_c4this->m_size;
    }

    // insert

    iterator insert(const_iterator pos, T const& value)
    {
        C4_XASSERT(is_valid_iterator(pos));
        I ipos = I(pos - _c4this->m_ptr);
        _c4this->_growto(_c4this->m_size + 1, pos);
        pos = _c4this->m_ptr + ipos;
        _c4this->_construct(pos, value);
        ++_c4this->m_size;
        return (iterator)pos;
    }
    iterator insert(const_iterator pos, T&& value)
    {
        C4_XASSERT(is_valid_iterator(pos));
        I ipos = I(pos - _c4this->m_ptr);
        _c4this->_growto(_c4this->m_size + 1, pos);
        pos = _c4this->m_ptr + ipos;
        _c4this->_construct(pos, std::move(value));
        ++_c4this->m_size;
        return (iterator)pos;
    }
    iterator insert(const_iterator pos, I count, T const& value)
    {
        C4_XASSERT(is_valid_iterator(pos));
        I ipos = I(pos - _c4this->m_ptr);
        _c4this->_growto(_c4this->m_size + count, pos);
        pos = _c4this->m_ptr + ipos;
        _c4this->_construct_n(pos, value, count);
        _c4this->m_size += count;
        return (iterator)pos;
    }
    template< class InputIt >
    iterator insert(const_iterator pos, InputIt first, InputIt last)
    {
        C4_XASSERT(is_valid_iterator(pos));
        I ipos = I(pos - _c4this->m_ptr);
        I count = (I)std::distance(first, last);
        _c4this->_growto(_c4this->m_size + count, pos);
        pos = _c4this->m_ptr + ipos;
        for(I i = 0; first != last; ++first, ++i)
        {
            _c4this->_construct(pos + i, first);
        }
        _c4this->m_size += count;
        return (iterator)pos;
    }
    iterator insert(const_iterator pos, aggregate_t, std::initializer_list<T> ilist)
    {
        C4_XASSERT(is_valid_iterator(pos));
        I ipos = I(pos - _c4this->m_ptr);
        _c4this->_growto(_c4this->m_size + ilist.size(), pos);
        pos = _c4this->m_ptr + ipos;
        I i = 0;
        for(auto const& v : ilist)
        {
            _c4this->_construct((pos++) + i, v);
        }
        pos = _c4this->m_ptr + ipos;
        _c4this->m_size += ilist.size();
        return (iterator)pos;
    }

    /** removes the element at pos */
    iterator erase(const_iterator pos)
    {
        C4_XASSERT(is_valid_iterator(pos) && _c4this->size() > 0);
        I ipos = I(pos - _c4this->m_ptr);
        _c4this->_grow_to(_c4this->size() - 1, pos);
        pos = _c4this->m_ptr + ipos;
        --_c4this->m_size;
        return (iterator)pos;
    }
    /** removes the elements in the range [first; last). */
    iterator erase(const_iterator first, const_iterator last)
    {
        I dist = (I)std::distance(first, last);
        if(!dist) return (iterator)first;
        C4_XASSERT(is_valid_iterator(first) && _c4this->size() >= dist);
        I ipos = I(first - _c4this->m_ptr);
        _c4this->_grow_to(_c4this->size() - dist, first);
        first = _c4this->m_ptr + ipos;
        _c4this->m_size -= dist;
        return (iterator)first;
    }

};

#undef _c4this
#undef _c4cthis

//-----------------------------------------------------------------------------
/** contiguous storage, fixed size+capacity (cannot resize down) */
template< class T, size_t N, class I = uint32_t, I Alignment = alignof(T) >
class contiguous_fixed_size : public contiguous_base< T, I, contiguous_fixed_size< T, N, I, Alignment > >
{
    C4_STATIC_ASSERT(N <= std::numeric_limits< I >::max());

    friend struct contiguous_base< T, I, contiguous_fixed_size< T, N, I, Alignment > >;
    using base_type = contiguous_base< T, I, contiguous_fixed_size< T, N, I, Alignment > >;

protected:

    alignas(Alignment) T m_ptr[N];

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);

    enum { alignment = Alignment };

public:

    C4_ALWAYS_INLINE operator array_view< T, I > () { return array_view< T, I >(m_ptr, N); }
    C4_ALWAYS_INLINE operator array_view< const T, I > () const { return array_view< const T, I >(m_ptr, N); }

public:

    contiguous_fixed_size() {}
    contiguous_fixed_size(array_view< T, I > const& v) { assign(v); }
    contiguous_fixed_size(array_view< T, I >     && v) { assign(std::move(v)); }
    //contiguous_fixed_size(std::initializer_list< T > il) { assign(il); }

    // provided for compatibility with other contiguous storages
    contiguous_fixed_size(I sz) { C4_ASSERT(sz == N); }
    contiguous_fixed_size(I sz, with_capacity_t, I cap) { C4_ASSERT(sz == N && cap == N); }

    C4_ALWAYS_INLINE constexpr bool empty()    const { return false; }
    C4_ALWAYS_INLINE constexpr I    size()     const { return N; }
    C4_ALWAYS_INLINE constexpr I    max_size() const { return N; }
    C4_ALWAYS_INLINE constexpr I    capacity() const { return N; }

    C4_ALWAYS_INLINE                 iterator  begin()       { return m_ptr; }
    C4_ALWAYS_INLINE constexpr const_iterator  begin() const { return m_ptr; }
    C4_ALWAYS_INLINE constexpr const_iterator cbegin() const { return m_ptr; }

    C4_ALWAYS_INLINE                 iterator  end()       { return m_ptr + I(N); }
    C4_ALWAYS_INLINE constexpr const_iterator  end() const { return m_ptr + I(N); }
    C4_ALWAYS_INLINE constexpr const_iterator cend() const { return m_ptr + I(N); }

    C4_ALWAYS_INLINE           T&          front()       { return m_ptr[0]; }
    C4_ALWAYS_INLINE constexpr fastcref<T> front() const { return m_ptr[0]; }

    C4_ALWAYS_INLINE           T&          back()       { return m_ptr[I(N) - 1]; }
    C4_ALWAYS_INLINE constexpr fastcref<T> back() const { return m_ptr[I(N) - 1]; }

    C4_ALWAYS_INLINE           T      * data()       { return m_ptr; }
    C4_ALWAYS_INLINE constexpr T const* data() const { return m_ptr; }

    C4_ALWAYS_INLINE T&          operator[] (I i)       { C4_XASSERT(i >= 0 && i < N); return m_ptr[i]; }
    C4_ALWAYS_INLINE fastcref<T> operator[] (I i) const { C4_XASSERT(i >= 0 && i < N); return m_ptr[i]; }

private:

    // these functions are provided for compatibility with the scaffold CRTP

    C4_ALWAYS_INLINE void reserve(I cap) { C4_ASSERT(cap == I(N)); }
    C4_ALWAYS_INLINE void shrink_to_fit() {}
    C4_ALWAYS_INLINE void resize(I sz) { C4_ASSERT(sz == I(N)); }
    C4_ALWAYS_INLINE void clear() { C4_NEVER_REACH(); }

    template< class U, class... Args >
    C4_ALWAYS_INLINE static void _construct(U *ptr, Args&&... args)
    {
        ::c4::construct(ptr, std::forward< Args >(args)...);
    }

};

//-----------------------------------------------------------------------------
/** contiguous storage, variable size, fixed capacity. */
template< class T, size_t N, class I = uint32_t, I Alignment = alignof(T) >
class contiguous_fixed_capacity : public contiguous_base_rs< T, I, contiguous_fixed_capacity< T, N, I, Alignment > >
{
    C4_STATIC_ASSERT(N <= std::numeric_limits< I >::max());

    friend struct contiguous_base< T, I, contiguous_fixed_capacity< T, N, I, Alignment > >;
    friend struct contiguous_base_rs< T, I, contiguous_fixed_capacity< T, N, I, Alignment > >;
    using base_type = contiguous_base_rs< T, I, contiguous_fixed_capacity< T, N, I, Alignment > >;

protected:

    union {
        alignas(Alignment) char _m_buf[N * sizeof(T)];
        alignas(Alignment) T m_ptr[N];
    };

    I m_size;

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);

    enum { alignment = Alignment };

public:

    contiguous_fixed_capacity() : m_size{0} {}
    contiguous_fixed_capacity(I sz) : contiguous_fixed_capacity() { resize(sz); }

    // provided for compatibility
    contiguous_fixed_capacity(with_capacity_t, I cap) : contiguous_fixed_capacity() { C4_ASSERT_MSG(cap == N, "capacity: asked %u, should be %u", (uint32_t)cap, (uint32_t)N); }
    contiguous_fixed_capacity(I sz, with_capacity_t, I cap) : contiguous_fixed_capacity() { C4_ASSERT_MSG(cap == N, "capacity: asked %u, should be %u", (uint32_t)cap, (uint32_t)N); resize(sz); }

    contiguous_fixed_capacity(aggregate_t, std::initializer_list< T > il) { assign(aggregate_t{}, il); }

    C4_ALWAYS_INLINE           bool empty()    const { return m_size == 0; }
    C4_ALWAYS_INLINE           I    size()     const { return m_size; }
    C4_ALWAYS_INLINE constexpr I    max_size() const { return N; }
    C4_ALWAYS_INLINE constexpr I    capacity() const { return N; }

    C4_ALWAYS_INLINE           T      * data()       { return m_ptr; }
    C4_ALWAYS_INLINE constexpr T const* data() const { return m_ptr; }

    C4_ALWAYS_INLINE                 iterator  begin()       { return m_ptr; }
    C4_ALWAYS_INLINE constexpr const_iterator  begin() const { return m_ptr; }
    C4_ALWAYS_INLINE constexpr const_iterator cbegin() const { return m_ptr; }

    C4_ALWAYS_INLINE                 iterator  end()       { return m_ptr + m_size; }
    C4_ALWAYS_INLINE           const_iterator  end() const { return m_ptr + m_size; }
    C4_ALWAYS_INLINE           const_iterator cend() const { return m_ptr + m_size; }

    C4_ALWAYS_INLINE T&          front()       { C4_XASSERT(!empty()); return m_ptr[0]; }
    C4_ALWAYS_INLINE fastcref<T> front() const { C4_XASSERT(!empty()); return m_ptr[0]; }

    C4_ALWAYS_INLINE T&          back()       { C4_XASSERT(!empty()); return m_ptr[m_size - 1]; }
    C4_ALWAYS_INLINE fastcref<T> back() const { C4_XASSERT(!empty()); return m_ptr[m_size - 1]; }

    C4_ALWAYS_INLINE T&          operator[] (I i)       { C4_XASSERT(i >= 0 && i < m_size); return m_ptr[i]; }
    C4_ALWAYS_INLINE fastcref<T> operator[] (I i) const { C4_XASSERT(i >= 0 && i < m_size); return m_ptr[i]; }

    C4_ALWAYS_INLINE void resize(I sz) { C4_ASSERT(sz >= 0 && sz <= N); m_size = sz; }
    C4_ALWAYS_INLINE void clear() { m_size = 0; }

protected:

    // these functions are provided for compatibility with the scaffold CRTP

    C4_ALWAYS_INLINE void reserve(I cap) { C4_ASSERT(cap == N); }
    C4_ALWAYS_INLINE void shrink_to_fit() {}

    template< class U, class... Args >
    C4_ALWAYS_INLINE static void _construct(U *ptr, Args&&... args)
    {
        ::c4::construct(ptr, std::forward< Args >(args)...);
    }

    void _growto(I sz)
    {
        C4_ASSERT(sz <= N);
    }
    void _growto(I sz, const_iterator before_this)
    {
        C4_ASSERT(sz <= N);
        C4_NOT_IMPLEMENTED();
    }
};


//-----------------------------------------------------------------------------
/** contiguous storage, allocatable */
template
<
    class T,
    class I = uint32_t,
    I Alignment = alignof(T),
    class Alloc = c4::Allocator< T >,
    class GrowthType = growth_default
>
struct contiguous : public contiguous_base_rs< T, I, contiguous< T, I, Alignment, Alloc, GrowthType > >
{
    friend struct contiguous_base< T, I, contiguous< T, I, Alignment, Alloc, GrowthType > >;
    friend struct contiguous_base_rs< T, I, contiguous< T, I, Alignment, Alloc, GrowthType > >;
    using base_type = contiguous_base_rs< T, I, contiguous< T, I, Alignment, Alloc, GrowthType > >;
protected:

    T*    m_ptr;
    I     m_size;
    I     m_capacity;
    Alloc m_allocator;

public:

    _c4_DEFINE_ARRAY_TYPES(T, I);

    enum { alignment = Alignment };
    using allocator_type = Alloc;
    using growth_type = GrowthType;

    C4_ALWAYS_INLINE operator array_view< T, I > () { return array_view< T, I >(m_ptr, m_size); }
    C4_ALWAYS_INLINE operator array_view< const T, I > () const { return array_view< const T, I >(m_ptr, m_size); }

public:

    allocator_type get_allocator() const { return m_allocator; }

    C4_ALWAYS_INLINE bool empty()    const { return m_size == 0; }
    C4_ALWAYS_INLINE I    size()     const { return m_size; }
    C4_ALWAYS_INLINE I    max_size() const { return allocator_type::max_size(); }
    C4_ALWAYS_INLINE I    capacity() const { return m_capacity; }

    C4_ALWAYS_INLINE T      * data()       { return m_ptr; }
    C4_ALWAYS_INLINE T const* data() const { return m_ptr; }

    C4_ALWAYS_INLINE       iterator  begin()       { return m_ptr; }
    C4_ALWAYS_INLINE const_iterator  begin() const { return m_ptr; }
    C4_ALWAYS_INLINE const_iterator cbegin() const { return m_ptr; }

    C4_ALWAYS_INLINE       iterator  end()       { return m_ptr + m_size; }
    C4_ALWAYS_INLINE const_iterator  end() const { return m_ptr + m_size; }
    C4_ALWAYS_INLINE const_iterator cend() const { return m_ptr + m_size; }

    C4_ALWAYS_INLINE T&          front()       { C4_XASSERT(!empty()); return *m_ptr; }
    C4_ALWAYS_INLINE fastcref<T> front() const { C4_XASSERT(!empty()); return *m_ptr; }

    C4_ALWAYS_INLINE T&          back()       { C4_XASSERT(!empty()); return *(m_ptr + (m_size - 1)); }
    C4_ALWAYS_INLINE fastcref<T> back() const { C4_XASSERT(!empty()); return *(m_ptr + (m_size - 1)); }

    C4_ALWAYS_INLINE T&          operator[] (I i)       { C4_XASSERT(i >= 0 && i < m_size); return m_ptr[i]; }
    C4_ALWAYS_INLINE fastcref<T> operator[] (I i) const { C4_XASSERT(i >= 0 && i < m_size); return m_ptr[i]; }

    contiguous()                        : m_ptr(nullptr), m_size(0), m_capacity(0), m_allocator( ) {}
    contiguous(allocator_type const& a) : m_ptr(nullptr), m_size(0), m_capacity(0), m_allocator(a) {}

    contiguous(I sz)                          : contiguous( ) { resize(sz); }
    contiguous(I sz, allocator_type const& a) : contiguous(a) { resize(sz); }

    contiguous(with_capacity_t, I cap)                          : contiguous( ) { reserve(cap); }
    contiguous(with_capacity_t, I cap, allocator_type const& a) : contiguous(a) { reserve(cap); }

    contiguous(I sz, with_capacity_t, I cap)                          : contiguous( ) { reserve(cap); resize(sz); }
    contiguous(I sz, with_capacity_t, I cap, allocator_type const& a) : contiguous(a) { reserve(cap); resize(sz); }

    contiguous(contiguous const& that) : contiguous(that.m_allocator) { assign(that); }
    contiguous(contiguous     && that) : contiguous(that.m_allocator) { assign(std::move(that)); }

    contiguous& operator= (contiguous const& that) { assign(that); return *this; }
    contiguous& operator= (contiguous     && that) { assign(std::move(that)); return *this; }

    ~contiguous()
    {
        _free();
    }

    void clear()
    {
        resize(0);
    }

    using base_type::assign;
    void assign(contiguous const& that)
    {
        if(&that == this) return;
        if(that.m_size == 0)
        {
            clear();
        }
        else if(that.m_size > 0)
        {
            if(that.m_size == m_size)
            {
                copy_assign_n(m_ptr, that.m_ptr, that.m_size);
            }
            else if(that.m_size < m_size)
            {
                copy_assign_n(m_ptr, that.m_ptr, that.m_size);
                m_allocator.destroy_n(m_ptr+that.m_size, m_size - that.m_size);
            }
            else if(that.m_size > m_size)
            {
                reserve(that.m_size);
                copy_assign_n(m_ptr, that.m_ptr, m_size);
                copy_construct_n(m_ptr+m_size, that.m_ptr, that.m_size - m_size);
            }
            m_size = that.m_size;
        }
    }
    void assign(contiguous && that)
    {
        if(&that == this) return;
        C4_ASSERT(m_ptr != that.m_ptr);
        clear();
        m_allocator.deallocate(m_ptr, m_capacity, Alignment);
        m_ptr = that.m_ptr;
        m_size = that.m_size;
        m_capacity = that.m_capacity;
        that.m_ptr = nullptr;
        that.m_size = 0;
        that.m_capacity = 0;
    }

    void resize(I sz)
    {
        if(sz == m_size) return;
        if(sz > m_capacity)
        {
            reserve(sz);
        }
        if(sz > m_size)
        {
            m_allocator.construct_n(m_ptr + m_size, sz - m_size);
        }
        else if(sz < m_size)
        {
            m_allocator.destroy_n(m_ptr + sz, m_size - sz);
        }
        m_size = sz;
    }
    void resize(I sz, fastcref<T> value)
    {
        if(sz == m_size) return;
        if(sz > m_capacity)
        {
            reserve(sz);
        }
        if(sz > m_size)
        {
            m_allocator.construct_n(m_ptr + m_size, sz - m_size, value);
        }
        else if(sz < m_size)
        {
            m_allocator.destroy_n(m_ptr + sz, m_size - sz);
        }
        m_size = sz;
    }
    void reserve(I cap)
    {
        _reserve(m_size, cap, m_ptr + m_size);
    }
    void shrink_to_fit()
    {
        if(m_size == m_capacity) return;
        if(m_size == 0)
        {
            _free();
        }
        else
        {
            T *tmp = m_allocator.allocate(m_size, Alignment);
            if(m_ptr)
            {
                move_construct_n(tmp, m_ptr, m_size);
                m_allocator.deallocate(m_ptr, m_capacity, Alignment);
            }
            m_ptr = tmp;
            m_capacity = m_size;
        }
    }

protected:

    void _growto(I sz)
    {
        C4_XASSERT(m_size <= m_capacity);
        if(sz > m_capacity)
        {
            size_t next = growth_type::next_size(m_capacity, sz);
            C4_XASSERT(next < size_t(std::numeric_limits< I >::max()));
            _reserve(sz, (I)next, m_ptr + m_capacity);
        }
        if(sz < m_size)
        {
            C4_NOT_IMPLEMENTED();
        }
    }
    void _growto(I sz, const_iterator before_this)
    {
        C4_XASSERT(m_size <= m_capacity);
        if(sz > m_capacity)
        {
            size_t next = growth_type::next_size(m_capacity, sz);
            C4_XASSERT(next < size_t(std::numeric_limits< I >::max()));
            _reserve(sz, (I)next, before_this);
        }
        else
        {
            if(sz > m_size)
            {
                I num_after = m_ptr+m_size - before_this;
                for(I i = 0; i < num_after; ++i)
                {
                    T* curr = m_ptr + m_size - i;
                    move_construct(curr, curr-1);
                }
            }
            else if(sz < m_size)
            {
                C4_NOT_IMPLEMENTED();
            }
        }
    }
    void _reserve(I sz, I cap, const_iterator before_this)
    {
        if(cap <= m_capacity) return;
        T *tmp = m_allocator.allocate(cap, Alignment);
        if(m_ptr)
        {
            if(sz >= m_size) // add more before_this
            {
                I num_more = sz - m_size; // to construct (later)
                I num_before = before_this - m_ptr; // to move before
                I num_after = m_ptr+m_size - before_this; // to move after
                move_construct_n(tmp, m_ptr, num_before);
                move_construct_n(tmp + num_before + num_more, m_ptr + num_before, num_after);
            }
            else // remove some before_this
            {
                I num_less = m_size - sz; // to remove
                I num_before = before_this-num_less - m_ptr; // to move before
                I num_after = m_ptr+m_size - before_this; // to move after
                m_allocator.destroy_n(m_ptr + num_before, num_less);
                move_construct_n(tmp, m_ptr, num_before);
                move_construct_n(tmp + num_before, m_ptr + num_before + num_less, num_after);
            }
            m_allocator.deallocate(m_ptr, m_capacity, Alignment);
        }
        m_ptr = tmp;
        m_capacity = cap;
    }

    void _free()
    {
        m_allocator.destroy_n(m_ptr, m_size);
        m_allocator.deallocate(m_ptr, m_capacity, Alignment);
        m_size = 0;
        m_ptr = nullptr;
        m_capacity = 0;
    }


    template< class U, class... Args >
    C4_ALWAYS_INLINE void _construct(U *ptr, Args&&... args)
    {
        m_allocator.construct(ptr, std::forward< Args >(args)...);
    }

    template< class U, class... Args >
    C4_ALWAYS_INLINE void _construct_n(U *ptr, Args&&... args)
    {
        m_allocator.construct_n(ptr, std::forward< Args >(args)...);
    }
};


//-----------------------------------------------------------------------------

/** @todo add search algorithm transparency (binary/ternary/golden)*/
template
<
    class T,
    class Compare = std::less< T >,
    class Storage = c4::stg::contiguous< T >
>
class contiguous_sorted : protected Storage
{
public:

    using value_type = T;
    using size_type = typename Storage::size_type;
    using iterator = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;
    using reference = value_type&;
    using const_reference = value_type const&;
    using pointer = value_type*;
    using const_pointer = value_type const*;

public:

    contiguous_sorted() : m_valid(true) {}
    contiguous_sorted(aggregate_t, std::initializer_list< T > il) { Storage::assign(aggregate_t{}, il); }

    using Storage::empty;
    using Storage::size;
    using Storage::capacity;
    using Storage::reserve;
    using Storage::shrink_to_fit;

    using Storage::type_size;
    using Storage::byte_size;

    using Storage::begin;
    using Storage::end;
    using Storage::data;

    using Storage::front;
    using Storage::back;

    using Storage::operator[];

    bool valid() const { return m_valid; }

    template< class L >
    iterator find(L const& v)
    {
        C4_STATIC_ASSERT(m_valid);
        auto it = lower_bound< L >(v);
        if(it == end()) return it;
        if( ! m_compare(*it, v) && ! m_compare(v, *it)) return it;
        return end();
    }
    iterator find(T const& v)
    {
        C4_STATIC_ASSERT(m_valid);
        auto it = lower_bound(v);
        if(it == end()) return it;
        if( ! m_compare(*it, v) && ! m_compare(v, *it)) return it;
        return end();
    }

    template< class L >
    const_iterator find(L const& v) const
    {
        C4_STATIC_ASSERT(m_valid);
        auto it = lower_bound< L >(v);
        if(it == end()) return it;
        if( ! m_compare(*it, v) && ! m_compare(v, *it)) return it;
        return end();
    }
    const_iterator find(T const& v) const
    {
        C4_STATIC_ASSERT(m_valid);
        auto it = lower_bound(v);
        if(it == end()) return it;
        if( ! m_compare(*it, v) && ! m_compare(v, *it)) return it;
        return end();
    }

    // insert into the proper place. order remains valid.

    iterator insert(T const& v)
    {
        if(empty())
        {
            Storage::push_back(v);
            return end() - 1;
        }
        if( ! m_valid) fix();
        auto lb = lower_bound(v);
        auto it = Storage::insert(lb, v);
        return it;
    }
    iterator insert(T && v)
    {
        if(empty())
        {
            Storage::push_back(std::move(v));
            return end() - 1;
        }
        if( ! m_valid) fix();
        auto lb = lower_bound(v);
        auto it = Storage::insert(lb, std::move(v));
        return it;
    }
    template< class... Args >
    C4_ALWAYS_INLINE iterator emplace(Args&&... args)
    {
        if(empty())
        {
            Storage::emplace_back(std::forward< Args >(args)...);
            return end() - 1;
        }
        if( ! m_valid) fix();
        auto lb = lower_bound(v);
        auto it = Storage::emplace(lb, std::forward< Args >(args)...);
        return it;
    }

    // insert at the end. order continues valid if and only if the value is not less than back()

    iterator push_back_nosort(T const& v)
    {
        if( ! empty())
        {
            m_valid = m_compare(v, back());
        }
        this->push_back(v);
        return end() - 1;
    }
    iterator push_back_nosort(T && v)
    {
        if( ! empty())
        {
            m_valid = m_compare(v, back());
        }
        this->push_back(std::move(v));
        return end() - 1;
    }
    template< class... Args >
    C4_ALWAYS_INLINE iterator emplace_back_nosort(Args&&... args)
    {
        return this->push_back_nosort(std::move(T(std::forward< Args >(args)...)));
    }

    // insert before the given position. Order continues valid if and only if the value is not less than pos->first

    iterator insert_nosort(const_iterator pos, T const& v)
    {
        C4_XASSERT(this->is_valid_iterator(pos));
        if( ! empty() && pos != end())
        {
            m_valid = m_compare(v, *pos);
        }
        size_type i = (size_type)(pos - begin());
        Storage::insert(pos, v);
        return begin() + i;
    }
    iterator insert_nosort(const_iterator pos, T && v)
    {
        C4_XASSERT(this->is_valid_iterator(pos));
        if( ! empty() && pos != end())
        {
            m_valid = m_compare(v, *pos);
        }
        size_type i = (size_type)(pos - begin());
        Storage::insert(pos, std::move(v));
        return begin() + i;
    }
    template< class... Args >
    C4_ALWAYS_INLINE iterator emplace_nosort(const_iterator pos, Args&&... args)
    {
        return insert_nosort(pos, std::move(T(std::forward< Args >(args)...)));
    }

    bool fix()
    {
        if(m_valid) return false;
        std::sort(begin(), end(), m_compare);
        m_valid = true;
        return true;
    }


    void assign(T const* mem, size_type sz)
    {
        Storage::assign(mem, sz);
        m_valid = _check(); // O(N) pass
        fix(); // O(N log N) pass
    }
    void assign_nosort(T const* mem, size_type sz)
    {
        Storage::assign(mem, sz);
        m_valid = _check(); // O(N) pass
    }
    void assign_nocheck(T const* mem, size_type sz)
    {
        Storage::assign(mem, sz);
    }
    void assign(aggregate_t a, std::initializer_list< T > il)
    {
        Storage::assign(a, il);
        m_valid = _check(); // O(N) pass
        fix(); // O(N log N) pass
    }
    void assign_nosort(aggregate_t a, std::initializer_list< T > il)
    {
        Storage::assign(a, il);
        m_valid = _check(); // O(N) pass
    }
    void assign_nocheck(aggregate_t a, std::initializer_list< T > il)
    {
        Storage::assign(a, il);
    }

    /** Returns an iterator pointing to the first element that is _NOT LESS THAN_ key.*/
    template< class U >
    C4_ALWAYS_INLINE const_iterator lower_bound(U const& v) const { return std::lower_bound(begin(), end(), v, m_compare); }
    /** Returns an iterator pointing to the first element that is _NOT LESS THAN_ key.*/
    C4_ALWAYS_INLINE const_iterator lower_bound(T const& v) const { return std::lower_bound(begin(), end(), v, m_compare); }

    /** Returns an iterator pointing to the first element that is _NOT LESS THAN_ key.*/
    template< class U >
    C4_ALWAYS_INLINE iterator lower_bound(U const& v) { return std::lower_bound(begin(), end(), v, m_compare); }
    /** Returns an iterator pointing to the first element that is _NOT LESS THAN_ key.*/
    C4_ALWAYS_INLINE iterator lower_bound(T const& v) { return std::lower_bound(begin(), end(), v, m_compare); }

    /** Returns an iterator pointing to the first element that is _NOT MORE THAN_ key.*/
    template< class U >
    C4_ALWAYS_INLINE const_iterator upper_bound(U const& v) const { return std::upper_bound(begin(), end(), v, m_compare); }
    /** Returns an iterator pointing to the first element that is _NOT MORE THAN_ key.*/
    C4_ALWAYS_INLINE const_iterator upper_bound(T const& v) const { return std::upper_bound(begin(), end(), v, m_compare); }

    /** Returns an iterator pointing to the first element that is _NOT MORE THAN_ key.*/
    template< class U >
    C4_ALWAYS_INLINE iterator upper_bound(U const& v) { return std::upper_bound(begin(), end(), v, m_compare); }
    /** Returns an iterator pointing to the first element that is _NOT MORE THAN_ key.*/
    C4_ALWAYS_INLINE iterator upper_bound(T const& v) { return std::upper_bound(begin(), end(), v, m_compare); }

protected:

    bool _check() const
    {
        for(auto itm1 = begin(), it = itm1+1, e = end(); it < e; ++itm1, ++it)
        {
            if( ! m_compare(itm1, it))
            {
                return false;
            }
        }
        return true;
    }

private:

    Compare m_compare;
    bool    m_valid;

};

//-----------------------------------------------------------------------------

// Capacity growth policies

/** Grow by the least possible amount. */
struct growth_least
{
    static size_t next_size(size_t curr, size_t at_least) noexcept
    {
        if(at_least <= curr) return curr;
        return at_least;
    }
};
/** Grow to the double of the current size if it is bigger than at_least;
 * if not, then just to at_least. */
struct growth_pot
{
    static size_t next_size(size_t curr, size_t at_least) noexcept
    {
        if(at_least <= curr) return curr;
        size_t nxt = (curr << 1);
        return nxt > at_least ? nxt : at_least;
    }
};
/** Grow by the Fibonacci ratio if the result is bigger than at_least;
 * if not, then just to at_least. */
struct growth_phi
{
    static size_t next_size(size_t curr, size_t at_least) noexcept
    {
        if(at_least <= curr) return curr;
        size_t nxt = size_t(float(curr) * 1.618f);
        nxt = nxt > 0 ? nxt : 1;
        return nxt > at_least ? nxt : at_least;
    }
};
/** grow another growth policy in fixed chunk sizes. Useful for SIMD buffers. */
template< class Growth, size_t PowerOfTwoChunkSize >
struct growth_by_chunks
{
    C4_STATIC_ASSERT(PowerOfTwoChunkSize > 1);
    C4_STATIC_ASSERT_MSG((PowerOfTwoChunkSize & (PowerOfTwoChunkSize - 1)) == 0, "chunk size must be a power of two");

    constexpr static const size_t chunk_size = PowerOfTwoChunkSize;

    static size_t next_size(size_t curr, size_t at_least) noexcept
    {
        size_t next = Growth::next_size(curr, at_least);
        size_t rem = (next & (PowerOfTwoChunkSize-1));
        next += rem ? PowerOfTwoChunkSize - rem : 0;
        C4_ASSERT((next % PowerOfTwoChunkSize) == 0);
        return next;
    }
};
/** first, powers of 2, then Fibonacci ratio */
struct growth_default
{
    static size_t next_size(size_t curr, size_t at_least) noexcept
    {
        if(at_least <= 1024)
            return growth_pot::next_size(curr, at_least);
        else
            return growth_phi::next_size(curr, at_least);
    }
};

C4_END_NAMESPACE(stg)

//-----------------------------------------------------------------------------
using stg::with_capacity_t;
constexpr const stg::with_capacity_t with_capacity;

using stg::aggregate_t;
constexpr const stg::aggregate_t aggregate;

template< class T, size_t N, class I = uint32_t >
using array = c4::stg::contiguous_fixed_size< T, N, I, alignof(T) >;

template< class T, size_t N, class I = uint32_t >
using static_vector = c4::stg::contiguous_fixed_capacity< T, N, I, alignof(T) >;

template< class T, class I = uint32_t >
using vector = c4::stg::contiguous< T, I, alignof(T), c4::Allocator< T >, c4::stg::growth_default >;

template< class T, size_t N = 16, class I = uint8_t >
using small_vector = c4::stg::contiguous< T, I, alignof(T), c4::SmallAllocator< T, N, alignof(T) >, c4::stg::growth_default >;

template< class T, class Compare = std::less< T >, class Storage = vector< T > >
using sorted_vector = c4::stg::contiguous_sorted< T, Compare, Storage >;

//-----------------------------------------------------------------------------

template< class K, class T, class Compare = std::less< K > >
struct flat_map_compare
{
    using value_type = std::pair<K,T>;

    Compare comp;

    flat_map_compare() : comp() {}
    flat_map_compare(Compare const& c) : comp(c) {}

    // compare value vs value
    C4_ALWAYS_INLINE bool operator() (value_type const& l, value_type const& r) const { return comp(l.first, r.first); }

    // compare key vs value
    template< class L >
    C4_ALWAYS_INLINE bool operator() (L const& k, value_type const& v) const { return comp(k, v.first); }
    C4_ALWAYS_INLINE bool operator() (K const& k, value_type const& v) const { return comp(k, v.first); }

    // compare value vs key
    template< class L >
    C4_ALWAYS_INLINE bool operator() (value_type const& v, L const& k) const { return comp(v.first, k); }
    C4_ALWAYS_INLINE bool operator() (value_type const& v, K const& k) const { return comp(v.first, k); }
};

//-----------------------------------------------------------------------------
/** A map based on a sorted array. Lookup is O(log N) BUT insertion/removal are O(N).
 * This associative container is good for any of the following scenarios:
 *  -frequent lookups with infrequent modifications.
 *  -OR small sizes. Depending on the size, the array's better cache
 *    properties compensate the more expensive linear modification time.
 * Of course, YMMV. */
template
<
    class K,
    class T,
    class Compare = std::less< K >,
    class Storage = c4::stg::contiguous< std::pair< K, T > >
>
class flat_map : protected stg::contiguous_sorted< std::pair< K, T >, flat_map_compare< K, T, Compare >, Storage >
{
    C4_STATIC_ASSERT((std::is_same< std::pair< K, T >, typename Storage::value_type >::value == true));
    using base_type = stg::contiguous_sorted< std::pair< K, T >, flat_map_compare< K, T, Compare >, Storage >;

public:

    using key_type = K;
    using mapped_type = T;
    using size_type      = typename Storage::size_type;
    using allocator_type = typename Storage::allocator_type;
    using value_type     = typename Storage::value_type;
    using iterator       = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;

public:

    using base_type::base_type;

    using base_type::empty;
    using base_type::size;
    using base_type::capacity;
    using base_type::reserve;
    using base_type::shrink_to_fit;

    using base_type::type_size;
    using base_type::byte_size;

    using base_type::data;

    using base_type::begin;
    using base_type::end;

    using base_type::front;
    using base_type::back;

    using base_type::valid;
    using base_type::find;

    using base_type::lower_bound;
    using base_type::upper_bound;

    using base_type::clear;

    using base_type::insert;
    using base_type::emplace;

    using base_type::push_back_nosort;
    using base_type::emplace_back_nosort;

    using base_type::insert_nosort;
    using base_type::emplace_nosort;

    using base_type::fix;

    using base_type::assign;
    using base_type::assign_nosort;
    using base_type::assign_nocheck;

    template< class L >
    T& operator[] (L const& key)
    {
        iterator it = this->template lower_bound< L >(key);
        if(it == this->end() || it->first != key)
        {
            it = this->emplace_nosort(it, key, std::move(T()));
        }
        C4_XASSERT(it->first == key);
        return it->second;
    }
    T& operator[] (K const& key)
    {
        auto it = this->lower_bound(key);
        if(it == this->end() || it->first != key)
        {
            it = this->emplace_nosort(it, key, std::move(T()));
        }
        C4_XASSERT(it->first == key);
        return it->second;
    }

    template< class L >
    fastcref< T > operator[] (L const& key) const
    {
        auto it = this->template find< L >(key);
        C4_XASSERT(it != this->end());
        C4_XASSERT(it->first == key);
        return it->second;
    }
    fastcref< T > operator[] (K const& key) const
    {
        auto it = this->find(key);
        C4_XASSERT(it != this->end());
        C4_XASSERT(it->first == key);
        return it->second;
    }

};

//-----------------------------------------------------------------------------

template< class T, class I >
struct flat_forward_list_node
{
    using value_type = T;
    using size_type = I;
    T val;
    I next;
};
template< class T, class I >
struct flat_list_node
{
    using value_type = T;
    using size_type = I;
    T val;
    I prev;
    I next;
};

//-----------------------------------------------------------------------------

template< class T, class I = uint32_t, class RawStorage = stg::raw< flat_forward_list_node<T,I>, I > >
struct flat_forward_list
{
    C4_STATIC_ASSERT((std::is_same< I, typename RawStorage::size_type >::value));
    C4_STATIC_ASSERT((std::is_same< flat_forward_list_node<T,I>, typename RawStorage::value_type >::value));
public:

    using node_type = flat_forward_list_node< T, I >;
    using value_type = T;
    using size_type = I;
    using storage_type = RawStorage;

public:

    flat_forward_list() : m_storage(), m_size(0), m_head(0), m_tail(0), m_fhead(0)
    {
    }

    I size() const { return m_size; }
    I capacity() const { return m_storage.capacity(); }

    bool empty() const { return m_size == 0; }
    bool full() const { return m_fhead == m_storage.capacity(); }

    void push_front(T const& val)
    {
        auto *n = _push_front();
        n->val = val;
    }
    void push_front(T && val)
    {
        auto *n = _push_front();
        n->val = std::move(val);
    }
    template< class... Args >
    void emplace_front(Args&&... args)
    {
        auto *n = _push_front();
        construct(&n->val, std::forward< Args >(args)...);
    }
    void pop_front()
    {
        C4_XASSERT(m_size > 0);
        auto *c = n(m_head);
        m_head = c->next;
        c->val.~T();
        --m_size;
        C4_NOT_IMPLEMENTED();
    }

    void push_back(T const& val)
    {
        auto *c = _push_back();
        c->val = val;
    }
    void push_back(T && val)
    {
        auto *c = _push_back();
        c->val = std::move(val);
    }
    template< class... Args >
    void emplace_back(Args&&... args)
    {
        auto *c = _push_back();
        construct(&c->val, std::forward< Args >(args)...);
    }
    void pop_back()
    {
        C4_XASSERT(m_size > 1);
        C4_NOT_IMPLEMENTED();
    }

    void clear()
    {
        if(m_size > 0)
        {
            n(m_tail)->next = m_fhead;
            m_fhead = m_head;
            m_head = 0;
            m_tail = 0;
        }
        m_size = 0;
    }

    /** reorders the elements in memory in order of appearance on the list */
    void sort()
    {
        node_type *d = m_storage.data();
        I curr = m_head;
        for(I i = 0; i < m_size; ++i)
        {
            while(curr < i)
            {
                curr = d[curr].next;
            }
            std::swap(d[i], d[curr]);
            std::swap(d[i].next, curr);
        }
        I cap = m_storage.capacity();
        for(I i = 0; i < cap; ++i)
        {
            d[i].next = i+1;
        }
        m_head = 0;
        m_tail = 0;
        if(m_size > 0)
        {
            d[m_size-1].next = cap;
            m_tail = m_size - 1;
        }
        m_fhead = m_storage.capacity();
        m_fhead = m_size < m_fhead ? m_size : m_fhead;
    }

protected:

    C4_ALWAYS_INLINE node_type      * n(I i)       { C4_XASSERT(i >= 0 && i <= m_storage.capacity()); return m_storage.data() + i; }
    C4_ALWAYS_INLINE node_type const* n(I i) const { C4_XASSERT(i >= 0 && i <= m_storage.capacity()); return m_storage.data() + i; }

    node_type* _push_back()
    {
        if(C4_UNLIKELY(full()))
        {
            _growto(m_size + 1);
        }
        node_type* d = m_storage.data();
        node_type* c = d + m_fhead;
        I fhead = c->next;
        c->next = m_storage.capacity();
        if(C4_LIKELY(m_size > 0))
        {
            d[m_tail].next = m_fhead;
        }
        m_tail = m_fhead;
        m_fhead = fhead;
        ++m_size;
        return c;
    }
    node_type* _push_front()
    {
        if(C4_UNLIKELY(full()))
        {
            _growto(m_size + 1);
        }
        node_type* d = m_storage.data();
        node_type* c = d + m_fhead;
        I fhead = c->next;
        if(C4_LIKELY(m_size > 0))
        {
            c->next = m_head;
        }
        else
        {
            c->next = m_storage.capacity();
            m_tail = m_fhead;
        }
        m_head = m_fhead;
        m_fhead = fhead;
        ++m_size;
        return c;
    }
    node_type* _insert_after(I after_this)
    {
        C4_XASSERT(!empty());
        C4_XASSERT(after_this >= 0 && after_this < m_storage.capacity());
        if(C4_UNLIKELY(full()))
        {
            _growto(m_size + 1);
        }
        node_type* d = m_storage.data();
        node_type* c = d + m_fhead;
        I next_fhead = c->next;
        c->next = d[after_this].next;
        d[after_this].next = m_fhead;
        m_tail = (after_this == m_tail) ? m_fhead : m_tail;
        m_fhead = next_fhead;
        ++m_size;
        return c;
    }

    void _growto(I cap)
    {
        if(cap <= m_storage.capacity()) return;
        C4_CHECK_MSG(cap <= m_storage.max_capacity(), "asked %lu, max is %lu", (uint64_t)cap, (uint64_t)m_storage.max_capacity());
        I next_cap = m_storage.next_capacity(cap);
        C4_CHECK_MSG(next_cap >= cap, "could not allocate storage for more elements. next=%lu cap=%lu", (uint64_t)next_cap, (uint64_t)cap);
        _sort_into(m_storage.alloc(next_cap), next_cap);
        C4_ASSERT(m_fhead != m_storage.capacity());
    }

    /** swaps the list into another array, pasting in list order */
    void _sort_into(node_type* dst, I next_cap)
    {
        if(m_size != 0)
        {
            node_type * src = m_storage.data();
            node_type * sc = src + m_head;
            node_type * e  = src + m_storage.capacity();
            node_type * dc = dst;
            I curr = 0;
            while(sc != e)
            {
                move_construct(&dc->val, &sc->val);
                sc = src + sc->next;
                dc->next = 1 + curr++;
                ++dc;
            }
        }
        m_head = 0;
        m_tail = 0;
        if(m_size > 0)
        {
            m_tail = m_size - 1;
            dst[m_tail].next = next_cap;
        }
        m_fhead = m_size;
        m_storage.reset(dst, next_cap);
        for(I i = m_fhead; i < next_cap; ++i)
        {
            (dst + i)->next = i+1;
        }
    }

protected:

    RawStorage m_storage;
    I m_size;
    I m_head;
    I m_tail;
    I m_fhead; // free head: first free element

protected:

    template< class U >
    friend class iterator_impl;

    template< class U >
    class iterator_impl
    {
        flat_forward_list *list;
        I node;

        C4_ALWAYS_INLINE bool _valid() const { return list != nullptr && list->_valid_node(node); }

    public:

        using value_type = typename U::value_type;
        using size_type = typename U::size_type;

        iterator_impl(flat_forward_list *li, I n) : list(li), node(n) {}

        value_type& operator*  () { return list->n(node)->val; }
        value_type* operator-> () { return &list->n(node)->val; }

        iterator_impl& operator++ () { C4_XASSERT(_valid()); node = list->n(node)->next; return *this; }
        iterator_impl& operator++ (int) { C4_XASSERT(_valid()); iterator_impl it = *this; node = list->n(node).next; return it; }

        bool operator== (iterator_impl const& that) const { return list == that.list && node == that.node; }
        bool operator!= (iterator_impl const& that) const { return list != that.list || node != that.node; }

    };

public:

    using iterator = iterator_impl< node_type >;
    using const_iterator = iterator_impl< const node_type >;

    C4_ALWAYS_INLINE bool _valid_node(I node)
    {
        node_type const* c = n(node);
        return c >= m_storage.data() && c < m_storage.data() + m_storage.capacity();
    }

    C4_ALWAYS_INLINE iterator begin() { return iterator(this, m_head); }
    C4_ALWAYS_INLINE iterator end  () { return iterator(this, m_storage.capacity()); }

    C4_ALWAYS_INLINE const_iterator begin() const { return iterator(this, m_head); }
    C4_ALWAYS_INLINE const_iterator end  () const { return iterator(this, m_storage.capacity()); }

};

C4_END_NAMESPACE(c4)

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// THREAD UTILS

#ifdef C4_LOG_THREAD_SAFE

C4_BEGIN_NAMESPACE(c4)

uint32_t thread_number()
{
    static std::mutex mtx;
    {
        std::lock_guard< std::mutex > lock(mtx);
        static flat_map< std::thread::id, uint32_t > ids;
        static uint32_t count = 0;

        auto id = std::this_thread::get_id();
        auto it = ids.find(id);
        if(it == ids.end())
        {
            it = ids.emplace(id, count++);
        }

        return it->second;
    }
}

/** A simple class for implementing thread local vars.
 * @warning It is inefficient for heavy use. */
template< class T >
class ThreadLocal : private c4::flat_map< std::thread::id, T >
{
    typedef c4::flat_map< std::thread::id, T > map_type;
public:

    using map_type::map_type;

    C4_ALWAYS_INLINE operator T& () { return get(); }
    C4_ALWAYS_INLINE T& get()
    {
        std::lock_guard< std::mutex > lock(m_mtx);
        T& obj = (*this)[std::this_thread::get_id()];
        return obj;
    }

private:

    std::mutex m_mtx;

};

#if 0
#   define C4_THREAD_LOCAL(type) c4::ThreadLocal< type >
#else
#   define C4_THREAD_LOCAL(type) thread_local type
#endif

C4_END_NAMESPACE(c4)

#endif //  C4_LOG_THREAD_SAFE

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// LOGGING

C4_BEGIN_NAMESPACE(c4)

// a stringstream output buffer used by the logger
template< class I = uint16_t >
struct LogBuffer
{
    typedef I index_type;

    vector< char, I > buf;
    I pos;

    LogBuffer() : buf(C4_LOG_BUFFER_INITIAL_SIZE), pos(0) {}

    // string to read from
    C4_ALWAYS_INLINE const char* rd() const { return buf.data(); }
    // string to write into
    C4_ALWAYS_INLINE char* wt() { return buf.data() + pos; }
    // remaining size
    C4_ALWAYS_INLINE I rem()
    {
        C4_XASSERT(pos <= buf.size());
        return buf.size() - pos;
    }

    void clear()
    {
        pos = 0;
        buf[0] = '\0';
    }
    void growto(uint16_t sz) // grow by the max of sz and the golden ratio
    {
        float n = 1.618f * float(buf.size());
        assert(size_t(n) < max_idx);
        auto next = I(n);
        next = next > sz ? next : sz;
        buf.resize(next);
    }
    void write(const char *cstr)
    {
        write(cstr, strlen(cstr));
    }
    void write(const char *str, size_t sz)
    {
        assert(sz <= max_idx);
        assert(sz + size_t(pos + 1) < max_idx);
        if(sz+1 > rem()) growto(pos + sz + 1);
        ::strncpy(wt(), str, sz);
        pos += sz;
        buf[pos] = '\0';
    }
    void printf(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        int inum = ::vsnprintf(wt(), rem(), fmt, args);
        I num = abs(inum); // silently skip output errors
        assert(num < max_idx);
        if(num >= rem()) // not enough space?
        {
            va_start(args, fmt);
            growto(pos + num + 1);
            assert(size_t(pos) + size_t(num) + 1 < max_idx);
            inum = ::vsnprintf(wt(), rem(), fmt, args);
            num = abs(inum);
            assert(num < max_idx);
        }
        assert(size_t(pos) + size_t(num) < max_idx);
        pos += num;
        buf[pos] = '\0';
        va_end(args);
    }
    void vprintf(const char *fmt, va_list args)
    {
        va_list args2;
        va_copy(args2, args);
        int inum = ::vsnprintf(wt(), rem(), fmt, args);
        I num = abs(inum); // silently skip output errors
        assert(num < max_idx);
        if(num >= rem()) // not enough space?
        {
            assert(num < max_idx);
            assert(size_t(pos) + size_t(num) + 1 < max_idx);
            growto(buf.size() + num + 1);
            inum = ::vsnprintf(wt(), rem(), fmt, args2);
            num = abs(inum);
        }
        assert(size_t(pos) + size_t(num) + 1 < max_idx);
        pos += num;
        buf[pos] = '\0';
        va_end(args2);
    }

    constexpr static const size_t max_idx = std::numeric_limits< I >::max();
};

//-----------------------------------------------------------------------------
class Log
{
public:

    typedef enum {
        ALWAYS = -10,
        ERR = -2,
        WARN = -1,
        INFO = 0,
        DEBUG = 1,
        TRACE1 = 2,
        TRACE2 = 3,
        TRACE3 = 4,
    } Level_e;

    typedef enum {
        SHOW_TIMESTAMP = 1 << 0,
        TO_TERM = 1 << 1,
        TO_FILE = 1 << 2,
        TO_STR  = 1 << 3,
        DEFAULT_MODE = SHOW_TIMESTAMP|TO_TERM,
    } Mode_e;

    struct Channel
    {
        uint8_t level;
        uint8_t name_len;
        char    name[30];

        Channel() : level{INFO}, name_len{0}, name{0} {}
        Channel(const char *str, Level_e lev)
        {
            level = lev;
            name_len = strlen(str);
            if(name_len > (sizeof(name) - 1)) abort();
            if(name_len > 0)
            {
                snprintf(name, sizeof(name), "%s", str);
            }
            else
            {
                name[name_len] = '\0';
            }
        }
        inline bool cmp(const char *str) const
        {
            if(name_len == 0) return str[0] == '\0';
            return strncmp(name, str, name_len) == 0;
        }
        C4_ALWAYS_INLINE bool skip(Level_e lev) const
        {
            return lev > level && lev != ALWAYS;
        }
    };

    static uint8_t& _mode() { static uint8_t m = DEFAULT_MODE; return m; }
    static uint8_t mode() { return _mode(); }
    static void mode(uint8_t mode_flags) { _mode() = mode_flags; }

    static FILE*& _file() { static FILE *f = nullptr; return f; }
    static FILE* file() { return _file(); }
    static void file(FILE* f) { _file() = f; }

    static LogBuffer< size_t >& strbuf() { static LogBuffer< size_t > b; return b; }
#ifdef C4_LOG_THREAD_SAFE
    static std::mutex& strbuf_mtx() { static std::mutex m; return m; }
#endif

    class StrReader
    {
#ifdef C4_LOG_THREAD_SAFE
        std::lock_guard< std::mutex > l;
        const char* s;
    public:
        C4_ALWAYS_INLINE operator const char* () const { return s; }
        StrReader(std::mutex &m, const char* s_) : l(m, std::adopt_lock_t{}), s(s_) {}
#else
        const char* s;
    public:
        C4_ALWAYS_INLINE operator const char* () const { return s; }
        StrReader(const char* s_) : s(s_) {}
#endif
    };

    static StrReader&& str()
    {
#ifdef C4_LOG_THREAD_SAFE
        auto& m = strbuf_mtx();
        m.lock();
abort();
        return std::move(StrReader(m, (mode() & TO_STR) ? strbuf().rd() : ""));
#else
        return std::move(StrReader((mode() & TO_STR) ? strbuf().rd() : ""));
#endif
    }

    static void str_clear()
    {
        if(mode() & TO_STR)
        {
#ifdef C4_LOG_THREAD_SAFE
            std::lock_guard< std::mutex > lock(strbuf_mtx());
#endif
            strbuf().clear();
        }
    }

    /** return the buffer for this thread */
    static LogBuffer< uint16_t >& buf()
    {
        // using static vars here saves us the need to declare them
        // in a source file, allowing to use this as a drop-in header.
#ifndef C4_LOG_THREAD_SAFE
        static LogBuffer< uint16_t > b;
        return b;
#else // C4_LOG_THREAD_SAFE
        // using a thread-local buffer saves us from locking when formatting
        static C4_THREAD_LOCAL(LogBuffer< uint16_t >) s_buffer;
        return s_buffer;
#endif // C4_LOG_THREAD_SAFE
    }
    static Channel* main_channel()
    {
        return &_channels()[0];
    }
    static Channel* channel(uint8_t i)
    {
        assert(i < _channels().size());
        return &_channels()[i];
    }
    static Channel* channel(const char *name)
    {
        for(auto &ch : _channels())
        {
            if(ch.cmp(name))
            {
                return &ch;
            }
        }
        return nullptr;
    }
    static Channel* add_channel(const char *name, Level_e lev = INFO)
    {
        auto& chs = _channels();
        assert(chs.size() < C4_LOG_MAX_CHANNELS);
        assert(channel(name) == nullptr);
        chs.emplace_back(name, lev);
        return &chs.back();
    }

private:

    static inline c4::vector< Channel >& _channels()
    {
        static c4::vector< Channel > s_channels(1, c4::with_capacity, C4_LOG_MAX_CHANNELS);
        return s_channels;
    }

public:

    /** set the level of all channels */
    static void level(Level_e l)
    {
        for(auto &ch : _channels())
        {
            ch.level = l;
        }
    }

    template< class I >
    static void _print_prefix(Channel const& ch, LogBuffer< I > &buf)
    {
        uint8_t md = mode();
        if((md & SHOW_TIMESTAMP) && (ch.name_len > 0))
        {
            buf.printf("%lfms[%s]: ", exetime()/1.e3, ch.name);
        }
        else if((md & SHOW_TIMESTAMP))
        {
            buf.printf("%lfms: ", exetime()/1.e3, ch.name);
        }
        else if((ch.name_len > 0))
        {
            buf.printf("[%s]: ", ch.name);
        }
    }

    /** print formatted output to the main channel, at INFO level */
    static void printf(const char *fmt, ...)
    {
        Channel &ch = *main_channel();
        if(ch.skip(INFO)) return;
        va_list args;
        va_start(args, fmt);
        auto& b = buf();
        _print_prefix(ch, b);
        b.vprintf(fmt, args);
        pump(b.rd(), b.pos);
        b.clear();
    }
    /** print formatted output to the main channel, at the given level */
    static void printfl(Level_e level, const char *fmt, ...)
    {
        Channel &ch = *main_channel();
        if(ch.skip(level)) return;
        va_list args;
        va_start(args, fmt);
        auto& b = buf();
        _print_prefix(ch, b);
        b.vprintf(fmt, args);
        pump(b.rd(), b.pos);
        b.clear();
    }
    /** print formatted output to the given channel at the given level */
    static void printfcl(Channel *ch, Level_e level, const char *fmt, ...)
    {
        if(ch->skip(level)) return;
        va_list args;
        va_start(args, fmt);
        auto& b = buf();
        _print_prefix(*ch, b);
        b.vprintf(fmt, args);
        pump(b.rd(), b.pos);
        b.clear();
    }

    /** directly print a string to the main channel at INFO level */
    static void write(const char *s) { write(s, strlen(s)); }
    /** directly print a string with specified size to the main channel at INFO level */
    static void write(const char *s, size_t sz)
    {
        Channel &ch = *main_channel();
        if(ch.skip(INFO)) return;
        auto& b = buf();
        _print_prefix(ch, b);
        b.write(s, sz);
        pump(b.rd(), sz);
        b.clear();
    }
    /** directly print a string to the main channel at the given level */
    static void writel(Level_e level, const char *s) { writel(level, s, strlen(s)); }
    /** directly print a string with specified size to the main channel at the given level */
    static void writel(Level_e level, const char *s, size_t sz)
    {
        Channel &ch = *main_channel();
        if(ch.skip(level)) return;
        auto& b = buf();
        _print_prefix(ch, b);
        b.write(s, sz);
        pump(b.rd(), sz);
        b.clear();
    }
    /** directly print a string to the given channel at the given level */
    static void writecl(Channel *ch, Level_e level, const char *s) { writel(level, s, strlen(s)); }
    /** directly print a string with specified size to the given channel at the given level */
    static void writecl(Channel *ch, Level_e level, const char *s, size_t sz)
    {
        if(ch->skip(level)) return;
        auto& b = buf();
        _print_prefix(*ch, b);
        b.write(s, sz);
        pump(b.rd(), sz);
        b.clear();
    }

    static void pump(const char *str, size_t sz)
    {
        uint8_t md = mode();
        if(md & TO_TERM)
        {
#ifndef _MSC_VER
            ::printf("%.*s", (int)sz, str);
#else
            if( ! IsDebuggerPresent())
            {
                ::printf("%.*s", (int)sz, str);
            }
            else
            {
                OutputDebugStrA(str);
            }
#endif
        }
        if(md & TO_FILE)
        {
            if(file() == nullptr) abort();
            fprintf(file(), "%.*s", (int)sz, str);
        }
        if(md & TO_STR)
        {
#ifdef C4_LOG_THREAD_SAFE
            std::lock_guard< std::mutex > lock(strbuf_mtx());
#endif
            strbuf().write(str, sz);
        }
    }

    static void flush()
    {
        uint8_t md = mode();
        if(md & TO_TERM)
        {
#ifndef _MSC_VER
            fflush(stdout);
#else
            if( ! IsDebuggerPresent())
            {
                fflush(stdout);
            }
#endif
        }
        if(md & TO_FILE)
        {
            fflush(file());
        }
    }

    /** A proxy object which buffers prints to a log buffer.
     * It accumulates << calls and outputs once after the last call.
     * The buffer is set to NULL when the channel's log level
     * is incompatible with the given log level. */
    struct Proxy
    {
        Channel const& channel;
        Level_e level;
        LogBuffer< uint16_t >* buf;
        Proxy(Channel const* ch, Level_e lev) : channel(*ch), level(lev), buf(nullptr)
        {
            if(C4_LIKELY(channel.skip(level))) return;
            buf = &Log::buf();
            Log::_print_prefix(channel, *buf);
        }
        ~Proxy()
        {
            if(C4_LIKELY(!buf)) return;
            Log::pump(buf->rd(), buf->pos);
            buf->clear();
        }
        template< typename T >
        void printf(const char *fmt, T const& var) const
        {
            if(C4_LIKELY(!buf)) return;
            buf->printf(fmt, var);
            if(buf->pos > C4_LOG_BUFFER_REF_SIZE)
            {
                Log::pump(buf->rd(), buf->pos);
                buf->clear();
            }
        }
    };
    Proxy operator() (Channel const *ch, Level_e lev) { return Proxy(ch, lev); }
    Proxy operator() (Channel const *ch) { return Proxy(ch, INFO); }
    Proxy operator() (Level_e lev) { return Proxy(&_channels()[0], INFO); }

    /** create a temporary proxy object to handle all the calls to <<.
     * It will accumulate the calls and output once after the last call. */
    template< class T >
    Proxy operator<< (T const& v)
    {
        Proxy s(main_channel(), INFO);
        s << v;
        return s;
    }
};

using LogProxy = const Log::Proxy;

C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, void *      var) { ss.printf("%p",   var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, double      var) { ss.printf("%lg",  var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, float       var) { ss.printf("%g",   var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, char        var) { ss.printf("%c",   var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss,  int64_t    var) { ss.printf("%lld", var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, uint64_t    var) { ss.printf("%llu", var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss,  int32_t    var) { ss.printf("%d",   var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, uint32_t    var) { ss.printf("%u",   var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss,  int16_t    var) { ss.printf("%hd",  var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, uint16_t    var) { ss.printf("%hu",  var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss,  int8_t     var) { ss.printf("%hhd", var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, uint8_t     var) { ss.printf("%hhu", var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss,       char *var) { ss.printf("%s",   var); return ss; }
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, const char *var) { ss.printf("%s",   var); return ss; }
//C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, std::string const& var) { ss.printf(var.c_str(), var.size()); return ss; }
template< size_t N >
C4_ALWAYS_INLINE LogProxy& operator<< (LogProxy& ss, const char (&var)[N]) { ss.printf("%.*s", (int)(N-1), &var[0]); return ss; }

#define c4_log c4::Log()

#define C4_LOG(fmt, ...) C4_LOG_INFO(fmt, ## __VA_ARGS__)
#define C4_LOG_ERR(fmt, ...) c4_log.printfl(c4::Log::ERR, fmt, ## __VA_ARGS__)
#define C4_LOG_WARN(fmt, ...) c4_log.printfl(c4::Log::WARN, fmt, ## __VA_ARGS__)
#define C4_LOG_INFO(fmt, ...) c4_log.printfl(c4::Log::INFO, fmt, ## __VA_ARGS__)
#define C4_LOG_DEBUG(fmt, ...) c4_log.printfl(c4::Log::DEBUG, fmt, ## __VA_ARGS__)
#define C4_LOG_TRACE1(fmt, ...) c4_log.printfl(c4::Log::TRACE1, fmt, ## __VA_ARGS__)
#define C4_LOG_TRACE2(fmt, ...) c4_log.printfl(c4::Log::TRACE2, fmt, ## __VA_ARGS__)
#define C4_LOG_TRACE3(fmt, ...) c4_log.printfl(c4::Log::TRACE3, fmt, ## __VA_ARGS__)

#define C4_LOGC(channel, fmt, ...) C4_LOGC_INFO(channel, fmt, ## __VA_ARGS__)
#define C4_LOGC_ERR(channel, fmt, ...) c4_log.printfcl(channel, c4::Log::ERR, fmt, ## __VA_ARGS__)
#define C4_LOGC_WARN(channel, fmt, ...) c4_log.printfcl(channel, c4::Log::WARN, fmt, ## __VA_ARGS__)
#define C4_LOGC_INFO(channel, fmt, ...) c4_log.printfcl(channel, c4::Log::INFO, fmt, ## __VA_ARGS__)
#define C4_LOGC_DEBUG(channel, fmt, ...) c4_log.printfcl(channel, c4::Log::DEBUG, fmt, ## __VA_ARGS__)
#define C4_LOGC_TRACE1(channel, fmt, ...) c4_log.printfcl(channel, c4::Log::TRACE1, fmt, ## __VA_ARGS__)
#define C4_LOGC_TRACE2(channel, fmt, ...) c4_log.printfcl(channel, c4::Log::TRACE2, fmt, ## __VA_ARGS__)
#define C4_LOGC_TRACE3(channel, fmt, ...) c4_log.printfcl(channel, 4::Log::TRACE3, fmt, ## __VA_ARGS__)


C4_END_NAMESPACE(c4)

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// ERROR REPORTING: implementation

C4_BEGIN_NAMESPACE(c4)

/** Raise an error, and report a printf-formatted message.
 * If an error callback was set, it will be called.
 * @see set_error_callback() */
inline void report_error(const char *file, int line, const char *func, const char *fmt, ...)
{
    char msg[256];
    va_list args;
    va_start(args, fmt);
    int num = vsnprintf(msg, sizeof(msg), fmt, args);
    if(num > 0)
    {
        C4_LOG_ERR("\n%s:%d: ERROR: %s\n", file, line, msg);
    }
    C4_LOG_ERR("\n%s:%d: ERROR: %s\n", file, line, func);
    C4_LOG_ERR("\n%s:%d: ERROR: ABORTING...\n", file, line);
    c4_log.flush();
    auto fn = get_error_callback();
    if(fn)
    {
        fn();
    }
}

C4_END_NAMESPACE(c4)

//------------------------------------------------------------
//------------------------------------------------------------
//------------------------------------------------------------
// ALLOCATIONS - implementation

C4_BEGIN_NAMESPACE(c4)

C4_BEGIN_NAMESPACE(detail)
#ifndef C4_NO_ALLOC_DEFAULTS
inline void free_impl(void *ptr)
{
    ::free(ptr);
}
inline void afree_impl(void *ptr)
{
#if defined(C4_WIN) || defined(C4_XBOX)
    ::_aligned_free(ptr);
#else
    ::free(ptr);
#endif
}
inline void* alloc_impl(size_t size)
{
    void* mem = ::malloc(size);
    C4_CHECK(mem != nullptr || size == 0);
    return mem;
}
inline void* aalloc_impl(size_t size, size_t alignment)
{
    void *mem;
#if defined(C4_WIN) || defined(C4_XBOX)
    mem = ::_aligned_malloc(size, alignment);
    C4_CHECK(mem != nullptr || size == 0);
#elif defined(C4_POSIX)
    // NOTE: alignment needs to be sized in multiples of sizeof(void*)
    size_t amult = alignment;
    if(C4_UNLIKELY(alignment < sizeof(void*)))
    {
        amult = sizeof(void*);
    }
    int ret = ::posix_memalign(&mem, amult, size);
    if(C4_UNLIKELY(ret))
    {
        if(ret == EINVAL)
        {
            C4_ERROR("The alignment argument %lu was not a power of two, "
                     "or was not a multiple of sizeof(void*)",
                     (uint64_t)alignment);
        }
        else if(ret == ENOMEM)
        {
            C4_ERROR("There was insufficient memory to fulfill the "
                     "allocation request of %lu bytes (alignment=%lu)",
                     (uint64_t)size, (uint64_t)size);
        }
        if(mem)
        {
            afree(mem);
        }
        return nullptr;
    }
#else
    C4_NOT_IMPLEMENTED_MSG("need to implement an aligned allocation for this platform");
#endif
    C4_ASSERT_MSG((size_t(mem) & (alignment-1)) == 0, "address %p is not aligned to %lu boundary", mem, (uint64_t)alignment);
    return mem;
}
inline void* realloc_impl(void* ptr, size_t oldsz, size_t newsz)
{
    C4_UNUSED(oldsz);
    void *nptr = ::realloc(ptr, newsz);
    return nptr;
}
inline void* arealloc_impl(void* ptr, size_t oldsz, size_t newsz, size_t alignment)
{
    /** @todo make this more efficient
     * @see http://stackoverflow.com/a/9078627/5875572
     * @see look for qReallocAligned() in http://code.qt.io/cgit/qt/qtbase.git/tree/src/corelib/global/qmalloc.cpp
     */
    void *tmp = aalloc(newsz, alignment);
    size_t min = newsz < oldsz ? newsz : oldsz;
    ::memcpy(tmp, ptr, min);
    afree(ptr);
    return tmp;
}

#endif // C4_NO_ALLOC_DEFAULTS

C4_ALWAYS_INLINE alloc_type& get_alloc()
{
#ifndef C4_NO_ALLOC_DEFAULTS
    static alloc_type fn = &alloc_impl;
#else
    static alloc_type fn = nullptr;
#endif
    return fn;
}
C4_ALWAYS_INLINE aalloc_type& get_aalloc()
{
#ifndef C4_NO_ALLOC_DEFAULTS
    static aalloc_type fn = &aalloc_impl;
#else
    static aalloc_type fn = nullptr;
#endif
    return fn;
}

C4_ALWAYS_INLINE free_type& get_free()
{
#ifndef C4_NO_ALLOC_DEFAULTS
    static free_type fn = &free_impl;
#else
    static free_type fn = nullptr;
#endif
    return fn;
}

C4_ALWAYS_INLINE free_type& get_afree()
{
#ifndef C4_NO_ALLOC_DEFAULTS
    static free_type fn = &afree_impl;
#else
    static free_type fn = nullptr;
#endif
    return fn;
}

C4_ALWAYS_INLINE realloc_type& get_realloc()
{
#ifndef C4_NO_ALLOC_DEFAULTS
    static realloc_type fn = &realloc_impl;
#else
    static realloc_type fn = nullptr;
#endif
    return fn;
}
C4_ALWAYS_INLINE arealloc_type& get_arealloc()
{
#ifndef C4_NO_ALLOC_DEFAULTS
    static arealloc_type fn = &arealloc_impl;
#else
    static arealloc_type fn = nullptr;
#endif
    return fn;
}

C4_END_NAMESPACE(detail)


C4_ALWAYS_INLINE alloc_type get_alloc()
{
    return detail::get_alloc();
}
C4_ALWAYS_INLINE void set_alloc(alloc_type fn)
{
    detail::get_alloc() = fn;
}

C4_ALWAYS_INLINE aalloc_type get_aalloc()
{
    return detail::get_aalloc();
}
C4_ALWAYS_INLINE void set_aalloc(aalloc_type fn)
{
    detail::get_aalloc() = fn;
}

C4_ALWAYS_INLINE free_type get_free()
{
    return detail::get_free();
}
C4_ALWAYS_INLINE void set_free(free_type fn)
{
    detail::get_free() = fn;
}

C4_ALWAYS_INLINE free_type get_afree()
{
    return detail::get_afree();
}
C4_ALWAYS_INLINE void set_afree(free_type fn)
{
    detail::get_afree() = fn;
}

C4_ALWAYS_INLINE realloc_type get_realloc()
{
    return detail::get_realloc();
}
C4_ALWAYS_INLINE void set_realloc(realloc_type fn)
{
    detail::get_realloc() = fn;
}

C4_ALWAYS_INLINE arealloc_type get_arealloc()
{
    return detail::get_arealloc();
}
C4_ALWAYS_INLINE void set_arealloc(arealloc_type fn)
{
    detail::get_arealloc() = fn;
}


inline void* alloc(size_t sz)
{
    C4_ASSERT_MSG(c4::get_alloc() != nullptr, "did you forget to call set_alloc()?");
    auto fn = c4::get_alloc();
    void* ptr = fn(sz);
    return ptr;
}
inline void* aalloc(size_t sz, size_t alignment)
{
    C4_ASSERT_MSG(c4::get_aalloc() != nullptr, "did you forget to call set_aalloc()?");
    auto fn = c4::get_aalloc();
    void* ptr = fn(sz, alignment);
    return ptr;
}
inline void free(void* ptr)
{
    C4_ASSERT_MSG(c4::get_free() != nullptr, "did you forget to call set_free()?");
    auto fn = c4::get_free();
    fn(ptr);
}
inline void afree(void* ptr)
{
    C4_ASSERT_MSG(c4::get_afree() != nullptr, "did you forget to call set_afree()?");
    auto fn = c4::get_afree();
    fn(ptr);
}

inline void* realloc(void *ptr, size_t oldsz, size_t newsz)
{
    C4_ASSERT_MSG(c4::get_realloc() != nullptr, "did you forget to call set_realloc()?");
    auto fn = c4::get_realloc();
    void* nptr = fn(ptr, oldsz, newsz);
    return nptr;
}
inline void* arealloc(void *ptr, size_t oldsz, size_t newsz, size_t alignment)
{
    C4_ASSERT_MSG(c4::get_arealloc() != nullptr, "did you forget to call set_arealloc()?");
    auto fn = c4::get_arealloc();
    void* nptr = fn(ptr, oldsz, newsz, alignment);
    return nptr;
}

C4_END_NAMESPACE(c4)

#ifdef C4_REDEFINE_CPPNEW
#include <new>
void* operator new(size_t size)
{
    return ::c4::alloc(size);
}
void operator delete(void *p) noexcept
{
    ::c4::free(p);
}
void operator delete(void *p, size_t)
{
    ::c4::free(p);
}
void* operator new[](size_t size)
{
    return operator new(size);
}
void operator delete[](void *p) noexcept
{
    operator delete(p);
}
void operator delete[](void *p, size_t)
{
    operator delete(p);
}
void* operator new(size_t size, std::nothrow_t)
{
    return operator new(size);
}
void operator delete(void *p, std::nothrow_t)
{
    operator delete(p);
}
void operator delete(void *p, size_t, std::nothrow_t)
{
    operator delete(p);
}
void* operator new[](size_t size, std::nothrow_t)
{
    return operator new(size);
}
void operator delete[](void *p, std::nothrow_t)
{
    operator delete(p);
}
void operator delete[](void *p, size_t, std::nothrow_t)
{
    operator delete(p);
}
#endif // C4_REDEFINE_CPPNEW


#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif //_C4_UTIL_HPP_
