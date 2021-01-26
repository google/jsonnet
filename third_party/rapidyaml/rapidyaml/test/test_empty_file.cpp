#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define EMPTY_FILE_CASES \
    "empty0-nochars", \
    "empty0-multiline", \
    "empty0-multiline-with-comments"


CASE_GROUP(EMPTY_FILE)
{
    APPEND_CASES(
C("empty0-nochars",
"",
NOTYPE),

C("empty0-multiline", R"(


)", NOTYPE),

C("empty0-multiline-with-comments", R"(
# well hello sir, I see you are fine
# very fine thank you
# send my very best wishes
)", NOTYPE),
        );
}

INSTANTIATE_GROUP(EMPTY_FILE)

} // namespace yml
} // namespace c4
