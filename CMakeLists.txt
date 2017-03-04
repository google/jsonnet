# Travis CI runs CMake 2.8.7 so we are pinned to that verison.
cmake_minimum_required(VERSION 2.8.7)
include(ExternalProject)

# User-configurable options.
option(BUILD_JSONNET "Build jsonnet command-line tool." ON)
option(BUILD_TESTS "Build and run jsonnet tests." ON)
set(GLOBAL_OUTPUT_PATH_SUFFIX "bin" CACHE STRING
    "Output artifacts directory.")

project(jsonnet)

# Discourage in-source builds because they overwrite the hand-written Makefile.
# Use `cmake . -B<dir>` or the CMake GUI to do an out-of-source build.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR} AND
        ${CMAKE_GENERATOR} MATCHES "Makefile")
    message(WARNING "In-source builds with the a makefile generator overwrite the handwritten Makefile. Out-of-source builds are recommended for this project.")
endif()


# Disable CMake >3.0 warnings.
set(CMAKE_MACOSX_RPATH 1)  

# Set output paths.
set(GLOBAL_OUTPUT_PATH "${PROJECT_BINARY_DIR}/${GLOBAL_OUTPUT_PATH_SUFFIX}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${GLOBAL_OUTPUT_PATH})

# Include external googletest project.
if (BUILD_TESTS)
    enable_testing()

    # TODO: Support manual googletest path.
    # Download googletest library source.
    set(GOOGLETEST_OUTPUT_DIR ${GLOBAL_OUTPUT_PATH}/googletest)
    ExternalProject_Add(
        googletest

        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG "release-1.8.0"

        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        TEST_COMMAND ""

        PREFIX ${PROJECT_SOURCE_DIR}/external/googletest
        CMAKE_ARGS -DBUILD_GTEST=ON -DBUILD_GMOCK=OFF
                -DCMAKE_INSTALL_PREFIX=${GOOGLETEST_OUTPUT_DIR}
    )

    # Copy googletest build outputs to global output path.
    ExternalProject_Add_Step(
        googletest copy_to_bin
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${GOOGLETEST_OUTPUT_DIR}/lib ${GLOBAL_OUTPUT_PATH}
        DEPENDEES install
    )

    include_directories(${GOOGLETEST_OUTPUT_DIR}/include)
    set(
        GTEST_LIBRARY
        ${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
    set(
        GTEST_MAIN_LIBRARY
        ${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
endif()

# Compiler flags.
if (${CMAKE_CXX_COMPILER_ID} MATCHES "Clang" OR
        ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -Wextra -pedantic -std=c99")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -Woverloaded-virtual -pedantic -std=c++0x -fPIC")
else()
    # TODO: Windows support.
    message(FATAL_ERROR "Compiler ${CMAKE_CXX_COMPILER_ID} not supported")
endif()

# Look for libraries in global output path.
link_directories(${GLOBAL_OUTPUT_PATH})

# Targets

include_directories(
    include
    third_party/md5
    core)

install(DIRECTORY include DESTINATION include)

add_subdirectory(third_party/md5)
add_subdirectory(core)
add_subdirectory(cmd)
add_subdirectory(test_suite)

if (BUILD_TESTS)
    # Set JSONNET_CMD variable required for regression tests.
    file(TO_NATIVE_PATH ${GLOBAL_OUTPUT_PATH}/jsonnet JSONNET_CMD)
    set(CTEST_ENVIRONMENT
        "JSONNET_CMD=${JSONNET_CMD}")

    # |run_tests| target builds and runs all tests. The cmake-generated |test|
    # target runs tests without building them.
    add_custom_target(run_tests COMMAND ${CMAKE_CTEST_COMMAND}
        DEPENDS libjsonnet_test libjsonnet_test_file libjsonnet_test_snippet
                jsonnet parser_test lexer_test
    )

endif()