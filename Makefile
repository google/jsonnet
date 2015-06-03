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

###################################################################################################
# User-servicable parts:
###################################################################################################

# C/C++ compiler -- clang also works
CXX ?= g++
CC ?= gcc

# Emscripten -- For Jsonnet in the browser
EMCXX ?= em++
EMCC ?= emcc

CP ?= cp
OD ?= od

CXXFLAGS ?= -g -O3 -Wall -Wextra -pedantic -std=c++0x -fPIC
CFLAGS ?= -g -O3 -Wall -Wextra -pedantic -std=c99 -fPIC
EMCXXFLAGS = $(CXXFLAGS) --memory-init-file 0 -s DISABLE_EXCEPTION_CATCHING=0
EMCFLAGS = $(CFLAGS) --memory-init-file 0 -s DISABLE_EXCEPTION_CATCHING=0
LDFLAGS ?=

SHARED_LDFLAGS ?= -shared

###################################################################################################
# End of user-servicable parts
###################################################################################################

LIB_SRC = lexer.cpp parser.cpp static_analysis.cpp vm.cpp libjsonnet.cpp
LIB_OBJ = $(LIB_SRC:.cpp=.o)

ALL = jsonnet libjsonnet.so libjsonnet_test_snippet libjsonnet_test_file libjsonnet.js doc/libjsonnet.js $(LIB_OBJ)
ALL_HEADERS = libjsonnet.h vm.h static_analysis.h parser.h lexer.h ast.h static_error.h state.h std.jsonnet.h

default: jsonnet

all: $(ALL)

TEST_SNIPPET = "std.assertEqual(({ x: 1, y: self.x } { x: 2 }).y, 2)"
test: jsonnet libjsonnet.so libjsonnet_test_snippet libjsonnet_test_file
	./jsonnet -e $(TEST_SNIPPET)
	LD_LIBRARY_PATH=. ./libjsonnet_test_snippet $(TEST_SNIPPET)
	LD_LIBRARY_PATH=. ./libjsonnet_test_file "test_suite/object.jsonnet"
	cd examples ; ./check.sh
	cd examples/terraform ; ./check.sh
	cd test_suite ; ./run_tests.sh

depend:
	makedepend -f- $(LIB_SRC) jsonnet.cpp libjsonnet_test_snippet.c libjsonnet_test_file.c > Makefile.depend 

parser.cpp: std.jsonnet.h

# Object files
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Commandline executable.
jsonnet: jsonnet.cpp $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< $(LIB_SRC:.cpp=.o) -o $@

# C binding.
libjsonnet.so: $(LIB_OBJ)
	$(CXX) $(LDFLAGS) $(LIB_OBJ) $(SHARED_LDFLAGS) -o $@

# Javascript build of C binding
libjsonnet.js: $(LIB_SRC) $(ALL_HEADERS)
	$(EMCXX) -s 'EXPORTED_FUNCTIONS=["_jsonnet_make", "_jsonnet_evaluate_snippet", "_jsonnet_realloc", "_jsonnet_destroy"]' $(EMCXXFLAGS) $(LDFLAGS) $(LIB_SRC) -o $@

# Copy javascript build to doc directory
doc/libjsonnet.js: libjsonnet.js
	$(CP) $^ $@

# Tests for C binding.
libjsonnet_test_snippet: libjsonnet_test_snippet.c libjsonnet.so libjsonnet.h
	$(CC) $(CFLAGS) $(LDFLAGS) $< -L. -ljsonnet -o $@

libjsonnet_test_file: libjsonnet_test_file.c libjsonnet.so libjsonnet.h
	$(CC) $(CFLAGS) $(LDFLAGS) $< -L. -ljsonnet -o $@

# Encode standard library for embedding in C
%.jsonnet.h: %.jsonnet
	(($(OD) -v -Anone -t u1 $< | tr " " "\n" | grep -v "^$$" | tr "\n" "," ) && echo "0") > $@
	echo >> $@

clean:
	rm -vf */*~ *~ .*~ */.*.swp .*.swp $(ALL) *.o *.jsonnet.h

-include Makefile.depend



