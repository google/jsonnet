cmake_minimum_required(VERSION 3.9)

project(fast_float VERSION 0.7.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



option(FASTFLOAT_SANITIZE "Sanitize addresses" OFF)

if (NOT CMAKE_BUILD_TYPE)
  if(FASTFLOAT_SANITIZE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)
  else()
    message(STATUS "No build type selected, default to Release")
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  endif()
endif()


add_library(fast_float INTERFACE)
target_include_directories(fast_float INTERFACE include/)
if(FASTFLOAT_SANITIZE)
  target_compile_options(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(fast_float INTERFACE -fuse-ld=gold)
  endif()
endif()

if(FASTFLOAT_TEST)
  enable_testing()
  add_subdirectory(tests)
endif(FASTFLOAT_TEST)
