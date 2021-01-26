
#include "fast_float/fast_float.h"

#include <iostream>
#include <cassert>
#include <cmath>

template <typename T> char *to_string(T d, char *buffer) {
  auto written = std::snprintf(buffer, 64, "%.*e",
                               std::numeric_limits<T>::max_digits10 - 1, d);
  return buffer + written;
}


bool basic_test_64bit(std::string vals, double val) {
  double result_value;
  auto result = fast_float::from_chars(vals.data(), vals.data() + vals.size(),
                                      result_value);
  if (result.ec != std::errc()) {
    std::cerr << " I could not parse " << vals << std::endl;
    return false;
  }
  if (std::isnan(val)) {
    if (!std::isnan(result_value)) {
      std::cerr << vals << std::endl;
      std::cerr << "not nan" << result_value << std::endl;
      return false;
    } 
  } else if(copysign(1,result_value) != copysign(1,val)) {
    std::cerr << "I got " << std::hexfloat << result_value << " but I was expecting " << val
              << std::endl;
    return false; 
  } else if (result_value != val) {
    std::cerr << vals << std::endl;
    std::cerr << "I got " << std::hexfloat << result_value << " but I was expecting " << val
              << std::endl;
    std::cerr << std::dec;
    std::cerr << "string: " << vals << std::endl;
    return false;
  }
  return true;
}


void all_32bit_values() {
  char buffer[64];
  for (uint64_t w = 0; w <= 0xFFFFFFFF; w++) {
    float v32;
    if ((w % 1048576) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint32_t word = uint32_t(w);
    memcpy(&v32, &word, sizeof(v32));
    double v = v32;

    {
      const char *string_end = to_string(v, buffer);
      std::string s(buffer, size_t(string_end-buffer));
      if(!basic_test_64bit(s,v)) {
        return;
      }
    }
  }
  std::cout << std::endl;
}

int main() {
  all_32bit_values();
  std::cout << std::endl;
  std::cout << "all ok" << std::endl;
  return EXIT_SUCCESS;
}
