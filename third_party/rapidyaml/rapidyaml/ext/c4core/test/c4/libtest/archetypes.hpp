#ifndef _C4_TEST_ARCHETYPES_HPP_
#define _C4_TEST_ARCHETYPES_HPP_

#include "c4/test.hpp"
#include "c4/memory_resource.hpp"
#include "c4/allocator.hpp"
#include "c4/char_traits.hpp"
#include <vector>
#include <string>
#include <array>

namespace c4 {

template< class String > class sstream;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
namespace archetypes {

template< class T > void check_archetype(T const& a) { a.check(); }
template< class T > void check_archetype(T const& a, T const& ref) { a.check(ref); }
inline void check_archetype(char    ) {}
inline void check_archetype(wchar_t ) {}
inline void check_archetype(int8_t  ) {}
inline void check_archetype(uint8_t ) {}
inline void check_archetype(int16_t ) {}
inline void check_archetype(uint16_t) {}
inline void check_archetype(int32_t ) {}
inline void check_archetype(uint32_t) {}
inline void check_archetype(int64_t ) {}
inline void check_archetype(uint64_t) {}
inline void check_archetype(float   ) {}
inline void check_archetype(double  ) {}
inline void check_archetype(char     a, char     ref) { CHECK_EQ(a, ref); }
inline void check_archetype(wchar_t  a, wchar_t  ref) { CHECK_EQ(a, ref); }
inline void check_archetype(int8_t   a, int8_t   ref) { CHECK_EQ(a, ref); }
inline void check_archetype(uint8_t  a, uint8_t  ref) { CHECK_EQ(a, ref); }
inline void check_archetype(int16_t  a, int16_t  ref) { CHECK_EQ(a, ref); }
inline void check_archetype(uint16_t a, uint16_t ref) { CHECK_EQ(a, ref); }
inline void check_archetype(int32_t  a, int32_t  ref) { CHECK_EQ(a, ref); }
inline void check_archetype(uint32_t a, uint32_t ref) { CHECK_EQ(a, ref); }
inline void check_archetype(int64_t  a, int64_t  ref) { CHECK_EQ(a, ref); }
inline void check_archetype(uint64_t a, uint64_t ref) { CHECK_EQ(a, ref); }
inline void check_archetype(float    a, float    ref) { CHECK_EQ((double)a, doctest::Approx((double)ref)); }
inline void check_archetype(double   a, double   ref) { CHECK_EQ(a, doctest::Approx(ref)); }


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template< class T, class Proto >
struct archetype_proto_base
{
    static T const& get(size_t which)
    {
        auto const& a = Proto::arr();
        C4_ASSERT(which < (int)a.size());
        return a[which];
    }
    static std::array< T, 8 > dup()
    {
        std::array< T, 8 > d = Proto::arr;
        return d;
    }
    static std::array< Counting<T>, 8 > cdup()
    {
        std::array< Counting<T>, 8 > d = Proto::carr;
        return d;
    }
    static std::vector< T > dup(size_t n)
    {
        auto const& a = Proto::arr();
        std::vector< T > d;
        d.reserve(n);
        for(size_t i = 0, pos = 0; i < n; ++i, pos = ((pos+1)%a.size()))
        {
            d.push_back(a[pos]);
        }
        return d;
    }
    static std::vector< Counting<T> > cdup(size_t n)
    {
        auto const& a = Proto::arr();
        std::vector< Counting<T> > d;
        d.reserve(n);
        for(size_t i = 0, pos = 0; i < n; ++i, pos = ((pos+1)%a.size()))
        {
            d.push_back(a[pos]);
        }
        return d;
    }
};

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wmissing-braces" //  warning : suggest braces around initialization of subobject [-Wmissing-braces]
#endif

// for scalar types: ints and floats
template< class T >
struct archetype_proto : public archetype_proto_base< T, archetype_proto<T> >
{
    static_assert(std::is_fundamental< T >::value, "T must be a fundamental type");
    static std::array<T, 8> const& arr()
    {
        static const std::array<T, 8> arr_{0, 1, 2, 3, 4, 5, 6, 7};
        return arr_;
    }
    static std::array<Counting<T>, 8> const& carr()
    {
        static const std::array<Counting<T>, 8> arr_ = {0, 1, 2, 3, 4, 5, 6, 7};
        return arr_;
    }
    static std::initializer_list< T > il()
    {
        static const std::initializer_list< T > l{0, 1, 2, 3, 4, 5, 6, 7};
        return l;
    }
    static std::initializer_list< Counting<T> > cil()
    {
        static const std::initializer_list< Counting<T> > l = {0, 1, 2, 3, 4, 5, 6, 7};
        C4_ASSERT(l.size() == 8);
        return l;
    }
};

#define _C4_DECLARE_ARCHETYPE_PROTO(ty, ...)                            \
template<>                                                              \
struct archetype_proto<ty> : public archetype_proto_base< ty, archetype_proto<ty> > \
{                                                                       \
    static std::array<ty, 8> const& arr()                               \
    {                                                                   \
        static const std::array<ty, 8> arr_{__VA_ARGS__};               \
        return arr_;                                                    \
    }                                                                   \
    static std::array<Counting<ty>, 8> const& carr()                    \
    {                                                                   \
        static const std::array<Counting<ty>, 8> arr_{__VA_ARGS__};     \
        return arr_;                                                    \
    }                                                                   \
    static std::initializer_list< ty > il()                             \
    {                                                                   \
        static const std::initializer_list< ty > l{__VA_ARGS__};        \
        return l;                                                       \
    }                                                                   \
    static std::initializer_list< Counting<ty> > cil()                  \
    {                                                                   \
        static const std::initializer_list< Counting<ty> > l{__VA_ARGS__}; \
        return l;                                                       \
    }                                                                   \
}

#define _C4_DECLARE_ARCHETYPE_PROTO_TPL1(tplparam1, ty, ...)            \
template< tplparam1 >                                                   \
struct archetype_proto< ty > : public archetype_proto_base< ty, archetype_proto<ty> > \
{                                                                       \
    static std::array<ty, 8> const& arr()                               \
    {                                                                   \
        static const std::array<ty, 8> arr_{__VA_ARGS__};               \
        return arr_;                                                    \
    }                                                                   \
    static std::array<Counting<ty>, 8> const& carr()                    \
    {                                                                   \
        static const std::array<Counting<ty>, 8> arr_{__VA_ARGS__};     \
        return arr_;                                                    \
    }                                                                   \
    static std::initializer_list< ty > il()                             \
    {                                                                   \
        static const std::initializer_list< ty > l{__VA_ARGS__};        \
        return l;                                                       \
    }                                                                   \
    static std::initializer_list< Counting<ty> > cil()                  \
    {                                                                   \
        static const std::initializer_list< Counting<ty> > l{__VA_ARGS__}; \
        return l;                                                       \
    }                                                                   \
}

_C4_DECLARE_ARCHETYPE_PROTO(std::string,
    "str0", "str1", "str2", "str3",
    "str4", "str5", "str6", "str7");

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** Resource-owning archetype */

template< class T >
struct exvec3
{
    T x, y, z;
    bool operator== (exvec3 const& that) const
    {
        return x == that.x && y == that.y && z == that.z;
    }
};
template< class String, class T >
sstream< String >& operator<< (sstream< String >& ss, exvec3<T> const& v)
{
    using char_type = typename sstream< String >::char_type;
    ss.printp(C4_TXTTY("({},{},{})", char_type), v.x, v.y, v.z);
    return ss;
}
template< class String, class T >
sstream< String >& operator>> (sstream< String >& ss, exvec3<T> & v)
{
    using char_type = typename sstream< String >::char_type;
    ss.scanp(C4_TXTTY("({},{},{})", char_type), v.x, v.y, v.z);
    return ss;
}

#define _ C4_COMMA
#define c4v(v0, v1, v2) exvec3<T>{v0 _ v1 _ v2}
_C4_DECLARE_ARCHETYPE_PROTO_TPL1(class T, exvec3<T>,
            c4v(0, 1, 2), c4v(3, 4, 5), c4v(6, 7, 8), c4v(9, 10, 11),
            c4v(100, 101, 102), c4v(103, 104, 105), c4v(106, 107, 108), c4v(109, 110, 111)
        );
#undef c4v
#undef _

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** Resource-owning archetype */
struct IdOwner
{
    static int s_current;
    int id;
    int val;

