#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define GENERIC_SEQ_CASES                       \
    "generic seq v0",                              \
        "generic seq v1"

CASE_GROUP(GENERIC_SEQ)
{
    APPEND_CASES(

C("generic seq v0",
R"(
- item 1
- item 2
- - item 3.1
  - item 3.2
- key 1: value 1
  key 2: value 2
)",
  L{
      N("item 1"),
      N("item 2"),
      N(L{N("item 3.1"), N("item 3.2")}),
      N(L{N("key 1", "value 1"), N("key 2", "value 2")})
  }
),

C("generic seq v1",
R"(
- item 1
- item 2
-
  - item 3.1
  - item 3.2
-
  key 1: value 1
  key 2: value 2
)",
  L{
      N("item 1"),
      N("item 2"),
      N(L{N("item 3.1"), N("item 3.2")}),
      N(L{N("key 1", "value 1"), N("key 2", "value 2")})
  }
),
    )
}

INSTANTIATE_GROUP(GENERIC_SEQ)

} // namespace yml
} // namespace c4
