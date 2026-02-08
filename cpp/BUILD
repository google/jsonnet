load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

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

cc_test(
    name = "libjsonnet_locale_test",
    srcs = ["libjsonnet_locale_test.cpp"],
    data = ["//cpp/testdata"],
    deps = [
        ":libjsonnet++",
        "@googletest//:gtest_main",
    ],
)