    void check() const
    {
        CHECK_UNARY(id > 0);
    }
    void check(IdOwner const& that) const
    {
        check();
        CHECK_NE(id, that.id);
    }

    IdOwner(int v = 0) { id = ++s_current; val = v; }
    ~IdOwner() { if(id > 0) --s_current; }
    IdOwner(IdOwner const& that) { id = ++s_current; val = that.val; }
    IdOwner(IdOwner     && that) { id = that.id; val = that.val; that.id = 0; }
    IdOwner& operator= (IdOwner const& that) { C4_CHECK(id > 0); --s_current; id = ++s_current; val = that.val; return *this; }
    IdOwner& operator= (IdOwner     && that) { C4_CHECK(id > 0); --s_current; id = that.id; val = that.val; that.id = 0; return *this; }
    bool operator== (IdOwner const& that) const
    {
        return val == that.val;
    }
};

_C4_DECLARE_ARCHETYPE_PROTO(IdOwner, 0, 1, 2, 3, 4, 5, 6, 7);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** Memory-owning archetype, raw mem resource calls */
template< class T >
struct MemOwner
{
    T *mem;
    // prevent initialization order problems by using a memory resource here
    MemoryResourceMalloc mr;

    void check() const
    {
        EXPECT_NE(mem, nullptr);
        check_archetype(*mem);
    }
    void check(MemOwner const& that) const
    {
        EXPECT_NE(mem, that.mem);
    }

