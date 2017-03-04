# Utility to convert file to a list of integeters.

add_executable(to_c_array to_c_array.cpp)

add_executable(to_c_array_test to_c_array_test.cpp)
add_dependencies(to_c_array_test to_c_array)
target_link_libraries(to_c_array_test gtest)

# Custom command that will only build stdlib when it changes.
add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/core/std.jsonnet.h
	COMMAND ${GLOBAL_OUTPUT_PATH}/to_c_array
			    ${PROJECT_SOURCE_DIR}/stdlib/std.jsonnet
					${PROJECT_SOURCE_DIR}/core/std.jsonnet.h
	DEPENDS to_c_array std.jsonnet)

# Standard library build target that libjsonnet can depend on.
add_custom_target(stdlib ALL
	DEPENDS ${PROJECT_SOURCE_DIR}/core/std.jsonnet.h)
