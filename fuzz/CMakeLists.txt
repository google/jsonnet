# libFuzzer-based fuzz targets for jsonnet.
# Only built with -DBUILD_FUZZERS=ON and requires clang. See README.md
# in this directory.

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Building the fuzzers requires the Clang compiler (-fsanitize=fuzzer).")
endif()

set(JSONNET_FUZZ_TARGETS
    convert_jsonnet_fuzzer_regular
    convert_jsonnet_fuzzer_stream
    convert_jsonnet_fuzzer_multi)

foreach(fuzz_target ${JSONNET_FUZZ_TARGETS})
    add_executable(${fuzz_target} ${fuzz_target}.cc)
    configure_target_cc_props(${fuzz_target})
    target_link_libraries(${fuzz_target} PRIVATE jsonnet_lib_for_binaries)
    target_compile_options(${fuzz_target} PRIVATE -fsanitize=fuzzer-no-link,address)
    target_link_options(${fuzz_target} PRIVATE -fsanitize=fuzzer,address)
endforeach()
