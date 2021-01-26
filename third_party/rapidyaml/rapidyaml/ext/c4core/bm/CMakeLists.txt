c4_setup_benchmarking()

#----------------------------------------------

# libraries to compare

c4_require_subproject(fp
  REMOTE
    GIT_REPOSITORY https://github.com/jk-jeon/fp
    GIT_TAG master)

# ryu requires too much code; use an include for clarity
include(./ryu.cmake)


#----------------------------------------------

c4_add_executable(c4core-bm-format
    SOURCES bm_format.cpp
    LIBS c4core benchmark
    FOLDER bm)

c4_add_target_benchmark(c4core-bm-format cat FILTER "^cat_.*")
c4_add_target_benchmark(c4core-bm-format catsep FILTER "^catsep_.*")
c4_add_target_benchmark(c4core-bm-format format FILTER "^format_.*")


#----------------------------------------------

c4_add_executable(c4core-bm-charconv
    SOURCES bm_charconv.cpp
    LIBS c4core benchmark ryu_c4
    INC_DIRS ${C4CORE_SRC_DIR}/c4/ext/fast_float/include
    FOLDER bm)

get_property(known_features GLOBAL PROPERTY CMAKE_CXX_KNOWN_FEATURES)
if(cxx_std_17 IN_LIST known_features)
    target_link_libraries(c4core-bm-charconv PRIVATE jkj_fp)
endif()


function(c4core_charconv_benchmark pfx)
    foreach(case ${ARGN})
        c4_add_target_benchmark(c4core-bm-charconv ${pfx}-${case} FILTER "^${pfx}.*<${case}.*")
    endforeach()
endfunction()

c4core_charconv_benchmark(xtoa uint8 int8 uint16 int16 uint32 int32 uint64 int64 float double)
c4core_charconv_benchmark(atox uint8 int8 uint16 int16 uint32 int32 uint64 int64 float double)


#----------------------------------------------

option(C4CORE_BM_FLOAT_CMP "compare between alternative implementations for reading/writing floats" ON)
if(C4CORE_BM_FLOAT_CMP)
    add_subdirectory(float)
endif()
