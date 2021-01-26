#include "./test_group.hpp"

namespace c4 {
namespace yml {

//     SIMPLE_ANCHOR/YmlTestCase.parse_using_ryml/0

/** verify that the reference class is working correctly (meta testing, yay) */
TEST(CaseNode, anchors)
{
    const NodeType mask = KEYREF|KEYANCH|VALREF|VALANCH;
    {
        CaseNode n("<<", "*base", AR(VALANCH, "base"));
        EXPECT_EQ(n.key, "<<");
        EXPECT_EQ(n.val, "*base");
        EXPECT_EQ(n.type & mask, VALANCH);
        EXPECT_EQ(n.key_anchor.type, NOTYPE);
        EXPECT_EQ(n.val_anchor.type, VALANCH);
        EXPECT_EQ(n.key_anchor.str, "");
        EXPECT_EQ(n.val_anchor.str, "base");
    }

    {
        CaseNode n("base", L{N("name", "Everyone has same name")}, AR(VALANCH, "base"));
        EXPECT_EQ(n.key, "base");
        EXPECT_EQ(n.val, "");
        EXPECT_NE(n.type.is_seq(), true);
        EXPECT_EQ(n.type & mask, VALANCH);
        EXPECT_EQ(n.key_anchor.type, NOTYPE);
        EXPECT_EQ(n.val_anchor.type, VALANCH);
        EXPECT_EQ(n.key_anchor.str, "");
        EXPECT_EQ(n.val_anchor.str, "base");
    }

    {
        L l{N("<<", "*base", AR(VALREF, "base"))};
        CaseNode const& base = *l.begin();
        EXPECT_EQ(base.key, "<<");
        EXPECT_EQ(base.val, "*base");
        EXPECT_EQ(base.type.is_keyval(), true);
        EXPECT_EQ(base.type & mask, VALREF);
        EXPECT_EQ(base.key_anchor.type, NOTYPE);
        EXPECT_EQ(base.val_anchor.type, VALREF);
        EXPECT_EQ(base.key_anchor.str, "");
        EXPECT_EQ(base.val_anchor.str, "base");
    }

    {
        L l{N("<<", "*base", AR(VALREF, "base")), N("age", "10")};
        CaseNode const& base = *l.begin();
        CaseNode const& age = *(&base + 1);
        EXPECT_EQ(base.key, "<<");
        EXPECT_EQ(base.val, "*base");
        EXPECT_EQ(base.type.is_keyval(), true);
        EXPECT_EQ(base.type & mask, VALREF);
        EXPECT_EQ(base.key_anchor.type, NOTYPE);
        EXPECT_EQ(base.val_anchor.type, VALREF);
        EXPECT_EQ(base.key_anchor.str, "");
        EXPECT_EQ(base.val_anchor.str, "base");

        EXPECT_EQ(age.key, "age");
        EXPECT_EQ(age.val, "10");
        EXPECT_EQ(age.type.is_keyval(), true);
        EXPECT_EQ(age.type & mask, 0);
        EXPECT_EQ(age.key_anchor.type, NOTYPE);
        EXPECT_EQ(age.val_anchor.type, NOTYPE);
        EXPECT_EQ(age.key_anchor.str, "");
        EXPECT_EQ(age.val_anchor.str, "");
    }

    {
        CaseNode n("foo", L{N("<<", "*base", AR(VALREF, "base")), N("age", "10")}, AR(VALANCH, "foo"));
        EXPECT_EQ(n.key, "foo");
        EXPECT_EQ(n.val, "");
        EXPECT_EQ(n.type.has_key(), true);
        EXPECT_EQ(n.type.is_map(), true);
        EXPECT_EQ(n.type & mask, VALANCH);
        EXPECT_EQ(n.key_anchor.type, NOTYPE);
        EXPECT_EQ(n.val_anchor.type, VALANCH);
        EXPECT_EQ(n.key_anchor.str, "");
        EXPECT_EQ(n.val_anchor.str, "foo");

        CaseNode const& base = n.children[0];
        EXPECT_EQ(base.key, "<<");
        EXPECT_EQ(base.val, "*base");
        EXPECT_EQ(base.type.has_key() && base.type.has_val(), true);
        EXPECT_EQ(base.type & mask, VALREF);
        EXPECT_EQ(base.key_anchor.type, NOTYPE);
        EXPECT_EQ(base.val_anchor.type, VALREF);
        EXPECT_EQ(base.key_anchor.str, "");
        EXPECT_EQ(base.val_anchor.str, "base");
    }

}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SIMPLE_ANCHOR_CASES                            \
    "simple anchor 1, implicit, unresolved",\
    "simple anchor 1, implicit, resolved",\
    "simple anchor 1, explicit, unresolved",\
    "simple anchor 1, explicit, resolved",\
    "anchor example 2, unresolved",\
    "anchor example 2, resolved",\
    "anchor example 3, unresolved",\
    "anchor example 3, resolved",\
    "merge example, unresolved",\
    "merge example, resolved",\
    "tagged doc with anchors 9KAX"


CASE_GROUP(SIMPLE_ANCHOR)
{
    APPEND_CASES(

C("merge example, unresolved",
R"(# https://yaml.org/type/merge.html
- &CENTER { x: 1, y: 2 }
- &LEFT { x: 0, y: 2 }
- &BIG { r: 10 }
- &SMALL { r: 1 }

# All the following maps are equal:

- # Explicit keys
  x: 1
  y: 2
  r: 10
  label: center/big

- # Merge one map
  << : *CENTER
  r: 10
  label: center/big

- # Merge multiple maps
  << : [ *CENTER, *BIG ]
  label: center/big

- # Override
  << : [ *BIG, *LEFT, *SMALL ]
  x: 1
  label: center/big
)",
L{
    N(L{N("x", "1" ), N("y", "2")}, AR(VALANCH, "CENTER")),
    N(L{N("x", "0" ), N("y", "2")}, AR(VALANCH, "LEFT"  )),
    N(L{N("r", "10")             }, AR(VALANCH, "BIG"   )),
    N(L{N("r", "1" )             }, AR(VALANCH, "SMALL" )),
    N(L{N("x", "1" ), N("y", "2"), N("r", "10"), N("label", "center/big")}),
    N(L{N("<<", "*CENTER", AR(VALREF, "*CENTER")), N("r", "10"), N("label", "center/big")}),
    N(L{N("<<", L{N("*CENTER", AR(VALREF, "*CENTER")), N("*BIG", AR(VALREF, "*BIG"))}), N("label", "center/big")}),
    N(L{N("<<", L{N("*BIG", AR(VALREF, "*BIG")), N("*LEFT", AR(VALREF, "*LEFT")), N("*SMALL", AR(VALREF, "*SMALL"))}), N("x", "1"), N("label", "center/big")}),
}),

C("merge example, resolved", RESOLVE_REFS,
R"(# https://yaml.org/type/merge.html
- &CENTER { x: 1, y: 2 }
- &LEFT { x: 0, y: 2 }
- &BIG { r: 10 }
- &SMALL { r: 1 }

# All the following maps are equal:

- # Explicit keys
  x: 1
  y: 2
  r: 10
  label: center/big

- # Merge one map
  << : *CENTER
  r: 10
  label: center/big

- # Merge multiple maps
  << : [ *CENTER, *BIG ]
  label: center/big

- # Override
  << : [ *SMALL, *LEFT, *BIG ]
  x: 1
  label: center/big
)",
L{
    N(L{N("x", "1" ), N("y", "2")}),
    N(L{N("x", "0" ), N("y", "2")}),
    N(L{N("r", "10")             }),
    N(L{N("r", "1" )             }),
    N(L{N("x", "1" ), N("y", "2"), N("r", "10"), N("label", "center/big")}),
    N(L{N("x", "1" ), N("y", "2"), N("r", "10"), N("label", "center/big")}),
    N(L{N("x", "1" ), N("y", "2"), N("r", "10"), N("label", "center/big")}),
    N(L{N("x", "1" ), N("y", "2"), N("r", "10"), N("label", "center/big")}),
}),

C("simple anchor 1, implicit, unresolved",
R"(
anchored_content: &anchor_name This string will appear as the value of two keys.
other_anchor: *anchor_name
anchors_in_seqs:
  - &anchor_in_seq this value appears in both elements of the sequence
  - *anchor_in_seq
base: &base
    name: Everyone has same name
foo: &foo
    <<: *base
    age: 10
bar: &bar
    <<: *base
    age: 20
)",
  L{
      N("anchored_content", "This string will appear as the value of two keys.", AR(VALANCH, "anchor_name")),
      N("other_anchor", "*anchor_name", AR(VALREF, "anchor_name")),
      N("anchors_in_seqs", L{
              N("this value appears in both elements of the sequence", AR(VALANCH, "anchor_in_seq")),
              N("*anchor_in_seq", AR(VALREF, "anchor_in_seq")),
          }),
      N("base", L{N("name", "Everyone has same name")}, AR(VALANCH, "base")),
      N("foo", L{N("<<", "*base", AR(VALREF, "base")), N("age", "10")}, AR(VALANCH, "foo")),
      N("bar", L{N("<<", "*base", AR(VALREF, "base")), N("age", "20")}, AR(VALANCH, "bar")),
  }
),

C("simple anchor 1, explicit, unresolved",
R"({
anchored_content: &anchor_name This string will appear as the value of two keys.,
other_anchor: *anchor_name,
anchors_in_seqs: [
  &anchor_in_seq this value appears in both elements of the sequence,
  *anchor_in_seq
  ],
base: &base {
    name: Everyone has same name
  },
foo: &foo {
    <<: *base,
    age: 10
  },
bar: &bar {
    <<: *base,
    age: 20
  }
})",
  L{
      N("anchored_content", "This string will appear as the value of two keys.", AR(VALANCH, "anchor_name")),
      N("other_anchor", "*anchor_name", AR(VALREF, "anchor_name")),
      N("anchors_in_seqs", L{
              N("this value appears in both elements of the sequence", AR(VALANCH, "anchor_in_seq")),
              N("*anchor_in_seq", AR(VALREF, "anchor_in_seq")),
          }),
      N("base", L{N("name", "Everyone has same name")}, AR(VALANCH, "base")),
      N("foo", L{N("<<", "*base", AR(VALREF, "base")), N("age", "10")}, AR(VALANCH, "foo")),
      N("bar", L{N("<<", "*base", AR(VALREF, "base")), N("age", "20")}, AR(VALANCH, "bar")),
  }
),

