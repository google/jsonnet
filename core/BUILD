package(default_visibility = ["//visibility:public"])

cc_library(
    name = "jsonnet-common",
    srcs = [
        "desugarer.cpp",
        "formatter.cpp",
        "libjsonnet.cpp",
        "lexer.cpp",
        "parser.cpp",
        "static_analysis.cpp",
        "string_utils.cpp",
        "vm.cpp",
        "//stdlib:std.jsonnet.h",
    ],
    hdrs = [
        "ast.h",
        "desugarer.h",
        "formatter.h",
        "lexer.h",
        "parser.h",
        "state.h",
        "static_analysis.h",
        "static_error.h",
        "string_utils.h",
        "unicode.h",
        "vm.h",
    ],
    deps = [
        "//include:libjsonnet",
    ],
    linkopts = ["-lm"],
    includes = ["."],
)

cc_library(
    name = "libjsonnet",
    srcs = ["libjsonnet.cpp"],
    deps = [
        ":jsonnet-common",
        "//include:libjsonnet",
    ],
)

cc_test(
    name = "libjsonnet_test",
    srcs = ["libjsonnet_test.cc"],
    deps = [
        ":jsonnet-common",
        "//external:gtest_main",
    ],
)
