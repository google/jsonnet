

# this function works both with multiconfig and single-config generators.
function(set_default_build_type which)
    # CMAKE_CONFIGURATION_TYPES is available only for multiconfig generators.
    # so set the build type only if CMAKE_CONFIGURATION_TYPES does not exist.
    if(NOT CMAKE_CONFIGURATION_TYPES) # not a multiconfig generator?
        if(NOT CMAKE_BUILD_TYPE)
            if(NOT which)
                set(which RelWithDebInfo)
            endif()
            message("Defaulting to ${which} build.")
            set(CMAKE_BUILD_TYPE ${which} CACHE STRING "")
        endif()
    endif()
endfunction()


# https://stackoverflow.com/questions/31546278/where-to-set-cmake-configuration-types-in-a-project-with-subprojects
function(setup_configuration_types)
    set(options0arg
    )
    set(options1arg
        DEFAULT
    )
    set(optionsnarg
        TYPES
    )
    cmake_parse_arguments("" "${options0arg}" "${options1arg}" "${optionsnarg}" ${ARGN})

    if(NOT TYPES)
        set(TYPES Release Debug RelWithDebInfo MinSizeRel)
    endif()

    # make it safe to call repeatedly
    if(NOT _setup_configuration_types_done)
        set(_setup_configuration_types_done 1 CACHE INTERNAL "")

        # No reason to set CMAKE_CONFIGURATION_TYPES if it's not a multiconfig generator
        # Also no reason mess with CMAKE_BUILD_TYPE if it's a multiconfig generator.

        if(CMAKE_CONFIGURATION_TYPES) # multiconfig generator?
            set(CMAKE_CONFIGURATION_TYPES "${TYPES}" CACHE STRING "")
        else() # single-config generator
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build")
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${TYPES}")
            # set the valid options for cmake-gui drop-down list
        endif()
    endif()
endfunction()


# https://stackoverflow.com/questions/31546278/where-to-set-cmake-configuration-types-in-a-project-with-subprojects
function(add_configuration_type name)
    set(flag_vars
        C_FLAGS
        CXX_FLAGS
        SHARED_LINKER_FLAGS
        STATIC_LINKER_FLAGS
        MODULE_LINKER_FLAGS
        EXE_LINKER_FLAGS
        RC_FLAGS
    )

    set(options0arg
        PREPEND  # when defaulting to a config, prepend to it instead of appending to it
        SET_MAIN_FLAGS # eg, set CMAKE_CXX_FLAGS from CMAKE_CXX_FLAGS_${name}
    )
    set(options1arg
        DEFAULT_FROM # take the initial value of the flags from this config
    )
    set(optionsnarg
        C_FLAGS
        CXX_FLAGS
        SHARED_LINKER_FLAGS
        STATIC_LINKER_FLAGS
        MODULE_LINKER_FLAGS
        EXE_LINKER_FLAGS
        RC_FLAGS
    )
    cmake_parse_arguments(_act "${options0arg}" "${options1arg}" "${optionsnarg}" ${ARGN})

    string(TOUPPER ${name} UNAME)

    # make it safe to call repeatedly
    if(NOT _add_configuration_type_${name})
        set(_add_configuration_type_${name} 1 CACHE INTERNAL "")

        setup_configuration_types()

        if(CMAKE_CONFIGURATION_TYPES) # multiconfig generator?
            set(CMAKE_CONFIGURATION_TYPES "${CMAKE_CONFIGURATION_TYPES};${name}" CACHE STRING "" FORCE)
        else() # single-config generator
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY HELPSTRING "Choose the type of build" FORCE)
            set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "${CMAKE_BUILD_TYPES};${name}" FORCE)
            # set the valid options for cmake-gui drop-down list
        endif()

        # now set up the configuration
        message(STATUS "config: CMAKE_${f}_${UNAME} --- ${val}")
        foreach(f ${flag_vars})
            set(val ${_act_${f}})
            message(STATUS "config: ${name}: ${f} --- ${val}")
            if(_act_DEFAULT_FROM)
                if(_act_PREPEND)
                    set(val "${val} ${CMAKE_${f}_${_act_DEFAULT_FROM}}")
                else()
                    set(val "${CMAKE_${f}_${_act_DEFAULT_FROM}} ${val}")
                endif()
            endif()
            message(STATUS "config: CMAKE_${f}_${UNAME} --- ${val}")
            set(CMAKE_${f}_${UNAME} "${val}" CACHE STRING "" FORCE)
            mark_as_advanced(CMAKE_${f}_${UNAME})
            if(_act_SET_MAIN_FLAGS)
                set(CMAKE_${f} "${CMAKE_${f}_${UNAME}}" CACHE STRING "" FORCE)
            endif()
        endforeach()
    endif()

endfunction()
