#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define EMPTY_MAP_CASES                         \
    "empty map, explicit",                      \
    "empty map, explicit, whitespace",          \
    "empty map, multiline",                     \
    "empty map, multilines"


CASE_GROUP(EMPTY_MAP)
{
    APPEND_CASES(


C("empty map, explicit",
"{}",
    MAP
),


C("empty map, explicit, whitespace",
" {}",
    MAP
),


C("empty map, multiline",
R"({

}
)",
    MAP
),


C("empty map, multilines",
R"({
# ksjdfkjhsdfkjhsdfkjh


}
)",
    MAP
),
    );
}

INSTANTIATE_GROUP(EMPTY_MAP)

} // namespace yml
} // namespace c4
