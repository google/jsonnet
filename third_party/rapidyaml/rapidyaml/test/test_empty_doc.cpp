#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define EMPTY_DOC_CASES                             \
    "one empty doc",                                \
    "one empty doc, explicit termination",          \
    "two empty docs",                               \
    "two empty docs, with termination"

CASE_GROUP(EMPTY_DOC)
{
    APPEND_CASES(
C("one empty doc",
R"(---
)",
    N(STREAM, L{DOC})
),

C("one empty doc, explicit termination",
R"(---
...
)",
    N(STREAM, L{DOC})
),

C("two empty docs",
R"(---
---
)",
    N(STREAM, L{DOC, DOC})
),

C("two empty docs, with termination",
R"(---
...
---
)",
    N(STREAM, L{DOC, DOC})
),
       );
}

INSTANTIATE_GROUP(EMPTY_DOC)

} // namespace yml
} // namespace c4
