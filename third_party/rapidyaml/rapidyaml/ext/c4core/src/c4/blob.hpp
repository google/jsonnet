#ifndef _C4_BLOB_HPP_
#define _C4_BLOB_HPP_

#include "c4/types.hpp"
#include "c4/error.hpp"

/** @file blob.hpp Mutable and immutable binary data blobs.
*/

namespace c4 {

template<class T>
struct blob_
{
    T *    buf;
    size_t len;

    C4_ALWAYS_INLINE blob_() noexcept : buf(), len() {}

    C4_ALWAYS_INLINE blob_(blob_ const& that) noexcept = default;
    C4_ALWAYS_INLINE blob_(blob_     && that) noexcept = default;
    C4_ALWAYS_INLINE blob_& operator=(blob_     && that) noexcept = default;
    C4_ALWAYS_INLINE blob_& operator=(blob_ const& that) noexcept = default;

    // need to sfinae out copy constructors! (why? isn't the above sufficient?)
    #define _C4_REQUIRE_NOT_SAME class=typename std::enable_if<( ! std::is_same<U, blob_>::value) && ( ! std::is_pointer<U>::value), T>::type
    template<class U, _C4_REQUIRE_NOT_SAME> C4_ALWAYS_INLINE blob_(U &var) noexcept : buf(reinterpret_cast<T*>(&var)), len(sizeof(U)) {}
    template<class U, _C4_REQUIRE_NOT_SAME> C4_ALWAYS_INLINE blob_& operator= (U &var) noexcept { buf = reinterpret_cast<T*>(&var); len = sizeof(U); return *this; }
    #undef _C4_REQUIRE_NOT_SAME

    template<class U, size_t N> C4_ALWAYS_INLINE blob_(U (&arr)[N]) noexcept : buf(reinterpret_cast<T*>(arr)), len(sizeof(U) * N) {}
    template<class U, size_t N> C4_ALWAYS_INLINE blob_& operator= (U (&arr)[N]) noexcept { buf = reinterpret_cast<T*>(arr); len = sizeof(U) * N; return *this; }

    template<class U>
    C4_ALWAYS_INLINE blob_(U          *ptr, size_t n) noexcept : buf(reinterpret_cast<T*>(ptr)), len(sizeof(U) * n) { C4_ASSERT(is_aligned(ptr)); }
    C4_ALWAYS_INLINE blob_(void       *ptr, size_t n) noexcept : buf(reinterpret_cast<T*>(ptr)), len(n) {}
    C4_ALWAYS_INLINE blob_(void const *ptr, size_t n) noexcept : buf(reinterpret_cast<T*>(ptr)), len(n) {}
};

/** an immutable binary blob */
using cblob = blob_<cbyte>;
/** a mutable binary blob */
using  blob = blob_< byte>;

C4_MUST_BE_TRIVIAL_COPY(blob);
C4_MUST_BE_TRIVIAL_COPY(cblob);

} // namespace c4

#endif // _C4_BLOB_HPP_
