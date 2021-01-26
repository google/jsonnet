cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
include(./cmake/c4Project.cmake)
project(c4core
    DESCRIPTION "Multiplatform low-level C++ utilities"
    HOMEPAGE_URL "https://github.com/biojppm/c4core"
    LANGUAGES CXX)
c4_project(VERSION 0.0.1
    AUTHOR "Joao Paulo Magalhaes <dev@jpmag.me>")

c4_add_library(c4core
    INC_DIRS
       $<BUILD_INTERFACE:${C4CORE_SRC_DIR}> $<INSTALL_INTERFACE:include>
    SOURCE_ROOT ${C4CORE_SRC_DIR}
    SOURCES
        c4/allocator.hpp
        c4/base64.hpp
        c4/base64.cpp
        c4/blob.hpp
        c4/bitmask.hpp
        c4/charconv.hpp
        c4/c4_pop.hpp
        c4/c4_push.hpp
        c4/char_traits.cpp
        c4/char_traits.hpp
        c4/common.hpp
        c4/compiler.hpp
        c4/config.hpp
        c4/cpu.hpp
        c4/ctor_dtor.hpp
        c4/enum.hpp
        c4/error.cpp
        c4/error.hpp
        c4/export.hpp
        c4/format.hpp
        c4/format.cpp
        c4/hash.hpp
        c4/language.hpp
        c4/language.cpp
        c4/memory_resource.cpp
        c4/memory_resource.hpp
        c4/memory_util.cpp
        c4/memory_util.hpp
        c4/platform.hpp
        c4/preprocessor.hpp
        c4/restrict.hpp
        c4/span.hpp
        c4/std/std.hpp
        c4/std/string.hpp
        c4/std/tuple.hpp
        c4/std/vector.hpp
        c4/substr.hpp
        c4/substr_fwd.hpp
        c4/szconv.hpp
        c4/time.hpp
        c4/time.cpp
        c4/type_name.hpp
        c4/types.hpp
        c4/unrestrict.hpp
        c4/windows.hpp
        c4/windows_pop.hpp
        c4/windows_push.hpp
        c4/c4core.natvis
        #
        c4/ext/debugbreak/debugbreak.h
        c4/ext/rng/rng.hpp
        c4/ext/sg14/inplace_function.h
        c4/ext/fast_float.hpp
        c4/ext/fast_float/include/fast_float/ascii_number.h
        c4/ext/fast_float/include/fast_float/decimal_to_binary.h
        c4/ext/fast_float/include/fast_float/fast_float.h
        c4/ext/fast_float/include/fast_float/fast_table.h
        c4/ext/fast_float/include/fast_float/float_common.h
        c4/ext/fast_float/include/fast_float/parse_number.h
        c4/ext/fast_float/include/fast_float/simple_decimal_conversion.h
)


#-------------------------------------------------------

c4_install_target(c4core)
c4_install_exports()
c4_pack_project()

c4_add_dev_targets()

c4_set_default_pack_properties(TYPE LIBRARY)
include(CPack)
