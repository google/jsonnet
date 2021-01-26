#ifndef _C4_SUBSTR_HPP_
#define _C4_SUBSTR_HPP_

/** @file substr.hpp read+write string views */

#include <string.h>
#include <ctype.h>
#include <type_traits>

#include "c4/config.hpp"
#include "c4/error.hpp"
#include "c4/substr_fwd.hpp"

#ifdef __clang__
#   pragma clang diagnostic push
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wtype-limits" // disable warnings on size_t>=0, used heavily in assertions below. These are a preparation step for providing the index type as a template parameter.
#   pragma GCC diagnostic ignored "-Wuseless-cast"
#endif


namespace c4 {


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace detail {

template<typename C>
static inline void _do_reverse(C *C4_RESTRICT first, C *C4_RESTRICT last)
{
    while(last > first)
    {
        C tmp = *last;
        *last-- = *first;
        *first++ = tmp;
    }
}

} // namespace detail


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// utility macros to deuglify SFINAE code; undefined after the class.
// https://stackoverflow.com/questions/43051882/how-to-disable-a-class-member-funrtion-for-certain-template-types
#define C4_REQUIRE_RW(ret_type) \
    template <typename U=C> \
    typename std::enable_if< ! std::is_const<U>::value, ret_type>::type
// non-const-to-const
#define C4_NC2C(ty) \
    typename std::enable_if<std::is_const<C>::value && ( ! std::is_const<ty>::value), ty>::type


/** a non-owning string-view, consisting of a character pointer
 * and a length.
 *
 * @note The pointer is explicitly restricted.
 * @note Because of a C++ limitation, there cannot coexist overloads for
 * constructing from a char[N] and a char*; the latter will always be chosen
 * by the compiler. To construct an object of this type, call to_substr() or
 * to_csubstr(). For a more detailed explanation on why the overloads cannot
 * coexist, see http://cplusplus.bordoon.com/specializeForCharacterArrays.html
 *
 * @see to_substr()
 * @see to_csubstr()
 */
template<class C>
struct C4CORE_EXPORT basic_substring
{
public:

    /** a restricted pointer to the first character of the substring */
    C * C4_RESTRICT str;
    /** the length of the substring */
    size_t          len;

public:

    /** @name Types */
    /** @{ */

    using  CC  = typename std::add_const<C>::type;     //!< CC=const char
    using NCC_ = typename std::remove_const<C>::type; //!< NCC_=non const char

    using ro_substr = basic_substring<CC>;
    using rw_substr = basic_substring<NCC_>;

    using char_type = C;
    using size_type = size_t;

    using iterator = C*;
    using const_iterator = CC*;

    enum : size_t { npos = (size_t)-1, NONE = (size_t)-1 };

    /// convert automatically to substring of const C
    operator ro_substr () const { ro_substr s(str, len); return s; }

    /** @} */

public:

    /** @name Default construction and assignment */
    /** @{ */

    constexpr basic_substring() : str(nullptr), len(0) {}

    constexpr basic_substring(basic_substring const&) = default;
    constexpr basic_substring(basic_substring     &&) = default;
    constexpr basic_substring(std::nullptr_t) : str(nullptr), len(0) {}

    basic_substring& operator= (basic_substring const&) = default;
    basic_substring& operator= (basic_substring     &&) = default;
    basic_substring& operator= (std::nullptr_t) { str = nullptr; len = 0; return *this; }

    /** @} */

public:

    /** @name Construction and assignment from characters with the same type */
    /** @{ */

    //basic_substring(C *s_) : str(s_), len(s_ ? strlen(s_) : 0) {}
    /** the overload for receiving a single C* pointer will always
     * hide the array[N] overload. So it is disabled. If you want to
     * construct a substr from a single pointer containing a C-style string,
     * you can call c4::to_substr()/c4::to_csubstr().
     * @see c4::to_substr()
     * @see c4::to_csubstr() */
    template<size_t N>
    constexpr basic_substring(C (&s_)[N]) noexcept : str(s_), len(N-1) {}
    basic_substring(C *s_, size_t len_) : str(s_), len(len_) { C4_ASSERT(str || !len_); }
    basic_substring(C *beg_, C *end_) : str(beg_), len(static_cast<size_t>(end_ - beg_)) { C4_ASSERT(end_ >= beg_); }

    //basic_substring& operator= (C *s_) { this->assign(s_); return *this; }
    template<size_t N>
    basic_substring& operator= (C (&s_)[N]) { this->assign<N>(s_); return *this; }

    //void assign(C *s_) { str = (s_); len = (s_ ? strlen(s_) : 0); }
    /** the overload for receiving a single C* pointer will always
     * hide the array[N] overload. So it is disabled. If you want to
     * construct a substr from a single pointer containing a C-style string,
     * you can call c4::to_substr()/c4::to_csubstr().
     * @see c4::to_substr()
     * @see c4::to_csubstr() */
    template<size_t N>
    void assign(C (&s_)[N]) { str = (s_); len = (N-1); }
    void assign(C *s_, size_t len_) { str = s_; len = len_; C4_ASSERT(str || !len_); }
    void assign(C *beg_, C *end_) { C4_ASSERT(end_ >= beg_); str = (beg_); len = (end_ - beg_); }

    void clear() { str = nullptr; len = 0; }

    /** @} */

public:

    /** @name Construction from non-const characters */
    /** @{ */

    // when the char type is const, allow construction and assignment from non-const chars

    /** only available when the char type is const */
    template<size_t N, class U=NCC_> explicit basic_substring(C4_NC2C(U) (&s_)[N]) { str = s_; len = N-1; }
    /** only available when the char type is const */
    template<          class U=NCC_>          basic_substring(C4_NC2C(U) *s_, size_t len_) { str = s_; len = len_; }
    /** only available when the char type is const */
    template<          class U=NCC_>          basic_substring(C4_NC2C(U) *beg_, C4_NC2C(U) *end_) { C4_ASSERT(end_ >= beg_); str = beg_; len = end_ - beg_;  }

    /** only available when the char type is const */
    template<size_t N, class U=NCC_> void assign(C4_NC2C(U) (&s_)[N]) { str = s_; len = N-1; }
    /** only available when the char type is const */
    template<          class U=NCC_> void assign(C4_NC2C(U) *s_, size_t len_) { str = s_; len = len_; }
    /** only available when the char type is const */
    template<          class U=NCC_> void assign(C4_NC2C(U) *beg_, C4_NC2C(U) *end_) { C4_ASSERT(end_ >= beg_); str = beg_; len = end_ - beg_;  }

