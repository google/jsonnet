package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    hdrs = [
        "unicode.h",
        "static_error.h",
    ],
    includes = ["."],
)

cc_library(
    name = "lexer",
    srcs = ["lexer.cpp"],
    hdrs = ["lexer.h"],
    deps = [
        ":common",
    ],
    includes = ["."],
)

cc_test(
    name = "lexer_test",
    srcs = ["lexer_test.cc"],
    deps = [
        ":lexer",
        "//external:gtest_main",
    ],
)

cc_library(
    name = "jsonnet-common",
    srcs = [
        "desugarer.cpp",
        "formatter.cpp",
        "libjsonnet.cpp",
        "parser.cpp",
        "static_analysis.cpp",
        "string_utils.cpp",
        "vm.cpp",
    ],
    hdrs = [
        "ast.h",
        "desugarer.h",
        "formatter.h",
        "parser.h",
        "state.h",
        "static_analysis.h",
        "string_utils.h",
        "vm.h",
    ],
    deps = [
        ":common",
        ":lexer",
        "//include:libjsonnet",
        "//stdlib:std",
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
