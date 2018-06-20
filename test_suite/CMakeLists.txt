find_program(BASH bash)
if (BASH STREQUAL "BASH-NOTFOUND")
    message(WARNING "Bash not found, can't run regression tests.")
else()
    foreach(TEST_SUITE run_tests run_fmt_tests run_fmt_idempotence_tests)
        set(TMP_DIR "${CMAKE_CURRENT_BINARY_DIR}/tmp_dir_${TEST_SUITE}")
        if (EXISTS "${TMP_DIR}")
            file(REMOVE_RECURSE ${TMP_DIR})
        endif()
        file(MAKE_DIRECTORY ${TMP_DIR})

        add_test(
            NAME regression_${TEST_SUITE}
            COMMAND ${BASH} ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SUITE}.sh
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")

        set_property(   TEST    regression_${TEST_SUITE}
                        PROPERTY ENVIRONMENT "JSONNET_BIN=$<TARGET_FILE:jsonnet>"
                        PROPERTY ENVIRONMENT "TMP_DIR=${TMP_DIR}")
    endforeach()
endif()
