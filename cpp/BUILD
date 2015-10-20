package(default_visibility = ["//visibility:public"])

cc_library(
    name = "libjsonnet++",
    srcs = ["libjsonnet++.cc"],
    deps = [
        "//:libjsonnet",
        "//include:libjsonnet++",
    ],
    includes = ["include"],
)

cc_test(
    name = "libjsonnet++_test",
    srcs = ["libjsonnet++_test.cc"],
    data = ["//cpp/testdata"],
    deps = [
        ":libjsonnet++",
        "//external:gtest_main",
    ],
)
