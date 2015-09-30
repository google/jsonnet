package(default_visibility = ["//visibility:public"])

filegroup(
    name = "std",
    srcs = ["stdlib/std.jsonnet"],
)

genrule(
    name = "gen-std-jsonnet-h",
    srcs = ["stdlib/std.jsonnet"],
    outs = ["stdlib/std.jsonnet.h"],
    cmd = "((od -v -Anone -t u1 $< | tr \" \" \"\n\" | grep -v \"^$$\" " +
          "| tr \"\n\" \",\" ) && echo \"0\") > $@; " +
          "echo >> $@",
)

# TODO(dzc): Remove the includes = ["."] lines from all cc_* targets once
# bazelbuild/bazel#445 is fixed.
cc_library(
    name = "jsonnet-common",
    srcs = [
        "core/lexer.cpp",
        "core/parser.cpp",
        "core/static_analysis.cpp",
        "core/vm.cpp",
        "stdlib/std.jsonnet.h",
    ],
    hdrs = [
        "core/lexer.h",
        "core/parser.h",
        "core/static_analysis.h",
        "core/static_error.h",
        "core/vm.h",
    ],
    includes = ["."],
)

cc_library(
    name = "libjsonnet",
    srcs = ["core/libjsonnet.cpp"],
    hdrs = ["core/libjsonnet.h"],
    deps = [":jsonnet-common"],
    includes = ["."],
)

cc_binary(
    name = "jsonnet",
    srcs = ["cmd/jsonnet.cpp"],
    deps = [":libjsonnet"],
    includes = ["."],
)

cc_binary(
    name = "libjsonnet_test_snippet",
    srcs = ["core/libjsonnet_test_snippet.c"],
    deps = [":libjsonnet"],
    includes = ["."],
)

cc_binary(
    name = "libjsonnet_test_file",
    srcs = ["core/libjsonnet_test_file.c"],
    deps = [":libjsonnet"],
    includes = ["."],
)

filegroup(
    name = "object_jsonnet",
    srcs = ["test_suite/object.jsonnet"],
)

sh_test(
    name = "libjsonnet_test",
    srcs = ["core/libjsonnet_test.sh"],
    data = [
        ":jsonnet",
        ":libjsonnet_test_snippet",
        ":libjsonnet_test_file",
        ":object_jsonnet",
    ],
)
