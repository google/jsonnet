/*
Copyright 2019 Google Inc. All rights reserved.

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

#include <fstream>
#include <iostream>

#include "utils.h"

long strtol_check(const std::string &str)
{
    const char *arg = str.c_str();
    char *ep;
    long r = std::strtol(arg, &ep, 10);
    if (*ep != '\0' || *arg == '\0') {
        std::cerr << "ERROR: invalid integer \"" << arg << "\"\n" << std::endl;
        exit(EXIT_FAILURE);
    }
    return r;
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

std::vector<std::string> simplify_args(int argc, const char **argv)
{
    std::vector<std::string> r;
    for (int i = 1; i < argc; ++i) {
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
            for (unsigned j = 1; j < arg.length(); ++j) {
                r.push_back("-" + arg.substr(j, 1));
            }
        } else {
            r.push_back(arg);
        }
    }
    return r;
}

bool read_input_content(std::string filename, std::string *input)
{
    // Input file "-" tells Jsonnet to read stdin.
    if (filename == "-") {
        input->assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    } else {
        std::ifstream f;
        f.open(filename);
        if (!f.good()) {
            std::string msg = "Opening input file: " + filename;
            perror(msg.c_str());
            return false;
        }
        input->assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        if (!f.good()) {
            std::string msg = "Reading input file: " + filename;
            perror(msg.c_str());
            return false;
        }
    }
    return true;
}

void change_special_filename(bool filename_is_code, std::string *filename)
{
    if (filename_is_code) {
        *filename = "<cmdline>";
    } else if (*filename == "-") {
        *filename = "<stdin>";
    }
}

bool read_input(
    bool filename_is_code, std::string *filename, std::string *input)
{
    bool ok;
    if (filename_is_code) {
        *input = *filename;
        ok = true;
    } else {
        ok = read_input_content(*filename, input);
    }
    // Let's change the filename to something we can show the user
    // if it is not a real filename.
    change_special_filename(filename_is_code, filename);
    return ok;
}

bool write_output_file(const char *output, const std::string &output_file)
{
    std::ostream *o;
    std::ofstream f;

    if (output_file.empty()) {
        o = &std::cout;
    } else {
        f.open(output_file.c_str());
        if (!f.good()) {
            std::string msg = "Writing to output file: " + output_file;
            perror(msg.c_str());
            return false;
        }
        o = &f;
    }

    (*o) << output;

    if (output_file.empty()) {
        std::cout.flush();
    } else {
        f.close();
        if (!f.good()) {
            std::string msg = "Writing to output file: " + output_file;
            perror(msg.c_str());
            return false;
        }
    }
    return true;
}

