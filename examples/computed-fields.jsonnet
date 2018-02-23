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

local Margarita(salted) = {
  ingredients: [
    { kind: 'Tequila Blanco', qty: 2 },
    { kind: 'Lime', qty: 1 },
    { kind: 'Cointreau', qty: 1 },
  ],
  [if salted then 'garnish']: 'Salt',
};
{
  Margarita: Margarita(true),
  'Margarita Unsalted': Margarita(false),
}
