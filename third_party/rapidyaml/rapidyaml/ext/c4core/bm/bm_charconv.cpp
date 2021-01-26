#include <string>
#include <sstream>
#include <c4/c4_push.hpp>
#include <c4/substr.hpp>
#include <c4/std/std.hpp>
#include <c4/charconv.hpp>
#include <c4/format.hpp>
#include <inttypes.h>
#include <stdio.h>
#include <algorithm>
#include <stdlib.h>
#include <vector>
#include <c4/ext/rng/rng.hpp>

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

#define C4BM_TEMPLATE(fn, ...) BENCHMARK_TEMPLATE(fn, __VA_ARGS__) _c4bm_stats


// benchmarks depending on c++17 features are disabled using the
// preprocessor.  google benchmark has state.SkipWithError() but it
// makes the program return a nonzero exit code when it finishes. So
// we resort to the preprocessor to conditionally disable these
// benchmarks
#if C4_CPP >= 17 && defined(__cpp_lib_to_chars)
#define C4BM_TEMPLATE_CPP17(fn, ...) BENCHMARK_TEMPLATE(fn, __VA_ARGS__) _c4bm_stats
#else
#define C4BM_TEMPLATE_CPP17(fn, ...) void shutup_extra_semicolon()
#endif

#ifdef __clang__
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wdouble-promotion"
#   pragma clang diagnostic ignored "-Wdeprecated"
#   pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdouble-promotion"
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#   pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <benchmark/benchmark.h>
#include <fast_float/fast_float.h>
#include <ryu/ryu.h>
#include <ryu/ryu_parse.h>
#if C4_CPP >= 17
#include <jkj/fp/from_chars/from_chars.h>
#include <charconv>
#endif


namespace bm = benchmark;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// utilities for use in the benchmarks below


// facilities to deuglify SFINAE
#define C4FOR(ty, condition)                            \
    template<class ty>                                  \
    typename std::enable_if<condition(ty), void>::type
#define C4FOR2(ty1, ty2, condition)                             \
    template<class ty1, class ty2>                              \
    typename std::enable_if<condition(ty1), void>::type

#define isint(ty) std::is_integral<ty>::value
#define isiint(ty) std::is_integral<ty>::value && !std::is_unsigned<ty>::value
#define isuint(ty) std::is_integral<ty>::value && std::is_unsigned<ty>::value
#define isreal(ty) std::is_floating_point<ty>::value
#define isfloat(ty) std::is_same<ty, float>::value
#define isdouble(ty) std::is_same<ty, double>::value


/** convenience wrapper to avoid boilerplate code */
template<class T>
void report(bm::State &st)
{
    st.SetBytesProcessed(static_cast<int64_t>(st.iterations() * sizeof(T)));
    st.SetItemsProcessed(static_cast<int64_t>(st.iterations()));
}


/** generate in place a random sequence of values: integral version*/
C4FOR(T, isint)
generate_n(T *begin, T *end)
{
    c4::rng::pcg rng;
    // do not use T in the distribution:
    //  N4659 29.6.1.1 [rand.req.genl]/1e requires one of short, int, long, long long, unsigned short, unsigned int, unsigned long, or unsigned long long
    std::uniform_int_distribution<uint32_t> idist;
    std::generate(begin, end, [&](){ return (T) idist(rng); });
}


/** generate in place a random sequence of values: real-number version*/
C4FOR(T, isreal)
generate_n(T *begin, T *end)
{
    c4::rng::pcg rng;
    // make sure we also have some integral numbers in the real sequence
    T *rstart = begin + (std::distance(begin, end) / 20); // 5% integral numbers
    std::uniform_int_distribution<uint32_t> idist;
    std::uniform_real_distribution<T> rdist;
    std::generate(begin , rstart, [&](){ return idist(rng); });
    std::generate(rstart,    end, [&](){ return rdist(rng); });
}


constexpr const size_t kNumValues = 16384;


/** a ring buffer with input values for xtoa benchmarks */
template<class T>
struct random_values
{
    std::vector<T> v;
    size_t curr;
    T next() { T f = v[curr]; curr = (curr + 1) % v.size(); return f; }

