#include "./test_group.hpp"

namespace c4 {
namespace yml {


#define MAP_OF_SEQ_CASES \
    "map of empty seqs", \
    "map of seqs, one line", \
    "map of seqs",           \
    "map of seqs, not indented", \
    "map of seqs, not indented, more", \
    "map of seqs, next line",\
    "map of seqs, next line without space",\
    "map of seqs, deal with unk"


CASE_GROUP(MAP_OF_SEQ)
{
    APPEND_CASES(

C("map of empty seqs",
R"({foo: [], bar: [], baz: []})",
     L{
         N(KEYSEQ, "foo", L()),
         N(KEYSEQ, "bar", L()),
         N(KEYSEQ, "baz", L()),
     }
),

C("map of seqs, one line",
R"({men: [John Smith, Bill Jones], women: [Mary Smith, Susan Williams]})",
     L{
         N("men", L{N{"John Smith"}, N{"Bill Jones"}}),
         N("women", L{N{"Mary Smith"}, N{"Susan Williams"}})
     }
),

C("map of seqs",
R"(
men:
  - John Smith
  - Bill Jones
women:
  - Mary Smith
  - Susan Williams
)",
     L{
         N("men", L{N{"John Smith"}, N{"Bill Jones"}}),
         N("women", L{N{"Mary Smith"}, N{"Susan Williams"}})
     }
),

C("map of seqs, not indented",
R"(
men:
- John Smith
- Bill Jones
women:
- Mary Smith
- Susan Williams
)",
     L{
         N("men", L{N{"John Smith"}, N{"Bill Jones"}}),
         N("women", L{N{"Mary Smith"}, N{"Susan Williams"}})
     }
),

C("map of seqs, not indented, more",
R"(
product:
- sku: BL4438H
  quantity: 1
  description: Super Hoop
  price: 2392.00  # jumping one level here would be wrong.
tax: 1234.5       # we must jump two levels
product2:
  subproduct1:
  - sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00  # jumping one level here would be wrong.
  subproduct2:
  - sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00  # jumping one level here would be wrong.
  tax2: 789.10      # we must jump two levels
tax3: 1234.5
product3:
  subproduct1:
  - sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00  # jumping one level here would be wrong.
  subproduct2:
  - sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00  # jumping one level here would be wrong.
  # a comment here, will it ruin parsing?
  tax2: 789.10      # we must jump two levels
tax4: 1234.5
product4:
  subproduct1:
  - sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00  # jumping one level here would be wrong.
  subproduct2:
  - sku: BL4438H
    quantity: 1
    description: Super Hoop
    price: 2392.00  # jumping one level here would be wrong.
 # what about here?
  tax2: 789.10      # we must jump two levels
tax5: 1234.5
)",
L{
    N("product", L{
       N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
    }),
    N("tax", "1234.5"),
    N("product2", L{
      N("subproduct1", L{
        N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
      }),
      N("subproduct2", L{
        N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
      }),
      N("tax2", "789.10"),
    }),
    N("tax3", "1234.5"),
    N("product3", L{
      N("subproduct1", L{
        N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
      }),
      N("subproduct2", L{
        N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
      }),
      N("tax2", "789.10"),
    }),
    N("tax4", "1234.5"),
    N("product4", L{
      N("subproduct1", L{
        N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
      }),
      N("subproduct2", L{
        N(L{N("sku", "BL4438H"), N("quantity", "1"), N("description", "Super Hoop"), N("price", "2392.00")}),
      }),
      N("tax2", "789.10"),
    }),
    N("tax5", "1234.5"),
}),

C("map of seqs, next line",
R"(
men:
  - 
    John Smith
  - 
    Bill Jones
women:
  - 
    Mary Smith
  - 
    Susan Williams
)",
     L{
         N("men", L{N{"John Smith"}, N{"Bill Jones"}}),
         N("women", L{N{"Mary Smith"}, N{"Susan Williams"}})
     }
),

C("map of seqs, next line without space",
R"(
men:
  -
    John Smith
  -
    Bill Jones
women:
  -
    Mary Smith
  -
    Susan Williams
)",
     L{
         N("men", L{N{"John Smith"}, N{"Bill Jones"}}),
         N("women", L{N{"Mary Smith"}, N{"Susan Williams"}})
     }
),

C("map of seqs, deal with unk",
R"(
skip_commits:
  files:
    - a
    - b
    - c
    - d
    - e
)",
L{
  N("skip_commits", L{N("files",
    L{N("a"), N("b"), N("c"), N("d"), N("e")}
  )}),
}
),
    )
}

INSTANTIATE_GROUP(MAP_OF_SEQ)

} // namespace yml
} // namespace c4
