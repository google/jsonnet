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

{
  // Abstract template of a "sour" cocktail.
  Sour: {
    local drink = self,

    // Hidden fields can be referred to
    // and overrridden, but do not appear
    // in the JSON output.
    citrus:: {
      kind: 'Lemon Juice',
      qty: 1,
    },
    sweetener:: {
      kind: 'Simple Syrup',
      qty: 0.5,
    },

    // A field that must be overridden.
    spirit:: error 'Must override "spirit"',

    ingredients: [
      { kind: drink.spirit, qty: 2 },
      drink.citrus,
      drink.sweetener,
    ],
    garnish: self.citrus.kind + ' twist',
    served: 'Straight Up',
  },
}