    random_values(size_t sz=kNumValues) : v(sz), curr(0)
    {
        generate_n<T>(&v.front(), &v.back());
    }
};


/** a ring buffer with input strings for atox benchmarks */
struct random_strings
{
    std::vector<std::string> v;
    size_t curr;
    c4::csubstr next() { c4::csubstr f = c4::to_csubstr(v[curr]); curr = (curr + 1) % v.size(); return f; }
    std::string const& next_s() { std::string const& f = v[curr]; curr = (curr + 1) % v.size(); return f; }

    random_strings(size_t sz=kNumValues) : v(sz), curr(0) {}

    template<class T>
    void init_as()
    {
        random_values<T> tmp(v.size());
        generate_n<T>(&tmp.v.front(), &tmp.v.back());
        for(size_t i = 0; i < v.size(); ++i)
        {
            c4::catrs(&v[i], tmp.v[i]);
        }
    }

    template<class T>
    void init_as_hex()
    {
        random_values<T> tmp(v.size());
        generate_n<T>(&tmp.v.front(), &tmp.v.back());
        for(size_t i = 0; i < v.size(); ++i)
        {
            c4::catrs(&v[i], c4::fmt::hex(tmp.v[i]));
        }
    }
    template<class T>
    void init_as_oct()
    {
        random_values<T> tmp(v.size());
        generate_n<T>(&tmp.v.front(), &tmp.v.back());
        for(size_t i = 0; i < v.size(); ++i)
        {
            c4::catrs(&v[i], c4::fmt::oct(tmp.v[i]));
        }
    }
    template<class T>
    void init_as_bin()
    {
        random_values<T> tmp(v.size());
        generate_n<T>(&tmp.v.front(), &tmp.v.back());
        for(size_t i = 0; i < v.size(); ++i)
        {
            c4::catrs(&v[i], c4::fmt::bin(tmp.v[i]));
        }
    }

};

template<class T>
random_strings mkstrings()
{
    random_strings rs;
    rs.init_as<T>();
    return rs;
}


/** a character buffer, easily convertible to c4::substr */
template<size_t Dim=128>
struct sbuf
{
    char buf_[Dim];
    c4::substr buf;
    sbuf() : buf_(), buf(buf_) {}
    inline operator c4::substr& () { return buf; }
    char* begin() { return buf.begin(); }
    char* end() { return buf.end(); }
};

using string_buffer = sbuf<>;


// some of the benchmarks do not need to be templates,
// but it helps in the naming scheme.

// xtoa means <X> to string
// atox means string to <X>


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

