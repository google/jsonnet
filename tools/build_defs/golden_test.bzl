"""Bazel rules for running golden tests."""

def _gen_test_script(ctx):
    return """\
#!/bin/bash

set -uo pipefail

# We don't run 'init' or 'deinit' from tests.source, as these
# just create & remove a TMP dir and Bazel deals with that for us;
# instead we just need to ensure that the path for the  Bazel
# managed temp dir is available in TMP_DIR.

TMP_DIR="$TEST_TMPDIR"
VERBOSE=true
DISABLE_ERROR_TESTS=
SUMMARY_ONLY=

source ./test_suite/tests.source

GOLDEN_OUTPUT=$(cat '{golden_path}')

test_eval '{jsonnet_path}' '{input_path}' '{expected_exit_code}' "$GOLDEN_OUTPUT" '{golden_kind}'

if [ $FAILED -eq 0 ] ; then
    echo "$0: All $EXECUTED test scripts pass."
    exit 0
else
    echo "$0: FAILED: $FAILED / $EXECUTED"
    exit 1
fi
""".format(
        jsonnet_path = ctx.executable.jsonnet.short_path,
        input_path = ctx.file.src.short_path,
        expected_exit_code = 1 if ctx.attr.expect_error else 0,
        golden_path = ctx.file.golden.short_path,
        golden_kind = "PLAIN",
    )

def _jsonnet_json_golden_test_impl(ctx):
    test_script = ctx.actions.declare_file(ctx.label.name)
    ctx.actions.write(
        output = test_script,
        is_executable = True,
        content = _gen_test_script(ctx),
    )
    return DefaultInfo(
        executable = test_script,
        runfiles = ctx.runfiles(
            transitive_files = ctx.attr._test_sh_lib.files,
            files = [
                ctx.executable.jsonnet,
                ctx.file.src,
                ctx.file.golden,
            ] + ctx.files.data,
        ),
    )

jsonnet_json_golden_test = rule(
    implementation = _jsonnet_json_golden_test_impl,
    test = True,
    attrs = {
        "src": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "data": attr.label_list(allow_files = True),
        "golden": attr.label(
            mandatory = True,
            allow_single_file = True,
        ),
        "jsonnet": attr.label(
            default = "//cmd:jsonnet",
            executable = True,
            cfg = "exec",
        ),
        "expect_error": attr.bool(
            doc = "If True, the golden file is the expected stderr output from jsonnet",
        ),
        "canonicalize_golden": attr.bool(
            doc = "If True, the golden file will be reformatted prior to comparing against the jsonnet output",
        ),
        "_test_sh_lib": attr.label(
            default = "//test_suite:tests_sh_lib",
        ),
    },
)
