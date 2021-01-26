cmake_minimum_required(VERSION 3.12)
project(ryml-consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../../ foobar-ryml)

include(./check.cmake)  # run some checks (used in CI tests)

add_executable(ryml-consumer main.cpp)
target_link_libraries(ryml-consumer ryml)
add_custom_command(TARGET ryml-consumer POST_BUILD
    COMMAND $<TARGET_FILE:ryml-consumer>
    COMMENT "running test: $<TARGET_FILE:ryml-consumer>")
