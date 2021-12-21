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
    # If updating googletest version, also update CMakeLists.txt.in.
    commit = "2fe3bd994b3189899d93f1d5a881e725e046fdc2", # release: release-1.8.1
    shallow_since = "1535728917 -0400",
)

# This allows using py_test and py_library against python3.
register_toolchains("//platform_defs:default_python3_toolchain")

# This allows building C++ against python3 headers.
load("//tools/build_defs:python_repo.bzl", "python_headers")
python_headers(name = "default_python3_headers")

