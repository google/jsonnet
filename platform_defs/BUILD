load("@rules_python//python:defs.bzl", "py_runtime", "py_runtime_pair")

py_runtime(
    name = "python3",
    interpreter_path = "/usr/bin/python3",
    python_version = "PY3",
    stub_shebang = "#!/usr/bin/env python3",
)

py_runtime_pair(
    name = "just_python3",
    py2_runtime = None,
    py3_runtime = ":python3",
)

toolchain(
    name = "default_python3_toolchain",
    toolchain = ":just_python3",
    toolchain_type = "@rules_python//python:toolchain_type",
)
