#ifndef _C4_ERROR_HPP_
#define _C4_ERROR_HPP_

/** @file error.hpp Facilities for error reporting and runtime assertions. */

/** @defgroup error_checking Error checking */

#include "c4/config.hpp"

#ifdef _DOXYGEN_
    /** if this is defined and exceptions are enabled, then calls to C4_ERROR()
     * will throw an exception
     * @ingroup error_checking */
#   define C4_EXCEPTIONS_ENABLED
    /** if this is defined and exceptions are enabled, then calls to C4_ERROR()
     *  will throw an exception
     * @see C4_EXCEPTIONS_ENABLED
     * @ingroup error_checking */
#   define C4_ERROR_THROWS_EXCEPTION
    /** evaluates to noexcept when C4_ERROR might be called and
     * exceptions are disabled. Otherwise, defaults to nothing.
     * @ingroup error_checking */
#   define C4_NOEXCEPT
#endif // _DOXYGEN_

#if defined(C4_EXCEPTIONS_ENABLED) && defined(C4_ERROR_THROWS_EXCEPTION)
#   define C4_NOEXCEPT
#else
#   define C4_NOEXCEPT noexcept
#endif


//-----------------------------------------------------------------------------

#define C4_ASSERT_SAME_TYPE(ty1, ty2)                       \
    C4_STATIC_ASSERT(std::is_same<ty1 C4_COMMA_X ty2>::value)

#define C4_ASSERT_DIFF_TYPE(ty1, ty2)                       \
    C4_STATIC_ASSERT( ! std::is_same<ty1 C4_COMMA_X ty2>::value)


//-----------------------------------------------------------------------------

#ifdef _DOXYGEN_
/** utility macro that triggers a breakpoint when
 * the debugger is attached and NDEBUG is not defined.
 * @ingroup error_checking */
#   define C4_DEBUG_BREAK()
#endif // _DOXYGEN_


#ifdef NDEBUG
#   define C4_DEBUG_BREAK()
#else
#   ifdef __clang__
#       pragma clang diagnostic push
#       if (__clang_major__ >= 10)
#           pragma clang diagnostic ignored "-Wgnu-inline-cpp-without-extern" // debugbreak/debugbreak.h:50:16: error: 'gnu_inline' attribute without 'extern' in C++ treated as externally available, this changed in Clang 10 [-Werror,-Wgnu-inline-cpp-without-extern]
#       endif
#   elif defined(__GNUC__)
#   endif

#   include <c4/ext/debugbreak/debugbreak.h>
#   define C4_DEBUG_BREAK() if(c4::is_debugger_attached()) { ::debug_break(); }

#   ifdef __clang__
#       pragma clang diagnostic pop
#   elif defined(__GNUC__)
#   endif
#endif

namespace c4 {
bool is_debugger_attached();
} // namespace c4


//-----------------------------------------------------------------------------

#ifdef __clang__
    /* NOTE: using , ## __VA_ARGS__ to deal with zero-args calls to
     * variadic macros is not portable, but works in clang, gcc, msvc, icc.
     * clang requires switching off compiler warnings for pedantic mode.
     * @see http://stackoverflow.com/questions/32047685/variadic-macro-without-arguments */
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments" // warning: token pasting of ',' and __VA_ARGS__ is a GNU extension
#elif defined(__GNUC__)
    /* GCC also issues a warning for zero-args calls to variadic macros.
     * This warning is switched on with -pedantic and apparently there is no
     * easy way to turn it off as with clang. But marking this as a system
     * header works.
     * @see https://gcc.gnu.org/onlinedocs/cpp/System-Headers.html
     * @see http://stackoverflow.com/questions/35587137/ */
#   pragma GCC system_header
#endif


//-----------------------------------------------------------------------------

