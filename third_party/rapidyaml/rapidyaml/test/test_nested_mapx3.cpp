#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define NESTED_MAPX3_CASES \
    "nested map x3, explicit",               \
        "nested map x3"


CASE_GROUP(NESTED_MAPX3)
{
    APPEND_CASES(

C("nested map x3, explicit",
R"({
  foo0: {
     foo1: {foo2: 000, bar2: 001, baz2: 002},
     bar1: {foo2: 010, bar2: 011, baz2: 012},
     baz1: {foo2: 020, bar2: 021, baz2: 022}
  },
  bar0: {
     foo1: {foo2: 100, bar2: 101, baz2: 102},
     bar1: {foo2: 110, bar2: 111, baz2: 112},
     baz1: {foo2: 120, bar2: 121, baz2: 122}
  },
  baz0: {
     foo1: {foo2: 200, bar2: 201, baz2: 202},
     bar1: {foo2: 210, bar2: 211, baz2: 212},
     baz1: {foo2: 220, bar2: 221, baz2: 222}
  }
})",
    L{
      N{"foo0", L{
         N{"foo1", L{N{"foo2", "000"}, N{"bar2", "001"}, N{"baz2", "002"}}},
         N{"bar1", L{N{"foo2", "010"}, N{"bar2", "011"}, N{"baz2", "012"}}},
         N{"baz1", L{N{"foo2", "020"}, N{"bar2", "021"}, N{"baz2", "022"}}} }},
      N{"bar0", L{
         N{"foo1", L{N{"foo2", "100"}, N{"bar2", "101"}, N{"baz2", "102"}}},
         N{"bar1", L{N{"foo2", "110"}, N{"bar2", "111"}, N{"baz2", "112"}}},
         N{"baz1", L{N{"foo2", "120"}, N{"bar2", "121"}, N{"baz2", "122"}}} }},
      N{"baz0", L{
         N{"foo1", L{N{"foo2", "200"}, N{"bar2", "201"}, N{"baz2", "202"}}},
         N{"bar1", L{N{"foo2", "210"}, N{"bar2", "211"}, N{"baz2", "212"}}},
         N{"baz1", L{N{"foo2", "220"}, N{"bar2", "221"}, N{"baz2", "222"}}} }},
          }
),

C("nested map x3",
R"(
foo0:
  foo1:
    foo2: 000
    bar2: 001
    baz2: 002
  bar1:
    foo2: 010
    bar2: 011
    baz2: 012
  baz1:
    foo2: 020
    bar2: 021
    baz2: 022
bar0:
  foo1:
    foo2: 100
    bar2: 101
    baz2: 102
  bar1:
    foo2: 110
    bar2: 111
    baz2: 112
  baz1:
    foo2: 120
    bar2: 121
    baz2: 122
baz0:
  foo1:
    foo2: 200
    bar2: 201
    baz2: 202
  bar1:
    foo2: 210
    bar2: 211
    baz2: 212
  baz1:
    foo2: 220
    bar2: 221
    baz2: 222
)",
    L{
      N{"foo0", L{
         N{"foo1", L{N{"foo2", "000"}, N{"bar2", "001"}, N{"baz2", "002"}}},
         N{"bar1", L{N{"foo2", "010"}, N{"bar2", "011"}, N{"baz2", "012"}}},
         N{"baz1", L{N{"foo2", "020"}, N{"bar2", "021"}, N{"baz2", "022"}}} }},
      N{"bar0", L{
         N{"foo1", L{N{"foo2", "100"}, N{"bar2", "101"}, N{"baz2", "102"}}},
         N{"bar1", L{N{"foo2", "110"}, N{"bar2", "111"}, N{"baz2", "112"}}},
         N{"baz1", L{N{"foo2", "120"}, N{"bar2", "121"}, N{"baz2", "122"}}} }},
      N{"baz0", L{
         N{"foo1", L{N{"foo2", "200"}, N{"bar2", "201"}, N{"baz2", "202"}}},
         N{"bar1", L{N{"foo2", "210"}, N{"bar2", "211"}, N{"baz2", "212"}}},
         N{"baz1", L{N{"foo2", "220"}, N{"bar2", "221"}, N{"baz2", "222"}}} }},
      }
),
    )
}


INSTANTIATE_GROUP(NESTED_MAPX3)

} // namespace yml
} // namespace c4
