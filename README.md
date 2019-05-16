# Jsonnet - The data templating language

[![Build Status](https://travis-ci.org/google/jsonnet.svg?branch=master)](https://travis-ci.org/google/jsonnet)

For an introduction to Jsonnet and documentation,
[visit our website](http://jsonnet.org).

Visit our [discussion forum](https://groups.google.com/forum/#!forum/jsonnet).

## Building Jsonnet

You can use either GCC or Clang to build Jsonnet. Note that on recent versions
of macOS, `/usr/bin/gcc` and `/usr/bin/g++` are actually Clang, so there is no
difference.

### Makefile

To build Jsonnet with GCC, run:

```
make
```

To build Jsonnet with Clang, run:

```
make CC=clang CXX=clang++
```

To run the output binary, run:

```
./jsonnet
```

To run the reformatter, run:

```
./jsonnetfmt
```

### Bazel

Bazel builds are also supported.
Install [Bazel](https://www.bazel.io/versions/master/docs/install.html) if it is
not installed already. Then, run the following command to build with GCC:

```
bazel build -c opt //cmd:all
```

To build with Clang, use one of these two options:

```
env CC=clang CXX=clang++ bazel build -c opt //cmd:all

# OR

bazel build -c opt --action_env=CC=clang --action_env=CXX=clang++ //cmd:all
```

This builds the `jsonnet` and `jsonnetfmt` targets defined in [`cmd/BUILD`](./cmd/BUILD). To launch
the output binaries, run:

```
bazel-bin/cmd/jsonnet
bazel-bin/cmd/jsonnetfmt
```


### Cmake


```
cmake . -Bbuild
```

```
cmake --build build --target run_tests
```

## Contributing

See the [contributing page](http://jsonnet.org/contributing.html) on our website.


## Developing Jsonnet

### Running tests

To run the comprehensive suite:

```
make test
```


### Locally serving the website

First, you need to build the JavaScript version of Jsonnet (which requires
[_emscripten_](https://emscripten.org/)):

```
make doc/js/libjsonnet.js
```

Then, from the root of the repository you can generate and serve the website using
[Jekyll](https://jekyllrb.com/):

```
tools/scripts/serve_docs.sh
```

This should the website on localhost:8080, automatically rebuild when you change any underlying
files, and automatically refresh your browser when that happens.
