load("@rules_python//python:defs.bzl", "py_library", "py_test")

package(default_visibility = ["//visibility:private"])

cc_binary(
    name = "_jsonnet.so",
    srcs = ["_jsonnet.c"],
    linkshared = 1,
    deps = [
        "//core:libjsonnet",
        "@rules_python//python/cc:current_py_cc_headers",
    ],
)

py_library(
    name = "_jsonnet",
    data = [":_jsonnet.so"],
    imports = ["."],
    srcs_version = "PY3",
    visibility = ["//visibility:public"],
)

py_test(
    name = "_jsonnet_test",
    size = "small",
    srcs = ["_jsonnet_test.py"],
    data = [
        "testdata/basic_check.jsonnet",
        "testdata/binary123.bin",
        "testdata/binary1230123.bin",
        "testdata/trivial.jsonnet",
        "testdata/trivial_no_eol.jsonnet",
    ],
    python_version = "PY3",
    deps = [":_jsonnet"],
)
