load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["permissive"])

package(default_visibility = ["//visibility:private"])

cc_library(
    name = "json",
    hdrs = [
        "nlohmann/json.hpp",
    ],
    includes = ["."],
    visibility = ["//core:__pkg__"],
)
