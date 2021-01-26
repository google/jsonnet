#include <string>
#include <c4/c4_push.hpp>
#include <c4/std/std.hpp>
#include <c4/format.hpp>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <benchmark/benchmark.h>

namespace bm = benchmark;

double getmax(std::vector<double> const& v)
{
    return *(std::max_element(std::begin(v), std::end(v)));
}
double getmin(std::vector<double> const& v)
{
    return *(std::min_element(std::begin(v), std::end(v)));
}
double getrange(std::vector<double> const& v)
{
    auto min_max = std::minmax_element(std::begin(v), std::end(v));
    return *min_max.second - *min_max.first;
}

#define _c4bm_stats                                                     \
    /*->Repetitions(20)*/                                               \
    ->DisplayAggregatesOnly(true)                                       \
    ->ComputeStatistics("range", &getrange)                             \
    ->ComputeStatistics("max", &getmax)                                 \
    ->ComputeStatistics("min", &getmin)

#define C4BM(fn) BENCHMARK(fn) _c4bm_stats

/** convenience wrapper to avoid boilerplate code */
void report(bm::State &st, size_t sz)
{
    st.SetBytesProcessed(static_cast<int64_t>(st.iterations() * sz));
    st.SetItemsProcessed(static_cast<int64_t>(st.iterations()));
}


#define _c4argbundle_fmt "hello here you have some numbers: "\
    "1={}, 2={}, 3={}, 4={}, 5={}, 6={}, 7={}, 8={}, 9={}, size_t(283482349)={}, "\
    "\" \"=\"{}\", \"haha\"=\"{}\", std::string(\"hehe\")=\"{}\", "\
    "str=\"{}\""

#define _c4argbundle_fmt_printf "hello here you have some numbers: "\
    "1=%d, 2=%d, 3=%d, 4=%d, 5=%d, 6=%d, 7=%d, 8=%d, 9=%d, size_t(283482349)=%zu, "\
    "\" \"=\"%s\", \"haha\"=\"%s\", std::string(\"hehe\")=\"%s\", "\
    "str=\"%s\""

#define _c4argbundle \
    1, 2, 3, 4, 5, 6, 7, 8, 9, size_t(283482349),\
    " ", "haha", std::string("hehe"),\
    std::string("asdlklkasdlkjasd asdlkjasdlkjasdlkjasdoiasdlkjasldkj")

#define _c4argbundle_printf \
    1, 2, 3, 4, 5, 6, 7, 8, 9, size_t(283482349),\
    " ", "haha", std::string("hehe").c_str(),\
    std::string("asdlklkasdlkjasd asdlkjasdlkjasdlkjasdoiasdlkjasldkj").c_str()

#define _c4argbundle_lshift \
    1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9 << size_t(283482349)\
      << " " << "haha" << std::string("hehe")\
      << std::string("asdlklkasdlkjasd asdlkjasdlkjasdlkjasdoiasdlkjasldkj")


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void cat_c4cat_substr(bm::State &st)
{
    char buf_[256];
    c4::substr buf(buf_);
    size_t sz = 0;
    for(auto _ : st)
    {
        sz = cat(buf, _c4argbundle);
    }
    report(st, sz);
}

void catsep_c4cat_substr(bm::State &st)
{
    char buf_[256];
    c4::substr buf(buf_);
    size_t sz = 0;
    for(auto _ : st)
    {
        sz = catsep(buf, _c4argbundle);
    }
    report(st, sz);
}

void cat_c4catrs_reuse(bm::State &st)
{
    std::string buf;
    size_t sz = 0;
    for(auto _ : st)
    {
        c4::catrs(&buf, _c4argbundle);
        sz = buf.size();
    }
    report(st, sz);
}

void catsep_c4catrs_reuse(bm::State &st)
{
    std::string buf;
    size_t sz = 0;
    for(auto _ : st)
    {
        c4::catseprs(&buf, _c4argbundle);
        sz = buf.size();
    }
    report(st, sz);
}

void cat_c4catrs_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        auto buf = c4::catrs<std::string>(_c4argbundle);
        sz = buf.size();
    }
    report(st, sz);
}

void catsep_c4catrs_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        auto buf = c4::catseprs<std::string>(_c4argbundle);
        sz = buf.size();
    }
    report(st, sz);
}

//-----------------------------------------------------------------------------
void cat_std_stringstream_impl(std::stringstream &)
{
}
void catsep_std_stringstream_impl(std::stringstream &)
{
}

template<class Arg, class... Args>
void cat_std_stringstream_impl(std::stringstream &ss, Arg const& a, Args const& ...args)
{
    ss << a;
    cat_std_stringstream_impl(ss, args...);
}

template<class Arg, class... Args>
void catsep_std_stringstream_impl(std::stringstream &ss, Arg const& a, Args const& ...args)
{
    ss << ' ' << a;
    cat_std_stringstream_impl(ss, args...);
}

void cat_stdsstream_reuse(bm::State &st)
{
    size_t sz = 0;
    std::stringstream ss;
    for(auto _ : st)
    {
        ss.clear();
        ss.str("");
        cat_std_stringstream_impl(ss, _c4argbundle);
        sz = ss.str().size();
    }
    report(st, sz);
}

