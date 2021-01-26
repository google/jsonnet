find_package(Python COMPONENTS Interpreter)

# this benchmark compares the binaries from several float libraries

function(c4core_bm_readfloat name define)
    set(target c4core-bm-readfloat-${name})
    c4_add_executable(${target}
        SOURCES read.cpp
        LIBS ${ARGN}
        FOLDER bm/float)
    use_static_libc(${target})
    if(NOT "${define}" STREQUAL "")
        target_compile_definitions(${target} PUBLIC ${define}=1)
    endif()
    target_include_directories(${target} PUBLIC ${C4CORE_SRC_DIR})
    if(NOT TARGET c4core-bm-readfloat)
        add_custom_target(c4core-bm-readfloat)
    endif()
    add_dependencies(c4core-bm-readfloat ${target})
    _c4_set_target_folder(c4core-bm-readfloat bm)
    set(measure ${CMAKE_CURRENT_LIST_DIR}/measure.py)
    target_sources(${target} PRIVATE ${measure})
    if(UNIX)
        set(unix --unix)  # $<$<BOOL:UNIX>:--unix>
    endif()
    add_custom_command(TARGET ${target} PRE_BUILD
        COMMAND ${Python_EXECUTABLE} ${measure} start ${target}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${Python_EXECUTABLE} ${measure} finish ${unix} ${target} $<TARGET_FILE:${target}>
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction()


function(use_static_libc target)
    if(MSVC)
        # https://cmake.org/cmake/help/v3.15/prop_tgt/MSVC_RUNTIME_LIBRARY.html
        set_property(TARGET ${target} PROPERTY
            MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        # https://stackoverflow.com/questions/38694058/cmake-linking-statically-against-libgcc-and-libstdc-into-a-shared-library
        # see also:
        # https://github.com/fastfloat/fast_float/pull/41#issuecomment-733346131
        target_link_libraries(${target} PUBLIC -static -static-libgcc -static-libstdc++)
    endif()
endfunction()


c4core_bm_readfloat(baseline "")
c4core_bm_readfloat(std_atof     C4FLOAT_STD_ATOF)
c4core_bm_readfloat(sscanf_f     C4FLOAT_SSCANF_F)
c4core_bm_readfloat(sscanf_d     C4FLOAT_SSCANF_D)
c4core_bm_readfloat(iostream_f   C4FLOAT_IOSTREAM_F)
c4core_bm_readfloat(iostream_d   C4FLOAT_IOSTREAM_D)
c4core_bm_readfloat(fast_float_f C4FLOAT_FASTFLOAT_F)
c4core_bm_readfloat(fast_float_d C4FLOAT_FASTFLOAT_D)
c4core_bm_readfloat(ryu_f        C4FLOAT_RYU_F ryu_c4)
c4core_bm_readfloat(ryu_d        C4FLOAT_RYU_D ryu_c4)

get_property(known_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
if(cxx_std_17 IN_LIST known_features)
    c4core_bm_readfloat(fast_fp_f_limited    C4FLOAT_FP_F_LIMITED jkj_fp)
    c4core_bm_readfloat(fast_fp_d_limited    C4FLOAT_FP_D_LIMITED jkj_fp)
    c4core_bm_readfloat(fast_fp_f_unlimited    C4FLOAT_FP_F_UNLIMITED jkj_fp)
    c4core_bm_readfloat(fast_fp_d_unlimited    C4FLOAT_FP_D_UNLIMITED jkj_fp)
    target_compile_features(c4core-bm-readfloat-fast_fp_f_limited PRIVATE cxx_std_17)
    target_compile_features(c4core-bm-readfloat-fast_fp_d_limited PRIVATE cxx_std_17)
    target_compile_features(c4core-bm-readfloat-fast_fp_f_unlimited PRIVATE cxx_std_17)
    target_compile_features(c4core-bm-readfloat-fast_fp_d_unlimited PRIVATE cxx_std_17)
    # incredible - 3 years of C++17 and from_chars()/to_chars() is not available
    if(NOT ((CMAKE_CXX_COMPILER_ID STREQUAL GNU) OR (CMAKE_CXX_COMPILER_ID STREQUAL Clang)))
        c4core_bm_readfloat(std_from_chars_f C4FLOAT_STD_FROM_CHARS_F)
        c4core_bm_readfloat(std_from_chars_d C4FLOAT_STD_FROM_CHARS_D)
        target_compile_features(c4core-bm-readfloat-std_from_chars_f PRIVATE cxx_std_17)
        target_compile_features(c4core-bm-readfloat-std_from_chars_d PRIVATE cxx_std_17)
    endif()
endif()
