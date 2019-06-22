# libjsonnet++

set(LIBJSONNETPP_HEADERS
    ../include/libjsonnet++.h
)

set(LIBJSONNETPP_SOURCE
    libjsonnet++.cpp
)

add_library(libjsonnet++ SHARED ${LIBJSONNETPP_HEADERS} ${LIBJSONNETPP_SOURCE})
add_dependencies(libjsonnet++ jsonnet)
# target_link_libraries(libjsonnet libjsonnet)

# CMake prepends CMAKE_SHARED_LIBRARY_PREFIX to shared libraries, so without
# this step the output would be |liblibjsonnet|.
set_target_properties(libjsonnet++ PROPERTIES OUTPUT_NAME jsonnet++
	VERSION     "0.13.0"
	SOVERSION   "0"
	PUBLIC_HEADER "${LIB_HEADER}")
install(TARGETS libjsonnet++
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Static library for jsonnet command-line tool.
add_library(libjsonnet++_static STATIC ${LIBJSONNETPP_SOURCE})
add_dependencies(libjsonnet++_static jsonnet)
target_link_libraries(libjsonnet++_static libjsonnet_static)
set_target_properties(libjsonnet++_static PROPERTIES OUTPUT_NAME jsonnet++)
install(TARGETS libjsonnet_static DESTINATION "${CMAKE_INSTALL_LIBDIR}")

# Tests
function(add_test_executablepp test_name)
    if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/${test_name}.cpp)
        set(TEST_EXT cpp)
    else()
        set(TEST_EXT c)
    endif()
    add_executable(${test_name} ${test_name}.${TEST_EXT})

    add_dependencies(${test_name} libjsonnet++_static jsonnet gtest_main)
    target_link_libraries(
        ${test_name} gtest gtest_main libjsonnet++_static libjsonnet_static)
endfunction()

if (BUILD_TESTS)
    add_test_executablepp(libjsonnet++_test)
    add_test(libjsonnet++_test ${GLOBAL_OUTPUT_PATH}/libjsonnet++_test)

endif()
