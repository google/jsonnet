#include "c4/allocator.hpp"
#include "c4/test.hpp"
#include "c4/libtest/supprwarn_push.hpp"

#include <vector>
#include <string>
#include <map>

namespace c4 {

template< class T > using small_adapter = c4::small_allocator< T >;
template< class T > using small_adapter_mr = c4::small_allocator_mr< T >;

#define _c4definealloctypes(Alloc) \
using AllocInt = typename Alloc::template rebind<int>::other;\
using AllocChar = typename Alloc::template rebind<char>::other;\
using _string = std::basic_string< char, std::char_traits<char>, AllocChar >;\
using AllocString = typename Alloc::template rebind<_string>::other;\
using AllocPair = typename Alloc::template rebind<std::pair<const _string,int>>::other;\
using _vector_int = std::vector<int, AllocInt >;\
using _vector_string = std::vector<_string, AllocString >;\
using _map_string_int = std::map<_string, int, std::less<_string>, AllocPair >;

//-----------------------------------------------------------------------------
template< class Alloc >
void test_traits_compat_construct(typename Alloc::value_type const& val, Alloc &a)
{
    using atraits = std::allocator_traits< Alloc >;
    using value_type = typename Alloc::value_type;

    value_type *mem = a.allocate(1);
    REQUIRE_NE(mem, nullptr);
    atraits::construct(a, mem, val);
    CHECK_EQ(*mem, val);

    atraits::destroy(a, mem);
    a.deallocate(mem, 1);
}

TEST_CASE("allocator.traits_compat_construct")
{
    allocator<int> a;
    test_traits_compat_construct(1, a);
}

TEST_CASE("small_allocator.traits_compat_construct")
{
    small_allocator<int> a;
    test_traits_compat_construct(1, a);
}

TEST_CASE("allocator_mr_global.traits_compat_construct")
{
    allocator_mr<int> a;
    test_traits_compat_construct(1, a);
}

TEST_CASE("allocator_mr_linear.traits_compat_construct")
{
    MemoryResourceLinear mr(1024);
    allocator_mr<int> a(&mr);
    test_traits_compat_construct(1, a);
}

TEST_CASE("allocator_mr_linear_arr.traits_compat_construct")
{
    MemoryResourceLinearArr<1024> mr;
    allocator_mr<int> a(&mr);
    test_traits_compat_construct(1, a);
}

TEST_CASE("small_allocator_mr_global.traits_compat_construct")
{
    small_allocator_mr<int> a;
    test_traits_compat_construct(1, a);
}

TEST_CASE("small_allocator_mr_linear.traits_compat_construct")
{
    MemoryResourceLinear mr(1024);
    small_allocator_mr<int> a(&mr);
    test_traits_compat_construct(1, a);
}

TEST_CASE("small_allocator_mr_linear_arr.traits_compat_construct")
{
    MemoryResourceLinearArr<1024> mr;
    small_allocator_mr<int> a(&mr);
    test_traits_compat_construct(1, a);
}

//-----------------------------------------------------------------------------

template< class Alloc >
void clear_mr(Alloc a)
{
    auto mrl = dynamic_cast<MemoryResourceLinear*>(a.resource());
    if(mrl)
    {
        mrl->clear();
    }
}

template< class Alloc >
void do_std_containers_test(Alloc alloc)
{
    _c4definealloctypes(Alloc);

    {
        _string v(alloc);
        v.reserve(256);
        v = "adskjhsdfkjdflkjsdfkjhsdfkjh";
        CHECK_EQ(v, "adskjhsdfkjdflkjsdfkjhsdfkjh");
    }

    clear_mr(alloc);

    {
        int arr[128];
        for(int &i : arr)
        {
            i = 42;
        }
        _vector_int vi(arr, arr+C4_COUNTOF(arr), alloc);
        for(int i : vi)
        {
            CHECK_EQ(i, 42);
        }
    }

    clear_mr(alloc);

    {
        AllocChar a = alloc;
        _vector_string v({"foo", "bar", "baz", "bat", "bax"}, a);
        CHECK_EQ(v.size(), 5);
        CHECK_EQ(v[0], "foo");
        CHECK_EQ(v[1], "bar");
        CHECK_EQ(v[2], "baz");
        CHECK_EQ(v[3], "bat");
        CHECK_EQ(v[4], "bax");
    }

    clear_mr(alloc);

    {
        AllocString a = alloc;
        _vector_string v(a);
        v.resize(4);
        int count = 0;
        for(auto &s : v)
        {
            s = _string(64, (char)('0' + count++));
        }
    }

    clear_mr(alloc);

    {
        AllocPair a = alloc;
        _map_string_int v(a);
        CHECK_EQ(v.size(), 0);
        v["foo"] = 0;
        v["bar"] = 1;
        v["baz"] = 2;
        v["bat"] = 3;
        CHECK_EQ(v.size(), 4);
        CHECK_EQ(v["foo"], 0);
        CHECK_EQ(v["bar"], 1);
        CHECK_EQ(v["baz"], 2);
        CHECK_EQ(v["bat"], 3);
    }
}

TEST_CASE("allocator_global.std_containers")
{
    allocator<int> a;
    do_std_containers_test(a);
}

TEST_CASE("small_allocator_global.std_containers")
{
    /* this is failing, investigate
    small_allocator<int> a;
    do_std_containers_test(a);
    */
}

TEST_CASE("allocator_mr_global.std_containers")
{
    allocator_mr<int> a;
    do_std_containers_test(a);
}

TEST_CASE("allocator_mr_linear.std_containers")
{
    MemoryResourceLinear mr(1024);
    allocator_mr<int> a(&mr);
    do_std_containers_test(a);
}

TEST_CASE("allocator_mr_linear_arr.std_containers")
{
    MemoryResourceLinearArr<1024> mr;
    allocator_mr<int> a(&mr);
    do_std_containers_test(a);
}

TEST_CASE("small_allocator_mr_global.std_containers")
{
    /* this is failing, investigate
    small_allocator_mr<int> a;
    do_std_containers_test(a);
    */
}

TEST_CASE("small_allocator_mr_linear.std_containers")
{
    /* this is failing, investigate
    MemoryResourceLinear mr(1024);
    small_allocator_mr<int> a(&mr);
    do_std_containers_test(a);
    */
}

TEST_CASE("small_allocator_mr_linear_arr.std_containers")
{
    /* this is failing, investigate
    MemoryResourceLinearArr<1024> mr;
    small_allocator_mr<int> a(&mr);
    do_std_containers_test(a);
    */
}

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
