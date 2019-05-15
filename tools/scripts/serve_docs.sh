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

# Brings up the Jsonnet website locally.

set -e

function check {
  which $1 > /dev/null || (echo "$1 not installed. Please install $1."; exit 1)
}


check jekyll

if [ ! -r 'doc/_config.yml' ]; then
echo 'No doc/_config.yml file found.' >&1
echo 'Are you running this script from the root of the Jsonnet repository?' >&1
exit 1
fi

cd doc
# TODO: use --livereload, but it's not available on Jekyll 3.1.6
jekyll server --port 8200 --watch