C4FOR(T, isint)
atox_c4_read_dec(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val;
    for(auto _ : st)
    {
        c4::read_dec(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_c4_read_hex(bm::State& st)
{
    random_strings strings;
    strings.init_as_hex<T>();
    T val;
    for(auto _ : st)
    {
        c4::read_hex(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_c4_read_oct(bm::State& st)
{
    random_strings strings;
    strings.init_as_oct<T>();
    T val;
    for(auto _ : st)
    {
        c4::read_oct(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_c4_read_bin(bm::State& st)
{
    random_strings strings;
    strings.init_as_bin<T>();
    T val;
    for(auto _ : st)
    {
        c4::read_bin(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isiint)
xtoa_c4_itoa(bm::State& st)
{
    string_buffer buf;
    T i = -10;
    for(auto _ : st)
    {
        c4::itoa(buf, ++i);
    }
    report<T>(st);
}

C4FOR(T, isuint)
xtoa_c4_utoa(bm::State& st)
{
    string_buffer buf;
    T i = 0;
    for(auto _ : st)
    {
        ++i;
        c4::utoa(buf, ++i);
    }
    report<T>(st);
}

C4FOR(T, isfloat)
xtoa_c4_ftoa(bm::State& st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        c4::ftoa(buf, values.next());
    }
    report<T>(st);
}

C4FOR(T, isdouble)
xtoa_c4_dtoa(bm::State& st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        c4::dtoa(buf, values.next());
    }
    report<T>(st);
}

template<class T>
void xtoa_c4_xtoa(bm::State& st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        c4::xtoa(buf, values.next());
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isiint)
atox_c4_atoi(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        c4::atoi(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isuint)
atox_c4_atou(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        c4::atou(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isfloat)
atox_c4_atof(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        c4::atof(strings.next(), &val);
    }
    report<T>(st);
}

C4FOR(T, isdouble)
atox_c4_atod(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        c4::atod(strings.next(), &val);
    }
    report<T>(st);
}

template<class T>
void atox_c4_atox(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        c4::atox(strings.next(), &val);
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isint)
atox_std_atoi(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        val = (T) std::atoi(strings.next().data());
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_std_atol(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        val = (T) std::atol(strings.next().data());
    }
    report<T>(st);
}

C4FOR(T, isreal)
atox_std_atof(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        val = (T) std::atof(strings.next().data());
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isint)
atox_std_strtol(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        auto s = strings.next();
        val = (T) std::strtol(s.begin(), nullptr, 10);
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_std_strtoll(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        auto s = strings.next();
        val = (T) std::strtoll(s.begin(), nullptr, 10);
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_std_strtoul(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        auto s = strings.next();
        val = (T) std::strtoul(s.begin(), nullptr, 10);
    }
    report<T>(st);
}

C4FOR(T, isint)
atox_std_strtoull(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        auto s = strings.next();
        val = (T) std::strtoull(s.begin(), nullptr, 10);
    }
    report<T>(st);
}

C4FOR(T, isreal)
atox_std_strtof(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        auto s = strings.next();
        val = (T) std::strtof(s.begin(), nullptr);
    }
    report<T>(st);
}

C4FOR(T, isreal)
atox_std_strtod(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        auto s = strings.next();
        val = (T) std::strtod(s.begin(), nullptr);
    }
    report<T>(st);
}

C4FOR(T, isreal)
atox_std_stof(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        val = std::stof(strings.next_s());
    }
    report<T>(st);
}

C4FOR(T, isreal)
atox_std_stod(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        val = std::stod(strings.next_s());
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isfloat)
atox_ryu_s2f(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::string const& s = strings.next_s();
        s2f_n(s.data(), (int) s.size(), &val);
    }
    report<T>(st);
}

C4FOR(T, isdouble)
atox_ryu_s2d(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::string const& s = strings.next_s();
        s2d_n(s.data(), (int) s.size(), &val);
    }
    report<T>(st);
}

C4FOR(T, isfloat)
xtoa_ryu_f2s(bm::State &st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        f2s_buffered_n(values.next(), buf.buf.str);
    }
    report<T>(st);
}

C4FOR(T, isdouble)
xtoa_ryu_d2s(bm::State &st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        d2s_buffered_n(values.next(), buf.buf.str);
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

// fp is still experimental and undocumented;
// some assertions are firing in debug builds
// so we make these benchmarks available only with NDEBUG
#if !defined(NDEBUG) || (C4_CPP < 17)
#define C4BM_FP_BENCHMARK(name, ...) void shutup_extra_semicolon()
#else
#define C4BM_FP_BENCHMARK(name, ...) C4BM_TEMPLATE(name, __VA_ARGS__)

C4FOR(T, isreal)
atox_fp_from_chars_limited(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::string const& s = strings.next_s();
        val = jkj::fp::from_chars_limited<T>(s.data(), s.data()+s.size()).to_float();
    }
    report<T>(st);
}

C4FOR(T, isreal)
atox_fp_from_chars_unlimited(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::string const& s = strings.next_s();
        val = jkj::fp::from_chars_unlimited<T>(s.data(), s.data()+s.size()).to_float();
    }
    report<T>(st);
}
#endif


//-----------------------------------------------------------------------------

template<class T>
void atox_fast_float(bm::State &st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::string const& s = strings.next_s();
        fast_float::from_chars(s.data(), s.data()+s.size(), val);
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

template<class T> struct fmtspec;
template<> struct fmtspec< uint8_t> { static const char w[], r[]; };
template<> struct fmtspec<  int8_t> { static const char w[], r[]; };
template<> struct fmtspec<uint16_t> { static const char w[], r[]; };
template<> struct fmtspec< int16_t> { static const char w[], r[]; };
template<> struct fmtspec<uint32_t> { static const char w[], r[]; };
template<> struct fmtspec< int32_t> { static const char w[], r[]; };
template<> struct fmtspec<uint64_t> { static const char w[], r[]; };
template<> struct fmtspec< int64_t> { static const char w[], r[]; };
template<> struct fmtspec< float  > { static const char w[], r[]; };
template<> struct fmtspec< double > { static const char w[], r[]; };

constexpr const char fmtspec< uint8_t>::w[] = "%" PRIu8 ;
constexpr const char fmtspec<  int8_t>::w[] = "%" PRIi8 ;
constexpr const char fmtspec<uint16_t>::w[] = "%" PRIu16;
constexpr const char fmtspec< int16_t>::w[] = "%" PRIi16;
constexpr const char fmtspec<uint32_t>::w[] = "%" PRIu32;
constexpr const char fmtspec< int32_t>::w[] = "%" PRIi32;
constexpr const char fmtspec<uint64_t>::w[] = "%" PRIu64;
constexpr const char fmtspec< int64_t>::w[] = "%" PRIi64;
constexpr const char fmtspec< float  >::w[] = "%g"      ;
constexpr const char fmtspec< double >::w[] = "%lg"     ;

constexpr const char fmtspec< uint8_t>::r[] = "%" SCNu8 ;
constexpr const char fmtspec<  int8_t>::r[] = "%" SCNi8 ;
constexpr const char fmtspec<uint16_t>::r[] = "%" SCNu16;
constexpr const char fmtspec< int16_t>::r[] = "%" SCNi16;
constexpr const char fmtspec<uint32_t>::r[] = "%" SCNu32;
constexpr const char fmtspec< int32_t>::r[] = "%" SCNi32;
constexpr const char fmtspec<uint64_t>::r[] = "%" SCNu64;
constexpr const char fmtspec< int64_t>::r[] = "%" SCNi64;
constexpr const char fmtspec< float  >::r[] = "%g"      ;
constexpr const char fmtspec< double >::r[] = "%lg"     ;

C4FOR(T, isint)
xtoa_sprintf(bm::State& st)
{
    string_buffer buf;
    T i = 0;
    for(auto _ : st)
    {
        ++i;
        ::snprintf(buf.buf.str, buf.buf.len, fmtspec<T>::w, i);
    }
    report<T>(st);
}

C4FOR(T, isreal)
xtoa_sprintf(bm::State& st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        ::snprintf(buf.buf.str, buf.buf.len, fmtspec<T>::w, values.next());
    }
    report<T>(st);
}

template<class T>
void atox_scanf(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        ::sscanf(strings.next().str, fmtspec<T>::r, &val);
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isint)
xtoa_sstream(bm::State& st)
{
    T i = 0;
    std::string out; C4_UNUSED(out);
    for(auto _ : st)
    {
        std::stringstream ss;
        ss << ++i;
        out = ss.str();
    }
    report<T>(st);
}

C4FOR(T, isreal)
xtoa_sstream(bm::State& st)
{
    random_values<T> values;
    std::string out; C4_UNUSED(out);
    for(auto _ : st)
    {
        std::stringstream ss;
        ss << values.next();
        out = ss.str();
    }
    report<T>(st);
}

C4FOR(T, isint)
xtoa_sstream_reuse(bm::State& st)
{
    std::stringstream ss;
    std::string out; C4_UNUSED(out);
    T i = 0;
    for(auto _ : st)
    {
        ss.clear();
        ss.str("");
        ss << ++i;
    }
    report<T>(st);
}

C4FOR(T, isreal)
xtoa_sstream_reuse(bm::State& st)
{
    random_values<T> values;
    std::stringstream ss;
    std::string out; C4_UNUSED(out);
    for(auto _ : st)
    {
        ss.clear();
        ss.str("");
        ss << values.next();
    }
    report<T>(st);
}

template<class T>
void atox_sstream(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::stringstream ss(strings.next_s());
        ss >> val;
    }
    report<T>(st);
}

template<class T>
void atox_sstream_reuse(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    std::stringstream ss;
    for(auto _ : st)
    {
        ss.clear();
        ss.str(strings.next_s());
        ss >> val;
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isint)
xtoa_std_to_string(bm::State& st)
{
    T i = 0;
    for(auto _ : st)
    {
        std::string out = std::to_string(++i);
        C4_UNUSED(out);
    }
    report<T>(st);
}

C4FOR(T, isreal)
xtoa_std_to_string(bm::State& st)
{
    random_values<T> values;
    for(auto _ : st)
    {
        std::string out = std::to_string(values.next());
        C4_UNUSED(out);
    }
    report<T>(st);
}


//-----------------------------------------------------------------------------

C4FOR(T, isint)
xtoa_c4_to_chars(bm::State& st)
{
    string_buffer buf;
    T i = 0;
    for(auto _ : st)
    {
        c4::to_chars(buf, ++i);
    }
    report<T>(st);
}

C4FOR(T, isreal)
xtoa_c4_to_chars(bm::State& st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        c4::to_chars(buf, values.next());
    }
    report<T>(st);
}

template<class T>
void atox_c4_from_chars(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val;
    for(auto _ : st)
    {
        c4::from_chars(strings.next(), &val);
    }
    report<T>(st);
}

#if (C4_CPP >= 17) && defined(__cpp_lib_to_chars)
C4FOR(T, isint)
xtoa_std_to_chars(bm::State& st)
{
    string_buffer buf;
    T i = 0;
    for(auto _ : st)
    {
        std::to_chars(buf.begin(), buf.end(), ++i);
    }
    report<T>(st);
}

C4FOR(T, isreal)
xtoa_std_to_chars(bm::State& st)
{
    string_buffer buf;
    random_values<T> values;
    for(auto _ : st)
    {
        std::to_chars(buf.begin(), buf.end(), values.next());
    }
    report<T>(st);
}

template<class T>
void atox_std_from_chars(bm::State& st)
{
    random_strings strings = mkstrings<T>();
    T val; C4_UNUSED(val);
    for(auto _ : st)
    {
        std::string const& s = strings.next_s();
        std::from_chars(s.data(), s.data()+s.size(), val);
    }
    report<T>(st);
}
#endif


//-----------------------------------------------------------------------------

C4BM_TEMPLATE(xtoa_c4_utoa,  uint8_t);
C4BM_TEMPLATE(xtoa_c4_xtoa,  uint8_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  uint8_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  uint8_t);
C4BM_TEMPLATE(xtoa_std_to_string,  uint8_t);
C4BM_TEMPLATE(xtoa_sprintf,  uint8_t);
C4BM_TEMPLATE(xtoa_sstream_reuse,  uint8_t);
C4BM_TEMPLATE(xtoa_sstream,  uint8_t);

C4BM_TEMPLATE(xtoa_c4_itoa,   int8_t);
C4BM_TEMPLATE(xtoa_c4_xtoa,   int8_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  int8_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  int8_t);
C4BM_TEMPLATE(xtoa_std_to_string,  int8_t);
C4BM_TEMPLATE(xtoa_sprintf,  int8_t);
C4BM_TEMPLATE(xtoa_sstream_reuse,   int8_t);
C4BM_TEMPLATE(xtoa_sstream,   int8_t);

C4BM_TEMPLATE(xtoa_c4_utoa, uint16_t);
C4BM_TEMPLATE(xtoa_c4_xtoa, uint16_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  uint16_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  uint16_t);
C4BM_TEMPLATE(xtoa_std_to_string,  uint16_t);
C4BM_TEMPLATE(xtoa_sprintf,  uint16_t);
C4BM_TEMPLATE(xtoa_sstream_reuse, uint16_t);
C4BM_TEMPLATE(xtoa_sstream, uint16_t);

C4BM_TEMPLATE(xtoa_c4_itoa,  int16_t);
C4BM_TEMPLATE(xtoa_c4_xtoa,  int16_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  int16_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  int16_t);
C4BM_TEMPLATE(xtoa_std_to_string,  int16_t);
C4BM_TEMPLATE(xtoa_sprintf,  int16_t);
C4BM_TEMPLATE(xtoa_sstream_reuse,  int16_t);
C4BM_TEMPLATE(xtoa_sstream,  int16_t);

C4BM_TEMPLATE(xtoa_c4_utoa, uint32_t);
C4BM_TEMPLATE(xtoa_c4_xtoa, uint32_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  uint32_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  uint32_t);
C4BM_TEMPLATE(xtoa_std_to_string,  uint32_t);
C4BM_TEMPLATE(xtoa_sprintf,  uint32_t);
C4BM_TEMPLATE(xtoa_sstream_reuse, uint32_t);
C4BM_TEMPLATE(xtoa_sstream, uint32_t);

C4BM_TEMPLATE(xtoa_c4_itoa,  int32_t);
C4BM_TEMPLATE(xtoa_c4_xtoa,  int32_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  int32_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  int32_t);
C4BM_TEMPLATE(xtoa_std_to_string,  int32_t);
C4BM_TEMPLATE(xtoa_sprintf,  int32_t);
C4BM_TEMPLATE(xtoa_sstream_reuse,  int32_t);
C4BM_TEMPLATE(xtoa_sstream,  int32_t);

C4BM_TEMPLATE(xtoa_c4_utoa, uint64_t);
C4BM_TEMPLATE(xtoa_c4_xtoa, uint64_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  uint64_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  uint64_t);
C4BM_TEMPLATE(xtoa_std_to_string,  uint64_t);
C4BM_TEMPLATE(xtoa_sprintf,  uint64_t);
C4BM_TEMPLATE(xtoa_sstream_reuse, uint64_t);
C4BM_TEMPLATE(xtoa_sstream, uint64_t);

C4BM_TEMPLATE(xtoa_c4_itoa,  int64_t);
C4BM_TEMPLATE(xtoa_c4_xtoa,  int64_t);
C4BM_TEMPLATE(xtoa_c4_to_chars,  int64_t);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  int64_t);
C4BM_TEMPLATE(xtoa_std_to_string,  int64_t);
C4BM_TEMPLATE(xtoa_sprintf,  int64_t);
C4BM_TEMPLATE(xtoa_sstream_reuse,  int64_t);
C4BM_TEMPLATE(xtoa_sstream,  int64_t);

