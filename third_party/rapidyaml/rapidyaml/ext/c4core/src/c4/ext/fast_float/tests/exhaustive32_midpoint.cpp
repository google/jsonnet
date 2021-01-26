#include "fast_float/fast_float.h"

#include <iostream>
#include <cassert>
#include <cmath>

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) 
// Anything at all that is related to cygwin, msys and so forth will
// always use this fallback because we cannot rely on it behaving as normal
// gcc.
#include <locale>
#include <sstream>
// workaround for CYGWIN
double cygwin_strtod_l(const char* start, char** end) {
    double d;
    std::stringstream ss;
    ss.imbue(std::locale::classic());
    ss << start;
    ss >> d;
    if(ss.fail()) { *end = nullptr; }
    if(ss.eof()) { ss.clear(); }
    auto nread = ss.tellg();
    *end = const_cast<char*>(start) + nread;
    return d;
}
float cygwin_strtof_l(const char* start, char** end) {
    float d;
    std::stringstream ss;
    ss.imbue(std::locale::classic());
    ss << start;
    ss >> d;
    if(ss.fail()) { *end = nullptr; }
    if(ss.eof()) { ss.clear(); }
    auto nread = ss.tellg();
    *end = const_cast<char*>(start) + nread;
    return d;
}
#endif

template <typename T> char *to_string(T d, char *buffer) {
  auto written = std::snprintf(buffer, 64, "%.*e",
                               std::numeric_limits<T>::max_digits10 - 1, d);
  return buffer + written;
}

void strtof_from_string(const char * st, float& d) {
    char *pr = (char *)st;
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)  || defined(sun) || defined(__sun)
    d = cygwin_strtof_l(st, &pr);
#elif defined(_WIN32)
    static _locale_t c_locale = _create_locale(LC_ALL, "C");
    d = _strtof_l(st, &pr,  c_locale);
#else
    static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
    d = strtof_l(st, &pr,  c_locale);
#endif
    if (pr == st) {
      throw std::runtime_error("bug in strtod_from_string");
    }
}

bool allvalues() {
  char buffer[64];
  for (uint64_t w = 0; w <= 0xFFFFFFFF; w++) {
    float v;
    if ((w % 1048576) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t word = uint32_t(w);
    memcpy(&v, &word, sizeof(v));
    if(std::isfinite(v)) { 
      float nextf = std::nextafterf(v, INFINITY);
      if(copysign(1,v) != copysign(1,nextf)) { continue; }
      if(!std::isfinite(nextf)) { continue; }
      double v1{v};
      assert(float(v1) == v);
      double v2{nextf};
      assert(float(v2) == nextf);
      double midv{v1 + (v2 - v1) / 2};
      float expected_midv = float(midv);

      const char *string_end = to_string(midv, buffer);
      float str_answer;
      strtof_from_string(buffer, str_answer);

      float result_value;
      auto result = fast_float::from_chars(buffer, string_end, result_value);
      if (result.ec != std::errc()) {
        std::cerr << "parsing error ? " << buffer << std::endl;
        return false;
      }
      if (std::isnan(v)) {
        if (!std::isnan(result_value)) {
          std::cerr << "not nan" << buffer << std::endl;
          std::cerr << "v " << std::hexfloat << v << std::endl;
          std::cerr << "v2 " << std::hexfloat << v2 << std::endl;
          std::cerr << "midv " << std::hexfloat << midv << std::endl;
          std::cerr << "expected_midv " << std::hexfloat << expected_midv << std::endl;
          return false;
        }
      } else if(copysign(1,result_value) != copysign(1,v)) {
        std::cerr << buffer << std::endl;
        std::cerr << "v " << std::hexfloat << v << std::endl;
        std::cerr << "v2 " << std::hexfloat << v2 << std::endl;
        std::cerr << "midv " << std::hexfloat << midv << std::endl;
        std::cerr << "expected_midv " << std::hexfloat << expected_midv << std::endl;
        std::cerr << "I got " << std::hexfloat << result_value << " but I was expecting " << v
              << std::endl;
        return false;
      } else if (result_value != str_answer) {
        std::cerr << "no match ? " << buffer << std::endl;
        std::cerr << "v " << std::hexfloat << v << std::endl;
        std::cerr << "v2 " << std::hexfloat << v2 << std::endl;
        std::cerr << "midv " << std::hexfloat << midv << std::endl;
        std::cerr << "expected_midv " << std::hexfloat << expected_midv << std::endl;
        std::cout << "started with " << std::hexfloat << midv << std::endl;
        std::cout << "round down to " << std::hexfloat << str_answer << std::endl;
        std::cout << "got back " << std::hexfloat << result_value << std::endl; 
        std::cout << std::dec;
        return false;
      }
    }
  }
  std::cout << std::endl;
  return true;
}

inline void Assert(bool Assertion) {
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)  || defined(sun) || defined(__sun)
  if (!Assertion) { std::cerr << "Omitting hard falure on msys/cygwin/sun systems."; }
#else 
  if (!Assertion) { throw std::runtime_error("bug"); }
#endif
}
int main() {
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(sun) || defined(__sun)
  std::cout << "Warning: msys/cygwin or solaris detected. This particular test is likely to generate false failures due to our reliance on the underlying runtime library as a gold standard." << std::endl;
#endif
  Assert(allvalues());
  std::cout << std::endl;
  std::cout << "all ok" << std::endl;
  return EXIT_SUCCESS;
}
