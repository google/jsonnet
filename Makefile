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

# C/C++ compiler; to use Clang, build with `make CC=clang CXX=clang++`
CXX ?= g++
CC ?= gcc
# coreutils `install` command
INSTALL ?= install
# The `help2man` command
HELP2MAN ?= help2man

# Directories to place installed files.
PREFIX ?= /usr/local
MAN1_DIR ?= man/man1

# Default optimisation level (override by passing OPT=... in the make invocation)
OPT ?= -O3
# Default flags (can be overriden by passing CXXFLAGS=... in the make invocation)
CXXFLAGS ?= -g $(OPT) -Wall -Wextra -Woverloaded-virtual -pedantic -fPIC
# Required flags (including -std=c++17)
CXXFLAGS += -std=c++17 -Iinclude -Ithird_party/md5 -Ithird_party/json -Ithird_party/rapidyaml/
# Default C compilation flags (can be overridden by passing CFLAGS=... in the make invocation)
CFLAGS ?= -g $(OPT) -Wall -Wextra -pedantic -fPIC
# Required C compilation flags (including -std=c99)
CFLAGS += -std=c99 -Iinclude
# Default flags for linking binaries
LDFLAGS ?=
# Default flags for linking .so shared objects
SHARED_LDFLAGS ?= -shared

###
# GoogleTest library, from the system.
#

GTEST_PKG := gtest_main

GTEST_FOUND := $(shell pkg-config --exists '$(GTEST_PKG)' && echo yes || echo no)
ifeq ($(GTEST_FOUND),yes)
GTEST_CXXFLAGS := $(shell pkg-config --cflags '$(GTEST_PKG)')
GTEST_LDFLAGS  := $(shell pkg-config --libs '$(GTEST_PKG)')
endif

HELP2MAN_FOUND := $(shell command -v '$(HELP2MAN)' > /dev/null 2>&1 && echo yes || echo no)
ifneq ($(HELP2MAN_FOUND),yes)
$(warning help2man was not found under the name $(HELP2MAN); manpages will not be built.)
endif

ifeq ($(origin GTEST_ENABLED),undefined)
GTEST_ENABLED := $(GTEST_FOUND)
ifeq ($(GTEST_ENABLED),no)
$(warning GoogleTest package was not found on the system, some tests will be skipped.)
endif
else ifeq ($(GTEST_ENABLED),$(GTEST_FOUND))
# Desired state matches found state; we're fine.
else ifeq ($(GTEST_ENABLED),no)
# GoogleTest was explicitly disabled; that's fine.
else ifeq ($(GTEST_ENABLED),yes)
$(error GoogleTest was explicitly requested but is not found on the system.)
override GTEST_ENABLED := no
else
$(error GTEST_ENABLED was set to some invalid value, it must be unset, 'yes', or 'no')
endif

# --- End Google Test Integration ---

################################################################################
# End of user-servicable parts
################################################################################

# Disable all GNU Make builtin rules, we want to specify our own.
.SUFFIXES:

