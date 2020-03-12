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
from __future__ import print_function

import errno
import os
import subprocess
from io import open

from setuptools import Extension
from setuptools import setup
from setuptools.command.build_ext import build_ext

DIR = os.path.abspath(os.path.dirname(__file__))


def to_c_array(in_file, out_file):
    """
    Python port of stdlib/to_c_array.cpp
    """
    first_character = True

    with open(out_file, 'wb') as out:
        with open(in_file, 'rb') as i:
            while True:
                ba = i.read(1)
                if not ba:
                    break
                b = ba[0]
                if not isinstance(b, int):
                    b = ord(b)
                if first_character:
                    first_character = False;
                else:
                    out.write(b',')
                out.write(str(b).encode())
        out.write(b",0")


def get_version():
    """
    Parses the version out of libjsonnet.h
    """
    with open(os.path.join(DIR, 'include/libjsonnet.h')) as f:
        for line in f:
            if '#define' in line and 'LIB_JSONNET_VERSION' in line:
                v_code = line.partition('LIB_JSONNET_VERSION')[2].strip('\n "')
                if v_code[0] == "v":
                    v_code = v_code[1:]
                return v_code


def get_cpp_extra_compile_args(compiler):
    """
    Get the extra compile arguments for libjsonnet C++ compilation.
    """
    if compiler.compiler_type == 'msvc':
        return []
    else:
        return ["-Wextra", "-Woverloaded-virtual", "-pedantic", "-std=c++11"]


def get_c_extra_compile_args(compiler):
    """
    Get the extra compile arguments for python jsonnet C compilation.
    """
    if compiler.compiler_type == 'msvc':
        return []
    else:
        return ["-Wextra", "-pedantic", "-std=c99"]


def is_cmake_available():
    try:
        returncode = subprocess.call(["cmake", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return returncode == 0
    except OSError as e:
        if e.errno == errno.ENOENT:
            return False
        raise


def is_make_available():
    try:
        returncode = subprocess.call(["make", "--version"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        return returncode == 0
    except OSError as e:
        if e.errno == errno.ENOENT:
            return False
        raise


class custom_build_ext(build_ext):
    def build_extension(self, ext):
        if ext.name == '_jsonnet':
            libjsonnet_built = False

            if not libjsonnet_built:
                if is_cmake_available():
                    print("CMake is available.")
                    print("Building libjsonnet with CMake ...")
                    subprocess.call(['cmake', '.', '-Bbuild'], cwd=DIR)
                    returncode = subprocess.call(['cmake', '--build', 'build', '--target', 'libjsonnet_static'],
                                                 cwd=DIR)

                    ext.extra_objects = ['build/core/CMakeFiles/libjsonnet_static.dir/desugarer.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/formatter.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/libjsonnet.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/lexer.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/parser.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/pass.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/static_analysis.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/string_utils.cpp.o',
                                         'build/core/CMakeFiles/libjsonnet_static.dir/vm.cpp.o',
                                         'build/third_party/md5/CMakeFiles/md5.dir/md5.cpp.o']

                    if returncode == 0:
                        libjsonnet_built = True
                    else:
                        print("libjsonnet build has failed using CMake. Falling back to another build method ...")
                else:
                    print("CMake is not available.")
            if not libjsonnet_built:
                if is_make_available():
                    print("make is available.")
                    print("Building libjsonnet with make ...")

                    ext.extra_objects = [
                        'core/desugarer.o',
                        'core/formatter.o',
                        'core/libjsonnet.o',
                        'core/lexer.o',
                        'core/parser.o',
                        'core/pass.o',
                        'core/static_analysis.o',
                        'core/string_utils.o',
                        'core/vm.o',
                        'third_party/md5/md5.o']

                    returncode = subprocess.call(['make'] + ext.extra_objects, cwd=DIR)

                    if returncode == 0:
                        libjsonnet_built = True
                    else:
                        print("libjsonnet build has failed using make. Falling back to another build method ...")
                else:
                    print("make is not available")
            if not libjsonnet_built:
                print("Building libjsonnet with distutils compiler ...")
                try:
                    to_c_array(os.path.join(DIR, 'stdlib/std.jsonnet'), os.path.join(DIR, 'core/std.jsonnet.h'))

                    ext.extra_objects = self.compiler.compile([
                        'core/desugarer.cpp',
                        'core/formatter.cpp',
                        'core/libjsonnet.cpp',
                        'core/lexer.cpp',
                        'core/parser.cpp',
                        'core/pass.cpp',
                        'core/static_analysis.cpp',
                        'core/string_utils.cpp',
                        'core/vm.cpp',
                        'third_party/md5/md5.cpp'],
                        extra_postargs=get_cpp_extra_compile_args(self.compiler),
                        include_dirs=['include', 'third_party/md5', 'third_party/json'])

                    libjsonnet_built = True
                except Exception as e:
                    print(e)
                    print("libjsonnet build has failed using distutils compiler.")
                    raise Exception("Could not build libjsonnet")

            ext.extra_compile_args = get_c_extra_compile_args(self.compiler)

        build_ext.build_extension(self, ext)


jsonnet_ext = Extension(
    '_jsonnet',
    sources=['python/_jsonnet.c'],
    include_dirs=['include', 'third_party/md5', 'third_party/json'],
    language='c++'
)

setup(name='jsonnet',
      url='https://jsonnet.org',
      description='Python bindings for Jsonnet - The data templating language ',
      author='David Cunningham',
      author_email='dcunnin@google.com',
      version=get_version(),
      cmdclass={
          'build_ext': custom_build_ext,
      },
      ext_modules=[jsonnet_ext],
      test_suite="python._jsonnet_test",
      )
