#ifndef _C4_STD_STRING_HPP_
#define _C4_STD_STRING_HPP_

/** @file string.hpp */

#include "c4/substr.hpp"

#include <string>

namespace c4 {

//-----------------------------------------------------------------------------

/** get a writeable view to an existing std::string */
inline c4::substr to_substr(std::string &s)
{
    char* data = ! s.empty() ? &s[0] : nullptr;
    return c4::substr(data, s.size());
}

/** get a readonly view to an existing std::string */
inline c4::csubstr to_csubstr(std::string const& s)
{
    const char* data = ! s.empty() ? &s[0] : nullptr;
    return c4::csubstr(data, s.size());
}

//-----------------------------------------------------------------------------

inline bool operator== (c4::csubstr ss, std::string const& s) { return ss.compare(to_csubstr(s)) == 0; }
inline bool operator!= (c4::csubstr ss, std::string const& s) { return ss.compare(to_csubstr(s)) != 0; }
inline bool operator>= (c4::csubstr ss, std::string const& s) { return ss.compare(to_csubstr(s)) >= 0; }
inline bool operator>  (c4::csubstr ss, std::string const& s) { return ss.compare(to_csubstr(s)) >  0; }
inline bool operator<= (c4::csubstr ss, std::string const& s) { return ss.compare(to_csubstr(s)) <= 0; }
inline bool operator<  (c4::csubstr ss, std::string const& s) { return ss.compare(to_csubstr(s)) <  0; }

inline bool operator== (std::string const& s, c4::csubstr ss) { return ss.compare(to_csubstr(s)) == 0; }
inline bool operator!= (std::string const& s, c4::csubstr ss) { return ss.compare(to_csubstr(s)) != 0; }
inline bool operator>= (std::string const& s, c4::csubstr ss) { return ss.compare(to_csubstr(s)) <= 0; }
inline bool operator>  (std::string const& s, c4::csubstr ss) { return ss.compare(to_csubstr(s)) <  0; }
inline bool operator<= (std::string const& s, c4::csubstr ss) { return ss.compare(to_csubstr(s)) >= 0; }
inline bool operator<  (std::string const& s, c4::csubstr ss) { return ss.compare(to_csubstr(s)) >  0; }

//-----------------------------------------------------------------------------

/** copy an std::string to a writeable string view */
inline size_t to_chars(c4::substr buf, std::string const& s)
{
    C4_ASSERT(!buf.overlaps(to_csubstr(s)));
    size_t len = buf.len < s.size() ? buf.len : s.size();
    memcpy(buf.str, s.data(), len);
    return s.size(); // return the number of needed chars
}

/** copy a string view to an existing std::string */
inline bool from_chars(c4::csubstr buf, std::string * s)
{
    s->resize(buf.len);
    C4_ASSERT(!buf.overlaps(to_csubstr(*s)));
    memcpy(&(*s)[0], buf.str, buf.len);
    return true;
}

} // namespace c4

#endif // _C4_STD_STRING_HPP_
