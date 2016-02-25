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

local expect = (import "stdlib/truth.jsonnet").expect;


local evaluate(case) =
    expect()
        .that(case.input)
        .isEqualTo(case.output);

{
    "Test !": {
        evaluate: evaluate,
        cases: [
            {
                input: !false,
                output: true,
            },
            {
                input: !true,
                output: false,
            },
            {
                input: !!true,
                output: true,
            },
            {
                input: !!false,
                output: false
            },
        ],
    },
    "Test &&": {
        evaluate: evaluate,
        cases: [
            {
                input:false && false,
                output: false,
            },
            {
                input:false && true,
                output: false,
            },
            {
                input:true && false,
                output: false,
            },
            {
                input:true && true,
                output: true,
            },
        ]
    },
    "Test ||": {
        evaluate: evaluate,
        cases: [
            {
                input: false || false,
                output: false,
            },
            {
                input: false || true,
                output: true,
            },
            {
                input: true || false,
                output: true,
            },
            {
                input: true || true,
                output: true,
            },
        ]
    },
    "Test shortcut semantics": {
        evaluate: evaluate,
        cases: [
            {
                input: false && error "foo",
                output: false,
            },
            {
                input: true || error "foo",
                output: true,
            },
        ],
    },
    "Test ==": {
        evaluate: evaluate,
        cases: [
            {
                input: false == false,
                output: true,
            },
            {
                input: false == true,
                output: false,
            },
            {
                input: true == false,
                output: false,
            },
            {
                input: true == true,
                output: true,
            },
        ],
    },
    "Test !=": {
        evaluate: evaluate,
        cases: [
            {
                input: false != false,
                output: false,
            },
            {
                input: false != true,
                output: true,
            },
            {
                input: true != false,
                output: true,
            },
            {
                input: true != true,
                output: false,
            },
        ],
    },
    "Test no implicit conversions": {
        evaluate: evaluate,
        cases: [
            {
                input: "1" == 1,
                output: false,
            },
            {
                input: "true" == true,
                output: false,
            },
        ],
    },
    "Test if": {
        evaluate: evaluate,
        cases: [
            {
                input: if true then 3 else 5,
                output: 3,
            },
            {
                input: if false then 3 else 5,
                output: 5,
            },
        ],
    },
}