    ~MemOwner()
    {
        if(!mem) return;
        mem->~T();
        mr.deallocate(mem, sizeof(T), alignof(T));
        mem = nullptr;
    }

    MemOwner()
    {
        mem = (T*)mr.allocate(sizeof(T), alignof(T));
        new (mem) T();
    }
    template< class ...Args >
    MemOwner(varargs_t, Args && ...args)
    {
        mem = (T*)mr.allocate(sizeof(T), alignof(T));
        new (mem) T(std::forward< Args >(args)...);
    }
    MemOwner(MemOwner const& that)
    {
        mem = (T*)mr.allocate(sizeof(T), alignof(T));
        new (mem) T(*that.mem);
    }
    MemOwner(MemOwner && that)
    {
        mem = that.mem;
        that.mem = nullptr;
    }
    MemOwner& operator= (MemOwner const& that)
    {
        if(!mem)
        {
            mem = (T*)mr.allocate(sizeof(T), alignof(T));
        }
        else
        {
            mem->~T();
        }
        new (mem) T(*that.mem);
        return *this;
    }
    MemOwner& operator= (MemOwner && that)
    {
        if(mem)
        {
            mem->~T();
            mr.deallocate(mem, sizeof(T), alignof(T));
        }
        mem = that.mem;
        that.mem = nullptr;
        return *this;
    }
    bool operator== (MemOwner const& that) const
    {
        return *mem == *that.mem;
    }
};

#define _ C4_COMMA
#define c4v(which) MemOwner<T>{varargs _ archetype_proto<T>::get(which)}
_C4_DECLARE_ARCHETYPE_PROTO_TPL1(class T, MemOwner<T>,
            c4v(0), c4v(1), c4v(2), c4v(3),
            c4v(4), c4v(5), c4v(6), c4v(7)
        );
#undef c4v
#undef _


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** Memory-owning archetype, with allocator */
template< class T >
struct MemOwnerAlloc
{
    T *mem;
    // prevent initialization order problems by using a memory resource here
    MemoryResourceMalloc mr;
    c4::Allocator< T > m_alloc;
    using alloc_traits = std::allocator_traits< c4::Allocator< T > >;

    void check() const
    {
        EXPECT_NE(mem, nullptr);
        check_archetype(*mem);
    }
    void check(MemOwnerAlloc const& that) const
    {
        check();
        EXPECT_NE(mem, that.mem);
    }

    void free()
    {
        alloc_traits::destroy(m_alloc, mem);
        alloc_traits::deallocate(m_alloc, mem, 1);
        mem = nullptr;
    }
    ~MemOwnerAlloc()
    {
        if(!mem) return;
        free();
    }

    MemOwnerAlloc() : m_alloc(&mr)
    {
        C4_ASSERT(m_alloc.resource() == &mr);
        mem = alloc_traits::allocate(m_alloc, 1);
        alloc_traits::construct(m_alloc, mem);
    }
    template< class ...Args >
    MemOwnerAlloc(varargs_t, Args && ...args) : m_alloc(&mr)
    {
        mem = alloc_traits::allocate(m_alloc, 1);
        alloc_traits::construct(m_alloc, mem, std::forward< Args >(args)...);
    }

    MemOwnerAlloc(MemOwnerAlloc const& that) : m_alloc(&mr)
    {
        mem = alloc_traits::allocate(m_alloc, 1);
        alloc_traits::construct(m_alloc, mem, *that.mem);
    }
    MemOwnerAlloc(MemOwnerAlloc && that) : m_alloc(&mr)
    {
        mem = that.mem;
        that.mem = nullptr;
    }