C4BM_TEMPLATE(xtoa_c4_ftoa,  float);
C4BM_TEMPLATE(xtoa_c4_xtoa,  float);
C4BM_TEMPLATE(xtoa_c4_to_chars,  float);
C4BM_TEMPLATE(xtoa_ryu_f2s,  float);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  float);
C4BM_TEMPLATE(xtoa_std_to_string,  float);
C4BM_TEMPLATE(xtoa_sprintf,  float);
C4BM_TEMPLATE(xtoa_sstream_reuse,  float);
C4BM_TEMPLATE(xtoa_sstream,  float);

C4BM_TEMPLATE(xtoa_c4_dtoa,  double);
C4BM_TEMPLATE(xtoa_c4_xtoa,  double);
C4BM_TEMPLATE(xtoa_c4_to_chars,  double);
C4BM_TEMPLATE(xtoa_ryu_d2s,  double);
C4BM_TEMPLATE_CPP17(xtoa_std_to_chars,  double);
C4BM_TEMPLATE(xtoa_std_to_string,  double);
C4BM_TEMPLATE(xtoa_sprintf,  double);
C4BM_TEMPLATE(xtoa_sstream_reuse,  double);
C4BM_TEMPLATE(xtoa_sstream,  double);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

C4BM_TEMPLATE(atox_c4_read_dec, uint8_t);
C4BM_TEMPLATE(atox_c4_read_hex, uint8_t);
C4BM_TEMPLATE(atox_c4_read_oct, uint8_t);
C4BM_TEMPLATE(atox_c4_read_bin, uint8_t);
C4BM_TEMPLATE(atox_c4_atou,  uint8_t);
C4BM_TEMPLATE(atox_c4_atox,  uint8_t);
C4BM_TEMPLATE(atox_c4_from_chars, uint8_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, uint8_t);
C4BM_TEMPLATE(atox_std_atoi,   uint8_t);
C4BM_TEMPLATE(atox_std_strtoul,   uint8_t);
C4BM_TEMPLATE(atox_scanf,   uint8_t);
C4BM_TEMPLATE(atox_sstream,   uint8_t);
C4BM_TEMPLATE(atox_sstream_reuse,   uint8_t);