    /** only available when the char type is const */
    template<size_t N, class U=NCC_>
    basic_substring& operator=(C4_NC2C(U) (&s_)[N]) { str = s_; len = N-1; return *this; }

    /** @} */

public:

    /** @name Standard accessor methods */
    /** @{ */

    bool   has_str()   const { return ! empty() && str[0] != C(0); }
    bool   empty()     const { return (len == 0 || str == nullptr); }
    bool   not_empty() const { return (len != 0 && str != nullptr); }
    size_t size()      const { return len; }

    iterator begin() { return str; }
    iterator end  () { return str + len; }

    const_iterator begin() const { return str; }
    const_iterator end  () const { return str + len; }

    C      * data()       { return str; }
    C const* data() const { return str; }

    inline C      & operator[] (size_t i)       { C4_ASSERT(i >= 0 && i < len); return str[i]; }
    inline C const& operator[] (size_t i) const { C4_ASSERT(i >= 0 && i < len); return str[i]; }

    inline C      & front()       { C4_ASSERT(len > 0 && str != nullptr); return *str; }
    inline C const& front() const { C4_ASSERT(len > 0 && str != nullptr); return *str; }

    inline C      & back()       { C4_ASSERT(len > 0 && str != nullptr); return *(str + len - 1); }
    inline C const& back() const { C4_ASSERT(len > 0 && str != nullptr); return *(str + len - 1); }

    /** @} */

public:

    /** @name Comparison methods */
    /** @{ */

    int compare(C const c) const
    {
        C4_XASSERT((str != nullptr) || len == 0);
        if( ! len)
        {
            return -1;
        }
        if(*str == c)
        {
            return static_cast<int>(len - 1);
        }
        return *str - c;
    }

    int compare(const char *that, size_t sz) const
    {
        C4_XASSERT(that || sz  == 0);
        C4_XASSERT(str  || len == 0);
        if(C4_LIKELY(str && that))
        {
            int ret = strncmp(str, that, len < sz ? len : sz);
            if(ret == 0 && len != sz)
            {
                ret = len < sz ? -1 : 1;
            }
            return ret;
        }
        if((!str && !that) || (len == sz))
        {
            C4_XASSERT(len == 0 && sz == 0);
            return 0;
        }
        return len < sz ? -1 : 1;
    }

    C4_ALWAYS_INLINE int compare(ro_substr const that) const { return this->compare(that.str, that.len); }

    C4_ALWAYS_INLINE bool operator== (std::nullptr_t) const { return str == nullptr; }
    C4_ALWAYS_INLINE bool operator!= (std::nullptr_t) const { return str != nullptr; }

    C4_ALWAYS_INLINE bool operator== (C const c) const { return this->compare(c) == 0; }
    C4_ALWAYS_INLINE bool operator!= (C const c) const { return this->compare(c) != 0; }
    C4_ALWAYS_INLINE bool operator<  (C const c) const { return this->compare(c) <  0; }
    C4_ALWAYS_INLINE bool operator>  (C const c) const { return this->compare(c) >  0; }
    C4_ALWAYS_INLINE bool operator<= (C const c) const { return this->compare(c) <= 0; }
    C4_ALWAYS_INLINE bool operator>= (C const c) const { return this->compare(c) >= 0; }

    template<class U> C4_ALWAYS_INLINE bool operator== (basic_substring<U> const that) const { return this->compare(that) == 0; }
    template<class U> C4_ALWAYS_INLINE bool operator!= (basic_substring<U> const that) const { return this->compare(that) != 0; }
    template<class U> C4_ALWAYS_INLINE bool operator<  (basic_substring<U> const that) const { return this->compare(that) <  0; }
    template<class U> C4_ALWAYS_INLINE bool operator>  (basic_substring<U> const that) const { return this->compare(that) >  0; }
    template<class U> C4_ALWAYS_INLINE bool operator<= (basic_substring<U> const that) const { return this->compare(that) <= 0; }
    template<class U> C4_ALWAYS_INLINE bool operator>= (basic_substring<U> const that) const { return this->compare(that) >= 0; }

    template<size_t N> C4_ALWAYS_INLINE bool operator== (const char (&that)[N]) const { return this->compare(that, N-1) == 0; }
    template<size_t N> C4_ALWAYS_INLINE bool operator!= (const char (&that)[N]) const { return this->compare(that, N-1) != 0; }
    template<size_t N> C4_ALWAYS_INLINE bool operator<  (const char (&that)[N]) const { return this->compare(that, N-1) <  0; }
    template<size_t N> C4_ALWAYS_INLINE bool operator>  (const char (&that)[N]) const { return this->compare(that, N-1) >  0; }
    template<size_t N> C4_ALWAYS_INLINE bool operator<= (const char (&that)[N]) const { return this->compare(that, N-1) <= 0; }
    template<size_t N> C4_ALWAYS_INLINE bool operator>= (const char (&that)[N]) const { return this->compare(that, N-1) >= 0; }

    /** @} */

public:

    /** @name Sub-selection methods */
    /** @{ */

    /** true if *this is a substring of that (ie, from the same buffer) */
    inline bool is_sub(ro_substr const that) const
    {
        return that.is_super(*this);
    }

    /** true if that is a substring of *this (ie, from the same buffer) */
    inline bool is_super(ro_substr const that) const
    {
        if(C4_UNLIKELY(len == 0))
        {
            return that.len == 0 && that.str == str && str != nullptr;
        }
        return that.begin() >= begin() && that.end() <= end();
    }

    /** true if there is overlap of at least one element between that and *this */
    inline bool overlaps(ro_substr const that) const
    {
        CC * b =      begin(), * e =      end(),
           *tb = that.begin(), *te = that.end();
        return (
                (tb <= b && te >  b)
                ||
                (tb <  e && te >= e)
                ||
                (tb >= b && te <= e)
               )
               &&
               (b != nullptr && tb != nullptr);
    }

public:

    /** return [first,len[ */
    basic_substring sub(size_t first) const
    {
        C4_ASSERT(first >= 0 && first <= len);
        return basic_substring(str + first, len - first);
    }

    /** return [first,first+num[. If num==npos, return first
     * if*/
    basic_substring sub(size_t first, size_t num) const
    {
        C4_ASSERT(first >= 0 && first <= len);
        C4_ASSERT((num >= 0 && num <= len) || (num == npos));
        size_t rnum = num != npos ? num : len - first;
        C4_ASSERT((first >= 0 && first + rnum <= len) || (num == 0));
        return basic_substring(str + first, rnum);
    }