C("simple anchor 1, implicit, resolved", RESOLVE_REFS,
R"(
anchored_content: &anchor_name This string will appear as the value of two keys.
other_anchor: *anchor_name
anchors_in_seqs:
  - &anchor_in_seq this value appears in both elements of the sequence
  - *anchor_in_seq
base: &base
    name: Everyone has same name
foo: &foo
    <<: *base
    age: 10
bar: &bar
    <<: *base
    age: 20
)",
  L{
      N("anchored_content", "This string will appear as the value of two keys."),
      N("other_anchor", "This string will appear as the value of two keys."),
      N("anchors_in_seqs", L{
              N("this value appears in both elements of the sequence"),
              N("this value appears in both elements of the sequence"),
          }),
      N("base", L{N("name", "Everyone has same name")}),
      N("foo", L{N("name", "Everyone has same name"), N("age", "10")}),
      N("bar", L{N("name", "Everyone has same name"), N("age", "20")}),
  }
),

C("simple anchor 1, explicit, resolved", RESOLVE_REFS,
R"({
anchored_content: &anchor_name This string will appear as the value of two keys.,
other_anchor: *anchor_name,
anchors_in_seqs: [
  &anchor_in_seq this value appears in both elements of the sequence,
  *anchor_in_seq
  ],
base: &base {
    name: Everyone has same name
  },
foo: &foo {
    <<: *base,
    age: 10
  },
bar: &bar {
    <<: *base,
    age: 20
  }
})",
  L{
      N("anchored_content", "This string will appear as the value of two keys."),
      N("other_anchor", "This string will appear as the value of two keys."),
      N("anchors_in_seqs", L{
              N("this value appears in both elements of the sequence"),
              N("this value appears in both elements of the sequence"),
          }),
      N("base", L{N("name", "Everyone has same name")}),
      N("foo", L{N("name", "Everyone has same name"), N("age", "10")}),
      N("bar", L{N("name", "Everyone has same name"), N("age", "20")}),
  }
),