C4BM_TEMPLATE(atox_c4_read_dec, int8_t);
C4BM_TEMPLATE(atox_c4_read_hex, int8_t);
C4BM_TEMPLATE(atox_c4_read_oct, int8_t);
C4BM_TEMPLATE(atox_c4_read_bin, int8_t);
C4BM_TEMPLATE(atox_c4_atoi,   int8_t);
C4BM_TEMPLATE(atox_c4_atox,   int8_t);
C4BM_TEMPLATE(atox_c4_from_chars, int8_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, int8_t);
C4BM_TEMPLATE(atox_std_atoi,   int8_t);
C4BM_TEMPLATE(atox_std_strtol,   int8_t);
C4BM_TEMPLATE(atox_scanf,   int8_t);
C4BM_TEMPLATE(atox_sstream,   int8_t);
C4BM_TEMPLATE(atox_sstream_reuse,   int8_t);

C4BM_TEMPLATE(atox_c4_read_dec, uint16_t);
C4BM_TEMPLATE(atox_c4_read_hex, uint16_t);
C4BM_TEMPLATE(atox_c4_read_oct, uint16_t);
C4BM_TEMPLATE(atox_c4_read_bin, uint16_t);
C4BM_TEMPLATE(atox_c4_atou, uint16_t);
C4BM_TEMPLATE(atox_c4_atox, uint16_t);
C4BM_TEMPLATE(atox_c4_from_chars, uint16_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, uint16_t);
C4BM_TEMPLATE(atox_std_atoi,   uint16_t);
C4BM_TEMPLATE(atox_std_strtoul,   uint16_t);
C4BM_TEMPLATE(atox_scanf,   uint16_t);
C4BM_TEMPLATE(atox_sstream,   uint16_t);
C4BM_TEMPLATE(atox_sstream_reuse,   uint16_t);

