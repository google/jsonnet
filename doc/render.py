# Copyright 2015 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import codecs
import sys

import jinja2

env = jinja2.Environment(loader=jinja2.FileSystemLoader(searchpath="."),
                         autoescape=True)

def myinclude(filename):
    with codecs.open(filename, 'r', 'utf-8') as f:
        lines = f.readlines()
        # Strip copyright header, if it has one.
        if lines[1][0:10] == "Copyright ":
            lines = lines[16:]
    return "".join(lines)

env.globals['myinclude'] = myinclude

if len(sys.argv) != 3:
    sys.stderr.write("Usage: %s <filename.html.jinja> <output.html>\n" % sys.argv[0])
    sys.exit(1)

html = env.get_template(sys.argv[1]).render()
myfile = codecs.open(sys.argv[2], 'w', 'utf-8')
myfile.write(html)
myfile.close()

