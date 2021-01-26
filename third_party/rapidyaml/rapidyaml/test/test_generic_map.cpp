#include "./test_group.hpp"

namespace c4 {
namespace yml {


#define GENERIC_MAP_CASES \
    "generic map",\
    "seq nested in map",\
    "seq nested in map, v2"


CASE_GROUP(GENERIC_MAP)
{
    APPEND_CASES(

C("generic map",
R"(
a simple key: a value   # The KEY token is produced here.
? a complex key
: another value
a mapping:
  key 1: value 1
  key 2: value 2
a sequence:
  - item 1
  - item 2
)",
  L{
      N("a simple key", "a value"),
      N("a complex key", "another value"),
      N("a mapping", L{N("key 1", "value 1"), N("key 2", "value 2")}),
      N("a sequence", L{N("item 1"), N("item 2")}),
  }
),


C("seq nested in map",
R"(
items:
    - part_no:   A4786
      descrip:   Water Bucket (Filled)
      price:     1.47
      quantity:  4
    - part_no:   E1628
      descrip:   High Heeled "Ruby" Slippers
      size:      8
      price:     133.7
      quantity:  1
)",
L{
  N{"items", L{
    N{L{N{"part_no",   "A4786"},
        N{"descrip",   "Water Bucket (Filled)"},
        N{"price",     "1.47"},
        N{"quantity",  "4"},}},
    N{L{N{"part_no", "E1628"},
        N{"descrip",   "High Heeled \"Ruby\" Slippers"},
        N{"size",      "8"},
        N{"price",     "133.7"},
        N{"quantity",  "1"},}}}},
  }
),

C("seq nested in map, v2",
R"(
items:
    -
      part_no:   A4786
      descrip:   Water Bucket (Filled)
      price:     1.47
      quantity:  4
    -
      part_no:   E1628
      descrip:   High Heeled "Ruby" Slippers
      size:      8
      price:     133.7
      quantity:  1
)",
L{
  N{"items", L{
    N{L{N{"part_no",   "A4786"},
        N{"descrip",   "Water Bucket (Filled)"},
        N{"price",     "1.47"},
        N{"quantity",  "4"},}},
    N{L{N{"part_no", "E1628"},
        N{"descrip",   "High Heeled \"Ruby\" Slippers"},
        N{"size",      "8"},
        N{"price",     "133.7"},
        N{"quantity",  "1"},}}}},
  }
),

    )
}

INSTANTIATE_GROUP(GENERIC_MAP)

} // namespace yml
} // namespace c4
