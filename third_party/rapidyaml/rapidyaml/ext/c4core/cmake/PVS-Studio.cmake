# 2006-2008 (c) Viva64.com Team
# 2008-2016 (c) OOO "Program Verification Systems"
#
# Version 2

function (pvs_studio_relative_path VAR ROOT FILEPATH)
    set("${VAR}" "${FILEPATH}" PARENT_SCOPE)
    if ("${FILEPATH}" MATCHES "^/.*$")
        file(RELATIVE_PATH RPATH "${ROOT}" "${FILEPATH}")
        if (NOT "${RPATH}" MATCHES "^\\.\\..*$")
            set("${VAR}" "${RPATH}" PARENT_SCOPE)
        endif ()
    endif ()
endfunction ()

function (pvs_studio_join_path VAR DIR1 DIR2)
    if ("${DIR2}" MATCHES "^(/|~).*$" OR "${DIR1}" STREQUAL "")
        set("${VAR}" "${DIR2}" PARENT_SCOPE)
    else ()
        set("${VAR}" "${DIR1}/${DIR2}" PARENT_SCOPE)
    endif ()
endfunction ()

macro (pvs_studio_append_flags_from_property CXX C DIR PREFIX)
    if (NOT "${PROPERTY}" STREQUAL "NOTFOUND" AND NOT "${PROPERTY}" STREQUAL "PROPERTY-NOTFOUND")
        foreach (PROP ${PROPERTY})
            pvs_studio_join_path(PROP "${DIR}" "${PROP}")
            list(APPEND "${CXX}" "${PREFIX}${PROP}")
            list(APPEND "${C}" "${PREFIX}${PROP}")
        endforeach ()
    endif ()
endmacro ()

macro (pvs_studio_append_standard_flag FLAGS STANDARD)
    if ("${STANDARD}" MATCHES "^(99|11|14|17)$")
        if ("${PVS_STUDIO_PREPROCESSOR}" MATCHES "gcc|clang")
            list(APPEND "${FLAGS}" "-std=c++${STANDARD}")
        endif ()
    endif ()
endmacro ()

function (pvs_studio_set_directory_flags DIRECTORY CXX C)
    set(CXX_FLAGS "${${CXX}}")
    set(C_FLAGS "${${C}}")

    get_directory_property(PROPERTY DIRECTORY "${DIRECTORY}" INCLUDE_DIRECTORIES)
    pvs_studio_append_flags_from_property(CXX_FLAGS C_FLAGS "${DIRECTORY}" "-I")

    get_directory_property(PROPERTY DIRECTORY "${DIRECTORY}" COMPILE_DEFINITIONS)
    pvs_studio_append_flags_from_property(CXX_FLAGS C_FLAGS "" "-D")

    set("${CXX}" "${CXX_FLAGS}" PARENT_SCOPE)
    set("${C}" "${C_FLAGS}" PARENT_SCOPE)
endfunction ()

function (pvs_studio_set_target_flags TARGET CXX C)
    set(CXX_FLAGS "${${CXX}}")
    set(C_FLAGS "${${C}}")

    get_target_property(PROPERTY "${TARGET}" INCLUDE_DIRECTORIES)
    pvs_studio_append_flags_from_property(CXX_FLAGS C_FLAGS "${DIRECTORY}" "-I")

    get_target_property(PROPERTY "${TARGET}" COMPILE_DEFINITIONS)
    pvs_studio_append_flags_from_property(CXX_FLAGS C_FLAGS "" "-D")

    get_target_property(PROPERTY "${TARGET}" CXX_STANDARD)
    pvs_studio_append_standard_flag(CXX_FLAGS "${PROPERTY}")

    set("${CXX}" "${CXX_FLAGS}" PARENT_SCOPE)
    set("${C}" "${C_FLAGS}" PARENT_SCOPE)
endfunction ()

