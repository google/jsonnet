# Jsonnet - The data templating language

[![Build Status](https://travis-ci.org/google/jsonnet.svg?branch=master)](https://travis-ci.org/google/jsonnet)

For an introduction to Jsonnet and documentation,
[visit our website](http://jsonnet.org).

Visit our [discussion forum](https://groups.google.com/forum/#!forum/jsonnet).

## Building Jsonnet

The GCC C++ (g++) compiler is required to build Jsonnet. Clang is also
supported.

### Makefile

To build jsonnet with g++, run:

```
make
```

To run the output binary, run:

```
./jsonnet
```

### Bazel

Bazel builds are also supported.
[Install Bazel](https://www.bazel.io/versions/master/docs/install.html) if it is
not installed already. Then, run the following command to build:

```
bazel build -c opt //cmd:jsonnet
```

This builds the `jsonnet` target defined in [cmd/BUILD](./cmd/BUILD) To run the
output binary, run:

```
bazel-bin/cmd/jsonnet
```


## Contributing

See the [contributing page](http://jsonnet.org/contributing.html) on our website.