    MemOwnerAlloc& operator= (MemOwnerAlloc const& that)
    {
        if(!mem)
        {
            mem = alloc_traits::allocate(m_alloc, 1);
        }
        else
        {
            mem->~T();
        }
        alloc_traits::construct(m_alloc, mem, *that.mem);
        return *this;
    }
    MemOwnerAlloc& operator= (MemOwnerAlloc && that)
    {
        if(mem)
        {
            free();
        }
        mem = that.mem;
        that.mem = nullptr;
        return *this;
    }

    bool operator== (MemOwnerAlloc const& that) const
    {
        return *mem == *that.mem;
    }
};

#define _ C4_COMMA
#define c4v(which) MemOwnerAlloc<T>{varargs _ archetype_proto<T>::get(which)}
_C4_DECLARE_ARCHETYPE_PROTO_TPL1(class T, MemOwnerAlloc<T>,
            c4v(0), c4v(1), c4v(2), c4v(3),
            c4v(4), c4v(5), c4v(6), c4v(7)
        );
#undef c4v
#undef _

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** base class archetype */
struct Base
{
    virtual ~Base() = default;
protected:
    Base() = default;
};
/** derived class archetype */
struct Derived : public Base
{

};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template< class T >
struct InsidePtr
{
    T a;
    T b;
    T c;
    T *ptr;

    InsidePtr(int which = 0) : a(), b(), c(), ptr(&a + (which % 3)) {}
    InsidePtr(InsidePtr const& that) : a(that.a), b(that.b), c(that.c), ptr(&a + (that.ptr - &that.a)) {}
    InsidePtr(InsidePtr     && that) : a(std::move(that.a)), b(std::move(that.b)), c(std::move(that.c)), ptr(&a + (that.ptr - &that.a)) { that.ptr = nullptr; }
    InsidePtr& operator= (InsidePtr const& that) { a = (that.a); b = (that.b); c = (that.c); ptr = (&a + (that.ptr - &that.a)); return *this; }
    InsidePtr& operator= (InsidePtr     && that) { a = std::move(that.a); b = std::move(that.b); c = std::move(that.c); ptr = (&a + (that.ptr - &that.a)); that.ptr = nullptr; return *this; }
    ~InsidePtr() { EXPECT_TRUE(ptr == &a || ptr == &b || ptr == &c || ptr == nullptr); }

    void check() const
    {
        EXPECT_TRUE(ptr == &a || ptr == &b || ptr == &c);
    }
    void check(InsidePtr const& that) const
    {
        check();
        EXPECT_EQ(ptr - &a, that.ptr - &that.a);
    }
    bool operator== (InsidePtr const& that) const
    {
        return that.a == a && that.b == b && that.c == c && (ptr - &a) == (that.ptr - &that.a);
    }

};

#define _ C4_COMMA
_C4_DECLARE_ARCHETYPE_PROTO_TPL1(class T, InsidePtr<T>,
            0, 1, 2, 3, 4, 5, 6, 7
        );
#undef _

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#   define CALL_FOR_SCALAR_ARCHETYPES(mcr)      \
    mcr(int  , int)                             \
    mcr(uint64_t , uint64_t)

#   define CALL_FOR_CONTAINEE_ARCHETYPES(mcr)                           \
    CALL_FOR_SCALAR_ARCHETYPES(mcr)                                     \
    mcr(MemOwnerAlloc_std_string   , archetypes::MemOwnerAlloc<std::string>)



using scalars_quick = std::tuple<int, uint64_t>;
using scalars = std::tuple<
    char, wchar_t, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double
>;
using containees_quick = std::tuple<
    int,
    uint64_t,
    archetypes::MemOwnerAlloc<std::string>
>;
using containees = std::tuple<
    char, wchar_t, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t, uint64_t, float, double,
    archetypes::exvec3<int>,
    archetypes::exvec3<float>,
    archetypes::IdOwner,
    archetypes::MemOwner<int>,
    archetypes::MemOwner<std::string>,
    archetypes::MemOwnerAlloc<int>,
    archetypes::MemOwnerAlloc<std::string>,
    archetypes::InsidePtr<int>,
    archetypes::InsidePtr<std::string>
>;


#ifdef __clang__
#   pragma clang diagnostic pop
#endif

} // namespace archetypes
} // namespace c4

#endif // _C4_TEST_ARCHETYPES_HPP_
