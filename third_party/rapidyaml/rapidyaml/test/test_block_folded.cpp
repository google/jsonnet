#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define BLOCK_FOLDED_CASES \
    "block folded as seq val, implicit indentation 2", \
    "block folded as map val, implicit indentation 2",\
    "block folded as map val, implicit indentation 2, chomp=keep",\
    "block folded as map val, implicit indentation 2, chomp=strip",\
    "block folded as map val, implicit indentation 3",\
    "block folded as map val, implicit indentation 4",\
    "block folded as map val, implicit indentation 9",\
    "block folded as map val, explicit indentation 2",\
    "block folded as map val, explicit indentation 2, chomp=keep",\
    "block folded as map val, explicit indentation 2, chomp=strip",\
    "block folded as map val, explicit indentation 3",\
    "block folded as map val, explicit indentation 4",\
    "block folded as map val, explicit indentation 9"


CASE_GROUP(BLOCK_FOLDED)
{
    APPEND_CASES(

C("block folded as seq val, implicit indentation 2",
R"(
- >
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
- another val
)",
  L{
    N("Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another val")
  }
),

C("block folded as map val, implicit indentation 2",
R"(
example: >
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, implicit indentation 2, chomp=keep",
R"(
example: >+
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n\n\n"),
    N("another", "val")
  }
),

C("block folded as map val, implicit indentation 2, chomp=strip",
R"(
example: >-
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end."),
    N("another", "val")
  }
),

C("block folded as map val, explicit indentation 2",
R"(
example: >2
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, explicit indentation 2, chomp=keep",
R"(
example: >+2
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n\n\n"),
    N("another", "val")
  }
),

C("block folded as map val, explicit indentation 2, chomp=strip",
R"(
example: >-2
  Several lines of text,
  with some "quotes" of various 'types',
  and also a blank line:
  
  plus another line at the end.
  
  
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end."),
    N("another", "val")
  }
),

C("block folded as map val, implicit indentation 3",
R"(
example: >
   Several lines of text,
   with some "quotes" of various 'types',
   and also a blank line:
   
   plus another line at the end.
   
   
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, explicit indentation 3",
R"(
example: >3
   Several lines of text,
   with some "quotes" of various 'types',
   and also a blank line:
   
   plus another line at the end.
   
   
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, implicit indentation 4",
R"(
example: >
    Several lines of text,
    with some "quotes" of various 'types',
    and also a blank line:
    
    plus another line at the end.
    
    
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, explicit indentation 4",
R"(
example: >4
    Several lines of text,
    with some "quotes" of various 'types',
    and also a blank line:
    
    plus another line at the end.
    
    
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, implicit indentation 9",
R"(
example: >
         Several lines of text,
         with some "quotes" of various 'types',
         and also a blank line:
         
         plus another line at the end.
         
         
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),

C("block folded as map val, explicit indentation 9",
R"(
example: >9
         Several lines of text,
         with some "quotes" of various 'types',
         and also a blank line:
         
         plus another line at the end.
         
         
another: val
)",
  L{
    N("example", "Several lines of text, with some \"quotes\" of various 'types', and also a blank line:\nplus another line at the end.\n"),
    N("another", "val")
  }
),
    )
}

INSTANTIATE_GROUP(BLOCK_FOLDED)

} // namespace yml
} // namespace c4
