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

/** Evaluate a file, return a JSON string.
 *
 * If the evaluation is successful, the JSON string is returned, which should be cleaned up with
 * jsonnet_delete.  If an error occured, NULL is returned and the error string is populated with a
 * message.
 *
 * \param filename Path to a file containing Jsonnet code
 * \param error Return by reference error string
 * \returns Either NULL, or a string containing JSON
 */
const char *jsonnet_evaluate_file(const char *filename,
                                  const char **error);

/** Evaluate a Jsonnet string, return a JSON string.
 *
 * If the evaluation is successful, the JSON string is returned, which should be cleaned up with
 * jsonnet_delete.  If an error occured, NULL is returned and the error string is populated with a
 * message.
 *
 * \param filename Used in stack traces, you can use any meaningful string, e.g. "snippet"
 * \param snippet The Jsonnet code
 * \param error Return by reference error string
 * \returns Either NULL or a string containing JSON
 */
const char *jsonnet_evaluate_snippet(const char *filename,
                                     const char *snippet,
                                     const char **error);

/** Clean up returned strings.
 *
 * \param str Either the returned JSON, or the error message, or NULL.
 **/
void jsonnet_delete(const char *str);
