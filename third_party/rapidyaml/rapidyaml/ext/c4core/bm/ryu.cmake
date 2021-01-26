# ryu does not have a cmakelists
enable_language(C)
c4_download_remote_proj(ryu RYU_DIR
    GIT_REPOSITORY https://github.com/ulfjack/ryu
    GIT_TAG master)
set(RYU_HDR
    ${RYU_DIR}/ryu/common.h
    ${RYU_DIR}/ryu/d2fixed_full_table.h
    ${RYU_DIR}/ryu/d2s_full_table.h
    ${RYU_DIR}/ryu/d2s_intrinsics.h
    ${RYU_DIR}/ryu/d2s_small_table.h
    ${RYU_DIR}/ryu/digit_table.h
    ${RYU_DIR}/ryu/f2s_full_table.h
    ${RYU_DIR}/ryu/f2s_intrinsics.h
    ${RYU_DIR}/ryu/ryu.h
    ${RYU_DIR}/ryu/ryu_parse.h
)
set(RYU_SRC
    ${RYU_DIR}/ryu/d2fixed.c
    ${RYU_DIR}/ryu/d2s.c
    ${RYU_DIR}/ryu/f2s.c
    ${RYU_DIR}/ryu/s2d.c
    ${RYU_DIR}/ryu/s2f.c
)
add_library(ryu_c4 ${RYU_SRC} ${RYU_HDR})
target_include_directories(ryu_c4 PUBLIC $<BUILD_INTERFACE:${RYU_DIR}>)
set_target_properties(ryu_c4 PROPERTIES LINKER_LANGUAGE CXX)
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(ryu_c4 PRIVATE -Wno-sign-conversion)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
    target_compile_options(ryu_c4 -Wno-deprecated)
endif()
_c4_set_target_folder(ryu_c4 ext)
