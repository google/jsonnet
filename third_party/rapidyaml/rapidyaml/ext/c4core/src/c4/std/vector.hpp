#ifndef _C4_STD_VECTOR_HPP_
#define _C4_STD_VECTOR_HPP_

/** @file vector.hpp provides conversion and comparison facilities
 * from/between std::vector<char> to c4::substr and c4::csubstr.
 * @todo add to_span() and friends
 */

#include "c4/substr.hpp"

#include <vector>

namespace c4 {

//-----------------------------------------------------------------------------

/** get a substr (writeable string view) of an existing std::vector<char> */
template<class Alloc>
c4::substr to_substr(std::vector<char, Alloc> &vec)
{
    char *data = vec.empty() ? nullptr : vec.data(); // data() may or may not return a null pointer.
    return c4::substr(data, vec.size());
}

/** get a csubstr (read-only string) view of an existing std::vector<char> */
template<class Alloc>
c4::csubstr to_csubstr(std::vector<char, Alloc> const& vec)
{
    const char *data = vec.empty() ? nullptr : vec.data(); // data() may or may not return a null pointer.
    return c4::csubstr(data, vec.size());
}

/** get a csubstr (read-only string view) of an existing std::vector<const char> */
template<class Alloc>
c4::csubstr to_csubstr(std::vector<const char, Alloc> const& vec)
{
    const char *data = vec.empty() ? nullptr : vec.data(); // data() may or may not return a null pointer.
    return c4::csubstr(data, vec.size());
}

//-----------------------------------------------------------------------------
// comparisons between substrings std::vector<char>/std::vector<const char>

template<class Alloc> inline bool operator== (c4::csubstr ss, std::vector<char, Alloc> const& s) { return ss == to_csubstr(s); }
template<class Alloc> inline bool operator>= (c4::csubstr ss, std::vector<char, Alloc> const& s) { return ss >= to_csubstr(s); }
template<class Alloc> inline bool operator>  (c4::csubstr ss, std::vector<char, Alloc> const& s) { return ss >  to_csubstr(s); }
template<class Alloc> inline bool operator<= (c4::csubstr ss, std::vector<char, Alloc> const& s) { return ss <= to_csubstr(s); }
template<class Alloc> inline bool operator<  (c4::csubstr ss, std::vector<char, Alloc> const& s) { return ss <  to_csubstr(s); }

template<class Alloc> inline bool operator== (std::vector<char, Alloc> const& s, c4::csubstr ss) { return ss == to_csubstr(s); }
template<class Alloc> inline bool operator>= (std::vector<char, Alloc> const& s, c4::csubstr ss) { return ss <= to_csubstr(s); }
template<class Alloc> inline bool operator>  (std::vector<char, Alloc> const& s, c4::csubstr ss) { return ss <  to_csubstr(s); }
template<class Alloc> inline bool operator<= (std::vector<char, Alloc> const& s, c4::csubstr ss) { return ss >= to_csubstr(s); }
template<class Alloc> inline bool operator<  (std::vector<char, Alloc> const& s, c4::csubstr ss) { return ss >  to_csubstr(s); }

template<class Alloc> inline bool operator== (c4::csubstr ss, std::vector<const char, Alloc> const& s) { return ss == to_csubstr(s); }
template<class Alloc> inline bool operator>= (c4::csubstr ss, std::vector<const char, Alloc> const& s) { return ss >= to_csubstr(s); }
template<class Alloc> inline bool operator>  (c4::csubstr ss, std::vector<const char, Alloc> const& s) { return ss >  to_csubstr(s); }
template<class Alloc> inline bool operator<= (c4::csubstr ss, std::vector<const char, Alloc> const& s) { return ss <= to_csubstr(s); }
template<class Alloc> inline bool operator<  (c4::csubstr ss, std::vector<const char, Alloc> const& s) { return ss <  to_csubstr(s); }

template<class Alloc> inline bool operator== (std::vector<const char, Alloc> const& s, c4::csubstr ss) { return ss == to_csubstr(s); }
template<class Alloc> inline bool operator>= (std::vector<const char, Alloc> const& s, c4::csubstr ss) { return ss <= to_csubstr(s); }
template<class Alloc> inline bool operator>  (std::vector<const char, Alloc> const& s, c4::csubstr ss) { return ss <  to_csubstr(s); }
template<class Alloc> inline bool operator<= (std::vector<const char, Alloc> const& s, c4::csubstr ss) { return ss >= to_csubstr(s); }
template<class Alloc> inline bool operator<  (std::vector<const char, Alloc> const& s, c4::csubstr ss) { return ss >  to_csubstr(s); }

} // namespace c4

#endif // _C4_STD_VECTOR_HPP_
