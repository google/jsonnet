option(RYML_BUILD_API_PYTHON "Enable Python API" ON)
option(RYML_API_TESTS "Enable API tests" ${RYML_BUILD_TESTS})
option(RYML_API_BENCHMARK "Enable API tests" ${RYML_BUILD_BENCHMARKS})

c4_log("enabling API")

cmake_policy(PUSH)
cmake_policy(SET CMP0078 NEW) # https://cmake.org/cmake/help/v3.14/policy/CMP0078.html
cmake_policy(SET CMP0086 NEW) # https://cmake.org/cmake/help/v3.14/policy/CMP0086.html

find_package(SWIG REQUIRED)
c4_log("found swig ${SWIG_VERSION}: ${SWIG_EXECUTABLE}")

# https://cmake.org/cmake/help/v3.13/module/UseSWIG.html
include(UseSWIG)

set(RYML_API_DIR ${CMAKE_CURRENT_LIST_DIR})
set(RYML_SWIG_SRC ${RYML_API_DIR}/ryml.i)

foreach(f ${RYML_SWIG_SRC})
    set_source_files_properties(${f} PROPERTIES
        CPLUSPLUS ON
        SWIG_MODULE_NAME ryml
        SWIG_FLAGS "-includeall"
        #INCLUDE_DIRECTORIES "${RYML_SRC_DIR}" # this needs quotes here!
        )
endforeach()


add_custom_target(ryml-api-build)
c4_set_folder_remote_project_targets("api" ryml-api-build)


