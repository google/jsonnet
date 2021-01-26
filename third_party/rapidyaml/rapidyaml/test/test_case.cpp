#include "./test_case.hpp"
#include "c4/format.hpp"
#include "c4/span.hpp"
#include "c4/yml/std/std.hpp"
#include "c4/yml/detail/print.hpp"
#include "c4/yml/detail/checks.hpp"

#include <gtest/gtest.h>

#if defined(_MSC_VER)
#   pragma warning(push)
#elif defined(__clang__)
#   pragma clang diagnostic push
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wuseless-cast"
#   if __GNUC__ >= 6
#       pragma GCC diagnostic ignored "-Wnull-dereference"
#   endif
#endif

namespace c4 {
namespace yml {

size_t _num_leaves(Tree const& t, size_t node)
{
    size_t count = 0;
    for(size_t i = t.first_child(node); i != NONE; i = t.next_sibling(i))
    {
        count += _num_leaves(t, i);
    }
    return count;
}


void test_compare(Tree const& a, Tree const& b)
{
    EXPECT_EQ(a.size(), b.size());
    EXPECT_EQ(_num_leaves(a, a.root_id()), _num_leaves(b, b.root_id()));
    test_compare(a, a.root_id(), b, b.root_id(), 0);
}


void test_compare(Tree const& a, size_t node_a,
     Tree const& b, size_t node_b,
     size_t level)
{
    ASSERT_NE(node_a, NONE);
    ASSERT_NE(node_b, NONE);
    ASSERT_LT(node_a, a.capacity());
    ASSERT_LT(node_b, b.capacity());

    EXPECT_EQ(a.type(node_a), b.type(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;

    EXPECT_EQ(a.has_key(node_a), b.has_key(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    if(a.has_key(node_a) && b.has_key(node_b))
    {
        EXPECT_EQ(a.key(node_a), b.key(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    }

    EXPECT_EQ(a.has_val(node_a), b.has_val(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    if(a.has_val(node_a) && b.has_val(node_b))
    {
        EXPECT_EQ(a.val(node_a), b.val(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    }

    EXPECT_EQ(a.has_key_tag(node_a), b.has_key_tag(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    if(a.has_key_tag(node_a) && b.has_key_tag(node_b))
    {
        EXPECT_EQ(a.key_tag(node_a), b.key_tag(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    }

    EXPECT_EQ(a.has_val_tag(node_a), b.has_val_tag(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    if(a.has_val_tag(node_a) && b.has_val_tag(node_b))
    {
        EXPECT_EQ(a.val_tag(node_a), b.val_tag(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    }

    EXPECT_EQ(a.has_key_anchor(node_a), b.has_key_anchor(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    if(a.has_key_anchor(node_a) && b.has_key_anchor(node_b))
    {
        EXPECT_EQ(a.key_anchor(node_a), b.key_anchor(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    }

    EXPECT_EQ(a.has_val_anchor(node_a), b.has_val_anchor(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    if(a.has_val_anchor(node_a) && b.has_val_anchor(node_b))
    {
        EXPECT_EQ(a.val_anchor(node_a), b.val_anchor(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    }

    EXPECT_EQ(a.num_children(node_a), b.num_children(node_b)) << "id_a=" << node_a << " vs id_b=" << node_b;
    for(size_t ia = a.first_child(node_a), ib = b.first_child(node_b);
        ia != NONE && ib != NONE;
        ia = a.next_sibling(ia), ib = b.next_sibling(ib))
    {
        test_compare(a, ia, b, ib, level+1);
    }
}

void test_arena_not_shared(Tree const& a, Tree const& b)
{
    for(NodeData *n = a.m_buf, *e = a.m_buf + a.m_cap; n != e; ++n)
    {
        EXPECT_FALSE(b.in_arena(n->m_key.scalar)) << n - a.m_buf;
        EXPECT_FALSE(b.in_arena(n->m_key.tag   )) << n - a.m_buf;
        EXPECT_FALSE(b.in_arena(n->m_key.anchor)) << n - a.m_buf;
        EXPECT_FALSE(b.in_arena(n->m_val.scalar)) << n - a.m_buf;
        EXPECT_FALSE(b.in_arena(n->m_val.tag   )) << n - a.m_buf;
        EXPECT_FALSE(b.in_arena(n->m_val.anchor)) << n - a.m_buf;
    }
    for(NodeData *n = b.m_buf, *e = b.m_buf + b.m_cap; n != e; ++n)
    {
        EXPECT_FALSE(a.in_arena(n->m_key.scalar)) << n - b.m_buf;
        EXPECT_FALSE(a.in_arena(n->m_key.tag   )) << n - b.m_buf;
        EXPECT_FALSE(a.in_arena(n->m_key.anchor)) << n - b.m_buf;
        EXPECT_FALSE(a.in_arena(n->m_val.scalar)) << n - b.m_buf;
        EXPECT_FALSE(a.in_arena(n->m_val.tag   )) << n - b.m_buf;
        EXPECT_FALSE(a.in_arena(n->m_val.anchor)) << n - b.m_buf;
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

// ensure coverage of the default callback report
#ifndef RYML_NO_DEFAULT_CALLBACKS
extern void report_error_impl(const char* msg, size_t len, Location loc, FILE *file);
#endif

std::string format_error(const char* msg, size_t len, Location loc)
{
    // ensure coverage of the default callback report
    #ifndef RYML_NO_DEFAULT_CALLBACKS
    report_error_impl(msg, len, loc, nullptr);
    #endif
    if(!loc) return msg;
    std::string out;
    if(!loc.name.empty()) c4::formatrs(append, &out, "{}:", loc.name);
    c4::formatrs(append, &out, "{}:{}:", loc.line, loc.col);
    if(loc.offset) c4::formatrs(append, &out, " (@{}B):", loc.offset);
    c4::formatrs(append, &out, "{}:", csubstr(msg, len));
    return out;
}

struct ExpectedError : public std::runtime_error
{
    Location error_location;
    ExpectedError(const char* msg, size_t len, Location loc)
        : std::runtime_error(format_error(msg, len, loc))
        , error_location(loc)
    {
    }
};


//-----------------------------------------------------------------------------

ExpectError::ExpectError(Location loc)
    : m_got_an_error(false)
    , m_prev(c4::yml::get_callbacks())
    , expected_location(loc)
{
    #ifdef RYML_NO_DEFAULT_CALLBACKS
    c4::yml::Callbacks cb((void*)this, m_prev.m_allocate, m_prev.m_free, &ExpectError::error);
    #else
    c4::yml::Callbacks cb((void*)this, nullptr,           nullptr,       &ExpectError::error);
    #endif
    c4::yml::set_callbacks(cb);
}

ExpectError::~ExpectError()
{
    c4::yml::set_callbacks(m_prev);
}

void ExpectError::error(const char* msg, size_t len, Location loc, void *user_data)
{
    ExpectError *this_ = reinterpret_cast<ExpectError*>(user_data);
    this_->m_got_an_error = true;
    throw ExpectedError(msg, len, loc);
}

void ExpectError::do_check(std::function<void()> fn, Location expected_location)
{
    auto context = ExpectError(expected_location);
    try
    {
        fn();
    }
    catch(ExpectedError const& e)
    {
        #ifdef RYML_DBG
        std::cout << "---------------\n";
        std::cout << "got an expected error:\n" << e.what() << "\n";
        std::cout << "---------------\n";
        #endif
        if(context.expected_location)
        {
            EXPECT_TRUE(e.error_location);
            EXPECT_EQ(e.error_location.line, context.expected_location.line);
            EXPECT_EQ(e.error_location.col, context.expected_location.col);
            if(context.expected_location.offset)
            {
                EXPECT_EQ(e.error_location.offset, context.expected_location.offset);
            }
        }
    };
    EXPECT_TRUE(context.m_got_an_error);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

using N = CaseNode;
using L = CaseNode::iseqmap;

TEST(CaseNode, setting_up)
{
    L tl1 = {DOC, DOC};
    L tl2 = {(DOC), (DOC)};

    ASSERT_EQ(tl1.size(), tl2.size());
    N const& d1 = *tl1.begin();
    N const& d2 = *(tl1.begin() + 1);
    ASSERT_EQ(d1.reccount(), d2.reccount());
    ASSERT_EQ(d1.type, DOC);
    ASSERT_EQ(d2.type, DOC);

    N n1(tl1);
    N n2(tl2);
    ASSERT_EQ(n1.reccount(), n2.reccount());
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
NodeType_e CaseNode::_guess() const
{
    NodeType t;
    C4_ASSERT(!val.empty() != !children.empty() || (val.empty() && children.empty()));
    if(children.empty())
    {
        C4_ASSERT(parent);
        if(key.empty())
        {
            t = VAL;
        }
        else
        {
            t = KEYVAL;
        }
    }
    else
    {
        NodeType_e has_key = key.empty() ? NOTYPE : KEY;
        auto const& ch = children.front();
        if(ch.key.empty())
        {
            t = (has_key|SEQ);
        }
        else
        {
            t = (has_key|MAP);
        }
    }
    if( ! key_tag.empty())
    {
        C4_ASSERT( ! key.empty());
        t.add(KEYTAG);
    }
    if( ! val_tag.empty())
    {
        C4_ASSERT( ! val.empty() || ! children.empty());
        t.add(VALTAG);
    }
    if( ! key_anchor.str.empty())
    {
        t.add(key_anchor.type);
    }
    if( ! val_anchor.str.empty())
    {
        t.add(val_anchor.type);
    }
    return t;
}


//-----------------------------------------------------------------------------
void CaseNode::compare_child(yml::NodeRef const& n, size_t pos) const
{
    EXPECT_TRUE(pos < n.num_children());
    EXPECT_TRUE(pos < children.size());

    if(pos >= n.num_children() || pos >= children.size()) return;

    ASSERT_GT(n.num_children(), pos);
    auto const& ch = children[pos];

    if(type & MAP)
    {
        ASSERT_TRUE(n.find_child(ch.key).valid());
        EXPECT_NE(n.find_child(ch.key).get(), nullptr);
        auto fch = n.find_child(ch.key);
        if(fch != nullptr)
        {
            // there may be duplicate keys.
            if(fch.id() != n[pos].id()) fch = n[pos];
            //EXPECT_EQ(fch, n[ch.key]);
            EXPECT_EQ(fch.get(), n[pos].get());
            //EXPECT_EQ(n[pos], n[ch.key]);
            EXPECT_EQ(n[ch.key].key(), ch.key);
        }
        else
        {
            printf("error: node should have child %.*s: ", (int)ch.key.len, ch.key.str);
            print_path(n);
            printf("\n");
            print_node(n);
        }
    }

    if(type & SEQ)
    {
        EXPECT_FALSE(n[pos].has_key());
        EXPECT_EQ(n[pos].get()->m_key.scalar, children[pos].key);
        auto fch = n.child(pos);
        EXPECT_EQ(fch.get(), n[pos].get());
    }

    if(ch.type & KEY)
    {
        auto fch = n[pos];
        EXPECT_TRUE(fch.has_key()) << "id=" << fch.id();
        if(fch.has_key())
        {
            EXPECT_EQ(fch.key(), ch.key) << "id=" << fch.id();
        }

        if( ! ch.key_tag.empty())
        {
            EXPECT_TRUE(fch.has_key_tag()) << "id=" << fch.id();
            if(fch.has_key_tag())
            {
                EXPECT_EQ(fch.key_tag(), ch.key_tag) << "id=" << fch.id();
            }
        }
    }

    if(ch.type & VAL)
    {
        auto fch = n[pos];
        EXPECT_TRUE(fch.has_val()) << "id=" << fch.id();
        if(fch.has_val())
        {
            EXPECT_EQ(fch.val(), ch.val) << "id=" << fch.id();
        }

        if( ! ch.val_tag.empty())
        {
            EXPECT_TRUE(fch.has_val_tag()) << "id=" << fch.id();
            if(fch.has_val_tag())
            {
                EXPECT_EQ(fch.val_tag(), ch.val_tag) << "id=" << fch.id();
            }
        }
    }
}

void CaseNode::compare(yml::NodeRef const& n) const
{
    EXPECT_EQ(n.get()->m_type, type) << "id=" << n.id(); // the type() method masks the type, and thus tag flags are omitted on its return value
    EXPECT_EQ(n.num_children(), children.size()) << "id=" << n.id();

    if(n.has_key())
    {
        EXPECT_EQ(n.key(), key) << "id=" << n.id();
    }

    if(n.has_val())
    {
        EXPECT_EQ(n.val(), val) << "id=" << n.id();
    }

    // check that the children are in the same order
    {
        EXPECT_EQ(children.size(), n.num_children()) << "id=" << n.id();

        size_t ic = 0;
        for(auto const &ch : children)
        {
            SCOPED_TRACE("comparing: iteration based on the ref children");
            (void)ch; // unused
            compare_child(n, ic++);
        }

        ic = 0;
        for(auto const ch : n.children())
        {
            SCOPED_TRACE("comparing: iteration based on the yml::Node children");
            (void)ch; // unused
            compare_child(n, ic++);
        }

        if(n.first_child() != nullptr)
        {
            ic = 0;
            for(auto const ch : n.first_child().siblings())
            {
                SCOPED_TRACE("comparing: iteration based on the yml::Node siblings");
                (void)ch; // unused
                compare_child(n, ic++);
            }
        }
    }

    for(size_t i = 0, ei = n.num_children(), j = 0, ej = children.size(); i < ei && j < ej; ++i, ++j)
    {
        children[j].compare(n[i]);
    }
}

void CaseNode::recreate(yml::NodeRef *n) const
{
    C4_ASSERT( ! n->has_children());
    auto *nd = n->get();
    nd->m_type = type|key_anchor.type|val_anchor.type;
    nd->m_key.scalar = key;
    nd->m_key.tag = (key_tag);
    nd->m_key.anchor = key_anchor.str;
    nd->m_val.scalar = val;
    nd->m_val.tag = (val_tag);
    nd->m_val.anchor = val_anchor.str;
    auto &tree = *n->tree();
    size_t nid = n->id(); // don't use node from now on
    for(auto const& ch : children)
    {
        size_t id = tree.append_child(nid);
        NodeRef chn(n->tree(), id);
        ch.recreate(&chn);
    }
}


//-----------------------------------------------------------------------------

void print_path(NodeRef const& n)
{
    size_t len = 0;
    char buf[1024];
    NodeRef p = n;
    while(p != nullptr)
    {
        if(p.has_key())
        {
            len += 1 + p.key().len;
        }
        else
        {
            int ret = snprintf(buf, sizeof(buf), "/%zd", p.has_parent() ? p.parent().child_pos(p) : 0);
            RYML_ASSERT(ret >= 0);
            len += static_cast<size_t>(ret);
        }
        p = p.parent();
    };
    C4_ASSERT(len < sizeof(buf));
    size_t pos = len;
    p = n;
    while(p != nullptr)
    {
        if(p.has_key())
        {
            size_t tl = p.key().len;
            int ret = snprintf(buf + pos - tl, tl, "%.*s", (int)tl, p.key().str);
            RYML_ASSERT(ret >= 0);
            pos -= static_cast<size_t>(ret);
        }
        else
        {
            pos = p.parent().child_pos(p);
            int ret = snprintf(buf, 0, "/%zd", pos);
            RYML_ASSERT(ret >= 0);
            size_t tl = static_cast<size_t>(ret);
            RYML_ASSERT(pos >= tl);
            ret = snprintf(buf + static_cast<size_t>(pos - tl), tl, "/%zd", pos);
            RYML_ASSERT(ret >= 0);
            pos -= static_cast<size_t>(ret);
        }
        p = p.parent();
    };
    printf("%.*s", (int)len, buf);
}



void print_node(CaseNode const& p, int level)
{
    printf("%*s%p", (2*level), "", (void*)&p);
    if( ! p.parent)
    {
        printf(" [ROOT]");
    }
    printf(" %s:", NodeType::type_str(p.type));
    if(p.has_key())
    {
        if(p.has_key_anchor())
        {
            csubstr ka = p.key_anchor.str;
            printf(" &%.*s", (int)ka.len, ka.str);
        }
        if(p.key_tag.empty())
        {
            csubstr v  = p.key;
            printf(" '%.*s'", (int)v.len, v.str);
        }
        else
        {
            csubstr vt = p.key_tag;
            csubstr v  = p.key;
            printf(" '%.*s %.*s'", (int)vt.len, vt.str, (int)v.len, v.str);
        }
    }
    if(p.has_val())
    {
        if(p.val_tag.empty())
        {
            csubstr v  = p.val;
            printf(" '%.*s'", (int)v.len, v.str);
        }
        else
        {
            csubstr vt = p.val_tag;
            csubstr v  = p.val;
            printf(" '%.*s %.*s'", (int)vt.len, vt.str, (int)v.len, v.str);
        }
    }
    else
    {
        if( ! p.val_tag.empty())
        {
            csubstr vt = p.val_tag;
            printf(" %.*s", (int)vt.len, vt.str);
        }
    }
    if(p.has_val_anchor())
    {
        auto &a = p.val_anchor.str;
        printf(" valanchor='&%.*s'", (int)a.len, a.str);
    }
    printf(" (%zd sibs)", p.parent ? p.parent->children.size() : 0);
    if(p.is_container())
    {
        printf(" %zd children:", p.children.size());
    }
    printf("\n");
}


void print_tree(NodeRef const& p, int level)
{
    print_node(p, level);
    for(NodeRef const ch : p.children())
    {
        print_tree(ch, level+1);
    }
}

void print_tree(CaseNode const& p, int level)
{
    print_node(p, level);
    for(auto const& ch : p.children)
    {
        print_tree(ch, level+1);
    }
}

void print_tree(CaseNode const& t)
{
    printf("--------------------------------------\n");
    print_tree(t, 0);
    printf("#nodes: %zd\n", t.reccount());
    printf("--------------------------------------\n");
}

void test_invariants(NodeRef const n)
{
    if(n.is_root())
    {
        EXPECT_FALSE(n.has_other_siblings());
    }
    // keys or vals cannot be root
    if(n.has_key() || n.is_val() || n.is_keyval())
    {
        EXPECT_FALSE(n.is_root());
        EXPECT_FALSE(n.is_root());
        EXPECT_FALSE(n.is_root());
    }
    // vals cannot be containers
    if( ! n.empty() && ! n.is_doc())
    {
        EXPECT_NE(n.has_val(), n.is_container());
    }
    if(n.has_children())
    {
        EXPECT_TRUE(n.is_container());
        EXPECT_FALSE(n.is_val());
    }
    // check parent & sibling reciprocity
    for(NodeRef const s : n.siblings())
    {
        EXPECT_TRUE(n.has_sibling(s));
        EXPECT_TRUE(s.has_sibling(n));
        EXPECT_EQ(s.parent().get(), n.parent().get());
    }
    if(n.parent() != nullptr)
    {
        EXPECT_TRUE(n.parent().has_child(n));
        EXPECT_EQ(n.parent().num_children(), n.num_siblings());
        // doc parent must be a seq and a stream
        if(n.is_doc())
        {
            EXPECT_TRUE(n.parent().is_seq());
            EXPECT_TRUE(n.parent().is_stream());
        }
    }
    else
    {
        EXPECT_TRUE(n.is_root());
    }
    if(n.is_seq())
    {
        EXPECT_TRUE(n.is_container());
        EXPECT_FALSE(n.is_map());
        for(NodeRef const ch : n.children())
        {
            EXPECT_FALSE(ch.is_keyval());
            EXPECT_FALSE(ch.has_key());
        }
    }
    if(n.is_map())
    {
        EXPECT_TRUE(n.is_container());
        EXPECT_FALSE(n.is_seq());
        for(NodeRef const ch : n.children())
        {
            EXPECT_TRUE(ch.has_key());
        }
    }
    if(n.has_key_anchor())
    {
        EXPECT_FALSE(n.key_anchor().empty());
        EXPECT_FALSE(n.is_key_ref());
    }
    if(n.has_val_anchor())
    {
        EXPECT_FALSE(n.val_anchor().empty());
        EXPECT_FALSE(n.is_val_ref());
    }
    if(n.is_key_ref())
    {
        EXPECT_FALSE(n.key_ref().empty());
        EXPECT_FALSE(n.has_key_anchor());
    }
    if(n.is_val_ref())
    {
        EXPECT_FALSE(n.val_ref().empty());
        EXPECT_FALSE(n.has_val_anchor());
    }
    // ... add more tests here

    // now recurse into the children
    for(NodeRef const ch : n.children())
    {
        test_invariants(ch);
    }
}

size_t test_tree_invariants(NodeRef const& n)
{
    auto parent = n.parent();

    if(n.get()->m_prev_sibling == NONE)
    {
        if(parent != nullptr)
        {
            EXPECT_EQ(parent.first_child().get(), n.get());
            EXPECT_EQ(parent.first_child().id(), n.id());
        }
    }

    if(n.get()->m_next_sibling == NONE)
    {
        if(parent != nullptr)
        {
            EXPECT_EQ(parent.last_child().get(), n.get());
            EXPECT_EQ(parent.last_child().id(), n.id());
        }
    }

    if(parent == nullptr)
    {
        EXPECT_TRUE(n.is_root());
        EXPECT_EQ(n.prev_sibling().get(), nullptr);
        EXPECT_EQ(n.next_sibling().get(), nullptr);
    }

    size_t count = 1, num = 0;
    for(NodeRef const ch : n.children())
    {
        EXPECT_NE(ch.id(), n.id());
        count += test_tree_invariants(ch);
        ++num;
    }

    EXPECT_EQ(num, n.num_children());

    return count;
}

void test_invariants(Tree const& t)
{

    EXPECT_LE(t.size(), t.capacity());
    EXPECT_EQ(t.size() + t.slack(), t.capacity());

    if(t.empty()) return;

    size_t count = test_tree_invariants(t.rootref());
    EXPECT_EQ(count, t.size());

    check_invariants(t);

    return;
#if 0 == 1
    for(size_t i = 0; i < t.m_size; ++i)
    {
        auto n = t.get(i);
        if(n->m_prev_sibling == NONE)
        {
            EXPECT_TRUE(i == t.m_head || i == t.m_free_head);
        }
        if(n->m_next_sibling == NONE)
        {
            EXPECT_TRUE(i == t.m_tail || i == t.m_free_tail);
        }
    }

    std::vector<bool> touched(t.capacity());

    for(size_t i = t.m_head; i != NONE; i = t.get(i)->m_next_sibling)
    {
        touched[i] = true;
    }

    size_t size = 0;
    for(auto v : touched)
    {
        size += v;
    }

    EXPECT_EQ(size, t.size());

    touched.clear();
    touched.resize(t.capacity());

    for(size_t i = t.m_free_head; i != NONE; i = t.get(i)->m_next_sibling)
    {
        touched[i] = true;
    }

    size_t slack = 0;
    for(auto v : touched)
    {
        slack += v;
    }

    EXPECT_EQ(slack, t.slack());
    EXPECT_EQ(size+slack, t.capacity());

    // there are more checks to be done
#endif
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef JAVAI
int do_test()
{
    using namespace c4::yml;

    using C = Case;
    using N = CaseNode;
    using L = CaseNode::iseqmap;



    CaseContainer tests({
//-----------------------------------------------------------------------------
// https://en.wikipedia.org/wiki/YAML

//-----------------------------------------------------------------------------
C("literal block scalar as map entry",
R"(
data: |
   There once was a short man from Ealing
   Who got on a bus to Darjeeling
       It said on the door
       \"Please don't spit on the floor\"
   So he carefully spat on the ceiling
)",
     N{"data", "There once was a short man from Ealing\nWho got on a bus to Darjeeling\n    It said on the door\n    \"Please don't spit on the floor\"\nSo he carefully spat on the ceiling\n"}
),

//-----------------------------------------------------------------------------
C("folded block scalar as map entry",
R"(
data: >
   Wrapped text
   will be folded
   into a single
   paragraph

   Blank lines denote
   paragraph breaks
)",
  N{"data", "Wrapped text will be folded into a single paragraph\nBlank lines denote paragraph breaks\n"}
),

//-----------------------------------------------------------------------------
C("two scalars in a block, html example",
R"(
---
example: >
        HTML goes into YAML without modification
message: |
        <blockquote style=\"font: italic 12pt Times\">
        <p>\"Three is always greater than two,
           even for large values of two\"</p>
        <p>--Author Unknown</p>
        </blockquote>
date: 2007-06-01
)",
     N{DOC, L{
          N{"example", "HTML goes into YAML without modification"},
          N{"message", R"(<blockquote style=\"font: italic 12pt Times\">
<p>\"Three is always greater than two,
   even for large values of two\"</p>
<p>--Author Unknown</p>
</blockquote>
)"},
          N{"date","2007-06-01"},
              }}
),



//-----------------------------------------------------------------------------
C("scalar block, literal, no chomp, no indentation",
R"(example: |
  Several lines of text,
  with some \"quotes\" of various 'types',
  and also a blank line:

  plus another line at the end.

another: text
)",
     L{
      N{"example", "Several lines of text,\nwith some \"quotes\" of various 'types',\nand also a blank line:\n\nplus another line at the end.\n"},
      N{"another", "text"},
          }
),

//-----------------------------------------------------------------------------
C("scalar block, folded, no chomp, no indentation",
R"(example: >
  Several lines of text,
  with some \"quotes\" of various 'types',
  and also a blank line:

  plus another line at the end.

another: text
)",
     L{
      N{"example", "Several lines of text,  with some \"quotes\" of various 'types',  and also a blank line:\nplus another line at the end.\n"},
      N{"another", "text"},
          }
),
    }); // end examples

    return tests.run();
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

CaseData* get_data(csubstr name)
{
    static std::map<csubstr, CaseData> m;

    auto it = m.find(name);
    CaseData *cd;
    if(it == m.end())
    {
        cd = &m[name];
        Case const* c = get_case(name);
        RYML_CHECK(c->src.find("\n\r") == csubstr::npos);
        {
            std::string tmp;
            replace_all("\r", "", c->src, &tmp);
            cd->unix_style.src_buf.assign(tmp.begin(), tmp.end());
            cd->unix_style.src = to_substr(cd->unix_style.src_buf);
        }
        {
            std::string tmp;
            replace_all("\n", "\r\n", cd->unix_style.src, &tmp);
            cd->windows_style.src_buf.assign(tmp.begin(), tmp.end());
            cd->windows_style.src = to_substr(cd->windows_style.src_buf);
        }
    }
    else
    {
        cd = &it->second;
    }
    return cd;
}

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
