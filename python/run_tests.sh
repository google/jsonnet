#!/bin/bash

TEST_SNIPPET="$(cat test.jsonnet)"

echo "Python testing Jsonnet snippet..."
OUTPUT="$(python jsonnet_test_snippet.py "${TEST_SNIPPET}")"
if [ "$?" != "0" ] ; then
    echo "Jsonnet execution failed:"
    echo "$OUTPUT"
    exit 1
fi
if [ "$OUTPUT" != "true" ] ; then
    echo "Got bad output:"
    echo "$OUTPUT"
    exit 1
fi

echo "Python testing Jsonnet file..."
OUTPUT="$(python jsonnet_test_file.py "test.jsonnet")"
if [ "$?" != "0" ] ; then
    echo "Jsonnet execution failed:"
    echo "$OUTPUT"
    exit 1
fi
if [ "$OUTPUT" != "true" ] ; then
    echo "Got bad output:"
    echo "$OUTPUT"
    exit 1
fi

echo "Python test passed."
