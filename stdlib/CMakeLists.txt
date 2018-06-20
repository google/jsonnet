# Utility to convert file to a list of integeters.

file(READ ${PROJECT_SOURCE_DIR}/stdlib/std.jsonnet STD_JSONNET_DATA HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," STD_JSONNET_DATA ${STD_JSONNET_DATA})
set(STD_JSONNET_DATA "${STD_JSONNET_DATA}0x00")
file(WRITE ${PROJECT_SOURCE_DIR}/core/std.jsonnet.h ${STD_JSONNET_DATA})
