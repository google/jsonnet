# Jsonnet command-line tool.

if (BUILD_JSONNET OR BUILD_TESTS)
    add_executable(jsonnet ${LIBJSONNET_SOURCE} jsonnet.cpp utils.cpp)
    add_dependencies(jsonnet libjsonnet_static)
    target_link_libraries(jsonnet libjsonnet_static)

	install(TARGETS jsonnet DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if (BUILD_JSONNETFMT OR BUILD_TESTS)
    add_executable(jsonnetfmt ${LIBJSONNET_SOURCE} jsonnetfmt.cpp utils.cpp)
    add_dependencies(jsonnetfmt libjsonnet_static)
    target_link_libraries(jsonnetfmt libjsonnet_static)

	install(TARGETS jsonnetfmt DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
