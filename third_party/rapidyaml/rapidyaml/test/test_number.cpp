#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define NUMBER_CASES \
    "integer numbers, expl",\
    "integer numbers, impl",\
    "floating point numbers, expl",\
    "floating point numbers, impl"


CASE_GROUP(NUMBER)
{
    APPEND_CASES(

C("integer numbers, expl",
R"(translation: [-2, -2, 5])",
L{N("translation", L{N("-2"), N("-2"), N("5")})}
),

C("integer numbers, impl",
R"(translation:
  - -2
  - -2
  - -5
)",
L{N("translation", L{N("-2"), N("-2"), N("-5")})}
),

C("floating point numbers, expl",
R"([-2.0, -2.1, 0.1, .1, -.2, -2.e+6, -3e-6, 1.12345e+011])",
L{N("-2.0"), N("-2.1"), N("0.1"), N(".1"), N("-.2"), N("-2.e+6"), N("-3e-6"), N("1.12345e+011")}
),

C("floating point numbers, impl",
R"(
- -2.0
- -2.1
- 0.1
- .1
- -.2
- -2.e+6
- -3e-6
- 1.12345e+011
)",
L{N("-2.0"), N("-2.1"), N("0.1"), N(".1"), N("-.2"), N("-2.e+6"), N("-3e-6"), N("1.12345e+011")}
),
    )
}


INSTANTIATE_GROUP(NUMBER)

} // namespace yml
} // namespace c4
