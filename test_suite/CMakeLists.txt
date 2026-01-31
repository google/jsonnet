find_program(BASH bash)
if (BASH STREQUAL "BASH-NOTFOUND")
    message(WARNING "Bash not found, can't run regression tests.")
else()
    message("Testing with Jsonnet executable: $<TARGET_FILE:jsonnet>")
    add_test(
        NAME regression_test
        COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.sh
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(
         regression_test
             PROPERTIES
             ENVIRONMENT JSONNET_BIN=$<TARGET_FILE:jsonnet>)

endif()
