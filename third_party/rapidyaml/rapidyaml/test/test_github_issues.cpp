#include "./test_group.hpp"

namespace c4 {
namespace yml {

TEST(github, 78)
{
    Tree t = parse("{foo: 1, bar: [2, 3]}");
    EXPECT_EQ(t["foo"].val(), "1");
    EXPECT_EQ(t["bar"][0].val(), "2");
    EXPECT_EQ(t["bar"][1].val(), "3");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST(github, 60)
{
    Tree tree = parse(R"(
    traits:
        roleBonuses:
        -   bonus: 5
            bonusText:
                de: Bonus auf die Virusstärke von <a href=showinfo:22177>Relikt-</a>
                    und <a href=showinfo:22175>Datenanalysatoren</a>
                en: bonus to <a href=showinfo:22177>Relic</a> and <a href=showinfo:22175>Data
                    Analyzer</a> virus strength
                fr: de bonus à la puissance du virus des <a href=showinfo:22177>analyseurs
                    de reliques</a> et des <a href=showinfo:22175>analyseurs de données</a>
                ja: <a href=showinfo:22177>遺物アナライザー</a>と<a href=showinfo:22175>データアナライザー</a>のウイルス強度が増加
                ru: повышается степень опасности вирусов, применяемых в <a href=showinfo:22175>комплексах
                    анализа данных</a> и <a href=showinfo:22177>комплексах анализа
                    артефактов</a>
                zh: <a href="showinfo:22177">遗迹分析仪</a>和<a href="showinfo:22175">数据分析仪</a>病毒强度加成
            importance: 1
            unitID: 139
)");
    auto root = tree.rootref();
    ASSERT_TRUE(root.is_map());
    ASSERT_TRUE(root.has_child("traits"));
    auto rb = root["traits"]["roleBonuses"][0];
    ASSERT_TRUE(rb.valid());
    EXPECT_EQ(rb["bonus"].val(), "5");
    auto txt = rb["bonusText"];
    ASSERT_TRUE(txt.valid());
    ASSERT_TRUE(txt.is_map());
    EXPECT_TRUE(txt.has_child("de"));
    EXPECT_TRUE(txt.has_child("en"));
    EXPECT_TRUE(txt.has_child("fr"));
    EXPECT_TRUE(txt.has_child("ja"));
    EXPECT_TRUE(txt.has_child("ru"));
    EXPECT_TRUE(txt.has_child("zh"));
    EXPECT_EQ(txt["de"].val(), "Bonus auf die Virusstärke von <a href=showinfo:22177>Relikt-</a> und <a href=showinfo:22175>Datenanalysatoren</a>");
    EXPECT_EQ(txt["en"].val(), "bonus to <a href=showinfo:22177>Relic</a> and <a href=showinfo:22175>Data Analyzer</a> virus strength");
    EXPECT_EQ(txt["fr"].val(), "de bonus à la puissance du virus des <a href=showinfo:22177>analyseurs de reliques</a> et des <a href=showinfo:22175>analyseurs de données</a>");
    EXPECT_EQ(txt["ja"].val(), "<a href=showinfo:22177>遺物アナライザー</a>と<a href=showinfo:22175>データアナライザー</a>のウイルス強度が増加");
    EXPECT_EQ(txt["ru"].val(), "повышается степень опасности вирусов, применяемых в <a href=showinfo:22175>комплексах анализа данных</a> и <a href=showinfo:22177>комплексах анализа артефактов</a>");
    EXPECT_EQ(txt["zh"].val(), "<a href=\"showinfo:22177\">遗迹分析仪</a>和<a href=\"showinfo:22175\">数据分析仪</a>病毒强度加成");


    tree = parse(R"(208:
    basePrice: 3000.0
    description:
        de: Ursprünglich als Rakete für den Fangschuss entworfen, um einem beschädigten
            Schiff den Todesstoß zu geben, hat die Inferno Heavy Missile seither eine
            Reihe technischer Upgrades durchlaufen. Die neueste Version hat eine leichtere
            Sprengladung als das Original, aber stark verbesserte Lenksysteme.
        en: Originally designed as a 'finisher' - the killing blow to a crippled ship
            - the Inferno heavy missile has since gone through various technological
            upgrades. The latest version has a lighter payload than the original,
            but much improved guidance systems.
        fr: Conçu à l'origine pour donner le coup de grâce, le missile lourd Inferno
            a depuis subi de nombreuses améliorations techniques. La dernière version
            emporte une charge utile réduite par rapport à l'originale, mais est dotée
            de systèmes de guidage améliorés.
        ja: 元々「フィニッシャー」―大破した船にとどめを刺す兵器として設計されたインフェルノヘビーミサイルは、以来各種の技術改良を経てきた。現行型は初期型より軽い弾頭を採用しているが、それを補って余りある優れた誘導システムを持つ。
        ru: Тяжелая ракета Inferno изначально была спроектирована как «оружие последнего
            удара» для уничтожения подбитых кораблей. С тех пор было выпущено несколько
            ее модификаций. В последней модификации используется заряд меньшей мощности,
            но более совершенная система наведения.
        zh: 炼狱重型导弹历经多种技术改良，原本被设计为给予落魄敌舰最后一击的“终结者”角色。相比原型，最新版导弹载荷较轻，但装配了大幅改进的制导系统。
    graphicID: 20048
    groupID: 385
    iconID: 188
    marketGroupID: 924
    mass: 1000.0
    name:
        de: Inferno Heavy Missile
        en: Inferno Heavy Missile
        fr: Missile lourd Inferno
        ja: インフェルノヘビーミサイル
        ru: Inferno Heavy Missile
        zh: 炼狱重型导弹
    portionSize: 100
    published: true
    radius: 300.0
    volume: 0.03
)");
    root = tree.rootref()["208"];
    EXPECT_EQ(root["description"]["ja"].val(), "元々「フィニッシャー」―大破した船にとどめを刺す兵器として設計されたインフェルノヘビーミサイルは、以来各種の技術改良を経てきた。現行型は初期型より軽い弾頭を採用しているが、それを補って余りある優れた誘導システムを持つ。");
    EXPECT_EQ(root["name"]["ja"].val(), "インフェルノヘビーミサイル");
    EXPECT_EQ(root["name"]["zh"].val(), "炼狱重型导弹");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST(github, 31)
{
    Tree tree;
    NodeRef r = tree.rootref();
    r |= MAP;

    auto meas = r["meas"];
    meas |= MAP;

    auto plist = meas["createParameterList"];
    plist |= SEQ;

    {
        auto lumi = plist.append_child();
        lumi << "Lumi";
        EXPECT_TRUE(lumi.is_val());
    }

    {
        auto lumi = plist.append_child();
        lumi |= MAP;
        lumi["value"] << 1;
        lumi["relErr"] << 0.1;
        EXPECT_TRUE(lumi.is_map());
    }

    {
        #if RYML_USE_ASSERT
        ExpectError::do_check([&](){
            auto lumi = plist.append_child();
            lumi << "Lumi";
            lumi |= MAP;
        });
        #endif // RYML_USE_ASSERT
    }

    {
        #if RYML_USE_ASSERT
        ExpectError::do_check([&](){
            auto lumi = plist.append_child();
            lumi << "Lumi";
            lumi |= SEQ;
        });
        #endif // RYML_USE_ASSERT
    }

    {
        #if RYML_USE_ASSERT
        ExpectError::do_check([&](){
            auto lumi = plist.append_child();
            lumi |= MAP;
            lumi << "Lumi";
        });
        #endif // RYML_USE_ASSERT
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#define GITHUB_ISSUES_CASES \
        "github3-problem1",\
        "github3-problem2-ex1",\
        "github3-problem2-ex2",\
        "github3-problem3",\
        /*"github3",*/\
        "github6-problem1",\
        "github6",\
        "github34/ex1",\
        "github34/ex2",\
        "github34",\
        "github35/expected_error11",\
        "github35/expected_error12",\
        "github35/expected_error21",\
        "github35/expected_error22"


CASE_GROUP(GITHUB_ISSUES)
{
    APPEND_CASES(

C("github3-problem1",
R"(
translation: [-2, -2, 5])",
L{N("translation", L{N("-2"), N("-2"), N("5")})}
),

// these must work without quotes
C("github3-problem2-ex1",
R"(
audio resource:
)",
L{N(KEYVAL, "audio resource", /*"~"*/{})}
),
C("github3-problem2-ex2",
R"(
audio resource:
more:
  example: y
)",
L{N(KEYVAL, "audio resource", /*"~"*/{}), N("more", L{N("example", "y")})}
),

C("github3-problem3",
R"(component:
  type: perspective camera component
  some_data: {}  # this was working
  data:
    {}           # but this was not working
)",
L{N("component", L{
        N("type", "perspective camera component"),
        N(KEYMAP, "some_data", L{}),
        N(KEYMAP, "data", L{})
    }
)}
),

/* THIS IS CAUSING VS TO CRASH OUT OF HEAP SPACE
C("github3",
R"(
universe:
  objects:
    object:
      uuid: A7AB039C0EF3A74480A1B398247039A7
      components:
        - component:
            type: name component
            data:
              object name: Root Node
        - component:
            type: transform component
            data:
              translation: [-2, -2, 5]
              rotation: [0, 0, 0, 1]
              scaling: [1, 1, 1]
        - component:
            type: perspective camera component
            data:
             {}
        - component:
            type: mesh component
            data:
              mesh resource: TODO
        - component:
            type: lua script component
            data:
             {}
        - component:
            type: audio component
            data:
              audio resource: ''
              type: 0
              current sample: 184102
              spatialized: true
      children:
        - object:
            uuid: E1C364A925D649408E83C8EEF5179A87
            components:
              - component:
                  type: name component
                  data:
                    object name: Prepend
            children:
              []
        - object:
            uuid: 377DBA885AF4CD42B8A56BB3471F60E5
            components:
              - component:
                  type: name component
                  data:
                    object name: pivot
            children:
              []
        - object:
            uuid: 6DD1835797DADB4F95232CE7E9DE41BA
            components:
              - component:
                  type: name component
                  data:
                    object name: Append
            children:
              []
)",
  L{N("universe", L{
        N("objects", L{
            N("object", L{
                N("uuid", "A7AB039C0EF3A74480A1B398247039A7"),
                N("components", L{
                    N(L{N("component", L{N("type", "name component"), N("data", L{N("object name", "Root Node")}), }), }),
                    N(L{N("component", L{N("type", "transform component"), N("data", L{N("translation", L{N("-2"), N("-2"), N("5")}), N("rotation", L{N("0"), N("0"), N("0"), N("1")}), N("scaling", L{N("1"), N("1"), N("1")}),}), }), }),
                    N(L{N("component", L{N("type", "perspective camera component"), N(KEYMAP, "data", L{}), }), }),
                    N(L{N("component", L{N("type", "mesh component"), N("data", L{N("mesh resource", "TODO")}), }), }),
                    N(L{N("component", L{N("type", "lua script component"), N(KEYMAP, "data", L{}), }), }),
                    N(L{N("component", L{N("type", "audio component"), N("data", L{N("audio resource", ""), N("type", "0"), N("current sample", "184102"), N("spatialized", "true"), }), }), }), // component
                  }), // components
                N("children", L{
                    N(L{N("object", L{
                        N("uuid", "E1C364A925D649408E83C8EEF5179A87"),
                        N("components", L{N(L{N("component", L{N("type", "name component"), N("data", L{N("object name", "Prepend")}), }), }), }),
                        N(KEYSEQ, "children", L{}),
                    }), }), // object
                    N(L{N("object", L{
                        N("uuid", "377DBA885AF4CD42B8A56BB3471F60E5"),
                        N("components", L{N(L{N("component", L{N("type", "name component"), N("data", L{N("object name", "pivot")}), }), }), }),
                        N(KEYSEQ, "children", L{}),
                    }), }), // object
                    N(L{N("object", L{
                        N("uuid", "6DD1835797DADB4F95232CE7E9DE41BA"),
                        N("components", L{N(L{N("component", L{N("type", "name component"), N("data", L{N("object name", "Append")}), }), }), }),
                        N(KEYSEQ, "children", L{}),
                    }), }), // object
                  }), // children
                }), // object
              }) // objects
          }) // universe
      }
),
*/

C("github6-problem1",
R"(
- UQxRibHKEDI:
  - 0.mp4
  - 1.mp4
  - 2.mp4
  - 3.mp4
- DcYsg8VFdC0:
  - 0.mp4
  - 1.mp4
  - 2.mp4
  - 3.mp4
- Yt3ymqZXzLY:
  - 0.mp4
  - 1.mp4
  - 2.mp4
  - 3.mp4
)",
L{
N(L{N("UQxRibHKEDI", L{N("0.mp4"), N("1.mp4"), N("2.mp4"), N("3.mp4")})}),
N(L{N("DcYsg8VFdC0", L{N("0.mp4"), N("1.mp4"), N("2.mp4"), N("3.mp4")})}),
N(L{N("Yt3ymqZXzLY", L{N("0.mp4"), N("1.mp4"), N("2.mp4"), N("3.mp4")})}),
}
),

C("github6",
R"(videos:
- UQxRibHKEDI:
  - 0.mp4
  - 1.mp4
  - 2.mp4
  - 3.mp4
- DcYsg8VFdC0:
  - 0.mp4
  - 1.mp4
  - 2.mp4
  - 3.mp4
- Yt3ymqZXzLY:
  - 0.mp4
  - 1.mp4
  - 2.mp4
  - 3.mp4
)",
L{N("videos", L{
N(L{N("UQxRibHKEDI", L{N("0.mp4"), N("1.mp4"), N("2.mp4"), N("3.mp4")})}),
N(L{N("DcYsg8VFdC0", L{N("0.mp4"), N("1.mp4"), N("2.mp4"), N("3.mp4")})}),
N(L{N("Yt3ymqZXzLY", L{N("0.mp4"), N("1.mp4"), N("2.mp4"), N("3.mp4")})}),
})}
),

C("github34/ex1",
R"(
# correct:
MessageID1: 'MapRegion_HyrulePrairie'
MessageID2: "MapRegion_HyrulePrairie"
MessageID3: 'MapRegion_HyrulePrairie'
MessageID4: "MapRegion_HyrulePrairie"
# incorrect: uninitialised memory?
MessageID5:  'MapRegion_HyrulePrairie'
MessageID6:  "MapRegion_HyrulePrairie"
MessageID7:   'MapRegion_HyrulePrairie'
MessageID8:   "MapRegion_HyrulePrairie"
MessageID9:          'MapRegion_HyrulePrairie'
MessageID0:          "MapRegion_HyrulePrairie"
)",
L{
  N("MessageID1", "MapRegion_HyrulePrairie"),
  N("MessageID2", "MapRegion_HyrulePrairie"),
  N("MessageID3", "MapRegion_HyrulePrairie"),
  N("MessageID4", "MapRegion_HyrulePrairie"),
  N("MessageID5", "MapRegion_HyrulePrairie"),
  N("MessageID6", "MapRegion_HyrulePrairie"),
  N("MessageID7", "MapRegion_HyrulePrairie"),
  N("MessageID8", "MapRegion_HyrulePrairie"),
  N("MessageID9", "MapRegion_HyrulePrairie"),
  N("MessageID0", "MapRegion_HyrulePrairie"),
}
),

C("github34/ex2",
R"(
# correct:
- MessageID1: 'MapRegion_HyrulePrairie'
- MessageID2: "MapRegion_HyrulePrairie"
-  MessageID3: 'MapRegion_HyrulePrairie'
-  MessageID4: "MapRegion_HyrulePrairie"
# incorrect: uninitialised memory?
- MessageID5:  'MapRegion_HyrulePrairie'
- MessageID6:  "MapRegion_HyrulePrairie"
- MessageID7:   'MapRegion_HyrulePrairie'
- MessageID8:   "MapRegion_HyrulePrairie"
-  MessageID9:          'MapRegion_HyrulePrairie'
-  MessageID0:          "MapRegion_HyrulePrairie"
)",
L{
  N(L{N("MessageID1", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID2", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID3", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID4", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID5", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID6", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID7", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID8", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID9", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID0", "MapRegion_HyrulePrairie")}),
}
),

C("github34",
R"(
# incorrect: uninitialised memory?
-  MessageID1:          'MapRegion_HyrulePrairie'
-  MessageID2:          "MapRegion_HyrulePrairie"

# incorrect: uninitialised memory?
-  MessageID3:          'MapRegion_HyrulePrairie '
-  MessageID4:          "MapRegion_HyrulePrairie "

# incorrect: for some reason the ' is included in the string
-  MessageID5: 'MapRegion_HyrulePrairie '
- MessageID6: 'MapRegion_HyrulePrairie '
-  MessageID7: "MapRegion_HyrulePrairie "
- MessageID8: "MapRegion_HyrulePrairie "

# incorrect: same issue
- MessageID9: 'MapRegion_HyrulePrairie '
- MessageID10: "MapRegion_HyrulePrairie "

# incorrect: still has the trailing quote
- MessageID11: 'MapRegion_HyrulePrairie'
- MessageID12: "MapRegion_HyrulePrairie"

# the string is parsed correctly in this case
- key1: true1
  MessageID1:          'MapRegion_HyrulePrairie1 '
- key2: true2
  MessageID2:          "MapRegion_HyrulePrairie2 "
)",
L{
  N(L{N("MessageID1",  "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID2",  "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID3",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID4",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID5",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID6",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID7",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID8",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID9",  "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID10", "MapRegion_HyrulePrairie ")}),
  N(L{N("MessageID11", "MapRegion_HyrulePrairie")}),
  N(L{N("MessageID12", "MapRegion_HyrulePrairie")}),
  N(L{N("key1", "true1"), N("MessageID1", "MapRegion_HyrulePrairie1 ")}),
  N(L{N("key2", "true2"), N("MessageID2", "MapRegion_HyrulePrairie2 ")}),
}
),

C("github35/expected_error11", HAS_PARSE_ERROR,
R"(
# *segfault* // not anymore!
- key1: true1
 MessageID1:          'MapRegion_HyrulePrairie1 '
)",
  LineCol(4, 1)
),

C("github35/expected_error12", HAS_PARSE_ERROR,
R"(
# *segfault* // not anymore!
- key2: true2
 MessageID2:          "MapRegion_HyrulePrairie2 "
)",
  LineCol(4, 1)
),

C("github35/expected_error21", HAS_PARSE_ERROR,
R"(
# *segfault* // not anymore!
- key1: true1
    MessageID1:          'MapRegion_HyrulePrairie1 '
)",
  LineCol(4, 15)
),

C("github35/expected_error22", HAS_PARSE_ERROR,
R"(
# *segfault* // not anymore!
- key2: true2
    MessageID2:          "MapRegion_HyrulePrairie2 "
)",
  LineCol(4, 15)
),

    )
}

INSTANTIATE_GROUP(GITHUB_ISSUES)

} // namespace yml
} // namespace c4