C("anchor example 2, unresolved",
R"(
receipt:     Oz-Ware Purchase Invoice
date:        2012-08-06
customer:
    first_name:   Dorothy
    family_name:  Gale
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
bill-to:  &id001
    street: |
            123 Tornado Alley
            Suite 16
    city:   East Centerville
    state:  KS
ship-to:  *id001
specialDelivery:  >
    Follow the Yellow Brick
    Road to the Emerald City.
    Pay no attention to the
    man behind the curtain.
)",
L{
  N{"receipt", "Oz-Ware Purchase Invoice"},
  N{"date",    "2012-08-06"},
  N{"customer", L{N{"first_name", "Dorothy"}, N{"family_name", "Gale"}}},
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
   N{"bill-to", L{
        N{"street", "123 Tornado Alley\nSuite 16\n"},
        N{"city", "East Centerville"},
        N{"state", "KS"},}, AR(VALANCH, "id001")},
   N{"ship-to", "*id001", AR(VALREF, "id001")},
   N{"specialDelivery", "Follow the Yellow Brick Road to the Emerald City. Pay no attention to the man behind the curtain.\n"}
  }
),


C("anchor example 2, resolved", RESOLVE_REFS,
R"(
receipt:     Oz-Ware Purchase Invoice
date:        2012-08-06
customer:
    first_name:   Dorothy
    family_name:  Gale
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
bill-to:  &id001
    street: |
            123 Tornado Alley
            Suite 16
    city:   East Centerville
    state:  KS
ship-to:  *id001
specialDelivery:  >
    Follow the Yellow Brick
    Road to the Emerald City.
    Pay no attention to the
    man behind the curtain.
)",
L{
  N{"receipt", "Oz-Ware Purchase Invoice"},
  N{"date",    "2012-08-06"},
  N{"customer", L{N{"first_name", "Dorothy"}, N{"family_name", "Gale"}}},
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
   N{"bill-to", L{
        N{"street", "123 Tornado Alley\nSuite 16\n"},
        N{"city", "East Centerville"},
        N{"state", "KS"},}},
   N{"ship-to", L{
        N{"street", "123 Tornado Alley\nSuite 16\n"},
        N{"city", "East Centerville"},
        N{"state", "KS"},}},
   N{"specialDelivery", "Follow the Yellow Brick Road to the Emerald City. Pay no attention to the man behind the curtain.\n"}
  }
),

