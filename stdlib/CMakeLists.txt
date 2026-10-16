add_executable(to_c_array EXCLUDE_FROM_ALL to_c_array.cpp)
configure_target_cc_props(to_c_array)

if(CMAKE_CROSSCOMPILING)
	# to_c_array is built for the target and cannot run on the build
	# machine, so pack std.jsonnet at configure time instead. Same layout
	# as to_c_array: comma-separated byte values, NUL-terminated.
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS std.jsonnet)
	file(READ "${CMAKE_CURRENT_SOURCE_DIR}/std.jsonnet" std_hex HEX)
	string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," std_bytes "${std_hex}")
	file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/std.jsonnet.h" "${std_bytes}0")
	add_library(stdlib_h INTERFACE)
else()
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
endif()
target_include_directories(stdlib_h INTERFACE "${CMAKE_CURRENT_BINARY_DIR}")
