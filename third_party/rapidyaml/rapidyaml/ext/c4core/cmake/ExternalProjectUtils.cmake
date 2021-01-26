# (C) 2017 Joao Paulo Magalhaes <dev@jpmag.me>

include(CMakeParseArguments)

#------------------------------------------------------------------------------
# Usage:
#
#   ExternalProject_GetFwdArgs(output_var
#       [NO_DEFAULTS]
#       [VARS var1 var2 ...]
#       [EXCLUDE xvar1 xvar2 ...]
#       [QUIET]
#   )
#
# Get the current cmake environment in a sequence of -DVAR=${VAR}
# tokens so that the environment can be forwarded to an external
# cmake project through CMAKE_ARGS.
#
# Example:
#   ExternalProject_GetFwdArgs(FWD_ARGS)
#   ExternalProject_Add(foo SOURCE_DIR ../foo
#       CMAKE_ARGS ${FWD_ARGS}
#       ... etc)
#
# Use this function to enable forwarding the current cmake environment
# to an external project. It outputs all the needed variables in the
# form of a sequence of -DVAR=value, suitable for use in the CMAKE_ARGS
# clause of ExternalProject_Add().
#
# This function uses ExternalProject_GetFwdVarNames() to find out the
# list of variables to export. If this behaviour does not fit your
# needs you can:
#
#     * append more of your own variables (using the VARS
#       argument). The vars specified in this option will each be
#       added to the output in the form of -Dvar=${var}
#
#     * you can also avoid any defaults obtained through usage of
#       ExternalProject_GetFwdNames() by specifying NO_DEFAULTS.
#
# Example with custom variable names (adding more):
#   ExternalProject_GetFwdVarNames(FWD_ARGS VARS USER_VAR1 USER_VAR2)
#   ExternalProjectAdd(foo SOURCE_DIR ../foo CMAKE_ARGS ${FWD_ARGS})
#
# Example with custom variable names (just your own):
#   ExternalProject_GetFwdVarNames(FWD_ARGS NO_DEFAULTS VARS USER_VAR1 USER_VAR2)
#   ExternalProjectAdd(foo SOURCE_DIR ../foo CMAKE_ARGS ${FWD_ARGS})
#
function(ExternalProject_GetFwdArgs output_var)
    set(options0arg
        NO_DEFAULTS
        QUIET
        )
    set(options1arg
        )
    set(optionsnarg
        VARS
        EXCLUDE
        )
    cmake_parse_arguments(_epgfa "${options0arg}" "${options1arg}" "${optionsnarg}" ${ARGN})
    if(NOT _epfga_NO_DEFAULTS)
        ExternalProject_GetFwdVarNames(_fwd_names)
    endif()
    if(_epgfa_VARS)
        list(APPEND _fwd_names ${_epgfa_VARS})
    endif()
    if(_epgfa_EXCLUDE)
        list(REMOVE_ITEM _fwd_names ${_epgfa_EXCLUDE})
    endif()
    set(_epgfa_args)
    foreach(_f ${_fwd_names})
        if(${_f})
            list(APPEND _epgfa_args -D${_f}=${${_f}})
            if(NOT _epfga_QUIET)
                message(STATUS "ExternalProject_GetFwdArgs: ${_f}=${${_f}}")
            endif()
        endif()
    endforeach()
    set(${output_var} "${_epgfa_args}" PARENT_SCOPE)
endfunction(ExternalProject_GetFwdArgs)


#------------------------------------------------------------------------------
# Gets a default list with the names of variables to forward to an
# external project. This function creates a list of common cmake
# variable names which have an impact in the output binaries or their
# placement.
function(ExternalProject_GetFwdVarNames output_var)
    # these common names are irrespective of build type
    set(names
        CMAKE_GENERATOR
        CMAKE_INSTALL_PREFIX
        CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        CMAKE_LIBRARY_OUTPUT_DIRECTORY
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        CMAKE_AR
        CMAKE_BUILD_TYPE
        CMAKE_INCLUDE_PATH
        CMAKE_LIBRARY_PATH
        #CMAKE_MODULE_PATH # this is dangerous as it can override the external project's build files.
        CMAKE_PREFIX_PATH
        BUILD_SHARED_LIBS
        CMAKE_CXX_COMPILER
        CMAKE_C_COMPILER
        CMAKE_LINKER
        CMAKE_MAKE_PROGRAM
        CMAKE_NM
        CMAKE_OBJCOPY
        CMAKE_RANLIB
        CMAKE_STRIP
        CMAKE_TOOLCHAIN_FILE
        #CMAKE_CONFIGURATION_TYPES # not this. external projects will have their own build configurations
        )
    # these names have per-build type values;
    # use CMAKE_CONFIGURATION_TYPES to construct the list
    foreach(v
            CMAKE_CXX_FLAGS
            CMAKE_C_FLAGS
            CMAKE_EXE_LINKER_FLAGS
            CMAKE_MODULE_LINKER_FLAGS
            CMAKE_SHARED_LINKER_FLAGS)
        list(APPEND names ${v})
        foreach(t ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${t} u)
            list(APPEND names ${v}_${u})
        endforeach()
    endforeach()
    set(${output_var} "${names}" PARENT_SCOPE)
