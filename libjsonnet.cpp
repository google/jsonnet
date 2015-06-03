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

#include <cstdlib>
#include <cstring>

#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

extern "C" {
    #include "libjsonnet.h"
}

#include "parser.h"
#include "static_analysis.h"
#include "vm.h"

static void memory_panic(void)
{
    fputs("FATAL ERROR: A memory allocation error occurred.\n", stderr);
    abort();
}

static char *from_string(JsonnetVm* vm, const std::string &v)
{
    char *r = jsonnet_realloc(vm, nullptr, v.length() + 1);
    std::strcpy(r, v.c_str());
    return r;
}

/** Resolve the absolute path and use C++ file io to load the file.
 */
static char *default_import_callback(void *ctx, const char *base, const char *file, int *success)
{
    auto *vm = static_cast<JsonnetVm*>(ctx);

    if (std::strlen(file) == 0) {
        *success = 0;
        return from_string(vm, "The empty string is not a valid filename");
    }

    if (file[std::strlen(file) - 1] == '/') {
        *success = 0;
        return from_string(vm, "Attempted to import a directory");
    }

    std::string abs_path;
    // It is possible that file is an absolute path
    if (file[0] == '/')
        abs_path = file;
    else
        abs_path = std::string(base) + file;

    std::ifstream f;
    f.open(abs_path.c_str());
    if (!f.good()) {
        *success = 0;
        return from_string(vm, std::strerror(errno));
    }
    try {
        std::string input;
        input.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        *success = 1;
        return from_string(vm, input);
    } catch (const std::ios_base::failure &io_err) {
        *success = 0;
        return from_string(vm, io_err.what());
    }
}


struct JsonnetVm {
    double gcGrowthTrigger;
    unsigned maxStack;
    unsigned gcMinObjects;
    bool debugAst;
    unsigned maxTrace;
    std::map<std::string, std::string> extVars;
    JsonnetImportCallback *importCallback;
    void *importCallbackContext;
    bool stringOutput;
    JsonnetVm(void)
      : gcGrowthTrigger(2.0), maxStack(500), gcMinObjects(1000), debugAst(false), maxTrace(20),
        importCallback(default_import_callback), importCallbackContext(this), stringOutput(false)
    { }
};

#define TRY try {
#define CATCH(func) \
    } catch (const std::bad_alloc &) {\
        memory_panic(); \
    } catch (const std::exception &e) {\
        std::cerr << "Something went wrong during " func ", please report this: " \
                  << e.what() << std::endl; \
        abort(); \
    }

const char *jsonnet_version(void)
{
    return LIB_JSONNET_VERSION;
}

JsonnetVm *jsonnet_make(void)
{
    TRY
    return new JsonnetVm();
    CATCH("jsonnet_make")
    return nullptr;
}

void jsonnet_destroy(JsonnetVm *vm)
{
    TRY
    delete vm;
    CATCH("jsonnet_destroy")
}

void jsonnet_max_stack(JsonnetVm *vm, unsigned v)
{
    vm->maxStack = v;
}

void jsonnet_gc_min_objects(JsonnetVm *vm, unsigned v)
{
    vm->gcMinObjects = v;
}

void jsonnet_gc_growth_trigger(JsonnetVm *vm, double v)
{
    vm->gcGrowthTrigger = v;
}

void jsonnet_string_output(struct JsonnetVm *vm, int v)
{
    vm->stringOutput = bool(v);
}

void jsonnet_import_callback(struct JsonnetVm *vm, JsonnetImportCallback *cb, void *ctx)
{
    vm->importCallback = cb;
    vm->importCallbackContext = ctx;
}

void jsonnet_ext_var(JsonnetVm *vm, const char *key, const char *val)
{
    vm->extVars[key] = val;
}

void jsonnet_debug_ast(JsonnetVm *vm, int v)
{
    vm->debugAst = v;
}

void jsonnet_max_trace(JsonnetVm *vm, unsigned v)
{
    vm->maxTrace = v;
}