namespace c4 {

typedef enum : uint32_t {
    /** when an error happens and the debugger is attached, call C4_DEBUG_BREAK().
     * Without effect otherwise. */
    ON_ERROR_DEBUGBREAK = 0x01 << 0,
    /** when an error happens log a message. */
    ON_ERROR_LOG = 0x01 << 1,
    /** when an error happens invoke a callback if it was set with
     * set_error_callback(). */
    ON_ERROR_CALLBACK = 0x01 << 2,
    /** when an error happens call std::terminate(). */
    ON_ERROR_ABORT = 0x01 << 3,
    /** when an error happens and exceptions are enabled throw an exception.
     * Without effect otherwise. */
    ON_ERROR_THROW = 0x01 << 4,
    /** the default flags. */
    ON_ERROR_DEFAULTS = ON_ERROR_DEBUGBREAK|ON_ERROR_LOG|ON_ERROR_CALLBACK|ON_ERROR_ABORT
} ErrorFlags_e;
using error_flags = uint32_t;
void set_error_flags(error_flags f);
error_flags get_error_flags();


using error_callback_type = void (*)(const char* msg, size_t msg_size);
void set_error_callback(error_callback_type cb);
error_callback_type get_error_callback();


//-----------------------------------------------------------------------------
/** RAII class controling the error settings inside a scope. */
struct ScopedErrorSettings
{
    error_flags m_flags;
    error_callback_type m_callback;

