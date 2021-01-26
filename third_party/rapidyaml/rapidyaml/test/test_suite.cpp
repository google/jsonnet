#include <gtest/gtest.h>

#include <c4/substr.hpp>
#include <c4/std/string.hpp>
#include <c4/fs/fs.hpp>
#include <c4/log/log.hpp>

#include <c4/yml/tree.hpp>
#include <c4/yml/parse.hpp>
#include <c4/yml/emit.hpp>
#include <c4/yml/detail/print.hpp>
#include <c4/yml/detail/checks.hpp>

#include "test_case.hpp"

namespace c4 {
namespace yml {


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// some utility functions, used below


size_t find_first_after(size_t pos, std::initializer_list<size_t> candidates)
{
    size_t ret = npos;
    for(size_t s : candidates)
    {
        if(s > pos && s < ret) ret = s;
    }
    return ret;
}


c4::csubstr filter_out_indentation(c4::csubstr src, std::string *dst)
{
    if( ! src.begins_with("    "))
    {
        dst->assign(src.begin(), src.end());
        return c4::to_csubstr(*dst);
    }
    return replace_all("\n    ", "\n", src.sub(4), dst);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/* Each case from the test suite contains:
 *
 *  - (awkward) input yaml (in_yaml)
 *  - (somewhat standard) output equivalent (out_yaml)
 *  - (when meaningful/possible) json equivalent (in_json)
 *  - yaml parsing events (events)
 *
 * Running a test consists of parsing the contents above into a data
 * structure, and then repeatedly parsing and emitting yaml in a sort
 * of pipe. Ie, (eg for in_yaml) parse in_yaml, emit corresponding
 * yaml, then parse this emitted yaml, and so on. Each parse/emit pair
 * is named a processing level in this test. */


// this is the number of processing levels
#define NLEVELS 4


typedef enum {
    CPART_NONE = 0,
    CPART_IN_YAML = 1 << 0,
    CPART_IN_JSON = 1 << 1,
    CPART_OUT_YAML = 1 << 2,
    CPART_EMIT_YAML = 1 << 3,
    CPART_EVENTS = 1 << 4,
    CPART_ALL = CPART_IN_YAML|CPART_IN_JSON|CPART_OUT_YAML|CPART_EMIT_YAML|CPART_EVENTS,
    CPART_ANY = CPART_ALL,
} CasePart_e;
constexpr CasePart_e operator| (CasePart_e lhs, CasePart_e rhs) noexcept { return (CasePart_e)((int)lhs|(int)rhs); }


c4::csubstr to_csubstr(CasePart_e cp) noexcept
{
    if(cp == CPART_NONE) return {"NONE"};
    else if(cp == CPART_IN_YAML) return {"IN_YAML"};
    else if(cp == CPART_IN_JSON) return {"IN_JSON"};
    else if(cp == CPART_OUT_YAML) return {"OUT_YAML"};
    else if(cp == CPART_EVENTS) return {"EVENTS"};
    else if(cp == CPART_ALL) return {"ALL"};
    return {"<unknown>"};
}


struct AllowedFailure
{
    c4::csubstr test_code;
    CasePart_e contexts;
    c4::csubstr reason;
    constexpr AllowedFailure() : test_code(), contexts(), reason() {}
    constexpr AllowedFailure(c4::csubstr tc, CasePart_e ctx, c4::csubstr r) : test_code(tc), contexts(ctx), reason(r) {}
    bool skip() const { return skip(CPART_ALL); }
    bool skip(CasePart_e case_part) const
    {
        if((case_part != CPART_ALL && (contexts & case_part))
           ||
           (case_part == CPART_ALL && (contexts == CPART_ALL)))
        {
            c4::log("skipping test {} in {}: matches {}. Reason: {}",
                    test_code, to_csubstr(case_part), to_csubstr(contexts), reason);
            return true;
        }
        return false;
    }
};


// don't forget to list these allowed failures in the repo's README.md,
// under the section "Known limitations"
constexpr const AllowedFailure g_allowed_failures[] = {
    {"3UYS", CPART_IN_YAML, "no need to escape the slash in \"a\\/b\""},
    // malformed json
    {"35KP", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"6XDY", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"6ZKB", CPART_IN_JSON|CPART_IN_YAML, "malformed JSON from multiple documents|TODO[next]: document handling"},
    {"7Z25", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"9DXL", CPART_IN_JSON|CPART_IN_YAML, "malformed JSON from multiple documents|TODO[next]: document handling"},
    {"9KAX", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"JHB9", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"KSS4", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"M7A3", CPART_IN_JSON|CPART_IN_YAML, "malformed JSON from multiple documents|TODO[next]: plain scalar parsing, same indentation on next line is problematic"},
    {"RZT7", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"U9NS", CPART_IN_JSON, "malformed JSON from multiple documents"},
    {"W4TN", CPART_IN_JSON, "malformed JSON from multiple documents"},
    // we do not accept container keys
    {"4FJ6", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"6BFJ", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"6PBE", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"KK5P", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"KZN9", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"LX3P", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"M5DY", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"Q9WF", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"SBG9", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"X38W", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},
    {"XW4D", CPART_ALL, "only string keys allowed (keys cannot be maps or seqs}"},

    // TODO
    {"735Y", CPART_IN_YAML, "TODO[next]: plain scalar parsing"},
    {"EXG3", CPART_IN_YAML, "TODO[next]: plain scalar parsing, same indentation on next line is problematic"},
    {"82AN", CPART_IN_YAML, "TODO[next]: plain scalar parsing, same indentation on next line is problematic"},
    {"9YRD", CPART_IN_YAML, "TODO[next]: plain scalar parsing, same indentation on next line is problematic"},
    {"EX5H", CPART_IN_YAML, "TODO[next]: plain scalar parsing, same indentation on next line is problematic"},
    {"HS5T", CPART_IN_YAML, "TODO[next]: plain scalar parsing, same indentation on next line is problematic"},
    {"7T8X", CPART_IN_YAML|CPART_OUT_YAML, "TODO[next]: scalar block parsing"},
    {"RZP5", CPART_IN_YAML|CPART_OUT_YAML, "TODO[next]: plain scalar block parsing, anchors"},
    {"FH7J", CPART_IN_YAML|CPART_OUT_YAML, "TODO[next]: implicit keys"},
    {"PW8X", CPART_IN_YAML|CPART_OUT_YAML, "TODO[next]: anchors with implicit key"},
    {"CN3R", CPART_IN_YAML|CPART_OUT_YAML, "TODO[next]: anchors + maps nested in seqs"},
    {"6BCT", CPART_IN_YAML, "TODO[hard]: allow tabs after - or :"},
    {"DC7X", CPART_IN_YAML, "TODO[next]: improve handling of tab characters"},
    {"G5U8", CPART_ALL, "TODO[next]: sequences with -"},
    {"K858", CPART_OUT_YAML|CPART_IN_JSON, "TODO[next]: emitting block scalars is not idempotent"},
    {"NAT4", CPART_IN_YAML|CPART_IN_JSON, "TODO[next]: emitting block scalars is not idempotent"},
    {"9MMW", CPART_IN_YAML, "TODO[next]: re the json/yaml incompatibility where a space is required after :"},
};


AllowedFailure is_failure_expected(c4::csubstr filename)
{
    RYML_CHECK(filename.ends_with(".tml"));
    auto test_code = filename.basename();
    test_code = test_code.offs(0, 4);
    for(auto af : g_allowed_failures)
    {
        if(af.test_code == test_code)
        {
            return af;
        }
    }
    return {};
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** a processing level */
struct ProcLevel
{
    c4::csubstr     filename;
    std::string     src;
    c4::yml::Parser parser;
    c4::yml::Tree   tree;
    std::string     emitted;

    AllowedFailure  allowed_failure;
    CasePart_e      case_part;
    bool            immutable = false;
    bool            reuse = false;
    bool            was_parsed = false;
    bool            was_emitted = false;

    void init(c4::csubstr filename_, c4::csubstr src_, bool immutable_, bool reuse_, CasePart_e case_part_, AllowedFailure af)
    {
        filename = filename_;
        src.assign(src_.begin(), src_.end());
        allowed_failure = af;
        case_part = case_part_;
        immutable = immutable_;
        reuse = reuse_;
        was_parsed = false;
        was_emitted = false;
    }

    void receive_src(ProcLevel & prev)
    {
        if(!prev.was_emitted)
        {
            prev.emit();
        }
        if(src != prev.emitted)
        {
            was_parsed = false;
            was_emitted = false;
            src = prev.emitted;
        }
    }

    template<class T>
    void log(const char* context, T const& v)
    {
        C4_UNUSED(context);
        C4_UNUSED(v);
#ifdef RYML_DBG
        constexpr const char sep[] = "+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+\n";
        c4::log("{}:\n{}{}{}", context, sep, v, sep);
#endif
    }

    void parse()
    {
        if(allowed_failure.skip(case_part)) return;
        if(was_parsed) return;
        log("parsing source", src);
        if(case_part == CPART_EVENTS)
        {
            C4_NOT_IMPLEMENTED();
        }
        else
        {
            if(reuse)
            {
                tree.clear();
                if(immutable)
                {
                    parser.parse(filename, c4::to_csubstr(src), &tree);
                }
                else
                {
                    parser.parse(filename, c4::to_substr(src), &tree);
                }
            }
            else
            {
                if(immutable)
                {
                    tree = c4::yml::parse(filename, c4::to_csubstr(src));
                }
                else
                {
                    tree = c4::yml::parse(filename, c4::to_substr(src));
                }
            }
        }
        was_parsed = true;
#ifdef RYML_DBG
        c4::yml::print_tree(tree);
#endif
    }

    void emit()
    {
        if(allowed_failure.skip(case_part)) return;
        if(was_emitted) return;
        if(!was_parsed) parse();
        emitrs(tree, &emitted);
        auto ss = c4::to_csubstr(emitted);
        if(ss.ends_with("\n...\n"))
        {
            emitted.resize(emitted.size() - 4);
        }
        log("emitted YAML", emitted);
        was_emitted = true;
        #ifdef RYML_DBG
        c4::log("EMITTED:\n{}", emitted);
        #endif
    }

    void compare_trees(ProcLevel const& prev)
    {
        if(allowed_failure.skip(case_part)) return;
        if(!was_parsed) parse();
        #ifdef RYML_DBG
        c4::print("PREV:"); print_tree(prev.tree);
        c4::print("CURR:"); print_tree(tree);
        #endif
        test_compare(tree, prev.tree);
    }

    void compare_emitted(ProcLevel const& prev)
    {
        if(allowed_failure.skip(case_part)) return;
        if(!was_emitted) emit();
        #ifdef RYML_DBG
        c4::log("PREV:\n{}", prev.emitted);
        c4::log("CURR:\n{}", emitted);
        #endif
        EXPECT_EQ(emitted, prev.emitted);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** holds data for one particular test suite approach. */
struct Approach
{
    ProcLevel levels[NLEVELS];
    bool enabled = false;

    void init(c4::csubstr filename, c4::csubstr src, bool immutable_, bool reuse_, CasePart_e case_part, AllowedFailure af)
    {
        enabled = true;
        for(auto &l : levels)
        {
            l.init(filename, src, immutable_, reuse_, case_part, af);
            if(case_part == CPART_EVENTS)
            {
                case_part = CPART_IN_YAML; // only the first one
            }
        }
    }

    void parse(size_t num, bool emit)
    {
        for(size_t i = 0; i < num; ++i)
        {
            levels[i].parse();
            if(emit) levels[i].emit();
            if(i + 1 < num)
            {
                levels[i+1].receive_src(levels[i]);
            }
        }
    }

    void compare_trees(size_t num)
    {
        for(size_t i = 1; i < num; ++i)
        {
            levels[i].compare_trees(levels[i-1]);
        }
    }
    void compare_trees(size_t num, Approach const& other)
    {
        for(size_t i = 0; i < num; ++i)
        {
            levels[i].compare_trees(other.levels[i]);
        }
    }

    void compare_emitted(size_t num)
    {
        for(size_t i = 1; i < num; ++i)
        {
            levels[i].compare_emitted(levels[i-1]);
        }
    }
    void compare_emitted(size_t num, Approach const& other)
    {
        for(size_t i = 0; i < num; ++i)
        {
            levels[i].compare_emitted(other.levels[i]);
        }
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
/** Each approach can be read from mutable/immutable yaml source and
 * with/without reuse. */
struct Subject
{
    Approach unix_ro;
    Approach unix_ro_reuse;
    Approach unix_rw;
    Approach unix_rw_reuse;

    Approach windows_ro;
    Approach windows_ro_reuse;
    Approach windows_rw;
    Approach windows_rw_reuse;

    std::string unix_src;
    std::string windows_src;

    void init(c4::csubstr filename, c4::csubstr src, CasePart_e case_part, AllowedFailure af)
    {
        src = replace_all("\r", "", src, &unix_src);

        unix_ro      .init(filename, src, /*immutable*/true , /*reuse*/false, case_part, af);
        unix_ro_reuse.init(filename, src, /*immutable*/true , /*reuse*/true , case_part, af);
        unix_rw      .init(filename, src, /*immutable*/false, /*reuse*/false, case_part, af);
        unix_rw_reuse.init(filename, src, /*immutable*/false, /*reuse*/true , case_part, af);

        src = replace_all("\n", "\r\n", src, &windows_src);

        windows_ro      .init(filename, src, /*immutable*/true , /*reuse*/false, case_part, af);
        windows_ro_reuse.init(filename, src, /*immutable*/true , /*reuse*/true , case_part, af);
        windows_rw      .init(filename, src, /*immutable*/false, /*reuse*/false, case_part, af);
        windows_rw_reuse.init(filename, src, /*immutable*/false, /*reuse*/true , case_part, af);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** now finally all the ways that a test case can be processed are
 * available through this class. Tests are defined below and use only
 * one of these. */
struct SuiteCase
{
    c4::csubstr filename;
    std::string file_contents;

    c4::csubstr desc;
    c4::csubstr from;
    c4::csubstr tags;

    Subject     in_yaml;
    Subject     in_json;
    Subject     out_yaml;
    Subject     emit_yaml;
    Subject     events;

    static c4::csubstr src(Subject const& s)
    {
        return c4::to_csubstr(s.unix_ro.levels[0].src);
    }

    /** loads the several types of tests from an input test suite
     * template file (tml)*/
    bool load(const char* filename_, AllowedFailure allowed_failure)
    {
        filename = c4::to_csubstr(filename_);

        // read the file
        c4::fs::file_get_contents(filename_, &file_contents);
        c4::csubstr contents = c4::to_csubstr(file_contents);

        // now parse the file
        c4::csubstr ws = " \t\r\n";
        c4::csubstr txt;
        size_t b, e;

        // desc
        RYML_CHECK(contents.begins_with("=== "));
        e = contents.find("--- from: ", 4);
        RYML_CHECK(e != npos);
        desc = contents.range(4, e).trimr(ws);

        // from
        b = e + 4;
        e = contents.find("--- tags: ", b);
        RYML_CHECK(e != npos);
        from = contents.range(b, e);
        RYML_CHECK(from.begins_with("from: "));
        RYML_CHECK(from.size() >= 6);
        from = from.sub(6).trimr(ws);

        // tags
        b = e + 4;
        e = contents.find("--- in-yaml", b);
        RYML_CHECK(e != npos);
        tags = contents.range(b, e);
        RYML_CHECK(tags.begins_with("tags: "));
        RYML_CHECK(tags.size() >= 6);
        tags = tags.sub(6).trimr(ws);

        if(tags.find("error") != npos)
        {
            c4::log("{}: test case tagged with error: {}:\n{}\n", filename, tags, contents);
            return false; // tagged with error. skip this test.
        }
        bool has_whitespace = tags.find("whitespace");

        size_t end_tags        = e;
        size_t begin_in_yaml   = contents.find("--- in-yaml"   , end_tags);
        size_t begin_in_json   = contents.find("--- in-json"   , end_tags);
        size_t begin_out_yaml  = contents.find("--- out-yaml"  , end_tags);
        size_t begin_emit_yaml = contents.find("--- emit-yaml" , end_tags);
        size_t begin_events    = contents.find("--- test-event", end_tags);
        std::initializer_list<size_t> all = {
            begin_in_yaml,
            begin_in_json,
            begin_out_yaml,
            begin_emit_yaml,
            begin_events,
            contents.size()
        };

        // some of the examples have their code indented
        std::string tmpa;
        std::string tmpb;

        // in_yaml
        RYML_CHECK(begin_in_yaml != npos);
        size_t first_after_in_yaml = find_first_after(begin_in_yaml, all);
        begin_in_yaml = 1 + contents.find('\n', begin_in_yaml); // skip this line
        txt = contents.range(begin_in_yaml, first_after_in_yaml);
        RYML_CHECK( ! txt.first_of_any("--- in-yaml", "--- in-json", "--- out-yaml", "--- emit-yaml", "---test-event"));
        txt = filter_out_indentation(txt, &tmpa);
        if(has_whitespace)
        {
            txt = replace_all("<SPC>", " ", txt, &tmpb);
            txt = replace_all("<TAB>", "\t", txt, &tmpa);
        }
        in_yaml.init(filename, txt, CPART_IN_YAML, allowed_failure);

        // in_json
        if(begin_in_json != npos)
        {
            size_t first_after = find_first_after(begin_in_json, all);
            begin_in_json = 1 + contents.find('\n', begin_in_json); // skip this line
            txt = contents.range(begin_in_json, first_after);
            RYML_CHECK( ! txt.first_of_any("--- in-yaml", "--- in-json", "--- out-yaml", "--- emit-yaml", "---test-event"));
            in_json.init(filename, txt, CPART_IN_JSON, allowed_failure);
        }

        // out_yaml
        if(begin_out_yaml != npos)
        {
            size_t first_after = find_first_after(begin_out_yaml, all);
            begin_out_yaml = 1 + contents.find('\n', begin_out_yaml); // skip this line
            txt = contents.range(begin_out_yaml, first_after);
            RYML_CHECK( ! txt.first_of_any("--- in-yaml", "--- in-json", "--- out-yaml", "--- emit-yaml", "---test-event"));
            txt = filter_out_indentation(txt, &tmpa);
            out_yaml.init(filename, txt, CPART_OUT_YAML, allowed_failure);
        }

        // emit_yaml
        if(begin_emit_yaml != npos)
        {
            size_t first_after = find_first_after(begin_emit_yaml, all);
            begin_emit_yaml = 1 + contents.find('\n', begin_emit_yaml); // skip this line
            txt = contents.range(begin_emit_yaml, first_after);
            RYML_CHECK( ! txt.first_of_any("--- in-yaml", "--- in-json", "--- out-yaml", "--- emit-yaml", "---test-event"));
            txt = filter_out_indentation(txt, &tmpa);
            emit_yaml.init(filename, txt, CPART_EMIT_YAML, allowed_failure);
        }

        // events
        RYML_CHECK(begin_events != npos);
        //size_t first_after_events = find_first_after(begin_events, all);
        begin_events = 1 + contents.find('\n', begin_events); // skip this line
        txt = contents.sub(begin_events);
        RYML_CHECK( ! txt.first_of_any("--- in-yaml", "--- in-json", "--- out-yaml", "--- emit-yaml", "---test-event"));
        events.init(filename, txt, CPART_EVENTS, allowed_failure);

        return true;
    }

    void print() const
    {
        c4::dump("~~~ file   : "   , filename      , "~~~\n",
                 "~~~ desc   : "   , desc          , "~~~\n",
                 "~~~ from   : "   , from          , "~~~\n",
                 "~~~ tags   : "   , tags          , "~~~\n",
                 "~~~ in-yaml:\n"  , src(in_yaml  ), "~~~\n",
                 "~~~ in-json:\n"  , src(in_json  ), "~~~\n",
                 "~~~ out-yaml:\n" , src(out_yaml ), "~~~\n",
                 "~~~ emit-yaml:\n", src(emit_yaml), "~~~\n",
                 "~~~ events :\n"  , src(events   ), "~~~\n");
    }

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


// a global holding the test case data
SuiteCase* g_suite_case = nullptr;


#define DECLARE_TEST_CLASS(cls, pfx)                            \
                                                                \
                                                                \
struct cls##_##pfx : public ::testing::TestWithParam<size_t>    \
{                                                               \
    Approach* get_test_case() const                             \
    {                                                           \
        RYML_CHECK(g_suite_case != nullptr);                    \
        RYML_CHECK(GetParam() < NLEVELS);                       \
        if(g_suite_case->cls.pfx.enabled)                       \
        {                                                       \
            return &g_suite_case->cls.pfx;                      \
        }                                                       \
        c4::dump(#cls "." #pfx ": no input for this case\n");   \
        return nullptr;                                         \
    }                                                           \
};                                                              \
                                                                \
                                                                \
TEST_P(cls##_##pfx, parse)                                      \
{                                                               \
    if(!get_test_case()) return;                                \
    get_test_case()->parse(1 + GetParam(), false);              \
}                                                               \
                                                                \
                                                                \
TEST_P(cls##_##pfx, compare_trees)                              \
{                                                               \
    if(!get_test_case()) return;                                \
    get_test_case()->compare_trees(1 + GetParam());             \
}                                                               \
                                                                \
                                                                \
TEST_P(cls##_##pfx, emit)                                       \
{                                                               \
    if(!get_test_case()) return;                                \
    get_test_case()->parse(1 + GetParam(), true);               \
}                                                               \
                                                                \
                                                                \
TEST_P(cls##_##pfx, compare_emitted)                            \
{                                                               \
    if(!get_test_case()) return;                                \
    get_test_case()->compare_emitted(1 + GetParam());           \
}                                                               \



#define DECLARE_TESTS(cls)                                              \
                                                                        \
DECLARE_TEST_CLASS(cls, unix_ro)                                        \
DECLARE_TEST_CLASS(cls, unix_rw)                                        \
DECLARE_TEST_CLASS(cls, unix_ro_reuse)                                  \
DECLARE_TEST_CLASS(cls, unix_rw_reuse)                                  \
DECLARE_TEST_CLASS(cls, windows_ro)                                     \
DECLARE_TEST_CLASS(cls, windows_rw)                                     \
DECLARE_TEST_CLASS(cls, windows_ro_reuse)                               \
DECLARE_TEST_CLASS(cls, windows_rw_reuse)                               \
                                                                        \
INSTANTIATE_TEST_SUITE_P(_, cls##_unix_ro      , testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_unix_rw      , testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_unix_ro_reuse, testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_unix_rw_reuse, testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_windows_ro      , testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_windows_rw      , testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_windows_ro_reuse, testing::Range<size_t>(0, NLEVELS)); \
INSTANTIATE_TEST_SUITE_P(_, cls##_windows_rw_reuse, testing::Range<size_t>(0, NLEVELS));


DECLARE_TESTS(out_yaml)
//DECLARE_TESTS(emit_yaml)
DECLARE_TESTS(in_json)
DECLARE_TESTS(in_yaml)


//-------------------------------------------
//-------------------------------------------
// this is needed to use the test case library
Case const* get_case(csubstr /*name*/)
{
    return nullptr;
}

} // namespace yml
} // namespace c4




//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, char* argv[])
{
    using namespace c4::yml;

    // make gtest parse its args
    ::testing::InitGoogleTest(&argc, argv);

    // now we have only our args to consider
    if(argc != 2)
    {
        c4::log("usage:\n{} <test-suite-file>", argv[0]);
        return 1;
    }

    // load the test case from the suite file
    auto path = c4::to_substr(argv[1]);
    path.replace('\\', '/');
    RYML_CHECK(path.len > 0);
    RYML_CHECK(path[0] != '-');
    RYML_CHECK(c4::fs::path_exists(path.str));
    c4::log("testing suite case: {} ({})", path.basename(), path);

    AllowedFailure allowed_to_fail = is_failure_expected(path);
    if(allowed_to_fail.skip())
    {
        c4::log("\n{}: this case is deliberately not implemented in rapidyaml: {}\n",
                allowed_to_fail.test_code, allowed_to_fail.reason);
        return 0;
    }

    SuiteCase suite_case;
    if( ! suite_case.load(path.str, allowed_to_fail))
    {
        // if an error occurs during loading, the test intentionally crashes,
        // so the load() above never returns. This is NOT the same as
        // a return of false. That means the test was tagged as error,
        // ie, that an error is expected while parsing the code in it,
        // and for now we skip those, and return success.
        c4::log("\n{}: this case is tagged as error, skipping\n", path);
        return 0;
    }
    c4::print(suite_case.file_contents);
    suite_case.print();
    g_suite_case = &suite_case;

    // run all tests!
    int status = RUN_ALL_TESTS();

    // a terminating message
    if(suite_case.filename.not_empty())
    {
        c4::log("\n{}: TESTS {}: {}\n",
            suite_case.filename.basename(),
            status == 0 ? "SUCCEEDED" : "FAILED",
            suite_case.filename);
    }

    return status;
}
