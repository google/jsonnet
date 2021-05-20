#include "c4/format.hpp"

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif

namespace c4 {


size_t to_chars(substr buf, fmt::const_raw_wrapper r)
{
    void * vptr = buf.str;
    size_t space = buf.len;
    auto ptr = (decltype(buf.str)) std::align(r.alignment, r.len, vptr, space);
    if(ptr == nullptr)
    {
        // if it was not possible to align, return a conservative estimate
        // of the required space
        return r.alignment + r.len;
    }
    C4_CHECK(ptr >= buf.begin() && ptr <= buf.end());
    size_t sz = static_cast<size_t>(ptr - buf.str) + r.len;
    if(sz <= buf.len)
    {
        memcpy(ptr, r.buf, r.len);
    }
    return sz;
}


bool from_chars(csubstr buf, fmt::raw_wrapper *r)
{
    void * vptr = (void*)buf.str;
    size_t space = buf.len;
    auto ptr = (decltype(buf.str)) std::align(r->alignment, r->len, vptr, space);
    C4_CHECK(ptr != nullptr);
    C4_CHECK(ptr >= buf.begin() && ptr <= buf.end());
    //size_t dim = (ptr - buf.str) + r->len;
    memcpy(r->buf, ptr, r->len);
    return true;
}


} // namespace c4

#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
