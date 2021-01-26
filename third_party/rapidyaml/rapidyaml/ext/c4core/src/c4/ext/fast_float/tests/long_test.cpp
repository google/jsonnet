#include "fast_float/fast_float.h"

#include <vector>

inline void Assert(bool Assertion) {
  if (!Assertion) { throw std::runtime_error("bug"); }
}

template <typename T>
bool test() {
  std::string input = "0.156250000000000000000000000000000000000000  3.14159265358979323846264338327950288419716939937510 2.71828182845904523536028747135266249775724709369995";
  std::vector<T> answers = {T(0.15625), T(3.141592653589793), T(2.718281828459045)};
  const char * begin = input.data();
  const char * end = input.data() + input.size();
  for(size_t i = 0; i < answers.size(); i++) {
    T result_value;
    auto result = fast_float::from_chars(begin, end,
                                      result_value);
    if (result.ec != std::errc()) {
      printf("parsing %.*s\n", int(end - begin), begin);
      std::cerr << " I could not parse " << std::endl;
      return false;
    }
    if(result_value != answers[i]) {
      printf("parsing %.*s\n", int(end - begin), begin);
      std::cerr << " Mismatch " << std::endl;
      std::cerr << " Expected " << answers[i] << std::endl;
      std::cerr << " Got      " << result_value << std::endl;

      return false;

    }
    begin = result.ptr;
  }
  if(begin != end) {
      std::cerr << " bad ending " << std::endl;
      return false;    
  }
  return true;
}

int main() {

  std::cout << "32 bits checks" << std::endl;
  Assert(test<float>());

  std::cout << "64 bits checks" << std::endl;
  Assert(test<double>());

  std::cout << "All ok" << std::endl;
  return EXIT_SUCCESS;
}
