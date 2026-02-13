load("@rules_shell//shell:sh_library.bzl", "sh_library")
load("@rules_shell//shell:sh_test.bzl", "sh_test")

sh_test(
    name = "cmd_test",
    srcs = ["run_cmd_tests.sh"],
    deps = [":cmd_test_lib"],
    data = [
        "//cmd:jsonnet",
        "//cmd:jsonnetfmt",
    ] + glob(
        include = ["**"],
        exclude = [
            "BUILD",
            "run_cmd_tests.sh",
            "cmd_tests.source",
            ".gitignore",
        ]
    ),
    env = {
        "JSONNET_BIN": "$(rlocationpath //cmd:jsonnet)",
        "JSONNETFMT_BIN": "$(rlocationpath //cmd:jsonnetfmt)",
    },
    # `use_bash_launcher` requires rules_shell module which requires Bazel >= 8.0.
    use_bash_launcher = True,
)

sh_library(
    name = "cmd_test_lib",
    srcs = ["cmd_tests.source"],
)
