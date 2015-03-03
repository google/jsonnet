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

std.assertEqual((function(x)x*x)(5), 5*5) &&

// Correct self binding from within a function.
std.assertEqual({ x: 1, y: {x: 0, y:function() self.x}.y() }, { x: 1, y: 0 }) &&

local max = function(a,b) if a > b then a else b;
local inv(d) = if d != 0 then 1 / d else 0;

std.assertEqual(max(4, 8), 8) &&
std.assertEqual(inv(0.5), 2) &&

local is_even = function(n) if n == 0 then true else is_odd(n - 1),
      is_odd = function(n) if n == 1 then true else is_even(n - 1);

std.assertEqual(is_even(42), true) &&
std.assertEqual(is_odd(41), true) &&

true
