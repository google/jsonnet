#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define NESTED_MAPX4_CASES                      \
        "nested map x4, explicit",              \
        "nested map x4"

CASE_GROUP(NESTED_MAPX4)
{
    APPEND_CASES(

C("nested map x4, explicit",
R"({
  foo0: {
     foo1: { foo2: {foo3: 0000, bar3: 0001, baz3: 0002}, bar2: {foo3: 0010, bar3: 0011, baz3: 0012}, baz2: {foo3: 0020, bar3: 0021, baz3: 0022} },
     bar1: { foo2: {foo3: 0100, bar3: 0101, baz3: 0102}, bar2: {foo3: 0110, bar3: 0111, baz3: 0112}, baz2: {foo3: 0120, bar3: 0121, baz3: 0122} },
     baz1: { foo2: {foo3: 0200, bar3: 0201, baz3: 0202}, bar2: {foo3: 0210, bar3: 0211, baz3: 0212}, baz2: {foo3: 0220, bar3: 0221, baz3: 0222} },
  },
  bar0: {
     foo1: { foo2: {foo3: 1000, bar3: 1001, baz3: 1002}, bar2: {foo3: 1010, bar3: 1011, baz3: 1012}, baz2: {foo3: 1020, bar3: 1021, baz3: 1022} },
     bar1: { foo2: {foo3: 1100, bar3: 1101, baz3: 1102}, bar2: {foo3: 1110, bar3: 1111, baz3: 1112}, baz2: {foo3: 1120, bar3: 1121, baz3: 1122} },
     baz1: { foo2: {foo3: 1200, bar3: 1201, baz3: 1202}, bar2: {foo3: 1210, bar3: 1211, baz3: 1212}, baz2: {foo3: 1220, bar3: 1221, baz3: 1222} },
  },
  baz0: {
     foo1: { foo2: {foo3: 2000, bar3: 2001, baz3: 2002}, bar2: {foo3: 2010, bar3: 2011, baz3: 2012}, baz2: {foo3: 2020, bar3: 2021, baz3: 2022} },
     bar1: { foo2: {foo3: 2100, bar3: 2101, baz3: 2102}, bar2: {foo3: 2110, bar3: 2111, baz3: 2112}, baz2: {foo3: 2120, bar3: 2121, baz3: 2122} },
     baz1: { foo2: {foo3: 2200, bar3: 2201, baz3: 2202}, bar2: {foo3: 2210, bar3: 2211, baz3: 2212}, baz2: {foo3: 2220, bar3: 2221, baz3: 2222} },
  },
})",
    L{
      N("foo0", L{
         N("foo1", L{N("foo2", L{N("foo3", "0000"), N("bar3", "0001"), N("baz3", "0002")}),  N("bar2", L{N("foo3", "0010"), N("bar3", "0011"), N("baz3", "0012")}),  N("baz2", L{N("foo3", "0020"), N("bar3", "0021"), N("baz3", "0022")})}),
         N("bar1", L{N("foo2", L{N("foo3", "0100"), N("bar3", "0101"), N("baz3", "0102")}),  N("bar2", L{N("foo3", "0110"), N("bar3", "0111"), N("baz3", "0112")}),  N("baz2", L{N("foo3", "0120"), N("bar3", "0121"), N("baz3", "0122")})}),
         N("baz1", L{N("foo2", L{N("foo3", "0200"), N("bar3", "0201"), N("baz3", "0202")}),  N("bar2", L{N("foo3", "0210"), N("bar3", "0211"), N("baz3", "0212")}),  N("baz2", L{N("foo3", "0220"), N("bar3", "0221"), N("baz3", "0222")})}),
      }),
      N("bar0", L{
         N("foo1", L{N("foo2", L{N("foo3", "1000"), N("bar3", "1001"), N("baz3", "1002")}),  N("bar2", L{N("foo3", "1010"), N("bar3", "1011"), N("baz3", "1012")}),  N("baz2", L{N("foo3", "1020"), N("bar3", "1021"), N("baz3", "1022")})}),
         N("bar1", L{N("foo2", L{N("foo3", "1100"), N("bar3", "1101"), N("baz3", "1102")}),  N("bar2", L{N("foo3", "1110"), N("bar3", "1111"), N("baz3", "1112")}),  N("baz2", L{N("foo3", "1120"), N("bar3", "1121"), N("baz3", "1122")})}),
         N("baz1", L{N("foo2", L{N("foo3", "1200"), N("bar3", "1201"), N("baz3", "1202")}),  N("bar2", L{N("foo3", "1210"), N("bar3", "1211"), N("baz3", "1212")}),  N("baz2", L{N("foo3", "1220"), N("bar3", "1221"), N("baz3", "1222")})}),
      }),
      N("baz0", L{
         N("foo1", L{N("foo2", L{N("foo3", "2000"), N("bar3", "2001"), N("baz3", "2002")}),  N("bar2", L{N("foo3", "2010"), N("bar3", "2011"), N("baz3", "2012")}),  N("baz2", L{N("foo3", "2020"), N("bar3", "2021"), N("baz3", "2022")})}),
         N("bar1", L{N("foo2", L{N("foo3", "2100"), N("bar3", "2101"), N("baz3", "2102")}),  N("bar2", L{N("foo3", "2110"), N("bar3", "2111"), N("baz3", "2112")}),  N("baz2", L{N("foo3", "2120"), N("bar3", "2121"), N("baz3", "2122")})}),
         N("baz1", L{N("foo2", L{N("foo3", "2200"), N("bar3", "2201"), N("baz3", "2202")}),  N("bar2", L{N("foo3", "2210"), N("bar3", "2211"), N("baz3", "2212")}),  N("baz2", L{N("foo3", "2220"), N("bar3", "2221"), N("baz3", "2222")})}),
      })
    }
),

