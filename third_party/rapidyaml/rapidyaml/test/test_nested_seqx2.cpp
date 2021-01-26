#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define NESTED_SEQX2_CASES                                          \
"nested seq x2, empty, oneline",                                    \
"nested seq x2, explicit, same line",                               \
"nested seq x2, explicit first+last level, same line, no spaces",   \
"nested seq x2, explicit",                                          \
"nested seq x2",                                                    \
"nested seq x2, next line",                                         \
"nested seq x2, all next line",                                     \
"nested seq x2, implicit first, explicit last level"

CASE_GROUP(NESTED_SEQX2)
{
    APPEND_CASES(

C("nested seq x2, empty, oneline",
R"([[], [], []])",
    L{SEQ, SEQ, SEQ}
),

C("nested seq x2, explicit, same line",
R"([[00, 01, 02], [10, 11, 12], [20, 21, 22]])",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),

C("nested seq x2, explicit first+last level, same line, no spaces",
R"([[00,01,02],[10,11,12],[20,21,22]])",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),

C("nested seq x2, explicit",
R"([
[00, 01, 02],
[10, 11, 12],
[20, 21, 22],
])",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),

C("nested seq x2",
R"(
- - 00
  - 01
  - 02
- - 10
  - 11
  - 12
- - 20
  - 21
  - 22
)",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),

C("nested seq x2, next line",
R"(
-
  - 00
  - 01
  - 02
-
  - 10
  - 11
  - 12
-
  - 20
  - 21
  - 22
)",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),

C("nested seq x2, all next line",
R"(
-
  -
    00
  -
    01
  -
    02
-
  -
    10
  -
    11
  -
    12
-
  -
    20
  -
    21
  -
    22
)",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),

C("nested seq x2, implicit first, explicit last level",
R"(
- [00, 01, 02]
- [10, 11, 12]
- [20, 21, 22]
)",
    L{
      N{L{N{"00"}, N{"01"}, N{"02"}}},
      N{L{N{"10"}, N{"11"}, N{"12"}}},
      N{L{N{"20"}, N{"21"}, N{"22"}}},
          }
),
    )
}


INSTANTIATE_GROUP(NESTED_SEQX2)

} // namespace yml
} // namespace c4
