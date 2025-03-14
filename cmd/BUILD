package(default_visibility = ["//visibility:private"])

cc_library(
    name = "utils",
    srcs = ["utils.cpp"],
    hdrs = ["utils.h"],
    includes = ["."],
)

cc_binary(
    name = "jsonnet",
    srcs = ["jsonnet.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":utils",
        "//core:libjsonnet",
    ],
)

cc_binary(
    name = "jsonnetfmt",
    srcs = ["jsonnetfmt.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        ":utils",
        "//core:libjsonnet",
    ],
)
