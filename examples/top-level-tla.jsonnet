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

local lib = import 'library-tla.libsonnet';

// Here is the top-level function, note brunch
// now has a default value.
function(prefix, brunch=false) {

  [prefix + 'Pina Colada']: {
    ingredients: [
      { kind: 'Rum', qty: 3 },
      { kind: 'Pineapple Juice', qty: 6 },
      { kind: 'Coconut Cream', qty: 2 },
      { kind: 'Ice', qty: 12 },
    ],
    garnish: 'Pineapple slice',
    served: 'Frozen',
  },

  [if brunch then prefix + 'Bloody Mary']: {
    ingredients: [
      { kind: 'Vodka', qty: 1.5 },
      { kind: 'Tomato Juice', qty: 3 },
      { kind: 'Lemon Juice', qty: 1.5 },
      { kind: 'Worcestershire', qty: 0.25 },
      { kind: 'Tobasco Sauce', qty: 0.15 },
    ],
    garnish: 'Celery salt & pepper',
    served: 'Tall',
  },

  [prefix + 'Mimosa']: lib.Mimosa(brunch),
}
