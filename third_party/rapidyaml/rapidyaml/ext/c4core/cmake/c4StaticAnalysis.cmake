include(PVS-Studio)
include(GetFlags)
include(c4GetTargetPropertyRecursive)


function(_c4sta_default_if_not_set var dft)
    if("${${var}}" STREQUAL "")
        set(${var} "${dft}" PARENT_SCOPE)
    endif()
endfunction()


function(c4_setup_static_analysis umbrella_option)
    if(WIN32)
        c4_dbg("no static analyzer available in WIN32")
        return()
    endif()
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage")
        c4_dbg("Coverage build: disabling static analyzers")
        return()
    endif()
    _c4sta_default_if_not_set(C4_LINT ${umbrella_option})
    _c4sta_default_if_not_set(C4_LINT_TESTS ${umbrella_option})
    _c4sta_default_if_not_set(C4_LINT_CLANG_TIDY ${umbrella_option})
    _c4sta_default_if_not_set(C4_LINT_PVS_STUDIO OFF)
    # option to turn lints on/off
    cmake_dependent_option(${_c4_uprefix}LINT "add static analyzer targets" ${C4_LINT} ${umbrella_option} OFF)
    cmake_dependent_option(${_c4_uprefix}LINT_TESTS "add tests to run static analyzer targets" ${C4_LINT_TESTS} ${umbrella_option} OFF)
    # options for individual lints - contingent on linting on/off
    cmake_dependent_option(${_c4_uprefix}LINT_CLANG_TIDY "use the clang-tidy static analyzer" ${C4_LINT_CLANG_TIDY} "${_c4_uprefix}LINT" ON)
    cmake_dependent_option(${_c4_uprefix}LINT_PVS_STUDIO "use the PVS-Studio static analyzer https://www.viva64.com/en/b/0457/" ${C4_LINT_PVS_STUDIO} "${_c4_uprefix}LINT" OFF)
    if(${_c4_uprefix}LINT_CLANG_TIDY)
        find_program(CLANG_TIDY clang-tidy)
    endif()
    if(${_c4_uprefix}LINT_PVS_STUDIO)
        set(${_c4_uprefix}LINT_PVS_STUDIO_FORMAT "errorfile" CACHE STRING "PVS-Studio output format. Choices: xml,csv,errorfile(like gcc/clang),tasklist(qtcreator)")
    endif()
    #
    set(sa)
    if(${_c4_uprefix}LINT_CLANG_TIDY)
        set(sa "clang_tidy")
    endif()
    if(${_c4_uprefix}LINT_PVS_STUDIO)
        set(sa "${sa} PVS-Studio")
    endif()
    if(sa)
        c4_dbg("enabled static analyzers: ${sa}")
    endif()
endfunction()


function(c4_static_analysis_target target_name folder generated_targets)
    set(any_linter OFF)
    if(${_c4_uprefix}LINT_CLANG_TIDY OR ${_c4_uprefix}LINT_PVS_STUDIO)
        set(any_linter ON)
    endif()
    if(${_c4_uprefix}LINT AND any_linter)
        # umbrella target for running all linters for this particular target
        if(any_linter AND NOT TARGET ${_c4_lprefix}lint-all)
            add_custom_target(${_c4_lprefix}lint-all)
            if(folder)
                #message(STATUS "${target_name}: folder=${folder}")
                set_target_properties(${_c4_lprefix}lint-all PROPERTIES FOLDER "${folder}")
            endif()
        endif()
        if(${_c4_uprefix}LINT_CLANG_TIDY)
            c4_static_analysis_clang_tidy(${target_name}
                ${target_name}-lint-clang_tidy
                ${_c4_lprefix}lint-all-clang_tidy
                "${folder}")
            list(APPEND ${generated_targets} ${_c4_lprefix}lint-clang_tidy)
            add_dependencies(${_c4_lprefix}lint-all ${_c4_lprefix}lint-all-clang_tidy)
        endif()
        if(${_c4_uprefix}LINT_PVS_STUDIO)
            c4_static_analysis_pvs_studio(${target_name}
                ${target_name}-lint-pvs_studio
                ${_c4_lprefix}lint-all-pvs_studio
                "${folder}")
            list(APPEND ${generated_targets} ${_c4_lprefix}lint-pvs_studio)
            add_dependencies(${_c4_lprefix}lint-all ${_c4_lprefix}lint-all-pvs_studio)
        endif()
    endif()