C("nested map x4",
R"(
foo0:
  foo1:
    foo2:
      foo3: 0000
      bar3: 0001
      baz3: 0002
    bar2:
      foo3: 0010
      bar3: 0011
      baz3: 0012
    baz2:
      foo3: 0020
      bar3: 0021
      baz3: 0022
  bar1:
    foo2:
      foo3: 0100
      bar3: 0101
      baz3: 0102
    bar2:
      foo3: 0110
      bar3: 0111
      baz3: 0112
    baz2:
      foo3: 0120
      bar3: 0121
      baz3: 0122
  baz1:
    foo2:
      foo3: 0200
      bar3: 0201
      baz3: 0202
    bar2:
      foo3: 0210
      bar3: 0211
      baz3: 0212
    baz2:
      foo3: 0220
      bar3: 0221
      baz3: 0222
bar0:
  foo1:
    foo2:
      foo3: 1000
      bar3: 1001
      baz3: 1002
    bar2:
      foo3: 1010
      bar3: 1011
      baz3: 1012
    baz2:
      foo3: 1020
      bar3: 1021
      baz3: 1022
  bar1:
    foo2:
      foo3: 1100
      bar3: 1101
      baz3: 1102
    bar2:
      foo3: 1110
      bar3: 1111
      baz3: 1112
    baz2:
      foo3: 1120
      bar3: 1121
      baz3: 1122
  baz1:
    foo2:
      foo3: 1200
      bar3: 1201
      baz3: 1202
    bar2:
      foo3: 1210
      bar3: 1211
      baz3: 1212
    baz2:
      foo3: 1220
      bar3: 1221
      baz3: 1222
baz0:
  foo1:
    foo2:
      foo3: 2000
      bar3: 2001
      baz3: 2002
    bar2:
      foo3: 2010
      bar3: 2011
      baz3: 2012
    baz2:
      foo3: 2020
      bar3: 2021
      baz3: 2022
  bar1:
    foo2:
      foo3: 2100
      bar3: 2101
      baz3: 2102
    bar2:
      foo3: 2110
      bar3: 2111
      baz3: 2112
    baz2:
      foo3: 2120
      bar3: 2121
      baz3: 2122
  baz1:
    foo2:
      foo3: 2200
      bar3: 2201
      baz3: 2202
    bar2:
      foo3: 2210
      bar3: 2211
      baz3: 2212
    baz2:
      foo3: 2220
      bar3: 2221
      baz3: 2222
)",
    L{
      N("foo0", L{
         N("foo1", L{N("foo2", L{N("foo3", "0000"), N("bar3", "0001"), N("baz3", "0002")}),  N("bar2", L{N("foo3", "0010"), N("bar3", "0011"), N("baz3", "0012")}),  N("baz2", L{N("foo3", "0020"), N("bar3", "0021"), N("baz3", "0022")})}),
         N("bar1", L{N("foo2", L{N("foo3", "0100"), N("bar3", "0101"), N("baz3", "0102")}),  N("bar2", L{N("foo3", "0110"), N("bar3", "0111"), N("baz3", "0112")}),  N("baz2", L{N("foo3", "0120"), N("bar3", "0121"), N("baz3", "0122")})}),
         N("baz1", L{N("foo2", L{N("foo3", "0200"), N("bar3", "0201"), N("baz3", "0202")}),  N("bar2", L{N("foo3", "0210"), N("bar3", "0211"), N("baz3", "0212")}),  N("baz2", L{N("foo3", "0220"), N("bar3", "0221"), N("baz3", "0222")})}),
      }),
      N("bar0", L{
         N("foo1", L{N("foo2", L{N("foo3", "1000"), N("bar3", "1001"), N("baz3", "1002")}),  N("bar2", L{N("foo3", "1010"), N("bar3", "1011"), N("baz3", "1012")}),  N("baz2", L{N("foo3", "1020"), N("bar3", "1021"), N("baz3", "1022")})}),
         N("bar1", L{N("foo2", L{N("foo3", "1100"), N("bar3", "1101"), N("baz3", "1102")}),  N("bar2", L{N("foo3", "1110"), N("bar3", "1111"), N("baz3", "1112")}),  N("baz2", L{N("foo3", "1120"), N("bar3", "1121"), N("baz3", "1122")})}),
         N("baz1", L{N("foo2", L{N("foo3", "1200"), N("bar3", "1201"), N("baz3", "1202")}),  N("bar2", L{N("foo3", "1210"), N("bar3", "1211"), N("baz3", "1212")}),  N("baz2", L{N("foo3", "1220"), N("bar3", "1221"), N("baz3", "1222")})}),
      }),
      N("baz0", L{
         N("foo1", L{N("foo2", L{N("foo3", "2000"), N("bar3", "2001"), N("baz3", "2002")}),  N("bar2", L{N("foo3", "2010"), N("bar3", "2011"), N("baz3", "2012")}),  N("baz2", L{N("foo3", "2020"), N("bar3", "2021"), N("baz3", "2022")})}),
         N("bar1", L{N("foo2", L{N("foo3", "2100"), N("bar3", "2101"), N("baz3", "2102")}),  N("bar2", L{N("foo3", "2110"), N("bar3", "2111"), N("baz3", "2112")}),  N("baz2", L{N("foo3", "2120"), N("bar3", "2121"), N("baz3", "2122")})}),
         N("baz1", L{N("foo2", L{N("foo3", "2200"), N("bar3", "2201"), N("baz3", "2202")}),  N("bar2", L{N("foo3", "2210"), N("bar3", "2211"), N("baz3", "2212")}),  N("baz2", L{N("foo3", "2220"), N("bar3", "2221"), N("baz3", "2222")})}),
      })
    }
),
    )
}

INSTANTIATE_GROUP(NESTED_MAPX4)

} // namespace yml
} // namespace c4