C4BM_TEMPLATE(atox_c4_read_dec, int16_t);
C4BM_TEMPLATE(atox_c4_read_hex, int16_t);
C4BM_TEMPLATE(atox_c4_read_oct, int16_t);
C4BM_TEMPLATE(atox_c4_read_bin, int16_t);
C4BM_TEMPLATE(atox_c4_atoi,  int16_t);
C4BM_TEMPLATE(atox_c4_atox,  int16_t);
C4BM_TEMPLATE(atox_c4_from_chars, int16_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, int16_t);
C4BM_TEMPLATE(atox_std_atoi,   int16_t);
C4BM_TEMPLATE(atox_std_strtol,   int16_t);
C4BM_TEMPLATE(atox_scanf,   int16_t);
C4BM_TEMPLATE(atox_sstream,   int16_t);
C4BM_TEMPLATE(atox_sstream_reuse,   int16_t);

C4BM_TEMPLATE(atox_c4_read_dec, uint32_t);
C4BM_TEMPLATE(atox_c4_read_hex, uint32_t);
C4BM_TEMPLATE(atox_c4_read_oct, uint32_t);
C4BM_TEMPLATE(atox_c4_read_bin, uint32_t);
C4BM_TEMPLATE(atox_c4_atou, uint32_t);
C4BM_TEMPLATE(atox_c4_atox, uint32_t);
C4BM_TEMPLATE(atox_c4_from_chars, uint32_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, uint32_t);
C4BM_TEMPLATE(atox_std_atoi,   uint32_t);
C4BM_TEMPLATE(atox_std_strtoul,   uint32_t);
C4BM_TEMPLATE(atox_scanf,   uint32_t);
C4BM_TEMPLATE(atox_sstream,   uint32_t);
C4BM_TEMPLATE(atox_sstream_reuse,   uint32_t);

