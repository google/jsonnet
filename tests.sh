set -e

JSONNET_BIN="${JSONNET_BIN:-./jsonnet}"
TEST_SNIPPET="std.assertEqual(({ x: 1, y: self.x } { x: 2 }).y, 2)"
"$JSONNET_BIN" -e "${TEST_SNIPPET}" || FAIL=TRUE

if [ -z "$DISABLE_LIB_TESTS" ]; then
    LD_LIBRARY_PATH=. ./libjsonnet_test_snippet "${TEST_SNIPPET}" || FAIL=TRUE
    LD_LIBRARY_PATH=. ./libjsonnet_test_file "test_suite/object.jsonnet" || FAIL=TRUE
fi
(cd examples ; ./check.sh) || FAIL=TRUE
(cd examples/terraform ; ./check.sh) || FAIL=TRUE
(cd test_suite ; ./run_tests.sh) || FAIL=TRUE
if [ -z "$DISABLE_FMT_TESTS" ]; then
    (cd test_suite ; ./run_fmt_tests.sh) || FAIL=TRUE
    (cd test_suite ; ./run_fmt_idempotence_tests.sh) || FAIL=TRUE
fi
if [ -n "$FAIL" ]; then
    echo "TESTS FAILED"
fi
