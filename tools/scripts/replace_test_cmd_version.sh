#!/bin/bash

set -e

if [ "$#" != 1 ]; then
    echo "Usage: $0 <NEW_VERSION>"
fi

NEW_VERSION="$1"

set -x

find test_cmd -name '*.cpp' -o -name '*.golang' -o -name '*.stdout' -o -name '*.stderr' | \
	xargs sed -i 's/ v0[.][0-9.]*\(-pre[0-9]*\)\{0,1\}/ '"$NEW_VERSION"'/g'
