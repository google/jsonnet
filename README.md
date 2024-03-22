# Jsonnet - The data templating language

![master branch build status badge](https://github.com/google/jsonnet/actions/workflows/build_and_test.yml/badge.svg?event=push&branch=master)

For an introduction to Jsonnet and documentation,
[visit our website](https://jsonnet.org).

This repository contains the original implementation. You can also try [go-jsonnet](https://github.com/google/go-jsonnet), a newer implementation which in some cases is orders of magnitude faster.

Visit our [discussion forum](https://groups.google.com/g/jsonnet).

## Packages

Jsonnet is available on Homebrew:

```
brew install jsonnet
```

[Jsonnet](https://packages.msys2.org/base/mingw-w64-jsonnet) is available on [MSYS2](https://www.msys2.org/):

```
pacman -S mingw-w64-clang-i686-jsonnet
```
```
pacman -S mingw-w64-clang-x86_64-jsonnet
```
```
pacman -S mingw-w64-i686-jsonnet
```
```
pacman -S mingw-w64-x86_64-jsonnet
```
```
pacman -S mingw-w64-ucrt-x86_64-jsonnet
```

The Python binding is on pypi:

```
pip install jsonnet
```

You can also download and install Jsonnet using the [vcpkg](https://github.com/Microsoft/vcpkg/)
dependency manager:

```
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
vcpkg install jsonnet
```

The Jsonnet port in vcpkg is kept up to date by Microsoft team members and community contributors.
If the version is out of date, please [create an issue or pull
request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.

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

See the [contributing page](https://jsonnet.org/learning/community.html#license) on our website.


## Developing Jsonnet

### Running tests

To run the comprehensive suite:

```
make test
```


### Locally serving the website

You need a `doc/js/libjsonnet.wasm` which can either be downloaded from the
production website:

```
wget https://jsonnet.org/js/libjsonnet.wasm -O doc/js/libjsonnet.wasm
```

Or you can build it yourself, which requires checking out
[go-jsonnet](https://github.com/google/go-jsonnet).  See the README.md in
that repo for instructions.

The standard library is documented in a structured format in `doc/_stdlib_gen/stdlib-content.jsonnet`.
The HTML (input for Jekyll) is regenerated using the following command:

```
tools/scripts/update_web_content.sh
```

Then, from the root of the repository you can generate and serve the website using
[Jekyll](https://jekyllrb.com/) (you need version 4.3.0 or later):

```
jekyll serve -s doc/
```

This should build and serve the website locally, and automatically rebuild
when you change any underlying files.
