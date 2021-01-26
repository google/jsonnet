#include "./test_group.hpp"
#include "test_case.hpp"

namespace c4 {
namespace yml {

#define SEQ_OF_MAP_CASES                                \
    "seq of empty maps, one line",                      \
    "seq of maps, one line",                            \
    "seq of maps, implicit seq, explicit maps",         \
    "seq of maps",                                      \
    "seq of maps, next line",                           \
    "seq of maps, bug #32 ex1",                         \
    "seq of maps, bug #32 ex2",                         \
    "seq of maps, bug #32 ex3",                         \
    "seq of maps, implicit map in seq",                 \
    "seq of maps, implicit map in seq, missing scalar"


CASE_GROUP(SEQ_OF_MAP)
{
    APPEND_CASES(

C("seq of empty maps, one line",
R"([{}, {}, {}])",
  L{MAP, MAP, MAP}
),

C("seq of maps, one line",
R"([{name: John Smith, age: 33}, {name: Mary Smith, age: 27}])",
  L{
      N{L{N("name", "John Smith"), N("age", "33")}},
      N{L{N("name", "Mary Smith"), N("age", "27")}}
  }
),

C("seq of maps, implicit seq, explicit maps",
R"(
- {name: John Smith, age: 33}
- {name: Mary Smith, age: 27}
)",
  L{
      N{L{N("name", "John Smith"), N("age", "33")}},
      N{L{N("name", "Mary Smith"), N("age", "27")}}
  }
),

C("seq of maps",
R"(
- name: John Smith
  age: 33
- name: Mary Smith
  age: 27
)",
  L{
      N{L{N("name", "John Smith"), N("age", "33")}},
      N{L{N("name", "Mary Smith"), N("age", "27")}}
  }
),

C("seq of maps, next line",
R"(
- 
  name:
    John Smith
  age:
    33
- 
  name: 
    Mary Smith
  age:
    27
)",
  L{
      N{L{N("name", "John Smith"), N("age", "33")}},
      N{L{N("name", "Mary Smith"), N("age", "27")}}
  }
),

C("seq of maps, bug #32 ex1",
R"(
- 'a': 1
  b: 2
)",
  L{
      N{L{N("a", "1"), N("b", "2")}}
  }
),

C("seq of maps, bug #32 ex2",
R"(
- a: 1
  b: 2
- b: 2
  'a': 1
- b: 2
  'a': 1
  c: 3
- {'a': 1, b: 2}
)",
  L{
      N{L{N("a", "1"), N("b", "2")}},
      N{L{N("b", "2"), N("a", "1")}},
      N{L{N("b", "2"), N("a", "1"), N("c", "3")}},
      N{L{N("a", "1"), N("b", "2")}},
  }
),

C("seq of maps, bug #32 ex3",
R"(
'a': 1
b: 2
b: 2
'a': 1
)",
  L{
      N("a", "1"), N("b", "2"), N("b", "2"), N("a", "1"),
  }
),

C("seq of maps, implicit map in seq",
R"('implicit block key' : [
  'implicit flow key 1' : value1,
  'implicit flow key 2' : value2,
  'implicit flow key 3' : value3,
  'implicit flow key m' : {key1: val1, key2: val2},
  'implicit flow key s' : [val1, val2],
])",
L{N("implicit block key", L{
  N(L{N("implicit flow key 1", "value1")}),
  N(L{N("implicit flow key 2", "value2")}),
  N(L{N("implicit flow key 3", "value3")}),
  N(L{N("implicit flow key m", L{N("key1", "val1"), N("key2", "val2")})}),
  N(L{N("implicit flow key s", L{N("val1"), N("val2")})}),
})}
),

C("seq of maps, implicit map in seq, missing scalar",
R"({a : [
  : foo
],
b : [
  :
foo
],
c : [
  :
,
  :
]})",
L{
  N("a", L{N(MAP, L{N("", "foo")}),}),
  N("b", L{N(MAP, L{N("", "foo")}),}),
  N("c", L{N(MAP, L{N(KEYVAL, "", /*"~"*/{})}), N(MAP, L{N(KEYVAL, "", /*"~"*/{})}),}),
}
),

    )
}

INSTANTIATE_GROUP(SEQ_OF_MAP)

} // namespace yml
} // namespace c4
