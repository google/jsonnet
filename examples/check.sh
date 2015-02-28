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

FAILED=0
SUCCESS=0

for I in *.golden ; do
    TEST=$(basename "$I" .golden)
    OUT1=$(../jsonnet "$TEST" 2>&1)
    OUT2=$(../jsonnet "$I" 2>&1)
    if [ "$OUT1" == "$OUT2" ] ; then
        SUCCESS=$((SUCCESS + 1))
    else
        echo "Failed: $TEST"
        echo "Got:"
        echo "$OUT1"
        echo "Expected:"
        echo "$OUT2"
        FAILED=$((FAILED + 1))
    fi
done

for I in *.error ; do
    TEST=$(basename "$I" .error)
    OUT1=$(../jsonnet "$TEST" 2>&1)
    OUT2=$(cat "$I" 2>&1)
    if [ "$OUT1" == "$OUT2" ] ; then
        SUCCESS=$((SUCCESS + 1))
    else
        echo "Failed: $TEST"
        echo "Got:"
        echo "$OUT1"
        echo "Expected:"
        echo "$OUT2"
        FAILED=$((FAILED + 1))
    fi
done

if [ "$FAILED" -eq 0 ] ; then
    echo "All $SUCCESS examples executed correctly."
else
    echo "$FAILED / $((FAILED+SUCCESS)) tests failed."
    exit 1
fi
