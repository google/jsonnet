# Rapid YAML
[![MIT Licensed](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/biojppm/rapidyaml/blob/master/LICENSE.txt)
[![release](https://img.shields.io/github/v/release/biojppm/rapidyaml?color=g&include_prereleases&label=release%20&sort=semver)](https://github.com/biojppm/rapidyaml/releases)
[![API Docs](https://img.shields.io/badge/docs-docsforge-blue)](https://rapidyaml.docsforge.com/)
[![Gitter](https://badges.gitter.im/rapidyaml/community.svg)](https://gitter.im/rapidyaml/community)

[![ci](https://github.com/biojppm/rapidyaml/workflows/ci/badge.svg?branch=master)](https://github.com/biojppm/rapidyaml/actions?query=workflow%3Aci)
[![Coverage: coveralls](https://coveralls.io/repos/github/biojppm/rapidyaml/badge.svg?branch=master)](https://coveralls.io/github/biojppm/rapidyaml)
[![Coverage: codecov](https://codecov.io/gh/biojppm/rapidyaml/branch/master/graph/badge.svg?branch=master)](https://codecov.io/gh/biojppm/rapidyaml)
[![Total alerts](https://img.shields.io/lgtm/alerts/g/biojppm/rapidyaml.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/biojppm/rapidyaml/alerts/)
[![Language grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/biojppm/rapidyaml.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/biojppm/rapidyaml/context:cpp)


Or ryml, for short. ryml is a library to parse and emit YAML, and do it fast.

ryml parses both read-only and in-situ source buffers; the resulting data
nodes hold only views to sub-ranges of the source buffer. No string copies or
duplications are done, and no virtual functions are used. The data tree is a
flat index-based structure stored in a single array. Serialization happens
only at your direct request, after parsing / before emitting. Internally the
data tree representation has no knowledge of types (but of course, every node
can have a YAML type tag). It is easy and fast to read, write and iterate
through the data tree.

ryml can use custom per-tree memory allocators, and is
exception-agnostic. Errors are reported via a custom error handler callback.
A default error handler implementation using `std::abort()` is provided, but
you can opt out, or provide your exception-throwing callback.

ryml has respect for your compilation times and therefore it is NOT
header-only. It uses standard cmake build files, so it is easy to compile and
install.

ryml has no dependencies, not even on the STL (although it does use the
libc). It provides optional headers that let you serialize/deserialize
STL strings and containers (or show you how to do it).

ryml is written in C++11, and is known to compile with:
* Visual Studio 2015 and later
* clang++ 3.9 and later
* g++ 5 and later

ryml is [extensively unit-tested in Linux, Windows and
MacOS](https://github.com/biojppm/rapidyaml/actions?query=workflow%3Arun_tests). The
tests include analysing ryml with:
  * valgrind
  * clang-tidy
  * clang sanitizers:
    * memory
    * address
    * undefined behavior
    * thread
  * [LGTM.com](https://lgtm.com/projects/g/biojppm/rapidyaml)

ryml is also partially available in Python, with more languages to follow (see
below).

See also [the changelog](./changelog) and [the roadmap](./ROADMAP.md).


------

## Table of contents

* [Is it rapid?](#is-it-rapid)
   * [Comparison with yaml-cpp](#comparison-with-yaml-cpp)
   * [Performance reading JSON](#performance-reading-json)
   * [Performance emitting](#performance-emitting)
* [Installing and using](#installing-and-using)
   * [Using ryml as cmake subproject](#using-ryml-as-cmake-subproject)
   * [The traditional way: using an installed version](#the-traditional-way-using-an-installed-version)
   * [cmake build settings for ryml](#cmake-build-settings-for-ryml)
* [Quick start](#quick-start)
   * [Parsing](#parsing)
   * [References: anchors and aliases](#references-anchors-and-aliases)
   * [Traversing the tree](#traversing-the-tree)
   * [Creating a tree](#creating-a-tree)
   * [Low-level API](#low-level-api)
   * [Custom types](#custom-types)
      * [Leaf types](#leaf-types)
      * [Container types](#container-types)
   * [STL interoperation](#stl-interoperation)
   * [Custom formatting for intrinsic types](#custom-formatting-for-intrinsic-types)
   * [Custom allocators and error handlers](#custom-allocators-and-error-handlers)
   * [Using ryml to parse JSON, and preprocessing functions](#using-ryml-to-parse-json-and-preprocessing-functions)
* [Other languages](#other-languages)
   * [Python](#python)
* [YAML standard conformance](#yaml-standard-conformance)
* [Known limitations](#known-limitations)
* [Alternative libraries](#alternative-libraries)
* [License](#license)

------

## Is it rapid?

You bet!

(The results presented below are a bit scattered; and they need to be
sistematized.) On a i7-6800K CPU @3.40GHz:
 * ryml parses YAML at about ~150MB/s on Linux and ~100MB/s on Windows (vs2017). 
 * **ryml parses JSON at about ~450MB/s on Linux**, faster than sajson (didn't
   try yet on Windows).
 * compared against the other existing YAML libraries for C/C++:
   * ryml is in general between 2 and 3 times faster than [libyaml](https://github.com/yaml/libyaml)
   * ryml is in general between 20 and 70 times faster than
     [yaml-cpp](https://github.com/jbeder/yaml-cpp), and in some cases as
     much as 100x and [even
     200x](https://github.com/biojppm/c4core/pull/16#issuecomment-700972614) faster.

[Here's the benchmark](./bm/bm_parse.cpp). Using different
approaches within ryml (in-situ/read-only vs. with/without reuse), a YAML /
JSON buffer is repeatedly parsed, and compared against other libraries.

### Comparison with yaml-cpp

The first result set is for Windows, and is using a [appveyor.yml config
file](./bm/cases/appveyor.yml). A comparison of these results is
summarized on the table below:

| Read rates (MB/s)            | ryml   | yamlcpp | compared     |
|------------------------------|--------|---------|--------------|
| appveyor / vs2017 / Release  | 101.5  | 5.3     |  20x / 5.2%  |
| appveyor / vs2017 / Debug    |   6.4  | 0.0844  |  76x / 1.3%  |


The next set of results is taken in Linux, comparing g++ 8.2 and clang++ 7.0.1 in
parsing a YAML buffer from a [travis.yml config
file](./bm/cases/travis.yml) or a JSON buffer from a [compile_commands.json
file](./bm/cases/compile_commands.json). You
can [see the full results here](./bm/results/parse.linux.i7_6800K.md).
Summarizing:

| Read rates (MB/s)           | ryml   | yamlcpp | compared   |
|-----------------------------|--------|---------|------------|
| json   / clang++ / Release  | 453.5  | 15.1    |  30x / 3%  |
| json   /     g++ / Release  | 430.5  | 16.3    |  26x / 4%  |
| json   / clang++ / Debug    |  61.9  | 1.63    |  38x / 3%  |
| json   /     g++ / Debug    |  72.6  | 1.53    |  47x / 2%  |
| travis / clang++ / Release  | 131.6  | 8.08    |  16x / 6%  |
| travis /     g++ / Release  | 176.4  | 8.23    |  21x / 5%  |
| travis / clang++ / Debug    |  10.2  | 1.08    |   9x / 1%  |
| travis /     g++ / Debug    |  12.5  | 1.01    |  12x / 8%  |

The 450MB/s read rate for JSON puts ryml squarely in the same ballpark
as [RapidJSON](https://github.com/Tencent/rapidjson) and other fast json
readers
([data from here](https://lemire.me/blog/2018/05/03/how-fast-can-you-parse-json/)).
Even parsing full YAML is at ~150MB/s, which is still in that performance
ballpark, albeit at its lower end. This is something to be proud of, as the
YAML specification is much more complex than JSON: [23449 vs 1969 words](https://www.arp242.net/yaml-config.html#its-pretty-complex).


### Performance reading JSON

So how does ryml compare against other JSON readers? Well, it's one of the
fastest! 

The benchmark is the [same as above](./bm/parse.cpp), and it is reading
the [compile_commands.json](./bm/cases/compile_commands.json), The `_ro`
suffix notes parsing a read-only buffer (so buffer copies are performed),
while the `_rw` suffix means that the source buffer can be parsed in
situ. The `_reuse` means the data tree and/or parser are reused on each
benchmark repeat.

Here's what we get with g++ 8.2:

```
|------------------|-------------------------------|-------------------------------
|                  |           Release             |           Debug               
| Benchmark        |  Iterations    Bytes/sec      |  Iterations    Bytes/sec      
|------------------|-------------------------------|-------------------------------
| rapidjson_ro     |        7941    509.855M/s     |         633    43.3632M/s     
| rapidjson_rw     |       21400    1.32937G/s     |        1067    68.171M/s     
| sajson_rw        |        6808    434.245M/s     |        2770    176.478M/s     
| sajson_ro        |        6726    430.723M/s     |        2748    175.613M/s     
| jsoncpp_ro       |        2871    183.616M/s     |        2941    187.937M/s     
| nlohmann_json_ro |        1807    115.801M/s     |         337    21.5237M/s     
| yamlcpp_ro       |         261    16.6322M/s     |          25    1.58178M/s     
| libyaml_ro       |        1786    113.909M/s     |         560    35.6599M/s     
| libyaml_ro_reuse |        1797    114.594M/s     |         561    35.8531M/s     
| ryml_ro          |        6088    388.585M/s     |         576    36.8634M/s     
| ryml_rw          |        6179    393.658M/s     |         577    36.8474M/s     
| ryml_ro_reuse    |        6986    446.248M/s     |        1164    74.636M/s      
| ryml_rw_reuse    |        7157    457.076M/s     |        1175    74.8721M/s     
|------------------|-------------------------------|-------------------------------
```

You can verify that (at least for this test) ryml beats most json parsers at
their own game, with the notable exception
of [rapidjson](https://github.com/Tencent/rapidjson) --- *but this occurs
only in Release mode*. When in Debug
mode, [rapidjson](https://github.com/Tencent/rapidjson) is actually slower
than ryml, and only [sajson](https://github.com/chadaustin/sajson) manages to
be faster.

More json comparison benchmarks will be added, but seem unlikely to
significantly alter these results.


### Performance emitting

Emitting benchmarks were not created yet, but feedback from some users
reports as much as 25x speedup from yaml-cpp [(eg,
here)](https://github.com/biojppm/rapidyaml/issues/28#issue-553855608).

If you have data or YAML code for this, please submit a merge request, or
just send us the files!


------

## Installing and using

First, clone the repo:
```bash
git clone --recursive https://github.com/biojppm/rapidyaml
```
Take care to use the `--recursive` flag to force git to clone the 
submodules as well. If you omit `--recursive`, after cloning you
will have to do `git submodule init` and ` submodule update` 
to ensure the submodules are checked out.

Next, you can either use ryml as a cmake subdirectory or build and install to
a directory of your choice. Currently [cmake](https://cmake.org/) is required
for using ryml; we recommend a recent cmake version, at least 3.13.

### Using ryml as cmake subproject

ryml is a small library, so this is the advised way.
```cmake
# somewhere in your CMakeLists.txt

# this is the target you wish to link with ryml
add_library(foolib a.cpp b.cpp)

# make ryml a subproject of your project
add_subdirectory(path/to/rapidyaml ryml)
target_link_libraries(foolib PUBLIC ryml)  # that's it!

add_executable(fooexe main.cpp)
target_link_libraries(fooexe foolib) # brings in ryml
```
If you're using git, we also suggest you add ryml as git submodule of
your repo. This makes it easy to track any upstream changes in ryml.

### The traditional way: using an installed version

You can also use ryml in the customary cmake way, by first building and
installing it, and then consuming it in your project via `find_package()`.

First, build ryml. For Visual Studio & multi-configuration CMake generators,
this would be:
```bash
cmake -S path/to/rapidyaml -B path/to/ryml/build/dir \
      -DCMAKE_INSTALL_PREFIX=path/to/ryml/install/dir
cmake --build path/to/ryml/build/dir --parallel --config Release
```
whereas for single configuration CMake generators (Unix Makefiles, etc), this
would be:
```bash
cmake -S path/to/rapidyaml -B path/to/ryml/build/dir \
      -DCMAKE_INSTALL_PREFIX=path/to/ryml/install/dir \
      -DCMAKE_BUILD_TYPE=Release
cmake --build path/to/ryml/build/dir --parallel
```
(Note the `-S` and `-B` options first appeared in cmake 3.13 and are not
available in earlier cmake versions). Now you can install ryml:
```bash
cmake --build path/to/ryml/build/dir --target install
```

This will get ryml installed into the directory
`path/to/ryml/install/dir`, together with cmake export files for ryml,
which `find_package()` will need to successfully import ryml to your project.

Now to consume this installed ryml version, do the following:
```cmake
# somewhere in your CMakeLists.txt

# this is the target you wish to link with ryml
add_library(foolib a.cpp b.cpp)

# instruct cmake to search for ryml
find_package(ryml REQUIRED)
target_link_libraries(foolib PUBLIC ryml::ryml)  # NOTE namespace ryml::

add_executable(fooexe main.cpp)
target_link_libraries(fooexe foolib) # brings in ryml
```
Note a significant difference to the subdirectory approach from the previous
section: the installed ryml cmake exports file places the ryml library target in the
`ryml::` namespace.

Now when building your project, you will need to point cmake to the installed ryml.
To do this, simply add the ryml install directory to your project's `CMAKE_PREFIX_PATH` by doing eg
`-DCMAKE_PREFIX_PATH=path/to/ryml/install/dir` when configuring your project,
or by setting this variable in the cmake GUI if that's what you prefer to use.

You can also set (via command line or GUI) the variable `ryml_DIR` to the
directory where the exports file `rymlConfig.cmake` was installed (which is
different across platforms); search for this file in the ryml install tree,
and provide the directory where it is located. For example, in Windows with
the example above, this would be `-Dryml_DIR=path/to/ryml/install/dir/cmake`.


### cmake build settings for ryml
The following cmake variables can be used to control the build behavior of
ryml:

  * `RYML_DEFAULT_CALLBACKS=ON/OFF`. Enable/disable ryml's default
    implementation of error and allocation callbacks. Defaults to `ON`.
  * `RYML_STANDALONE=ON/OFF`. ryml uses
    [c4core](https://github.com/biojppm/c4core), a C++ library with low-level
    multi-platform utilities for C++. When `RYML_STANDALONE=ON`, c4core is
    incorporated into ryml as if it is the same library. Defaults to `ON`.

If you're developing ryml or just debugging problems with ryml itself, the
following variables can be helpful:
  * `RYML_DEV=ON/OFF`: a bool variable which enables development targets such as
    unit tests, benchmarks, etc. Defaults to `OFF`.
  * `RYML_DBG=ON/OFF`: a bool variable which enables verbose prints from
    parsing code; can be useful to figure out parsing problems. Defaults to
    `OFF`.


------

## Quick start

If you're wondering whether ryml's speed comes at a usage cost, you need
not. With ryml, you can have your cake and eat it too: being rapid is
definitely NOT the same as being unpractical! ryml was written with easy AND
efficient usage in mind, and comes with a two level API for accessing and
traversing the data tree.

The low-level interface is an index-based API available through
the [`ryml::Tree`](src/c4/yml/tree.hpp) class (see examples below). This
class is essentially a contiguous array of `NodeData` elements; these are
linked to parent, children and siblings via indices.

On top of this index-based API, there is a thin
abstraction [`ryml::NodeRef`](src/c4/yml/node.hpp) which is essentially a
non-owning pointer to a `NodeData` element. It provides convenient methods
for accessing the `NodeData` properties wrapping it via a class allowing for
a more object-oriented use.


### Parsing

A parser takes a source buffer and fills
a [`ryml::Tree`](src/c4/yml/tree.hpp) object:

```c++
#include <ryml.hpp>

// not needed by ryml, just for these examples (and below)
#include <iostream>
// convenience functions to print a node
void show_keyval(ryml::NodeRef n)
{
    assert(n.has_keyval());
    std::cout << n.key() << ": " << n.val() << "\n";
}
void show_val(ryml::NodeRef n)
{
    assert(n.has_val());
    std::cout << n.val() << "\n";
}
    
int main()
{
    // ryml can parse in situ (and read-only buffers too):
    char src[] = "{foo: 1, bar: [2, 3]}";
    ryml::substr srcview = src_; // a mutable view to the source buffer
    // there are also overloads for reusing the tree and parser
    ryml::Tree tree = ryml::parse(srcview);

    // get a reference to the "foo" node
    ryml::NodeRef node = tree["foo"];

    show_keyval(node);  // "foo: 1"
    show_val(node["bar"][0]);  // "2"
    show_val(node["bar"][1]);  // "3"

    // deserializing:
    int foo;
    node >> foo; // now foo == 1
}
```

It is also possible to parse read-only buffers, but note these will be copied
over to an arena buffer in the tree object, and that buffer copy will be the
one to be parsed:

```c++
// "{foo: 1}" is a const char[], so a read-only buffer; it will be
// copied to the tree's arena before parsing
ryml::Tree tree = ryml::parse("{foo: 1}");
```

When parsing, you can reuse the existing trees and parsers. You can also
parse into particular tree nodes, so that you can parse an entire file into a
node which is deep in the hierarchy of an existing tree. To see the various
parse overloads, consult the [c4/yml/parse.hpp header](src/c4/yml/parse.hpp).
The free-standing `parse()` functions (towards the end of the file) are just
convenience wrappers for calling the several `Parser::parse()` overloads.


### References: anchors and aliases

Note that dereferencing is opt-in; after parsing, you have to call
`Tree::resolve()` explicitly if you want resolved references in the
tree. This method will resolve all references and substitute the anchored
values in place of the reference.

The `Tree::resolve()` method first does a full traversal of the tree to
gather all anchors and references in a separate collection, then it goes
through that collection to locate the names, which it does by obeying the
YAML standard diktat that

    an alias node refers to the most recent node in the serialization having the specified anchor

So, depending on the number of anchor/alias nodes, this is a potentially
expensive operation, with a best-case linear complexity (from the initial
traversal) and a worst-case quadratic complexity (if every node has an
alias/anchor). This potential cost is the reason for requiring an explicit
call to `Tree::resolve()`.


### Traversing the tree

The data tree is an index-linked array of `NodeData` elements. These are
defined roughly as (browse the [c4/yml/tree.hpp header](src/c4/yml/tree.hpp)):

```c++
// (inside namespace c4::yml)

typedef enum : int // bitflags for marking node features
{
   KEY=1<<0,
   VAL=1<<1,
   MAP=1<<2,
   SEQ=1<<4,
   DOC=1<<5,
   TAG=...,
   REF=...,
   ANCHOR=..., // etc 
} NodeType_e;
struct NodeType
{
   NodeType_e m_flags;
   // ... predicate methods such as
   // has_key(), is_map(), is_seq(), etc
};
struct NodeScalar // this is both for keys and vals
{
    csubstr tag;
    csubstr scalar;
    csubstr anchor;
    // csubstr is a constant substring:
    // a non-owning read-only string view
    // consisting of a pointer and a length
}
constexpr const size_t NONE = (size_t)-1;
struct NodeData
{
    NodeType   type;
    NodeScalar key; // data for the key (if applicable)
    NodeScalar val; // data for the value
    
    size_t     parent;      // NONE when this is the root node
    size_t     first_child; // NONE if this is a leaf node
    size_t     last_child;  // etc
    size_t     next_sibling;
    size_t     prev_sibling;
}
```

Please note that you should not rely on this particular structure; the above
definitions are given only to provide an idea on how the tree is organized.
To access and modify node properties, please use the APIs provided through
the `Tree` (low-level) or the `NodeRef` (high-level) classes.

You may have noticed above the use of a `csubstr` class. This class is
defined in another library, [c4core](https://github.com/biojppm/c4core),
which is imported by ryml (so technically it's not a dependency, is
it?). This is a library I use with my projects consisting of multiplatform
low-level utilities. One of these is `c4::csubstr` (the name comes from
"constant substring") which is a non-owning read-only string view, with many
methods that make it practical to use (I would certainly argue more practical
than `std::string`). In fact, `c4::csubstr` and its writeable counterpart
`c4::substr` are the workhorses of the ryml parsing and serialization code;
you can browse these classes here:
[c4/substr.hpp](https://github.com/biojppm/c4core/blob/master/src/c4/substr.hpp).

Now, let's parse and traverse a tree. To obtain a `NodeRef` from the tree,
you only need to invoke `operator[]`. This operator can take indices (when
invoked on sequence and map nodes) and also strings (only when invoked on map
nodes):

```c++
ryml::Tree tree = ryml::parse("[a, b, {c: 0, d: 1}]");

// note: show_val() was defined above

show_val(tree[0]); // "a"
show_val(tree[1]); // "b"
show_val(tree[2][ 0 ]); // "0" // index-based
show_val(tree[2][ 1 ]); // "1" // index-based
show_val(tree[2]["c"]); // "0" // string-based
show_val(tree[2]["d"]); // "1" // string-based

// note that trying to obtain the value on a non-value
// node such as a container will fail an assert:
// ERROR, assertion triggered: a container has no value
show_val(tree[2]);
// ERROR: the same
show_val(tree.rootref());

// the same for keys:
show_keyval(tree[0]); // ERROR: sequence element has no key
show_keyval(tree[2][0]); // ok
```
The square bracket operators `Tree::operator[csubstr]` and
`Tree::operator[size_t]` do a lookup on the root node and return a
`NodeRef`. The first overload (valid only for map nodes) looks for a child having the given key, and the
second overload looks for the i-th root's child. If you prefer to stick to
the low level API, you can use `Tree::find_child()` which takes a node on
which the child should be looked for and also that child's key or position
within the parent.

Please note that since a ryml tree uses indexed linked lists for storing
children, the complexity of `Tree::operator[csubstr]` and
`Tree::operator[size_t]` is linear on the number of root children. If you use
it with a large tree where the root has many children, you may get a
performance hit. To avoid this hit, you can create your own accelerator
structure. For example, before doing a lookup, do a single traverse at the
root level to fill an `std::map<csubstr,size_t>` mapping key names to node
indices; with a node index, a lookup (via `Tree::get()`) is O(1), so this way
you can get O(log n) lookup from a key.

As for `NodeRef`, the difference from `NodeRef::operator[]`
to `Tree::operator[]` is that the latter refers to the root node, whereas
the former can be invoked on any node. But the lookup process is the same for
both and their algorithmic complexity is the same: they are both linear in
the number of direct children; but depending on the data, that number may
be very different from one to another.

Now, let's address how to mutate the tree via `operator[]`. We should stress
that there is an important difference to the mutability behavior of the STL's
`std::map::operator[]`. Consider when a non-existing key or index is
requested via `operator[]`. Unlike with `std::map`, **this operator does not
modify the tree**. Instead you get a seed-state `NodeRef`, and the tree will
be modified only when this seed-state reference is written to. Thus `NodeRef`
can either point to a valid tree node, or if no such node exists it will be
in seed-state by holding the index or name passed to `operator[]`. To allow
for this, `NodeRef` is a simple structure with a declaration like:

```c++
class NodeRef
{
    // a pointer to the tree
    Tree * m_tree; 
    // either the (tree-scoped) index of an existing node or the (node-scoped) index of a seed state
    size_t m_node_or_seed_id;
    // the key name of a seed state. null when valid
    const char* m_seed_name;

public:

    // this can be used to query whether a node is in seed state
    bool valid()
    {
        return m_node_or_seed_id != NONE
               &&
               m_seed_name == nullptr;
    }

    // forward all calls to m_tree. For example:
    csubstr val() const { assert(valid()); return m_tree->val(m_node_or_seed_id); }
    void set_val(csubstr v) { if(!valid()) {/*create node in tree*/;} m_tree->set_val(m_node_or_seed_id, v); }

    // etc...
};
```

To iterate over children:
```c++
for(NodeRef c : node.children())
{
    std::cout << c.key() << "---" << c.val() << "\n";
}
```

To iterate over siblings:
```c++
for(NodeRef c : node.siblings())
{
    std::cout << c.key() << "---" << c.val() << "\n";
}
```


### Creating a tree

To create a tree programatically:
```c++
ryml::Tree tree;
NodeRef r = tree.rootref();

// Each container node must be explicitly set (either MAP or SEQ):
r |= ryml::MAP;

r["foo"] = "1"; // ryml works only with strings.
// Note that the tree will be __pointing__ at the
// strings "foo" and "1" used here. You need
// to make sure they have at least the same
// lifetime as the tree.

// does not change the tree until s is written to.
ryml::NodeRef s = r["seq"]; // here, s is not valid()
s |= ryml::SEQ; // now s is valid()

s.append_child() = "bar0"; // this child is now __pointing__ at "bar0"
s.append_child() = "bar1";
s.append_child() = "bar2";

// emit to stdout (can also emit to FILE* or ryml::span)
emit(tree); // prints the following:
            // foo: 1
            // seq:
            //  - bar0
            //  - bar1
            //  - bar2

// serializing: using operator<< instead of operator=
// will make the tree serialize the value into a char
// arena inside the tree. This arena can be reserved at will.
int ch3 = 33, ch4 = 44;
s.append_child() << ch3;
s.append_child() << ch4;

{
    std::string tmp = "child5";
    s.append_child() << tmp;   // requires #include <c4/yml/std/string.hpp>
    // now tmp can go safely out of scope, as it was
    // serialized to the tree's internal string arena
    // Note the include highlighted above is required so that ryml
    // knows how to turn an std::string into a c4::csubstr/c4::substr.
}

emit(tree); // now prints the following:
            // foo: 1
            // seq:
            //  - bar0
            //  - bar1
            //  - bar2
            //  - 33
            //  - 44
            //  - child5

// to serialize keys:
r.append_child() << ryml::key(66) << 7;

emit(tree); // now prints the following:
            // foo: 1
            // seq:
            //  - bar0
            //  - bar1
            //  - bar2
            //  - 33
            //  - 44
            //  - child5
            // 66: 7
}
```




### Low-level API

The low-level api is an index-based API accessible from
the [`ryml::Tree`](src/c4/yml/tree.hpp) object. Here are some examples:

```c++
void print_keyval(Tree const& t, size_t elm_id)
{
    std::cout << t.get_key(elm_id)
              << ": "
              << t.get_val(elm_id) << "\n";
}

ryml::Tree t = parse("{foo: 1, bar: 2, baz: 3}")

size_t root_id = t.root_id();
size_t foo_id  = t.first_child(root_id);
size_t bar_id  = t.next_sibling(foo_id);
size_t baz_id  = t.last_child(root_id);

assert(baz == t.next_sibling(bar_id));
assert(bar == t.prev_sibling(baz_id));

print_keyval(t, foo_id); // "foo: 1"
print_keyval(t, bar_id); // "bar: 2"
print_keyval(t, baz_id); // "baz: 3"

// to iterate over the children of a node:
for(size_t i  = t.first_child(root_id);
           i != ryml::NONE;
           i  = t.next_sibling(i))
{
    // ...
}

// to iterate over the siblings of a node:
for(size_t i  = t.first_sibling(foo_id);
           i != ryml::NONE;
           i  = t.next_sibling(i))
{
    // ...
}
```


### Custom types

ryml provides code to serialize the basic intrinsic types (integers, floating
points and strings): you can see it in the [the `c4/charconv.hpp`
header](https://github.com/biojppm/c4core/blob/master/src/c4/charconv.hpp). For
types other than these, you need to instruct ryml how to serialize your
type, and here we explain how to do this.

There are two distinct type categories when serializing to a YAML tree:
leaf types (value or key-value) and container types (sequences or maps).


#### Leaf types

These are types which should serialize to a string, resulting in a leaf node
in the YAML tree.

For these, overload the `to_chars(c4::substr, T)/from_chars(c4::csubstr, *T)`
functions.

Here's an example for a 3D vector type:
```c++
struct vec3 { float x, y, z; };

// format v to the given string view + return the number of
// characters written into it. The view size (buf.len) must
// be strictly respected. Return the number of characters
// that need to be written. So if the return value
// is larger than buf.len, ryml will resize the buffer and
// call this again with a larger buffer of the correct sizeXS.
size_t to_chars(c4::substr buf, vec3 v)
{
    // this call to c4::format() is the type-safe equivalent
    // of snprintf(buf.str, buf.len, "(%f,%f,%f)", v.x, v.y, v.z)
    return c4::format(buf, "({},{},{})", v.x, v.y, v.z);
}

bool from_chars(c4::csubstr buf, vec3 *v)
{
    // equivalent to sscanf(buf.str, "(%f,%f,%f)", &v->x, &v->y, &v->z)
    // --- actually snscanf(buf.str, buf.len, ...) but there's
    // no such function in the standard.
    size_t ret = c4::unformat(buf, "({},{},{})", v->x, v->y, v->z);
    return ret != c4::csubstr::npos;
}
```
Now you can provide your formats with
For a live example you can look at [the `std::string` serialization code](https://github.com/biojppm/c4core/blob/master/src/c4/std/string.hpp).


#### Container types

These are types requiring child nodes (ie, either sequences or maps).

For these, overload the `write()/read()` functions. For example,
```c++
namespace foo {
struct MyStruct; // a container-type struct
{
    int subject;
    std::map<std::string, int> counts;
};
  
// ... will need these functions to convert to YAML:
void write(c4::yml::NodeRef *n, MyStruct const& v);
void  read(c4::yml::NodeRef const& n, MyStruct *v);
} // namespace foo
```
which could be implemented as:
```c++
// include the functions for std::string and std::map (not included by default)
#include <c4/yml/std/map.hpp>
#include <c4/yml/std/string.hpp>
  
void foo::read(c4::yml::NodeRef const& n, MyStruct *v)
{
    n["subject"] >> v->subject;
    n["counts"] >> v->counts;
}
  
void foo::write(c4::yml::NodeRef *n, MyStruct const& v)
{
    *n |= c4::yml::MAP;

    NodeRef ch = n->append_child();
    ch.set_key("subject");
    ch.set_val_serialized(v.subject);

    ch = n->append_child();
    ch.set_key("counts");
    write(&ch, v.counts);
}
```
To harness [C++'s ADL rules](http://en.cppreference.com/w/cpp/language/adl),
it is important to overload these functions in the namespace of the type
you're serializing (or in the c4::yml namespace). Generic
examples can be seen in the (optional) implementations of `std::vector`
or `std::map`, at their respective headers
[`c4/yml/std/vector.hpp`](src/c4/yml/std/vector.hpp) and
[`c4/yml/std/map.hpp`](src/c4/yml/std/map.hpp).



### STL interoperation

ryml does not use the STL internally, but you can use ryml to serialize and
deserialize STL containers. That is, the use of STL is opt-in: you need to
`#include` the proper ryml header for the container you want to serialize, or
provide an implementation of your own, as above. Having done that, you can
serialize / deserialize your containers with a single step. For example:

```c++
#include <ryml_std.hpp> // include this before any other ryml header
#include <ryml.hpp>
int main()
{
    std::map<std::string, int> m({{"foo", 1}, {"bar", 2}});
    ryml::Tree t;
    t.rootref() << m; // serialization of the map happens here
    
    emit(t);
    // foo: 1
    // bar: 2
    
    t["foo"] << 1111;  // serialize an integer into
                       // the tree's arena, and make
                       // foo's value point at it
    t["bar"] << 2222;  // the same, but for bar
    
    emit(t);
    // foo: 1111
    // bar: 2222
    
    m.clear();
    t.rootref() >> m; // deserialization of the map happens here

    assert(m["foo"] == 1111); // ok
    assert(m["bar"] == 2222); // ok
}
```
The [`<ryml_std.hpp>`](src/ryml_std.hpp) header includes every std type
implementation available in ryml. But you can include just a specific header
if you are interested only in a particular container; these headers are
located under a specific directory in the ryml source
folder: [c4/yml/std](src/c4/yml/std). You can browse them to learn how to
implement your custom type: for containers, see for example
[the `std::vector` implementation](src/c4/yml/std/vector.hpp),
or [the `std::map` implementation](src/c4/yml/std/map.hpp); for an example
of value nodes, see
[the `std::string` implementation](https://github.com/biojppm/c4core/src/c4/std/string.hpp).
If you'd like to see a particular STL container implemented, feel free to
[submit a pull request or open an issue](https://github.com/biojppm/rapidyaml/issues).

The need for separate inclusion of ryml's std interoperation headers is dictated
by ryml's design requirement of not forcing clients to use the STL. 

Please take note of the following pitfall when using the std headers: you have to include
the std header before any other headers that use functions from it. For example:
```c++
// the to_csubstr(std::string const&) overload is not found in the resolution set
#include <ryml.hpp>
#include <ryml_std.hpp>
int main()
{
    std::string in = R"({"a":"b","c":null,"d":"e"})";
    // COMPILE ERROR: to_csubstr() not found for std::string
    std::string yaml = ryml::preprocess_json<std::string>(c4::to_csubstr(in));
}
```
But this works:
```c++
#include <ryml_std.hpp> // note the inclusion order changed
#include <ryml.hpp>
int main()
{
    std::string in = R"({"a":"b","c":null,"d":"e"})";
    // OK:
    std::string yaml = ryml::preprocess_json<std::string>(c4::to_csubstr(in));
}
```
This constraint also applies to the conversion functions for your types;
just like with the STL's headers, they should be included prior to
ryml's headers.


### Custom formatting for intrinsic types

Sometimes the general formatting from ryml may not be what is required.

Consider the following:
```c++
NodeRef r = tree.rootref();

bool t = true;
float a = 24.0f;
float b = 2.41f;

r["t"] << t;
r["a"] << a;
r["b"] << b;
print_keyval(r["t"]); // "t=1" -- true was formatted as an int
print_keyval(r["a"]); // "a=24" -- the decimal was lost with general formatting
print_keyval(r["b"]); // "b=2.41" -- as expected
```
The behavior above may not be ideal in some cases. There are alternatives for
this situation:
```c++
// if you want the decimal to remain, you can provide the string yourself:
r["t"] << (t ? "true" : "false");
std::string sa = ...; // something resulting in "24.0"
r["a"] << c4::to_csubstr(sa);

print_keyval(r["t"]); // "t=true" -- as expected
print_keyval(r["a"]); // "a=24.0" -- as expected
print_keyval(r["b"]); // "b=2.41" -- as expected
```
If, understandably, you want to avoid the likely allocation caused
by the `std::to_string()` antipattern (or even worse, the `std::stringstream::str()`
allocation cookie monster), [ryml has you covered](https://github.com/biojppm/c4core/tree/master/src/c4/format.hpp):
```c++
#include <c4/format.hpp> // look for the appropriate formatting functions in this header
//...

int precision = 2; // print floats with two digits.
r["a"] << c4::fmt::real(val, precision); // OK, result: "24.00"

// c4::fmt::real() is a lazy marker which will be used by ryml to format the
// float directly in the arena without any extra allocation (other than
// possible arena growth, which would happen just the same for the approach
// above). It calls ftoa() on the arena range.
```
Again, note that you don't have to use the tree's arena. If you use
`operator=`, the `csubstr` you provide will be directly used instead.


### Custom allocators and error handlers

ryml accepts your own allocators and error handlers. Read through [this
header file](src/c4/yml/common.hpp) to set it up.

Please note the following about the use of custom allocators with ryml. If
you use static ryml trees or parsers, you need to make sure that their
allocator has the same lifetime. So you can't use ryml's default allocator,
as it is declared in a ryml file, and the standard provides no guarantee on
the relative initialization order, such that the allocator is constructed
before and destroyed after your variables (in fact you are pretty much
guaranteed to see this fail). So please carefully consider your choices, and
ponder whether you really need to use ryml static trees and parsers. If you
do need this, then you will need to declare and use an allocator from a ryml
memory resource that outlives the tree and/or parser.


### Using ryml to parse JSON, and preprocessing functions

Although JSON is generally a subset of YAML, [there is an exception that is
valid JSON, but not valid YAML](https://stackoverflow.com/questions/42124227/why-does-the-yaml-spec-mandate-a-space-after-the-colon):
```yaml
{"a":"b"}  # note the missing space after the semicolon
```
As a result, you will get a parse error if you try to do this:
```c++
auto tree = ryml::parse("{\"a\":\"b\"}");
```
This behavior is intended, and this was chosen to save added complexity in
the parser code.

However, you can still parse this with ryml if (prior to parsing) you
preprocess the JSON into valid YAML, adding the missing spaces after the
semicolons. ryml provides a freestanding function to do this:
`ryml::preprocess_json()`:

```c++
#include <c4/yml/preprocess.hpp>
// you can also use in-place overloads
auto yaml = ryml::preprocess_json<std::string>("{\"a\":\"b\"}");
// now you have a buffer with valid yaml - note the space:
std::cout << yaml << "\n"; // {"a": "b"}
// ... which you can parse:
ryml::Tree t = ryml::parse(to_substr(yaml));
std::cout << t["a"] << "\n"; // b
```

There is also `ryml::preprocess_rxmap()`, a function to convert non-standard
relaxed maps (ie, keys with implicit true values) into standard YAML maps.

```c++
#include <c4/yml/preprocess.hpp>
// you can also use in-place overloads
auto yaml = ryml::preprocess_rxmap<std::string>("{a, b, c, d: [e, f, g]}");
std::cout << yaml << "\n"; // {a: 1, b: 1, c: 1, d: [e, f, g]}
ryml::Tree t = ryml::parse(to_substr(yaml));
std::cout << t["a"] << "\n"; // 1
```


------

## Other languages

One of the aims of ryml is to provide an efficient YAML API for other
languages. There's already a cursory implementation for Python (using only
the low-level API). After ironing out the general approach, other languages
are likely to follow: probably (in order) JavaScript, C#, Java, Ruby, PHP,
Octave and R (all of this is possible because we're
using [SWIG](http://www.swig.org/), which makes it easy to do so).

### Python

(Note that this is a work in progress. Additions will be made and things will
be changed.) With that said, here's an example of the Python API:

```python
import ryml

# because ryml does not take ownership of the source buffer
# ryml cannot accept strings; only bytes or bytearrays
src = b"{HELLO: a, foo: b, bar: c, baz: d, seq: [0, 1, 2, 3]}"

def check(tree):
    # for now, only the index-based low-level API is implemented
    assert tree.size() == 10
    assert tree.root_id() == 0
    assert tree.first_child(0) == 1
    assert tree.next_sibling(1) == 2
    assert tree.first_sibling(5) == 2
    assert tree.last_sibling(1) == 5
    # use bytes objects for queries
    assert tree.find_child(0, b"foo") == 1
    assert tree.key(1) == b"foo")
    assert tree.val(1) == b"b")
    assert tree.find_child(0, b"seq") == 5
    assert tree.is_seq(5)
    # to loop over children:
    for i, ch in enumerate(ryml.children(tree, 5)):
        assert tree.val(ch) == [b"0", b"1", b"2", b"3"][i]
    # to loop over siblings:
    for i, sib in enumerate(ryml.siblings(tree, 5)):
        assert tree.key(sib) == [b"HELLO", b"foo", b"bar", b"baz", b"seq"][i]
    # to walk over all elements
    visited = [False] * tree.size()
    for n, indentation_level in ryml.walk(tree):
        # just a dumb emitter
        left = "  " * indentation_level
        if tree.is_keyval(n):
           print("{}{}: {}".format(left, tree.key(n), tree.val(n))
        elif tree.is_val(n):
           print("- {}".format(left, tree.val(n))
        elif tree.is_keyseq(n):
           print("{}{}:".format(left, tree.key(n))
        visited[inode] = True
    assert False not in visited
    # NOTE about encoding!
    k = tree.get_key(5)
    print(k)  # '<memory at 0x7f80d5b93f48>'
    assert k == b"seq"               # ok, as expected
    assert k != "seq"                # not ok - NOTE THIS! 
    assert str(k) != "seq"           # not ok
    assert str(k, "utf8") == "seq"   # ok again

# parse immutable buffer
tree = ryml.parse(src)
check(tree) # OK

# also works, but requires bytearrays or
# objects offering writeable memory
mutable = bytearray(src)
tree = ryml.parse_in_situ(mutable)
check(tree) # OK
```

As expected, the performance results so far are encouraging. In
a [timeit benchmark](api/python/parse_bm.py) compared
against [PyYaml](https://pyyaml.org/)
and [ruamel.yaml](https://yaml.readthedocs.io/en/latest/), ryml parses
quicker by a factor of 30x-50x:

```
+-----------------------+-------+----------+---------+----------------+
| case                  | iters | time(ms) | avg(ms) | avg_read(MB/s) |
+-----------------------+-------+----------+---------+----------------+
| parse:RuamelYaml      |    88 | 800.483  |  9.096  |      0.234     |
| parse:PyYaml          |    88 | 541.370  |  6.152  |      0.346     |
| parse:RymlRo          |  3888 | 776.020  |  0.200  |     10.667     |
| parse:RymlRoReuse     |  1888 | 381.558  |  0.202  |     10.535     |
| parse:RymlRw          |  3888 | 775.121  |  0.199  |     10.679     |
| parse:RymlRwReuse     |  3888 | 774.534  |  0.199  |     10.687     |
+-----------------------+-------+----------+---------+----------------+
```

(Note that the results above are somewhat biased towards ryml, because it does
not perform any type conversions: return types are merely `memoryviews` to
the source buffer.)


------

## YAML standard conformance

ryml is under active development, but is close to feature complete. The
following YAML core features are well covered in the unit tests:
* mappings
* sequences
* complex keys
* literal blocks
* quoted scalars
* tags
* anchors and references
* UTF8 is expected to mostly work
  
Of course, there are many dark corners in YAML, and there certainly can
appear cases which ryml fails to parse. Your [bug reports or pull
requests!](https://github.com/biojppm/rapidyaml/issues) are very welcome.

See also [the roadmap](./ROADMAP.md) for a list of future work.


### Test suite status

Integration of the >300 cases in the [YAML test
suite](https://github.com/yaml/yaml-test-suite) is ongoing work. Each of
these tests have several subparts:
 * in-yaml: mildly, plainly or extremely difficult-to-parse yaml
 * in-json: equivalent json (where possible/meaningful)
 * out-yaml: equivalent standard yaml
 * events: equivalent libyaml events allowing to establish correctness of
   the parsed results

When testing, ryml tries to parse each of the 3 yaml/json parts. If the
parsing suceeds, then the ryml test will emit the parsed tree, then parse the
emitted result and verify that emission is idempotent, ie that the emitted
result is the same as its input without any loss of information. To ensure
correctness, this happens over four levels of parse/emission pairs, resulting
on ~200 checks per test case.

Please note that in [their own words](http://matrix.yaml.io/), the tests from
the YAML test suite *contain a lot of edge cases that don't play such an
important role in real world examples*. Despite the extreme focus of the test
suite, as of May 2020, ryml only fails to parse ~30 out of the ~1000=~3x300
cases from the test suite. Out of all other cases, all the ~200 checks per
case are 100% successful for consistency over parse/emit pairs --- but please
note that the events part is not yet read in and used to check for
correctness, and therefore that **even though ryml may suceed in parsing,
there still exists a minority of cases which may not be correct**. Currently,
I would estimate this fraction at somewhere around 5%. These are the suite
cases where ryml fails to parse any of its subparts:
[EXG3](https://github.com/yaml/yaml-test-suite/tree/master/test/EXG3.tml),
[M7A3](https://github.com/yaml/yaml-test-suite/tree/master/test/M7A3.tml),
[735Y](https://github.com/yaml/yaml-test-suite/tree/master/test/735Y.tml),
[82AN](https://github.com/yaml/yaml-test-suite/tree/master/test/82AN.tml),
[9YRD](https://github.com/yaml/yaml-test-suite/tree/master/test/9YRD.tml),
[EX5H](https://github.com/yaml/yaml-test-suite/tree/master/test/EX5H.tml),
[HS5T](https://github.com/yaml/yaml-test-suite/tree/master/test/HS5T.tml),
[7T8X](https://github.com/yaml/yaml-test-suite/tree/master/test/7T8X.tml),
[RZP5](https://github.com/yaml/yaml-test-suite/tree/master/test/RZP5.tml),
[FH7J](https://github.com/yaml/yaml-test-suite/tree/master/test/FH7J.tml),
[PW8X](https://github.com/yaml/yaml-test-suite/tree/master/test/PW8X.tml),
[CN3R](https://github.com/yaml/yaml-test-suite/tree/master/test/CN3R.tml),
[6BCT](https://github.com/yaml/yaml-test-suite/tree/master/test/6BCT.tml),
[G5U8](https://github.com/yaml/yaml-test-suite/tree/master/test/G5U8.tml),
[K858](https://github.com/yaml/yaml-test-suite/tree/master/test/K858.tml),
[NAT4](https://github.com/yaml/yaml-test-suite/tree/master/test/NAT4.tml),
[9MMW](https://github.com/yaml/yaml-test-suite/tree/master/test/9MMW.tml),
[DC7X](https://github.com/yaml/yaml-test-suite/tree/master/test/DC7X.tml),
[L94M](https://github.com/yaml/yaml-test-suite/tree/master/test/L94M.tml),

Except for the known limitations listed next, all other suite cases are
expected to work.


--------- 

## Known limitations

ryml makes no effort to follow the standard in the following situations:

* `%YAML` directives have no effect and are ignored.
* `%TAG` directives have no effect and are ignored. All schemas are assumed
  to be the default YAML 2002 schema.
* container elements are not accepted as mapping keys. keys must be
  simple strings and cannot themselves be mappings or sequences. But mapping
  values can be any of the above. [YAML test
  suite](https://github.com/yaml/yaml-test-suite) cases:
  [4FJ6](https://github.com/yaml/yaml-test-suite/tree/master/test/4FJ6.tml),
  [6BFJ](https://github.com/yaml/yaml-test-suite/tree/master/test/6BFJ.tml),
  [6PBE](https://github.com/yaml/yaml-test-suite/tree/master/test/6PBE.tml),
  [6PBE](https://github.com/yaml/yaml-test-suite/tree/master/test/6PBE.tml),
  [KK5P](https://github.com/yaml/yaml-test-suite/tree/master/test/KK5P.tml),
  [KZN9](https://github.com/yaml/yaml-test-suite/tree/master/test/KZN9.tml),
  [LX3P](https://github.com/yaml/yaml-test-suite/tree/master/test/LX3P.tml),
  [M5DY](https://github.com/yaml/yaml-test-suite/tree/master/test/M5DY.tml),
  [Q9WF](https://github.com/yaml/yaml-test-suite/tree/master/test/Q9WF.tml),
  [SBG9](https://github.com/yaml/yaml-test-suite/tree/master/test/SBG9.tml),
  [X38W](https://github.com/yaml/yaml-test-suite/tree/master/test/X38W.tml),
  [XW4D](https://github.com/yaml/yaml-test-suite/tree/master/test/XW4D.tml).


------

## Alternative libraries

Why this library? Because none of the existing libraries was quite what I
wanted. There are two C/C++ libraries that I know of:

* [libyaml](https://github.com/yaml/libyaml)
* [yaml-cpp](https://github.com/jbeder/yaml-cpp)

The standard [libyaml](https://github.com/yaml/libyaml) is a bare C
library. It does not create a representation of the data tree, so it can't
qualify as practical. My initial idea was to wrap parsing and emitting around
libyaml, but to my surprise I found out it makes heavy use of allocations and
string duplications when parsing. I briefly pondered on sending PRs to reduce
these allocation needs, but not having a permanent tree to store the parsed
data was too much of a downside.

[yaml-cpp](https://github.com/jbeder/yaml-cpp) is full of functionality, but
is heavy on the use of node-pointer-based structures like `std::map`,
allocations, string copies and slow C++ stream serializations. This is
generally a sure way of making your code slower, and strong evidence of this
can be seen in the benchmark results above.

When performance and low latency are important, using contiguous structures
for better cache behavior and to prevent the library from trampling over the
client's caches, parsing in place and using non-owning strings is of central
importance. Hence this Rapid YAML library which, with minimal compromise,
bridges the gap from efficiency to usability. This library takes inspiration
from [RapidJSON](https://github.com/Tencent/rapidjson)
and [RapidXML](http://rapidxml.sourceforge.net/).


------
## License

ryml is permissively licensed under the [MIT license](LICENSE.txt).
