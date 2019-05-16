package(default_visibility = ["//visibility:public"])

cc_library(
    name = "utils",
    srcs = ["utils.cpp"],
    hdrs = ["utils.h"],
    includes = ["."],
)

cc_binary(
    name = "jsonnet",
    srcs = ["jsonnet.cpp"],
    deps = [
        ":utils",
        "//core:libjsonnet",
    ],
)

cc_binary(
    name = "jsonnetfmt",
    srcs = ["jsonnetfmt.cpp"],
    deps = [
        ":utils",
        "//core:libjsonnet",
    ],
)
