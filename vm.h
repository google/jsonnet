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

#ifndef VM_H
#define VM_H

#include "ast.h"
#include "libjsonnet.h"

/** A single line of a stack trace from a runtime error.
 */
struct TraceFrame {
    LocationRange location;
    std::string name;
    TraceFrame(const LocationRange &location, const std::string &name="")
      : location(location), name(name)
    { }
};

/** Exception that is thrown by the interpreter when it reaches an error construct, or divide by
 * zero, array bounds error, dynamic type error, etc.
 */
struct RuntimeError {
    std::vector<TraceFrame> stackTrace;
    std::string msg;
    RuntimeError(const std::vector<TraceFrame> stack_trace, const std::string &msg)
      : stackTrace(stack_trace), msg(msg)
    { }
};

/** Execute the program and return the value as a JSON string.
 *
 * \param alloc The allocator used to create the ast.
 * \param ast The program to execute.
 * \param max_stack Recursion beyond this level gives an error.
 * \param gc_min_objects The garbage collector does not run when the heap is this small.
 * \param gc_growth_trigger Growth since last garbage collection cycle to trigger a new cycle.
 * \param import_callback A callback to handle imports
 * \param import_callback_ctx Context param for the import callback.
 * \param output_string Whether to expect a string and output it without JSON encoding
 * \throws RuntimeError reports runtime errors in the program.
 * \returns The JSON result in string form.
 */
std::string jsonnet_vm_execute(Allocator &alloc, const AST *ast,
                               const std::map<std::string, std::string> &ext_vars,
                               unsigned max_stack, double gc_min_objects,
                               double gc_growth_trigger,
                               JsonnetImportCallback *import_callback, void *import_callback_ctx,
                               bool string_output);

/** Execute the program and return the value as a number of JSON files.
 *
 * This assumes the given program yields an object whose keys are filenames.
 *
 * \param alloc The allocator used to create the ast.
 * \param ast The program to execute.
 * \param max_stack Recursion beyond this level gives an error.
 * \param gc_min_objects The garbage collector does not run when the heap is this small.
 * \param gc_growth_trigger Growth since last garbage collection cycle to trigger a new cycle.
 * \param import_callback A callback to handle imports
 * \param import_callback_ctx Context param for the import callback.
 * \param output_string Whether to expect a string and output it without JSON encoding
 * \throws RuntimeError reports runtime errors in the program.
 * \returns A mapping from filename to the JSON strings for that file.
 */
std::map<std::string, std::string> jsonnet_vm_execute_multi(
    Allocator &alloc, const AST *ast, const std::map<std::string, std::string> &ext_vars,
    unsigned max_stack, double gc_min_objects, double gc_growth_trigger,
    JsonnetImportCallback *import_callback, void *import_callback_ctx,
    bool string_output);

#endif
