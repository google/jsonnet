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

# Enable next line to test the garbage collector
#PARAMS="--gc-min-objects 1 --gc-growth-trigger 1"

# Enable next line for a slow and thorough test
#VALGRIND="valgrind -q"

# Display successful as well as failed tests (useful if they are slow).
VERBOSE=false

TFILE=$(tempfile)

for TEST in *.jsonnet ; do
    EXPECTED_EXIT_CODE=0
    GOLDEN="true"
    GOLDEN_REGEX=""
    if [ $(echo "$TEST" | cut -b 1-12) == "error.parse." ] ; then
        continue  # No point testing these
    fi
    if [ $(echo "$TEST" | cut -b 1-6) == "error." ] ; then
        EXPECTED_EXIT_CODE=1
    fi
    if [ -r "$TEST.golden" ] ; then
        GOLDEN=$(cat "$TEST.golden")
    fi
    if [ -r "$TEST.golden_regex" ] ; then
        GOLDEN_REGEX=$(cat "$TEST.golden_regex")
    fi
    OUTPUT="$($VALGRIND ../jsonnet --debug-ast "$TEST" | \
            $VALGRIND ../jsonnet $PARAMS --var var1=test --code-var var2='{x:1, y: 2}' - 2>&1 )"
    EXIT_CODE=$?
    if [ $EXIT_CODE -ne $EXPECTED_EXIT_CODE ] ; then
        FAILED=$((FAILED + 1))
        echo -e "\e[31;1mFAIL\e[0m \e[1m(exit code)\e[0m: \e[36m$TEST\e[0m"
        echo "Output:"
        echo "$OUTPUT"
    else
        if [ "$EXPECTED_EXIT_CODE" == 1 ] ; then
            true  # Since the stacktraces won't line up, ignore this.
        else
            if [ -n "$GOLDEN_REGEX" ] ; then
                if [[ ! "$OUTPUT" =~ $GOLDEN_REGEX ]] ; then
                    FAILED=$((FAILED + 1))
                    echo -e "\e[31;1mFAIL\e[0m \e[1m(regex mismatch)\e[0m: \e[36m$TEST\e[0m"
                    echo "Output:"
                    echo "$OUTPUT"
                else
                    $($VERBOSE) && echo -e "\e[32mSUCCESS\e[0m: \e[36m$TEST\e[0m"
                fi
            else
                if [ "$OUTPUT" != "$GOLDEN" ] ; then
                    FAILED=$((FAILED + 1))
                    echo -e "\e[31;1mFAIL\e[0m \e[1m(output mismatch)\e[0m: \e[36m$TEST\e[0m"
                    echo "Output:"
                    echo "$OUTPUT"
                else
                    $($VERBOSE) && echo -e "\e[32mSUCCESS\e[0m: \e[36m$TEST\e[0m"
                fi
            fi
        fi
    fi
    EXECUTED=$((EXECUTED + 1))
done

if [ $FAILED -eq 0 ] ; then
    echo "All $EXECUTED test scripts pass."
else
    echo "FAILED: $FAILED / $EXECUTED"
    exit 1
fi