    /** return [first,last[ */
    basic_substring range(size_t first, size_t last=npos) const
    {
        C4_ASSERT(first >= 0 && first <= len);
        last = last != npos ? last : len;
        C4_ASSERT(first <= last);
        C4_ASSERT(last  >= 0 && last  <= len);
        return basic_substring(str + first, last - first);
    }

    /** return [0,num[*/
    basic_substring first(size_t num) const
    {
        return sub(0, num);
    }

    /** return [len-num,len[*/
    basic_substring last(size_t num) const
    {
        if(num == npos)
        {
            return *this;
        }
        return sub(len - num);
    }

    /** offset from the ends: return [left,len-right[ ; ie, trim a
        number of characters from the left and right. This is
        equivalent to python's negative list indices. */
    basic_substring offs(size_t left, size_t right) const
    {
        C4_ASSERT(left  >= 0 && left  <= len);
        C4_ASSERT(right >= 0 && right <= len);
        C4_ASSERT(left  <= len - right + 1);
        return basic_substring(str + left, len - right - left);
    }

    /** return [0, pos+include_pos[ */
    basic_substring left_of(size_t pos, bool include_pos=false) const
    {
        if(pos == npos)
        {
            return *this;
        }
        return first(pos + include_pos);
    }

    /** return [pos+!include_pos, len[ */
    basic_substring right_of(size_t pos, bool include_pos=false) const
    {
        if(pos == npos)
        {
            return sub(len, 0);
        }
        if( ! include_pos)
        {
            ++pos;
        }
        return sub(pos);
    }

public:

    /** given @p subs a substring of the current string, get the
     * portion of the current string to the left of it*/
    basic_substring left_of(ro_substr const subs) const
    {
        C4_ASSERT(this->is_super(subs) || subs.empty());
        auto ssb = subs.begin();
        auto b = begin();
        auto e = end();
        if(ssb >= b && ssb <= e)
        {
            return sub(0, static_cast<size_t>(ssb - b));
        }
        else
        {
            return sub(0, 0);
        }
    }

    /** given @p subs a substring of the current string, get the
     * portion of the current string to the right of it*/
    basic_substring right_of(ro_substr const subs) const
    {
        C4_ASSERT(is_super(subs) || subs.empty());
        auto sse = subs.end();
        auto b = begin();
        auto e = end();
        if(sse >= b && sse <= e)
        {
            return sub(static_cast<size_t>(sse - b), static_cast<size_t>(e - sse));
        }
        else
        {
            return sub(0, 0);
        }
    }

    /** @} */

public:

    /** @name Removing characters (trim()) / patterns (strip()) from the tips of the string */
    /** @{ */

    /** trim left */
    basic_substring triml(const C c) const
    {
        //return right_of(first_not_of(c), /*include_pos*/true);
        return triml({&c, 1});
    }
    /** trim left ANY of the characters.
     * @see stripl() to remove a pattern from the left */
    basic_substring triml(ro_substr chars) const
    {
        //return right_of(first_not_of(chars), /*include_pos*/true);
        if( ! empty())
        {
            size_t pos = first_not_of(chars, 0);
            if(pos != npos)
            {
                return sub(pos);
            }
        }
        return sub(0, 0);
    }

    /** trim the character c from the right */
    basic_substring trimr(const C c) const
    {
        //return left_of(last_not_of(c), /*include_pos*/true);
        return trimr({&c, 1});
    }
    /** trim right ANY of the characters
     * @see stripr() to remove a pattern from the right  */
    basic_substring trimr(ro_substr chars) const
    {
        //return left_of(last_not_of(chars), /*include_pos*/true);
        if( ! empty())
        {
            size_t pos = last_not_of(chars, npos);
            if(pos != npos)
            {
                return sub(0, pos+1);
            }
        }
        return sub(0, 0);
    }

    /** trim the character c left and right */
    basic_substring trim(const C c) const
    {
        return triml(c).trimr(c);
    }
    /** trim left and right ANY of the characters
     * @see strip() to remove a pattern from the left and right */
    basic_substring trim(ro_substr const chars) const
    {
        return triml(chars).trimr(chars);
    }

    /** remove a pattern from the left
     * @see triml() to remove characters*/
    basic_substring stripl(ro_substr pattern) const
    {
        if( ! begins_with(pattern)) return *this;
        return sub(pattern.len < len ? pattern.len : len);
    }

    /** remove a pattern from the right
     * @see trimr() to remove characters*/
    basic_substring stripr(ro_substr pattern) const
    {
        if( ! ends_with(pattern)) return *this;
        return left_of(len - (pattern.len < len ? pattern.len : len));
    }

    /** @} */

public:

    /** @name Lookup methods */
    /** @{ */

    inline size_t find(const C c, size_t start_pos=0) const
    {
        return first_of(c, start_pos);
    }
    inline size_t find(ro_substr pattern, size_t start_pos=0) const
    {
        C4_ASSERT(start_pos == npos || (start_pos >= 0 && start_pos <= len));
        if(len < pattern.len) return npos;
        for(size_t i = start_pos, e = len - pattern.len + 1; i < e; ++i)
        {
            bool gotit = true;
            for(size_t j = 0; j < pattern.len; ++j)
            {
                C4_ASSERT(i + j < len);
                if(str[i + j] != pattern.str[j])
                {
                    gotit = false;
                    break;
                }
            }
            if(gotit)
            {
                return i;
            }
        }
        return npos;
    }

public:

    /** count the number of occurrences of c */
    inline size_t count(const C c, size_t pos=0) const
    {
        C4_ASSERT(pos >= 0 && pos <= len);
        size_t num = 0;
        pos = find(c, pos);
        while(pos != npos)
        {
            ++num;
            pos = find(c, pos + 1);
        }
        return num;
    }

    /** get the substr consisting of the first occurrence of @p c after @p pos, or an empty substr if none occurs */
    inline basic_substring select(const C c, size_t pos=0) const
    {
        pos = find(c, pos);
        return pos != npos ? sub(pos, 1) : basic_substring();
    }

    /** get the substr consisting of the first occurrence of @p pattern after @p pos, or an empty substr if none occurs */
    inline basic_substring select(ro_substr pattern, size_t pos=0) const
    {
        pos = find(pattern, pos);
        return pos != npos ? sub(pos, pattern.len) : basic_substring();
    }

public:

    struct first_of_any_result
    {
        size_t which;
        size_t pos;
        inline operator bool() const { return which != NONE && pos != npos; }
    };

    first_of_any_result first_of_any(ro_substr s0, ro_substr s1) const
    {
        ro_substr s[2] = {s0, s1};
        return first_of_any_iter(&s[0], &s[0] + 2);
    }

