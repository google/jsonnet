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

#include <cstdlib>
#include <cstring>

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

struct JsonnetVM {
    double gcGrowthTrigger;
    unsigned maxStack;
    unsigned gcMinObjects;
    bool debugAst;
    unsigned maxTrace;
    std::map<std::string, std::string> env;
    JsonnetVM(void)
      : gcGrowthTrigger(2.0), maxStack(500), gcMinObjects(1000), debugAst(false), maxTrace(20)
    { }
};

JsonnetVM *jsonnet_make(void)
{
    return new JsonnetVM();
}

void jsonnet_destroy(JsonnetVM *vm)
{
    delete vm;
}

void jsonnet_max_stack(JsonnetVM *vm, unsigned v)
{
    vm->maxStack = v;
}

void jsonnet_gc_min_objects(JsonnetVM *vm, unsigned v)
{
    vm->gcMinObjects = v;
}

void jsonnet_gc_growth_trigger(JsonnetVM *vm, double v)
{
    vm->gcGrowthTrigger = v;
}

void jsonnet_env(JsonnetVM *vm, const char *key, const char *val)
{
    vm->env[key] = val;
}

void jsonnet_debug_ast(JsonnetVM *vm, int v)
{
    vm->debugAst = v;
}

void jsonnet_max_trace(JsonnetVM *vm, unsigned v)
{
    vm->maxTrace = v;
}

const char *jsonnet_evaluate_file(JsonnetVM *vm, const char *filename, int *error)
{
    std::ifstream f;
    f.open(filename);
    if (!f.good()) {
        std::stringstream ss;
        ss << "Opening input file: " << filename << ": " << strerror(errno);
        *error = true;
        return ::strdup(ss.str().c_str());
    }
    std::string input;
    input.assign(std::istreambuf_iterator<char>(f),
                 std::istreambuf_iterator<char>());

    return jsonnet_evaluate_snippet(vm, filename, input.c_str(), error);
}

const char *jsonnet_evaluate_snippet(JsonnetVM *vm, const char *filename,
                                     const char *snippet, int *error)
{
    try {
        Allocator alloc;
        AST *expr = jsonnet_parse(alloc, filename, snippet);
        std::string json_str;
        if (vm->debugAst) {
            json_str = jsonnet_unparse_jsonnet(expr);
        } else {
            jsonnet_static_analysis(expr);
            json_str = jsonnet_vm_execute(alloc, expr, vm->env, vm->maxStack,
                                          vm->gcMinObjects, vm->gcGrowthTrigger);
        }
        json_str += "\n";
        *error = false;
        return ::strdup(json_str.c_str());

    } catch (StaticError &e) {
        std::stringstream ss;
        ss << "STATIC ERROR: " << e << std::endl;
        *error = true;
        return ::strdup(ss.str().c_str());

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
        return ::strdup(ss.str().c_str());
    }

}

void jsonnet_cleanup_string(JsonnetVM *vm, const char *str)
{
    (void) vm;
    // Const cast is valid because memory originated from ::strdup.
    ::free(const_cast<char*>(str));
}

