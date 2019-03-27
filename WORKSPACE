workspace(name = "jsonnet")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "io_bazel_rules_jsonnet",
    commit = "09ec18db5b9ad3129810f5f0ccc86363a8bfb6be",
    remote = "https://github.com/bazelbuild/rules_jsonnet.git",
)

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest.git",

    # If updating googletest version, also update CMakeLists.txt.in.
    tag = "release-1.8.1",
)

git_repository(
    name = "boringssl",
    commit = "b8a5219531146b0907a72da8e62f331bb0d673c5",
    remote = "https://boringssl.googlesource.com/boringssl",
)

git_repository(
    name = "com_google_absl",
    commit = "2a62fbdedf64673f7c858bc6487bd15bcd2ca180",
    remote = "https://github.com/abseil/abseil-cpp.git",
)

load("//tools/build_defs:python_repo.bzl", "python_interpreter")

python_interpreter(name = "default_python")
