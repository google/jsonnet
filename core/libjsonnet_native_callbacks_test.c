/*
Copyright 2015 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libjsonnet.h>

typedef struct JsonnetJsonValue JJV;

static JJV *native_concat(void *ctx, const JJV * const *argv, int *succ)
{
    struct JsonnetVm *vm = (struct JsonnetVm *)ctx;
    const char *a = jsonnet_json_extract_string(vm, argv[0]);
    const char *b = jsonnet_json_extract_string(vm, argv[1]);
    if (a == NULL || b == NULL) {
        *succ = 0;
        return jsonnet_json_make_string(vm, "Bad params.");
    }
    char *str = malloc(strlen(a) + strlen(b) + 1);
    sprintf(str, "%s%s", a, b);
    JJV *r = jsonnet_json_make_string(vm, str);
    free(str);
    *succ = 1;
    return r;
}

static JJV *native_square(void *ctx, const JJV * const *argv, int *succ)
{
    struct JsonnetVm *vm = (struct JsonnetVm *)ctx;
    double a;
    if (!jsonnet_json_extract_number(vm, argv[0], &a)) {
        *succ = 0;
        return jsonnet_json_make_string(vm, "Bad param 'a'.");
    }
    *succ = 1;
    return jsonnet_json_make_number(vm, a * a);
}

static JJV *native_build(void *ctx, const JJV * const *argv, int *succ)
{
    struct JsonnetVm *vm = (struct JsonnetVm *)ctx;
    (void) argv;
    JJV *obj_top = jsonnet_json_make_object(vm);
    JJV *arr_top = jsonnet_json_make_array(vm);
    JJV *arr1 = jsonnet_json_make_array(vm);
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_string(vm, "Test 1.1"));
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_string(vm, "Test 1.2"));
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_string(vm, "Test 1.3"));
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_bool(vm, 1));
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_number(vm, 42));
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_null(vm));
    jsonnet_json_array_append(vm, arr1, jsonnet_json_make_object(vm));
    jsonnet_json_array_append(vm, arr_top, arr1);
    JJV *arr2 = jsonnet_json_make_array(vm);
    jsonnet_json_array_append(vm, arr2, jsonnet_json_make_string(vm, "Test 2.1"));
    jsonnet_json_array_append(vm, arr2, jsonnet_json_make_string(vm, "Test 2.2"));
    jsonnet_json_array_append(vm, arr2, jsonnet_json_make_string(vm, "Test 2.3"));
    jsonnet_json_array_append(vm, arr2, jsonnet_json_make_bool(vm, 0));
    jsonnet_json_array_append(vm, arr2, jsonnet_json_make_number(vm, -42));
    jsonnet_json_array_append(vm, arr2, jsonnet_json_make_null(vm));
    JJV *little_obj = jsonnet_json_make_object(vm);
    jsonnet_json_object_append(vm, little_obj, "f", jsonnet_json_make_string(vm, "foo"));
    jsonnet_json_object_append(vm, little_obj, "g", jsonnet_json_make_string(vm, "bar"));
    jsonnet_json_array_append(vm, arr2, little_obj);
    jsonnet_json_array_append(vm, arr_top, arr2);
    jsonnet_json_object_append(vm, obj_top, "field", arr_top);
    *succ = 1;
    return obj_top;
}

#define BUILD_OUTPUT \
    "{\"field\":[[\"Test 1.1\",\"Test 1.2\",\"Test 1.3\",true,42,null,{}],"\
    "[\"Test 2.1\",\"Test 2.2\",\"Test 2.3\",false,-42,null,{\"f\":\"foo\",\"g\":\"bar\"}]]}"

static int eval_and_check(const char *test_name, const char *input, const char *expect, int expect_error)
{
    struct JsonnetVm *vm = jsonnet_make();

    static const char *PARAMS_0[] = {NULL};
    static const char *PARAMS_1[] = {"a", NULL};
    static const char *PARAMS_2[] = {"a", "b", NULL};
    jsonnet_native_callback(vm, "build", native_build, vm, PARAMS_0);
    jsonnet_native_callback(vm, "square", native_square, vm, PARAMS_1);
    jsonnet_native_callback(vm, "concat", native_concat, vm, PARAMS_2);

    int error = 0;
    char *output = jsonnet_evaluate_snippet(vm, "snippet", input, &error);
    if (expect && strcmp(output, expect)) {
        error = 1;
        fprintf(stderr, "FAIL: %s produced wrong output. Want:\n%s\n----- Got -----\n%s\n-----\n", test_name, expect, output);
    } else {
        if (error && expect_error) {
            error = 0;
        } else if (error) {
            fprintf(stderr, "FAIL: %s failed evaluation: %s\n", test_name, output);
        } else if (expect_error) {
            fprintf(stderr, "FAIL: %s evaluated unexpectedly. output was:\n%s\n", test_name, output);
        }
    }
    jsonnet_realloc(vm, output, 0);
    jsonnet_destroy(vm);
    if (!error) {
        fprintf(stderr, "PASS: %s\n", test_name);
    }
    return !error;
}

static int run_tests()
{
    int fail_count = 0;

    fail_count += !eval_and_check("build",
        "std.assertEqual(std.native('build')()," BUILD_OUTPUT ") && true",
        "true\n", 0);

    fail_count += !eval_and_check("square_noarg",
        "std.native('square')()",
        "RUNTIME ERROR: function parameter a not bound in call.\n\tsnippet:1:1-23\t\n", 1);

    fail_count += !eval_and_check("square_badarg",
        "std.native('square')('prune')",
        "RUNTIME ERROR: Bad param 'a'.\n\tsnippet:1:1-30\t\n", 1);

    fail_count += !eval_and_check("square_badarg2",
        "std.native('square')({})",
        "RUNTIME ERROR: native extensions can only take primitives.\n\tsnippet:1:1-25\t\n", 1);

    fail_count += !eval_and_check("square",
        "std.native('square')(42.5)",
        "1806.25\n", 0);

    fail_count += !eval_and_check("concat_noarg0",
        "std.native('concat')()",
        "RUNTIME ERROR: function parameter a not bound in call.\n\tsnippet:1:1-23\t\n", 1);

    fail_count += !eval_and_check("concat_noarg1",
        "std.native('concat')('hello')",
        "RUNTIME ERROR: function parameter b not bound in call.\n\tsnippet:1:1-30\t\n", 1);

    fail_count += !eval_and_check("concat_badarg1",
        "std.native('concat')(false, 'hello')",
        "RUNTIME ERROR: Bad params.\n\tsnippet:1:1-37\t\n", 1);

    fail_count += !eval_and_check("concat_badarg2",
        "std.native('concat')('hello', true)",
        "RUNTIME ERROR: Bad params.\n\tsnippet:1:1-36\t\n", 1);

    fail_count += !eval_and_check("concat_badarg3",
        "std.native('concat')(99, 100)",
        "RUNTIME ERROR: Bad params.\n\tsnippet:1:1-30\t\n", 1);

    fail_count += !eval_and_check("concat",
        "std.native('concat')('hello', 'world')",
        "\"helloworld\"\n", 0);

    return !fail_count;
}

int main(int argc, const char **argv)
{
    (void)argv; /* not unused */
    if (argc > 1) {
        fprintf(stderr, "libjsonnet_native_callbacks_test does not support any command line arguments");
        return EXIT_FAILURE;
    }
    const int ok = run_tests();
    return (ok ? EXIT_SUCCESS : EXIT_FAILURE);
}
