cmake_minimum_required(VERSION 3.15...4.1)

project(jsonnet_cmake_example VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Can use add_subdirectory to point to the jsonnet source.
# Disable tests (force BUILD_TEST OFF) so that jsonnet doesn't pull in GoogleTest.
# Use EXCLUDE_FROM_ALL so that targets won't be built if nothing depends on them.
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}/jsonnet
    EXCLUDE_FROM_ALL
)

add_executable(jsonnet_cmake_example main.cpp)
target_link_libraries(jsonnet_cmake_example PRIVATE jsonnet_cpp_static)
