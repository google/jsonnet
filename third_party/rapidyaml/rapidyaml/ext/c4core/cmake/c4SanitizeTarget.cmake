# (C) 2017 Joao Paulo Magalhaes <dev@jpmag.me>
if(NOT _c4_sanitize_target_included)
set(_c4_sanitize_target_included ON)

include(CMakeDependentOption)
include(PrintVar)

function(_c4_default_if_not_set var dft)
    if("${${var}}" STREQUAL "")
        option(${var} "" ${dft})
    endif()
endfunction()

#------------------------------------------------------------------------------
function(c4_setup_sanitize umbrella_option)
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Coverage")
        return()
    endif()
    if(NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang"))
        return()
    endif()

    _c4_default_if_not_set(C4_SANITIZE ON)
    _c4_default_if_not_set(C4_SANITIZE_ONLY OFF)
    _c4_default_if_not_set(C4_ASAN ON)
    _c4_default_if_not_set(C4_TSAN ON)
    _c4_default_if_not_set(C4_MSAN ON)
    _c4_default_if_not_set(C4_UBSAN ON)

    cmake_dependent_option(${_c4_uprefix}SANITIZE "turn on clang sanitizer targets" ${C4_SANITIZE} ${umbrella_option} OFF)
    cmake_dependent_option(${_c4_uprefix}SANITIZE_ONLY "compile only sanitize targets (not the regular unsanitized targets)" ${C4_SANITIZE_ONLY} ${umbrella_option} OFF)

    # options for individual sanitizers - contingent on sanitize on/off
    cmake_dependent_option(${_c4_uprefix}ASAN  "" ${C4_ASAN} "${_c4_uprefix}SANITIZE" OFF)
    cmake_dependent_option(${_c4_uprefix}TSAN  "" ${C4_TSAN} "${_c4_uprefix}SANITIZE" OFF)
    cmake_dependent_option(${_c4_uprefix}MSAN  "" ${C4_MSAN} "${_c4_uprefix}SANITIZE" OFF)
    cmake_dependent_option(${_c4_uprefix}UBSAN "" ${C4_UBSAN} "${_c4_uprefix}SANITIZE" OFF)

    if(${_c4_uprefix}SANITIZE)
        string(REGEX REPLACE "([0-9]+\\.[0-9]+).*" "\\1" LLVM_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
        find_program(LLVM_SYMBOLIZER llvm-symbolizer
            NAMES llvm-symbolizer-${LLVM_VERSION} llvm-symbolizer
            DOC "symbolizer to use in sanitize tools")
        if(NOT LLVM_SYMBOLIZER)
            string(REGEX REPLACE "([0-9]+)\\.[0-9]+.*" "\\1" LLVM_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
            find_program(LLVM_SYMBOLIZER llvm-symbolizer
                NAMES llvm-symbolizer-${LLVM_VERSION} llvm-symbolizer
                DOC "symbolizer to use in sanitize tools")
            if(NOT LLVM_SYMBOLIZER)
                message(FATAL_ERROR "could not find symbolizer. LLVM_VERSION=${LLVM_VERSION}")
            endif()
        endif()

        set(ss) # string to report enabled sanitizers

        if(${_c4_uprefix}ASAN)
            set(ss "asan")
            set(${_c4_uprefix}ASAN_CFLAGS "-O1 -g -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls" CACHE STRING "compile flags for clang address sanitizer: https://clang.llvm.org/docs/AddressSanitizer.html")
            set(${_c4_uprefix}ASAN_LFLAGS "-g -fsanitize=address" CACHE STRING "linker flags for clang address sanitizer: https://clang.llvm.org/docs/AddressSanitizer.html")
            set(${_c4_uprefix}ASAN_RENV  "env ASAN_SYMBOLIZER_PATH=${LLVM_SYMBOLIZER} ASAN_OPTIONS=symbolize=1" CACHE STRING "run environment for clang address sanitizer: https://clang.llvm.org/docs/AddressSanitizer.html")
            # the flags are strings; we need to separate them into a list
            # to prevent cmake from quoting them when passing to the targets
            separate_arguments(${_c4_uprefix}ASAN_CFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}ASAN_CFLAGS})
            separate_arguments(${_c4_uprefix}ASAN_LFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}ASAN_LFLAGS})
        endif()

        if(${_c4_uprefix}TSAN)
            set(ss "${ss} tsan")
            set(${_c4_uprefix}TSAN_CFLAGS "-O1 -g -fsanitize=thread -fno-omit-frame-pointer" CACHE STRING "compile flags for clang thread sanitizer: https://clang.llvm.org/docs/ThreadSanitizer.html")
            set(${_c4_uprefix}TSAN_LFLAGS "-g -fsanitize=thread" CACHE STRING "linker flags for clang thread sanitizer: https://clang.llvm.org/docs/ThreadSanitizer.html")
            set(${_c4_uprefix}TSAN_RENV  "env TSAN_SYMBOLIZER_PATH=${LLVM_SYMBOLIZER} TSAN_OPTIONS=symbolize=1" CACHE STRING "run environment for clang thread sanitizer: https://clang.llvm.org/docs/ThreadSanitizer.html")
            separate_arguments(${_c4_uprefix}TSAN_CFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}TSAN_CFLAGS})
            separate_arguments(${_c4_uprefix}TSAN_LFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}TSAN_LFLAGS})
        endif()

        if(${_c4_uprefix}MSAN)
            set(ss "${ss} msan")
            set(${_c4_uprefix}MSAN_CFLAGS "-O1 -g -fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -fno-optimize-sibling-calls" CACHE STRING "compile flags for clang memory sanitizer: https://clang.llvm.org/docs/MemorySanitizer.html")
            set(${_c4_uprefix}MSAN_LFLAGS "-g -fsanitize=memory" CACHE STRING "linker flags for clang memory sanitizer: https://clang.llvm.org/docs/MemorySanitizer.html")
            set(${_c4_uprefix}MSAN_RENV  "env MSAN_SYMBOLIZER_PATH=${LLVM_SYMBOLIZER} MSAN_OPTIONS=symbolize=1" CACHE STRING "run environment for clang memory sanitizer: https://clang.llvm.org/docs/MemorySanitizer.html")
            separate_arguments(${_c4_uprefix}MSAN_CFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}MSAN_CFLAGS})
            separate_arguments(${_c4_uprefix}MSAN_LFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}MSAN_LFLAGS})
        endif()

        if(${_c4_uprefix}UBSAN)
            set(ss "${ss} ubsan")
            set(${_c4_uprefix}UBSAN_CFLAGS "-g -fsanitize=undefined" CACHE STRING "compile flags for clang undefined behaviour sanitizer: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html")
            set(${_c4_uprefix}UBSAN_LFLAGS "-g -fsanitize=undefined" CACHE STRING "linker flags for clang undefined behaviour sanitizer: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html")
            set(${_c4_uprefix}UBSAN_RENV "env UBSAN_SYMBOLIZER_PATH=${LLVM_SYMBOLIZER} UBSAN_OPTIONS='symbolize=1 print_stacktrace=1'" CACHE STRING "run environment for clang undefined behaviour sanitizer: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html")
            separate_arguments(${_c4_uprefix}UBSAN_CFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}UBSAN_CFLAGS})
            separate_arguments(${_c4_uprefix}UBSAN_LFLAGS_SEP UNIX_COMMAND ${${_c4_uprefix}UBSAN_LFLAGS})
        endif()

        c4_dbg("enabled clang sanitizers: ${ss}")
    endif() # ${_c4_uprefix}SANITIZE