function (pvs_studio_set_source_file_flags SOURCE)
    set(LANGUAGE "")

    string(TOLOWER "${SOURCE}" SOURCE_LOWER)
    if ("${LANGUAGE}" STREQUAL "" AND "${SOURCE_LOWER}" MATCHES "^.*\\.(c|cpp|cc|cx|cxx|cp|c\\+\\+)$")
        if ("${SOURCE}" MATCHES "^.*\\.c$")
            set(LANGUAGE C)
        else ()
            set(LANGUAGE CXX)
        endif ()
    endif ()

    if ("${LANGUAGE}" STREQUAL "C")
        set(CL_PARAMS ${PVS_STUDIO_C_FLAGS} ${PVS_STUDIO_TARGET_C_FLAGS} -DPVS_STUDIO)
    elseif ("${LANGUAGE}" STREQUAL "CXX")
        set(CL_PARAMS ${PVS_STUDIO_CXX_FLAGS} ${PVS_STUDIO_TARGET_CXX_FLAGS} -DPVS_STUDIO)
    endif ()

    set(PVS_STUDIO_LANGUAGE "${LANGUAGE}" PARENT_SCOPE)
    set(PVS_STUDIO_CL_PARAMS "${CL_PARAMS}" PARENT_SCOPE)
endfunction ()

function (pvs_studio_analyze_file SOURCE SOURCE_DIR BINARY_DIR)
    set(PLOGS ${PVS_STUDIO_PLOGS})
    pvs_studio_set_source_file_flags("${SOURCE}")

    get_filename_component(SOURCE "${SOURCE}" REALPATH)
    pvs_studio_relative_path(SOURCE_RELATIVE "${SOURCE_DIR}" "${SOURCE}")
    pvs_studio_join_path(SOURCE "${SOURCE_DIR}" "${SOURCE}")

    set(LOG "${BINARY_DIR}/PVS-Studio/${SOURCE_RELATIVE}.plog")
    get_filename_component(LOG "${LOG}" REALPATH)
    get_filename_component(PARENT_DIR "${LOG}" DIRECTORY)

    if (EXISTS "${SOURCE}" AND NOT TARGET "${LOG}" AND NOT "${PVS_STUDIO_LANGUAGE}" STREQUAL "")
        add_custom_command(OUTPUT "${LOG}"
                           COMMAND mkdir -p "${PARENT_DIR}"
                           COMMAND rm -f "${LOG}"
                           COMMAND "${PVS_STUDIO_BIN}" analyze
                                                       --output-file "${LOG}"
                                                       --source-file "${SOURCE}"
                                                       ${PVS_STUDIO_ARGS}
                                                       --cl-params ${PVS_STUDIO_CL_PARAMS} "${SOURCE}"
                           WORKING_DIRECTORY "${BINARY_DIR}"
                           DEPENDS "${SOURCE}" "${PVS_STUDIO_CONFIG}"
                           VERBATIM
                           COMMENT "Analyzing ${PVS_STUDIO_LANGUAGE} file ${SOURCE_RELATIVE}")
        list(APPEND PLOGS "${LOG}")
    endif ()
    set(PVS_STUDIO_PLOGS "${PLOGS}" PARENT_SCOPE)
endfunction ()

function (pvs_studio_analyze_target TARGET DIR)
    set(PVS_STUDIO_PLOGS "${PVS_STUDIO_PLOGS}")
    set(PVS_STUDIO_TARGET_CXX_FLAGS "")
    set(PVS_STUDIO_TARGET_C_FLAGS "")

    get_target_property(PROPERTY "${TARGET}" SOURCES)
    pvs_studio_relative_path(BINARY_DIR "${CMAKE_SOURCE_DIR}" "${DIR}")
    if ("${BINARY_DIR}" MATCHES "^/.*$")
        pvs_studio_join_path(BINARY_DIR "${CMAKE_BINARY_DIR}" "PVS-Studio/__${BINARY_DIR}")
    else ()
        pvs_studio_join_path(BINARY_DIR "${CMAKE_BINARY_DIR}" "${BINARY_DIR}")
    endif ()

    file(MAKE_DIRECTORY "${BINARY_DIR}")

    pvs_studio_set_directory_flags("${DIR}" PVS_STUDIO_TARGET_CXX_FLAGS PVS_STUDIO_TARGET_C_FLAGS)
    pvs_studio_set_target_flags("${TARGET}" PVS_STUDIO_TARGET_CXX_FLAGS PVS_STUDIO_TARGET_C_FLAGS)

    if (NOT "${PROPERTY}" STREQUAL "NOTFOUND" AND NOT "${PROPERTY}" STREQUAL "PROPERTY-NOTFOUND")
        foreach (SOURCE ${PROPERTY})
            pvs_studio_join_path(SOURCE "${DIR}" "${SOURCE}")
            pvs_studio_analyze_file("${SOURCE}" "${DIR}" "${BINARY_DIR}")
        endforeach ()
    endif ()

    set(PVS_STUDIO_PLOGS "${PVS_STUDIO_PLOGS}" PARENT_SCOPE)
