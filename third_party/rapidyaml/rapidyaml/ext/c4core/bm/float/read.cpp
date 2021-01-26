#include <cstdio>


#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4430)
#   pragma warning(disable : 4305)
#   pragma warning(disable : 4309)
#   pragma warning(disable : 4838)
#   pragma warning(disable : 4996)
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wsign-conversion"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-conversion"
#endif


#if C4FLOAT_STD_ATOF
#include <cstdlib>
double doit(const char *s) { return atof(s); }
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_SSCANF_F
float doit(const char *s) { float val; sscanf(s, "%f", &val); return val; }
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_SSCANF_D
double doit(const char *s) { double val; sscanf(s, "%lf", &val); return val; }
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_IOSTREAM_F
#include <sstream>
float doit(const char *s) { std::stringstream ss; ss << s; float val; ss >> val; return val; }
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_IOSTREAM_D
#include <sstream>
double doit(const char *s) { std::stringstream ss; ss << s; double val; ss >> val; return val; }
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_IOSTREAM_D
#include <sstream>
double doit(const char *s) { std::stringstream ss; ss << s; double val; ss >> val; return val; }
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_FP_F_LIMITED
#include <jkj/fp/from_chars/from_chars.h>
float doit(const char *s)
{
    auto result = jkj::fp::from_chars_limited<float>(s, s+strlen(s));
    return result.to_float();
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_FP_D_LIMITED
#include <jkj/fp/from_chars/from_chars.h>
double doit(const char *s)
{
    auto result = jkj::fp::from_chars_limited<double>(s, s+strlen(s));
    return result.to_float();
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_FP_F_UNLIMITED
#include <jkj/fp/from_chars/from_chars.h>
float doit(const char *s)
{
    auto result = jkj::fp::from_chars_unlimited<float>(s, s+strlen(s));
    return result.to_float();
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_FP_D_UNLIMITED
#include <jkj/fp/from_chars/from_chars.h>
double doit(const char *s)
{
    auto result = jkj::fp::from_chars_unlimited<double>(s, s+strlen(s));
    return result.to_float();
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_FASTFLOAT_F
#include <c4/ext/fast_float.hpp>
#include <cstring>
float doit(const char *s)
{
    float result;
    fast_float::from_chars(s, s+strlen(s), result);
    return result;
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_FASTFLOAT_D
#include <c4/ext/fast_float.hpp>
#include <cstring>
double doit(const char *s)
{
    double result;
    fast_float::from_chars(s, s+strlen(s), result);
    return result;
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_STD_FROM_CHARS_F
#include <charconv>
#include <cstring>
float doit(const char *s)
{
    float result;
    std::from_chars(s, s+strlen(s), result);
    return result;
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_STD_FROM_CHARS_D
#include <charconv>
#include <cstring>
double doit(const char *s)
{
    double result;
    std::from_chars(s, s+strlen(s), result);
    return result;
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_RYU_F
#include <ryu/ryu_parse.h>
float doit(const char *s)
{
    float result;
    s2f(s, &result);
    return result;
}
#define C4_TO_REAL(s) doit(s)

#elif C4FLOAT_RYU_D
#include <ryu/ryu_parse.h>
double doit(const char *s)
{
    double result;
    s2d(s, &result);
    return result;
}
#define C4_TO_REAL(s) doit(s)

#else
#define C4_TO_REAL(s) 0
#endif

int main()
{
    #define BUFSIZE 128
    char buf[BUFSIZE];
    while(fgets(buf, BUFSIZE, stdin))
    {
        fputs(buf, stdout);
        (void) C4_TO_REAL(buf);
    }
}


#ifdef _MSC_VER
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
