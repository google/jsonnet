CFLAGS := -Os -Wall -g
CXXFLAGS := $(CFLAGS)

PROGRAMS := $(basename $(wildcard *.c test/*.c test/*.cc *.S))

.PHONY: all clean
all: $(PROGRAMS)
clean:
	rm -f $(PROGRAMS) cscope.out tags

%: %.S
	$(CC) $(CFLAGS) -nostdlib $< -o $@

# Not using builtin rules due to debugbreak.h dependency
%: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

%: %.cc
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@

test/%: CFLAGS +=-I.
test/%: CXXFLAGS +=-I.
$(PROGRAMS): debugbreak.h