    first_of_any_result first_of_any(ro_substr s0, ro_substr s1, ro_substr s2) const
    {
        ro_substr s[3] = {s0, s1, s2};
        return first_of_any_iter(&s[0], &s[0] + 3);
    }

    first_of_any_result first_of_any(ro_substr s0, ro_substr s1, ro_substr s2, ro_substr s3) const
    {
        ro_substr s[4] = {s0, s1, s2, s3};
        return first_of_any_iter(&s[0], &s[0] + 4);
    }

    first_of_any_result first_of_any(ro_substr s0, ro_substr s1, ro_substr s2, ro_substr s3, ro_substr s4) const
    {
        ro_substr s[5] = {s0, s1, s2, s3, s4};
        return first_of_any_iter(&s[0], &s[0] + 5);
    }

    template<class It>
    first_of_any_result first_of_any_iter(It first_span, It last_span) const
    {
        for(size_t i = 0; i < len; ++i)
        {
            size_t curr = 0;
            for(It it = first_span; it != last_span; ++curr, ++it)
            {
                auto const& chars = *it;
                if((i + chars.len) > len) continue;
                bool gotit = true;
                for(size_t j = 0; j < chars.len; ++j)
                {
                    C4_ASSERT(i + j < len);
                    if(str[i + j] != chars[j])
                    {
                        gotit = false;
                        break;
                    }
                }
                if(gotit)
                {
                    return {curr, i};
                }
            }
        }
        return {NONE, npos};
    }

public:

    /** true if the first character of the string is @p c */
    bool begins_with(const C c) const
    {
        return len > 0 ? str[0] == c : false;
    }

    /** true if the first @p num characters of the string are @p c */
    bool begins_with(const C c, size_t num) const
    {
        if(len < num)
        {
            return false;
        }
        for(size_t i = 0; i < num; ++i)
        {
            if(str[i] != c)
            {
                return false;
            }
        }
        return true;
    }

    /** true if the string begins with the given @p pattern */
    bool begins_with(ro_substr pattern) const
    {
        if(len < pattern.len)
        {
            return false;
        }
        for(size_t i = 0; i < pattern.len; ++i)
        {
            if(str[i] != pattern[i])
            {
                return false;
            }
        }
        return true;
    }

    /** true if the first character of the string is any of the given @p chars */
    bool begins_with_any(ro_substr chars) const
    {
        if(len == 0)
        {
            return false;
        }
        for(size_t i = 0; i < chars.len; ++i)
        {
            if(str[0] == chars.str[i])
            {
                return true;
            }
        }
        return false;
    }

    /** true if the last character of the string is @p c */
    bool ends_with(const C c) const
    {
        return len > 0 ? str[len-1] == c : false;
    }

    /** true if the last @p num characters of the string are @p c */
    bool ends_with(const C c, size_t num) const
    {
        if(len < num)
        {
            return false;
        }
        for(size_t i = len - num; i < len; ++i)
        {
            if(str[i] != c)
            {
                return false;
            }
        }
        return true;
    }

    /** true if the string ends with the given @p pattern */
    bool ends_with(ro_substr pattern) const
    {
        if(len < pattern.len)
        {
            return false;
        }
        for(size_t i = 0, s = len-pattern.len; i < pattern.len; ++i)
        {
            if(str[s+i] != pattern[i])
            {
                return false;
            }
        }
        return true;
    }

    /** true if the last character of the string is any of the given @p chars */
    bool ends_with_any(ro_substr chars) const
    {
        if(len == 0)
        {
            return false;
        }
        for(size_t i = 0; i < chars.len; ++i)
        {
            if(str[len - 1] == chars[i])
            {
                return true;
            }
        }
        return false;
    }

public:

    /** @return the first position where c is found in the string, or npos if none is found */
    size_t first_of(const C c, size_t start=0) const
    {
        C4_ASSERT(start == npos || (start >= 0 && start <= len));
        for(size_t i = start; i < len; ++i)
        {
            if(str[i] == c) return i;
        }
        return npos;
    }

    /** @return the last position where c is found in the string, or npos if none is found */
    size_t last_of(const C c, size_t start=npos) const
    {
        C4_ASSERT(start == npos || (start >= 0 && start <= len));
        if(start == npos) start = len;
        for(size_t i = start-1; i != size_t(-1); --i)
        {
            if(str[i] == c) return i;
        }
        return npos;
    }

    /** @return the first position where ANY of the chars is found in the string, or npos if none is found */
    size_t first_of(ro_substr chars, size_t start=0) const
    {
        C4_ASSERT(start == npos || (start >= 0 && start <= len));
        for(size_t i = start; i < len; ++i)
        {
            for(size_t j = 0; j < chars.len; ++j)
            {
                if(str[i] == chars[j]) return i;
            }
        }
        return npos;
    }

    /** @return the last position where ANY of the chars is found in the string, or npos if none is found */
    size_t last_of(ro_substr chars, size_t start=npos) const
    {
        C4_ASSERT(start == npos || (start >= 0 && start <= len));
        if(start == npos) start = len;
        for(size_t i = start-1; i != size_t(-1); --i)
        {
            for(size_t j = 0; j < chars.len; ++j)
            {
                if(str[i] == chars[j]) return i;
            }
        }
        return npos;
    }

public:

    size_t first_not_of(const C c, size_t start=0) const
    {
        C4_ASSERT((start >= 0 && start <= len) || (start == len && len == 0));
        for(size_t i = start; i < len; ++i)
        {
            if(str[i] != c) return i;
        }
        return npos;
    }

    size_t last_not_of(const C c, size_t start=npos) const
    {
        C4_ASSERT(start == npos || (start >= 0 && start <= len));
        if(start == npos) start = len;
        for(size_t i = start-1; i != size_t(-1); --i)
        {
            if(str[i] != c) return i;
        }
        return npos;
    }

    size_t first_not_of(ro_substr chars, size_t start=0) const
    {
        C4_ASSERT((start >= 0 && start <= len) || (start == len && len == 0));
        for(size_t i = start; i < len; ++i)
        {
            bool gotit = true;
            for(size_t j = 0; j < chars.len; ++j)
            {
                if(str[i] == chars.str[j])
                {
                    gotit = false;
                    break;
                }
            }
            if(gotit)
            {
                return i;
            }
        }
        return npos;
    }

