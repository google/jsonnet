package(default_visibility = ["//visibility:private"])

# Export the tests.source so it can be used by Bazel tests elsewhere in the repo.
sh_library(
    name = "tests_sh_lib",
    srcs = ["tests.source"],
    visibility = ["//tools/build_defs:__pkg__"],
)
