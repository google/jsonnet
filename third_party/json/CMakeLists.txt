add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
