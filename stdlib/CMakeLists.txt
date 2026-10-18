add_custom_command(
	COMMENT "Packing std.jsonnet into std.jsonnet.h"
	COMMAND
		"${CMAKE_COMMAND}"
			"-DINPUT_FILE=${CMAKE_CURRENT_SOURCE_DIR}/std.jsonnet"
			"-DOUTPUT_FILE=${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h"
			-P "${CMAKE_CURRENT_SOURCE_DIR}/pack_std_jsonnet.cmake"
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h"
	MAIN_DEPENDENCY std.jsonnet
	DEPENDS pack_std_jsonnet.cmake)

add_library(stdlib_h INTERFACE "${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h")
target_include_directories(stdlib_h INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
