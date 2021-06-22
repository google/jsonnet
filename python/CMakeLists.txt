# python-jsonnet

message("Building python version")

set(PYTHON_JSONNET_SRCS
    "_jsonnet.c"
)

add_library(python_jsonnet SHARED ${PYTHON_JSONNET_SRCS})
if (BUILD_SHARED_BINARIES)
    target_link_libraries(python_jsonnet libjsonnet)
    add_dependencies(python_jsonnet libjsonnet)
else()
    target_link_libraries(python_jsonnet libjsonnet_static)
    add_dependencies(python_jsonnet libjsonnet_static)
endif()

set_target_properties(python_jsonnet PROPERTIES PREFIX "")
set_target_properties(python_jsonnet PROPERTIES OUTPUT_NAME "_jsonnet")

# find C extension suffix
exec_program(${PYTHON_EXECUTABLE}
    ARGS "-c \"import sysconfig; print(sysconfig.get_config_vars()['EXT_SUFFIX'])\""
    OUTPUT_VARIABLE PYTHON_C_SUFFIX
)
set_target_properties(python_jsonnet PROPERTIES SUFFIX ${PYTHON_C_SUFFIX})

# get where to put it
exec_program(${PYTHON_EXECUTABLE}
    ARGS "-c \"import sysconfig; print(sysconfig.get_paths()['platlib'])\""
    OUTPUT_VARIABLE PYTHON_EXTENSION_LOCATION
)

target_include_directories(python_jsonnet PUBLIC
    ${PYTHON_INCLUDE_DIRS}
)

install(TARGETS python_jsonnet
        RUNTIME DESTINATION ${PYTHON_EXTENSION_LOCATION}
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
