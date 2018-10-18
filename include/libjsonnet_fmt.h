/*
Copyright 2018 Google Inc. All rights reserved.

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

#ifndef LIB_JSONNET_FMT_H
#define LIB_JSONNET_FMT_H

#include <stddef.h>

/** \file This file is a library interface for formatting Jsonnet code.
 */

/** Jsonnet virtual machine context. */
struct JsonnetVm;

/** Indentation level when reformatting (number of spaeces).
 *
 * \param n Number of spaces, must be > 0.
 */
void jsonnet_fmt_indent(struct JsonnetVm *vm, int n);

/** Indentation level when reformatting (number of spaeces).
 *
 * \param n Number of spaces, must be > 0.
 */
void jsonnet_fmt_max_blank_lines(struct JsonnetVm *vm, int n);

/** Preferred style for string literals ("" or '').
 *
 * \param c String style as a char ('d', 's', or 'l' (leave)).
 */
void jsonnet_fmt_string(struct JsonnetVm *vm, int c);

/** Preferred style for line comments (# or //).
 *
 * \param c Comment style as a char ('h', 's', or 'l' (leave)).
 */
void jsonnet_fmt_comment(struct JsonnetVm *vm, int c);

/** Whether to add an extra space on the inside of arrays.
 */
void jsonnet_fmt_pad_arrays(struct JsonnetVm *vm, int v);

/** Whether to add an extra space on the inside of objects.
 */
void jsonnet_fmt_pad_objects(struct JsonnetVm *vm, int v);

/** Use syntax sugar where possible with field names.
 */
void jsonnet_fmt_pretty_field_names(struct JsonnetVm *vm, int v);

/** Sort top-level imports in alphabetical order
 */
void jsonnet_fmt_sort_imports(struct JsonnetVm *vm, int v);

/** If set to 1, will reformat the Jsonnet input after desugaring. */
void jsonnet_fmt_debug_desugaring(struct JsonnetVm *vm, int v);

/** Reformat a file containing Jsonnet code, return a Jsonnet string.
 *
 * The returned string should be cleaned up with jsonnet_realloc.
 *
 * \param filename Path to a file containing Jsonnet code.
 * \param error Return by reference whether or not there was an error.
 * \returns Either Jsonnet code or the error message.
 */
char *jsonnet_fmt_file(struct JsonnetVm *vm, const char *filename, int *error);

/** Reformat a string containing Jsonnet code, return a Jsonnet string.
 *
 * The returned string should be cleaned up with jsonnet_realloc.
 *
 * \param filename Path to a file (used in error messages).
 * \param snippet Jsonnet code to execute.
 * \param error Return by reference whether or not there was an error.
 * \returns Either Jsonnet code or the error message.
 */
char *jsonnet_fmt_snippet(struct JsonnetVm *vm, const char *filename, const char *snippet,
                          int *error);

#endif  // LIB_JSONNET_FMT_H
