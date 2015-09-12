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

#include <exception>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
    #include "core/libjsonnet.h"
}

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
                                  std::string &content, std::string &found_here,
                                  std::string &err_msg)
{
    std::string abs_path;
    if (rel.length() == 0) {
        err_msg = "The empty string is not a valid filename";
        return IMPORT_STATUS_IO_ERROR;
    }
    // It is possible that rel is actually absolute.
    if (rel[0] == '/') {
        abs_path = rel;
    } else {
        abs_path = dir + rel;
    }

    if (abs_path[abs_path.length() - 1] == '/') {
        err_msg = "Attempted to import a directory";
        return IMPORT_STATUS_IO_ERROR;
    }

    std::ifstream f;
    f.open(abs_path.c_str());
    if (!f.good()) return IMPORT_STATUS_FILE_NOT_FOUND;
    try {
        content.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    } catch (const std::ios_base::failure &io_err) {
        err_msg = io_err.what();
        return IMPORT_STATUS_IO_ERROR;
    }
    if (!f.good()) {
        err_msg = strerror(errno);
        return IMPORT_STATUS_IO_ERROR;
    }

    found_here = abs_path;

    return IMPORT_STATUS_OK;
}

static char *from_string(JsonnetVm* vm, const std::string &v)
{
    char *r = jsonnet_realloc(vm, nullptr, v.length() + 1);
    std::strcpy(r, v.c_str());
    return r;
}

