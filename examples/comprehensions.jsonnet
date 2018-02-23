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

local arr = std.range(5, 8);
{
  array_comprehensions: {
    higher: [x + 3 for x in arr],
    lower: [x - 3 for x in arr],
    evens: [x for x in arr if x % 2 == 0],
    odds: [x for x in arr if x % 2 == 1],
    evens_and_odds: [
      '%d-%d' % [x, y]
      for x in arr
      if x % 2 == 0
      for y in arr
      if y % 2 == 1
    ],
  },
  object_comprehensions: {
    evens: {
      ['f' + x]: true
      for x in arr
      if x % 2 == 0
    },
    // Use object composition (+) to add in
    // static fields:
    mixture: {
      f: 1,
      g: 2,
    } + {
      [x]: 0
      for x in ['a', 'b', 'c']
    },
  },
}
