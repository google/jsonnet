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
import sys
import unittest

import _jsonnet


# Returns (full_path, contents) if the file was successfully retrieved,
# (full_path, None) if file not found, or throws an exception when the path
# is invalid or an IO error occured.
# It caches both hits and misses in the `cache` dict. Exceptions
# do not need to be cached, because they abort the computation anyway.
def try_path_cached(cache, dir, rel):
    if not rel:
        raise RuntimeError('Got invalid filename (empty string).')
    if rel[0] == '/':
        full_path = rel
    else:
        full_path = dir + rel
    if full_path[-1] == '/':
        raise RuntimeError('Attempted to import a directory')
    if full_path not in cache:
        if not os.path.isfile(full_path):
            cache[full_path] = None
        else:
            with open(full_path) as f:
                cache[full_path] = f.read().encode()
    return full_path, cache[full_path]

def import_callback_encode(dir, rel):
    cache = {}
    full_path, content = try_path_cached(cache, dir, rel)
    if content:
        return full_path, content
    raise RuntimeError('File not found')

def import_callback_empty_file_encode(dir, rel):
    return dir, b''


# Test native extensions
def concat(a, b):
    return a + b


def return_types():
    return {
        'a': [1, 2, 3, None, []],
        'b': 1.0,
        'c': True,
        'd': None,
        'e': {
            'x': 1,
            'y': 2,
            'z': ['foo']
        },
    }

native_callbacks = {
    'concat': (('a', 'b'), concat),
    'return_types': ((), return_types),
}


class JsonnetTests(unittest.TestCase):
    def setUp(self):
        base_dir = os.path.join(os.path.dirname(__file__), "testdata")
        self.input_filename = os.path.join(base_dir, "basic_check.jsonnet")
        self.trivial_filename = os.path.join(base_dir, "trivial.jsonnet")
        self.test_filename = os.path.join(base_dir, "test.jsonnet")
        with open(self.input_filename, "r") as infile:
            self.input_snippet = infile.read()

    def test_version(self):
        self.assertEqual(type(_jsonnet.version), str)

    def test_evaluate_file_encode(self):
        json_str = _jsonnet.evaluate_file(
            self.input_filename,
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "true\n")

    def test_evaluate_snippet_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            self.input_snippet,
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "true\n")

    def test_evaluate_snippet_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            self.input_snippet,
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "true\n")

    def test_import_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "import 'trivial.jsonnet'",
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "42\n")

    def test_import_no_eol_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "import 'trivial_no_eol.jsonnet'",
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "42\n")

    def test_import_binary_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "importbin 'binary123.bin'",
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "[\n   1,\n   2,\n   3\n]\n")

    def test_import_binary_sentinel_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "importbin 'binary1230123.bin'",
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "[\n   1,\n   2,\n   3,\n   0,\n   1,\n   2,\n   3\n]\n")

    def test_import_str_empty_file_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "importstr 'binary123.bin'",
            import_callback=import_callback_empty_file_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "\"\"\n")

    def test_import_binary_empty_file_encode(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "importbin 'binary123.bin'",
            import_callback=import_callback_empty_file_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "[ ]\n")

    def test_double_import(self):
        json_str = _jsonnet.evaluate_snippet(
            self.test_filename,
            "local x = import 'trivial.jsonnet';\n" +
            "local y = import 'trivial.jsonnet';\n" +
            "x + y",
            import_callback=import_callback_encode,
            native_callbacks=native_callbacks,
        )
        self.assertEqual(json_str, "84\n")

if __name__ == '__main__':
    unittest.main()
