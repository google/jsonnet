/*
Copyright 2014 Google Inc. All rights reserved.

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

/** \file This file is a library interface for evaluating Jsonnet.  It is written in C++ but exposes
 * a C interface for easier wrapping by other languages.  See \see jsonnet_lib_test.c for an example
 * of using the library.
 */


/** Jsonnet virtual machine context. */
struct JsonnetVM;

/** Create a new Jsonnet virtual machine. */
struct JsonnetVM *jsonnet_make(void);

/** Set the maximum stack depth. */
void jsonnet_max_stack(struct JsonnetVM *vm, unsigned v);

/** Set the number of objects required before a garbage collection cycle is allowed. */
void jsonnet_gc_min_objects(struct JsonnetVM *vm, unsigned v);

/** Run the garbage collector after this amount of growth in the number of objects. */
void jsonnet_gc_growth_trigger(struct JsonnetVM *vm, double v);

/** Bind a Jsonnet external var to the given value. */
void jsonnet_ext_var(struct JsonnetVM *vm, const char *key, const char *val);

/** If set to 1, will emit the Jsonnet input after parsing / desugaring. */
void jsonnet_debug_ast(struct JsonnetVM *vm, int v);

/** Set the number of lines of stack trace to display (0 for all of them). */
void jsonnet_max_trace(struct JsonnetVM *vm, unsigned v);

/** Evaluate a file containing Jsonnet code, return a JSON string.
 *
 * The returned string should be cleaned up with jsonnet_cleanup_string.
 *
 * \param filename Path to a file containing Jsonnet code.
 * \param error Return by reference whether or not there was an error.
 * \returns Either JSON or the error message.
 */
const char *jsonnet_evaluate_file(struct JsonnetVM *vm,
                                  const char *filename,
                                  int *error);

/** Evaluate a string containing Jsonnet code, return a JSON string.
 *
 * The returned string should be cleaned up with jsonnet_cleanup_string.
 *
 * \param filename Path to a file containing Jsonnet code.
 * \param error Return by reference whether or not there was an error.
 * \returns Either JSON or the error message.
 */
const char *jsonnet_evaluate_snippet(struct JsonnetVM *vm,
                                     const char *filename,
                                     const char *snippet,
                                     int *error);

/** Evaluate a file containing Jsonnet code, return a number of JSON files.
 *
 * The returned character buffer contains an even number of strings, the filename and JSON for each
 * JSON file interleaved.  It should be cleaned up with jsonnet_cleanup_string.
 *
 * \param filename Path to a file containing Jsonnet code.
 * \param error Return by reference whether or not there was an error.
 * \returns Either the error, or a sequence of strings separated by \0, terminated with \0\0.
 */
const char *jsonnet_evaluate_file_multi(struct JsonnetVM *vm,
                                        const char *filename,
                                        int *error);

/** Evaluate a string containing Jsonnet code, return a number of JSON files.
 *
 * The returned character buffer contains an even number of strings, the filename and JSON for each
 * JSON file interleaved.  It should be cleaned up with jsonnet_cleanup_string.
 *
 * \param filename Path to a file containing Jsonnet code.
 * \param error Return by reference whether or not there was an error.
 * \returns Either the error, or a sequence of strings separated by \0, terminated with \0\0.
 */
const char *jsonnet_evaluate_snippet_multi(struct JsonnetVM *vm,
                                           const char *filename,
                                           const char *snippet,
                                           int *error);

/** Clean up returned strings.
 *
 * \param str Either the returned JSON, or the error message, or NULL.
 **/
void jsonnet_cleanup_string(struct JsonnetVM *vm, const char *str);

/** Complement of \see jsonnet_vm_make. */
void jsonnet_destroy(struct JsonnetVM *vm);

