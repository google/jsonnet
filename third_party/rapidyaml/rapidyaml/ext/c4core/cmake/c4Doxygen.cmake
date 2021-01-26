# (C) 2019 Joao Paulo Magalhaes <dev@jpmag.me>
if(NOT _c4_doxygen_included)
set(_c4_doxygen_included ON)


#------------------------------------------------------------------------------
# TODO use customizations from https://cmake.org/cmake/help/v3.9/module/FindDoxygen.html
function(c4_setup_doxygen umbrella_option)
    cmake_dependent_option(${_c4_uprefix}BUILD_DOCS "Enable targets to build documentation for ${prefix}" ON "${umbrella_option}" OFF)
    if(${_c4_uprefix}BUILD_DOCS)
        find_package(Doxygen QUIET)
        if(DOXYGEN_FOUND)
            c4_log("enabling documentation targets")
        else()
            c4_dbg("doxygen not found")
        endif()
    endif()
endfunction()

#------------------------------------------------------------------------------
function(c4_add_doxygen doc_name)
    if(NOT ${_c4_uprefix}BUILD_DOCS)
        return()
    endif()
    #
    set(opt0
    )
    set(opt1
        DOXYFILE DOXYFILE_IN
        PROJ
        PROJ_BRIEF
        VERSION
        OUTPUT_DIR
        CLANG_DATABASE_PATH
    )
    set(optN
        INPUT
        FILE_PATTERNS
        EXCLUDE
        EXCLUDE_PATTERNS
        EXCLUDE_SYMBOLS
        STRIP_FROM_PATH
        STRIP_FROM_INC_PATH
        EXAMPLE_PATH
    )
    cmake_parse_arguments("" "${opt0}" "${opt1}" "${optN}" ${ARGN})
    #
    if(NOT _PROJ)
        set(_PROJ ${_c4_ucprefix})
    endif()
    if(NOT _DOXYFILE AND NOT _DOXYFILE_IN)
        set(_DOXYFILE_IN ${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in)
    endif()
    if(NOT _OUTPUT_DIR)
        if("${doc_name}" MATCHES "^[Dd]oc")
            set(_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${doc_name})
        else()
            set(_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/doc/${doc_name})
        endif()
    endif()
    #
    _c4_doxy_fwd_to_cmd(_PROJ OFF)
    _c4_doxy_fwd_to_cmd(_PROJ_BRIEF OFF)
    _c4_doxy_fwd_to_cmd(_VERSION OFF)
    _c4_doxy_fwd_to_cmd(_OUTPUT_DIR OFF)
    _c4_doxy_fwd_to_cmd(_CLANG_DATABASE_PATH OFF)
    _c4_doxy_fwd_to_cmd(_INPUT ON)
    _c4_doxy_fwd_to_cmd(_FILE_PATTERNS ON)
    _c4_doxy_fwd_to_cmd(_EXCLUDE ON)
    _c4_doxy_fwd_to_cmd(_EXCLUDE_PATTERNS ON)
    _c4_doxy_fwd_to_cmd(_EXCLUDE_SYMBOLS ON)
    _c4_doxy_fwd_to_cmd(_STRIP_FROM_PATH ON)
    _c4_doxy_fwd_to_cmd(_STRIP_FROM_INC_PATH ON)
    _c4_doxy_fwd_to_cmd(_EXAMPLE_PATH ON)
    #
    if("${doc_name}" MATCHES "^[Dd]oc")
        set(tgt ${_c4_lcprefix}-${doc_name})
    else()
        set(tgt ${_c4_lcprefix}-doc-${doc_name})
    endif()
    #
    if(_DOXYFILE)
        set(doxyfile_out ${_DOXYFILE})
    elseif(_DOXYFILE_IN)
        set(doxyfile_out ${_OUTPUT_DIR}/Doxyfile)
        set(config_script ${_c4_project_dir}/c4DoxygenConfig.cmake)
        add_custom_command(OUTPUT ${doxyfile_out}
            COMMAND ${CMAKE_COMMAND} -E remove -f ${doxyfile_out}
            COMMAND ${CMAKE_COMMAND} -DDOXYFILE_IN=${_DOXYFILE_IN}  -DDOXYFILE_OUT=${doxyfile_out} ${defs} '-DALLVARS=${allvars}' '-DLISTVARS=${listvars}' -P ${config_script}
            DEPENDS ${_DOXYFILE_IN} ${config_script}
            COMMENT "${tgt}: generating ${doxyfile_out}"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    #
    add_custom_target(${tgt}
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile_out}
        DEPENDS ${doxyfile_out}
        WORKING_DIRECTORY ${_OUTPUT_DIR}
        COMMENT "${tgt}: docs will be placed in ${_OUTPUT_DIR}"
        VERBATIM)
    _c4_set_target_folder(${tgt} doc)
endfunction()


macro(_c4_doxy_fwd_to_cmd varname is_list)
    if(NOT ("${${varname}}" STREQUAL ""))
        if("${defs}" STREQUAL "")
            set(li "-D${varname}=${${varname}}")
        else()
            set(li ${defs})
            list(APPEND li "-D${varname}='${${varname}}'")
        endif()
        set(defs ${li})
    endif()
    set(allvars "${allvars};${varname}")
    if(${is_list})
        set(listvars "${listvars};${varname}")
    endif()
endmacro()

endif(NOT _c4_doxygen_included)