endfunction()


function(c4_static_analysis_add_tests target_name)
    if(${_c4_uprefix}LINT_CLANG_TIDY AND ${_c4_uprefix}LINT_TESTS)
        add_test(NAME ${target_name}-lint-clang_tidy-run
            COMMAND
            ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target ${target_name}-lint-clang_tidy)
    endif()
    if(${_c4_uprefix}LINT_PVS_STUDIO AND ${_c4_uprefix}LINT_TESTS)
        add_test(NAME ${target_name}-lint-pvs_studio-run
            COMMAND
            ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target ${target_name}-lint-pvs_studio)
    endif()
endfunction()


#------------------------------------------------------------------------------
function(c4_static_analysis_clang_tidy subj_target lint_target umbrella_target folder)
    c4_static_analysis_clang_tidy_get_cmd(${subj_target} ${lint_target} cmd)
    string(REPLACE ";" " " cmd_str "${cmd}")
    add_custom_target(${lint_target}
        COMMAND ${CMAKE_COMMAND} -E echo "cd ${CMAKE_CURRENT_SOURCE_DIR} ; ${cmd_str}"
        COMMAND ${cmd}
        VERBATIM
        COMMENT "clang-tidy: analyzing sources of ${subj_target}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    if(folder)
        set_target_properties(${lint_target} PROPERTIES FOLDER "${folder}")
    endif()
    if(NOT TARGET ${umbrella_target})
        add_custom_target(${umbrella_target})
    endif()
    add_dependencies(${umbrella_target} ${lint_target})
endfunction()

function(c4_static_analysis_clang_tidy_get_cmd subj_target lint_target cmd)
    get_target_property(_clt_all_srcs ${subj_target} SOURCES)
    _c4cat_filter_srcs_hdrs("${_clt_all_srcs}" _clt_srcs)
    set(result "${CLANG_TIDY}" -p ${CMAKE_BINARY_DIR} --header-filter=.* ${_clt_srcs})
    set(${cmd} ${result} PARENT_SCOPE)
endfunction()


#------------------------------------------------------------------------------
function(c4_static_analysis_pvs_studio subj_target lint_target umbrella_target folder)
    c4_get_target_property_recursive(_c4al_pvs_incs ${subj_target} INCLUDE_DIRECTORIES)
    c4_get_include_flags(_c4al_pvs_incs ${_c4al_pvs_incs})
    separate_arguments(_c4al_cxx_flags_sep UNIX_COMMAND "${CMAKE_CXX_FLAGS} ${_c4al_pvs_incs}")
    separate_arguments(_c4al_c_flags_sep UNIX_COMMAND "${CMAKE_C_FLAGS} ${_c4al_pvs_incs}")
    pvs_studio_add_target(TARGET ${lint_target}
        ALL # indicates that the analysis starts when you build the project
        #PREPROCESSOR ${_c4al_preproc}
        FORMAT tasklist
        LOG "${CMAKE_CURRENT_BINARY_DIR}/${subj_target}.pvs-analysis.tasks"
        ANALYZE ${name} #main_target subtarget:path/to/subtarget
        CXX_FLAGS ${_c4al_cxx_flags_sep}
        C_FLAGS ${_c4al_c_flags_sep}
        #CONFIG "/path/to/PVS-Studio.cfg"
        )
    if(folder)
        set_target_properties(${lint_target} PROPERTIES FOLDER "${folder}")
    endif()
    if(NOT TARGET ${umbrella_target})
        add_custom_target(${umbrella_target})
    endif()
    add_dependencies(${umbrella_target} ${lint_target})
endfunction()

function(c4_static_analysis_pvs_studio_get_cmd subj_target lint_target cmd)
    set(${cmd} $<RULE_LAUNCH_CUSTOM:${subj_target}> PARENT_SCOPE)
endfunction()
