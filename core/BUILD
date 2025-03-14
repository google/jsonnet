package(default_visibility = ["//visibility:private"])

cc_library(
    name = "libjsonnet",
    srcs = [
        "ast.h",
        "desugarer.cpp",
        "desugarer.h",
        "formatter.cpp",
        "formatter.h",
        "json.h",
        "lexer.cpp",
        "lexer.h",
        "libjsonnet.cpp",
        "parser.cpp",
        "parser.h",
        "pass.cpp",
        "pass.h",
        "path_utils.cpp",
        "path_utils.h",
        "state.h",
        "static_analysis.cpp",
        "static_analysis.h",
        "static_error.h",
        "string_utils.cpp",
        "string_utils.h",
        "unicode.h",
        "vm.cpp",
        "vm.h",
    ],
    copts = ["-I."],
    implementation_deps = [
        "//stdlib:std",
        "//third_party/json",
        "//third_party/md5:libmd5",
        "//third_party/rapidyaml:ryml",
    ],
    linkopts = ["-lm"],
    visibility = ["//visibility:public"],
    deps = [
        "//include:libjsonnet",
        "//include:libjsonnet_fmt",
    ],
)

cc_test(
    name = "lexer_test",
    srcs = ["lexer_test.cpp"],
    deps = [
        ":libjsonnet",
        # Note: On Ubuntu, apt-get install libgtest-dev google-mock
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "parser_test",
    srcs = ["parser_test.cpp"],
    deps = [
        ":libjsonnet",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "libjsonnet_test",
    srcs = ["libjsonnet_test.cpp"],
    deps = [
        ":libjsonnet",
        "@googletest//:gtest_main",
    ],
)

cc_test(
    name = "unicode_test",
    srcs = ["unicode_test.cpp"],
    deps = [
        ":libjsonnet",
        "@googletest//:gtest_main",
    ],
)
