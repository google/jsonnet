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

std.assertEqual({}, {}) &&
std.assertEqual({x: 1, y: 2}.y, 2) &&
std.assertEqual({x: error "foo", y: 2}.y, 2) &&
std.assertEqual({x: 1, y: 2} + {z: 3}, {x: 1, y: 2, z: 3}) &&
std.assertEqual({x: 1, y: 2} + {y: 3}, {x: 1, y: 3}) &&
std.assertEqual({x: 1, y: 2} + {y+: 3}, {x: 1, y: 5}) &&
std.assertEqual({a: {x: 1}, b: {x: 2}} + {a+: {y: 3}, b: {x: 3}}, {a: {x: 1, y: 3}, b: {x: 3}}) &&

std.assertEqual({x: 1, y: 2} == {x: 1, y: 2}, true) &&
std.assertEqual({x: 1, y: 2} == {x: 1, y: 2, z: 3}, false) &&
std.assertEqual({x: 1, y: 2} != {x: 1, y: 2}, false) &&
std.assertEqual({x: 1, y: 2} != {x: 1, y: 2, z: 3}, true) &&

std.assertEqual({f(x,y,z): x, y: self.f(1,2,3)}.y, 1) &&
std.assertEqual({f(x,y,z): x, y(x): self.f(x,2,3), z:self.y(4)}.z, 4) &&

// Object Local

std.assertEqual({local foo = 3, local bar = foo+1, x: foo, y: foo+bar}, {x: 3, y: 7}) &&
std.assertEqual({local foo = bar[1], local bar = [foo, 3], x: foo, y: [foo]+bar}, {x: 3, y: [3,3,3]}) &&


// Computed field name
std.assertEqual(local f = "x"; {[f]: 1}, {x: 1}) &&
std.assertEqual({[pair[0]]: pair[1] for pair in [["x", 1], ["y", 2]]}, {x: 1, y: 2}) &&
std.assertEqual(local t = {a:1, b:2, c:null, d:4}; {[x]: t[x] for x in [y for y in ["a", "b", "c", "d"] if t[y] != null]}, {a: 1, b: 2, d:4}) &&

// Comprehension object
std.assertEqual(local x = 10; { [""+x]: x for x in [x, x+1, x+2] }, { "10": 10, "11": 11, "12": 12 }) &&
std.assertEqual(local x = "foo"; { local x = "bar", [x]: 1 }, { foo: 1 }) &&
std.assertEqual({[x+""]: if x == 1 then 1 else x + $["1"] for x in [1,2,3]}, { "1": 1, "2": 3, "3": 4 }) &&

std.assertEqual(local x="baz"; {local x="bar", [x]: x for x in ["foo"]}, { foo: "bar" }) &&


std.assertEqual({ f: "foo", g: { [self.f]: 7 } }, { f: "foo", g: { foo: 7 } }) &&

std.assertEqual({[k]: null for k in []}, {}) &&
std.assertEqual({[null]: "test"}, {}) &&

std.assertEqual({[""+k]:k  for k in [1,2,3]}, {"1": 1, "2": 2, "3": 3}) &&
std.assertEqual({[""+(k+1)]:(k+1)  for k in [0,1,2]}, {[""+k]:k  for k in [1,2,3]}) &&
std.assertEqual({[""+k]:k  for k in [1,2,3]}, {"1": 1, "2": 2, "3": 3}) &&
true