C4BM_TEMPLATE(atox_c4_read_dec, int32_t);
C4BM_TEMPLATE(atox_c4_read_hex, int32_t);
C4BM_TEMPLATE(atox_c4_read_oct, int32_t);
C4BM_TEMPLATE(atox_c4_read_bin, int32_t);
C4BM_TEMPLATE(atox_c4_atoi,  int32_t);
C4BM_TEMPLATE(atox_c4_atox,  int32_t);
C4BM_TEMPLATE(atox_c4_from_chars, int32_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, int32_t);
C4BM_TEMPLATE(atox_std_atoi,   int32_t);
C4BM_TEMPLATE(atox_std_strtol,   int32_t);
C4BM_TEMPLATE(atox_scanf,   int32_t);
C4BM_TEMPLATE(atox_sstream,   int32_t);
C4BM_TEMPLATE(atox_sstream_reuse,   int32_t);

C4BM_TEMPLATE(atox_c4_read_dec, uint64_t);
C4BM_TEMPLATE(atox_c4_read_hex, uint64_t);
C4BM_TEMPLATE(atox_c4_read_oct, uint64_t);
C4BM_TEMPLATE(atox_c4_read_bin, uint64_t);
C4BM_TEMPLATE(atox_c4_atou, uint64_t);
C4BM_TEMPLATE(atox_c4_atox, uint64_t);
C4BM_TEMPLATE(atox_c4_from_chars, uint64_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, uint64_t);
C4BM_TEMPLATE(atox_std_atol,   uint64_t);
C4BM_TEMPLATE(atox_std_strtoull,   uint64_t);
C4BM_TEMPLATE(atox_scanf,   uint64_t);
C4BM_TEMPLATE(atox_sstream,   uint64_t);
C4BM_TEMPLATE(atox_sstream_reuse,   uint64_t);