    size_t last_not_of(ro_substr chars, size_t start=npos) const
    {
        C4_ASSERT(start == npos || (start >= 0 && start <= len));
        if(start == npos) start = len;
        for(size_t i = len-1; i != size_t(-1); --i)
        {
            bool gotit = true;
            for(size_t j = 0; j < chars.len; ++j)
            {
                if(str[i] == chars.str[j])
                {
                    gotit = false;
                    break;
                }
            }
            if(gotit)
            {
                return i;
            }
        }
        return npos;
    }

    /** @} */

public:

    /** @name Range lookup methods */
    /** @{ */

    /** get the range delimited by an open-close pair of characters.
     * @note There must be no nested pairs.
     * @note No checks for escapes are performed. */
    basic_substring pair_range(CC open, CC close) const
    {
        size_t b = find(open);
        if(b == npos) return basic_substring();
        size_t e = find(close, b+1);
        if(e == npos) return basic_substring();
        basic_substring ret = range(b, e+1);
        C4_ASSERT(ret.sub(1).find(open) == npos);
        return ret;
    }

    /** get the range delimited by a single open-close character (eg, quotes).
     * @note The open-close character can be escaped. */
    basic_substring pair_range_esc(CC open_close, CC escape=CC('\\'))
    {
        size_t b = find(open_close);
        if(b == npos) return basic_substring();
        for(size_t i = b+1; i < len; ++i)
        {
            CC c = str[i];
            if(c == open_close)
            {
                if(str[i-1] != escape)
                {
                    return range(b, i+1);
                }
            }
        }
        return basic_substring();
    }

    /** get the range delimited by an open-close pair of characters,
     * with possibly nested occurrences. No checks for escapes are
     * performed. */
    basic_substring pair_range_nested(CC open, CC close) const
    {
        size_t b = find(open);
        if(b == npos) return basic_substring();
        size_t e, curr = b+1, count = 0;
        const char both[] = {open, close, '\0'};
        while((e = first_of(both, curr)) != npos)
        {
            if(str[e] == open)
            {
                ++count;
                curr = e+1;
            }
            else if(str[e] == close)
            {
                if(count == 0) return range(b, e+1);
                --count;
                curr = e+1;
            }
        }
        return basic_substring();
    }

    basic_substring unquoted() const
    {
        constexpr const C dq('"'), sq('\'');
        if(len >= 2 && (str[len - 2] != C('\\')) &&
           ((begins_with(sq) && ends_with(sq))
            ||
            (begins_with(dq) && ends_with(dq))))
        {
            return range(1, len -1);
        }
        return *this;
    }

    /** @} */

public:

    /** @name Number-matching query methods */
    /** @{ */

    /** @return true if the substring contents are a floating-point or integer number */
    bool is_number() const
    {
        if(empty() || (first_non_empty_span().empty())) return false;
        if(first_real_span() == *this) return true;
        if(first_int_span() == *this) return true;
        if(first_uint_span() == *this) return true;
        return false;
    }

    /** @return true if the substring contents are an integer number */
    bool is_integer() const
    {
        if(empty() || (first_non_empty_span().empty())) return false;
        if(first_int_span() == *this) return true;
        if(first_uint_span() == *this) return true;
        return false;
    }


    /** get the first span consisting exclusively of non-empty characters */
    basic_substring first_non_empty_span() const
    {
        constexpr const ro_substr empty_chars(" \n\r\t");
        size_t pos = first_not_of(empty_chars);
        if(pos == npos) return first(0);
        auto ret = sub(pos);
        pos = ret.first_of(empty_chars);
        return ret.first(pos);
    }

    /** get the first span which can be interpreted as an unsigned integer */
    basic_substring first_uint_span() const
    {
        basic_substring ne = first_non_empty_span();
        if(ne.empty()) return ne;
        if(ne.str[0] == '-') return first(0);
        size_t skip_start = (ne.str[0] == '+') ? 1 : 0;
        return ne._first_integral_span(skip_start);
    }

    /** get the first span which can be interpreted as a signed integer */
    basic_substring first_int_span() const
    {
        basic_substring ne = first_non_empty_span();
        if(ne.empty()) return ne;
        size_t skip_start = (ne.str[0] == '+' || ne.str[0] == '-') ? 1 : 0;
        return ne._first_integral_span(skip_start);
    }

    basic_substring _first_integral_span(size_t skip_start) const
    {
        C4_ASSERT(!empty());
        if(skip_start == len) {
            return first(0);
        }
        C4_ASSERT(skip_start < len);
        if(first_of_any("0x", "0X")) // hexadecimal
        {
            skip_start += 2;
            if(len == skip_start) return first(0);
            for(size_t i = skip_start; i < len; ++i)
            {
                if( ! _is_hex_char(str[i]))
                {
                    return _is_delim_char(str[i]) ? first(i) : first(0);
                }
            }
        }
        else if(first_of_any("0o", "0O")) // octal
        {
            skip_start += 2;
            if(len == skip_start) return first(0);
            for(size_t i = skip_start; i < len; ++i)
            {
                char c = str[i];
                if(c < '0' || c > '7')
                {
                    return _is_delim_char(str[i]) ? first(i) : first(0);
                }
            }
        }
        else if(first_of_any("0b", "0B")) // binary
        {
            skip_start += 2;
            if(len == skip_start) return first(0);
            for(size_t i = skip_start; i < len; ++i)
            {
                char c = str[i];
                if(c != '0' && c != '1')
                {
                    return _is_delim_char(c) ? first(i) : first(0);
                }
            }
        }
        else // otherwise, decimal
        {
            if(len == skip_start) return first(0);
            for(size_t i = skip_start; i < len; ++i)
            {
                char c = str[i];
                if(c < '0' || c > '9')
                {
                    return _is_delim_char(c) ? first(i) : first(0);
                }
            }
        }
        return *this;
    }