endfunction ()

function (pvs_studio_add_target)
    macro (default VAR VALUE)
        if ("${${VAR}}" STREQUAL "")
            set("${VAR}" "${VALUE}")
        endif ()
    endmacro ()

    set(PVS_STUDIO_SUPPORTED_PREPROCESSORS "gcc|clang")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        set(DEFAULT_PREPROCESSOR "clang")
    else ()
        set(DEFAULT_PREPROCESSOR "gcc")
    endif ()

    set(OPTIONAL OUTPUT ALL)
    set(SINGLE LICENSE CONFIG TARGET LOG FORMAT BIN CONVERTER PLATFORM PREPROCESSOR CFG_TEXT)
    set(MULTI SOURCES C_FLAGS CXX_FLAGS ARGS DEPENDS ANALYZE)
    cmake_parse_arguments(PVS_STUDIO "${OPTIONAL}" "${SINGLE}" "${MULTI}" ${ARGN})

    if ("${PVS_STUDIO_CFG}" STREQUAL "" OR NOT "${PVS_STUDIO_CFG_TEXT}" STREQUAL "")
        set(PVS_STUDIO_EMPTY_CONFIG ON)
    else ()
        set(PVS_STUDIO_EMPTY_CONFIG OFF)
    endif ()

    default(PVS_STUDIO_CFG_TEXT "analysis-mode=4")
    default(PVS_STUDIO_CONFIG "${CMAKE_BINARY_DIR}/PVS-Studio.cfg")
    default(PVS_STUDIO_C_FLAGS "")
    default(PVS_STUDIO_CXX_FLAGS "")
    default(PVS_STUDIO_TARGET "pvs")
    default(PVS_STUDIO_LOG "PVS-Studio.log")
    default(PVS_STUDIO_BIN "pvs-studio-analyzer")
    default(PVS_STUDIO_CONVERTER "plog-converter")
    default(PVS_STUDIO_PREPROCESSOR "${DEFAULT_PREPROCESSOR}")
    default(PVS_STUDIO_PLATFORM "linux64")

    if (PVS_STUDIO_EMPTY_CONFIG)
        set(PVS_STUDIO_CONFIG_COMMAND echo "${PVS_STUDIO_CFG_TEXT}" > "${PVS_STUDIO_CONFIG}")
    else ()
        set(PVS_STUDIO_CONFIG_COMMAND touch "${PVS_STUDIO_CONFIG}")
    endif ()

    add_custom_command(OUTPUT "${PVS_STUDIO_CONFIG}"
                       COMMAND ${PVS_STUDIO_CONFIG_COMMAND}
                       WORKING_DIRECTORY "${BINARY_DIR}"
                       COMMENT "Generating PVS-Studio.cfg")


    if (NOT "${PVS_STUDIO_PREPROCESSOR}" MATCHES "^${PVS_STUDIO_SUPPORTED_PREPROCESSORS}$")
        message(FATAL_ERROR "Preprocessor ${PVS_STUDIO_PREPROCESSOR} isn't supported. Available options: ${PVS_STUDIO_SUPPORTED_PREPROCESSORS}.")
    endif ()

    pvs_studio_append_standard_flag(PVS_STUDIO_CXX_FLAGS "${CMAKE_CXX_STANDARD}")
    pvs_studio_set_directory_flags("${CMAKE_CURRENT_SOURCE_DIR}" PVS_STUDIO_CXX_FLAGS PVS_STUDIO_C_FLAGS)

    if (NOT "${PVS_STUDIO_LICENSE}" STREQUAL "")
        pvs_studio_join_path(PVS_STUDIO_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}" "${PVS_STUDIO_LICENSE}")
        list(APPEND PVS_STUDIO_ARGS --lic-file "${PVS_STUDIO_LICENSE}")
    endif ()
    list(APPEND PVS_STUDIO_ARGS --cfg "${PVS_STUDIO_CONFIG}"
                                --platform "${PVS_STUDIO_PLATFORM}"
                                --preprocessor "${PVS_STUDIO_PREPROCESSOR}")

    set(PVS_STUDIO_PLOGS "")

    foreach (TARGET ${PVS_STUDIO_ANALYZE})
        set(DIR "${CMAKE_CURRENT_SOURCE_DIR}")
        string(FIND "${TARGET}" ":" DELIM)
        if ("${DELIM}" GREATER "-1")
            math(EXPR DELIMI "${DELIM}+1")
            string(SUBSTRING "${TARGET}" "${DELIMI}" "-1" DIR)
            string(SUBSTRING "${TARGET}" "0" "${DELIM}" TARGET)
            pvs_studio_join_path(DIR "${CMAKE_CURRENT_SOURCE_DIR}" "${DIR}")
        endif ()
        pvs_studio_analyze_target("${TARGET}" "${DIR}")
        list(APPEND PVS_STUDIO_DEPENDS "${TARGET}")
    endforeach ()

    set(PVS_STUDIO_TARGET_CXX_FLAGS "")
    set(PVS_STUDIO_TARGET_C_FLAGS "")
    foreach (SOURCE ${PVS_STUDIO_SOURCES})
        pvs_studio_analyze_file("${SOURCE}" "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
    endforeach ()

    pvs_studio_relative_path(LOG_RELATIVE "${CMAKE_BINARY_DIR}" "${PVS_STUDIO_LOG}")
    if (PVS_STUDIO_PLOGS)
        set(COMMANDS COMMAND cat ${PVS_STUDIO_PLOGS} > "${PVS_STUDIO_LOG}")
        set(COMMENT "Generating ${LOG_RELATIVE}")
        if (NOT "${PVS_STUDIO_FORMAT}" STREQUAL "" OR PVS_STUDIO_OUTPUT)
            if ("${PVS_STUDIO_FORMAT}" STREQUAL "")
                set(PVS_STUDIO_FORMAT "errorfile")
            endif ()
            list(APPEND COMMANDS
                 COMMAND mv "${PVS_STUDIO_LOG}" "${PVS_STUDIO_LOG}.pvs.raw"
                 COMMAND "${PVS_STUDIO_CONVERTER}" -t "${PVS_STUDIO_FORMAT}" "${PVS_STUDIO_LOG}.pvs.raw" -o "${PVS_STUDIO_LOG}"
                 COMMAND rm -f "${PVS_STUDIO_LOG}.pvs.raw")
        endif ()
    else ()
        set(COMMANDS COMMAND touch "${PVS_STUDIO_LOG}")
        set(COMMENT "Generating ${LOG_RELATIVE}: no sources found")
    endif ()

    add_custom_command(OUTPUT "${PVS_STUDIO_LOG}"
                       ${COMMANDS}
                       COMMENT "${COMMENT}"
                       DEPENDS ${PVS_STUDIO_PLOGS}
                       WORKING_DIRECTORY "${CMAKE_BINARY_DIR}")

    if (PVS_STUDIO_ALL)
        set(ALL "ALL")
    else ()
        set(ALL "")
    endif ()

    if (PVS_STUDIO_OUTPUT)
        set(COMMANDS COMMAND cat "${PVS_STUDIO_LOG}" 1>&2)
    else ()
        set(COMMANDS "")
    endif ()

    add_custom_target("${PVS_STUDIO_TARGET}" ${ALL} ${COMMANDS} WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" DEPENDS ${PVS_STUDIO_DEPENDS} "${PVS_STUDIO_LOG}")
endfunction ()

