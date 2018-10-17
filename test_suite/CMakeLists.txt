find_program(BASH bash)
if (BASH STREQUAL "BASH-NOTFOUND")
    message(WARNING "Bash not found, can't run regression tests.")
else()
    # Note: this test relies on the JSONNET_BIN variable, which
    # is set in the root CMakeLists.txt.
    message( "Testing with Jsonnet executable: ${JSONNET_BIN}" )
    add_test(
        NAME regression_test
        COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(
         regression_test
             PROPERTIES
             ENVIRONMENT JSONNET_BIN=${JSONNET_BIN})

endif()