static char *jsonnet_evaluate_snippet_aux(JsonnetVm *vm, const char *filename,
                                          const char *snippet, int *error, bool multi)
{
    try {
        Allocator alloc;
        AST *expr = jsonnet_parse(&alloc, filename, snippet);
        std::string json_str;
        std::map<std::string, std::string> files;
        if (vm->debugAst) {
            json_str = jsonnet_unparse_jsonnet(expr);
        } else {
            jsonnet_static_analysis(expr);
            if (multi) {
                files = jsonnet_vm_execute_multi(&alloc, expr, vm->extVars, vm->maxStack,
                                                 vm->gcMinObjects, vm->gcGrowthTrigger,
                                                 vm->importCallback, vm->importCallbackContext,
                                                 vm->stringOutput);
            } else {
                json_str = jsonnet_vm_execute(&alloc, expr, vm->extVars, vm->maxStack,
                                              vm->gcMinObjects, vm->gcGrowthTrigger,
                                              vm->importCallback, vm->importCallbackContext,
                                              vm->stringOutput);
            }
        }
        if (multi) {
            size_t sz = 1; // final sentinel
            for (const auto &pair : files) {
                sz += pair.first.length() + 1; // include sentinel
                sz += pair.second.length() + 2; // Add a '\n' as well as sentinel
            }
            char *buf = (char*)::malloc(sz);
            if (buf == nullptr) memory_panic();
            std::ptrdiff_t i = 0;
            for (const auto &pair : files) {
                memcpy(&buf[i], pair.first.c_str(), pair.first.length() + 1);
                i += pair.first.length() + 1;
                memcpy(&buf[i], pair.second.c_str(), pair.second.length());
                i += pair.second.length();
                buf[i] = '\n';
                i++;
                buf[i] = '\0';
                i++;
            }
            buf[i] = '\0'; // final sentinel
            *error = false;
            return buf;
        } else {
            json_str += "\n";
            *error = false;
            return from_string(vm, json_str);
        }

    } catch (StaticError &e) {
        std::stringstream ss;
        ss << "STATIC ERROR: " << e << std::endl;
        *error = true;
        return from_string(vm, ss.str());

    } catch (RuntimeError &e) {
        std::stringstream ss;
        ss << "RUNTIME ERROR: " << e.msg << std::endl;
        const long max_above = vm->maxTrace / 2;
        const long max_below = vm->maxTrace - max_above;
        const long sz = e.stackTrace.size();
        for (long i = 0 ; i < sz ; ++i) {
            const auto &f = e.stackTrace[i];
            if (vm->maxTrace > 0 && i >= max_above && i < sz - max_below) {
                if (i == max_above)
                    ss << "\t..." << std::endl;
            } else {
                ss << "\t" << f.location << "\t" << f.name << std::endl;
            }
        }
        *error = true;
        return from_string(vm, ss.str());
    }

}

static char *jsonnet_evaluate_file_aux(JsonnetVm *vm, const char *filename, int *error, bool multi)
{
    std::ifstream f;
    f.open(filename);
    if (!f.good()) {
        std::stringstream ss;
        ss << "Opening input file: " << filename << ": " << strerror(errno);
        *error = true;
        return from_string(vm, ss.str());
    }
    std::string input;
    input.assign(std::istreambuf_iterator<char>(f),
                 std::istreambuf_iterator<char>());

    return jsonnet_evaluate_snippet_aux(vm, filename, input.c_str(), error, multi);
}

char *jsonnet_evaluate_file(JsonnetVm *vm, const char *filename, int *error)
{
    TRY
    return jsonnet_evaluate_file_aux(vm, filename, error, false);
    CATCH("jsonnet_evaluate_file")
    return nullptr;  // Never happens.
}

char *jsonnet_evaluate_file_multi(JsonnetVm *vm, const char *filename, int *error)
{
    TRY
    return jsonnet_evaluate_file_aux(vm, filename, error, true);
    CATCH("jsonnet_evaluate_file_multi")
    return nullptr;  // Never happens.
}

char *jsonnet_evaluate_snippet(JsonnetVm *vm, const char *filename, const char *snippet, int *error)
{
    TRY
    return jsonnet_evaluate_snippet_aux(vm, filename, snippet, error, false);
    CATCH("jsonnet_evaluate_snippet")
    return nullptr;  // Never happens.
}

char *jsonnet_evaluate_snippet_multi(JsonnetVm *vm, const char *filename,
                                     const char *snippet, int *error)
{
    TRY
    return jsonnet_evaluate_snippet_aux(vm, filename, snippet, error, true);
    CATCH("jsonnet_evaluate_snippet_multi")
    return nullptr;  // Never happens.
}

char *jsonnet_realloc(JsonnetVm *vm, char *str, size_t sz)
{
    (void) vm;
    if (str == nullptr) {
        if (sz == 0) return nullptr;
        auto *r = static_cast<char*>(::malloc(sz));
        if (r == nullptr) memory_panic();
        return r;
    } else {
        if (sz == 0) {
            ::free(str);
            return nullptr;
        } else {
            auto *r = static_cast<char*>(::realloc(str, sz));
            if (r == nullptr) memory_panic();
            return r;
        }
    }
}

