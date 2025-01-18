workspace(name = "jsonnet")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "com_google_googletest",
    # If updating googletest version, also update CMakeLists.txt.in.
    commit = "2fe3bd994b3189899d93f1d5a881e725e046fdc2",  # release: release-1.8.1
    remote = "https://github.com/google/googletest.git",
    shallow_since = "1535728917 -0400",
)

# This allows using py_test and py_library against python3.
register_toolchains("//platform_defs:default_python3_toolchain")

# This allows building C++ against python3 headers.
load("//tools/build_defs:python_repo.bzl", "python_headers")

python_headers(name = "default_python3_headers")
