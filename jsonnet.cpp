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
#include <cassert>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
    #include "libjsonnet.h"
}

#define JSONNET_VERSION "v0.6.0-beta"

struct ImportCallbackContext {
    JsonnetVm *vm;
    std::vector<std::string> *jpaths;
};

enum ImportStatus {
    IMPORT_STATUS_OK,
    IMPORT_STATUS_FILE_NOT_FOUND,
    IMPORT_STATUS_IO_ERROR
};

static enum ImportStatus try_path(const std::string &dir, const std::string &rel,
                                  std::string &content)
{
    std::string abs_path;
    // It is possible that rel is actually absolute.
    if (rel.length() > 0 && rel[0] == '/') {
        abs_path = rel;
    } else {
        abs_path = dir + rel;
    }

    std::ifstream f;
    f.open(abs_path.c_str());
    if (!f.good()) return IMPORT_STATUS_FILE_NOT_FOUND;
    content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    if (!f.good()) return IMPORT_STATUS_IO_ERROR;
    return IMPORT_STATUS_OK;
}

static char *import_callback (void *ctx_, const char *dir, const char *file, int *success)
{
    const auto &ctx = *static_cast<ImportCallbackContext*>(ctx_);

    std::string input;

    ImportStatus status = try_path(dir, file, input);

    std::vector<std::string> jpaths(*ctx.jpaths);

    // If not found, try library search path.
    while (status == IMPORT_STATUS_FILE_NOT_FOUND) {
        if (jpaths.size() == 0) {
            *success = 0;
            const char *err = "No match locally or in the Jsonnet library path.";
            char *r = jsonnet_realloc(ctx.vm, nullptr, std::strlen(err) + 1);
            std::strcpy(r, err);
            return r;
        }
        status = try_path(jpaths.back(), file, input);
        jpaths.pop_back();
    }

    if (status == IMPORT_STATUS_IO_ERROR) {
        *success = 0;
        const char *err = std::strerror(errno);
        char *r = jsonnet_realloc(ctx.vm, nullptr, std::strlen(err) + 1);
        std::strcpy(r, err);
        return r;
    } else {
        assert(status == IMPORT_STATUS_OK);
        *success = 1;
        char *r = jsonnet_realloc(ctx.vm, nullptr, input.length() + 1);
        std::strcpy(r, input.c_str());
        return r;
    }
}

std::string next_arg(unsigned &i, const std::vector<std::string> &args)
{
    i++;
    if (i >= args.size()) {
        std::cerr << "Expected another commandline argument." << std::endl;
        exit(EXIT_FAILURE);
    }
    return args[i];
}

/** Collect commandline args into a vector of strings, and expand -foo to -f -o -o. */
std::vector<std::string> simplify_args(int argc, const char **argv)
{
    std::vector<std::string> r;
    for (int i=1 ; i<argc ; ++i) {
        std::string arg = argv[i];
        if (arg == "--") {
            // Add this arg and all remaining ones without simplification.
            r.push_back(arg);
            while ((++i) < argc)
                r.push_back(argv[i]);
            break;
        }
        // Check if it is of the form -abc and convert to -a -b -c
        if (arg.length() > 2 && arg[0] == '-' && arg[1] != '-') {
            for (unsigned j=1 ; j<arg.length() ; ++j) {
                r.push_back("-" + arg.substr(j,1));
            }
        } else {
            r.push_back(arg);
        }
    }
    return r;
}

void version(std::ostream &o)
{
    o << "Jsonnet commandline interpreter " << JSONNET_VERSION << std::endl;
}

void usage(std::ostream &o)
{
    version(o);
    o << "Usage:\n";
    o << "jsonnet {<option>} <filename>\n";
    o << "where <filename> can be - (stdin)\n";
    o << "and <option> can be:\n";
    o << "  -h / --help             This message\n";
    o << "  -e / --exec             Treat filename as code\n";
    o << "  -J / --jpath <dir>      Specify an additional library search dir\n";
    o << "  -V / --var <var>=<val>  Specify an 'external' var to the given value\n";
    o << "  -E / --env <var>        Bring in an environment var as an 'external' var\n";
    o << "  -m / --multi            Write multiple files, list files on stdout\n";
    o << "  -S / --string           Expect a string, manifest as plain text\n";
    o << "  -s / --max-stack <n>    Number of allowed stack frames\n";
    o << "  -t / --max-trace <n>    Max length of stack trace before cropping\n";
    o << "  --gc-min-objects <n>    Do not run garbage collector until this many\n";
    o << "  --gc-growth-trigger <n> Run garbage collector after this amount of object growth\n";
    o << "  --debug-ast             Unparse the parsed AST without executing it\n\n";
    o << "  --version               Print version\n";
    o << "Multichar options are expanded e.g. -abc becomes -a -b -c.\n";
    o << "The -- option suppresses option processing for subsequent arguments.\n";
    o << "Note that since jsonnet programs can begin with -, it is advised to\n";
    o << "use -- with -e if the program is unknown, e.g. jsonnet -e -- \"$CODE\".";
    o << std::endl;
}

