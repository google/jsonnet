#include "fast_float/fast_float.h"

#include <iostream>
#include <cassert>
#include <cmath>

template <typename T> char *to_string(T d, char *buffer) {
  auto written = std::snprintf(buffer, 128, "%.*e",
                               64, d);
  return buffer + written;
}

static fast_float::value128 g_lehmer64_state;

/**
 * D. H. Lehmer, Mathematical methods in large-scale computing units.
 * Proceedings of a Second Symposium on Large Scale Digital Calculating
 * Machinery;
 * Annals of the Computation Laboratory, Harvard Univ. 26 (1951), pp. 141-146.
 *
 * P L'Ecuyer,  Tables of linear congruential generators of different sizes and
 * good lattice structure. Mathematics of Computation of the American
 * Mathematical
 * Society 68.225 (1999): 249-260.
 */

static inline void lehmer64_seed(uint64_t seed) { 
  g_lehmer64_state.high = 0;
  g_lehmer64_state.low = seed; 
}

static inline uint64_t lehmer64() {
  fast_float::value128 v = fast_float::full_multiplication(g_lehmer64_state.low,UINT64_C(0xda942042e4dd58b5));
  v.high += g_lehmer64_state.high * UINT64_C(0xda942042e4dd58b5);
  g_lehmer64_state = v;
  return v.high;
}

size_t errors;

void random_values(size_t N) {
  char buffer[128];
  lehmer64_seed(N);
  for (size_t t = 0; t < N; t++) {
    if ((t % 1048576) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint64_t word = lehmer64();
    double v;
    memcpy(&v, &word, sizeof(v));
    {
      const char *string_end = to_string(v, buffer);
      double result_value;
      auto result = fast_float::from_chars(buffer, string_end, result_value);
      if (result.ec != std::errc()) {
        std::cerr << "parsing error ? " << buffer << std::endl;
        errors++;
        if (errors > 10) {
          abort();
        }
      }
      if (std::isnan(v)) {
        if (!std::isnan(result_value)) {
          std::cerr << "not nan" << buffer << std::endl;
          errors++;
          if (errors > 10) {
            abort();
          }
        }
      } else if(copysign(1,result_value) != copysign(1,v)) {
        std::cerr << buffer << std::endl;
        std::cerr << "I got " << std::hexfloat << result_value << " but I was expecting " << v
              << std::endl;
        abort();
      } else if (result_value != v) {
        std::cerr << "no match ? '" << buffer << "'" << std::endl;
        std::cout << "started with " << std::hexfloat << v << std::endl;
        std::cout << "got back " << std::hexfloat << result_value << std::endl; 
        std::cout << std::dec;
        errors++;
        if (errors > 10) {
          abort();
        }
      }
    }
  }
  std::cout << std::endl;
}

int main() {
  errors = 0;
  size_t N = size_t(1) << (sizeof(size_t) * 4); // shift: 32 for 64bit, 16 for 32bit
  random_values(N);
  if (errors == 0) {
    std::cout << std::endl;
    std::cout << "all ok" << std::endl;
    return EXIT_SUCCESS;
  }
  std::cerr << std::endl;
  std::cerr << "errors were encountered" << std::endl;
  return EXIT_FAILURE;
}