C("anchor example 3, unresolved",
R"(
- step:  &id001                  # defines anchor label &id001
    instrument:      Lasik 2000
    pulseEnergy:     5.4
    pulseDuration:   12
    repetition:      1000
    spotSize:        1mm
- step: &id002
    instrument:      Lasik 2000
    pulseEnergy:     5.0
    pulseDuration:   10
    repetition:      500
    spotSize:        2mm
- step: *id001                   # refers to the first step (with anchor &id001)
- step: *id002                   # refers to the second step
- step:
    <<: *id001
    spotSize: 2mm                # redefines just this key, refers rest from &id001
- step: *id002
)",
L{N(L{
N("step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.4"},
    N{"pulseDuration",   "12"},
    N{"repetition",      "1000"},
    N{"spotSize",        "1mm"},
        }, AR(VALANCH, "id001")),
    }), N(L{
N("step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.0"},
    N{"pulseDuration",   "10"},
    N{"repetition",      "500"},
    N{"spotSize",        "2mm"},
        }, AR(VALANCH, "id002")),
    }), N(L{
N{"step", "*id001", AR(VALREF, "id001")},
    }), N(L{
N{"step", "*id002", AR(VALREF, "id002")},
    }), N(L{
N{"step", L{
    N{"<<", "*id001", AR(VALREF, "id002")},
    N{"spotSize",        "2mm"},
        }},
    }), N(L{
N{"step", "*id002", AR(VALREF, "id002")},
    }),
    }
),

