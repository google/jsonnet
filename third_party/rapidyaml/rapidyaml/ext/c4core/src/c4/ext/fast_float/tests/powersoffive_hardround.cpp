#include "fast_float/fast_float.h"

#include <iostream>
#include <random>
#include <sstream>
#include <vector>


#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(sun) || defined(__sun)
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


std::pair<double, bool> strtod_from_string(const char *st) {
  double d;
  char *pr;
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)  || defined(sun) || defined(__sun)
    d = cygwin_strtod_l(st, &pr);
#elif defined(_WIN32)
  static _locale_t c_locale = _create_locale(LC_ALL, "C");
  d = _strtod_l(st, &pr, c_locale);
#else
  static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
  d = strtod_l(st, &pr, c_locale);
#endif
  if (st == pr) {
    std::cerr << "strtod_l could not parse '" << st << std::endl;
    return std::make_pair(0, false);
  }
  return std::make_pair(d, true);
}

std::pair<float, bool> strtof_from_string(char *st) {
  float d;
  char *pr;
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) || defined(sun) || defined(__sun)
  d = cygwin_strtof_l(st, &pr);
#elif defined(_WIN32)
  static _locale_t c_locale = _create_locale(LC_ALL, "C");
  d = _strtof_l(st, &pr, c_locale);
#else
  static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
  d = strtof_l(st, &pr, c_locale);
#endif
  if (st == pr) {
    std::cerr << "strtof_l could not parse '" << st << std::endl;
    return std::make_pair(0.0f, false);
  }
  return std::make_pair(d, true);
}

bool tester() {
  std::random_device rd;
  std::mt19937 gen(rd());
  for (int q = 18; q <= 27; q++) {
    std::cout << "q = " << -q << std::endl;
    uint64_t power5 = 1;
    for (int k = 0; k < q; k++) {
      power5 *= 5;
    }
    uint64_t low_threshold = 0x20000000000000 / power5 + 1;
    uint64_t threshold = 0xFFFFFFFFFFFFFFFF / power5;
    std::uniform_int_distribution<uint64_t> dis(low_threshold, threshold);
    for (size_t i = 0; i < 10000; i++) {
      uint64_t mantissa = dis(gen) * power5;
      std::stringstream ss;
      ss << mantissa;
      ss << "e";
      ss << -q;
      std::string to_be_parsed = ss.str();
      std::pair<double, bool> expected_double =
          strtod_from_string(to_be_parsed.c_str());
      double result_value;
      auto result =
          fast_float::from_chars(to_be_parsed.data(), to_be_parsed.data() + to_be_parsed.size(), result_value);
      if (result.ec != std::errc()) {
        std::cout << to_be_parsed << std::endl;
        std::cerr << " I could not parse " << std::endl;
        return false;
      }
      if (result_value != expected_double.first) {
        std::cout << to_be_parsed << std::endl;
        std::cerr << std::hexfloat << result_value << std::endl;
        std::cerr << std::hexfloat << expected_double.first << std::endl;
        std::cerr << " Mismatch " << std::endl;
        return false;
      }
    }
  }
  return true;
}

int main() {
  if (tester()) {
    std::cout << std::endl;
    std::cout << "all ok" << std::endl;
    return EXIT_SUCCESS;
  }
  std::cerr << std::endl;
  std::cerr << "errors were encountered" << std::endl;
  return EXIT_FAILURE;
}
