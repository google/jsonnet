#!/usr/bin/env bash

# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

TEST_SUITE_NAME="${TEST_SUITE_NAME:-$0}"

cd $(dirname $0)

JSONNET_BIN="${JSONNET_BIN:-../../jsonnet}"

source ../../test_suite/tests.source

init

shopt -s nullglob

FAILED=0
SUCCESS=0

test_eval "$JSONNET_BIN" "stdlib-content-test.jsonnet" 0 "true" "PLAIN"

deinit

if [ "$FAILED" -eq 0 ] ; then
    echo "$TEST_SUITE_NAME: All $EXECUTED tests executed correctly."
else
    echo "$TEST_SUITE_NAME: $FAILED / $EXECUTED tests failed."
    exit 1
fi


