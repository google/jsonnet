#ifndef _C4_CHARCONV_HPP_
#define _C4_CHARCONV_HPP_

/** @file charconv.hpp Lightweight generic type-safe wrappers for
 * converting individual values to/from strings.
 *
 * These are the main functions:
 *
 * @code{.cpp}
 * // Convert the given value, writing into the string.
 * // The resulting string will NOT be null-terminated.
 * // Return the number of characters needed.
 * // This function is safe to call when the string is too small -
 * // no writes will occur beyond the string's last character.
 * template<class T> size_t c4::to_chars(substr buf, T const& C4_RESTRICT val);
 *
 *
 * // Convert the given value to a string using to_chars(), and
 * // return the resulting string, up to and including the last
 * // written character.
 * template<class T> substr c4::to_chars_sub(substr buf, T const& C4_RESTRICT val);
 *
 *
 * // Read a value from the string, which must be
 * // trimmed to the value (ie, no leading/trailing whitespace).
 * // return true if the conversion succeeded.
 * template<class T> bool c4::from_chars(csubstr buf, T * C4_RESTRICT val);
 *
 *
 * // Read the first valid sequence of characters from the string,
 * // skipping leading whitespace, and convert it using from_chars().
 * // Return the number of characters read for converting.
 * template<class T> size_t c4::from_chars_first(csubstr buf, T * C4_RESTRICT val);
 * @endcode
 */

#include "c4/language.hpp"
#include <inttypes.h>
#include <type_traits>
#include <climits>
#include <limits>
#include <utility>

#include "c4/config.hpp"
#include "c4/substr.hpp"
#include "c4/memory_util.hpp"
#include "c4/szconv.hpp"

#ifndef C4CORE_NO_FAST_FLOAT
#   include "c4/ext/fast_float.hpp"
#   define C4CORE_HAVE_FAST_FLOAT 1
#   define C4CORE_HAVE_STD_FROMCHARS 0
#   if (C4_CPP >= 17)
#       if defined(_MSC_VER)
#           if (C4_MSVC_VERSION >= C4_MSVC_VERSION_2019)
#               include <charconv>
#               define C4CORE_HAVE_STD_TOCHARS 1
#           else
#               define C4CORE_HAVE_STD_TOCHARS 0
#           endif
#       else  // VS2017 and lower do not have these macros
#           if __has_include(<charconv>) && __cpp_lib_to_chars
#               define C4CORE_HAVE_STD_TOCHARS 1
#               include <charconv>
#           else
#               define C4CORE_HAVE_STD_TOCHARS 0
#           endif
#       endif
#   else
#       define C4CORE_HAVE_STD_TOCHARS 0
#   endif
#elif (C4_CPP >= 17)
#   if defined(_MSC_VER)
#       if (C4_MSVC_VERSION >= C4_MSVC_VERSION_2019)
#           include <charconv>
#           define C4CORE_HAVE_STD_TOCHARS 1
#           define C4CORE_HAVE_STD_FROMCHARS 1
#       else
#           define C4CORE_HAVE_STD_TOCHARS 0
#           define C4CORE_HAVE_STD_FROMCHARS 0
#       endif
#   else  // VS2017 and lower do not have these macros
#       if __has_include(<charconv>) && __cpp_lib_to_chars
#           define C4CORE_HAVE_STD_TOCHARS 1
#           define C4CORE_HAVE_STD_FROMCHARS 1
#           include <charconv>
#       else
#           define C4CORE_HAVE_STD_TOCHARS 0
#           define C4CORE_HAVE_STD_FROMCHARS 0
#       endif
#   endif
#else
#   define C4CORE_HAVE_STD_TOCHARS 0
#   define C4CORE_HAVE_STD_FROMCHARS 0
#endif


#ifdef _MSC_VER
#   pragma warning(push)
#   if C4_MSVC_VERSION != C4_MSVC_VERSION_2017
#       pragma warning(disable: 4800) //'int': forcing value to bool 'true' or 'false' (performance warning)
#   endif
#   pragma warning(disable: 4996) // snprintf/scanf: this function or variable may be unsafe
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wtautological-constant-out-of-range-compare"
#   pragma clang diagnostic ignored "-Wformat-nonliteral"
#   pragma clang diagnostic ignored "-Wdouble-promotion" // implicit conversion increases floating-point precision
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wformat-nonliteral"
#   pragma GCC diagnostic ignored "-Wdouble-promotion" // implicit conversion increases floating-point precision
#   pragma GCC diagnostic ignored "-Wuseless-cast"
#endif


