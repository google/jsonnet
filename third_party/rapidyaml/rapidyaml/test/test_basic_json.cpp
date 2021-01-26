#include "c4/yml/std/std.hpp"
#include "c4/yml/parse.hpp"
#include "c4/yml/emit.hpp"
#include <c4/format.hpp>
#include <c4/yml/detail/checks.hpp>
#include <c4/yml/detail/print.hpp>

#include "./test_case.hpp"

#include <gtest/gtest.h>

namespace foo {

template<class T>
struct vec2
{
    T x, y;
};
template<class T>
struct vec3
{
    T x, y, z;
};
template<class T>
struct vec4
{
    T x, y, z, w;
};

template<class T> size_t to_chars(c4::substr buf, vec2<T> v) { return c4::format(buf, "({},{})", v.x, v.y); }
template<class T> size_t to_chars(c4::substr buf, vec3<T> v) { return c4::format(buf, "({},{},{})", v.x, v.y, v.z); }
template<class T> size_t to_chars(c4::substr buf, vec4<T> v) { return c4::format(buf, "({},{},{},{})", v.x, v.y, v.z, v.w); }

template<class T> bool from_chars(c4::csubstr buf, vec2<T> *v) { size_t ret = c4::unformat(buf, "({},{})", v->x, v->y); return ret != c4::yml::npos; }
template<class T> bool from_chars(c4::csubstr buf, vec3<T> *v) { size_t ret = c4::unformat(buf, "({},{},{})", v->x, v->y, v->z); return ret != c4::yml::npos; }
template<class T> bool from_chars(c4::csubstr buf, vec4<T> *v) { size_t ret = c4::unformat(buf, "({},{},{},{})", v->x, v->y, v->z, v->w); return ret != c4::yml::npos; }

TEST(serialize, type_as_str)
{
    c4::yml::Tree t;

    auto r = t.rootref();
    r |= c4::yml::MAP;

    vec2<int> v2in{10, 11};
    vec2<int> v2out;
    r["v2"] << v2in;
    r["v2"] >> v2out;
    EXPECT_EQ(v2in.x, v2out.x);
    EXPECT_EQ(v2in.y, v2out.y);

    vec3<int> v3in{100, 101, 102};
    vec3<int> v3out;
    r["v3"] << v3in;
    r["v3"] >> v3out;
    EXPECT_EQ(v3in.x, v3out.x);
    EXPECT_EQ(v3in.y, v3out.y);
    EXPECT_EQ(v3in.z, v3out.z);

    vec4<int> v4in{1000, 1001, 1002, 1003};
    vec4<int> v4out;
    r["v4"] << v4in;
    r["v4"] >> v4out;
    EXPECT_EQ(v4in.x, v4out.x);
    EXPECT_EQ(v4in.y, v4out.y);
    EXPECT_EQ(v4in.z, v4out.z);
    EXPECT_EQ(v4in.w, v4out.w);

    char buf[256];
    c4::csubstr interm = c4::yml::emit_json(t, buf);
    c4::yml::Tree res = c4::yml::parse(interm);
    c4::substr ret = c4::yml::emit(res, buf);
    EXPECT_EQ(ret, R"(v2: '(10,11)'
v3: '(100,101,102)'
v4: '(1000,1001,1002,1003)'
)");
}
} // namespace foo

namespace c4 {
namespace yml {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST(general, emitting)
{
    std::string cmpbuf;
    std::string cmpbuf2;

    Tree tree;
    auto r = tree.rootref();

    r |= MAP;  // this is needed to make the root a map

    r["foo"] = "1"; // ryml works only with strings.
    // Note that the tree will be __pointing__ at the
    // strings "foo" and "1" used here. You need
    // to make sure they have at least the same
    // lifetime as the tree.

    auto s = r["seq"]; // does not change the tree until s is written to.
    s |= SEQ;
    r["seq"].append_child() = "bar0"; // value of this child is now __pointing__ at "bar0"
    r["seq"].append_child() = "bar1";
    r["seq"].append_child() = "bar2";

    //print_tree(tree);

