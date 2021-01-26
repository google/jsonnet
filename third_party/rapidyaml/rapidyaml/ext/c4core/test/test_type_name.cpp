#include "c4/test.hpp"

#include "c4/type_name.hpp"

class  SomeTypeName {};
struct SomeStructName {};

namespace c4 {

class  SomeTypeNameInsideANamespace {};
struct SomeStructNameInsideANamespace {};

template<size_t N>
cspan<char> cstr(const char (&s)[N])
{
    cspan<char> o(s, N-1);
    return o;
}

TEST_CASE("type_name.intrinsic_types")
{
    CHECK_EQ(type_name<int>(), cstr("int"));
    CHECK_EQ(type_name<float>(), cstr("float"));
    CHECK_EQ(type_name<double>(), cstr("double"));
}
TEST_CASE("type_name.classes")
{
    CHECK_EQ(type_name<SomeTypeName>(), cstr("SomeTypeName"));
    CHECK_EQ(type_name<::SomeTypeName>(), cstr("SomeTypeName"));
}
TEST_CASE("type_name.structs")
{
    CHECK_EQ(type_name<SomeStructName>(), cstr("SomeStructName"));
    CHECK_EQ(type_name<::SomeStructName>(), cstr("SomeStructName"));
}
TEST_CASE("type_name.inside_namespace")
{
    CHECK_EQ(type_name<SomeTypeNameInsideANamespace>(), cstr("c4::SomeTypeNameInsideANamespace"));
    CHECK_EQ(type_name<c4::SomeTypeNameInsideANamespace>(), cstr("c4::SomeTypeNameInsideANamespace"));
    CHECK_EQ(type_name<::c4::SomeTypeNameInsideANamespace>(), cstr("c4::SomeTypeNameInsideANamespace"));

    CHECK_EQ(type_name<SomeStructNameInsideANamespace>(), cstr("c4::SomeStructNameInsideANamespace"));
    CHECK_EQ(type_name<c4::SomeStructNameInsideANamespace>(), cstr("c4::SomeStructNameInsideANamespace"));
    CHECK_EQ(type_name<::c4::SomeStructNameInsideANamespace>(), cstr("c4::SomeStructNameInsideANamespace"));
}

} // namespace c4