    /** get the first span which can be interpreted as a real (floating-point) number */
    basic_substring first_real_span() const
    {
        basic_substring ne = first_non_empty_span();
        if(ne.empty()) return ne;
        size_t skip_start = (ne.str[0] == '+' || ne.str[0] == '-') ? 1 : 0;
        if(ne.first_of_any("0x", "0X")) // hexadecimal
        {
            skip_start += 2;
            if(ne.len == skip_start) return ne.first(0);
            for(size_t i = skip_start; i < ne.len; ++i)
            {
                char c = ne.str[i];
                if(( ! _is_hex_char(c)) && c != '.')
                {
                    return _is_delim_char(c) ? ne.first(i) : ne.first(0);
                }
            }
        }
        else if(ne.first_of_any("0b", "0B")) // binary
        {
            skip_start += 2;
            if(ne.len == skip_start) return ne.first(0);
            for(size_t i = skip_start; i < ne.len; ++i)
            {
                char c = ne.str[i];
                if(c != '0' && c != '1' && c != '.')
                {
                    return _is_delim_char(c) ? ne.first(i) : ne.first(0);
                }
            }
        }
        else // assume decimal
        {
            if(ne.len == skip_start) return ne.first(0);
            for(size_t i = skip_start; i < ne.len; ++i)
            {
                char c = ne.str[i];
                if((c < '0' || c > '9') && (c != '.' && c != 'e' && c != 'E'))
                {
                    if(c == '-' || c == '+')
                    {
                        // we can also have a sign for the exponent
                        if(i > 1 && (ne[i-1] == 'e' || ne[i-1] == 'E'))
                        {
                            continue;
                        }
                    }
                    return _is_delim_char(c) ? ne.first(i) : ne.first(0);
                }
            }
        }
        return ne;
    }

    /** true if the character is a delimiter character *at the end* */
    static constexpr C4_ALWAYS_INLINE bool _is_delim_char(char c) noexcept
    {
        return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\0'
            || c == ']' || c == ')'  || c == '}'
            || c == ',' || c == ';';
    }

