# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import re
import setuptools
from setuptools.command.build_ext import build_ext as BuildExt

DIR = os.path.abspath(os.path.dirname(__file__))
# NOTE: If you are editing the array below then you probably also need
# to change MANIFEST.in.
LIB_SOURCES = [
    "core/desugarer.cpp",
    "core/formatter.cpp",
    "core/libjsonnet.cpp",
    "core/lexer.cpp",
    "core/parser.cpp",
    "core/pass.cpp",
    "core/path_utils.cpp",
    "core/static_analysis.cpp",
    "core/string_utils.cpp",
    "core/vm.cpp",
    "third_party/md5/md5.cpp",
    "third_party/rapidyaml/rapidyaml.cpp",
    "python/_jsonnet.c",
]


def get_version():
    """
    Parses the version out of libjsonnet.h
    """
    rx = re.compile(
        r'^\s*#\s*define\s+LIB_JSONNET_VERSION\s+"v([0-9.]+(?:-?[a-z][a-z0-9]*)?)"\s*$'
    )
    with open(os.path.join(DIR, "include/libjsonnet.h")) as f:
        for line in f:
            m = rx.match(line)
            if m:
                return m.group(1)
    raise Exception(
        "could not find LIB_JSONNET_VERSION definition in include/libjsonnet.h"
    )


class BuildJsonnetExt(BuildExt):
    def _pack_std_jsonnet(self):
        print("generating core/std.jsonnet.h from stdlib/std.jsonnet")
        with open("stdlib/std.jsonnet", "rb") as f:
            stdlib = f.read()
        with open("core/std.jsonnet.h", "w", encoding="utf-8") as f:
            f.write(",".join(str(x) for x in stdlib))
            f.write(",0\n\n")

    def build_extensions(self):
        # At this point, the compiler has been chosen so we add compiler-specific flags.
        # There is unfortunately no built in support for this in setuptools.
        # Feature request: https://github.com/pypa/setuptools/issues/1819
        print("Adjusting compiler for compiler type " + self.compiler.compiler_type)
        # This is quite hacky as we're modifying the Extension object itself.
        if self.compiler.compiler_type == "msvc":
            for ext in self.extensions:
                ext.extra_compile_args.append("/std:c++17")
        else:
            # -std=c++17 should only be applied to C++ build,
            # not when compiling C source code. Unfortunately,
            # the extra_compile_args applies to both. Instead,
            # patch the CC/CXX commands in the compiler object.
            #
            # Note that older versions of distutils/setuptools do not
            # have the necessary separation between C and C++ compilers.
            # This requires setuptools 72.2.
            for v in ("compiler_cxx", "compiler_so_cxx"):
                if not hasattr(self.compiler, v):
                    print(
                        f"WARNING: cannot adjust flag {v}, "
                        f"compiler type {self.compiler.compiler_type}, "
                        f"compiler class {type(self.compiler).__name__}"
                    )
                    continue
                current = getattr(self.compiler, v)
                self.compiler.set_executable(v, current + ["-std=c++17"])
        super().build_extensions()

    def run(self):
        self._pack_std_jsonnet()
        super().run()


setuptools.setup(
    name="jsonnet",
    url="https://jsonnet.org",
    project_urls={
        "Source": "https://github.com/google/jsonnet",
    },
    description="Python bindings for Jsonnet - The data templating language ",
    license="Apache License 2.0",
    author="David Cunningham",
    author_email="dcunnin@google.com",
    version=get_version(),
    cmdclass={
        "build_ext": BuildJsonnetExt,
    },
    ext_modules=[
        setuptools.Extension(
            "_jsonnet",
            sources=LIB_SOURCES,
            include_dirs=[
                "include",
                "third_party/md5",
                "third_party/json",
                "third_party/rapidyaml",
            ],
            language="c++",
        )
    ],
    test_suite="python._jsonnet_test",
)
