cc_binary(
    name = "_jsonnet.so",
    srcs = ["_jsonnet.c"],
    linkshared = 1,
    deps = [
        "//core:libjsonnet",
        "@default_python3_headers//:headers",
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
    srcs = ["_jsonnet_test.py"],
    data = [
        "testdata/basic_check.jsonnet",
        "testdata/trivial.jsonnet",
    ],
    python_version = "PY3",
    deps = [":_jsonnet"],
)
