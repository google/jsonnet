workspace(name = "jsonnet")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "io_bazel_rules_jsonnet",
    commit = "ad2b4204157ddcf7919e8bd210f607f8a801aa7f",
    remote = "https://github.com/bazelbuild/rules_jsonnet.git",
    shallow_since = "1556260764 +0200",
)

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest.git",
    # If updating googletest version, also update GoogleTestCMakeLists.txt.in.
    commit = "2fe3bd994b3189899d93f1d5a881e725e046fdc2", # release: release-1.8.1
    shallow_since = "1535728917 -0400",
)

git_repository(
    name = "com_googlesource_code_re2",
    remote = "https://github.com/google/re2.git",
    # If updating RE2 version, also update RE2CMakeLists.txt.in.
    commit = "0c95bcce2f1f0f071a786ca2c42384b211b8caba", # release: 2019-06-01
    shallow_since = "1558525654 +0000",
)

load("//tools/build_defs:python_repo.bzl", "python_interpreter")

python_interpreter(name = "default_python")
