# c4core - C++ core utilities

[![MIT Licensed](https://img.shields.io/badge/License-MIT-green.svg)](https://github.com/biojppm/c4core/blob/master/LICENSE.txt)
[![ci](https://github.com/biojppm/c4core/workflows/ci/badge.svg)](https://github.com/biojppm/c4core/actions?query=ci)
[![Coverage: coveralls](https://coveralls.io/repos/github/biojppm/c4core/badge.svg)](https://coveralls.io/github/biojppm/c4core)
[![Coverage: codecov](https://codecov.io/gh/biojppm/c4core/branch/master/graph/badge.svg)](https://codecov.io/gh/biojppm/c4core)
[![LGTM alerts](https://img.shields.io/lgtm/alerts/g/biojppm/c4core.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/biojppm/c4core/alerts/)
[![LGTM grade: C/C++](https://img.shields.io/lgtm/grade/cpp/g/biojppm/c4core.svg?logo=lgtm&logoWidth=18)](https://lgtm.com/projects/g/biojppm/c4core/context:cpp)


c4core is a library of low-level C++ utilities, written with low-latency
projects in mind.

Some of the utilities provided by c4core have already equivalent
functionality in the C++ standard, but they are provided as the existing C++
equivalent may be insufficient (eg, std::string_view), inefficient (eg,
std::string), heavy (eg streams), or plainly unusable on some
platforms/projects, (eg exceptions); some other utilities have equivalent
under consideration for C++ standardisation; and yet some other utilities
have (to my knowledge) no equivalent under consideration. Be that as it may,
I've been using these utilities in this or similar forms for some years now,
and I've found them incredibly useful in my projects. I'm packing these as a
separate library, as all of my projects use it.


## Obtaining c4core

c4core uses git submodules. It is best to clone c4core with the `--recursive`
option:

```bash
# using --recursive makes sure git submodules are also cloned at the same time
git clone --recursive https://github.com/biojppm/c4core
```

If you ommit the `--recursive` option, then after cloning you will have to
make git checkout the current version of the submodules, using `git submodule
init` followed by `git submodule update`.


## Using c4core in your project

c4core is built with cmake, and assumes you also use cmake. Although c4core
is NOT header-only, and currently has no install target, you can very easily
use c4core in your project by using
`add_subdirectory(${path_to_c4core_root})` in your CMakeLists.txt; this will
add c4core as a subproject of your project. Doing this is not intrusive to
your cmake project because c4core is fast to build (typically under 10s), and
it also prefixes every cmake variable with `C4CORE_`. But more importantly
this will enable you to compile c4core with the exact same compile settings
used by your project.

Here's a very quick complete example of setting up your project to use
c4core:

```cmake
project(foo)

add_subdirectory(c4core)

add_library(foo foo.cpp)
target_link_libraries(foo PUBLIC c4core) # that's it!
```

Note above that the call to `target_link_libraries()` is using PUBLIC
linking. This is required to make sure the include directories from `c4core`
are transitively used.


## Quick tour

All of the utilities in this library are under the namespace `c4`; any
exposed macros use the prefix `C4_`: eg `C4_ASSERT()`.

### Multi-platform / multi-compiler utilities

```c++
// TODO: elaborate on the topics:
#include <c4/error.hpp>

C4_LIKELY()/C4_UNLIKELY()

C4_RESTRICT, $, c$, $$, c$$
#include <c4/restrict.hpp>
#include <c4/unrestrict.hpp>

#include <c4/windows_push.hpp>
#include <c4/windows_pop.hpp>

C4_UNREACHABLE()

c4::type_name()
```

### Runtime assertions and error handling

```c++
// TODO: elaborate on the topics:

error callback

C4_ASSERT()
C4_XASSERT()
C4_CHECK()

C4_ERROR()
C4_NOT_IMPLEMENTED()
```

### Memory allocation

```c++
// TODO: elaborate on the topics:

c4::aalloc(), c4::afree() // aligned allocation

c4::MemoryResource // global and scope

c4::Allocator
```

### Mass initialization/construction/destruction

```c++
// TODO: elaborate on the topics:

c4::construct()/c4::construct_n()

c4::destroy()/c4::destroy_n()

c4::copy_construct()/c4::copy_construct_n()

c4::copy_assign()/c4::copy_assign_n()

c4::move_construct()/c4::move_construct_n()

c4::move_assign()/c4::move_assign_n()

c4::make_room()/c4::destroy_room()
```


### Writeable string views: c4::substr and c4::csubstr

Here: [`#include <c4/substr.hpp>`](src/c4/substr.hpp)


### Value <-> character interoperation

Here: [`#include <c4/charconv.hpp>`](src/c4/charconv.hpp)

```c++
// TODO: elaborate on the topics:

c4::utoa(), c4::atou()
c4::itoa(), c4::atoi()
c4::ftoa(), c4::atof()
c4::dtoa(), c4::atod()

c4::to_chars(), c4::from_chars()
c4::to_chars_sub()
c4::to_chars_first()
```

### String formatting and parsing

* [`#include <c4/format.hpp>`](src/c4/format.hpp)

```c++
// TODO: elaborate on the topics:

c4::cat(), c4::uncat()

c4::catsep(), c4::uncatsep()

c4::format(), c4::unformat()

// formatting:
c4::raw, c4::craw
```

### `c4::span` and `c4::blob`

* [`#include <c4/span.hpp>`](src/c4/span.hpp)
* [`#include <c4/blob.hpp>`](src/c4/blob.hpp)


### Enums and enum symbols

[`#include <c4/enum.hpp>`](src/c4/enum.hpp)

```c++
// TODO: elaborate on the topics:

c4::e2str(), c4::str2e()
```

### Bitmasks and bitmask symbols

[`#include <c4/bitmask.hpp>`](src/c4/bitmask.hpp)

```c++
// TODO: elaborate on the topics:

c4::bm2str(), c4::str2bm()
```

### Base64 encoding / decoding

[`#include <c4/base64.hpp>`](src/c4/base64.hpp)

### Fuzzy float comparison
