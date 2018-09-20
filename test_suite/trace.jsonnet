/*
Copyright 2015 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

std.assertEqual(std.trace('', null), null) &&
std.assertEqual(std.trace('', true), true) &&
std.assertEqual(std.trace('', 77), 77) &&
std.assertEqual(std.trace('', 77.88), 77.88) &&
std.assertEqual(std.trace('', 'word'), 'word') &&
std.assertEqual(std.foldl(std.trace('', function(acc, i) acc + i), [1, 2, 3], 0), 6) &&
std.assertEqual(std.trace('', {}), {}) &&
std.assertEqual(std.trace('', { a: {} }), { a: {} }) &&
std.assertEqual(std.trace('', []), []) &&
std.assertEqual(std.trace('', [{ a: 'b' }, { a: 'b' }]), [{ a: 'b' }, { a: 'b' }]) &&
std.assertEqual(std.trace('Some Trace Message', { a: {} }), { a: {} }) &&

true
