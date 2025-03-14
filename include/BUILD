package(default_visibility = ["//visibility:private"])

cc_library(
    name = "libjsonnet",
    hdrs = ["libjsonnet.h"],
    includes = ["."],
    visibility = ["//core:__pkg__"],
)

cc_library(
    name = "libjsonnet_fmt",
    hdrs = ["libjsonnet_fmt.h"],
    includes = ["."],
    visibility = ["//core:__pkg__"],
)

cc_library(
    name = "libjsonnet++",
    hdrs = ["libjsonnet++.h"],
    includes = ["."],
    visibility = ["//cpp:__pkg__"],
    deps = ["//include:libjsonnet"],
)
