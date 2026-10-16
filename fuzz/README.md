# Fuzzing jsonnet

This directory contains libFuzzer-based fuzz targets for the C API of
libjsonnet. The same targets are also built and run by
[OSS-Fuzz](https://github.com/google/oss-fuzz).

## Targets

* `convert_jsonnet_fuzzer_regular` — evaluates the input as a Jsonnet
  snippet with `jsonnet_evaluate_snippet`.
* `convert_jsonnet_fuzzer_stream` — evaluates the input as a Jsonnet
  snippet with `jsonnet_evaluate_snippet_stream`.
* `convert_jsonnet_fuzzer_multi` — evaluates the input as a Jsonnet
  snippet with `jsonnet_evaluate_snippet_multi`.

All targets install an import callback that refuses to load any files,
so only self-contained snippets are exercised.

## Build

Configure and build with clang and `-DBUILD_FUZZERS=ON`, e.g.:

    cmake -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
          -DBUILD_FUZZERS=ON ../
    make -j$(nproc)

## Run

    ./convert_jsonnet_fuzzer_regular <CORPUS DIR>

where `<CORPUS DIR>` is a directory containing Jsonnet files, for
example files from the `test_suite/` directory.

If a fuzzer crashes, a `crash-*` file is written. Reproduce it with:

    ./<target> <crash-file>
