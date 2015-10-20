package(default_visibility = ["//visibility:public"])

load(
    "@bazel_tools//tools/build_defs/jsonnet:jsonnet.bzl",
    "jsonnet_to_json_test",
)

jsonnet_to_json_test(
    name = "example_test",
    src = "example.jsonnet",
    golden = "example_golden.json",
    jsonnet = "//:jsonnet",
    std = "//:std",
)

jsonnet_to_json_test(
    name = "importing_test",
    src = "importing.jsonnet",
    golden = "importing_golden.json",
    imports = ["."],
    jsonnet = "//:jsonnet",
    std = "//:std",
)

jsonnet_to_json_test(
    name = "invalid_test",
    src = "invalid.jsonnet",
    golden = "invalid.out",
    error = 1,
    jsonnet = "//:jsonnet",
    std = "//:std",
)

filegroup(
    name = "testdata",
    srcs = [
        "example.jsonnet",
        "example_golden.json",
        "importing.jsonnet",
        "importing_golden.json",
        "invalid.jsonnet",
        "invalid.out",
    ],
)
