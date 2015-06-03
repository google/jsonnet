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
from distutils.core import setup, Extension
from distutils.command.build import build as DistutilsBuild
from subprocess import Popen


DIR = os.path.abspath(os.path.dirname(__file__))
LIB_OBJECTS = ['libjsonnet.o', 'lexer.o', 'parser.o', 'static_analysis.o', 'vm.o']
MODULE_SOURCES = ['_jsonnet.c']


def get_version():
    """
    Parses the version out of libjsonnet.h
    """
    with open(os.path.join(DIR, 'libjsonnet.h')) as f:
        for line in f:
            if '#define' in line and 'LIB_JSONNET_VERSION' in line:
                return line.partition('LIB_JSONNET_VERSION')[2].strip('\n "')


def make_targets(targets):
    """
    Runs the jsonnet C makefile to build the given targets
    """
    p = Popen(['make'] + targets, cwd=DIR)
    p.wait()
    if p.returncode != 0:
        raise Exception('Could not build %s' % (', '.join(targets)))


class CustomBuild(DistutilsBuild):
    def run(self):
        make_targets(LIB_OBJECTS)
        DistutilsBuild.run(self)


jsonnet_ext = Extension(
    '_jsonnet',
    sources=MODULE_SOURCES,
    extra_objects=LIB_OBJECTS,
    language='c++'
)

setup(name='jsonnet',
      url='https://google.github.io/jsonnet/doc/',
      description='Python bindings for Jsonnet - The data templating language ',
      author='David Cunningham',
      author_email='dcunnin@google.com',
      version=get_version(),
      cmdclass={'build': CustomBuild},
      ext_modules=[jsonnet_ext]
)
