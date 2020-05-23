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

readonly WEB_DIR='doc'
readonly COPYRIGHT_LINE='Copyright 2015 Google Inc. All rights reserved.'

if [ ! -r "$WEB_DIR/_config.yml" ]; then
  echo "No $WEB_DIR/_config.yml file found." >&1
  echo 'Are you running this script from the root of the Jsonnet repository?' >&1
  exit 1
fi

function copy() {
  local src_file="$1"
  local dst_dir="$2"

  mkdir -p "${WEB_DIR}/_includes/${dst_dir}"

  local dst_file="${WEB_DIR}/_includes/${dst_dir}/$(basename "$src_file")"
  local second_line=$(sed -n '2{p;q;}' "$src_file")
  if [ "$second_line" == "$COPYRIGHT_LINE" ]; then
    tail -n +17 "$src_file" > "$dst_file"
  else
    cp "$src_file" "$dst_file"
  fi
}

function redirect() {
  local src_file="$1"
  local dst_file="$2"

  mkdir -p "${WEB_DIR}/$(dirname ${src_file})"

  echo '---' > "${WEB_DIR}/${src_file}"
  echo 'layout: redirect' >> "${WEB_DIR}/${src_file}"
  echo "redirect: /$dst_file" >> "${WEB_DIR}/${src_file}"
  echo '---' >> "${WEB_DIR}/${src_file}"
}


copy examples/syntax.jsonnet examples
copy examples/syntax.jsonnet.golden examples
copy examples/variables.jsonnet examples
copy examples/variables.jsonnet.golden examples
copy examples/references.jsonnet examples
copy examples/references.jsonnet.golden examples
copy examples/inner-reference.jsonnet examples
copy examples/inner-reference.jsonnet.golden examples
copy examples/arith.jsonnet examples
copy examples/arith.jsonnet.golden examples
copy examples/functions.jsonnet examples
copy examples/functions.jsonnet.golden examples
copy examples/sours.jsonnet examples
copy examples/sours.jsonnet.golden examples
copy examples/conditionals.jsonnet examples
copy examples/conditionals.jsonnet.golden examples
copy examples/computed-fields.jsonnet examples
copy examples/computed-fields.jsonnet.golden examples
copy examples/comprehensions.jsonnet examples
copy examples/comprehensions.jsonnet.golden examples
copy examples/cocktail-comprehensions.jsonnet examples
copy examples/cocktail-comprehensions.jsonnet.golden examples
copy examples/garnish.txt examples
copy examples/imports.jsonnet examples
copy examples/imports.jsonnet.golden examples
copy examples/martinis.libsonnet examples
copy examples/utils.libsonnet examples
copy examples/negroni.jsonnet examples
copy examples/negroni.jsonnet.golden examples
copy examples/error-examples.jsonnet examples
copy examples/error-examples.jsonnet.golden examples
copy examples/top-level-ext.jsonnet examples
copy examples/top-level-ext.jsonnet.golden examples
copy examples/library-ext.libsonnet examples
copy examples/top-level-tla.jsonnet examples
copy examples/top-level-tla.jsonnet.golden examples
copy examples/library-tla.libsonnet examples
copy examples/sours-oo.jsonnet examples
copy examples/templates.libsonnet examples
copy examples/sours-oo.jsonnet.golden examples
copy examples/oo-contrived.jsonnet examples
copy examples/oo-contrived.jsonnet.golden examples
copy examples/mixins.jsonnet examples
copy examples/mixins.jsonnet.golden examples

redirect docs/index.html ref/language.html
redirect docs/tutorial.html learning/tutorial.html
redirect docs/stdlib.html ref/stdlib.html
redirect case_studies/casestudy_fractal.1.html articles/fractal.1.html
redirect language/spec.html ref/spec.html
redirect contributing.html learning/community.html

./jsonnet -S "$WEB_DIR/_stdlib_gen/stdlib.jsonnet" > "$WEB_DIR/ref/stdlib.html"