namespace c4 {

typedef enum : uint8_t {
    /** print the real number in floating point format (like %f) */
    FTOA_FLOAT = 0,
    /** print the real number in scientific format (like %e) */
    FTOA_SCIENT = 1,
    /** print the real number in flexible format (like %g) */
    FTOA_FLEX = 2,
    /** print the real number in hexadecimal format (like %a) */
    FTOA_HEXA = 3,
    _FTOA_COUNT
} RealFormat_e;


inline C4_CONSTEXPR14 char to_c_fmt(RealFormat_e f)
{
    constexpr const char fmt[] = {
        'f',  // FTOA_FLOAT
        'e',  // FTOA_SCIENT
        'g',  // FTOA_FLEX
        'a',  // FTOA_HEXA
    };
    C4_STATIC_ASSERT(C4_COUNTOF(fmt) == _FTOA_COUNT);
    C4_ASSERT(f < _FTOA_COUNT);
    return fmt[f];
}


#if C4CORE_HAVE_STD_TOCHARS
inline constexpr std::chars_format to_std_fmt(RealFormat_e f)
{
    constexpr const std::chars_format fmt[] = {
        std::chars_format::fixed,       // FTOA_FLOAT
        std::chars_format::scientific,  // FTOA_SCIENT
        std::chars_format::general,     // FTOA_FLEX
        std::chars_format::hex,         // FTOA_HEXA
    };
    C4_STATIC_ASSERT(C4_COUNTOF(fmt) == _FTOA_COUNT);
    C4_ASSERT(f < _FTOA_COUNT);
    return fmt[f];
}
#endif // C4CORE_HAVE_STD_TOCHARS

/** in some platforms, int,unsigned int
 *  are not any of int8_t...int64_t and
 *  long,unsigned long are not any of uint8_t...uint64_t */
template<class T>
struct is_fixed_length
{
    enum : bool {
        /** true if T is one of the fixed length signed types */
        value_i = (std::is_integral<T>::value
                   && (std::is_same<T, int8_t>::value
                       || std::is_same<T, int16_t>::value
                       || std::is_same<T, int32_t>::value
                       || std::is_same<T, int64_t>::value)),
        /** true if T is one of the fixed length unsigned types */
        value_u = (std::is_integral<T>::value
                   && (std::is_same<T, uint8_t>::value
                       || std::is_same<T, uint16_t>::value
                       || std::is_same<T, uint32_t>::value
                       || std::is_same<T, uint64_t>::value)),
        /** true if T is one of the fixed length signed or unsigned types */
        value = value_i || value_u
    };
};


// generic versions
template<class T> bool atox(csubstr s, T *v);
template<class T> size_t xtoa(substr s, T v);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef _MSC_VER
#   pragma warning(push)
#elif defined(__clang__)
#   pragma clang diagnostic push
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wconversion"
#   if __GNUC__ >= 6
#       pragma GCC diagnostic ignored "-Wnull-dereference"
#   endif
#endif

// Helper macros, undefined below

#define _c4append(c) { if(C4_LIKELY(pos < buf.len)) { buf.str[pos++] = static_cast<char>(c); } else { ++pos; } }
#define _c4appendhex(i) { if(C4_LIKELY(pos < buf.len)) { buf.str[pos++] = hexchars[i]; } else { ++pos; } }


namespace detail {
template<class T>
C4_NO_INLINE size_t write_dec_neg(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(v < 0);
    size_t pos = 0;
    do {
        _c4append('0' - v % T(10));
        v /= T(10);
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}
template<class T>
C4_NO_INLINE size_t write_hex_neg(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(v < 0);
    constexpr const char hexchars[] = "0123456789abcdef";
    size_t pos = 0;
    do {
        _c4appendhex(-(v % T(16)));
        v /= 16;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}
template<class T>
C4_NO_INLINE size_t write_oct_neg(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(v < 0);
    size_t pos = 0;
    do {
        _c4append('0' - (v % T(8)));
        v /= 8;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}
template<class T>
C4_NO_INLINE size_t write_bin_neg(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(v < 0);
    size_t pos = 0;
    do {
        _c4append('0' - (v % T(2)));
        v /= 2;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}
} // namespace detail


/** write an integer to a string in decimal format. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @return the number of characters required for the string,
 * even if the string is not long enough for the result.
 * No writes are done past the end of the string. */
template<class T>
size_t write_dec(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_ASSERT(v >= 0);
    size_t pos = 0;
    do {
        _c4append('0' + (v % T(10)));
        v /= T(10);
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}


/** write an integer to a string in hexadecimal format. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @return the number of characters required for the string,
 * even if the string is not long enough for the result.
 * No writes are done past the end of the string. */
template<class T>
size_t write_hex(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_ASSERT(v >= 0);
    constexpr const char hexchars[] = "0123456789abcdef";
    size_t pos = 0;
    do {
        _c4appendhex(v & T(15));
        v >>= 4;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}

/** write an integer to a string in octal format. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @note does not prefix with 0o
 * @return the number of characters required for the string,
 * even if the string is not long enough for the result.
 * No writes are done past the end of the string. */
template<class T>
size_t write_oct(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_ASSERT(v >= 0);
    size_t pos = 0;
    do {
        _c4append('0' + (v & T(7)));
        v >>= 3;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}

/** write an integer to a string in binary format. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @note does not prefix with 0b
 * @return the number of characters required for the string,
 * even if the string is not long enough for the result.
 * No writes are done past the end of the string. */
template<class T>
size_t write_bin(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_ASSERT(v >= 0);
    size_t pos = 0;
    do {
        _c4append('0' + (v & T(1)));
        v >>= 1;
    } while(v);
    buf.reverse_range(0, pos <= buf.len ? pos : buf.len);
    return pos;
}


namespace detail {
template<class U> using NumberWriter = size_t (*)(substr, U);
template<class T>
size_t write_num_digits(NumberWriter<T> writer, substr buf, T v, size_t num_digits)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    size_t ret = writer(buf, v);
    if(ret >= num_digits)
    {
        return ret;
    }
    else if(ret >= buf.len || num_digits > buf.len)
    {
        return num_digits;
    }
    C4_ASSERT(num_digits >= ret);
    size_t delta = static_cast<size_t>(num_digits - ret);
    memmove(buf.str + delta, buf.str, ret);
    memset(buf.str, '0', delta);
    return num_digits;
}
} // namespace detail


/** same as c4::write_dec(), but pad with zeroes on the left
 * such that the resulting string is @p num_digits wide.
 * If the given number is wider than num_digits, then the number prevails. */
template<class T>
size_t write_dec(substr buf, T val, size_t num_digits)
{
    return detail::write_num_digits<T>(&write_dec<T>, buf, val, num_digits);
}

/** same as c4::write_hex(), but pad with zeroes on the left
 * such that the resulting string is @p num_digits wide.
 * If the given number is wider than num_digits, then the number prevails. */
template<class T>
size_t write_hex(substr buf, T val, size_t num_digits)
{
    return detail::write_num_digits<T>(&write_hex<T>, buf, val, num_digits);
}

/** same as c4::write_bin(), but pad with zeroes on the left
 * such that the resulting string is @p num_digits wide.
 * If the given number is wider than num_digits, then the number prevails. */
template<class T>
size_t write_bin(substr buf, T val, size_t num_digits)
{
    return detail::write_num_digits<T>(&write_bin<T>, buf, val, num_digits);
}

/** same as c4::write_oct(), but pad with zeroes on the left
 * such that the resulting string is @p num_digits wide.
 * If the given number is wider than num_digits, then the number prevails. */
template<class T>
size_t write_oct(substr buf, T val, size_t num_digits)
{
    return detail::write_num_digits<T>(&write_oct<T>, buf, val, num_digits);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** read a decimal integer from a string. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @note The string must be trimmed. Whitespace is not accepted.
 * @return true if the conversion was successful */
template<class I>
C4_ALWAYS_INLINE bool read_dec(csubstr s, I *C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<I>::value);
    *v = 0;
    for(char c : s)
    {
        if(C4_UNLIKELY(c < '0' || c > '9'))
        {
            return false;
        }
        *v = (*v) * I(10) + (I(c) - I('0'));
    }
    return true;
}

/** read an hexadecimal integer from a string. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @note does not accept leading 0x or 0X
 * @note the string must be trimmed. Whitespace is not accepted.
 * @return true if the conversion was successful */
template<class I>
C4_ALWAYS_INLINE bool read_hex(csubstr s, I *C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<I>::value);
    *v = 0;
    for(char c : s)
    {
        I cv;
        if(c >= '0' && c <= '9')
        {
            cv = I(c) - I('0');
        }
        else if(c >= 'a' && c <= 'f')
        {
            cv = I(10) + (I(c) - I('a'));
        }
        else if(c >= 'A' && c <= 'F')
        {
            cv = I(10) + (I(c) - I('A'));
        }
        else
        {
            return false;
        }
        *v = (*v) * I(16) + cv;
    }
    return true;
}

/** read a binary integer from a string. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @note does not accept leading 0b or 0B
 * @note the string must be trimmed. Whitespace is not accepted.
 * @return true if the conversion was successful */
template<class I>
C4_ALWAYS_INLINE bool read_bin(csubstr s, I *C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<I>::value);
    *v = 0;
    for(char c : s)
    {
        *v <<= 1;
        if(c == '1')
        {
            *v |= 1;
        }
        else if(c == '0')
        {
            ;
        }
        else
        {
            return false;
        }
    }
    return true;
}

/** read an octal integer from a string. This is the
 * lowest level (and the fastest) function to do this task.
 * @note does not accept negative numbers
 * @note does not accept leading 0o or 0O
 * @note the string must be trimmed. Whitespace is not accepted.
 * @return true if the conversion was successful */
template<class I>
C4_ALWAYS_INLINE bool read_oct(csubstr s, I *C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<I>::value);
    *v = 0;
    for(char c : s)
    {
        if(C4_UNLIKELY(c < '0' || c > '7'))
        {
            return false;
        }
        *v = (*v) * I(8) + (I(c) - I('0'));
    }
    return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace detail {
template<class T>
C4_NO_INLINE size_t itoa_neg(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(v < 0);
    if(buf.len > 0)
    {
        buf.str[0] = '-';
        return size_t(1) + detail::write_dec_neg(buf.sub(1), v);
    }
    return size_t(1) + detail::write_dec_neg({}, v);
}

template<class T>
C4_NO_INLINE size_t itoa_neg(substr buf, T v, T radix)
{
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    C4_ASSERT(v < 0);
    size_t pos = 0;
    _c4append('-');
    switch(radix)
    {
    case 10:                                 return pos + detail::write_dec_neg(pos < buf.len ? buf.sub(pos) : substr(), v);
    case 16: _c4append('0'); _c4append('x'); return pos + detail::write_hex_neg(pos < buf.len ? buf.sub(pos) : substr(), v);
    case 2 : _c4append('0'); _c4append('b'); return pos + detail::write_bin_neg(pos < buf.len ? buf.sub(pos) : substr(), v);
    case 8 : _c4append('0'); _c4append('o'); return pos + detail::write_oct_neg(pos < buf.len ? buf.sub(pos) : substr(), v);
    }
    C4_UNREACHABLE();
    return substr::npos;
}

template<class T>
C4_NO_INLINE size_t itoa_neg(substr buf, T v, T radix, size_t num_digits)
{
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    C4_ASSERT(v < 0);
    size_t pos = 0;
    _c4append('-');
    switch(radix)
    {
    case 10:                                 return pos + detail::write_num_digits<T>(&detail::write_dec_neg<T>, pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    case 16: _c4append('0'); _c4append('x'); return pos + detail::write_num_digits<T>(&detail::write_hex_neg<T>, pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    case 2 : _c4append('0'); _c4append('b'); return pos + detail::write_num_digits<T>(&detail::write_bin_neg<T>, pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    case 8 : _c4append('0'); _c4append('o'); return pos + detail::write_num_digits<T>(&detail::write_oct_neg<T>, pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    }
    C4_UNREACHABLE();
    return csubstr::npos;
}
} // namespace detail


/** convert an integral signed decimal to a string.
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient */
template<class T>
size_t itoa(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    if(v >= 0)
    {
        return write_dec(buf, v);
    }
    else if(C4_LIKELY(v != std::numeric_limits<T>::min()))
    {
        if(buf.len > 0)
        {
            buf.str[0] = '-';
            return size_t(1) + write_dec(buf.sub(1), -v);
        }
        return size_t(1) + write_dec({}, -v);
    }
    // when T is the min value (eg i8: -128), negating it
    // will overflow
    return detail::itoa_neg(buf, v);
}

/** convert an integral signed integer to a string, using a specific
 * radix. The radix must be 2, 8, 10 or 16.
 *
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient */
template<class T>
size_t itoa(substr buf, T v, T radix)
{
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    // when T is the min value (eg i8: -128), negating it
    // will overflow
    if(C4_LIKELY(v != std::numeric_limits<T>::min()))
    {
        size_t pos = 0;
        if(v < 0)
        {
            v = -v;
            _c4append('-');
        }
        switch(radix)
        {
        case 10:                                 return pos + write_dec(pos < buf.len ? buf.sub(pos) : substr(), v);
        case 16: _c4append('0'); _c4append('x'); return pos + write_hex(pos < buf.len ? buf.sub(pos) : substr(), v);
        case 2 : _c4append('0'); _c4append('b'); return pos + write_bin(pos < buf.len ? buf.sub(pos) : substr(), v);
        case 8 : _c4append('0'); _c4append('o'); return pos + write_oct(pos < buf.len ? buf.sub(pos) : substr(), v);
        }
    }
    // when T is the min value (eg i8: -128), negating it
    // will overflow
    return detail::itoa_neg<T>(buf, v, radix);
}


/** same as c4::itoa(), but pad with zeroes on the left such that the
 * resulting string is @p num_digits wide. The @p radix must be 2,
 * 8, 10 or 16.  The resulting string is NOT zero-terminated.  Writing
 * stops at the buffer's end.
 *
 * @return the number of characters needed for the result, even if
 * the buffer size is insufficient */
template<class T>
size_t itoa(substr buf, T v, T radix, size_t num_digits)
{
    C4_STATIC_ASSERT(std::is_signed<T>::value);
    C4_ASSERT(radix == 2 || radix == 8 || radix == 10 || radix == 16);
    // when T is the min value (eg i8: -128), negating it
    // will overflow
    if(C4_LIKELY(v != std::numeric_limits<T>::min()))
    {
        size_t pos = 0;
        if(v < 0)
        {
            v = -v;
            _c4append('-');
        }
        switch(radix)
        {
        case 10:                                 return pos + write_dec(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
        case 16: _c4append('0'); _c4append('x'); return pos + write_hex(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
        case 2 : _c4append('0'); _c4append('b'); return pos + write_bin(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
        case 8 : _c4append('0'); _c4append('o'); return pos + write_oct(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
        }
    }
    return detail::itoa_neg(buf, v, radix, num_digits);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** convert an integral unsigned decimal to a string.
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient */
template<class T>
size_t utoa(substr buf, T v)
{
    C4_STATIC_ASSERT(std::is_unsigned<T>::value);
    return write_dec(buf, v);
}

/** convert an integral unsigned integer to a string, using a specific radix. The radix must be 2, 8, 10 or 16.
 * The resulting string is NOT zero-terminated.
 * Writing stops at the buffer's end.
 * @return the number of characters needed for the result, even if the buffer size is insufficient */
template<class T>
size_t utoa(substr buf, T v, T radix)
{
    C4_STATIC_ASSERT(std::is_unsigned<T>::value);
    C4_ASSERT(radix == 10 || radix == 16 || radix == 2 || radix == 8);
    size_t pos = 0;
    switch(radix)
    {
    case 10:                                 return pos + write_dec(pos < buf.len ? buf.sub(pos) : substr(), v);
    case 16: _c4append('0'); _c4append('x'); return pos + write_hex(pos < buf.len ? buf.sub(pos) : substr(), v);
    case 2 : _c4append('0'); _c4append('b'); return pos + write_bin(pos < buf.len ? buf.sub(pos) : substr(), v);
    case 8 : _c4append('0'); _c4append('o'); return pos + write_oct(pos < buf.len ? buf.sub(pos) : substr(), v);
    }
    C4_UNREACHABLE();
    return substr::npos;
}

/** same as c4::utoa(), but pad with zeroes on the left such that the
 * resulting string is @p num_digits wide. The @p radix must be 2,
 * 8, 10 or 16.  The resulting string is NOT zero-terminated.  Writing
 * stops at the buffer's end.
 *
 * @return the number of characters needed for the result, even if
 * the buffer size is insufficient */
template<class T>
size_t utoa(substr buf, T v, T radix, size_t num_digits)
{
    C4_STATIC_ASSERT(std::is_unsigned<T>::value);
    C4_ASSERT(radix == 10 || radix == 16 || radix == 2 || radix == 8);
    size_t pos = 0;
    switch(radix)
    {
    case 10:                                 return pos + write_dec(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    case 16: _c4append('0'); _c4append('x'); return pos + write_hex(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    case 2 : _c4append('0'); _c4append('b'); return pos + write_bin(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    case 8 : _c4append('0'); _c4append('o'); return pos + write_oct(pos < buf.len ? buf.sub(pos) : substr(), v, num_digits);
    }
    C4_UNREACHABLE();
    return substr::npos;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** Convert a trimmed string to a signed integral value. The string
 * can be formatted as decimal, binary (prefix 0b or 0B), octal
 * (prefix 0o or 0O) or hexadecimal (prefix 0x or 0X). Strings with
 * leading zeroes are considered as decimal. Every character in the
 * input string is read for the conversion; it must not contain any
 * leading or trailing whitespace.
 *
 * @return true if the conversion was successful.
 *
 * @note overflow is not detected: the return status is true even if
 * the conversion would return a value outside of the type's range, in
 * which case the result will wrap around the type's range. This is similar to, just like the native.
 *
 * @see atoi_first() if the string is not trimmed to the value to read. */
template<class T>
bool atoi(csubstr str, T * C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);
    C4_STATIC_ASSERT(std::is_signed<T>::value);

    if(C4_UNLIKELY(str.len == 0))
    {
        return false;
    }

    T sign = 1;
    size_t start = 0;
    if(str.str[0] == '-')
    {
        if(C4_UNLIKELY(str.len == 1))
        {
            return false;
        }
        ++start;
        sign = -1;
    }

    if(str.str[start] != '0')
    {
        if(C4_UNLIKELY( ! read_dec(str.sub(start), v)))
        {
            return false;
        }
    }
    else
    {
        if(str.len == start+1)
        {
            *v = 0; // because the first character is 0
            return true;
        }
        else
        {
            char pfx = str.str[start+1];
            if(pfx == 'x' || pfx == 'X') // hexadecimal
            {
                if(C4_UNLIKELY(str.len <= start + 2))
                {
                    return false;
                }
                if(C4_UNLIKELY( ! read_hex(str.sub(start + 2), v)))
                {
                    return false;
                }
            }
            else if(pfx == 'b' || pfx == 'B') // binary
            {
                if(C4_UNLIKELY(str.len <= start + 2))
                {
                    return false;
                }
                if(C4_UNLIKELY( ! read_bin(str.sub(start + 2), v)))
                {
                    return false;
                }
            }
            else if(pfx == 'o' || pfx == 'O') // octal
            {
                if(C4_UNLIKELY(str.len <= start + 2))
                {
                    return false;
                }
                if(C4_UNLIKELY( ! read_oct(str.sub(start + 2), v)))
                {
                    return false;
                }
            }
            else
            {
                // we know the first character is 0
                auto fno = str.first_not_of('0', start + 1);
                if(fno == csubstr::npos)
                {
                    *v = 0;
                    return true;
                }
                if(C4_UNLIKELY( ! read_dec(str.sub(fno), v)))
                {
                    return false;
                }
            }
        }
    }
    *v *= sign;
    return true;
}


/** Select the next range of characters in the string that can be parsed
 * as a signed integral value, and convert it using atoi(). Leading
 * whitespace (space, newline, tabs) is skipped.
 * @return the number of characters read for conversion, or csubstr::npos if the conversion failed
 * @see atoi() if the string is already trimmed to the value to read.
 * @see csubstr::first_int_span() */
template<class T>
inline size_t atoi_first(csubstr str, T * C4_RESTRICT v)
{
    csubstr trimmed = str.first_int_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atoi(trimmed, v)) return static_cast<size_t>(trimmed.end() - str.begin());
    return csubstr::npos;
}


//-----------------------------------------------------------------------------

/** Convert a trimmed string to an unsigned integral value. The string can be
 * formatted as decimal, binary (prefix 0b or 0B), octal (prefix 0o or 0O)
 * or hexadecimal (prefix 0x or 0X). Every character in the input string is read
 * for the conversion; it must not contain any leading or trailing whitespace.
 *
 * @return true if the conversion was successful.
 *
 * @note overflow is not detected: the return status is true even if
 * the conversion would return a value outside of the type's range, in
 * which case the result will wrap around the type's range.
 *
 * @note If the string has a minus character, the return status
 * will be false.
 *
 * @see atou_first() if the string is not trimmed to the value to read. */
template<class T>
bool atou(csubstr str, T * C4_RESTRICT v)
{
    C4_STATIC_ASSERT(std::is_integral<T>::value);

    if(C4_UNLIKELY(str.len == 0 || str.front() == '-'))
    {
        return false;
    }

    if(str.str[0] != '0')
    {
        if(C4_UNLIKELY( ! read_dec(str, v)))
        {
            return false;
        }
    }
    else
    {
        if(str.len == 1)
        {
            *v = 0; // we know the first character is 0
            return true;
        }
        else
        {
            char pfx = str.str[1];
            if(pfx == 'x' || pfx == 'X') // hexadecimal
            {
                if(C4_UNLIKELY(str.len <= 2))
                {
                    return false;
                }
                return read_hex(str.sub(2), v);
            }
            else if(pfx == 'b' || pfx == 'B') // binary
            {
                if(C4_UNLIKELY(str.len <= 2))
                {
                    return false;
                }
                return read_bin(str.sub(2), v);
            }
            else if(pfx == 'o' || pfx == 'O') // octal
            {
                if(C4_UNLIKELY(str.len <= 2))
                {
                    return false;
                }
                return read_oct(str.sub(2), v);
            }
            else
            {
                // we know the first character is 0
                auto fno = str.first_not_of('0');
                if(fno == csubstr::npos)
                {
                    *v = 0;
                    return true;
                }
                return read_dec(str.sub(fno), v);
            }
        }
    }
    return true;
}


/** Select the next range of characters in the string that can be parsed
 * as an unsigned integral value, and convert it using atou(). Leading
 * whitespace (space, newline, tabs) is skipped.
 * @return the number of characters read for conversion, or csubstr::npos if the conversion faileds
 * @see atou() if the string is already trimmed to the value to read.
 * @see csubstr::first_uint_span() */
template<class T>
inline size_t atou_first(csubstr str, T *v)
{
    csubstr trimmed = str.first_uint_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atou(trimmed, v)) return static_cast<size_t>(trimmed.end() - str.begin());
    return csubstr::npos;
}


#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif


//-----------------------------------------------------------------------------

namespace detail {


/** @see http://www.exploringbinary.com/ for many good examples on float-str conversion */
template<size_t N>
void get_real_format_str(char (& C4_RESTRICT fmt)[N], int precision, RealFormat_e formatting, const char* length_modifier="")
{
    int iret;
    if(precision == -1)
    {
        iret = snprintf(fmt, sizeof(fmt), "%%%s%c", length_modifier, to_c_fmt(formatting));
    }
    else if(precision == 0)
    {
        iret = snprintf(fmt, sizeof(fmt), "%%.%s%c", length_modifier, to_c_fmt(formatting));
    }
    else
    {
        iret = snprintf(fmt, sizeof(fmt), "%%.%d%s%c", precision, length_modifier, to_c_fmt(formatting));
    }
    C4_ASSERT(iret >= 2 && size_t(iret) < sizeof(fmt));
    C4_UNUSED(iret);
}


/** @todo we're depending on snprintf()/sscanf() for converting to/from
 * floating point numbers. Apparently, this increases the binary size
 * by a considerable amount. There are some lightweight printf
 * implementations:
 *
 * @see http://www.sparetimelabs.com/tinyprintf/tinyprintf.php (BSD)
 * @see https://github.com/weiss/c99-snprintf
 * @see https://github.com/nothings/stb/blob/master/stb_sprintf.h
 * @see http://www.exploringbinary.com/
 * @see https://blog.benoitblanchon.fr/lightweight-float-to-string/
 * @see http://www.ryanjuckett.com/programming/printing-floating-point-numbers/
 */
template<class T>
size_t print_one(substr str, const char* full_fmt, T v)
{
#ifdef _MSC_VER
    /** use _snprintf() to prevent early termination of the output
     * for writing the null character at the last position
     * @see https://msdn.microsoft.com/en-us/library/2ts7cx93.aspx */
    int iret = _snprintf(str.str, str.len, full_fmt, v);
    if(iret < 0)
    {
        /* when buf.len is not enough, VS returns a negative value.
         * so call it again with a negative value for getting an
         * actual length of the string */
        iret = snprintf(nullptr, 0, full_fmt, v);
        C4_ASSERT(iret > 0);
    }
    size_t ret = (size_t) iret;
    return ret;
#else
    int iret = snprintf(str.str, str.len, full_fmt, v);
    C4_ASSERT(iret >= 0);
    size_t ret = (size_t) iret;
    if(ret >= str.len)
    {
        ++ret; /* snprintf() reserves the last character to write \0 */
    }
    return ret;
#endif
}

#if !defined(C4CORE_HAVE_STD_TOCHARS) && !defined(C4CORE_HAVE_FAST_FLOAT)
/** scans a string using the given type format, while at the same time
 * allowing non-null-terminated strings AND guaranteeing that the given
 * string length is strictly respected, so that no buffer overflows
 * might occur. */
template<typename T>
inline size_t scan_one(csubstr str, const char *type_fmt, T *v)
{
    /* snscanf() is absolutely needed here as we must be sure that
     * str.len is strictly respected, because substr is
     * generally not null-terminated.
     *
     * Alas, there is no snscanf().
     *
     * So we fake it by using a dynamic format with an explicit
     * field size set to the length of the given span.
     * This trick is taken from:
     * https://stackoverflow.com/a/18368910/5875572 */

    /* this is the actual format we'll use for scanning */
    char fmt[16];

    /* write the length into it. Eg "%12f".
     * Also, get the number of characters read from the string.
     * So the final format ends up as "%12f%n"*/
    int iret = snprintf(fmt, sizeof(fmt), "%%" "%zu" "%s" "%%n", str.len, type_fmt);
    /* no nasty surprises, please! */
    C4_ASSERT(iret >= 0 && size_t(iret) < C4_COUNTOF(fmt));

    /* now we scan with confidence that the span length is respected */
    int num_chars;
    iret = sscanf(str.str, fmt, v, &num_chars);
    /* scanf returns the number of successful conversions */
    if(iret != 1) return csubstr::npos;
    C4_ASSERT(num_chars >= 0);
    return (size_t)(num_chars);
}
#endif


#if C4CORE_HAVE_STD_TOCHARS
template<class T>
size_t rtoa(substr buf, T v, int precision=-1, RealFormat_e formatting=FTOA_FLEX)
{
    std::to_chars_result result;
    size_t pos = 0;
    if(formatting == FTOA_HEXA)
    {
        _c4append('0');
        _c4append('x');
    }
    if(precision == -1)
    {
        result = std::to_chars(buf.str + pos, buf.str + buf.len, v, to_std_fmt(formatting));
    }
    else
    {
        result = std::to_chars(buf.str + pos, buf.str + buf.len, v, to_std_fmt(formatting), precision);
    }
    if(result.ec == std::errc())
    {
        // all good, no errors.
        C4_ASSERT(result.ptr >= buf.str);
        ptrdiff_t delta = result.ptr - buf.str;
        return static_cast<size_t>(delta);
    }
    C4_ASSERT(result.ec == std::errc::value_too_large);
    // This is unfortunate.
    //
    // When the result can't fit in the given buffer,
    // std::to_chars() returns the end pointer it was originally
    // given, which is useless because here we would like to know
    // _exactly_ how many characters the buffer must have to fit
    // the result.
    //
    // So we take the pessimistic view, and assume as many digits
    // as could ever be required:
    size_t ret = static_cast<size_t>(std::numeric_limits<T>::max_digits10);
    return ret > buf.len ? ret : buf.len + 1;
}
#endif // C4CORE_HAVE_STD_TOCHARS

} // namespace detail


#undef _c4appendhex
#undef _c4append


/** Convert a single-precision real number to string.
 * The string will in general be NOT null-terminated.
 * For FTOA_FLEX, \p precision is the number of significand digits. Otherwise
 * \p precision is the number of decimals. */
inline size_t ftoa(substr str, float v, int precision=-1, RealFormat_e formatting=FTOA_FLEX)
{
#if C4CORE_HAVE_STD_TOCHARS
    return detail::rtoa(str, v, precision, formatting);
#else
    char fmt[16];
    detail::get_real_format_str(fmt, precision, formatting, /*length_modifier*/"");
    return detail::print_one(str, fmt, v);
#endif
}


/** Convert a double-precision real number to string.
 * The string will in general be NOT null-terminated.
 * For FTOA_FLEX, \p precision is the number of significand digits. Otherwise
 * \p precision is the number of decimals.
 *
 * @return the number of characters written.
 */
inline size_t dtoa(substr str, double v, int precision=-1, RealFormat_e formatting=FTOA_FLEX)
{
#if C4CORE_HAVE_STD_TOCHARS
    return detail::rtoa(str, v, precision, formatting);
#else
    char fmt[16];
    detail::get_real_format_str(fmt, precision, formatting, /*length_modifier*/"l");
    return detail::print_one(str, fmt, v);
#endif
}


/** Convert a string to a single precision real number.
 * The input string must be trimmed to the value, ie
 * no leading or trailing whitespace can be present.
 * @return true iff the conversion succeeded
 * @see atof_first() if the string is not trimmed
 */
inline bool atof(csubstr str, float * C4_RESTRICT v)
{
    C4_ASSERT(str == str.first_real_span());
#if C4CORE_HAVE_FAST_FLOAT
    fast_float::from_chars_result result;
    result = fast_float::from_chars(str.str, str.str + str.len, *v);
    return result.ec == std::errc();
#elif C4CORE_HAVE_STD_FROMCHARS
    std::from_chars_result result;
    result = std::from_chars(str.str, str.str + str.len, *v);
    return result.ec == std::errc();
#else
    size_t ret = detail::scan_one(str, "f", v);
    return ret != csubstr::npos;
#endif
}


/** Convert a string to a double precision real number.
 * The input string must be trimmed to the value, ie
 * no leading or trailing whitespace can be present.
 * @return true iff the conversion succeeded
 * @see atod_first() if the string is not trimmed
 */
inline bool atod(csubstr str, double * C4_RESTRICT v)
{
    C4_ASSERT(str == str.first_real_span());
#if C4CORE_HAVE_FAST_FLOAT
    fast_float::from_chars_result result;
    result = fast_float::from_chars(str.str, str.str + str.len, *v);
    return result.ec == std::errc();
#elif C4CORE_HAVE_STD_FROMCHARS
    std::from_chars_result result;
    result = std::from_chars(str.str, str.str + str.len, *v);
    return result.ec == std::errc();
#else
    size_t ret = detail::scan_one(str, "lf", v);
    return ret != csubstr::npos;
#endif
}


/** Convert a string to a single precision real number.
 * Leading whitespace is skipped until valid characters are found.
 * @return true iff the conversion succeeded */
inline size_t atof_first(csubstr str, float * C4_RESTRICT v)
{
    csubstr trimmed = str.first_real_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atof(trimmed, v)) return static_cast<size_t>(trimmed.end() - str.begin());
    return csubstr::npos;
}


/** Convert a string to a double precision real number.
 * Leading whitespace is skipped until valid characters are found.
 * @return true iff the conversion succeeded
 */
inline size_t atod_first(csubstr str, double * C4_RESTRICT v)
{
    csubstr trimmed = str.first_real_span();
    if(trimmed.len == 0) return csubstr::npos;
    if(atod(trimmed, v)) return static_cast<size_t>(trimmed.end() - str.begin());
    return csubstr::npos;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// generic versions

C4_ALWAYS_INLINE size_t xtoa(substr s,  uint8_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s, uint16_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s, uint32_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s, uint64_t v) { return utoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,   int8_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,  int16_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,  int32_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,  int64_t v) { return itoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,    float v) { return ftoa(s, v); }
C4_ALWAYS_INLINE size_t xtoa(substr s,   double v) { return dtoa(s, v); }

C4_ALWAYS_INLINE bool atox(csubstr s,  uint8_t *C4_RESTRICT v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s, uint16_t *C4_RESTRICT v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s, uint32_t *C4_RESTRICT v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s, uint64_t *C4_RESTRICT v) { return atou(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,   int8_t *C4_RESTRICT v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,  int16_t *C4_RESTRICT v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,  int32_t *C4_RESTRICT v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,  int64_t *C4_RESTRICT v) { return atoi(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,    float *C4_RESTRICT v) { return atof(s, v); }
C4_ALWAYS_INLINE bool atox(csubstr s,   double *C4_RESTRICT v) { return atod(s, v); }

C4_ALWAYS_INLINE size_t to_chars(substr buf,  uint8_t v) { return utoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf, uint16_t v) { return utoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf, uint32_t v) { return utoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf, uint64_t v) { return utoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf,   int8_t v) { return itoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf,  int16_t v) { return itoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf,  int32_t v) { return itoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf,  int64_t v) { return itoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf,    float v) { return ftoa(buf, v); }
C4_ALWAYS_INLINE size_t to_chars(substr buf,   double v) { return dtoa(buf, v); }

C4_ALWAYS_INLINE bool from_chars(csubstr buf,  uint8_t *C4_RESTRICT v) { return atou(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf, uint16_t *C4_RESTRICT v) { return atou(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf, uint32_t *C4_RESTRICT v) { return atou(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf, uint64_t *C4_RESTRICT v) { return atou(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf,   int8_t *C4_RESTRICT v) { return atoi(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf,  int16_t *C4_RESTRICT v) { return atoi(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf,  int32_t *C4_RESTRICT v) { return atoi(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf,  int64_t *C4_RESTRICT v) { return atoi(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf,    float *C4_RESTRICT v) { return atof(buf, v); }
C4_ALWAYS_INLINE bool from_chars(csubstr buf,   double *C4_RESTRICT v) { return atod(buf, v); }

C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,  uint8_t *C4_RESTRICT v) { return atou_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf, uint16_t *C4_RESTRICT v) { return atou_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf, uint32_t *C4_RESTRICT v) { return atou_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf, uint64_t *C4_RESTRICT v) { return atou_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,   int8_t *C4_RESTRICT v) { return atoi_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,  int16_t *C4_RESTRICT v) { return atoi_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,  int32_t *C4_RESTRICT v) { return atoi_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,  int64_t *C4_RESTRICT v) { return atoi_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,    float *C4_RESTRICT v) { return atof_first(buf, v); }
C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf,   double *C4_RESTRICT v) { return atod_first(buf, v); }


//-----------------------------------------------------------------------------
// on some platforms, (unsigned) int and (unsigned) long
// are not any of the fixed length types above

#define _C4_IF_NOT_FIXED_LENGTH_I(T, ty) C4_ALWAYS_INLINE typename std::enable_if<std::  is_signed<T>::value && !is_fixed_length<T>::value_i, ty>
#define _C4_IF_NOT_FIXED_LENGTH_U(T, ty) C4_ALWAYS_INLINE typename std::enable_if<std::is_unsigned<T>::value && !is_fixed_length<T>::value_i, ty>

template <class T> _C4_IF_NOT_FIXED_LENGTH_I(T, size_t)::type xtoa(substr buf, T v) { return itoa(buf, v); }
template <class T> _C4_IF_NOT_FIXED_LENGTH_U(T, size_t)::type xtoa(substr buf, T v) { return utoa(buf, v); }

template <class T> _C4_IF_NOT_FIXED_LENGTH_I(T, bool  )::type atox(csubstr buf, T *C4_RESTRICT v) { return atoi(buf, v); }
template <class T> _C4_IF_NOT_FIXED_LENGTH_U(T, bool  )::type atox(csubstr buf, T *C4_RESTRICT v) { return atou(buf, v); }

template <class T> _C4_IF_NOT_FIXED_LENGTH_I(T, size_t)::type to_chars(substr buf, T v) { return itoa(buf, v); }
template <class T> _C4_IF_NOT_FIXED_LENGTH_U(T, size_t)::type to_chars(substr buf, T v) { return utoa(buf, v); }

template <class T> _C4_IF_NOT_FIXED_LENGTH_I(T, bool  )::type from_chars(csubstr buf, T *C4_RESTRICT v) { return atoi(buf, v); }
template <class T> _C4_IF_NOT_FIXED_LENGTH_U(T, bool  )::type from_chars(csubstr buf, T *C4_RESTRICT v) { return atou(buf, v); }

template <class T> _C4_IF_NOT_FIXED_LENGTH_I(T, size_t)::type from_chars_first(csubstr buf, T *C4_RESTRICT v) { return atoi_first(buf, v); }
template <class T> _C4_IF_NOT_FIXED_LENGTH_U(T, size_t)::type from_chars_first(csubstr buf, T *C4_RESTRICT v) { return atou_first(buf, v); }

#undef _C4_IF_NOT_FIXED_LENGTH_I
#undef _C4_IF_NOT_FIXED_LENGTH_U


//-----------------------------------------------------------------------------
// for pointers

template <class T> C4_ALWAYS_INLINE size_t xtoa(substr s, T *v) { return xtoa(s, *v); }
template <class T> C4_ALWAYS_INLINE bool   atox(csubstr s, T **v) { return atox(s, *v); }
template <class T> C4_ALWAYS_INLINE size_t to_chars(substr s, T *v) { return to_chars(s, *v); }
template <class T> C4_ALWAYS_INLINE bool   from_chars(csubstr buf, T **v) { return from_chars(buf, *v); }
template <class T> C4_ALWAYS_INLINE size_t from_chars_first(csubstr buf, T **v) { return from_chars_first(buf, *v); }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** call to_chars() and return a substr consisting of the
 * written portion of the input buffer. Ie, same as to_chars(),
 * but return a substr instead of a size_t.
 *
 * @see to_chars() */
template<class T>
inline substr to_chars_sub(substr buf, T const& C4_RESTRICT v)
{
    size_t sz = to_chars(buf, v);
    return buf.left_of(sz <= buf.len ? sz : buf.len);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// bool implementation

inline size_t to_chars(substr buf, bool v)
{
    int val = v;
    return to_chars(buf, val);
}

inline bool from_chars(csubstr buf, bool * C4_RESTRICT v)
{
    if(buf == '0') { *v = false; return true; }
    else if(buf == '1') { *v = true; return true; }
    else if(buf == "false") { *v = false; return true; }
    else if(buf == "true") { *v = true; return true; }
    else if(buf == "False") { *v = false; return true; }
    else if(buf == "True") { *v = true; return true; }
    else if(buf == "FALSE") { *v = false; return true; }
    else if(buf == "TRUE") { *v = true; return true; }
    // fallback to c-style int bools
    int val = 0;
    bool ret = from_chars(buf, &val);
    if(C4_LIKELY(ret))
    {
        *v = (val != 0);
    }
    return ret;
}

inline size_t from_chars_first(csubstr buf, bool * C4_RESTRICT v)
{
    csubstr trimmed = buf.first_non_empty_span();
    if(trimmed.len == 0 || !from_chars(buf, v))
    {
        return csubstr::npos;
    }
    return trimmed.len;
}


//-----------------------------------------------------------------------------
// single-char implementation

inline size_t to_chars(substr buf, char v)
{
    if(buf.len > 0) buf[0] = v;
    return 1;
}

/** extract a single character from a substring
 * @note to extract a string instead and not just a single character, use the csubstr overload */
inline bool from_chars(csubstr buf, char * C4_RESTRICT v)
{
    if(buf.len != 1) return false;
    *v = buf[0];
    return true;
}

inline size_t from_chars_first(csubstr buf, char * C4_RESTRICT v)
{
    if(buf.len < 1) return csubstr::npos;
    *v = buf[0];
    return 1;
}


//-----------------------------------------------------------------------------
// csubstr implementation

inline size_t to_chars(substr buf, csubstr v)
{
    C4_ASSERT(!buf.overlaps(v));
    size_t len = buf.len < v.len ? buf.len : v.len;
    memcpy(buf.str, v.str, len);
    return v.len;
}

inline bool from_chars(csubstr buf, csubstr *C4_RESTRICT v)
{
    *v = buf;
    return true;
}

inline size_t from_chars_first(substr buf, csubstr * C4_RESTRICT v)
{
    csubstr trimmed = buf.first_non_empty_span();
    if(trimmed.len == 0) return csubstr::npos;
    *v = trimmed;
    return static_cast<size_t>(trimmed.end() - buf.begin());
}


//-----------------------------------------------------------------------------
// substr

inline size_t to_chars(substr buf, substr v)
{
    C4_ASSERT(!buf.overlaps(v));
    size_t len = buf.len < v.len ? buf.len : v.len;
    memcpy(buf.str, v.str, len);
    return v.len;
}

inline bool from_chars(csubstr buf, substr * C4_RESTRICT v)
{
    C4_ASSERT(!buf.overlaps(*v));
    if(buf.len <= v->len)
    {
        memcpy(v->str, buf.str, buf.len);
        v->len = buf.len;
        return true;
    }
    memcpy(v->str, buf.str, v->len);
    return false;
}

inline size_t from_chars_first(csubstr buf, substr * C4_RESTRICT v)
{
    csubstr trimmed = buf.first_non_empty_span();
    C4_ASSERT(!trimmed.overlaps(*v));
    if(C4_UNLIKELY(trimmed.len == 0)) return csubstr::npos;
    size_t len = trimmed.len > v->len ? v->len : trimmed.len;
    memcpy(v->str, trimmed.str, len);
    if(C4_UNLIKELY(trimmed.len > v->len)) return csubstr::npos;
    return static_cast<size_t>(trimmed.end() - buf.begin());
}


//-----------------------------------------------------------------------------

template<size_t N>
inline size_t to_chars(substr buf, const char (& C4_RESTRICT v)[N])
{
    csubstr sp(v);
    return to_chars(buf, sp);
}

inline size_t to_chars(substr buf, const char * C4_RESTRICT v)
{
    return to_chars(buf, to_csubstr(v));
}


} // namespace c4

#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

#endif /* _C4_CHARCONV_HPP_ */
