#!/bin/bash

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

EXECUTED=0
FAILED=0

for TEST in *.jsonnet ; do
    EXPECTED_EXIT_CODE=0
    GOLDEN="true"
    if [ $(echo "$TEST" | cut -b 1-5) == "error" ] ; then
        EXPECTED_EXIT_CODE=1
    fi
    if [ -r "$TEST.golden" ] ; then
        GOLDEN=$(cat "$TEST.golden")
    fi
    OUTPUT="$(../jsonnet --gc-min-objects 1 --gc-growth-trigger 1 "$TEST" 2>&1 )"
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne $EXPECTED_EXIT_CODE ] ; then
        FAILED=$((FAILED + 1))
        echo "FAIL (exit code): $TEST"
        echo "Output:"
        echo "$OUTPUT"
    elif [ "$OUTPUT" != "$GOLDEN" ] ; then
        FAILED=$((FAILED + 1))
        echo "FAIL (output): $TEST"
        echo "Output:"
        echo "$OUTPUT"
    else
        true
        #echo "SUCCESS: $TEST"
    fi
    EXECUTED=$((EXECUTED + 1))
done

if [ $FAILED -eq 0 ] ; then
    echo "All $EXECUTED test scripts pass."
else
    echo "FAILED: $FAILED / $EXECUTED"
    exit 1
fi
