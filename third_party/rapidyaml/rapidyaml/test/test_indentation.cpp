#include "./test_group.hpp"

namespace c4 {
namespace yml {

#define INDENTATION_CASES \
    "indented doc",\
    "4 chars",\
    "2 chars + 4 chars, ex0",\
    "2 chars + 4 chars, ex1",\
    "2 chars + 4 chars, ex2",\
    "non-indented blank lines",\
    "unnecessary indentation",\
    "blank lines indented, 1 - at same scope",\
    "indentation at start",\
    "unaligned comments",\
    "issue83"

CASE_GROUP(INDENTATION)
{
    APPEND_CASES(

C("indented doc", R"(
    # this is an indented doc
    ---
    - foo
    - bar
    - baz
)",
N(STREAM, L{N(DOCSEQ, L{N("foo"), N("bar"), N("baz")})})
),

C("4 chars",
R"(
key:
     value
another_key:
    sub_key0:
      - val0
      - val1
    sub_key1:
      - val2
      - val3
    sub_key2:
      - val4
      - val5
)",
L{
    N("key", "value"),
    N("another_key", L{
        N("sub_key0", L{N("val0"), N("val1")}),
        N("sub_key1", L{N("val2"), N("val3")}),
        N("sub_key2", L{N("val4"), N("val5")}),
    })
}),

C("2 chars + 4 chars, ex0",
R"(
key:
     value
another_key:
    sub_key0:
        - val0
        - val1
    sub_key1:
      - val2
      - val3
    sub_key2:
      - val4
      - val5
)",
L{
    N("key", "value"),
    N("another_key", L{
        N("sub_key0", L{N("val0"), N("val1")}),
        N("sub_key1", L{N("val2"), N("val3")}),
        N("sub_key2", L{N("val4"), N("val5")}),
    })
}),

C("2 chars + 4 chars, ex1",
R"(
key:
     value
another_key:
    sub_key0:
      - val0
      - val1
    sub_key1:
        - val2
        - val3
    sub_key2:
      - val4
      - val5
)",
L{
    N("key", "value"),
    N("another_key", L{
        N("sub_key0", L{N("val0"), N("val1")}),
        N("sub_key1", L{N("val2"), N("val3")}),
        N("sub_key2", L{N("val4"), N("val5")}),
    })
}),

C("2 chars + 4 chars, ex2",
R"(
key:
     value
another_key:
    sub_key0:
      - val0
      - val1
    sub_key1:
      - val2
      - val3
    sub_key2:
        - val4
        - val5
)",
L{
    N("key", "value"),
    N("another_key", L{
        N("sub_key0", L{N("val0"), N("val1")}),
        N("sub_key1", L{N("val2"), N("val3")}),
        N("sub_key2", L{N("val4"), N("val5")}),
    })
}),

C("non-indented blank lines",
R"(
matrix:

  include:  # next line is blank

    - env01
    - env02
    - env03
    - env04  # next line has one space
 
    - env11
    - env12
    - env13
    - env14  # next line has two spaces
  
    - env21
    - env22
    - env23
    - env24  # next line has three spaces
   
    - env31
    - env32
    - env33
    - env34  # next line has four spaces
    
    - env41
    - env42
    - env43
    - env44  # next line has five spaces
     
    - env51
    - env52
    - env53
    - env54  # next line has six spaces
      
    - env61
    - env62
    - env63
    - env64  # next line has five spaces
)",
L{N("matrix", L{
    N("include", L{
      N("env01"), N("env02"), N("env03"), N("env04"), 
      N("env11"), N("env12"), N("env13"), N("env14"), 
      N("env21"), N("env22"), N("env23"), N("env24"), 
      N("env31"), N("env32"), N("env33"), N("env34"), 
      N("env41"), N("env42"), N("env43"), N("env44"), 
      N("env51"), N("env52"), N("env53"), N("env54"), 
      N("env61"), N("env62"), N("env63"), N("env64"), 
        }
  )})
}),

C("unnecessary indentation",
R"(
skip_commits:
  files:
                - a
                - b
                - c
                - d
                - e
                - f
  more_files:
           - a
           - b
  even_more_files:
     - a
     - b
more_skip:
  files:
          - a
          - b
          - c
          - d
          - e
          - f
  more_files:
    - a
    - b
)",
L{
  N("skip_commits", L{
    N("files", L{N("a"), N("b"), N("c"), N("d"), N("e"), N("f"),}),
    N("more_files", L{N("a"), N("b"),}),
    N("even_more_files", L{N("a"), N("b"),}),
  }),
  N("more_skip", L{
    N("files", L{N("a"), N("b"), N("c"), N("d"), N("e"), N("f"),}),
    N("more_files", L{N("a"), N("b"),}),
  })
}),


C("blank lines indented, 1 - at same scope",
R"(
skip_commits:
  files:
                - a  # next line has 22 spaces (aligns with -)
                
                - b  # next line has 23 spaces (aligns with #)
                     
                - c  # next line has 3 spaces
   
                - d
)",
L{
  N("skip_commits", L{
    N("files", L{N("a"), N("b"), N("c"), N("d"),}),
  }),
}),

C("indentation at start",
R"(
      foo:
        - a
        - b
      bar:
        - c
        - d
)",
L{
  N("foo", L{N("a"), N("b"),}),
  N("bar", L{N("c"), N("d"),}),
}),
 
C("unaligned comments",
R"(
      stand2sit:
        map: mirror
        dat:
          - a
          - b
#
          - b1
 #
          - b2
  #
   #
    #
          - b3
     #
      #
       #
          - b4
        #
         # - c
          #- d
          - b5
           #- d2
            #- d3
             #- d4
          - b6
              #- d41
               #
          - b61
                 #
                   #
          - b62
                     #
                       #
                         #
          - b63
                           #
          - b64
                           #
          - b65
                         #
                       #
                     #
          - b66
                     #
                   #
                 #
               #
              #- d41
             #- d5
            #- d6
           #- d7
          - b7
          #- d8
         #
        #
       #
          - b8
      #
     #
    #
          - b9
   #
  #
          - b10
 #
#
          - e
          - f
          - g
)",
L{
  N("stand2sit", L{
    N("map", "mirror"),
    N("dat", L{N("a"), N("b"), N("b1"), N("b2"), N("b3"), N("b4"), N("b5"), N("b6"), N("b61"), N("b62"), N("b63"), N("b64"), N("b65"), N("b66"), N("b7"), N("b8"), N("b9"), N("b10"), N("e"), N("f"), N("g")}),
  }),
}),

C("issue83",
R"(
e:
  - f
g: h
a:
  - b
  
c: d
)",
L{
N("e", L{N("f")}),
N("g", "h"),
N("a", L{N("b")}),
N("c", "d"),
})

  )
}

INSTANTIATE_GROUP(INDENTATION)

} // namespace yml
} // namespace c4
