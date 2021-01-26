function(_c4_doxy_list_to_str var)
    set(il)
    foreach(i ${${var}})
        if("${il}" STREQUAL "")
            set(il "${i}")
        else()
            set(il "${il} ${i}")
        endif()
    endforeach()
    set(${var} "${il}" PARENT_SCOPE)
endfunction()

string(REPLACE " " ";" ALLVARS ${ALLVARS})
string(REPLACE " " ";" LISTVARS ${LISTVARS})

foreach(var ${LISTVARS})
    _c4_doxy_list_to_str(${var})
endforeach()

foreach(var ${ALLVARS})
    message(STATUS "${var}='${${var}}'")
endforeach()

configure_file(${DOXYFILE_IN} ${DOXYFILE_OUT} @ONLY)
