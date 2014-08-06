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

const char *jsonnet_evaluate_file(const char *filename, const char **error)
{
    std::ifstream f;
    f.open(filename);
    if (!f.good()) {
        std::stringstream ss;
        ss << "Opening input file: " << filename << ": " << strerror(errno);
        *error = ::strdup(ss.str().c_str());
        return NULL;
    }
    std::string input;
    input.assign(std::istreambuf_iterator<char>(f),
                 std::istreambuf_iterator<char>());

    return jsonnet_evaluate_snippet(filename, input.c_str(), error);
}

const char *jsonnet_evaluate_snippet(const char *filename, const char *snippet, const char **error)
{
    double gc_growth_trigger = 2.0;
    unsigned max_stack = 500;
    unsigned gc_min_objects = 1000;

    try {
        Allocator alloc;
        AST *expr = jsonnet_parse(alloc, filename, snippet);
        jsonnet_static_analysis(expr);
        std::string json_str = jsonnet_vm_execute(alloc, expr,
                                                  max_stack, gc_min_objects, gc_growth_trigger);
        json_str += "\n";
        return ::strdup(json_str.c_str());

    } catch (StaticError &e) {
        std::stringstream ss;
        ss << "STATIC ERROR: " << e << std::endl;
        *error = ::strdup(ss.str().c_str());
        return NULL;

    } catch (RuntimeError &e) {
        std::stringstream ss;
        ss << "RUNTIME ERROR: " << e.msg << std::endl;
        const long max_above = 10;
        const long max_below = 10;
        const long sz = e.stackTrace.size();
        for (long i = 0 ; i < sz ; ++i) {
            const auto &f = e.stackTrace[i];
            if (i >= max_above && i < sz - max_below) {
                if (i == max_above)
                    ss << "\t..." << std::endl;
            } else {
                ss << "\t" << f.location << "\t" << f.name << std::endl;
            }
        }
        *error = ::strdup(ss.str().c_str());
        return NULL;
    }

}

void jsonnet_delete(const char *str)
{
    // Const cast is valid because memory originated from ::strdup.
    ::free(const_cast<char*>(str));
}

