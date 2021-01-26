
function(_c4_intersperse_with_flag outvar flag)
    if(MSVC AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") # it may be clang as well
        set(f "/${flag}")
    else()
        set(f "-${flag}")
    endif()
    set(out)
    foreach(i ${ARGN})
        if(NOT "${i}" STREQUAL "")
            set(out "${out} ${f} '${i}'")

            # ... Following this are several unsuccessful attempts to make
            # sure that an empty generator expression passed as part of the
            # arguments won't be expanded to nothing between successive
            # flags.  For example, -I /some/include -I -I /other/include,
            # which is wrong as it misses an empty quote.  This causes
            # clang-tidy in particular to fail.  Maybe this is happening
            # because the result is passed to separate_arguments() which
            # prevents the lists from being evaluated correctly. Also, note
            # that add_custom_target() has the following options which may
            # help: COMMAND_EXPAND_LISTS and VERBATIM.

            # Anyway -- for now it is working, but maybe the generator
            # expression approach turns out to work while being much cleaner
            # than the current approach.

            #set(c $<GENEX_EVAL,$<BOOL:${i}>>)
            #set(c $<BOOL:${i}>)  # i may be a generator expression the evaluates to empty
            #set(s "${f} ${i}")
            #set(e "${f} aaaaaaWTF")
            #list(APPEND out $<IF:${c},${s},${e}>)
            #list(APPEND out $<${c},${s}>)
            #list(APPEND out $<GENEX_EVAL:${c},${s}>)
            #list(APPEND out $<TARGET_GENEX_EVAL:${tgt},${c},${s}>)
        endif()
    endforeach()
    ## https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html#string-valued-generator-expressions
    #if(ARGN)
    #    set(out "${f}$<JOIN:${ARGN},;${f}>")
    #endif()
    set(${outvar} ${out} PARENT_SCOPE)
endfunction()

function(c4_get_define_flags outvar)
    _c4_intersperse_with_flag(out D ${ARGN})
    set(${outvar} ${out} PARENT_SCOPE)
endfunction()

function(c4_get_include_flags outvar)
    _c4_intersperse_with_flag(out I ${ARGN})
    set(${outvar} ${out} PARENT_SCOPE)
endfunction()