    // emit to stdout (can also emit to FILE* or ryml::span)
    emitrs_json(tree, &cmpbuf);
    c4::substr c4cmp(to_substr(cmpbuf));
    c4::yml::Tree res = c4::yml::parse(c4cmp);
    emitrs(res, &cmpbuf2);
    const char* exp = R"(foo: 1
seq:
  - bar0
  - bar1
  - bar2
)";
    EXPECT_EQ(cmpbuf2, exp);

    // serializing: using operator<< instead of operator=
    // will make the tree serialize the value into a char
    // arena inside the tree. This arena can be reserved at will.
    int ch3 = 33, ch4 = 44;
    s.append_child() << ch3;
    s.append_child() << ch4;

    {
        std::string tmp = "child5";
        s.append_child() << tmp;
        // now tmp can go safely out of scope, as it was
        // serialized to the tree's internal string arena
    }

    emitrs_json(tree, &cmpbuf);
    c4cmp = to_substr(cmpbuf);
    res = c4::yml::parse(c4cmp);
    emitrs(res, &cmpbuf2);
    exp = R"(foo: 1
seq:
  - bar0
  - bar1
  - bar2
  - 33
  - 44
  - child5
)";
    EXPECT_EQ(cmpbuf2, exp);

    // to serialize keys:
    int k = 66;
    r.append_child() << key(k) << 7;

    emitrs_json(tree, &cmpbuf);
    c4cmp = to_substr(cmpbuf);    
    res = c4::yml::parse(c4cmp);
    emitrs(res, &cmpbuf2);
    exp = R"(foo: 1
seq:
  - bar0
  - bar1
  - bar2
  - 33
  - 44
  - child5
66: 7
)";
    EXPECT_EQ(cmpbuf2, exp);
}

TEST(general, map_to_root)
{
    std::string cmpbuf; const char *exp;
    std::map<std::string, int> m({{"bar", 2}, {"foo", 1}});
    Tree t;
    t.rootref() << m;

    emitrs_json(t, &cmpbuf);
    exp = "{\"bar\": 2,\"foo\": 1}";
    EXPECT_EQ(cmpbuf, exp);

    t["foo"] << 10;
    t["bar"] << 20;

    m.clear();
    t.rootref() >> m;

    EXPECT_EQ(m["foo"], 10);
    EXPECT_EQ(m["bar"], 20);
}

TEST(general, json_stream_operator)
{
    std::map<std::string, int> out, m({{"bar", 2}, {"foo", 1}, {"foobar_barfoo:barfoo_foobar", 1001}, {"asdfjkl;", 42}, {"00000000000000000000000000000000000000000000000000000000000000", 1}});
    Tree t;
    t.rootref() << m;
    std::string str;
    {
        std::stringstream ss;
        ss << as_json(t);
        str = ss.str();
    }
    Tree res = c4::yml::parse(to_substr(str));
    EXPECT_EQ(res["foo"].val(), "1");
    EXPECT_EQ(res["bar"].val(), "2");
    EXPECT_EQ(res["foobar_barfoo:barfoo_foobar"].val(), "1001");
    EXPECT_EQ(res["asdfjkl;"].val(), "42");
    EXPECT_EQ(res["00000000000000000000000000000000000000000000000000000000000000"].val(), "1");
    res.rootref() >> out;
    EXPECT_EQ(out["foo"], 1);
    EXPECT_EQ(out["bar"], 2);
    EXPECT_EQ(out["foobar_barfoo:barfoo_foobar"], 1001);
    EXPECT_EQ(out["asdfjkl;"], 42);
    EXPECT_EQ(out["00000000000000000000000000000000000000000000000000000000000000"], 1);
}

TEST(emit_json, issue72)
{
    Tree t;
    NodeRef r = t.rootref();

    r |= MAP;
    r["1"] = "null";
    r["2"] = "true";
    r["3"] = "false";
    r["null"] = "1";
    r["true"] = "2";
    r["false"] = "3";

    std::string out;
    emitrs_json(t, &out);

    EXPECT_EQ(out, R"({"1": null,"2": true,"3": false,"null": 1,"true": 2,"false": 3})");
}

//-------------------------------------------
// this is needed to use the test case library
Case const* get_case(csubstr /*name*/)
{
    return nullptr;
}

} // namespace yml
} // namespace c4
