cmake_minimum_required(VERSION 3.15...4.1)

#### Check important configuration pre-requisites.
#

# Discourage in-source builds; they overwrite the hand-written Makefile, and generally cause confusion.
# Use `cmake . -B<dir>` or the CMake GUI to do an out-of-source build.
if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "Out-of-source builds are recommended for this project.")
endif()

#### Extract version from include/libjsonnet.h so it can be put in the project version.
#

# Extract release version number from include/libjsonnet.h.
# We do this in various other (non-CMake-build) places too, e.g., the Python wheel build.

set(JSONNET_HEADER_VERSION_REGEX
    [=[^[ \t]*#[ \t]*define[ \t]+LIB_JSONNET_VERSION[ \t]+\"v([0-9.]+(-?[a-z][a-z0-9]*)?)\"[ \t]*$]=])
file(
    STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/include/libjsonnet.h JSONNET_HEADER_VERSION
    LIMIT_COUNT 1
    REGEX ${JSONNET_HEADER_VERSION_REGEX})
string(REGEX REPLACE ${JSONNET_HEADER_VERSION_REGEX} "\\1" JSONNET_HEADER_VERSION "${JSONNET_HEADER_VERSION}")
message(VERBOSE "Extracted version from include/libjsonnet.h: ${JSONNET_HEADER_VERSION}")

#### Set CMake project.
#

project(
    jsonnet
    VERSION ${JSONNET_HEADER_VERSION}
    HOMEPAGE_URL "https://jsonnet.org/"
    LANGUAGES C CXX)

#### Unconditional CMake module includes.
#

include(GNUInstallDirs)

#### Set up cacheable/user-configurable state.
#

# User-configurable options.
option(BUILD_JSONNET "Build jsonnet command-line tool." ON)
option(BUILD_JSONNETFMT "Build jsonnetfmt command-line tool." ON)
option(BUILD_TESTS "Build and run jsonnet tests." ON)
option(BUILD_STATIC_LIBS "Build a static libjsonnet." ON)
option(BUILD_SHARED_LIBS "Build shared libjsonnet." ON)
option(BUILD_SHARED_BINARIES "Link binaries to the shared libjsonnet instead of the static one." OFF)
option(USE_SYSTEM_GTEST "Use system-provided gtest library" OFF)
option(USE_SYSTEM_JSON "Use the system-provided json library" OFF)
# TODO: Support using a system Rapid YAML install.

#### Compute derived values from user-configurable state.
#

# Helper variable set to ON if there is any reason we need to build the jsonnet command binaries.
if(BUILD_TESTS OR BUILD_JSONNET OR BUILD_JSONNETFMT)
    set(BUILD_SOME_BINARIES ON)
else()
    set(BUILD_SOME_BINARIES OFF)
endif()

#### System package searches.
#

if(USE_SYSTEM_JSON)
    find_package(nlohmann_json 3.6.1 REQUIRED)
endif()

#### Utility function to configure a target with our preferred C and C++ compilation flags.
#

function(configure_target_cc_props LIB_TARGET_NAME)
    set_property(TARGET ${LIB_TARGET_NAME} PROPERTY CXX_STANDARD 17)
    set_property(TARGET ${LIB_TARGET_NAME} PROPERTY CXX_EXTENSIONS OFF)
    set_property(TARGET ${LIB_TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET ${LIB_TARGET_NAME} PROPERTY C_STANDARD 99)
    set_property(TARGET ${LIB_TARGET_NAME} PROPERTY C_EXTENSIONS OFF)
    set_property(TARGET ${LIB_TARGET_NAME} PROPERTY C_STANDARD_REQUIRED ON)

    if (NOT MSVC)
        target_compile_options(${LIB_TARGET_NAME}
            PRIVATE
                -Wall -Wextra -Wimplicit-fallthrough
                -pedantic
                $<$<COMPILE_LANGUAGE:CXX>:-Woverloaded-virtual>
        )
    endif()
endfunction()

#### Sub-directory CMakeLists includes.
#

add_subdirectory(stdlib)

#### Utility function to set up all the properties for an OBJECT library for the jsonnet core.
#
# This is done as a function because there separate object libraries for shared vs. static.
# Shared has slightly different compilation flags (it needs -fPIC).
#

function(configure_jsonnet_obj_target LIB_TARGET_NAME)
    configure_target_cc_props(${LIB_TARGET_NAME})
    target_sources(${LIB_TARGET_NAME}
        PRIVATE
            third_party/md5/md5.h
            third_party/md5/md5.cpp

            third_party/rapidyaml/rapidyaml-0.10.0.hpp
            third_party/rapidyaml/rapidyaml.cpp

            core/static_error.h
            core/desugarer.h
            core/unicode.h
            core/vm.h
            core/static_analysis.h
            core/path_utils.h
            core/state.h
            core/pass.h
            core/ast.h
            core/json.h
            core/string_utils.h
            core/formatter.h
            core/parser.h
            core/lexer.h

            core/desugarer.cpp
            core/formatter.cpp
            core/lexer.cpp
            core/libjsonnet.cpp
            core/parser.cpp
            core/pass.cpp
            core/path_utils.cpp
            core/static_analysis.cpp
            core/string_utils.cpp
            core/vm.cpp

        PUBLIC
            include/libjsonnet.h
            include/libjsonnet_fmt.h
    )

    target_link_libraries(${LIB_TARGET_NAME} PRIVATE stdlib_h)

    if(USE_SYSTEM_JSON)
        target_link_libraries(${LIB_TARGET_NAME} PRIVATE nlohmann_json::nlohmann_json)
    else()
        target_sources(${LIB_TARGET_NAME}
            PRIVATE
                third_party/json/nlohmann/json.hpp
        )
        target_include_directories(${LIB_TARGET_NAME}
            SYSTEM PRIVATE third_party/json)
    endif()

    target_include_directories(${LIB_TARGET_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
        PRIVATE
            third_party/md5
            third_party/rapidyaml
        )
endfunction()

#### Utility function to set up library target properties for the C lib.
#

function(configure_jsonnet_lib_target LIB_TARGET_NAME OBJ_TARGET_NAME)
    configure_target_cc_props(${LIB_TARGET_NAME})
    target_link_libraries(${LIB_TARGET_NAME} PUBLIC ${OBJ_TARGET_NAME})
    set(JSONNET_PUBLIC_HEADERS include/libjsonnet.h include/libjsonnet_fmt.h)
    set_target_properties(${LIB_TARGET_NAME} PROPERTIES
        OUTPUT_NAME jsonnet
        PUBLIC_HEADER "${JSONNET_PUBLIC_HEADERS}")
endfunction()

#### Utility function to set up library target properties for the C++ lib.
#

function(configure_jsonnet_cpp_lib_target LIB_TARGET_NAME OBJ_TARGET_NAME)
    configure_target_cc_props(${LIB_TARGET_NAME})
    target_sources(${LIB_TARGET_NAME}
        PRIVATE
            cpp/libjsonnet++.cpp
            include/libjsonnet++.h
    )
    target_link_libraries(${LIB_TARGET_NAME} PUBLIC ${OBJ_TARGET_NAME})
    set(JSONNET_PUBLIC_HEADERS include/libjsonnet.h include/libjsonnet++.h)
    set_target_properties(${LIB_TARGET_NAME} PROPERTIES
        OUTPUT_NAME jsonnet++
        PUBLIC_HEADER "${JSONNET_PUBLIC_HEADERS}")
endfunction()

#### Static library target setup.
#

if(BUILD_STATIC_LIBS OR (BUILD_SOME_BINARIES AND NOT BUILD_SHARED_BINARIES))
    add_library(jsonnet_obj_static OBJECT)
    configure_jsonnet_obj_target(jsonnet_obj_static)

    add_library(jsonnet_static STATIC)
    configure_jsonnet_lib_target(jsonnet_static jsonnet_obj_static)

    add_library(jsonnet_cpp_static STATIC)
    configure_jsonnet_cpp_lib_target(jsonnet_cpp_static jsonnet_obj_static)

    if(BUILD_STATIC_LIBS)
        install(TARGETS jsonnet_static jsonnet_cpp_static)
    endif()
endif()

#### Shared library target setup.
#

if(BUILD_SHARED_LIBS OR (BUILD_SOME_BINARIES AND BUILD_SHARED_BINARIES))
    add_library(jsonnet_obj_shared OBJECT)
    set_property(TARGET jsonnet_obj_shared PROPERTY POSITION_INDEPENDENT_CODE ON)
    configure_jsonnet_obj_target(jsonnet_obj_shared)

    add_library(jsonnet_shared SHARED)
    configure_jsonnet_lib_target(jsonnet_shared jsonnet_obj_shared)
    set_target_properties(jsonnet_shared PROPERTIES
        VERSION     "${JSONNET_HEADER_VERSION}"
        SOVERSION   "0")

    add_library(jsonnet_cpp_shared SHARED)
    configure_jsonnet_cpp_lib_target(jsonnet_cpp_shared jsonnet_obj_shared)
    set_target_properties(jsonnet_cpp_shared PROPERTIES
        VERSION     "${JSONNET_HEADER_VERSION}"
        SOVERSION   "0")

    if(BUILD_SHARED_LIBS)
        install(TARGETS jsonnet_shared jsonnet_cpp_shared)
    endif()
endif()

#### Alias the library to use for binaries.
#

if(BUILD_SHARED_BINARIES)
    add_library(jsonnet_lib_for_binaries ALIAS jsonnet_shared)
    add_library(jsonnet_cpp_lib_for_binaries ALIAS jsonnet_cpp_shared)
else()
    add_library(jsonnet_lib_for_binaries ALIAS jsonnet_static)
    add_library(jsonnet_cpp_lib_for_binaries ALIAS jsonnet_cpp_static)
endif()

#### An object library for the utils.cpp code which is shared by both CLI binaries.
#

if(BUILD_SOME_BINARIES)
    add_library(jsonnet_cmd_utils OBJECT
        cmd/utils.cpp
        cmd/utils.h
    )
endif()

#### CLI (jsonnet and jsonnetfmt) binary targets.
#

if(BUILD_JSONNET OR BUILD_TESTS)
    add_executable(jsonnet
        cmd/jsonnet.cpp
        $<TARGET_OBJECTS:jsonnet_cmd_utils>
    )
    target_link_libraries(jsonnet PRIVATE jsonnet_lib_for_binaries)
    if(BUILD_JSONNET)
        install(TARGETS jsonnet)
    endif()
endif()

if(BUILD_JSONNETFMT OR BUILD_TESTS)
    add_executable(jsonnetfmt
        cmd/jsonnetfmt.cpp
        $<TARGET_OBJECTS:jsonnet_cmd_utils>
    )
    target_link_libraries(jsonnetfmt PRIVATE jsonnet_lib_for_binaries)
    if(BUILD_JSONNETFMT)
        install(TARGETS jsonnetfmt)
    endif()
endif()

#### GoogleTest based tests.
#

if(BUILD_TESTS)
    enable_testing()
    include(GoogleTest)
    if(USE_SYSTEM_GTEST)
        find_package(GTest REQUIRED)
    else()
        include(FetchContent)
        FetchContent_Declare(googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.17.0
        )
        # For Windows: Prevent overriding the parent project's compiler/linker settings.
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        # Prevent including GoogleTest in the generated install rules.
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()

    function(jsonnet_add_gtest TESTNAME JSONNET_LIB_TARGET)
        add_executable(${TESTNAME} ${ARGN})
        target_link_libraries(${TESTNAME} ${JSONNET_LIB_TARGET} gtest gmock gtest_main)
        gtest_discover_tests(${TESTNAME}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            PROPERTIES
                VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                ENVIRONMENT "JSONNET_SOURCE_BASE=${PROJECT_SOURCE_DIR}"
        )
        set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
    endfunction()

    jsonnet_add_gtest(test_core_unicode jsonnet_lib_for_binaries core/unicode_test.cpp)
    jsonnet_add_gtest(test_core_lexer jsonnet_lib_for_binaries core/lexer_test.cpp)
    jsonnet_add_gtest(test_core_parser jsonnet_lib_for_binaries core/parser_test.cpp)
    jsonnet_add_gtest(test_core_libjsonnet jsonnet_lib_for_binaries core/libjsonnet_test.cpp)
    jsonnet_add_gtest(test_cpp_libjsonnet jsonnet_cpp_lib_for_binaries cpp/libjsonnet++_test.cpp)
    jsonnet_add_gtest(test_cpp_libjsonnet_locale jsonnet_cpp_lib_for_binaries cpp/libjsonnet_test_locale.cpp)

    add_subdirectory(test_suite)
endif()  # if(BUILD_TESTS)

# --- CMake debugging outputs.

# Note cmake_language GET_MESSAGE_LOG_LEVEL exists only in 3.25 and above.
if(CMAKE_VERSION VERSION_GREATER "3.25")
    include(CMakePrintHelpers)
    cmake_language(GET_MESSAGE_LOG_LEVEL CURRENT_CMAKE_LOG_LEVEL)
    if(CURRENT_CMAKE_LOG_LEVEL MATCHES "TRACE")
        # Print specific properties for specific targets
        cmake_print_properties(
            TARGETS
                to_c_array
                stdlib_h
                jsonnet_obj_static
                jsonnet_obj_shared
                jsonnet_static
                jsonnet_shared
                jsonnet_cpp_static
                jsonnet_cpp_shared
                jsonnet_cmd_utils
                jsonnet
                jsonnetfmt 
            PROPERTIES 
                TYPE
                C_STANDARD
                CXX_STANDARD
                POSITION_INDEPENDENT_CODE
                PUBLIC_LINK_DEPENDS
                INTERFACE_LINK_DEPENDS
                PUBLIC_LINK_LIBRARIES
                INTERFACE_LINK_LIBRARIES
                PUBLIC_INCLUDE_DIRECTORIES
                INTERFACE_INCLUDE_DIRECTORIES
                OUTPUT_NAME
                PUBLIC_HEADER
        )
    endif()
endif()
