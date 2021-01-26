#pragma once
#ifndef C4_RYML_TEST_GROUP_HPP_
#define C4_RYML_TEST_GROUP_HPP_

#include "./test_case.hpp"
#include "c4/span.hpp"

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4068/*unknown pragma*/)
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunknown-pragmas"
//#   pragma GCC diagnostic ignored "-Wpragma-system-header-outside-header"
#endif


namespace c4 {
namespace yml {

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// a fixture for running the tests
struct YmlTestCase : public ::testing::TestWithParam<const char*>
{
    csubstr const name;
    Case const* c;
    CaseData * d;

    YmlTestCase() : name(to_csubstr(GetParam()))
    {
        c = get_case(name);
        d = get_data(name);
    }

    void SetUp() override
    {
        // Code here will be called immediately after the constructor (right
        // before each test).
        std::cout << "-------------------------------------------\n";
        std::cout << "running test case '" << name << "'\n";
        std::cout << "-------------------------------------------\n";
    }

    void _test_parse_using_ryml(CaseDataLineEndings *cd);
    void _test_emit_yml_stdout(CaseDataLineEndings *cd);
    void _test_emit_yml_cout(CaseDataLineEndings *cd);
    void _test_emit_yml_stringstream(CaseDataLineEndings *cd);
    void _test_emit_yml_ofstream(CaseDataLineEndings *cd);
    void _test_emit_yml_string(CaseDataLineEndings *cd);
    void _test_emitrs(CaseDataLineEndings *cd);
    void _test_emitrs_cfile(CaseDataLineEndings *cd);
    void _test_complete_round_trip(CaseDataLineEndings *cd);
    void _test_recreate_from_ref(CaseDataLineEndings *cd);
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// facilities for declaring test data

using N = CaseNode;
using L = CaseNode::iseqmap;
using TS = TaggedScalar;
using TL = CaseNode::TaggedList;
using AR = AnchorRef;
using PT = std::pair<const csubstr, Case>;
#define C(name, ...)                                        \
    PT                                                      \
    (                                                       \
        std::piecewise_construct_t{} C4_COMMA               \
        std::forward_as_tuple(name) C4_COMMA                \
        std::forward_as_tuple(name C4_COMMA __VA_ARGS__)    \
    )


#define ADD_CASE_GROUP(name) \
    {\
        c4::cspan<PT> li = get_cases_##name();\
        cases.insert(li.begin(), li.end());\
    }


#define CASE_GROUP(name) \
    c4::cspan<PT> get_cases_##name()


#define APPEND_CASES(...) \
    static const PT arr[] = {__VA_ARGS__}; \
    c4::cspan<PT> buf(arr);                \
    return buf;


#define INSTANTIATE_GROUP(group_name)                                   \
                                                                        \
INSTANTIATE_TEST_SUITE_P(group_name, YmlTestCase, ::testing::Values(group_name##_CASES)); \
                                                                        \
Case const* get_case(csubstr name)                                      \
{                                                                       \
    static std::map<csubstr, Case> cases;                               \
    if(cases.empty())                                                   \
    {                                                                   \
        ADD_CASE_GROUP(group_name);                                     \
    }                                                                   \
    auto it = cases.find(name);                                         \
    C4_ASSERT(it != cases.end());                                       \
    return &it->second;                                                 \
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

#endif // C4_RYML_TEST_GROUP_HPP_