static char *import_callback(void *ctx_, const char *dir, const char *file,
                             char **found_here_cptr, int *success)
{
    const auto &ctx = *static_cast<ImportCallbackContext*>(ctx_);

    std::string input, found_here, err_msg;

    ImportStatus status = try_path(dir, file, input, found_here, err_msg);

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
        status = try_path(jpaths.back(), file, input, found_here, err_msg);
        jpaths.pop_back();
    }

    if (status == IMPORT_STATUS_IO_ERROR) {
        *success = 0;
        return from_string(ctx.vm, err_msg);
    } else {
        assert(status == IMPORT_STATUS_OK);
        *success = 1;
        *found_here_cptr = from_string(ctx.vm, found_here);
        return from_string(ctx.vm, input);
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
    o << "Jsonnet commandline interpreter " << jsonnet_version() << std::endl;
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
    o << "  --code-var <var>=<val>  As --var but value is Jsonnet code\n";
    o << "  --code-env <var>        As --env but env var contains Jsonnet code\n";
    o << "  -o / --output-file <file> Write to the output file rather than stdout\n";
    o << "  -m / --multi <dir>      Write multiple files to the directory, list files on stdout\n";
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

/** Class for representing configuration read from command line flags.  */
class JsonnetConfig {
  public:
    JsonnetConfig()
        : filename_is_code_(false),
          multi_(false) {
        jpaths_.emplace_back(
            "/usr/share/" + std::string(jsonnet_version()) + "/");
        jpaths_.emplace_back(
            "/usr/local/share/" + std::string(jsonnet_version()) + "/");
    }

    bool filename_is_code() const {
        return filename_is_code_;
    }

    void set_filename_is_code(bool filename_is_code) {
        filename_is_code_ = filename_is_code;
    }

    bool multi() const {
        return multi_;
    }

    void set_multi(bool multi) {
        multi_ = multi;
    }

    const std::string& input_file() const {
        return input_file_;
    }

    void set_input_file(const std::string& input_file) {
        input_file_ = input_file;
    }

    const std::string& output_file() const {
        return output_file_;
    }

    void set_output_file(const std::string& output_file) {
        output_file_ = output_file;
    }

    const std::string& output_dir() const {
        return output_dir_;
    }

    void set_output_dir(const std::string& output_dir) {
        output_dir_ = output_dir;
    }

    void AddJpath(const std::string& jpath) {
        jpaths_.emplace_back(jpath);
    }

    const std::vector<std::string>& jpaths() const {
        return jpaths_;
    }

    std::vector<std::string>* mutable_jpaths() {
        return &jpaths_;
    }

  private:
    bool filename_is_code_;
    bool multi_;
    std::string input_file_;
    std::string output_file_;
    std::string output_dir_;
    std::vector<std::string> jpaths_;
};

/** Parse the command line arguments, configuring the Jsonnet VM context and
 * populating the JsonnetConfig.
 */
static bool process_args(int argc,
                         const char **argv,
                         JsonnetConfig *config,
                         JsonnetVm *vm) {
    auto args = simplify_args(argc, argv);
    std::vector<std::string> remaining_args;

    for (unsigned i=0 ; i<args.size() ; ++i) {
        const std::string &arg = args[i];
        if (arg == "-h" || arg == "--help") {
            usage(std::cout);
            return false;
        } else if (arg == "-v" || arg == "--version") {
            version(std::cout);
            return false;
        } else if (arg == "-s" || arg == "--max-stack") {
            long l = strtol_check(next_arg(i, args));
            if (l < 1) {
                std::cerr << "ERROR: Invalid --max-stack value: " << l << "\n"
                          << std::endl;
                usage(std::cerr);
                return false;
            }
            jsonnet_max_stack(vm, l);
        } else if (arg == "-J" || arg == "--jpath") {
            std::string dir = next_arg(i, args);
            if (dir.length() == 0) {
                std::cerr << "ERROR: -J argument was empty string" << std::endl;
                return false;
            }
            if (dir[dir.length() - 1] != '/') {
                dir += '/';
            }
            config->AddJpath(dir);
        } else if (arg == "-E" || arg == "--env") {
            const std::string var = next_arg(i, args);
            const char *val = ::getenv(var.c_str());
            if (val == nullptr) {
                std::cerr << "ERROR: Environment variable " << var
                          << " was undefined." << std::endl;
                return false;
            }
            jsonnet_ext_var(vm, var.c_str(), val);
        } else if (arg == "-V" || arg == "--var") {
            const std::string var_val = next_arg(i, args);
            size_t eq_pos = var_val.find_first_of('=', 0);
            if (eq_pos == std::string::npos) {
                std::cerr << "ERROR: argument not in form <var>=<val> \""
                          << var_val << "\"." << std::endl;
                return false;
            }
            const std::string var = var_val.substr(0, eq_pos);
            const std::string val = var_val.substr(eq_pos + 1,
                                                   std::string::npos);
            jsonnet_ext_var(vm, var.c_str(), val.c_str());
        } else if (arg == "--code-env") {
            const std::string var = next_arg(i, args);
            const char *val = ::getenv(var.c_str());
            if (val == nullptr) {
                std::cerr << "ERROR: Environment variable " << var
                          << " was undefined." << std::endl;
                return EXIT_FAILURE;
            }
            jsonnet_ext_code(vm, var.c_str(), val);
        } else if (arg == "--code-var") {
            const std::string var_val = next_arg(i, args);
            size_t eq_pos = var_val.find_first_of('=', 0);
            if (eq_pos == std::string::npos) {
                std::cerr << "ERROR: argument not in form <var>=<val> \""
                          << var_val << "\"." << std::endl;
                return EXIT_FAILURE;
            }
            const std::string var = var_val.substr(0, eq_pos);
            const std::string val = var_val.substr(eq_pos + 1,
                                                   std::string::npos);
            jsonnet_ext_code(vm, var.c_str(), val.c_str());
        } else if (arg == "--gc-min-objects") {
            long l = strtol_check(next_arg(i, args));
            if (l < 0) {
                std::cerr << "ERROR: Invalid --gc-min-objects value: " << l
                          << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_gc_min_objects(vm, l);
        } else if (arg == "-t" || arg == "--max-trace") {
            long l = strtol_check(next_arg(i, args));
            if (l < 0) {
                std::cerr << "ERROR: Invalid --max-trace value: " << l
                          << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_max_trace(vm, l);
        } else if (arg == "--gc-growth-trigger") {
            const char *arg = next_arg(i,args).c_str();
            char *ep;
            double v = std::strtod(arg, &ep);
            if (*ep != '\0' || *arg == '\0') {
                std::cerr << "ERROR: Invalid number \"" << arg << "\""
                          << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            if (v < 0) {
                std::cerr << "ERROR: Invalid --gc-growth-trigger \""
                          << arg << "\"\n" << std::endl;
                usage(std::cerr);
                return EXIT_FAILURE;
            }
            jsonnet_gc_growth_trigger(vm, v);
        } else if (arg == "-e" || arg == "--exec") {
            config->set_filename_is_code(true);
        } else if (arg == "-m" || arg == "--multi") {
            config->set_multi(true);
            std::string output_dir = next_arg(i, args);
            if (output_dir.length() == 0) {
                std::cerr << "ERROR: -m argument was empty string" << std::endl;
                return EXIT_FAILURE;
            }
            if (output_dir[output_dir.length() - 1] != '/') {
                output_dir += '/';
            }
            config->set_output_dir(output_dir);
        } else if (arg == "-o" || arg == "--output-file") {
            std::string output_file = next_arg(i, args);
            if (output_file.length() == 0) {
                std::cerr << "ERROR: -o argument was empty string" << std::endl;
                return EXIT_FAILURE;
            }
            config->set_output_file(output_file);
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

    const char *want = config->filename_is_code() ? "code" : "filename";
    if (remaining_args.size() == 0) {
        std::cerr << "ERROR: Must give " << want << "\n" << std::endl;
        usage(std::cerr);
        return false;
    }

    std::string filename = remaining_args[0];
    if (remaining_args.size() > 1) {
        std::cerr << "ERROR: Already specified " << want
                  << " as \"" << filename << "\"\n"
                  << std::endl;
        usage(std::cerr);
        return false;
    }
    config->set_input_file(filename);
    return true;
}

/** Reads Jsonnet code from the input file or stdin into the input buffer. */
static bool read_input(JsonnetConfig* config, std::string* input) {
    if (config->filename_is_code()) {
        *input = config->input_file();
        config->set_input_file("<cmdline>");
    } else {
        if (config->input_file() == "-") {
            config->set_input_file("<stdin>");
            input->assign(std::istreambuf_iterator<char>(std::cin),
                          std::istreambuf_iterator<char>());
        } else {
            std::ifstream f;
            f.open(config->input_file().c_str());
            if (!f.good()) {
                std::string msg = "Opening input file: " + config->input_file();
                perror(msg.c_str());
                return false;
            }
            input->assign(std::istreambuf_iterator<char>(f),
                          std::istreambuf_iterator<char>());
            if (!f.good()) {
                std::string msg = "Reading input file: " + config->input_file();
                perror(msg.c_str());
                return false;
            }
        }
    }
    return true;
}

/** Writes output files for multiple file output */
static bool write_multi_output_files(JsonnetVm* vm, char* output,
                                     const std::string& output_dir) {
    // If multiple file output is used, then iterate over each string from
    // the sequence of strings returned by jsonnet_evaluate_snippet_multi,
    // construct pairs of filename and content, and write each output file.
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
        const std::string &filename = output_dir + pair.first;
        std::cout << filename << std::endl;
        {
            std::ifstream exists(filename.c_str());
            if (exists.good()) {
                std::string existing_content;
                existing_content.assign(std::istreambuf_iterator<char>(exists),
                                        std::istreambuf_iterator<char>());
                if (existing_content == new_content) {
                    // Do not bump the timestamp on the file if its content is
                    // the same. This may trigger other tools (e.g. make) to do
                    // unnecessary work.
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
            return false;
        }
        f << new_content;
        f.close();
        if (!f.good()) {
            std::string msg = "Writing to output file: " + filename;
            perror(msg.c_str());
            jsonnet_destroy(vm);
            return false;
        }
    }
    std::cout.flush();
    return true;
}

/** Writes the output JSON to the specified output file for single-file
 * output
 */
static bool write_output_file(const char* output,
                              const std::string &output_file) {
    if (output_file.empty()) {
        std::cout << output;
        std::cout.flush();
        return true;
    }
    std::ofstream f;
    f.open(output_file.c_str());
    if (!f.good()) {
        std::string msg = "Writing to output file: " + output_file;
        perror(msg.c_str());
        return false;
    }
    f << output;
    f.close();
    if (!f.good()) {
        std::string msg = "Writing to output file: " + output_file;
        perror(msg.c_str());
        return false;
    }
    return true;
}

int main(int argc, const char **argv)
{
    try {
        JsonnetVm *vm = jsonnet_make();
        JsonnetConfig config;
        if (!process_args(argc, argv, &config, vm)) {
            return EXIT_FAILURE;
        }

        // Read input files.
        std::string input;
        if (!read_input(&config, &input)) {
            return EXIT_FAILURE;
        }

        // Set import callbacks for jpaths.
        ImportCallbackContext import_callback_ctx{vm, config.mutable_jpaths()};
        jsonnet_import_callback(vm, import_callback, &import_callback_ctx);

        // Evaluate input Jsonnet and handle any errors from Jsonnet VM.
        int error;
        char *output;
        if (config.multi()) {
            output = jsonnet_evaluate_snippet_multi(
                vm, config.input_file().c_str(), input.c_str(), &error);
        } else {
            output = jsonnet_evaluate_snippet(
                vm, config.input_file().c_str(), input.c_str(), &error);
        }

        if (error) {
            std::cerr << output;
            std::cerr.flush();
            jsonnet_realloc(vm, output, 0);
            jsonnet_destroy(vm);
            return EXIT_FAILURE;
        }

        // Write output JSON.
        if (config.multi()) {
            if (!write_multi_output_files(vm, output, config.output_dir())) {
                return EXIT_FAILURE;
            }
        } else {
            bool successful =  write_output_file(output, config.output_file());
            jsonnet_realloc(vm, output, 0);
            if (!successful) {
                jsonnet_destroy(vm);
                return EXIT_FAILURE;
            }
        }
        jsonnet_destroy(vm);
        return EXIT_SUCCESS;

    } catch (const std::bad_alloc &) {
        // Avoid further allocation attempts
        fputs("Internal out-of-memory error (please report this)\n", stderr);
    } catch (const std::exception &e) {
        std::cerr << "Internal error (please report this): "
                  << e.what() << std::endl;
    } catch (...) {
        std::cerr << "An unknown exception occurred (please report this)."
                  << std::endl;
    }
    return EXIT_FAILURE;
}