long strtol_check(const std::string &str)
{
    const char *arg = str.c_str();
    char *ep;
    long r = std::strtol(arg, &ep, 10);
    if (*ep != '\0' || *arg == '\0') {
        std::cerr << "ERROR: Invalid integer \"" << arg << "\"\n" << std::endl;
        usage(std::cerr);
        exit(EXIT_FAILURE);
    }
    return r;
}

int main(int argc, const char **argv)
{
    std::vector<std::string> jpaths;
    jpaths.emplace_back("/usr/share/" JSONNET_VERSION "/");
    jpaths.emplace_back("/usr/local/share/" JSONNET_VERSION "/");

    JsonnetVm *vm = jsonnet_make();
    bool filename_is_code = false;
        
    bool multi = false;

    auto args = simplify_args(argc, argv);
    std::vector<std::string> remaining_args;

    for (unsigned i=0 ; i<args.size() ; ++i) {
        const std::string &arg = args[i];
        if (arg == "-h" || arg == "--help") {
            usage(std::cout);
            return EXIT_SUCCESS;
        } else if (arg == "-v" || arg == "--version") {
            version(std::cout);
            return EXIT_SUCCESS;
        } else if (arg == "-s" || arg == "--max-stack") {
            long l = strtol_check(next_arg(i, args));
            if (l < 1) {
                std::cerr << "ERROR: Invalid --max-stack value: " << l << "\n" << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_max_stack(vm, l);
        } else if (arg == "-J" || arg == "--jpath") {
            std::string dir = next_arg(i, args);
            if (dir.length() == 0) {
                std::cerr << "ERROR: -J argument was empty string" << std::endl;
                return EXIT_FAILURE;
            }
            if (dir[dir.length() - 1] != '/')
                dir += '/';
            jpaths.push_back(dir);
        } else if (arg == "-E" || arg == "--env") {
            const std::string var = next_arg(i, args);
            const char *val = ::getenv(var.c_str());
            if (val == nullptr) {
                std::cerr << "ERROR: Environment variable " << var
                          << " was undefined." << std::endl;
                return EXIT_FAILURE;
            }
            jsonnet_ext_var(vm, var.c_str(), val);
        } else if (arg == "-V" || arg == "--var") {
            const std::string var_val = next_arg(i, args);
            size_t eq_pos = var_val.find_first_of('=', 0);
            if (eq_pos == std::string::npos) {
                std::cerr << "ERROR: argument not in form <var>=<val> \""
                          << var_val << "\"." << std::endl;
                return EXIT_FAILURE;
            }
            const std::string var = var_val.substr(0, eq_pos);
            const std::string val = var_val.substr(eq_pos + 1, std::string::npos);
            jsonnet_ext_var(vm, var.c_str(), val.c_str());
        } else if (arg == "--gc-min-objects") {
            long l = strtol_check(next_arg(i, args));
            if (l < 0) {
                std::cerr << "ERROR: Invalid --gc-min-objects value: " << l << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_gc_min_objects(vm, l);
        } else if (arg == "-t" || arg == "--max-trace") {
            long l = strtol_check(next_arg(i, args));
            if (l < 0) {
                std::cerr << "ERROR: Invalid --max-trace value: " << l << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_max_trace(vm, l);
        } else if (arg == "--gc-growth-trigger") {
            const char *arg = next_arg(i,args).c_str();
            char *ep;
            double v = std::strtod(arg, &ep);
            if (*ep != '\0' || *arg == '\0') {
                std::cerr << "ERROR: Invalid number \"" << arg << "\"" << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            if (v < 0) {
                std::cerr << "ERROR: Invalid --gc-growth-trigger \"" << arg << "\"\n" << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_gc_growth_trigger(vm, v);
        } else if (arg == "-e" || arg == "--exec") {
            filename_is_code = true;
        } else if (arg == "-m" || arg == "--multi") {
            multi = true;
        } else if (arg == "-S" || arg == "--string") {
            jsonnet_string_output(vm, 1);
        } else if (arg == "--debug-ast") {
            jsonnet_debug_ast(vm, true);
        } else if (arg == "--") {
            // All subsequent args are not options.
            while ((++i) < args.size())
                remaining_args.push_back(args[i]);
            break;
        } else {
            remaining_args.push_back(args[i]);
        }
    }


    const char *want = filename_is_code ? "code" : "filename";

    if (remaining_args.size() == 0) {
        std::cerr << "ERROR: Must give " << want << "\n" << std::endl;
        usage(std::cerr);
        return EXIT_FAILURE;
    }

    std::string filename = remaining_args[0];

    if (remaining_args.size() > 1) {
        std::cerr << "ERROR: Already specified " << want << " as \"" << filename << "\"\n"
                  << std::endl;
        usage(std::cerr);
        return EXIT_FAILURE;
    }

    std::string input;
    if (filename_is_code) {
        input = filename;
        filename = "<cmdline>";
    } else {
        if (filename == "-") {
            filename = "<stdin>";
            input.assign(std::istreambuf_iterator<char>(std::cin),
                         std::istreambuf_iterator<char>());
        } else {
            std::ifstream f;
            f.open(filename.c_str());
            if (!f.good()) {
                std::string msg = "Opening input file: " + filename;
                perror(msg.c_str());
                return EXIT_FAILURE;
            }
            input.assign(std::istreambuf_iterator<char>(f),
                         std::istreambuf_iterator<char>());
            if (!f.good()) {
                std::string msg = "Reading input file: " + filename;
                perror(msg.c_str());
                return EXIT_FAILURE;
            }
        }
    }

    ImportCallbackContext import_callback_ctx { vm, &jpaths };
    jsonnet_import_callback(vm, import_callback, &import_callback_ctx);

    int error;
    char *output;
    if (multi) {
        output = jsonnet_evaluate_snippet_multi(vm, filename.c_str(), input.c_str(), &error);
    } else {
        output = jsonnet_evaluate_snippet(vm, filename.c_str(), input.c_str(), &error);
    }

    if (error) {
        std::cerr << output;
        std::cerr.flush();
        jsonnet_realloc(vm, output, 0);
        jsonnet_destroy(vm);
        return EXIT_FAILURE;
    }

    if (multi) {
        std::map<std::string, std::string> r;
        for (const char *c=output ; *c!='\0' ; ) {
            const char *filename = c;
            const char *c2 = c;
            while (*c2 != '\0') ++c2;
            ++c2;
            const char *json = c2;
            while (*c2 != '\0') ++c2;
            ++c2;
            c = c2;
            r[filename] = json;
        }
        jsonnet_realloc(vm, output, 0);
        for (const auto &pair : r) {
            const std::string &new_content = pair.second;
            const std::string &filename = pair.first;
            std::cout << filename << std::endl;
            {
                std::ifstream exists(filename.c_str());
                if (exists.good()) {
                    std::string existing_content;
                    existing_content.assign(std::istreambuf_iterator<char>(exists),
                                            std::istreambuf_iterator<char>());
                    if (existing_content == new_content) {
                        // Do not bump the timestamp on the file if its content is the same.
                        // This may trigger other tools (e.g. make) to do unnecessary work.
                        continue;
                    }
                }
            }
            std::ofstream f;
            f.open(filename.c_str());
            if (!f.good()) {
                std::string msg = "Opening output file: " + filename;
                perror(msg.c_str());
                jsonnet_destroy(vm);
                return EXIT_FAILURE;
            }
            f << new_content;
            f.close();
            if (!f.good()) {
                std::string msg = "Writing to output file: " + filename;
                perror(msg.c_str());
                jsonnet_destroy(vm);
                return EXIT_FAILURE;
            }
        }
        std::cout.flush();
    } else {
        std::cout << output;
        std::cout.flush();
        jsonnet_realloc(vm, output, 0);
    }
    jsonnet_destroy(vm);
    return EXIT_SUCCESS;
}

