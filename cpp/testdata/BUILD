load("//tools/build_defs:golden_test.bzl", "jsonnet_json_golden_test")

package(default_visibility = ["//visibility:public"])

jsonnet_json_golden_test(
    name = "example_test",
    src = "example.jsonnet",
    golden = "example_golden.json",
)

jsonnet_json_golden_test(
    name = "importing_test",
    src = "importing.jsonnet",
    data = ["example.jsonnet"],
    golden = "importing_golden.json",
)

jsonnet_json_golden_test(
    name = "invalid_test",
    src = "invalid.jsonnet",
    expect_error = True,
    golden = "invalid.out",
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
