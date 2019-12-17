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

set -e

source "tests.source"

if [ $# -eq 0 ] ; then
    echo "Usage: $0 <filename.jsonnet>" 2>&1
    exit 1
fi

for FILE in "$@" ; do
    if [[ $FILE == *.golden ]]; then
        echo "Specified file $FILE is already golden."
        echo "Please specify the input file instead."
        exit 1
    fi

    if [ ! -r "$FILE" ] ; then
        echo "Could not read: \"$FILE\"" 2>&1
        exit 1
    fi

    EXT_PARAMS=""
    TLA_PARAMS=""
    if [[ "$TEST" =~ ^tla[.] ]] ; then
        TLA_PARAMS="--tla-str var1=test --tla-code var2='{x:1,y:2}'"
    else
        EXT_PARAMS="--ext-str var1=test --ext-code var2='{x:1,y:2}'"
    fi

    if [ -n "$DISABLE_EXT_PARAMS" ]; then
        EXT_PARAMS=""
    fi
    JSONNET_CMD="$JSONNET_BIN $PARAMS $EXT_PARAMS $TLA_PARAMS"

    # Avoid set -e terminating us if the run fails.
    eval "$JSONNET_CMD" "$FILE" 2>&1 | postprocess_output > "${FILE}.golden"  || true

done


