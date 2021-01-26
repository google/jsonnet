#!/usr/bin/env python
from distutils.core import setup, Extension

_ext = Extension(
    '_' + '@_NAME@',
    sources=[@_SRC@],
    include_dirs=[@_INC_DIRS@],
)

setup(
    name        = '@_NAME@',
    version     = '@_VERSION@',
    author      = "@_AUTHOR@",
    description = """@_DESC@""",
    ext_modules = [_ext],
    py_modules  = ["@_NAME@"],
)
