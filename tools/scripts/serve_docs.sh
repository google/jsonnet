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
#jekyll server --port 8200 --watch
cat << EOF
Jekyll currently does not support wasm in local serving mode due to an
erroneous content-type, so as a workaround we are just /building/ the site
with jekyll.  To serve the built site, please change to the
doc/production directory and run a Python HTTP server like this:

python3 -c 'import http.server
class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-cache, no-store, must-revalidate")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        http.server.SimpleHTTPRequestHandler.end_headers(self)


http.server.test(HandlerClass=Handler,port=8200)
'

The site should then be accessible on localhost:8200 as normal.

Running jekyll build --watch now...
EOF
jekyll build --watch
