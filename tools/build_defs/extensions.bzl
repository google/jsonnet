# load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load(":python_repo.bzl", "python_headers")

def _impl(repository_ctx):
    python_headers(name = "default_python3_headers")

build_defs = module_extension(
    implementation = _impl,
)