void catsep_stdsstream_reuse(bm::State &st)
{
    size_t sz = 0;
    std::stringstream ss;
    for(auto _ : st)
    {
        ss.clear();
        ss.str("");
        catsep_std_stringstream_impl(ss, _c4argbundle);
        sz = ss.str().size();
    }
    report(st, sz);
}

void cat_stdsstream_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        std::stringstream ss;
        cat_std_stringstream_impl(ss, _c4argbundle);
        sz = ss.str().size();
    }
    report(st, sz);
}

void catsep_stdsstream_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        std::stringstream ss;
        catsep_std_stringstream_impl(ss, _c4argbundle);
        sz = ss.str().size();
    }
    report(st, sz);
}


//-----------------------------------------------------------------------------

template<class T>
C4_ALWAYS_INLINE typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
std_to_string(T const& a)
{
    return std::to_string(a);
}

template<class T>
C4_ALWAYS_INLINE typename std::enable_if<std::is_same<T, std::string>::value, std::string const&>::type
std_to_string(std::string const& a)
{
    return a;
}

template<class T>
C4_ALWAYS_INLINE typename std::enable_if< ! std::is_arithmetic<T>::value, std::string>::type
std_to_string(T const& a)
{
    return std::string(a);
}

C4_ALWAYS_INLINE void cat_std_string_impl(std::string *)
{
}

C4_ALWAYS_INLINE void catsep_std_string_impl(std::string *)
{
}

template<class Arg, class... Args>
void cat_std_string_impl(std::string *s, Arg const& a, Args const& ...args)
{
    *s += std_to_string(a);
    cat_std_string_impl(s, args...);
}

template<class Arg, class... Args>
void catsep_std_string_impl(std::string *s, Arg const& a, Args const& ...args)
{
    *s += ' ';
    *s += std_to_string(a);
    cat_std_string_impl(s, args...);
}

void cat_std_to_string_reuse(bm::State &st)
{
    size_t sz = 0;
    std::string s;
    for(auto _ : st)
    {
        s.clear();
        cat_std_string_impl(&s, _c4argbundle);
        sz = s.size();
    }
    report(st, sz);
}

void catsep_std_to_string_reuse(bm::State &st)
{
    size_t sz = 0;
    std::string s;
    for(auto _ : st)
    {
        s.clear();
        catsep_std_string_impl(&s, _c4argbundle);
        sz = s.size();
    }
    report(st, sz);
}

void cat_std_to_string_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        std::string s;
        cat_std_string_impl(&s, _c4argbundle);
        sz = s.size();
    }
    report(st, sz);
}

void catsep_std_to_string_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        std::string s;
        catsep_std_string_impl(&s, _c4argbundle);
        sz = s.size();
    }
    report(st, sz);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void format_c4_format(bm::State &st)
{
    char buf_[512];
    c4::substr buf(buf_);
    size_t sz = 0;
    for(auto _ : st)
    {
        sz = format(buf, _c4argbundle_fmt, _c4argbundle);
    }
    report(st, sz);
}

void format_c4_formatrs_reuse(bm::State &st)
{
    std::string buf;
    size_t sz = 0;
    for(auto _ : st)
    {
        c4::formatrs(&buf, _c4argbundle_fmt, _c4argbundle);
        sz = buf.size();
    }
    report(st, sz);
}

void format_c4_formatrs_no_reuse(bm::State &st)
{
    size_t sz = 0;
    for(auto _ : st)
    {
        auto buf = c4::formatrs<std::string>(_c4argbundle_fmt, _c4argbundle);
        sz = buf.size();
    }
    report(st, sz);
}

void format_snprintf(bm::State &st)
{
    char buf_[512];
    c4::substr buf(buf_);
    size_t sz = 0;
    for(auto _ : st)
    {
        sz = (size_t) snprintf(buf.str, buf.len, _c4argbundle_fmt_printf, _c4argbundle_printf);
    }
    report(st, sz);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

C4BM(cat_c4cat_substr);
C4BM(cat_c4catrs_reuse);
C4BM(cat_c4catrs_no_reuse);
C4BM(cat_std_to_string_reuse);
C4BM(cat_std_to_string_no_reuse);
C4BM(cat_stdsstream_reuse);
C4BM(cat_stdsstream_no_reuse);


C4BM(catsep_c4cat_substr);
C4BM(catsep_c4catrs_reuse);
C4BM(catsep_c4catrs_no_reuse);
C4BM(catsep_std_to_string_reuse);
C4BM(catsep_std_to_string_no_reuse);
C4BM(catsep_stdsstream_reuse);
C4BM(catsep_stdsstream_no_reuse);


C4BM(format_c4_format);
C4BM(format_c4_formatrs_reuse);
C4BM(format_c4_formatrs_no_reuse);
C4BM(format_snprintf);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    bm::Initialize(&argc, argv);
    bm::RunSpecifiedBenchmarks();
    return 0;
}


#include <c4/c4_pop.hpp>


