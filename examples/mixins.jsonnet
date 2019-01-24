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

local sours = import 'sours-oo.jsonnet';

local RemoveGarnish = {
  // Not technically removed, but made hidden.
  garnish:: super.garnish,
};

// Make virgin cocktails
local NoAlcohol = {
  local Substitute(ingredient) =
    local k = ingredient.kind;
    local bitters = 'Angustura Bitters';
    if k == 'Whiskey' then [
      { kind: 'Water', qty: ingredient.qty },
      { kind: bitters, qty: 'tsp' },
    ] else if k == 'Banks 7 Rum' then [
      { kind: 'Water', qty: ingredient.qty },
      { kind: 'Vanilla Essence', qty: 'dash' },
      { kind: bitters, qty: 'dash' },
    ] else [
      ingredient,
    ],
  ingredients: std.flattenArrays([
    Substitute(i)
    for i in super.ingredients
  ]),
};

local PartyMode = {
  served: 'In a plastic cup',
};

{
  'Whiskey Sour':
    sours['Whiskey Sour']
    + RemoveGarnish + PartyMode,

  'Virgin Whiskey Sour':
    sours['Whiskey Sour'] + NoAlcohol,

  'Virgin Daiquiri':
    sours.Daiquiri + NoAlcohol,

}
