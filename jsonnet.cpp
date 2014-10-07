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

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
    #include "libjsonnet.h"
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
std::vector<std::string> simplify_args (int argc, const char **argv)
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

void usage(std::ostream &o)
{
    o << "Usage:\n";
    o << "jsonnet {<option>} [<filename>]\n";
    o << "where <filename> defaults to - (stdin)\n";
    o << "and <option> can be:\n";
    o << "  -h / --help             This message\n";
    o << "  -e / --exec             Treat filename as code (requires explicit filename)\n";
    o << "  -E / --env              Add an environment variable\n\n";
    o << "  -m / --multi            Write multiple files\n";
    o << "  -s / --max-stack <n>    Number of allowed stack frames\n";
    o << "  -t / --max-trace <n>    Max length of stack trace before cropping\n";
    o << "  --gc-min-objects <n>    Do not run garbage collector until this many\n";
    o << "  --gc-growth-trigger <n> Run garbage collector after this amount of object growth\n";
    o << "  --debug-ast             Unparse the parsed AST without executing it\n\n";
    o << "Multichar options are expanded e.g. -abc becomes -a -b -c.\n";
    o << "The -- option suppresses option processing.  Note that since jsonnet programs can\n";
    o << "begin with -, it is advised to use -- with -e if the program is unknown.";
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
    JsonnetVM *vm = jsonnet_make();
    std::string filename = "-";
    bool filename_is_code = false;
    bool multi = false;

    auto args = simplify_args(argc, argv);
    std::vector<std::string> remaining_args;

    for (unsigned i=0 ; i<args.size() ; ++i) {
        const std::string &arg = args[i];
        if (arg == "-h" || arg == "--help") {
            usage(std::cout);
            exit(EXIT_SUCCESS);
        } else if (arg == "-s" || arg == "--max-stack") {
            long l = strtol_check(next_arg(i, args));
            if (l < 1) {
                std::cerr << "ERROR: Invalid --max-stack value: " << l << "\n" << std::endl;
                usage(std::cerr);
                exit(EXIT_FAILURE);
            }
            jsonnet_max_stack(vm, l);
        } else if (arg == "-E" || arg == "--env") {
            const std::string &var = next_arg(i, args);
            const char *val = ::getenv(var.c_str());
            if (val == nullptr) {
                std::cerr << "ERROR: Environment variable " << var << " was undefined." << std::endl;
                exit(EXIT_FAILURE);
            }
            jsonnet_env(vm, var.c_str(), val);
        } else if (arg == "--gc-min-objects") {
            long l = strtol_check(next_arg(i, args));
            if (l < 0) {
                std::cerr << "ERROR: Invalid --gc-min-objects value: " << l << std::endl;
                usage(std::cerr);
                exit(EXIT_FAILURE);
            }
            jsonnet_gc_min_objects(vm, l);
        } else if (arg == "-t" || arg == "--max-trace") {
            long l = strtol_check(next_arg(i, args));
            if (l < 0) {
                std::cerr << "ERROR: Invalid --max-trace value: " << l << std::endl;
                usage(std::cerr);
                exit(EXIT_FAILURE);
            }
            jsonnet_max_trace(vm, l);
        } else if (arg == "--gc-growth-trigger") {
            const char *arg = next_arg(i,args).c_str();
            char *ep;
            double v = std::strtod(arg, &ep);
            if (*ep != '\0' || *arg == '\0') {
                std::cerr << "ERROR: Invalid number \"" << arg << "\"" << std::endl;
                usage(std::cerr);
                exit(EXIT_FAILURE);
            }
            if (v < 0) {
                std::cerr << "ERROR: Invalid --gc-growth-trigger \"" << arg << "\"\n" << std::endl;
                usage(std::cerr);
                exit(EXIT_FAILURE);
            }
            jsonnet_gc_growth_trigger(vm, v);
        } else if (arg == "-e" || arg == "--exec") {
            filename_is_code = true;
        } else if (arg == "-m" || arg == "--multi") {
            multi = true;
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

    if (remaining_args.size() > 0) 
        filename = remaining_args[0];

    if (remaining_args.size() > 1) {
        std::cerr << "ERROR: Filename already specified as \"" << filename << "\"\n"
                  << std::endl;
        usage(std::cerr);
        exit(EXIT_FAILURE);
    }
    
    if (filename_is_code && remaining_args.size() == 0) {
        std::cerr << "ERROR: Must give filename when using -e, --exec\n" << std::endl;
        usage(std::cerr);
        exit(EXIT_FAILURE);
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

    int error;
    const char *output;
    if (multi) {
        output = jsonnet_evaluate_snippet_multi(vm, filename.c_str(), input.c_str(), &error);
    } else {
        output = jsonnet_evaluate_snippet(vm, filename.c_str(), input.c_str(), &error);
    }

    if (error) {
        std::cerr << output;
        std::cerr.flush();
        jsonnet_cleanup_string(vm, output);
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
        jsonnet_cleanup_string(vm, output);
        for (const auto &pair : r) {
            std::ofstream f;
            f.open(pair.first.c_str());
            if (!f.good()) {
                std::string msg = "Opening output file: " + pair.first;
                perror(msg.c_str());
                jsonnet_destroy(vm);
                return EXIT_FAILURE;
            }
            f << pair.second;
            f.close();
            if (!f.good()) {
                std::string msg = "Writing to output file: " + pair.first;
                perror(msg.c_str());
                jsonnet_destroy(vm);
                return EXIT_FAILURE;
            }
        }
    } else {
        std::cout << output;
        std::cout.flush();
        jsonnet_cleanup_string(vm, output);
    }
    jsonnet_destroy(vm);
    return EXIT_SUCCESS;
}

