build_file_contents = """\
package(default_visibility = ["//visibility:public"])

filegroup(
    name = "include",
    srcs = glob(["include/**"]),
)

cc_library(
    name = "headers",
    hdrs = [":include"],
    includes = ["include"]
)
"""

def _impl(repository_ctx):
  rctx = repository_ctx
  if "/" in rctx.attr.path or "\\" in rctx.attr.path:
    # Canonicalize the path
    realpath = rctx.path(rctx.attr.path)
  else:
    # Find it in $PATH
    realpath = rctx.which(rctx.attr.path)
  rctx.symlink(realpath, "bin/python")
  include_path = rctx.execute([
      realpath, "-c", "import distutils.sysconfig; print(distutils.sysconfig.get_python_inc())",
  ])
  if include_path.return_code != 0:
    fail("Failed to locate Python headers:\n" + include_path.stderr)
  rctx.symlink(include_path.stdout.strip(), "include")
  rctx.file(
      "WORKSPACE",
      'workspace(name = "{}")\n'.format(rctx.name),
  )
  rctx.file("BUILD", build_file_contents)


python_headers = repository_rule(
    implementation = _impl,
    local = True,
    attrs = {
        "path": attr.string(
            default = "python3",
        ),
    },
)
