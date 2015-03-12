
import os
from distutils.core import setup, Extension
from distutils.command.build import build as DistutilsBuild
from subprocess import Popen


DIR = os.path.abspath(os.path.dirname(__file__))
JSONNET_SOURCES = ['libjsonnet.cpp', 'lexer.cpp', 'parser.cpp', 'static_analysis.cpp', 'vm.cpp', '_jsonnet.c']
COMPILE_ARGS = ['-std=c++0x']


def get_version():
    """
    Parses the version out of jsonnet.cpp
    """
    with open(os.path.join(DIR, 'jsonnet.cpp')) as f:
        for line in f:
            if '#define' in line and 'JSONNET_VERSION' in line:
                return line.partition('JSONNET_VERSION')[2].strip('\n "')


def make_std_lib():
    """
    Runs the jsonnet C makefile to build std.jsonnet.h
    """
    p = Popen(['make', 'std.jsonnet.h'], cwd=DIR)
    p.wait()
    if p.returncode != 0:
        raise Exception('Could not build std.jsonnet.h')


class CustomBuild(DistutilsBuild):
    def run(self):
        make_std_lib()
        DistutilsBuild.run(self)


setup(name='jsonnet',
      url='https://google.github.io/jsonnet/doc/',
      description='Python bindings for Jsonnet - The data templating language ',
      author='David Cunningham',
      author_email='dcunnin@google.com',
      version=get_version(),
      cmdclass={'build': CustomBuild},
      ext_modules=[Extension('_jsonnet', extra_compile_args=COMPILE_ARGS,
                             sources=JSONNET_SOURCES)]
)