    explicit ScopedErrorSettings(error_callback_type cb)
    :   m_flags(get_error_flags()),
        m_callback(get_error_callback())
    {
        set_error_callback(cb);
    }
    explicit ScopedErrorSettings(error_flags flags)
    :   m_flags(get_error_flags()),
        m_callback(get_error_callback())
    {
        set_error_flags(flags);
    }
    explicit ScopedErrorSettings(error_flags flags, error_callback_type cb)
    :   m_flags(get_error_flags()),
        m_callback(get_error_callback())
    {
        set_error_flags(flags);
        set_error_callback(cb);
    }
    ~ScopedErrorSettings()
    {
        set_error_flags(m_flags);
        set_error_callback(m_callback);
    }
};


//-----------------------------------------------------------------------------

/** source location */
struct srcloc;

void handle_error(srcloc s, const char *fmt, ...);
void handle_warning(srcloc s, const char *fmt, ...);

#   define C4_ERROR(msg, ...)                             \
    if(c4::get_error_flags() & c4::ON_ERROR_DEBUGBREAK) { C4_DEBUG_BREAK() } \
    c4::handle_error(C4_SRCLOC(), msg, ## __VA_ARGS__)

#   define C4_WARNING(msg, ...)                                         \
    c4::handle_warning(C4_SRCLOC(), msg, ## __VA_ARGS__)


#if defined(C4_ERROR_SHOWS_FILELINE) && defined(C4_ERROR_SHOWS_FUNC)

struct srcloc
{
    const char *file;
    const char *func;
    int line;

    srcloc() : file(""), func(""), line() {}
    srcloc(const char *f, const char *fn, int l) : file(f), func(fn), line(l) {}
};
#define C4_SRCLOC() c4::srcloc(__FILE__, C4_PRETTY_FUNC, __LINE__)

#elif defined(C4_ERROR_SHOWS_FILELINE)

struct srcloc
{
    const char *file;
    int line;
    srcloc() : file(""), line() {}
    srcloc(const char *f, int l) : file(f), line(l) {}
};
#define C4_SRCLOC() c4::srcloc(__FILE__, __LINE__)

#elif ! defined(C4_ERROR_SHOWS_FUNC)

struct srcloc
{
};
#define C4_SRCLOC() c4::srcloc()

#else
#   error not implemented
#endif


//-----------------------------------------------------------------------------
// assertions

// Doxygen needs this so that only one definition counts
#ifdef _DOXYGEN_
    /** Explicitly enables assertions, independently of NDEBUG status.
     * This is meant to allow enabling assertions even when NDEBUG is defined.
     * Defaults to undefined.
     * @ingroup error_checking */
#   define C4_USE_ASSERT
    /** assert that a condition is true; this is turned off when NDEBUG
     * is defined and C4_USE_ASSERT is not true.
     * @ingroup error_checking  */
#   define C4_ASSERT
    /** same as C4_ASSERT(), additionally prints a printf-formatted message
     * @ingroup error_checking */
#   define C4_ASSERT_MSG
    /** evaluates to C4_NOEXCEPT when C4_XASSERT is disabled; otherwise, defaults
     * to noexcept
     * @ingroup error_checking */
#   define C4_NOEXCEPT_A
#endif // _DOXYGEN_

#ifndef C4_USE_ASSERT
#   ifdef NDEBUG
#       define C4_USE_ASSERT 0
#   else
#       define C4_USE_ASSERT 1
#   endif
#endif

#if C4_USE_ASSERT
#   define C4_ASSERT(cond) C4_CHECK(cond)
#   define C4_ASSERT_MSG(cond, /*fmt, */...) C4_CHECK_MSG(cond, ## __VA_ARGS__)
#   define C4_ASSERT_IF(predicate, cond) if(predicate) { C4_ASSERT(cond); }
#   define C4_NOEXCEPT_A C4_NOEXCEPT
#else
#   define C4_ASSERT(cond)
#   define C4_ASSERT_MSG(cond, /*fmt, */...)
#   define C4_ASSERT_IF(predicate, cond)
#   define C4_NOEXCEPT_A noexcept
#endif


//-----------------------------------------------------------------------------
// extreme assertions

// Doxygen needs this so that only one definition counts
#ifdef _DOXYGEN_
    /** Explicitly enables extreme assertions; this is meant to allow enabling
     * assertions even when NDEBUG is defined. Defaults to undefined.
     * @ingroup error_checking */
#   define C4_USE_XASSERT
    /** extreme assertion: can be switched off independently of
     * the regular assertion; use for example for bounds checking in hot code.
     * Turned on only when C4_USE_XASSERT is defined
     * @ingroup error_checking */
#   define C4_XASSERT
    /** same as C4_XASSERT(), and additionally prints a printf-formatted message
     * @ingroup error_checking */
#   define C4_XASSERT_MSG
    /** evaluates to C4_NOEXCEPT when C4_XASSERT is disabled; otherwise, defaults to noexcept
     * @ingroup error_checking */
#   define C4_NOEXCEPT_X
#endif // _DOXYGEN_

#ifndef C4_USE_XASSERT
#   define C4_USE_XASSERT C4_USE_ASSERT
#endif

#if C4_USE_XASSERT
#   define C4_XASSERT(cond) C4_CHECK(cond)
#   define C4_XASSERT_MSG(cond, /*fmt, */...) C4_CHECK_MSG(cond, ## __VA_ARGS__)
#   define C4_XASSERT_IF(predicate, cond) if(predicate) { C4_XASSERT(cond); }
#   define C4_NOEXCEPT_X C4_NOEXCEPT
#else
#   define C4_XASSERT(cond)
#   define C4_XASSERT_MSG(cond, /*fmt, */...)
#   define C4_XASSERT_IF(predicate, cond)
#   define C4_NOEXCEPT_X noexcept
#endif


//-----------------------------------------------------------------------------
// checks: never switched-off

/** Check that a condition is true, or raise an error when not
 * true. Unlike C4_ASSERT(), this check is not disabled in non-debug
 * builds.
 * @see C4_ASSERT
 * @ingroup error_checking
 *
 * @todo add constexpr-compatible compile-time assert:
 * https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
 */
#define C4_CHECK(cond)                          \
    if(C4_UNLIKELY(!(cond)))                    \
    {                                           \
        C4_ERROR("check failed: %s", #cond);    \
    }

/** like C4_CHECK(), and additionally log a printf-style message.
 * @see C4_CHECK
 * @ingroup error_checking */
#define C4_CHECK_MSG(cond, fmt, ...)                        \
    if(C4_UNLIKELY(!(cond)))                                    \
    {                                                           \
        C4_ERROR("check failed: " #cond "\n" fmt, ## __VA_ARGS__);  \
    }


 //-----------------------------------------------------------------------------
// Common error conditions
#define C4_NOT_IMPLEMENTED() C4_ERROR("NOT IMPLEMENTED")
#define C4_NOT_IMPLEMENTED_MSG(/*msg, */...) C4_ERROR("NOT IMPLEMENTED: " ## __VA_ARGS__)
#define C4_NOT_IMPLEMENTED_IF(condition) if(C4_UNLIKELY(condition)) { C4_ERROR("NOT IMPLEMENTED"); }
#define C4_NOT_IMPLEMENTED_IF_MSG(condition, /*msg, */...) if(C4_UNLIKELY(condition)) { C4_ERROR("NOT IMPLEMENTED: " ## __VA_ARGS__); }

#define C4_NEVER_REACH() C4_UNREACHABLE(); C4_ERROR("never reach this point")
#define C4_NEVER_REACH_MSG(/*msg, */...) C4_UNREACHABLE(); C4_ERROR("never reach this point: " ## __VA_ARGS__)

} // namespace c4

#ifdef __clang__
#   pragma clang diagnostic pop
#endif

#endif /* _C4_ERROR_HPP_ */
