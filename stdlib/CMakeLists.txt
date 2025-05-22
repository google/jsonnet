# Utility to convert file to a list of integeters.

add_executable(to_c_array to_c_array.cpp)

# Custom command that will only build stdlib when it changes.

if(MSVC)

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/core/std.jsonnet.h
	COMMAND ${CMAKE_BINARY_DIR}/$<CONFIG>/to_c_array
			    ${PROJECT_SOURCE_DIR}/stdlib/std.jsonnet
					${PROJECT_SOURCE_DIR}/core/std.jsonnet.h
	DEPENDS to_c_array std.jsonnet)

else()

add_custom_command(
	OUTPUT ${PROJECT_SOURCE_DIR}/core/std.jsonnet.h
	COMMAND ${GLOBAL_OUTPUT_PATH}/to_c_array
			    ${PROJECT_SOURCE_DIR}/stdlib/std.jsonnet
					${PROJECT_SOURCE_DIR}/core/std.jsonnet.h
	DEPENDS to_c_array std.jsonnet)
endif()

# Standard library build target that libjsonnet can depend on.
add_custom_target(stdlib ALL
	DEPENDS ${PROJECT_SOURCE_DIR}/core/std.jsonnet.h)
