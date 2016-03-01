package(default_visibility = ["//visibility:public"])

filegroup(
    name = "std",
    srcs = ["stdlib/std.jsonnet"],
)

genrule(
    name = "gen-std-jsonnet-h",
    srcs = ["stdlib/std.jsonnet"],
    outs = ["core/std.jsonnet.h"],
    cmd = "((od -v -Anone -t u1 $< | tr \" \" \"\n\" | grep -v \"^$$\" " +
          "| tr \"\n\" \",\" ) && echo \"0\") > $@; " +
          "echo >> $@",
)

cc_library(
    name = "jsonnet-common",
    srcs = [
        "core/desugarer.cpp",
        "core/formatter.cpp",
        "core/lexer.cpp",
        "core/parser.cpp",
        "core/static_analysis.cpp",
        "core/string_utils.cpp",
        "core/vm.cpp",
        "core/std.jsonnet.h",
    ],
    hdrs = [
        "core/ast.h",
        "core/desugarer.h",
        "core/formatter.h",
        "core/lexer.h",
        "core/parser.h",
        "core/state.h",
        "core/static_analysis.h",
        "core/static_error.h",
        "core/string_utils.h",
        "core/unicode.h",
        "core/vm.h",
        "include/libjsonnet.h",
    ],
    linkopts = ["-lm"],
    includes = ["core", "include"],
)

cc_library(
    name = "libjsonnet",
    srcs = ["core/libjsonnet.cpp"],
    hdrs = ["include/libjsonnet.h"],
    deps = [":jsonnet-common"],
    includes = ["include"],
)

cc_binary(
    name = "jsonnet",
    srcs = ["cmd/jsonnet.cpp"],
    deps = [":libjsonnet"],
    includes = ["include"],
)

cc_binary(
    name = "libjsonnet_test_snippet",
    srcs = ["core/libjsonnet_test_snippet.c"],
    deps = [":libjsonnet"],
    includes = ["includes"],
)

cc_binary(
    name = "libjsonnet_test_file",
    srcs = ["core/libjsonnet_test_file.c"],
    deps = [":libjsonnet"],
    includes = ["includes"],
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
