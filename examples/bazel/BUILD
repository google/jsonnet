load("@rules_python//python:defs.bzl", "py_binary")

cc_binary(
    name = "use_cpp_lib",
    srcs = ["use_cpp_lib.cc"],
    deps = ["@jsonnet//cpp:libjsonnet++"],
)

cc_binary(
    name = "use_c_lib",
    srcs = ["use_c_lib.c"],
    deps = ["@jsonnet//core:libjsonnet"],
)

py_binary(
    name = "use_py_module",
    srcs = ["use_py_module.py"],
    deps = ["@jsonnet//python:_jsonnet"],
)

genrule(
    name = "use_jsonnet_cmd",
    srcs = ["example.jsonnet"],
    outs = ["example.out"],
    cmd = "$(location @jsonnet//cmd:jsonnet) $< > $@",
    tools = ["@jsonnet//cmd:jsonnet"],
)