endfunction()

#------------------------------------------------------------------------------
function(c4_sanitize_get_target_command name which_sanitizer_ output)
    string(TOUPPER ${which_sanitizer_} which_sanitizer)
    if("${which_sanitizer}" STREQUAL ASAN)
    elseif("${which_sanitizer}" STREQUAL TSAN)
    elseif("${which_sanitizer}" STREQUAL MSAN)
    elseif("${which_sanitizer}" STREQUAL UBSAN)
    else()
        message(FATAL_ERROR "the sanitizer must be one of: ASAN, TSAN, MSAN, UBSAN")
    endif()
    separate_arguments(cmd UNIX_COMMAND "${${_c4_uprefix}${which_sanitizer}_RENV} ${name}")
    set(${output} ${cmd} PARENT_SCOPE)
endfunction()

function(_sanitize_set_target_folder tgt folder)
    if(folder)
        set_target_properties(${tgt} PROPERTIES FOLDER "${folder}")
    endif()
endfunction()

#------------------------------------------------------------------------------
function(c4_sanitize_target name)
    set(opt0arg
        LIBRARY
        EXECUTABLE
    )
    set(opt1arg
        OUTPUT_TARGET_NAMES
        FOLDER
    )
    set(optnarg
        SOURCES
        INC_DIRS
        LIBS
        LIB_DIRS
    )
    cmake_parse_arguments("" "${opt0arg}" "${opt1arg}" "${optnarg}" ${ARGN})

    if((NOT _LIBRARY) AND (NOT _EXECUTABLE))
        message(FATAL_ERROR "either LIBRARY or EXECUTABLE must be specified")
    endif()

    if(${_c4_uprefix}SANITIZE AND NOT TARGET ${_c4_lprefix}sanitize)
        add_custom_target(${_c4_lprefix}sanitize)
        _sanitize_set_target_folder(${_c4_lprefix}sanitize "${_FOLDER}")
    endif()
    if(${_c4_uprefix}ASAN AND NOT TARGET ${_c4_lprefix}asan-all)
        add_custom_target(${_c4_lprefix}asan-all)
        add_dependencies(${_c4_lprefix}sanitize ${_c4_lprefix}asan-all)
        _sanitize_set_target_folder(${_c4_lprefix}asan-all "${_FOLDER}")
    endif()
    if(${_c4_uprefix}MSAN AND NOT TARGET ${_c4_lprefix}msan-all)
        add_custom_target(${_c4_lprefix}msan-all)
        add_dependencies(${_c4_lprefix}sanitize ${_c4_lprefix}msan-all)
        _sanitize_set_target_folder(${_c4_lprefix}msan-all "${_FOLDER}")
    endif()
    if(${_c4_uprefix}TSAN AND NOT TARGET ${_c4_lprefix}tsan-all)
        add_custom_target(${_c4_lprefix}tsan-all)
        add_dependencies(${_c4_lprefix}sanitize ${_c4_lprefix}tsan-all)
        _sanitize_set_target_folder(${_c4_lprefix}tsan-all "${_FOLDER}")
    endif()
    if(${_c4_uprefix}UBSAN AND NOT TARGET ${_c4_lprefix}ubsan-all)
        add_custom_target(${_c4_lprefix}ubsan-all)
        add_dependencies(${_c4_lprefix}sanitize ${_c4_lprefix}ubsan-all)
        _sanitize_set_target_folder(${_c4_lprefix}ubsan-all "${_FOLDER}")
    endif()

    if(${_c4_uprefix}ASAN OR ${_c4_uprefix}MSAN OR ${_c4_uprefix}TSAN OR ${_c4_uprefix}UBSAN)
        add_custom_target(${name}-sanitize-all)
        _sanitize_set_target_folder(${name}-sanitize-all "${_FOLDER}")
    endif()

    set(targets)

    # https://clang.llvm.org/docs/AddressSanitizer.html
    if(${_c4_uprefix}ASAN)
        if(${_LIBRARY})
            add_library(${name}-asan EXCLUDE_FROM_ALL ${_SOURCES})
        elseif(${_EXECUTABLE})
            add_executable(${name}-asan EXCLUDE_FROM_ALL ${_SOURCES})
        endif()
        _sanitize_set_target_folder(${name}-asan "${_FOLDER}")
        list(APPEND targets ${name}-asan)
        target_include_directories(${name}-asan PUBLIC ${_INC_DIRS})
        set(_real_libs)
        foreach(_l ${_LIBS})
            if(TARGET ${_l}-asan)
                list(APPEND _real_libs ${_l}-asan)
            else()
                list(APPEND _real_libs ${_l})
            endif()
        endforeach()
        target_link_libraries(${name}-asan PUBLIC ${_real_libs})
        target_compile_options(${name}-asan PUBLIC ${${_c4_uprefix}ASAN_CFLAGS_SEP})
        # http://stackoverflow.com/questions/25043458/does-cmake-have-something-like-target-link-options
        target_link_libraries(${name}-asan PUBLIC ${${_c4_uprefix}ASAN_LFLAGS_SEP})
        add_dependencies(${_c4_lprefix}asan-all ${name}-asan)
        add_dependencies(${name}-sanitize-all ${name}-asan)
    endif()

    # https://clang.llvm.org/docs/ThreadSanitizer.html
    if(${_c4_uprefix}TSAN)
        if(${_LIBRARY})
            add_library(${name}-tsan EXCLUDE_FROM_ALL ${_SOURCES})
        elseif(${_EXECUTABLE})
            add_executable(${name}-tsan EXCLUDE_FROM_ALL ${_SOURCES})
        endif()
        _sanitize_set_target_folder(${name}-tsan "${_FOLDER}")
        list(APPEND targets ${name}-tsan)
        target_include_directories(${name}-tsan PUBLIC ${_INC_DIRS})
        set(_real_libs)
        foreach(_l ${_LIBS})
            if(TARGET ${_l}-tsan)
                list(APPEND _real_libs ${_l}-tsan)
            else()
                list(APPEND _real_libs ${_l})
            endif()
        endforeach()
        target_link_libraries(${name}-tsan PUBLIC ${_real_libs})
        target_compile_options(${name}-tsan PUBLIC ${${_c4_uprefix}TSAN_CFLAGS_SEP})
        # http://stackoverflow.com/questions/25043458/does-cmake-have-something-like-target-link-options
        target_link_libraries(${name}-tsan PUBLIC ${${_c4_uprefix}TSAN_LFLAGS_SEP})
        add_dependencies(${_c4_lprefix}tsan-all ${name}-tsan)
        add_dependencies(${name}-sanitize-all ${name}-tsan)
    endif()

    # https://clang.llvm.org/docs/MemorySanitizer.html
    if(${_c4_uprefix}MSAN)
        if(${_LIBRARY})
            add_library(${name}-msan EXCLUDE_FROM_ALL ${_SOURCES})
        elseif(${_EXECUTABLE})
            add_executable(${name}-msan EXCLUDE_FROM_ALL ${_SOURCES})
        endif()
        _sanitize_set_target_folder(${name}-msan "${_FOLDER}")
        list(APPEND targets ${name}-msan)
        target_include_directories(${name}-msan PUBLIC ${_INC_DIRS})
        set(_real_libs)
        foreach(_l ${_LIBS})
            if(TARGET ${_l}-msan)
                list(APPEND _real_libs ${_l}-msan)
            else()
                list(APPEND _real_libs ${_l})
            endif()
        endforeach()
        target_link_libraries(${name}-msan PUBLIC ${_real_libs})
        target_compile_options(${name}-msan PUBLIC ${${_c4_uprefix}MSAN_CFLAGS_SEP})
        # http://stackoverflow.com/questions/25043458/does-cmake-have-something-like-target-link-options
        target_link_libraries(${name}-msan PUBLIC ${${_c4_uprefix}MSAN_LFLAGS_SEP})
        add_dependencies(${_c4_lprefix}msan-all ${name}-msan)
        add_dependencies(${name}-sanitize-all ${name}-msan)
    endif()

    # https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
    if(${_c4_uprefix}UBSAN)
        if(${_LIBRARY})
            add_library(${name}-ubsan EXCLUDE_FROM_ALL ${_SOURCES})
        elseif(${_EXECUTABLE})
            add_executable(${name}-ubsan EXCLUDE_FROM_ALL ${_SOURCES})
        endif()
        _sanitize_set_target_folder(${name}-ubsan "${_FOLDER}")
        list(APPEND targets ${name}-ubsan)
        target_include_directories(${name}-ubsan PUBLIC ${_INC_DIRS})
        set(_real_libs)
        foreach(_l ${_LIBS})
            if(TARGET ${_l}-ubsan)
                list(APPEND _real_libs ${_l}-ubsan)
            else()
                list(APPEND _real_libs ${_l})
            endif()
        endforeach()
        target_link_libraries(${name}-ubsan PUBLIC ${_real_libs})
        target_compile_options(${name}-ubsan PUBLIC ${${_c4_uprefix}UBSAN_CFLAGS_SEP})
        # http://stackoverflow.com/questions/25043458/does-cmake-have-something-like-target-link-options
        target_link_libraries(${name}-ubsan PUBLIC ${${_c4_uprefix}UBSAN_LFLAGS_SEP})
        add_dependencies(${_c4_lprefix}ubsan-all ${name}-ubsan)
        add_dependencies(${name}-sanitize-all ${name}-ubsan)
    endif()

    if(_OUTPUT_TARGET_NAMES)
        set(${_OUTPUT_TARGET_NAMES} ${targets} PARENT_SCOPE)
    endif()
endfunction()

endif(NOT _c4_sanitize_target_included)
