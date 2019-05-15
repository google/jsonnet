workspace(name = "jsonnet")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "io_bazel_rules_jsonnet",
    remote = "https://github.com/bazelbuild/rules_jsonnet.git",
    commit = "fd484046f9c4a32f4d696f05578907162d5a631f",
    shallow_since = "1551169313 +0100",
)

git_repository(
    name = "com_google_googletest",
    remote = "https://github.com/google/googletest.git",
    # Release 1.8.1
    # If updating googletest version, also update CMakeLists.txt.in.
    commit = "2fe3bd994b3189899d93f1d5a881e725e046fdc2",
    shallow_since = "1535728917 -0400"
)

load("//tools/build_defs:python_repo.bzl", "python_interpreter")

python_interpreter(name = "default_python")