C4BM_TEMPLATE(atox_c4_read_dec, int64_t);
C4BM_TEMPLATE(atox_c4_read_hex, int64_t);
C4BM_TEMPLATE(atox_c4_read_oct, int64_t);
C4BM_TEMPLATE(atox_c4_read_bin, int64_t);
C4BM_TEMPLATE(atox_c4_atoi,  int64_t);
C4BM_TEMPLATE(atox_c4_atox,  int64_t);
C4BM_TEMPLATE(atox_c4_from_chars, int64_t);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, int64_t);
C4BM_TEMPLATE(atox_std_atol,   int64_t);
C4BM_TEMPLATE(atox_std_strtoll,   int64_t);
C4BM_TEMPLATE(atox_scanf,   int64_t);
C4BM_TEMPLATE(atox_sstream,   int64_t);
C4BM_TEMPLATE(atox_sstream_reuse,   int64_t);

C4BM_TEMPLATE(atox_c4_atof,  float);
C4BM_TEMPLATE(atox_c4_atox,  float);
C4BM_TEMPLATE(atox_c4_from_chars, float);
C4BM_TEMPLATE(atox_ryu_s2f,  float);
C4BM_TEMPLATE(atox_fast_float,  float);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, float);
C4BM_FP_BENCHMARK(atox_fp_from_chars_limited,  float);
C4BM_FP_BENCHMARK(atox_fp_from_chars_unlimited,  float);
C4BM_TEMPLATE(atox_std_atof,   float);
C4BM_TEMPLATE(atox_std_strtof,   float);
C4BM_TEMPLATE(atox_std_stof,   float);
C4BM_TEMPLATE(atox_scanf,   float);
C4BM_TEMPLATE(atox_sstream,   float);
C4BM_TEMPLATE(atox_sstream_reuse,   float);

C4BM_TEMPLATE(atox_c4_atod,  double);
C4BM_TEMPLATE(atox_c4_atox,  double);
C4BM_TEMPLATE(atox_c4_from_chars, double);
C4BM_TEMPLATE(atox_ryu_s2d,  double);
C4BM_TEMPLATE(atox_fast_float,  double);
C4BM_TEMPLATE_CPP17(atox_std_from_chars, double);
C4BM_FP_BENCHMARK(atox_fp_from_chars_limited,  double);
C4BM_FP_BENCHMARK(atox_fp_from_chars_unlimited,  double);
C4BM_TEMPLATE(atox_std_atof,   double);
C4BM_TEMPLATE(atox_std_strtod,   double);
C4BM_TEMPLATE(atox_std_stod,   double);
C4BM_TEMPLATE(atox_scanf,   double);
C4BM_TEMPLATE(atox_sstream,   double);
C4BM_TEMPLATE(atox_sstream_reuse,   double);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    bm::Initialize(&argc, argv);
    bm::RunSpecifiedBenchmarks();
    return 0;
}

#ifdef __clang__
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

#include <c4/c4_pop.hpp>
