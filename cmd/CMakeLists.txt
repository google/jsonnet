# Jsonnet command-line tool.

if (BUILD_JSONNET OR BUILD_TESTS)
    add_executable(jsonnet jsonnet.cpp utils.cpp)
    add_dependencies(jsonnet libjsonnet_for_binaries)
    target_link_libraries(jsonnet libjsonnet_for_binaries)

	install(TARGETS jsonnet DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

if (BUILD_JSONNETFMT OR BUILD_TESTS)
    add_executable(jsonnetfmt jsonnetfmt.cpp utils.cpp)
    add_dependencies(jsonnetfmt libjsonnet_for_binaries)
    target_link_libraries(jsonnetfmt libjsonnet_for_binaries)

	install(TARGETS jsonnetfmt DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
