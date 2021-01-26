if(NOT _c4_GTPR_included)
set(_c4_GTPR_included ON)

function(c4_get_target_property_recursive outputvar target property)
    #
    # helps for debugging
    if(_stack)
        set(_stack "${_stack}/${target}")
    else()
        set(_stack "${property}:${target}")
    endif()
    #
    # what type of target is this?
    get_target_property(_rec_target_type ${target} TYPE)
    c4_dbg("${_stack} [type=${_rec_target_type}]: get property ${property}")
    #
    # adjust the property names for interface targets
    set(_ept_prop_ll LINK_LIBRARIES)
    if(_rec_target_type STREQUAL "INTERFACE_LIBRARY")
        set(_ept_prop_ll INTERFACE_LINK_LIBRARIES)
        if(property STREQUAL "INCLUDE_DIRECTORIES")
            c4_dbg("${_stack} [type=${_rec_target_type}]: property ${property} ---> INTERFACE_INCLUDE_DIRECTORIES")
            set(property INTERFACE_INCLUDE_DIRECTORIES)
        elseif(property STREQUAL "LINK_LIBRARIES")
            c4_dbg("${_stack} [type=${_rec_target_type}]: property ${property} ---> INTERFACE_LINK_LIBRARIES")
            set(property INTERFACE_LINK_LIBRARIES)
        endif()
    endif()
    #
    get_target_property(_ept_li ${target} ${property})
    c4_dbg("${_stack} [type=${_rec_target_type}]: property ${property}=${_ept_li}")
    if(NOT _ept_li)  # the property may not be set (ie foo-NOTFOUND)
        set(_ept_li) # so clear it in that case
    endif()
    #
    # now descend and append the property for each of the linked libraries
    get_target_property(_ept_deps ${target} ${_ept_prop_ll})
    if(_ept_deps)
        foreach(_ept_ll ${_ept_deps})
            if(TARGET ${_ept_ll})
                c4_get_target_property_recursive(_ept_out ${_ept_ll} ${property})
                list(APPEND _ept_li ${_ept_out})
            endif()
        endforeach()
    endif()
    #
    foreach(le_ ${_ept_li})
        string(STRIP "${le_}" le)
        if(NOT le)
        elseif("${le}" STREQUAL "")
        else()
            list(APPEND _ept_li_f ${le})
        endif()
    endforeach()
    c4_dbg("${_stack} [type=${_rec_target_type}]: final=${_ept_li_f}")
    set(${outputvar} ${_ept_li_f} PARENT_SCOPE)
endfunction()


#------------------------------------------------------------------------------
#------------------------------------------------------------------------------
#------------------------------------------------------------------------------


function(c4_set_transitive_property target prop_name prop_value)
    set_target_properties(${target} PROPERTIES "${prop_name}" "${prop_value}")
endfunction()


function(c4_append_transitive_property target prop_name prop_value)
    get_target_property(curr_value ${target} "${prop_name}")
    if(curr_value)
        list(APPEND curr_value "${prop_value}")
    else()
        set(curr_value "${prop_value}")
    endif()
    c4_set_transitive_property(${target} "${prop_name}" "${curr_value}")
endfunction()


# TODO: maybe we can use c4_get_target_property_recursive()?
function(c4_get_transitive_property target prop_name out)
    if(NOT TARGET ${target})
        return()
    endif()
    # these will be the names of the variables we'll use to cache the result
    set(_trval _C4_TRANSITIVE_${prop_name})
    set(_trmark _C4_TRANSITIVE_${prop_name}_DONE)
    #
    get_target_property(cached ${target} ${_trmark})  # is it cached already
    if(cached)
        get_target_property(p ${target} _C4_TRANSITIVE_${prop_name})
        set(${out} ${p} PARENT_SCOPE)
        #c4_dbg("${target}: c4_get_transitive_property ${target} ${prop_name}: cached='${p}'")
    else()
        #c4_dbg("${target}: gathering transitive property: ${prop_name}...")
        set(interleaved)
        get_target_property(lv ${target} ${prop_name})
        if(lv)
            list(APPEND interleaved ${lv})
        endif()
        c4_get_transitive_libraries(${target} LINK_LIBRARIES libs)
        c4_get_transitive_libraries(${target} INTERFACE_LINK_LIBRARIES ilibs)
        list(APPEND libs ${ilibs})
        foreach(lib ${libs})
            #c4_dbg("${target}: considering ${lib}...")
            if(NOT lib)
                #c4_dbg("${target}: considering ${lib}: not found, skipping...")
                continue()
            endif()
            if(NOT TARGET ${lib})
                #c4_dbg("${target}: considering ${lib}: not a target, skipping...")
                continue()
            endif()
            get_target_property(lv ${lib} ${prop_name})
            if(lv)
                list(APPEND interleaved ${lv})
            endif()
            c4_get_transitive_property(${lib} ${prop_name} v)
            if(v)
                list(APPEND interleaved ${v})
            endif()
            #c4_dbg("${target}: considering ${lib}---${interleaved}")
        endforeach()
        #c4_dbg("${target}: gathering transitive property: ${prop_name}: ${interleaved}")
        set(${out} ${interleaved} PARENT_SCOPE)
        set_target_properties(${target} PROPERTIES
            ${_trmark} ON
            ${_trval} "${interleaved}")
    endif()
endfunction()


function(c4_get_transitive_libraries target prop_name out)
    if(NOT TARGET ${target})
        return()
    endif()
    # these will be the names of the variables we'll use to cache the result
    set(_trval _C4_TRANSITIVE_${prop_name})
    set(_trmark _C4_TRANSITIVE_${prop_name}_DONE)
    #
    get_target_property(cached ${target} ${_trmark})
    if(cached)
        get_target_property(p ${target} ${_trval})
        set(${out} ${p} PARENT_SCOPE)
        #c4_dbg("${target}: c4_get_transitive_libraries ${target} ${prop_name}: cached='${p}'")
    else()
        #c4_dbg("${target}: gathering transitive libraries: ${prop_name}...")
        get_target_property(target_type ${target} TYPE)
        set(interleaved)
        if(NOT ("${target_type}" STREQUAL "INTERFACE_LIBRARY") AND "${prop_name}" STREQUAL LINK_LIBRARIES)
            get_target_property(l ${target} ${prop_name})
            foreach(ll ${l})
                #c4_dbg("${target}: considering ${ll}...")
                if(NOT ll)
                    #c4_dbg("${target}: considering ${ll}: not found, skipping...")
                    continue()
                endif()
                if(NOT ll)
                    #c4_dbg("${target}: considering ${ll}: not a target, skipping...")
                    continue()
                endif()
                list(APPEND interleaved ${ll})
                c4_get_transitive_libraries(${ll} ${prop_name} v)
                if(v)
                    list(APPEND interleaved ${v})
                endif()
                #c4_dbg("${target}: considering ${ll}---${interleaved}")
            endforeach()
        endif()
        #c4_dbg("${target}: gathering transitive libraries: ${prop_name}: result='${interleaved}'")
        set(${out} ${interleaved} PARENT_SCOPE)
        set_target_properties(${target} PROPERTIES
            ${_trmark} ON
            ${_trval} "${interleaved}")
    endif()
endfunction()

endif(NOT _c4_GTPR_included)
