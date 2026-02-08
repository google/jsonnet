#!/usr/bin/env bash
set -e

JSONNET_BIN="${JSONNET_BIN:-./jsonnet}"
TEST_SNIPPET="std.assertEqual(({ x: 1, y: self.x } { x: 2 }).y, 2)"
printf "snippet output (should be true): "
"$JSONNET_BIN" -e "${TEST_SNIPPET}" || FAIL=TRUE

if [ -z "$DISABLE_LIB_TESTS" ]; then
    printf 'libjsonnet_native_callbacks_test: '
    ./libjsonnet_native_callbacks_test || FAIL=TRUE
    printf 'libjsonnet_file_test: '
    ./libjsonnet_file_test "test_suite/object.jsonnet" || FAIL=TRUE

    for gtest_bin in unicode_test lexer_test parser_test libjsonnet_test libjsonnet++_test; do
        if [[ -x "./${gtest_bin}" ]]; then
            "./${gtest_bin}" || FAIL=true
        fi
    done
fi
examples/check.sh || FAIL=TRUE
examples/terraform/check.sh || FAIL=TRUE
test_cmd/run_cmd_tests.sh || FAIL=TRUE
test_suite/run_tests.sh || FAIL=TRUE
doc/_stdlib_gen/run_tests.sh || FAIL=TRUE
if [ -z "$DISABLE_FMT_TESTS" ]; then
    test_suite/run_fmt_tests.sh || FAIL=TRUE
    test_suite/run_fmt_idempotence_tests.sh || FAIL=TRUE
fi
if [ -n "$FAIL" ]; then
    echo "TESTS FAILED"
    exit 1
fi
