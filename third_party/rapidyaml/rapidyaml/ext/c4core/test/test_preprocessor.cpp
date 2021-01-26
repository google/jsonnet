#include "c4/preprocessor.hpp"
#include "c4/language.hpp"

#ifdef WE_LL_GET_THERE___MSVC_CANT_HANDLE_THE_FOREACH_MACRO___NEEDS_TO_BE_FIXED
#include <string>
#include <map>

struct SomeStruct
{
    int32_t a;
    int32_t b;
    int32_t c;
    int32_t d;
};

TEST(TestForEach, print_offsets)
{
#define M_OFFS_(structure, field) m[#field] = offsetof(structure, field)
#define M_OFFS(field) M_OFFS_(SomeStruct, field)

    std::map< std::string, size_t > m;

    C4_FOR_EACH(M_OFFS, a, b, c);
    C4_FOR_EACH(M_OFFS, d);

    EXPECT_EQ(m["a"], 0);
    EXPECT_EQ(m["b"], 4);
    EXPECT_EQ(m["c"], 8);
    EXPECT_EQ(m["d"], 12);
}

//-----------------------------------------------------------------------------
// C4_BEGIN_NAMESPACE()/C4_END_NAMESPACE() are implemented with C4_FOR_EACH().
// Test these here too.

namespace a, b, c {
int a_var = 0;
} // namespace c, b
int var = 1; // a::var
namespace b {
int var = 2; // a::b::var
namespace c {
int var = 3; // a::b::c::var
} // namespace c, b, a

TEST(TestForEach, begin_end_namespace)
{
    EXPECT_EQ(a::b::c::a_var, 0);
    EXPECT_EQ(a::var, 1);
    EXPECT_EQ(a::b::var, 2);
    EXPECT_EQ(a::b::c::var, 3);
}
#endif
