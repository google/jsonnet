package(default_visibility = ["//visibility:private"])

cc_library(
    name = "libjsonnet++",
    srcs = ["libjsonnet++.cpp"],
    visibility = ["//visibility:public"],
    deps = [
        "//core:libjsonnet",
        "//include:libjsonnet++",
    ],
)

cc_test(
    name = "libjsonnet++_test",
    srcs = ["libjsonnet++_test.cpp"],
    data = ["//cpp/testdata"],
    deps = [
        ":libjsonnet++",
        "@googletest//:gtest_main",
    ],
)