    /** true if the character is in [0-9a-fA-F] */
    static constexpr C4_ALWAYS_INLINE bool _is_hex_char(char c) noexcept
    {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    /** @} */

public:

    /** @name Splitting methods */
    /** @{ */

    /** returns true if the string has not been exhausted yet, meaning
     * it's ok to call next_split() again. When no instance of sep
     * exists in the string, returns the full string. When the input
     * is an empty string, the output string is the empty string. */
    bool next_split(C sep, size_t *C4_RESTRICT start_pos, basic_substring *C4_RESTRICT out) const
    {
        if(C4_LIKELY(*start_pos < len))
        {
            for(size_t i = *start_pos, e = len; i < e; i++)
            {
                if(str[i] == sep)
                {
                    out->assign(str + *start_pos, i - *start_pos);
                    *start_pos = i+1;
                    return true;
                }
            }
            out->assign(str + *start_pos, len - *start_pos);
            *start_pos = len + 1;
            return true;
        }
        else
        {
            bool valid = len > 0 && (*start_pos == len);
            if(valid && !empty() && str[len-1] == sep)
            {
                out->assign(str + len, (size_t)0); // the cast is needed to prevent overload ambiguity
            }
            else
            {
                out->assign(str + len + 1, (size_t)0); // the cast is needed to prevent overload ambiguity
            }
            *start_pos = len + 1;
            return valid;
        }
    }

private:

    struct split_proxy_impl
    {
        struct split_iterator_impl
        {
            split_proxy_impl const* m_proxy;
            basic_substring m_str;
            size_t m_pos;
            NCC_ m_sep;

            split_iterator_impl(split_proxy_impl const* proxy, size_t pos, C sep)
                : m_proxy(proxy), m_pos(pos), m_sep(sep)
            {
                _tick();
            }

            void _tick()
            {
                m_proxy->m_str.next_split(m_sep, &m_pos, &m_str);
            }

            split_iterator_impl& operator++ () { _tick(); return *this; }
            split_iterator_impl  operator++ (int) { split_iterator_impl it = *this; _tick(); return it; }

            basic_substring& operator*  () { return  m_str; }
            basic_substring* operator-> () { return &m_str; }

            bool operator!= (split_iterator_impl const& that) const
            {
                return !(this->operator==(that));
            }
            bool operator== (split_iterator_impl const& that) const
            {
                C4_XASSERT((m_sep == that.m_sep) && "cannot compare split iterators with different separators");
                if(m_str.size() != that.m_str.size()) return false;
                if(m_str.data() != that.m_str.data()) return false;
                return m_pos == that.m_pos;
            }
        };

        basic_substring m_str;
        size_t m_start_pos;
        C m_sep;

        split_proxy_impl(basic_substring str_, size_t start_pos, C sep)
            : m_str(str_), m_start_pos(start_pos), m_sep(sep)
        {
        }

        split_iterator_impl begin() const
        {
            auto it = split_iterator_impl(this, m_start_pos, m_sep);
            return it;
        }
        split_iterator_impl end() const
        {
            size_t pos = m_str.size() + 1;
            auto it = split_iterator_impl(this, pos, m_sep);
            return it;
        }
    };

public:

    using split_proxy = split_proxy_impl;

    /** a view into the splits iterate throught splits with a view:*/
    split_proxy split(C sep, size_t start_pos=0) const
    {
        C4_XASSERT((start_pos >= 0 && start_pos < len) || empty());
        auto ss = sub(0, len);
        auto it = split_proxy(ss, start_pos, sep);
        return it;
    }

public:

    /** pop right: return the first split from the right. Use
     * gpop_left() to get the reciprocal part.
     */
    basic_substring pop_right(C sep=C('/'), bool skip_empty=false) const
    {
        if(C4_LIKELY(len > 1))
        {
            auto pos = last_of(sep);
            if(pos != npos)
            {
                if(pos + 1 < len) // does not end with sep
                {
                    return sub(pos + 1); // return from sep to end
                }
                else // the string ends with sep
                {
                    if( ! skip_empty)
                    {
                        return sub(pos + 1, 0);
                    }
                    auto ppos = last_not_of(sep); // skip repeated seps
                    if(ppos == npos) // the string is all made of seps
                    {
                        return sub(0, 0);
                    }
                    // find the previous sep
                    auto pos0 = last_of(sep, ppos);
                    if(pos0 == npos) // only the last sep exists
                    {
                        return sub(0); // return the full string (because skip_empty is true)
                    }
                    ++pos0;
                    return sub(pos0);
                }
            }
            else // no sep was found, return the full string
            {
                return *this;
            }
        }
        else if(len == 1)
        {
            if(begins_with(sep))
            {
                return sub(0, 0);
            }
            return *this;
        }
        else // an empty string
        {
            return basic_substring();
        }
    }

    /** return the first split from the left. Use gpop_right() to get
     * the reciprocal part. */
    basic_substring pop_left(C sep = C('/'), bool skip_empty=false) const
    {
        if(C4_LIKELY(len > 1))
        {
            auto pos = first_of(sep);
            if(pos != npos)
            {
                if(pos > 0)  // does not start with sep
                {
                    return sub(0, pos); //  return everything up to it
                }
                else  // the string starts with sep
                {
                    if( ! skip_empty)
                    {
                        return sub(0, 0);
                    }
                    auto ppos = first_not_of(sep); // skip repeated seps
                    if(ppos == npos) // the string is all made of seps
                    {
                        return sub(0, 0);
                    }
                    // find the next sep
                    auto pos0 = first_of(sep, ppos);
                    if(pos0 == npos) // only the first sep exists
                    {
                        return sub(0); // return the full string (because skip_empty is true)
                    }
                    C4_XASSERT(pos0 > 0);
                    // return everything up to the second sep
                    return sub(0, pos0);
                }
            }
            else // no sep was found, return the full string
            {
                return sub(0);
            }
        }
        else if(len == 1)
        {
            if(begins_with(sep))
            {
                return sub(0, 0);
            }
            return sub(0);
        }
        else // an empty string
        {
            return basic_substring();
        }
    }

public:

    /** greedy pop left. eg, csubstr("a/b/c").gpop_left('/')="c" */
    basic_substring gpop_left(C sep = C('/'), bool skip_empty=false) const
    {
        auto ss = pop_right(sep, skip_empty);
        ss = left_of(ss);
        if(ss.find(sep) != npos)
        {
            if(ss.ends_with(sep))
            {
                if(skip_empty)
                {
                    ss = ss.trimr(sep);
                }
                else
                {
                    ss = ss.sub(0, ss.len-1); // safe to subtract because ends_with(sep) is true
                }
            }
        }
        return ss;
    }

    /** greedy pop right. eg, csubstr("a/b/c").gpop_right('/')="a" */
    basic_substring gpop_right(C sep = C('/'), bool skip_empty=false) const
    {
        auto ss = pop_left(sep, skip_empty);
        ss = right_of(ss);
        if(ss.find(sep) != npos)
        {
            if(ss.begins_with(sep))
            {
                if(skip_empty)
                {
                    ss = ss.triml(sep);
                }
                else
                {
                    ss = ss.sub(1);
                }
            }
        }
        return ss;
    }

    /** @} */

public:

    /** @name Path-like manipulation methods */
    /** @{ */

    basic_substring basename(C sep=C('/')) const
    {
        auto ss = pop_right(sep, /*skip_empty*/true);
        ss = ss.trimr(sep);
        return ss;
    }

    basic_substring dirname(C sep=C('/')) const
    {
        auto ss = basename(sep);
        ss = ss.empty() ? *this : left_of(ss);
        return ss;
    }

    C4_ALWAYS_INLINE basic_substring name_wo_extshort() const
    {
        return gpop_left('.');
    }

    C4_ALWAYS_INLINE basic_substring name_wo_extlong() const
    {
        return pop_left('.');
    }

    C4_ALWAYS_INLINE basic_substring extshort() const
    {
        return pop_right('.');
    }

    C4_ALWAYS_INLINE basic_substring extlong() const
    {
        return gpop_right('.');
    }

    /** @} */

public:

    /** @name Content-modification methods (only for non-const C) */
    /** @{ */

    /** convert the string to upper-case
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) toupper()
    {
        for(size_t i = 0; i < len; ++i)
        {
            str[i] = static_cast<C>(::toupper(str[i]));
        }
    }

    /** convert the string to lower-case
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) tolower()
    {
        for(size_t i = 0; i < len; ++i)
        {
            str[i] = static_cast<C>(::tolower(str[i]));
        }
    }

public:

    /** fill the entire contents with the given @p val
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) fill(C val)
    {
        for(size_t i = 0; i < len; ++i)
        {
            str[i] = val;
        }
    }

public:

    /** set the current substring to a copy of the given csubstr
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) copy_from(ro_substr that, size_t ifirst=0, size_t num=npos)
    {
        C4_ASSERT(ifirst >= 0 && ifirst <= len);
        num = num != npos ? num : len - ifirst;
        num = num < that.len ? num : that.len;
        C4_ASSERT(ifirst + num >= 0 && ifirst + num <= len);
        memcpy(str + sizeof(C) * ifirst, that.str, sizeof(C) * num);
    }

public:

    /** reverse in place
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) reverse()
    {
        if(len == 0) return;
        detail::_do_reverse(str, str + len - 1);
    }

    /** revert a subpart in place
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) reverse_sub(size_t ifirst, size_t num)
    {
        C4_ASSERT(ifirst >= 0 && ifirst <= len);
        C4_ASSERT(ifirst + num >= 0 && ifirst + num <= len);
        if(num == 0) return;
        detail::_do_reverse(str + ifirst, ifirst + num - 1);
    }

    /** revert a range in place
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(void) reverse_range(size_t ifirst, size_t ilast)
    {
        C4_ASSERT(ifirst >= 0 && ifirst <= len);
        C4_ASSERT(ilast  >= 0 && ilast  <= len);
        if(ifirst == ilast) return;
        detail::_do_reverse(str + ifirst, str + ilast - 1);
    }

public:

    /** erase part of the string. eg, with char s[] = "0123456789",
     * substr(s).erase(3, 2) = "01256789", and s is now "01245678989"
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(basic_substring) erase(size_t pos, size_t num)
    {
        C4_ASSERT(pos >= 0 && pos+num <= len);
        size_t num_to_move = len - pos - num;
        memmove(str + pos, str + pos + num, sizeof(C) * num_to_move);
        return basic_substring{str, len - num};
    }

    /** @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(basic_substring) erase_range(size_t first, size_t last)
    {
        C4_ASSERT(first <= last);
        return erase(first, static_cast<size_t>(last-first));
    }

    /** erase a part of the string.
     * @note @p sub must be a substring of this string
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(basic_substring) erase(ro_substr sub)
    {
        C4_ASSERT(is_super(sub));
        C4_ASSERT(sub.str >= str);
        return erase(static_cast<size_t>(sub.str - str), sub.len);
    }

public:

    /** replace every occurrence of character @p value with the character @p repl
     * @return the number of characters that were replaced
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(size_t) replace(C value, C repl, size_t pos=0)
    {
        C4_ASSERT((pos >= 0 && pos <= len) || pos == npos);
        size_t did_it = 0;
        while((pos = find(value, pos)) != npos)
        {
            str[pos++] = repl;
            ++did_it;
        }
        return did_it;
    }

    /** replace every occurrence of each character in @p value with
     * the character @p repl.
     * @return the number of characters that were replaced
     * @note this method requires that the string memory is writeable and is SFINAEd out for const C */
    C4_REQUIRE_RW(size_t) replace(ro_substr chars, C repl, size_t pos=0)
    {
        C4_ASSERT((pos >= 0 && pos <= len) || pos == npos);
        size_t did_it = 0;
        while((pos = first_of(chars, pos)) != npos)
        {
            str[pos++] = repl;
            ++did_it;
        }
        return did_it;
    }

    /** replace @p pattern with @p repl, and write the result into
     * @dst. pattern and repl don't need equal sizes.
     *
     * @return the required size for dst. No overflow occurs if
     * dst.len is smaller than the required size; this can be used to
     * determine the required size for an existing container. */
    size_t replace_all(rw_substr dst, ro_substr pattern, ro_substr repl, size_t pos=0) const
    {
        C4_ASSERT( ! pattern.empty()); //!< @todo relax this precondition
        C4_ASSERT( ! this  ->overlaps(dst)); //!< @todo relax this precondition
        C4_ASSERT( ! pattern.overlaps(dst));
        C4_ASSERT( ! repl   .overlaps(dst));
        C4_ASSERT((pos >= 0 && pos <= len) || pos == npos);
#define _c4append(first, last)                                  \
        {                                                       \
            C4_ASSERT((last) >= (first));                       \
            size_t num = static_cast<size_t>((last) - (first)); \
            if(sz + num <= dst.len)                             \
            {                                                   \
                memcpy(dst.str + sz, first, num * sizeof(C));   \
            }                                                   \
            sz += num;                                          \
        }
        size_t sz = 0;
        size_t b = pos;
        _c4append(str, str + pos);
        do {
            size_t e = find(pattern, b);
            if(e == npos)
            {
                _c4append(str + b, str + len);
                break;
            }
            _c4append(str + b, str + e);
            _c4append(repl.begin(), repl.end());
            b = e + pattern.size();
        } while(b < len && b != npos);
        return sz;
#undef _c4append
    }

    /** @} */

}; // template class basic_substring


/** ConstantSUBSTRing: a non-owning read-only string view
 * @see to_substr()
 * @see to_csubstr() */
using csubstr = C4CORE_EXPORT basic_substring<const char>;


/** SUBSTRing: a non-owning read-write string view
 * @see to_substr()
 * @see to_csubstr() */
using substr = C4CORE_EXPORT basic_substring<char>;


#undef C4_REQUIRE_RW
#undef C4_REQUIRE_RO
#undef C4_NC2C


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** Because of a C++ limitation, substr cannot provide simultaneous
 * overloads for constructing from a char[N] and a char*; the latter
 * will always be chosen by the compiler. So this specialization is
 * provided to simplify obtaining a substr from a char*. Being a
 * function has the advantage of highlighting the strlen() cost.
 *
 * @see to_csubstr
 * @see For a more detailed explanation on why the overloads cannot
 * coexist, see http://cplusplus.bordoon.com/specializeForCharacterArrays.html */
inline substr to_substr(char *s)
{
    return substr(s, s ? strlen(s) : 0);
}

/** Because of a C++ limitation, substr cannot provide simultaneous
 * overloads for constructing from a char[N] and a char*; the latter
 * will always be chosen by the compiler. So this specialization is
 * provided to simplify obtaining a substr from a char*. Being a
 * function has the advantage of highlighting the strlen() cost.
 *
 * @see to_substr
 * @see For a more detailed explanation on why the overloads cannot
 * coexist, see http://cplusplus.bordoon.com/specializeForCharacterArrays.html */
inline csubstr to_csubstr(char *s)
{
    return csubstr(s, s ? strlen(s) : 0);
}

/** Because of a C++ limitation, substr cannot provide simultaneous
 * overloads for constructing from a const char[N] and a const char*;
 * the latter will always be chosen by the compiler. So this
 * specialization is provided to simplify obtaining a substr from a
 * char*. Being a function has the advantage of highlighting the
 * strlen() cost.
 *
 * @overload to_csubstr
 * @see to_substr
 * @see For a more detailed explanation on why the overloads cannot
 * coexist, see http://cplusplus.bordoon.com/specializeForCharacterArrays.html */
inline csubstr to_csubstr(const char *s)
{
    return csubstr(s, s ? strlen(s) : 0);
}


/** neutral version for use in generic code */
inline csubstr to_csubstr(csubstr s)
{
    return s;
}

/** neutral version for use in generic code */
inline csubstr to_csubstr(substr s)
{
    return s;
}

/** neutral version for use in generic code */
inline substr to_substr(substr s)
{
    return s;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<typename C, size_t N> inline bool operator== (const C (&s)[N], basic_substring<const C> const that) { return that.compare(s) == 0; }
template<typename C, size_t N> inline bool operator!= (const C (&s)[N], basic_substring<const C> const that) { return that.compare(s) != 0; }
template<typename C, size_t N> inline bool operator<  (const C (&s)[N], basic_substring<const C> const that) { return that.compare(s) >  0; }
template<typename C, size_t N> inline bool operator>  (const C (&s)[N], basic_substring<const C> const that) { return that.compare(s) <  0; }
template<typename C, size_t N> inline bool operator<= (const C (&s)[N], basic_substring<const C> const that) { return that.compare(s) >= 0; }
template<typename C, size_t N> inline bool operator>= (const C (&s)[N], basic_substring<const C> const that) { return that.compare(s) <= 0; }

template<typename C> inline bool operator== (C const c, basic_substring<const C> const that) { return that.compare(c) == 0; }
template<typename C> inline bool operator!= (C const c, basic_substring<const C> const that) { return that.compare(c) != 0; }
template<typename C> inline bool operator<  (C const c, basic_substring<const C> const that) { return that.compare(c) >  0; }
template<typename C> inline bool operator>  (C const c, basic_substring<const C> const that) { return that.compare(c) <  0; }
template<typename C> inline bool operator<= (C const c, basic_substring<const C> const that) { return that.compare(c) >= 0; }
template<typename C> inline bool operator>= (C const c, basic_substring<const C> const that) { return that.compare(c) <= 0; }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** @define C4_SUBSTR_NO_OSTREAM_LSHIFT doctest does not deal well with
 * template operator<<
 * @see https://github.com/onqtam/doctest/pull/431 */
#ifndef C4_SUBSTR_NO_OSTREAM_LSHIFT
#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

/** output the string to a stream */
template<class OStream, class C>
inline OStream& operator<< (OStream& os, basic_substring<C> s)
{
    os.write(s.str, s.len);
    return os;
}

// this causes ambiguity
///** this is used by google test */
//template<class OStream, class C>
//inline void PrintTo(basic_substring<C> s, OStream* os)
//{
//    os->write(s.str, s.len);
//}

#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
#endif // !C4_SUBSTR_NO_OSTREAM_LSHIFT

} // namespace c4


#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

#endif /* _C4_SUBSTR_HPP_ */
