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

CONTEXT = header.html.jinja footer.html.jinja ../examples/*.jsonnet ../examples/*.jsonnet.golden
PAGES = index.html \
		userdocs.html tutorial.html demo.html stdlib.html \
		casestudies.html casestudy_fractal.1.html casestudy_fractal.2.html casestudy_fractal.3.html \
        language.html design.html comparisons.html spec.html \
		implementation.html commandline.html bindings.html cpp.html javascript.html tests.html \
		contributing.html

all: $(PAGES)

%.html: %.html.jinja $(CONTEXT) render.py
	python render.py $< $@

clean:
	rm -vf $(PAGES)
