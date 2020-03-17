# libjsonnet

# Remember to update Bazel and Makefile builds when updating this list!
set(LIBJSONNET_HEADERS
    ast.h
    desugarer.h
    formatter.h
    lexer.h
    ../include/libjsonnet.h
    ../include/libjsonnet_fmt.h
    parser.h
    pass.h
    state.h
    static_analysis.h
    static_error.h
    string_utils.h
    unicode.h
    vm.h)

set(LIBJSONNET_SOURCE
    desugarer.cpp
    formatter.cpp
    lexer.cpp
    libjsonnet.cpp
    parser.cpp
    pass.cpp
    static_analysis.cpp
    string_utils.cpp
    vm.cpp)

add_library(libjsonnet SHARED ${LIBJSONNET_HEADERS} ${LIBJSONNET_SOURCE})
add_dependencies(libjsonnet md5 stdlib)
target_link_libraries(libjsonnet md5 nlohmann_json::nlohmann_json)

file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/../include/libjsonnet.h JSONNET_VERSION_DEF
     REGEX "[#]define[ \t]+LIB_JSONNET_VERSION[ \t]+")
string(REGEX REPLACE ".*\"v([^\"]+)\".*" "\\1" JSONNET_VERSION ${JSONNET_VERSION_DEF})
message("Extracted Jsonnet version: " ${JSONNET_VERSION})


# CMake prepends CMAKE_SHARED_LIBRARY_PREFIX to shared libraries, so without
# this step the output would be |liblibjsonnet|.
set_target_properties(libjsonnet PROPERTIES OUTPUT_NAME jsonnet
	VERSION     "${JSONNET_VERSION}"
	SOVERSION   "0"
	PUBLIC_HEADER "${LIB_HEADER}")
install(TARGETS libjsonnet
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
target_include_directories(libjsonnet INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)

if (BUILD_STATIC_LIBS)
    # Static library for jsonnet command-line tool.
    add_library(libjsonnet_static STATIC ${LIBJSONNET_SOURCE})
    add_dependencies(libjsonnet_static md5 stdlib)
    target_link_libraries(libjsonnet_static md5 nlohmann_json::nlohmann_json)
    set_target_properties(libjsonnet_static PROPERTIES OUTPUT_NAME jsonnet)
    install(TARGETS libjsonnet_static DESTINATION "${CMAKE_INSTALL_LIBDIR}")
    target_include_directories(libjsonnet_static INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>)
endif()

if (BUILD_SHARED_BINARIES OR NOT BUILD_STATIC_LIBS)
    add_library(libjsonnet_for_binaries ALIAS libjsonnet)
else()
    add_library(libjsonnet_for_binaries ALIAS libjsonnet_static)
endif()

# Tests

function(add_test_executable test_name)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${test_name}.cpp)
        set(TEST_EXT cpp)
    else()
        set(TEST_EXT c)
    endif()
    add_executable(${test_name} ${test_name}.${TEST_EXT})

    add_dependencies(${test_name} libjsonnet_for_binaries GTest::Main)
    target_link_libraries(
        ${test_name} GTest::GTest GTest::Main libjsonnet_for_binaries)
endfunction()

if (BUILD_TESTS)
    add_test_executable(lexer_test)
    add_test(lexer_test ${GLOBAL_OUTPUT_PATH}/lexer_test)

    add_test_executable(parser_test)
    add_test(parser_test ${GLOBAL_OUTPUT_PATH}/parser_test)

    add_test_executable(libjsonnet_test)
    add_test(libjsonnet_test ${GLOBAL_OUTPUT_PATH}/libjsonnet_test)

    add_test_executable(libjsonnet_test_file)
    add_test(libjsonnet_test_file
        ${GLOBAL_OUTPUT_PATH}/libjsonnet_test_file
        ${CMAKE_CURRENT_SOURCE_DIR}/../test_suite/object.jsonnet)

    set(TEST_SNIPPET "std.assertEqual(({ x: 1, y: self.x } { x: 2 }).y, 2)")
    add_test_executable(libjsonnet_test_snippet)
    add_test(libjsonnet_test_snippet
        ${GLOBAL_OUTPUT_PATH}/libjsonnet_test_snippet ${TEST_SNIPPET})

    add_test(jsonnet_test_snippet
        ${GLOBAL_OUTPUT_PATH}/jsonnet -e ${TEST_SNIPPET})
endif()
