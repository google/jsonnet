# FetchContent requires cmake >=3.11
# see https://cmake.org/cmake/help/v3.11/module/FetchContent.html
cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

include(FetchContent)

FetchContent_Declare(doctest
  GIT_REPOSITORY https://github.com/onqtam/doctest.git
  GIT_TAG 2.4.1)

# FetchContent_MakeAvailable() was only introduced in 3.14
# https://cmake.org/cmake/help/v3.14/release/3.14.html#modules
# FetchContent_MakeAvailable(doctest)
FetchContent_GetProperties(doctest)
if(NOT doctest_POPULATED)
  FetchContent_Populate(doctest)
  add_subdirectory(${doctest_SOURCE_DIR} ${doctest_BINARY_DIR})
endif()


function(fast_float_add_cpp_test TEST_NAME)
    add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
    add_test(${TEST_NAME} ${TEST_NAME})
    if(NOT WIN32)
      target_compile_options(${TEST_NAME} PUBLIC -Werror -Wall -Wextra -Weffc++)
      target_compile_options(${TEST_NAME} PUBLIC -Wsign-compare -Wshadow -Wwrite-strings -Wpointer-arith -Winit-self -Wconversion -Wsign-conversion)
    endif()
    target_link_libraries(${TEST_NAME} PUBLIC fast_float doctest)
endfunction(fast_float_add_cpp_test)
fast_float_add_cpp_test(powersoffive_hardround)
fast_float_add_cpp_test(short_random_string)
fast_float_add_cpp_test(exhaustive32_midpoint)
fast_float_add_cpp_test(random_string)
fast_float_add_cpp_test(string_test)
fast_float_add_cpp_test(exhaustive32)
fast_float_add_cpp_test(exhaustive32_64)
fast_float_add_cpp_test(long_exhaustive32)
fast_float_add_cpp_test(long_exhaustive32_64)
fast_float_add_cpp_test(long_random64)
fast_float_add_cpp_test(random64)
fast_float_add_cpp_test(basictest)
fast_float_add_cpp_test(example_test)