endfunction(ExternalProject_GetFwdVarNames)


#------------------------------------------------------------------------------
macro(ExternalProject_Import name)
    set(options0arg
        )
    set(options1arg
        PREFIX        # look only here when findind
        )
    set(optionsnarg
        INCLUDE_PATHS  # use these dirs for searching includes
        LIBRARY_PATHS  # use these dirs for searching libraries
        INCLUDES       # find these includes and append them to ${name}_INCLUDE_DIRS
        INCLUDE_DIR_SUFFIXES
        LIBRARIES      # find these libs and append them to ${name}_LIBRARIES
        LIBRARY_DIR_SUFFIXES
        )
    cmake_parse_arguments(_eep "${options0arg}" "${options1arg}" "${optionsnarg}" ${ARGN})

    if(NOT _eep_PREFIX)
        message(FATAL_ERROR "no prefix was given")
    endif()

    include(FindPackageHandleStandardArgs)

    #----------------------------------------------------------------
    # includes

    # the list of paths to search for includes
    set(_eep_ipaths ${_eep_PREFIX})
    foreach(_eep_i ${_eep_INCLUDE_DIRS})
        list(APPEND _eep_ipaths ${__eep_PREFIX}/${_eep_i})
    endforeach()

    # find the includes that were asked for, and add
    # their paths to the includes list
    set(_eep_idirs)
    foreach(_eep_i ${_eep_INCLUDES})
        find_path(_eep_path_${_eep_i} ${_eep_i}
            PATHS ${_eep_ipaths}
            PATH_SUFFIXES include ${_eep_INCLUDE_DIR_SUFFIXES}
            NO_DEFAULT_PATH
            )
        if(NOT _eep_path_${_eep_i})
            message(FATAL_ERROR "could not find include: ${_eep_i}")
        endif()
        #message(STATUS "include: ${_eep_i} ---> ${_eep_path_${_eep_i}}")
        list(APPEND _eep_idirs ${_eep_path_${_eep_i}})
        find_package_handle_standard_args(${_eep_i}_INCLUDE_DIR DEFAULT_MSG _eep_path_${_eep_i})
    endforeach()
    if(_eep_idirs)
        list(REMOVE_DUPLICATES _eep_idirs)
    endif()

    # save the include list
    set(${name}_INCLUDE_DIRS "${_eep_idirs}" CACHE STRING "" FORCE)

    #----------------------------------------------------------------
    # libraries

    # the list of paths to search for libraries
    set(_eep_lpaths ${_eep_PREFIX})
    foreach(_eep_i ${_eep_LIBRARIES})
        list(APPEND _eep_lpaths ${__eep_PREFIX}/${_eep_i})
    endforeach()

    # find any libraries that were asked for
    set(_eep_libs)
    foreach(_eep_i ${_eep_LIBRARIES})
        find_library(_eep_lib_${_eep_i} ${_eep_i}
            PATHS ${_eep_lpaths}
            PATH_SUFFIXES lib ${_eep_LIBRARY_DIR_SUFFIXES}
            NO_DEFAULT_PATH
            )
        if(NOT _eep_lib_${_eep_i})
            message(FATAL_ERROR "could not find library: ${_eep_i}")
        endif()
        #message(STATUS "lib: ${_eep_i} ---> ${_eep_lib_${_eep_i}}")
        list(APPEND _eep_libs ${_eep_lib_${_eep_i}})
        find_package_handle_standard_args(${_eep_i}_LIBRARY DEFAULT_MSG _eep_lib_${_eep_i})
    endforeach()

    # save the include list
    set(${name}_LIBRARIES ${_eep_libs} CACHE STRING "")

endmacro(ExternalProject_Import)
