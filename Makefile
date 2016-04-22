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

################################################################################
# User-servicable parts:
################################################################################

# C/C++ compiler -- clang also works
CXX ?= g++
CC ?= gcc

# Emscripten -- For Jsonnet in the browser
EMCXX ?= em++
EMCC ?= emcc

CP ?= cp
OD ?= od

OPT ?= -O3

CXXFLAGS ?= -g $(OPT) -Wall -Wextra -Woverloaded-virtual -pedantic -std=c++0x -fPIC -Iinclude
CFLAGS ?= -g $(OPT) -Wall -Wextra -pedantic -std=c99 -fPIC -Iinclude
EMCXXFLAGS = $(CXXFLAGS) --memory-init-file 0 -s DISABLE_EXCEPTION_CATCHING=0
EMCFLAGS = $(CFLAGS) --memory-init-file 0 -s DISABLE_EXCEPTION_CATCHING=0
LDFLAGS ?=

SHARED_LDFLAGS ?= -shared

################################################################################
# End of user-servicable parts
################################################################################

LIB_SRC = \
	core/desugarer.cpp \
	core/formatter.cpp \
	core/lexer.cpp \
	core/libjsonnet.cpp \
	core/parser.cpp \
	core/static_analysis.cpp \
	core/string_utils.cpp \
	core/vm.cpp

LIB_OBJ = $(LIB_SRC:.cpp=.o)

LIB_CPP_SRC = \
	cpp/libjsonnet++.cpp

LIB_CPP_OBJ = $(LIB_OBJ) $(LIB_CPP_SRC:.cpp=.o)

ALL = \
	jsonnet \
	libjsonnet.so \
	libjsonnet++.so \
	libjsonnet_test_snippet \
	libjsonnet_test_file \
	libjsonnet.js \
	doc/js/libjsonnet.js \
	$(LIB_OBJ)

ALL_HEADERS = \
	core/ast.h \
	core/desugarer.h \
	core/formatter.h \
	core/lexer.h \
	core/parser.h \
	core/state.h \
	core/static_analysis.h \
	core/static_error.h \
	core/string_utils.h \
	core/vm.h \
	core/std.jsonnet.h \
	include/libjsonnet.h \
	include/libjsonnet++.h

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
	cd test_suite ; ./run_fmt_tests.sh

MAKEDEPEND_SRCS = \
	cmd/jsonnet.cpp \
	core/libjsonnet_test_snippet.c \
	core/libjsonnet_test_file.c

depend:
	makedepend -f- $(LIB_SRC) $(MAKEDEPEND_SRCS) > Makefile.depend

core/desugarer.cpp: core/std.jsonnet.h

# Object files
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Commandline executable.
jsonnet: cmd/jsonnet.cpp $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< $(LIB_SRC:.cpp=.o) -o $@

# C binding.
libjsonnet.so: $(LIB_OBJ)
	$(CXX) $(LDFLAGS) $(LIB_OBJ) $(SHARED_LDFLAGS) -o $@

libjsonnet++.so: $(LIB_CPP_OBJ)
	$(CXX) $(LDFLAGS) $(LIB_CPP_OBJ) $(SHARED_LDFLAGS) -o $@

# Javascript build of C binding
JS_EXPORTED_FUNCTIONS = 'EXPORTED_FUNCTIONS=["_jsonnet_make", "_jsonnet_evaluate_snippet", "_jsonnet_realloc", "_jsonnet_destroy"]'

libjsonnet.js: $(LIB_SRC) $(ALL_HEADERS)
	$(EMCXX) -s $(JS_EXPORTED_FUNCTIONS) $(EMCXXFLAGS) $(LDFLAGS) $(LIB_SRC) -o $@

# Copy javascript build to doc directory
doc/js/libjsonnet.js: libjsonnet.js
	$(CP) $^ $@

# Tests for C binding.
LIBJSONNET_TEST_SNIPPET_SRCS = \
	core/libjsonnet_test_snippet.c \
	libjsonnet.so \
	include/libjsonnet.h

libjsonnet_test_snippet: $(LIBJSONNET_TEST_SNIPPET_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -L. -ljsonnet -o $@

LIBJSONNET_TEST_FILE_SRCS = \
	core/libjsonnet_test_file.c \
	libjsonnet.so \
	include/libjsonnet.h

libjsonnet_test_file: $(LIBJSONNET_TEST_FILE_SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) $< -L. -ljsonnet -o $@

# Encode standard library for embedding in C
core/%.jsonnet.h: stdlib/%.jsonnet
	(($(OD) -v -Anone -t u1 $< \
		| tr " " "\n" \
		| grep -v "^$$" \
		| tr "\n" "," ) && echo "0") > $@
	echo >> $@

clean:
	rm -vf */*~ *~ .*~ */.*.swp .*.swp $(ALL) *.o core/*.jsonnet.h Make.depend

-include Makefile.depend
