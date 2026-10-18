# Usage: cmake -DINPUT_FILE=<in> -DOUTPUT_FILE=<out> -P pack_std_jsonnet.cmake
#
# Writes the bytes of INPUT_FILE to OUTPUT_FILE as a comma-separated list of
# hex literals followed by a terminating 0, for use inside a C array
# initializer.
file(READ "${INPUT_FILE}" std_hex HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," std_bytes "${std_hex}")
file(WRITE "${OUTPUT_FILE}" "${std_bytes}0")
