load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["permissive"])

package(default_visibility = ["//visibility:private"])

cc_library(
    name = "ryml",
    srcs = [
        "rapidyaml.cpp",
    ],
    hdrs = [
        "rapidyaml-0.11.1.hpp",
    ],
    includes = ["."],
    visibility = ["//core:__pkg__"],
)
