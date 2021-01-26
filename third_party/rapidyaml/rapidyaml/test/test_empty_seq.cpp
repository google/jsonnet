#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define EMPTY_SEQ_CASES                     \
    "empty seq, explicit",                  \
    "empty seq, explicit, whitespace",      \
    "empty seq, multiline",                 \
    "empty seq, multilines"

CASE_GROUP(EMPTY_SEQ)
{
    APPEND_CASES(


C("empty seq, explicit",
"[]",
    SEQ
),


C("empty seq, explicit, whitespace",
" []",
    SEQ
),


C("empty seq, multiline",
R"([
]
)",
    SEQ
),

C("empty seq, multilines",
R"([
# ksjdfkjhsdfkjhsdfkjh


]
)",
    SEQ
),
    );
}

INSTANTIATE_GROUP(EMPTY_SEQ)

} // namespace yml
} // namespace c4