# Single sed invocation that finds and extracts the version from the libjsonnet.h header.
# Hash and Dollar characters use hex escapes to avoid angering the Makefile interpreter.
EXTRACT_VERSION_SED := /^[ \t]*\x23[ \t]*define[ \t]+LIB_JSONNET_VERSION[ \t]+["]v([0-9.]+(-?[a-z][a-z0-9]*)?)["][ \t]*\x24/ { s//\1/p; q; }

# Extract the library version from libjsonnet.h.
VERSION := $(shell sed -nE '$(EXTRACT_VERSION_SED)' include/libjsonnet.h)
SOVERSION := 0

ifeq ($(shell uname -s),Darwin)

SONAME_FLAG := -install_name
TEST_RPATH_FLAG := -Wl,-rpath,'@executable_path'

else  # else assume Linux-like

SONAME_FLAG := -soname
TEST_RPATH_FLAG := -Wl,-rpath,'$$ORIGIN'

endif  # platform switch

LIB_SRC := \
	core/desugarer.cpp \
	core/formatter.cpp \
	core/lexer.cpp \
	core/libjsonnet.cpp \
	core/parser.cpp \
	core/pass.cpp \
	core/path_utils.cpp \
	core/static_analysis.cpp \
	core/string_utils.cpp \
	core/vm.cpp \
	third_party/md5/md5.cpp \
	third_party/rapidyaml/rapidyaml.cpp

LIB_OBJ := $(addprefix .makebuild/,$(addsuffix .o,$(LIB_SRC)))

LIB_CPP_SRC := \
	cpp/libjsonnet++.cpp

LIB_CPP_OBJ := $(LIB_OBJ) $(addprefix .makebuild/,$(addsuffix .o,$(LIB_CPP_SRC)))

BINS_SRC := \
	cmd/utils.cpp \
	cmd/jsonnetfmt.cpp \
	cmd/jsonnet.cpp

BINS := \
	jsonnet \
	jsonnetfmt

MAN_PAGES := $(addprefix $(MAN1_DIR)/,$(addsuffix .1,$(BINS)))

LIBS := \
	libjsonnet.so \
	libjsonnet.so.$(SOVERSION) \
	libjsonnet.so.$(VERSION) \
	libjsonnet++.so \
	libjsonnet++.so.$(SOVERSION) \
	libjsonnet++.so.$(VERSION) \

PUBLIC_HEADERS := \
	include/libjsonnet.h \
	include/libjsonnet_fmt.h \
	include/libjsonnet++.h

PLAIN_TEST_SRC := \
	core/libjsonnet_file_test.c \
	core/libjsonnet_native_callbacks_test.c
GTEST_TEST_SRC := \
	cpp/libjsonnet_locale_test.cpp \
	cpp/libjsonnet++_test.cpp \
	core/libjsonnet_test.cpp \
	core/lexer_test.cpp \
	core/unicode_test.cpp \
	core/parser_test.cpp

PLAIN_TEST_BINS := $(basename $(notdir $(PLAIN_TEST_SRC)))
GTEST_TEST_BINS := $(basename $(notdir $(GTEST_TEST_SRC)))
ALL_TEST_BINS := $(PLAIN_TEST_BINS) $(GTEST_TEST_BINS)

DEPS_FILES := $(addprefix .makebuild/,$(addsuffix .d,$(LIB_SRC) $(LIB_CPP_SRC) $(BINS_SRC) $(TEST_SRC)))
# Intermediate build output directories.
BUILD_DIRS := $(sort $(dir $(DEPS_FILES)) .makebuild/stdlib/)

ifeq ($(GTEST_ENABLED),yes)
ENABLED_TEST_BINS := $(ALL_TEST_BINS)
else
ENABLED_TEST_BINS := $(PLAIN_TEST_BINS)
endif

################################################################################
# Targets / Build rules
################################################################################

ALL := $(LIBS) $(BINS) $(MAN_PAGES) $(ENABLED_TEST_BINS)
.PHONY: default bins libs man all install dist clean
default: $(LIBS) $(BINS)
bins: $(BINS)
libs: $(LIBS)
man: $(MAN_PAGES)
all: $(ALL)

test: bins libs $(ENABLED_TEST_BINS)
	./tests.sh

dist:
	@{ >&2 echo "The dist target is no longer supported, please don't use it." ; exit 1 ; }

clean:
	{ \
		rm -df \
			$(BINS) $(LIBS) $(MAN_PAGES) $(ALL_TEST_BINS) \
			stdlib/to_c_array core/std.jsonnet.h \
		&& rm -rf .makebuild ;\
	}

install: bins libs man
	{ \
		$(INSTALL) -Dm 644 -t $(DESTDIR)$(PREFIX)/bin/ $(BINS) ;\
		$(INSTALL) -Dm 644 -t $(DESTDIR)$(PREFIX)/lib/ $(LIBS) ;\
		$(INSTALL) -Dm 644 -t $(DESTDIR)$(PREFIX)/include/ $(PUBLIC_HEADERS) ;\
		$(INSTALL) -Dm 644 -t $(DESTDIR)$(PREFIX)/share/$(MAN1_DIR)/ $(MAN_PAGES) ;\
	}

CC_DEPS_FLAGS = -MMD -MP -MF "$(addsuffix .d,$@)"

.makebuild/%.cpp.o: %.cpp
	mkdir -p $(@D) && $(CXX) $(CC_DEPS_FLAGS) $(CXXFLAGS) -o $@ -c $<
.makebuild/%.c.o: %.c
	mkdir -p $(@D) && $(CC) $(CC_DEPS_FLAGS) $(CFLAGS) -o $@ -c $<

%.so.$(SOVERSION): %.so.$(VERSION)
	ln -sf $< $@
%.so: %.so.$(SOVERSION)
	ln -sf $< $@

ifeq ($(HELP2MAN_FOUND),yes)
$(MAN_PAGES): $(MAN1_DIR)/%.1: % | $(MAN1_DIR)
	$(HELP2MAN) --no-info --output=$@ ./$<

$(MAN1_DIR):
	mkdir -p $@
else
.PHONY: $(MAN1_DIR) $(MAN_PAGES)
$(MAN1_DIR) $(MAN_PAGES):
	@{ >&2 echo "Skipping $@: Cannot build manpages, help2man was not found." ; }
endif

jsonnet: .makebuild/cmd/jsonnet.cpp.o .makebuild/cmd/utils.cpp.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

jsonnetfmt: .makebuild/cmd/jsonnetfmt.cpp.o .makebuild/cmd/utils.cpp.o $(LIB_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

libjsonnet.so.$(VERSION): $(LIB_OBJ)
	$(CXX) $(LDFLAGS) $(LIB_OBJ) $(SHARED_LDFLAGS) -Wl,$(SONAME_FLAG),libjsonnet.so.$(SOVERSION) -o $@

libjsonnet++.so.$(VERSION): $(LIB_CPP_OBJ)
	$(CXX) $(LDFLAGS) $(LIB_CPP_OBJ) $(SHARED_LDFLAGS) -Wl,$(SONAME_FLAG),libjsonnet++.so.$(SOVERSION) -o $@

# std.jsonnet.h is a generated file so the first build will fail if it isn't
# specified as an explicit dependency (after that, deps are known from compiler output)
core/desugarer.cpp: core/std.jsonnet.h

# Encode standard library for embedding in C
.makebuild/stdlib/to_c_array: stdlib/to_c_array.cpp
	mkdir -p $(@D) && $(CXX) $(CXXFLAGS) -o "$@" $^

core/%.jsonnet.h: stdlib/%.jsonnet .makebuild/stdlib/to_c_array
	.makebuild/stdlib/to_c_array "$<" "$@"

# Plain-C tests (link to libjsonnet.so); don't use GoogleTest (which is C++ only)
libjsonnet_file_test libjsonnet_native_callbacks_test: %: .makebuild/core/%.c.o libjsonnet.so
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(TEST_RPATH_FLAG)

###
#  Tests that use GoogleTest
#

ifeq ($(GTEST_ENABLED),yes)

# C++ lib tests (links to libjsonnet++.so)
libjsonnet_locale_test libjsonnet++_test: %: .makebuild/cpp/%.cpp.o libjsonnet++.so
	$(CXX) $(CXXFLAGS) $(GTEST_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(GTEST_LDFLAGS) $(TEST_RPATH_FLAG)

# C++ tests of the C API (link to libjsonnet.so)
libjsonnet_test: %: .makebuild/core/%.cpp.o libjsonnet.so
	$(CXX) $(CXXFLAGS) $(GTEST_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(GTEST_LDFLAGS) $(TEST_RPATH_FLAG)

# Core component unit tests (narrow deps)
lexer_test: %: .makebuild/core/%.cpp.o .makebuild/core/lexer.cpp.o
	$(CXX) $(CXXFLAGS) $(GTEST_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(GTEST_LDFLAGS)
unicode_test: %: .makebuild/core/%.cpp.o
	$(CXX) $(CXXFLAGS) $(GTEST_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(GTEST_LDFLAGS)
parser_test: %: .makebuild/core/%.cpp.o .makebuild/core/parser.cpp.o .makebuild/core/lexer.cpp.o
	$(CXX) $(CXXFLAGS) $(GTEST_CXXFLAGS) -o $@ $^ $(LDFLAGS) $(GTEST_LDFLAGS)

endif  # GTEST_ENABLED

#
###

-include $(DEPS_FILES)
