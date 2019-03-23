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

### Bazel

Bazel builds are also supported.
Install [Bazel](https://www.bazel.io/versions/master/docs/install.html) if it is
not installed already. Then, run the following command to build with GCC:

```
bazel build -c opt //cmd:jsonnet
```

To build with Clang, use one of these two options:

```
env CC=clang CXX=clang++ bazel build -c opt //cmd:jsonnet

# OR

bazel build -c opt --action_env=CC=clang --action_env=CXX=clang++ //cmd:jsonnet
```

This builds the `jsonnet` target defined in [`cmd/BUILD`](./cmd/BUILD). To
launch the output binary, run:

```
bazel-bin/cmd/jsonnet
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

```
make && make libjsonnet.so &&  ./tests.sh
```


### Locally serving the website

First, you need to build the JavaScript version of Jsonnet (which requires [_emscripten_](https://emscripten.org/)):
```
make doc/js/libjsonnet.js
```

Then, inside the _doc_ directory, you can generate and serve the website using [Jekyll](https://jekyllrb.com/):
```
jekyll serve --watch --livereload
```
Or you can do the same more concisely:
```
jekyll s -w -l
```

This should serve the website, automatically rebuild when you change underlying files and automatically
refresh your browser when that happens.
