#include "./test_group.hpp"
#include "c4/yml/detail/print.hpp"
#include "test_case.hpp"
#include <c4/fs/fs.hpp>
#include <fstream>
#include <stdexcept>

// this is needed to include yaml-cpp (!)
#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4127/*conditional expression is constant*/)
#   pragma warning(disable: 4389/*'==': signed/unsigned mismatch*/)
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wshadow"
#   pragma clang diagnostic ignored "-Wfloat-equal"
#   pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wshadow"
#   pragma GCC diagnostic ignored "-Wfloat-equal"
#   pragma GCC diagnostic ignored "-Wunused-parameter"
#   pragma GCC diagnostic ignored "-Wconversion"
#endif
#include <yaml-cpp/yaml.h>
#if defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif


//-----------------------------------------------------------------------------
namespace c4 {
namespace yml {

void YmlTestCase::_test_parse_using_ryml(CaseDataLineEndings *cd)
{
#ifdef RYML_DBG
    std::cout << "---------------\n";
    std::cout << c->src;
    std::cout << "---------------\n";
#endif

    if(c->flags & EXPECT_PARSE_ERROR)
    {
        ExpectError::do_check([cd](){
            parse(cd->src, &cd->parsed_tree);
            #ifdef RYML_DBG
            // if this point was reached, then it means that the expected
            // error failed to occur. So print debugging info.
            std::cout << "failed to catch expected error while parsing.\nPARSED TREE:\n";
            print_tree(cd->parsed_tree);
            #endif
        }, c->expected_location);
        return;
    }

    parse(cd->src, &cd->parsed_tree);
    {
        SCOPED_TRACE("checking tree invariants of unresolved parsed tree");
        test_invariants(cd->parsed_tree);
    }
#ifdef RYML_DBG
    std::cout << "REF TREE:\n";
    print_tree(c->root);
    std::cout << "PARSED TREE:\n";
    print_tree(cd->parsed_tree);
#endif
    {
        SCOPED_TRACE("checking node invariants of unresolved parsed tree");
        test_invariants(cd->parsed_tree.rootref());
    }

    if(c->flags & RESOLVE_REFS)
    {
        cd->parsed_tree.resolve();
#ifdef RYML_DBG
        std::cout << "resolved tree!!!\n";
        print_tree(cd->parsed_tree);
#endif
        {
            SCOPED_TRACE("checking tree invariants of resolved parsed tree");
            test_invariants(cd->parsed_tree);
        }
        {
            SCOPED_TRACE("checking node invariants of resolved parsed tree");
            test_invariants(cd->parsed_tree.rootref());
        }
    }

    {
        SCOPED_TRACE("comparing parsed tree to ref tree");
        EXPECT_GE(cd->parsed_tree.capacity(), c->root.reccount());
        EXPECT_EQ(cd->parsed_tree.size(), c->root.reccount());
        c->root.compare(cd->parsed_tree.rootref());
    }

    if(c->flags & RESOLVE_REFS)
    {
        cd->parsed_tree.reorder();
#ifdef RYML_DBG
        std::cout << "reordered tree!!!\n";
        print_tree(cd->parsed_tree);
#endif
        {
            SCOPED_TRACE("checking tree invariants of reordered parsed tree after resolving");
            test_invariants(cd->parsed_tree);
        }
        {
            SCOPED_TRACE("checking node invariants of reordered parsed tree after resolving");
            test_invariants(cd->parsed_tree.rootref());
        }

        {
            SCOPED_TRACE("comparing parsed tree to ref tree");
            EXPECT_GE(cd->parsed_tree.capacity(), c->root.reccount());
            EXPECT_EQ(cd->parsed_tree.size(), c->root.reccount());
            c->root.compare(cd->parsed_tree.rootref());
        }
    }
}


//-----------------------------------------------------------------------------
void YmlTestCase::_test_emit_yml_stdout(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    if(cd->parsed_tree.empty())
    {
        parse(cd->src, &cd->parsed_tree);
    }
    if(cd->emit_buf.empty())
    {
        cd->emitted_yml = emitrs(cd->parsed_tree, &cd->emit_buf);
    }

    cd->numbytes_stdout = emit(cd->parsed_tree);
}

//-----------------------------------------------------------------------------
void YmlTestCase::_test_emit_yml_cout(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    if(cd->parsed_tree.empty())
    {
        parse(cd->src, &cd->parsed_tree);
    }
    if(cd->emit_buf.empty())
    {
        cd->emitted_yml = emitrs(cd->parsed_tree, &cd->emit_buf);
    }

    std::cout << cd->parsed_tree;
}


//-----------------------------------------------------------------------------
void YmlTestCase::_test_emit_yml_stringstream(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;

    std::string s;
    std::vector<char> v;
    csubstr sv = emitrs(cd->parsed_tree, &v);

    {
        std::stringstream ss;
        ss << cd->parsed_tree;
        s = ss.str();
        EXPECT_EQ(sv, s);
    }

    {
        std::stringstream ss;
        ss << cd->parsed_tree.rootref();
        s = ss.str();

        csubstr sv2 = emitrs(cd->parsed_tree, &v);
        EXPECT_EQ(sv2, s);
    }
}

//-----------------------------------------------------------------------------
void YmlTestCase::_test_emit_yml_ofstream(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    auto s = emitrs<std::string>(cd->parsed_tree);
    auto fn = c4::fs::tmpnam<std::string>();
    {
        std::ofstream f(fn);
        f << cd->parsed_tree;
    }
    auto r = c4::fs::file_get_contents<std::string>(fn.c_str());
    c4::fs::delete_file(fn.c_str());
    // using ofstream will use \r\n. So delete it.
    std::string filtered;
    filtered.reserve(r.size());
    for(auto c_ : r)
    {
        if(c_ != '\r') filtered += c_;
    }
    EXPECT_EQ(s, filtered);
}

//-----------------------------------------------------------------------------
void YmlTestCase::_test_emit_yml_string(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    auto em = emitrs(cd->parsed_tree, &cd->emit_buf);
    EXPECT_EQ(em.len, cd->emit_buf.size());
    EXPECT_EQ(em.len, cd->numbytes_stdout);
    cd->emitted_yml = em;

#ifdef RYML_DBG
    std::cout << em;
#endif
}

//-----------------------------------------------------------------------------
void YmlTestCase::_test_emitrs(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    using vtype = std::vector<char>;
    using stype = std::string;

    vtype vv, v = emitrs<vtype>(cd->parsed_tree);
    stype ss, s = emitrs<stype>(cd->parsed_tree);
    EXPECT_EQ(to_csubstr(v), to_csubstr(s));

    csubstr svv = emitrs(cd->parsed_tree, &vv);
    csubstr sss = emitrs(cd->parsed_tree, &ss);
    EXPECT_EQ(svv, sss);
}

//-----------------------------------------------------------------------------
void YmlTestCase::_test_emitrs_cfile(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    auto s = emitrs<std::string>(cd->parsed_tree);
    std::string r;
    {
        c4::fs::ScopedTmpFile f;
        emit(cd->parsed_tree, f.m_file);
        fflush(f.m_file);
        r = f.contents<std::string>();
    }
    EXPECT_EQ(s, r);
}


//-----------------------------------------------------------------------------
void YmlTestCase::_test_complete_round_trip(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;
    if(cd->parsed_tree.empty())
    {
        parse(cd->src, &cd->parsed_tree);
    }
    if(cd->emit_buf.empty())
    {
        cd->emitted_yml = emitrs(cd->parsed_tree, &cd->emit_buf);
    }

#ifdef RYML_DBG
    print_tree(cd->parsed_tree);
    std::cout << cd->emitted_yml;
#endif

    {
        SCOPED_TRACE("parsing emitted yml");
        cd->parse_buf = cd->emit_buf;
        cd->parsed_yml.assign(cd->parse_buf.data(), cd->parse_buf.size());
        parse(cd->parsed_yml, &cd->emitted_tree);
#ifdef RYML_DBG
        print_tree(cd->emitted_tree);
#endif
    }

    {
        SCOPED_TRACE("checking node invariants of parsed tree");
        test_invariants(cd->emitted_tree.rootref());
    }

    {
        SCOPED_TRACE("checking tree invariants of parsed tree");
        test_invariants(cd->emitted_tree);
    }

    {
        SCOPED_TRACE("comparing parsed tree to ref tree");
        EXPECT_GE(cd->emitted_tree.capacity(), c->root.reccount());
        EXPECT_EQ(cd->emitted_tree.size(), c->root.reccount());
        c->root.compare(cd->emitted_tree.rootref());
    }
}

//-----------------------------------------------------------------------------
void YmlTestCase::_test_recreate_from_ref(CaseDataLineEndings *cd)
{
    if(c->flags & EXPECT_PARSE_ERROR) return;

    if(cd->parsed_tree.empty())
    {
        parse(cd->src, &cd->parsed_tree);
    }
    if(cd->emit_buf.empty())
    {
        cd->emitted_yml = emitrs(cd->parsed_tree, &cd->emit_buf);
    }

    {
        SCOPED_TRACE("recreating a new tree from the ref tree");
        cd->recreated.reserve(cd->parsed_tree.size());
        NodeRef r = cd->recreated.rootref();
        c->root.recreate(&r);
    }

    {
        SCOPED_TRACE("checking node invariants of recreated tree");
        test_invariants(cd->recreated.rootref());
    }

    {
        SCOPED_TRACE("checking tree invariants of recreated tree");
        test_invariants(cd->recreated);
    }

    {
        SCOPED_TRACE("comparing recreated tree to ref tree");
        c->root.compare(cd->recreated.rootref());
    }
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, parse_unix_using_ryml)
{
    SCOPED_TRACE("unix style");
    _test_parse_using_ryml(&d->unix_style);
}

TEST_P(YmlTestCase, parse_windows_using_ryml)
{
    SCOPED_TRACE("windows style");
    _test_parse_using_ryml(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, emit_yml_unix_stdout)
{
    SCOPED_TRACE("unix style");
    _test_emit_yml_stdout(&d->unix_style);
}

TEST_P(YmlTestCase, emit_yml_windows_stdout)
{
    SCOPED_TRACE("windows style");
    _test_emit_yml_stdout(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, emit_yml_unix_cout)
{
    SCOPED_TRACE("unix style");
    _test_emit_yml_cout(&d->unix_style);
}

TEST_P(YmlTestCase, emit_yml_windows_cout)
{
    SCOPED_TRACE("windows style");
    _test_emit_yml_cout(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, emit_yml_unix_stringstream)
{
    SCOPED_TRACE("unix style");
    _test_emit_yml_stringstream(&d->unix_style);
}

TEST_P(YmlTestCase, emit_yml_windows_stringstream)
{
    SCOPED_TRACE("windows style");
    _test_emit_yml_stringstream(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, emit_yml_unix_ofstream)
{
    SCOPED_TRACE("unix style");
    _test_emit_yml_ofstream(&d->unix_style);
}

TEST_P(YmlTestCase, emit_yml_windows_ofstream)
{
    SCOPED_TRACE("windows style");
    _test_emit_yml_ofstream(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, emit_yml_unix_string)
{
    SCOPED_TRACE("unix style");
    _test_emit_yml_string(&d->unix_style);
}

TEST_P(YmlTestCase, emit_yml_windows_string)
{
    SCOPED_TRACE("windows style");
    _test_emit_yml_string(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, unix_emitrs)
{
    SCOPED_TRACE("unix style");
    _test_emitrs(&d->unix_style);
}

TEST_P(YmlTestCase, windows_emitrs)
{
    SCOPED_TRACE("windows style");
    _test_emitrs(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, unix_emitrs_cfile)
{
    SCOPED_TRACE("unix style");
    _test_emitrs_cfile(&d->unix_style);
}

TEST_P(YmlTestCase, windows_emitrs_cfile)
{
    SCOPED_TRACE("windows style");
    _test_emitrs_cfile(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, complete_unix_round_trip)
{
    SCOPED_TRACE("unix style");
    _test_complete_round_trip(&d->unix_style);
}

TEST_P(YmlTestCase, complete_windows_round_trip)
{
    SCOPED_TRACE("windows style");
    _test_complete_round_trip(&d->windows_style);
}

//-----------------------------------------------------------------------------
TEST_P(YmlTestCase, unix_recreate_from_ref)
{
    SCOPED_TRACE("unix style");
    _test_recreate_from_ref(&d->unix_style);
}

TEST_P(YmlTestCase, windows_recreate_from_ref)
{
    SCOPED_TRACE("windows style");
    _test_recreate_from_ref(&d->windows_style);
}


} // namespace yml
} // namespace c4
