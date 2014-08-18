/*
Copyright 2014 Google Inc. All rights reserved.

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

// Can capture std from another file.
std.assertEqual((import "lib/capture_std_func.jsonnet")().sqrt(4), 2) &&

// Each import has its own std.
std.assertEqual(local std = { sqrt: function(x) x }; local lib = import "lib/capture_std.jsonnet"; lib.sqrt(4), 2) &&

// Now, test each std library function in turn.

std.assertEqual(std.makeArray(10,function(i)i+1), [1,2,3,4,5,6,7,8,9,10]) &&
std.assertEqual(std.makeArray(0,function(i)null), []) &&

std.assertEqual(std.pow(3,2), 9) &&
std.assertEqual(std.floor(10), 10) &&
std.assertEqual(std.floor(10.99999), 10) &&
std.assertEqual(std.ceil(10), 10) &&
std.assertEqual(std.ceil(10.99999), 11) &&
std.assertEqual(std.sqrt(9), 3) &&
std.assertEqual(std.sin(0), 0) &&
std.assertEqual(std.cos(0), 1) &&
std.assertEqual(std.tan(0), 0) &&
std.assertEqual(std.asin(0), 0) &&
std.assertEqual(std.acos(1), 0) &&
std.assertEqual(std.atan(0), 0) &&

std.assertEqual(std.type(null), "null") &&
std.assertEqual(std.type(true), "boolean") &&
std.assertEqual(std.type(false), "boolean") &&
std.assertEqual(std.type(0), "number") &&
std.assertEqual(std.type(-1e10), "number") &&
std.assertEqual(std.type([1,2,3]), "array") &&
std.assertEqual(std.type([]), "array") &&
std.assertEqual(std.type(function(x)x), "function") &&
std.assertEqual(std.type({x:1, y:2}), "object") &&
std.assertEqual(std.type({}), "object") &&
std.assertEqual(std.type("fail"), "string") &&
std.assertEqual(std.type(""+{}), "string") &&

std.assertEqual(std.filter(function(x)x%2==0, [1,2,3,4]), [2,4]) &&
std.assertEqual(std.filter(function(x)false, [1,2,3,4]), []) &&
std.assertEqual(std.filter(function(x)x, []), []) &&

std.assertEqual(std.objectHas({x: 1, y: 2}, "x"), true) &&
std.assertEqual(std.objectHas({x: 1, y: 2}, "z"), false) &&
std.assertEqual(std.objectHas({}, "z"), false) &&

std.assertEqual(std.length("asdfasdf"), 8) &&
std.assertEqual(std.length([1,4,9,error "foo"]), 4) &&
std.assertEqual(std.length(function(x,y,z)error "foo"), 3) &&
std.assertEqual(std.length({x: 1, y: 2}), 2) &&
std.assertEqual(std.length({a: 1, b:2, c:0} + {c: 3, d: error "foo"}), 4) &&
std.assertEqual(std.length(""), 0) &&
std.assertEqual(std.length([]), 0) &&
std.assertEqual(std.length(function()error "foo"), 0) &&
std.assertEqual(std.length({}), 0) &&

std.assertEqual(std.objectFields({}), []) &&
std.assertEqual(std.objectFields({x: 1, y: 2}), ["x", "y"]) &&
std.assertEqual(std.objectFields({a:1, b:2, c:null, d:error "foo"}), ["a", "b", "c", "d"]) &&


std.assertEqual(std.toString({a: 1, b: 2}), "{\"a\": 1, \"b\": 2}") &&
std.assertEqual(std.toString({}), "{ }") &&
std.assertEqual(std.toString([1, 2]), "[1, 2]") &&
std.assertEqual(std.toString([]), "[ ]") &&
std.assertEqual(std.toString(null), "null") &&
std.assertEqual(std.toString(true), "true") &&
std.assertEqual(std.toString(false), "false") &&
std.assertEqual(std.toString("str"), "str") &&
std.assertEqual(std.toString(""), "") &&
std.assertEqual(std.toString([1,2,"foo"]), "[1, 2, \"foo\"]") &&

std.assertEqual(std.substr("cookie", 1, 3), "ook") &&
std.assertEqual(std.substr("cookie", 1, 0), "") &&

std.assertEqual(std.codepoint("a"), 97) &&
std.assertEqual(std.char(97), "a") &&

std.assertEqual(std.map(function(x)x*x, []), []) &&
std.assertEqual(std.map(function(x)x*x, [1,2,3,4]), [1,4,9,16]) &&
std.assertEqual(std.map(function(x) x*x, std.filter(function(x)x>5, std.range(1,10))), [36, 49, 64, 81, 100]) &&

std.assertEqual(std.filterMap(function(x)x>=0, function(x)x*x, [-3,-2,-1,0,1,2,3]), [0,1,4,9]) &&

std.assertEqual(std.foldl(function(x,y)[x,y], [], "foo"), "foo") &&
std.assertEqual(std.foldl(function(x,y)[x,y], [1,2,3,4], []), [[[[[], 1],2],3],4]) &&

std.assertEqual(std.foldr(function(x,y)[x,y], [], "bar"), "bar") &&
std.assertEqual(std.foldr(function(x,y)[x,y], [1,2,3,4], []), [1,[2,[3,[4,[]]]]]) &&

std.assertEqual(std.range(2,6), [2,3,4,5,6]) &&
std.assertEqual(std.range(2,2), [2]) &&
std.assertEqual(std.range(2,1), []) &&

std.assertEqual(std.join(["a","b"], [[1,2],[3,4,5],[6]]), [1,2,"a","b",3,4,5,"a","b",6]) &&
std.assertEqual(std.join("ab", ["12","345","6"]), "12ab345ab6") &&

true
