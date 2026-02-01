load("@rules_shell//shell:sh_library.bzl", "sh_library")

package(default_visibility = ["//visibility:private"])

# Export the tests.source so it can be used by Bazel tests elsewhere in the repo.
sh_library(
    name = "tests_sh_lib",
    srcs = ["tests.source"],
    visibility = ["//tools/build_defs:__pkg__"],
)

filegroup(
    name = "test_input_for_core_test",
    srcs = ["object.jsonnet"],
    visibility = ["//core:__pkg__"],
)
