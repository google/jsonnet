#include "c4/error.hpp"
#include "c4/test.hpp"
#include "c4/libtest/supprwarn_push.hpp"

C4_BEGIN_HIDDEN_NAMESPACE
bool got_an_error = false;
void error_callback(const char *msg, size_t msg_sz)
{
    CHECK_EQ(strncmp(msg, "bla bla", msg_sz), 0);
    CHECK_EQ(msg_sz, 7);
    got_an_error = true;
}
inline c4::ScopedErrorSettings tmp_err()
{
    got_an_error = false;
    return c4::ScopedErrorSettings(c4::ON_ERROR_CALLBACK, error_callback);
}
C4_END_HIDDEN_NAMESPACE

namespace c4 {

TEST_CASE("Error.scoped_callback")
{
    auto orig = get_error_callback();
    {
        auto tmp = tmp_err();
        CHECK_EQ(get_error_callback() == error_callback, true);
        C4_ERROR("bla bla");
        CHECK_EQ(got_an_error, true);
    }
    CHECK_EQ(get_error_callback() == orig, true);
}

} // namespace c4

TEST_CASE("Error.outside_of_c4_namespace")
{
    auto orig = c4::get_error_callback();
    {
        auto tmp = tmp_err();
        CHECK_EQ(c4::get_error_callback() == error_callback, true);
        C4_ERROR("bla bla");
        CHECK_EQ(got_an_error, true);
    }
    CHECK_EQ(c4::get_error_callback() == orig, true);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// WIP: new error handling code


#include <string>    // temporary; just for the exception example
#include <iostream>  // temporary; just for the exception example

namespace c4 {

#define C4_ERR_FMT_BUFFER_SIZE 256

using locref = c4::srcloc const& C4_RESTRICT;

using pfn_err       = void (*)(locref loc, void *data);
using pfn_warn      = void (*)(locref loc, void *data);
using pfn_msg_begin = void (*)(locref loc, void *data);
using pfn_msg_part  = void (*)(const char* msg, size_t size, void *data);
using pfn_msg_end   = void (*)(void *data);

struct ErrorCallbacks
{
    void          *user_data;

    pfn_err        err;
    pfn_warn       warn;
    pfn_msg_begin  msg_begin;
    pfn_msg_part   msg_part;
    pfn_msg_end    msg_end;

    bool msg_enabled() const { return msg_begin != nullptr; }

    template<size_t N>
    void msg(const char (&s)[N])
    {
        msg_part(s, N-1, user_data);
    }
    void msg(const char *msg, size_t sz)
    {
        msg_part(msg, sz, user_data);
    }
    void msg(char c)
    {
        msg_part(&c, 1, user_data);
    }
};


TEST_CASE("ErrorCallbacks.default_obj")
{
    ErrorCallbacks cb {};
    CHECK_EQ(cb.user_data, nullptr);
    CHECK_EQ(cb.err, nullptr);
    CHECK_EQ(cb.warn, nullptr);
    CHECK_EQ(cb.msg_begin, nullptr);
    CHECK_EQ(cb.msg_part, nullptr);
    CHECK_EQ(cb.msg_end, nullptr);
}

template<class ErrBhv>
struct ErrorCallbacksBridgeFull
{
    ErrorCallbacks callbacks() const
    {
        return {
            (ErrBhv*)this,
            ErrorCallbacksBridgeFull<ErrBhv>::on_err,
            ErrorCallbacksBridgeFull<ErrBhv>::on_warn,
            ErrorCallbacksBridgeFull<ErrBhv>::on_msg_begin,
            ErrorCallbacksBridgeFull<ErrBhv>::on_msg_part,
            ErrorCallbacksBridgeFull<ErrBhv>::on_msg_end,
        };
    }
    static void on_err(locref loc, void *data)
    {
        ((ErrBhv*)data)->err(loc);
    }
    static void on_warn(locref loc, void *data)
    {
        ((ErrBhv*)data)->warn(loc);
    }
    static void on_msg_begin(locref loc, void *data)
    {
        ((ErrBhv*)data)->msg_begin(loc);
    }
    static void on_msg_part(const char *part, size_t size, void *data)
    {
        ((ErrBhv*)data)->msg_part(part, size);
    }
    static void on_msg_end(void *data)
    {
        ((ErrBhv*)data)->msg_end();
    }
};

template<class ErrBhv>
struct ErrorCallbacksBridge
{
    ErrorCallbacks callbacks() const
    {
        return {
            (ErrBhv*)this,
            ErrorCallbacksBridge<ErrBhv>::on_err,
            ErrorCallbacksBridge<ErrBhv>::on_warn,
            (pfn_msg_begin)nullptr,
            (pfn_msg_part)nullptr,
            (pfn_msg_end)nullptr
        };
    }
    static void on_err(locref loc, void *data)
    {
        ((ErrBhv*)data)->err(loc);
    }
    static void on_warn(locref loc, void *data)
    {
        ((ErrBhv*)data)->warn(loc);
    }
};


void fputi(int val, FILE *f);

void _errmsg(locref loc)
{
    fputs(loc.file, stderr);
    fputc(':', stderr);
    fputi(loc.line, stderr);
    fputs(": ", stderr);
    fflush(stderr);
}

void _errmsg(const char *part, size_t part_size)
{
    fwrite(part, 1u, part_size, stderr);
    fflush(stderr);
}

/** example implementation using old-style abort */
struct ErrorBehaviorAbort : public ErrorCallbacksBridgeFull<ErrorBehaviorAbort>
{
    static void msg_begin(locref loc)
    {
        fputc('\n', stderr);
        _errmsg(loc);
    }
    static void msg_part(const char *part, size_t part_size)
    {
        _errmsg(part, part_size);
    }
    static void msg_end()
    {
        fputc('\n', stderr);
        fflush(stderr);
    }
    static void err(locref)
    {
        abort();
    }
    static void warn(locref)
    {
        // nothing to do
    }
};


TEST_CASE("ErrorBehaviorAbort.default_obj")
{
    ErrorBehaviorAbort bhv;
    auto cb = bhv.callbacks();
    CHECK_NE(cb.user_data, nullptr);
    CHECK_NE(cb.err, nullptr);
    CHECK_NE(cb.warn, nullptr);
    CHECK_NE(cb.msg_begin, nullptr);
    CHECK_NE(cb.msg_part, nullptr);
    CHECK_NE(cb.msg_end, nullptr);
}


void fputi(int val, FILE *f);
void _append(std::string *s, int line);

/** example implementation using vanilla c++ std::runtime_error */
struct ErrorBehaviorRuntimeError : public ErrorCallbacksBridgeFull<ErrorBehaviorRuntimeError>
{
    std::string exc_msg{};

    void msg_begin(locref loc)
    {
        exc_msg.reserve(strlen(loc.file) + 16);
        exc_msg = '\n';
        exc_msg += loc.file;
        exc_msg += ':';
        _append(&exc_msg, loc.line);
        exc_msg += ": ";
    }
    void msg_part(const char *part, size_t part_size)
    {
        exc_msg.append(part, part_size);
    }
    void msg_end()
    {
        std::cerr << exc_msg << "\n";
    }
    void err(locref)
    {
        throw std::runtime_error(exc_msg);
    }
    void warn(locref)
    {
    }
};



TEST_CASE("ErrorBehaviorRuntimeError.default_obj")
{
    ErrorBehaviorRuntimeError bhv;
    auto cb = bhv.callbacks();
    CHECK_NE(cb.user_data, nullptr);
    CHECK_NE(cb.err, nullptr);
    CHECK_NE(cb.warn, nullptr);
    CHECK_NE(cb.msg_begin, nullptr);
    CHECK_NE(cb.msg_part, nullptr);
    CHECK_NE(cb.msg_end, nullptr);
}




ErrorBehaviorAbort s_err_abort = ErrorBehaviorAbort();
ErrorCallbacks s_err_callbacks = s_err_abort.callbacks();


void new_handle_error(locref loc, size_t msg_size, const char *msg)
{
    if(s_err_callbacks.msg_enabled())
    {
        s_err_callbacks.msg_begin(loc, s_err_callbacks.user_data);
        s_err_callbacks.msg("ERROR: ");
        s_err_callbacks.msg(msg, msg_size);
        s_err_callbacks.msg_end(s_err_callbacks.user_data);
    }
    s_err_callbacks.err(loc, s_err_callbacks.user_data);
}

void new_handle_warning(locref loc, size_t msg_size, const char *msg)
{
    if(s_err_callbacks.msg_enabled())
    {
        s_err_callbacks.msg_begin(loc, s_err_callbacks.user_data);
        s_err_callbacks.msg("WARNING: ");
        s_err_callbacks.msg(msg, msg_size);
        s_err_callbacks.msg_end(s_err_callbacks.user_data);
    }
    s_err_callbacks.warn(loc, s_err_callbacks.user_data);
}

template<size_t N>
C4_ALWAYS_INLINE void new_handle_error(locref loc, const char (&msg)[N])
{
    new_handle_error(loc, N-1, msg);
}

template<size_t N>
C4_ALWAYS_INLINE void new_handle_warning(locref loc, const char (&msg)[N])
{
    new_handle_warning(loc, N-1, msg);
}


#define C4_ERROR_NEW(msg) c4::new_handle_error(C4_SRCLOC(), msg)
#define C4_WARNING_NEW(msg) c4::new_handle_warning(C4_SRCLOC(), msg)

#define C4_ERROR_NEW_SZ(msg, msglen) c4::new_handle_error(C4_SRCLOC(), msglen, msg)
#define C4_WARNING_NEW_SZ(msg, msglen) c4::new_handle_warning(C4_SRCLOC(), msglen, msg)

} // namespace c4


#include <c4/substr.hpp>

#include <c4/charconv.hpp>
namespace c4 {

void fputi(int val, FILE *f)
{
    char buf[16];
    size_t ret = c4::itoa(buf, val);
    ret = ret < sizeof(buf) ? ret : sizeof(buf);
    fwrite(buf, 1u, ret, f);
}

// to avoid using std::to_string()
void _append(std::string *s, int line)
{
    auto sz = s->size();
    s->resize(sz + 16);
    auto ret = itoa(substr(&((*s)[0]) + sz, 16u), line);
    s->resize(sz + ret);
    if(ret >= sz)
    {
        itoa(substr(&((*s)[0]) + sz, 16u), line);
    }
}

} // namespace c4
template<class ErrorBehavior>
struct ScopedErrorBehavior
{
    c4::ErrorCallbacks m_prev;
    ErrorBehavior m_tmp;
    const char *m_name;
    ScopedErrorBehavior(const char* name) : m_prev(c4::s_err_callbacks), m_tmp(), m_name(name)
    {
        c4::s_err_callbacks = m_tmp.callbacks();
    }
    ~ScopedErrorBehavior()
    {
        c4::s_err_callbacks = m_prev;
    }
};
#define C4_TMP_ERR_BHV(bhv_ty) ScopedErrorBehavior<c4::bhv_ty>(#bhv_ty)

template<size_t N>
void test_error_exception(const char (&msg)[N])
{
    INFO(msg);
    {
        auto tmp1 = C4_TMP_ERR_BHV(ErrorBehaviorAbort);

        {
            auto tmp2 = C4_TMP_ERR_BHV(ErrorBehaviorRuntimeError);

            bool got_exc = false;
            try {
                C4_ERROR_NEW(msg);
            }
            catch(std::exception const& e) {
                // check that the error terminates with the given message
                auto what = c4::to_csubstr(e.what()).last(N-1);
                CHECK_EQ(what, msg);
                got_exc = (what == msg);
            }
            CHECK(got_exc);

            got_exc = false;
            try {
                C4_ERROR_NEW_SZ(msg, N-1);
            }
            catch(std::exception const& e) {
                // check that the error terminates with the given message
                auto what = c4::to_csubstr(e.what()).last(N-1);
                CHECK_EQ(what, msg);
                got_exc = (what == msg);
            }
            CHECK(got_exc);
        }
    }
}

template<size_t N>
void test_warning_exception(const char (&msg)[N])
{
    auto tmp = C4_TMP_ERR_BHV(ErrorBehaviorRuntimeError);
    C4_WARNING_NEW(msg);
    auto const& wmsg = tmp.m_tmp.exc_msg;
    REQUIRE_FALSE(wmsg.empty());
    REQUIRE_GT(wmsg.size(), N);
    auto what = c4::to_csubstr(wmsg.c_str()).last(N-1);
    CHECK_EQ(what, msg);

    C4_WARNING_NEW_SZ(msg, N-1);
    REQUIRE_FALSE(wmsg.empty());
    REQUIRE_GT(wmsg.size(), N);
    what = c4::to_csubstr(wmsg.c_str()).last(N-1);
    CHECK_EQ(what, msg);
}

TEST_CASE("error.exception")
{
    test_error_exception("some error with some message");
    test_error_exception("some error with another message");
}

TEST_CASE("warning.exception")
{
    test_warning_exception("some warning");
    test_warning_exception("some other warning");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#include <c4/substr.hpp>
#include <c4/charconv.hpp>

namespace c4 {

template<class T>
void _err_send(T const& arg)
{
    char buf[C4_ERR_FMT_BUFFER_SIZE];
    size_t num = to_chars(buf, arg);
    num = num < C4_ERR_FMT_BUFFER_SIZE ? num : C4_ERR_FMT_BUFFER_SIZE;
    s_err_callbacks.msg_part(buf, num, s_err_callbacks.user_data);
}


size_t _find_fmt(const char *C4_RESTRICT fmt, size_t len)
{
    for(size_t i = 0; i < len; ++i)
    {
        if(fmt[i] != '{')
        {
            continue;
        }
        if(i + 1 == len)
        {
            break;
        }
        if(fmt[i+1] == '}')
        {
            return i;
        }
    }
    return (size_t)-1;
}

void _err_fmt(size_t fmt_size, const char *C4_RESTRICT fmt)
{
    s_err_callbacks.msg_part(fmt, fmt_size, s_err_callbacks.user_data);
}

template<class Arg, class ...Args>
void _err_fmt(size_t fmt_size, const char *C4_RESTRICT fmt, Arg const& C4_RESTRICT arg, Args const& C4_RESTRICT ...args)
{
    size_t pos = _find_fmt(fmt, fmt_size);
    if(pos == (size_t)-1)
    {
        s_err_callbacks.msg_part(fmt, fmt_size, s_err_callbacks.user_data);
        return;
    }
    s_err_callbacks.msg_part(fmt, pos, s_err_callbacks.user_data);
    _err_send(arg);
    pos += 2;
    _err_fmt(fmt_size - pos, fmt + pos, args...);
}

template<class ...Args>
void err_fmt(locref loc, size_t fmt_size, const char *fmt, Args const& C4_RESTRICT ...args)
{
    s_err_callbacks.msg_begin(loc, s_err_callbacks.user_data);
    s_err_callbacks.msg("ERROR: ");
    _err_fmt(fmt_size, fmt, args...);
    s_err_callbacks.msg_end(s_err_callbacks.user_data);
    s_err_callbacks.err(loc, s_err_callbacks.user_data);
}

template<class ...Args>
void warn_fmt(locref loc, size_t fmt_size, const char *fmt, Args const& C4_RESTRICT ...args)
{
    s_err_callbacks.msg_begin(loc, s_err_callbacks.user_data);
    s_err_callbacks.msg("WARNING: ");
    _err_fmt(fmt_size, fmt, args...);
    s_err_callbacks.msg_end(s_err_callbacks.user_data);
    s_err_callbacks.warn(loc, s_err_callbacks.user_data);
}

template<size_t N, class ...Args>
void err_fmt(locref loc, const char (&fmt)[N], Args const& C4_RESTRICT ...args)
{
    err_fmt(loc, N-1, fmt, args...);
}

template<size_t N, class ...Args>
void warn_fmt(locref loc, const char (&fmt)[N], Args const& C4_RESTRICT ...args)
{
    warn_fmt(loc, N-1, fmt, args...);
}

#define C4_ERROR_FMT_NEW(fmt, ...) c4::err_fmt(C4_SRCLOC(), fmt, __VA_ARGS__)
#define C4_WARNING_FMT_NEW(msg, ...) c4::warn_fmt(C4_SRCLOC(), fmt, __VA_ARGS__)

#define C4_ERROR_FMT_NEW_SZ(fmt, fmtlen, ...) c4::err_fmt(C4_SRCLOC(), fmtlen, fmt, __VA_ARGS__)
#define C4_WARNING_FMT_NEW_SZ(fmt, fmtlen, ...) c4::warn_fmt(C4_SRCLOC(), fmtlen, fmt, __VA_ARGS__)

} // namespace c4

template<size_t N, size_t M, class... Args>
void test_error_fmt_exception(const char (&expected)[M], const char (&fmt)[N], Args const& ...args)
{
    INFO(expected);
    {
        auto tmp1 = C4_TMP_ERR_BHV(ErrorBehaviorAbort);

        {
            auto tmp2 = C4_TMP_ERR_BHV(ErrorBehaviorRuntimeError);

            bool got_exc = false;
            try {
                C4_ERROR_FMT_NEW(fmt, args...);
            }
            catch(std::exception const& e) {
                // check that the error terminates with the given message
                auto what = c4::to_csubstr(e.what()).last(M-1);
                CHECK_EQ(what, expected);
                got_exc = (what == expected);
            }
            CHECK(got_exc);

            got_exc = false;
            try {
                C4_ERROR_FMT_NEW_SZ(fmt, N-1, args...);
            }
            catch(std::exception const& e) {
                // check that the error terminates with the given message
                auto what = c4::to_csubstr(e.what()).last(M-1);
                CHECK_EQ(what, expected);
                got_exc = (what == expected);
            }
            CHECK(got_exc);
        }
    }
}

template<size_t N, size_t M, class... Args>
void test_warning_fmt_exception(const char (&expected)[M], const char (&fmt)[N], Args const& ...args)
{
    INFO(expected);

    auto tmp = C4_TMP_ERR_BHV(ErrorBehaviorRuntimeError);
    auto const& wmsg = tmp.m_tmp.exc_msg;
    C4_WARNING_FMT_NEW(fmt, args...);
    REQUIRE_FALSE(wmsg.empty());
    REQUIRE_GT(wmsg.size(), M);
    auto what = c4::to_csubstr(wmsg.c_str()).last(M-1);
    CHECK_EQ(what, expected);

    C4_WARNING_FMT_NEW_SZ(fmt, N-1, args...);
    REQUIRE_FALSE(wmsg.empty());
    REQUIRE_GT(wmsg.size(), M);
    what = c4::to_csubstr(wmsg.c_str()).last(M-1);
    CHECK_EQ(what, expected);
}


TEST_CASE("error.fmt")
{
    test_error_fmt_exception("aaa is 2 is it not?",
                             "{} is {} is it not?", "aaa", 2);
    test_error_fmt_exception("aaa is bbb is it not?",
                             "{} is {} is it not?", "aaa", "bbb");
    test_error_fmt_exception("aaa is {} is it not?",
                             "{} is {} is it not?", "aaa");
    test_error_fmt_exception("aaa is {} is it not?",
                             "{} is {} is it not?", "aaa");
}

TEST_CASE("warning.fmt")
{
    test_warning_fmt_exception("aaa is 2 is it not?",
                               "{} is {} is it not?", "aaa", 2);
    test_warning_fmt_exception("aaa is bbb is it not?",
                               "{} is {} is it not?", "aaa", "bbb");
    test_warning_fmt_exception("aaa is {} is it not?",
                               "{} is {} is it not?", "aaa");
    test_warning_fmt_exception("aaa is {} is it not?",
                               "{} is {} is it not?", "aaa");
}


#include "c4/libtest/supprwarn_pop.hpp"