C("anchor example 3, resolved", RESOLVE_REFS,
R"(
- step:  &id001                  # defines anchor label &id001
    instrument:      Lasik 2000
    pulseEnergy:     5.4
    pulseDuration:   12
    repetition:      1000
    spotSize:        1mm
- step: &id002
    instrument:      Lasik 2000
    pulseEnergy:     5.0
    pulseDuration:   10
    repetition:      500
    spotSize:        2mm
- step: *id001                   # refers to the first step (with anchor &id001)
- step: *id002                   # refers to the second step
- step:
    <<: *id001
    spotSize: 2mm                # redefines just this key, refers rest from &id001
- step: *id002
)",
  L{N(L{
N{"step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.4"},
    N{"pulseDuration",   "12"},
    N{"repetition",      "1000"},
    N{"spotSize",        "1mm"},
        }},
    }), N(L{
N{"step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.0"},
    N{"pulseDuration",   "10"},
    N{"repetition",      "500"},
    N{"spotSize",        "2mm"},
        }},
    }), N(L{
N{"step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.4"},
    N{"pulseDuration",   "12"},
    N{"repetition",      "1000"},
    N{"spotSize",        "1mm"},
        }},
    }), N(L{
N{"step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.0"},
    N{"pulseDuration",   "10"},
    N{"repetition",      "500"},
    N{"spotSize",        "2mm"},
        }},
    }), N(L{
N{"step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.4"},
    N{"pulseDuration",   "12"},
    N{"repetition",      "1000"},
    N{"spotSize",        "2mm"},
        }},
    }), N(L{
N{"step", L{
    N{"instrument",      "Lasik 2000"},
    N{"pulseEnergy",     "5.0"},
    N{"pulseDuration",   "10"},
    N{"repetition",      "500"},
    N{"spotSize",        "2mm"},
        }},
    }),
    }
),

C("tagged doc with anchors 9KAX",
R"(
---
&a1
!!str
scalar1
--- &a1 !!str scalar1
---
!!str
&a1
scalar1
--- !!str &a1 scalar1
---
!!str
&a2
scalar2
--- &a2 !!str scalar2
---
&a3
!!str scalar3
--- &a3 !!str scalar3
---
&a4 !!map
&a5 !!str key5: value4
--- &a4 !!map
&a5 !!str key5: value4
---
a6: 1
&anchor6 b6: 2
---
!!map
&a8 !!str key8: value7
--- !!map
&a8 !!str key8: value7
---
!!map
!!str &a10 key10: value9
--- !!map
&a10 !!str key10: value9
---
!!str &a11
value11
--- &a11 !!str value11
)",
N(STREAM, L{
    N(DOCVAL, TS("!!str", "scalar1"), AR(VALANCH, "a1")),
    N(DOCVAL, TS("!!str", "scalar1"), AR(VALANCH, "a1")),
    N(DOCVAL, TS("!!str", "scalar1"), AR(VALANCH, "a1")),
    N(DOCVAL, TS("!!str", "scalar1"), AR(VALANCH, "a1")),
    N(DOCVAL, TS("!!str", "scalar2"), AR(VALANCH, "a2")),
    N(DOCVAL, TS("!!str", "scalar2"), AR(VALANCH, "a2")),
    N(DOCVAL, TS("!!str", "scalar3"), AR(VALANCH, "a3")),
    N(DOCVAL, TS("!!str", "scalar3"), AR(VALANCH, "a3")),
    N(DOCMAP, TL("!!map", L{N(TS("!!str", "key5"), AR(KEYANCH, "a5"), "value4")}), AR(VALANCH, "a4")),
    N(DOCMAP, TL("!!map", L{N(TS("!!str", "key5"), AR(KEYANCH, "a5"), "value4")}), AR(VALANCH, "a4")),
    N(DOCMAP, L{N("a6", "1"), N("b6", AR(KEYANCH, "anchor6"), "2")}),
    N(DOCMAP, TL("!!map", L{N(TS("!!str", "key8"), AR(KEYANCH, "a8"), "value7")})),
    N(DOCMAP, TL("!!map", L{N(TS("!!str", "key8"), AR(KEYANCH, "a8"), "value7")})),
    N(DOCMAP, TL("!!map", L{N(TS("!!str", "key10"), AR(KEYANCH, "a10"), "value9")})),
    N(DOCMAP, TL("!!map", L{N(TS("!!str", "key10"), AR(KEYANCH, "a10"), "value9")})),
    N(DOCVAL, TS("!!str", "value11"), AR(VALANCH, "a11")),
    N(DOCVAL, TS("!!str", "value11"), AR(VALANCH, "a11")),
})
),

    )
}

INSTANTIATE_GROUP(SIMPLE_ANCHOR)

} // namespace yml
} // namespace c4
