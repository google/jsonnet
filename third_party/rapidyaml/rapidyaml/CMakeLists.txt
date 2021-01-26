cmake_minimum_required(VERSION 3.12)
include(./ext/c4core/cmake/c4Project.cmake)
project(ryml
    DESCRIPTION "Rapid YAML parsing and emitting"
    HOMEPAGE_URL "https://github.com/biojppm/rapidyaml"
    LANGUAGES CXX)
c4_project(VERSION 0.1.0 STANDALONE
    AUTHOR "Joao Paulo Magalhaes <dev@jpmag.me>")


#-------------------------------------------------------

option(RYML_DEFAULT_CALLBACKS "Enable ryml's default implementation of callbacks: allocate(), free(), error()" ON)
option(RYML_BUILD_API "Enable API generation (python, etc)" OFF)
option(RYML_DBG "Enable (very verbose) ryml debug prints." OFF)


#-------------------------------------------------------

c4_require_subproject(c4core INCORPORATE
    SUBDIRECTORY ${RYML_EXT_DIR}/c4core)

c4_add_library(ryml
    SOURCES
        ryml.hpp
        ryml_std.hpp
        c4/yml/detail/checks.hpp
        c4/yml/detail/parser_dbg.hpp
        c4/yml/detail/stack.hpp
        c4/yml/common.hpp
        c4/yml/common.cpp
        c4/yml/emit.def.hpp
        c4/yml/emit.hpp
        c4/yml/export.hpp
        c4/yml/node.hpp
        c4/yml/parse.hpp
        c4/yml/parse.cpp
        c4/yml/preprocess.hpp
        c4/yml/preprocess.cpp
        c4/yml/std/map.hpp
        c4/yml/std/std.hpp
        c4/yml/std/string.hpp
        c4/yml/std/vector.hpp
        c4/yml/tree.hpp
        c4/yml/tree.cpp
        c4/yml/writer.hpp
        c4/yml/yml.hpp
        ryml.natvis
    SOURCE_ROOT ${RYML_SRC_DIR}
    INC_DIRS
        $<BUILD_INTERFACE:${RYML_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
    LIBS c4core
    INCORPORATE c4core
    )

if(NOT RYML_DEFAULT_CALLBACKS)
    target_compile_definitions(ryml PRIVATE RYML_NO_DEFAULT_CALLBACKS)
endif()

if(RYML_DBG)
    target_compile_definitions(ryml PRIVATE RYML_DBG)
endif()


#-------------------------------------------------------

c4_install_target(ryml)
c4_install_exports(DEPENDENCIES c4core)
c4_pack_project()


#-------------------------------------------------------
# developer targets

# extern libraries, used only for testing/benchmarking
if(RYML_BUILD_TESTS OR RYML_BUILD_BENCHMARKS)
    set(ed ${CMAKE_CURRENT_BINARY_DIR}/subprojects) # casual ryml extern dir (these projects are not part of ryml and are downloaded and compiled on the fly)
    # these are used both for testing and benchmarking
    c4_require_subproject(c4fs REMOTE
        GIT_REPOSITORY https://github.com/biojppm/c4fs
        GIT_TAG master)
    c4_require_subproject(libyaml REMOTE
        GIT_REPOSITORY https://github.com/yaml/libyaml
        GIT_TAG master
      OVERRIDE BUILD_TESTING OFF
      SET_FOLDER_TARGETS ext yaml)
    c4_import_remote_proj(yaml-cpp ${ed}/yaml-cpp REMOTE
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp
        # the master branch regularly screws up on windows builds.
        # so use fixed pre-validated commit hashes
        GIT_TAG 587b24e2eedea1afa21d79419008ca5f7bda3bf4
      OVERRIDE YAML_CPP_BUILD_TESTS OFF YAML_CPP_BUILD_TOOLS OFF YAML_CPP_BUILD_CONTRIB OFF YAML_CPP_BUILD_INSTALL OFF
      SET_FOLDER_TARGETS ext yaml-cpp format)
    set(ryml_yaml_cpp_inc ${ed}/yaml-cpp/src/include)
    if(MSVC)
        target_compile_definitions(yaml-cpp PUBLIC -D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)
    endif()
endif()

c4_add_dev_targets()
