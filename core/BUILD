package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    hdrs = [
        "ast.h",
        "static_error.h",
        "unicode.h",
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
    srcs = ["lexer_test.cpp"],
    deps = [
        ":lexer",
        "//external:gtest_main",
    ],
)

cc_library(
    name = "parser",
    srcs = [
        "desugarer.cpp",
        "parser.cpp",
        "string_utils.cpp",
    ],
    hdrs = [
        "desugarer.h",
        "parser.h",
        "string_utils.h",
    ],
    deps = [
        ":common",
        ":lexer",
        "//stdlib:std",
    ],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cpp"],
    deps = [
        ":parser",
        "//external:gtest_main",
    ],
)

cc_library(
    name = "jsonnet-common",
    srcs = [
        "desugarer.cpp",
        "formatter.cpp",
        "libjsonnet.cpp",
        "static_analysis.cpp",
        "vm.cpp",
    ],
    hdrs = [
        "desugarer.h",
        "formatter.h",
        "state.h",
        "static_analysis.h",
        "vm.h",
    ],
    deps = [
        ":common",
        ":lexer",
        ":parser",
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
    srcs = ["libjsonnet_test.cpp"],
    deps = [
        ":jsonnet-common",
        "//external:gtest_main",
    ],
)