if(RYML_API_TESTS)
    add_custom_target(ryml-api-test
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure ${${_c4_uprefix}CTEST_OPTIONS} -C $<CONFIG> -R "*api*"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    c4_set_folder_remote_project_targets("test" ryml-api-test)
endif()


if(RYML_API_BENCHMARKS)
    add_custom_target(ryml-api-bm)
    c4_set_folder_remote_project_targets("bm" ryml-api-bm)
endif()


if(RYML_BUILD_API_PYTHON)
    c4_log("enabling python3 API")
    find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
    c4_log("found python ${Python3_VERSION}: ${Python3_EXECUTABLE}")
    #
    set(t ryml-api-python3) # the target name
    set(g ${CMAKE_CURRENT_BINARY_DIR}/src/python3) # where to put c++ generated srcs
    set(r ${CMAKE_CURRENT_BINARY_DIR}/python3) # where to put the py files/libs
    #
    # alternative 1: roll out the extension using cmake
    #
    c4_get_transitive_property(ryml SOURCES ryml_srcs)
    c4_get_transitive_property(ryml INCLUDE_DIRECTORIES ryml_incs)
    swig_add_library(${t}
        LANGUAGE python
        OUTPUT_DIR ${r}
        OUTFILE_DIR ${g}
        SOURCES ${RYML_SWIG_SRC} ${ryml_srcs})
    #c4_set_folder_remote_project_targets("api" ${t})
    add_dependencies(ryml-api-build ${t})
    target_include_directories(${t} PUBLIC ${ryml_incs})
    swig_link_libraries(${t} ${Python3_LIBRARIES})
    set_target_properties(${t} PROPERTIES
        OUTPUT_NAME "ryml"
        SWIG_GENERATED_INCLUDE_DIRECTORIES ${Python3_INCLUDE_DIRS}
        ARCHIVE_OUTPUT_DIRECTORY "${r}/lib"
        LIBRARY_OUTPUT_DIRECTORY "${r}"
        RUNTIME_OUTPUT_DIRECTORY "${r}")
    if(WIN32)
        target_compile_definitions(${t} PUBLIC __WIN32__)
    endif()
    #target_link_libraries(${t} PUBLIC ryml) #"${RYML_SRC_DIR};${PROJECT_SOURCE_DIR}/ext/c4core/src")
    #target_include_directories(${t} PUBLIC "${RYML_SRC_DIR};${PROJECT_SOURCE_DIR}/ext/c4core/src")
    #
    # alternative 2: use distutils to create the extension
    #
    # get the list of generated files and format it as a python string list
    #c4_get_transitive_property(ryml SOURCES ryml_srcs)
    #c4_get_transitive_property(ryml INCLUDE_DIRS ryml_incs)
    #pylist("${ryml_srcs};rymlPYTHON_wrap.cxx" ryml_srcs)
    #pylist("${ryml_incs}" ryml_incs)
    #function(pylist li out)
    #    print_var(li)
    #    set(l)
    #    foreach(f ${li})
    #        set(l "${l}'${f}', ")
    #    endforeach()
    #    set(${out} ${l} PARENT_SCOPE)
    #endfunction()
    #function(create_setup_py src dst)
    #    set(opt0arg
    #        )
    #    set(opt1arg
    #        NAME
    #        VERSION
    #        AUTHOR
    #        DESC
    #        SRC
    #        )
    #    set(optnarg
    #        PYMODS
    #        )
    #    cmake_parse_arguments("" "${opt0arg}" "${opt1arg}" "${optnarg}" "${ARGN}")
    #    configure_file("${src}" "${dst}" @ONLY)
    #endfunction()
    #create_setup_py(${RYML_API_DIR}/python/setup.py ${g}/setup.py
    #    NAME ryml
    #    VERSION ${RYML_VERSION}
    #    AUTHOR "${RYML_AUTHOR}"
    #    DESC "${RYML_DESC}"
    #    SRC "${genfiles_str}"
    #    )
    #add_custom_target(${t}-distutils
    #    COMMAND ${Python3_EXECUTABLE} setup.py build_ext --inplace
    #    WORKING_DIRECTORY ${g})

    set(pydir ${CMAKE_CURRENT_LIST_DIR}/python)
    if(RYML_API_TESTS)
        add_custom_target(ryml-api-test-python3
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure ${${_c4_uprefix}CTEST_OPTIONS} -C $<CONFIG> -R ".*python3.*"
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            DEPENDS ${t}
            )
        add_dependencies(ryml-api-test ryml-api-test-python3)
        c4_set_folder_remote_project_targets("test" ryml-api-test-python3)
        function(add_python_test script)
            get_filename_component(script_name ${script} NAME_WE)
            set(script ${pydir}/${script})
            set(tn ryml-api-test-python3-${script_name})
            set(cmd ${CMAKE_COMMAND} -E env PYTHONPATH=${r} ${Python3_EXECUTABLE} ${script})
            add_custom_target(${tn}
                COMMAND ${cmd}
                DEPENDS ${t}
                WORKING_DIRECTORY ${pydir})
            c4_set_folder_remote_project_targets("test" ${tn})
            add_test(NAME ${tn}-run
                COMMAND ${cmd}
                WORKING_DIRECTORY ${pydir})
        endfunction()
        add_python_test(parse.py)
    endif()

    if(RYML_API_BENCHMARKS)
        add_custom_target(ryml-api-bm-python3)
        add_dependencies(ryml-api-bm ryml-api-bm-python3)
        c4_set_folder_remote_project_targets("bm" ryml-api-bm-python3)
        set(script ${pydir}/parse_bm.py)
        c4_add_benchmark_cmd(ryml-api-bm-python3-travis
            COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${r} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../bm/cases/travis.yml ryml)
        c4_add_benchmark_cmd(ryml-api-bm-python3-appveyor
            COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${r} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../bm/cases/appveyor.yml ryml)
        c4_add_benchmark_cmd(ryml-api-bm-python3-compile_commands
            COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${r} ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/../bm/cases/compile_commands.json ryml)
        c4_set_folder_remote_project_targets("bm" ryml-bm-api-python3-travis)
        c4_set_folder_remote_project_targets("bm" ryml-bm-api-python3-appveyor)
        c4_set_folder_remote_project_targets("bm" ryml-bm-api-python3-compile_commands)
    endif()
endif()


cmake_policy(POP)
