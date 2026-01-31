add_executable(to_c_array EXCLUDE_FROM_ALL to_c_array.cpp)
configure_target_cc_props(to_c_array)

add_custom_command(
	COMMENT "Packing std.jsonnet into std.jsonnet.h"
	COMMAND
		to_c_array
			"${CMAKE_CURRENT_SOURCE_DIR}/std.jsonnet"
			"${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h"
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h"
	MAIN_DEPENDENCY std.jsonnet
	DEPENDS to_c_array)

add_library(stdlib_h INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h")
target_include_directories(stdlib_h INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
