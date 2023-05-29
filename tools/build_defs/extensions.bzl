load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load(":python_repo.bzl", "python_headers")

def _impl(repository_ctx):
    git_repository(
         name = "io_bazel_rules_jsonnet",
         commit = "ad2b4204157ddcf7919e8bd210f607f8a801aa7f",
         remote = "https://github.com/bazelbuild/rules_jsonnet.git",
         shallow_since = "1556260764 +0200",
    )

    python_headers(name = "default_python3_headers")

build_defs = module_extension(
    implementation = _impl,
)
