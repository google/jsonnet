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
    #include <libjsonnet.h>
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
    o << "Typical Usage:\n";
    o << "jsonnet [<cmd>] {<option>} <filename>\n";
    o << "Where <cmd> is one of {eval, fmt} and defaults to eval.\n";
    o << "Available eval options:\n";
    o << "  -h / --help             This message\n";
    o << "  -e / --exec             Treat filename as code\n";
    o << "  -J / --jpath <dir>      Specify an additional library search dir\n";
    o << "  -V / --var <var>=<val>  Specify an 'external' var to the given value\n";
    o << "  -E / --env <var>        Bring in an environment var as an 'external' var\n";
    o << "  -F / --file <var=<val>  Specify and 'external' var as the contents of a given file\n";
    o << "  --code-var <var>=<val>  As --var but value is Jsonnet code\n";
    o << "  --code-env <var>        As --env but env var contains Jsonnet code\n";
    o << "  --code-file <var>=<val> As --file but file contents is Jsonnet code\n";
    o << "  -o / --output-file <file> Write to the output file rather than stdout\n";
    o << "  -m / --multi <dir>      Write multiple files to the directory, list files on stdout\n";
    o << "  -y / --yaml-stream      Write output as a YAML stream of JSON documents\n";
    o << "  -S / --string           Expect a string, manifest as plain text\n";
    o << "  -s / --max-stack <n>    Number of allowed stack frames\n";
    o << "  -t / --max-trace <n>    Max length of stack trace before cropping\n";
    o << "  --gc-min-objects <n>    Do not run garbage collector until this many\n";
    o << "  --gc-growth-trigger <n> Run garbage collector after this amount of object growth\n";
    o << "  --version               Print version\n";
    o << "\n";
    o << "Available fmt options:\n";
    o << "jsonnet fmt {<option>} <filename>\n";
    o << "and <option> can be:\n";
    o << "  -h / --help             This message\n";
    o << "  -e / --exec             Treat filename as code\n";
    o << "  -o / --output-file <file> Write to the output file rather than stdout\n";
    o << "  -i / --in-place         Update the Jsonnet file in place.  Same as -o <filename>\n";
    o << "  --test                  Exit with failure if reformatting changed the file.\n";
    o << "  -n / --indent <n>       Number of spaces to indent by (default 0, means no change)\n";
    o << "  --max-blank-lines <n>   Max vertical spacing, 0 means no change (default 2)\n";
    o << "  --string-style <d|s|l>  Enforce double, single quotes or 'leave' (the default)\n";
    o << "  --comment-style <h|s|l> # (h)  // (s)  or 'leave' (default) never changes she-bang\n";
    o << "  --[no-]pretty-field-names Use syntax sugar for fields and indexing (on by default)\n";
    o << "  --[no-]pad-arrays       [ 1, 2, 3 ] instead of [1, 2, 3]\n";
    o << "  --[no-]pad-objects      { x: 1, x: 2 } instead of {x: 1, y: 2} (on by default)\n";
    o << "  --debug-desugaring      Unparse the desugared AST without executing it\n";
    o << "  --version               Print version\n";
    o << "\n";
    o << "In all cases:\n";
    o << "<filename> can be - (stdin)\n";
    o << "Multichar options are expanded e.g. -abc becomes -a -b -c.\n";
    o << "The -- option suppresses option processing for subsequent arguments.\n";
    o << "Note that since filenames and jsonnet programs can begin with -, it is advised to\n";
    o << "use -- if the argument is unknown, e.g. jsonnet -- \"$FILENAME\".";
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

enum Command {
    EVAL,
    FMT,
};

/** Class for representing configuration read from command line flags.  */
struct JsonnetConfig {
    Command cmd;
    std::string inputFile;
    std::string outputFile;
    bool filenameIsCode;

    // EVAL flags
    bool evalMulti;
    bool evalStream;
    std::string evalMultiOutputDir;

    // FMT flags
    bool fmtInPlace;
    bool fmtTest;

    JsonnetConfig()
      : cmd(EVAL), filenameIsCode(false),
        evalMulti(false),
        evalStream(false),
        fmtInPlace(false),
        fmtTest(false)
    { }
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

    unsigned i = 0;
    if (args.size() > 0 && args[i] == "fmt") {
        config->cmd = FMT;
        i++;
    } else if (args.size() > 0 && args[i] == "eval") {
        config->cmd = EVAL;
        i++;
    }

    for (; i<args.size() ; ++i) {
        const std::string &arg = args[i];
        if (arg == "-h" || arg == "--help") {
            usage(std::cout);
            return false;
        } else if (arg == "-v" || arg == "--version") {
            version(std::cout);
            return false;
        } else if (arg == "-e" || arg == "--exec") {
            config->filenameIsCode = true;
        } else if (arg == "-o" || arg == "--output-file") {
            std::string output_file = next_arg(i, args);
            if (output_file.length() == 0) {
                std::cerr << "ERROR: -o argument was empty string" << std::endl;
                return EXIT_FAILURE;
            }
            config->outputFile = output_file;
        } else if (arg == "--") {
            // All subsequent args are not options.
            while ((++i) < args.size())
                remaining_args.push_back(args[i]);
            break;
        } else if (config->cmd == EVAL) {
            if (arg == "-s" || arg == "--max-stack") {
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
                jsonnet_jpath_add(vm, dir.c_str());
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
            } else if (arg == "--code-file") {
                const std::string var_val = next_arg(i, args);
                size_t eq_pos = var_val.find_first_of('=', 0);
                if (eq_pos == std::string::npos) {
                    std::cerr << "ERROR: argument not in form <var>=<val> \""
                              << var_val << "\"." << std::endl;
                    return EXIT_FAILURE;
                }
                const std::string var = var_val.substr(0, eq_pos);
                const std::string path = var_val.substr(eq_pos + 1,
                                                       std::string::npos);

                std::ifstream file(path);
                std::string val((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

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
            } else if (arg == "-m" || arg == "--multi") {
                config->evalMulti = true;
                std::string output_dir = next_arg(i, args);
                if (output_dir.length() == 0) {
                    std::cerr << "ERROR: -m argument was empty string" << std::endl;
                    return EXIT_FAILURE;
                }
                if (output_dir[output_dir.length() - 1] != '/') {
                    output_dir += '/';
                }
                config->evalMultiOutputDir = output_dir;
            } else if (arg == "-y" || arg == "--yaml-stream") {
                config->evalStream = true;
            } else if (arg == "-S" || arg == "--string") {
                jsonnet_string_output(vm, 1);
            } else {
                remaining_args.push_back(args[i]);
            }

        } else {
            assert(config->cmd == FMT);
            if (arg == "-i" || arg == "--in-place") {
                config->fmtInPlace = true;
            } else if (arg == "--test") {
                config->fmtTest = true;
            } else if (arg == "-n" || arg == "--indent") {
                long l = strtol_check(next_arg(i, args));
                if (l < 0) {
                    std::cerr << "ERROR: Invalid --indent value: " << l << "\n"
                              << std::endl;
                    usage(std::cerr);
                    return false;
                }
                jsonnet_fmt_indent(vm, l);
            } else if (arg == "--max-blank-lines") {
                long l = strtol_check(next_arg(i, args));
                if (l < 0) {
                    std::cerr << "ERROR: Invalid --max-blank-lines value: " << l << "\n"
                              << std::endl;
                    usage(std::cerr);
                    return false;
                }
                jsonnet_fmt_max_blank_lines(vm, l);
            } else if (arg == "--comment-style") {
                const std::string val = next_arg(i, args);
                if (val == "h") {
                    jsonnet_fmt_comment(vm, 'h');
                } else if (val == "s") {
                    jsonnet_fmt_comment(vm, 's');
                } else if (val == "l") {
                    jsonnet_fmt_comment(vm, 'l');
                } else {
                    std::cerr << "ERROR: Invalid --comment-style value: " << val << "\n"
                              << std::endl;
                    usage(std::cerr);
                    return false;
                }
            } else if (arg == "--string-style") {
                const std::string val = next_arg(i, args);
                if (val == "d") {
                    jsonnet_fmt_string(vm, 'd');
                } else if (val == "s") {
                    jsonnet_fmt_string(vm, 's');
                } else if (val == "l") {
                    jsonnet_fmt_string(vm, 'l');
                } else {
                    std::cerr << "ERROR: Invalid --string-style value: " << val << "\n"
                              << std::endl;
                    usage(std::cerr);
                    return false;
                }
            } else if (arg == "--pad-arrays") {
                jsonnet_fmt_pad_arrays(vm, true);
            } else if (arg == "--no-pad-arrays") {
                jsonnet_fmt_pad_arrays(vm, false);
            } else if (arg == "--pad-objects") {
                jsonnet_fmt_pad_objects(vm, true);
            } else if (arg == "--no-pad-objects") {
                jsonnet_fmt_pad_objects(vm, false);
            } else if (arg == "--pretty-field-names") {
                jsonnet_fmt_pretty_field_names(vm, true);
            } else if (arg == "--no-pretty-field-names") {
                jsonnet_fmt_pretty_field_names(vm, false);
            } else if (arg == "--debug-desugaring") {
                jsonnet_fmt_debug_desugaring(vm, true);
            } else {
                remaining_args.push_back(args[i]);
            }
        }
    }

    const char *want = config->filenameIsCode ? "code" : "filename";
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
    config->inputFile = filename;
    return true;
}

/** Reads Jsonnet code from the input file or stdin into the input buffer. */
static bool read_input(JsonnetConfig* config, std::string* input) {
    if (config->filenameIsCode) {
        *input = config->inputFile;
        config->inputFile = "<cmdline>";
    } else {
        if (config->inputFile == "-") {
            config->inputFile = "<stdin>";
            input->assign(std::istreambuf_iterator<char>(std::cin),
                          std::istreambuf_iterator<char>());
        } else {
            std::ifstream f;
            f.open(config->inputFile.c_str());
            if (!f.good()) {
                std::string msg = "Opening input file: " + config->inputFile;
                perror(msg.c_str());
                return false;
            }
            input->assign(std::istreambuf_iterator<char>(f),
                          std::istreambuf_iterator<char>());
            if (!f.good()) {
                std::string msg = "Reading input file: " + config->inputFile;
                perror(msg.c_str());
                return false;
            }
        }
    }
    return true;
}

/** Writes output files for multiple file output */
static bool write_multi_output_files(JsonnetVm* vm, char* output,
                                     const std::string& output_dir)
{
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

/** Writes output files for YAML stream output */
static bool write_output_stream(JsonnetVm* vm, char* output)
{
    // If YAML stream output is used, then iterate over each string from
    // the sequence of strings returned by jsonnet_evaluate_snippet_stream,
    // and add the --- and ... as defined by the YAML spec.
    std::vector<std::string> r;
    for (const char *c=output ; *c!='\0' ; ) {
        const char *json = c;
        while (*c != '\0') ++c;
        ++c;
        r.emplace_back(json);
    }
    jsonnet_realloc(vm, output, 0);
    for (const auto &str : r) {
        std::cout << "---\n";
        std::cout << str;
    }
    if (r.size() > 0)
        std::cout << "...\n";
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

        // Evaluate input Jsonnet and handle any errors from Jsonnet VM.
        int error;
        char *output;
        switch (config.cmd) {
            case EVAL: {
                if (config.evalMulti) {
                    output = jsonnet_evaluate_snippet_multi(
                        vm, config.inputFile.c_str(), input.c_str(), &error);
                } else if (config.evalStream) {
                    output = jsonnet_evaluate_snippet_stream(
                        vm, config.inputFile.c_str(), input.c_str(), &error);
                } else {
                    output = jsonnet_evaluate_snippet(
                        vm, config.inputFile.c_str(), input.c_str(), &error);
                }

                if (error) {
                    std::cerr << output;
                    std::cerr.flush();
                    jsonnet_realloc(vm, output, 0);
                    jsonnet_destroy(vm);
                    return EXIT_FAILURE;
                }

                // Write output JSON.
                if (config.evalMulti) {
                    if (!write_multi_output_files(vm, output, config.evalMultiOutputDir)) {
                        return EXIT_FAILURE;
                    }
                } else if (config.evalStream) {
                    if (!write_output_stream(vm, output)) {
                        return EXIT_FAILURE;
                    }
                } else {
                    bool successful = write_output_file(output, config.outputFile);
                    jsonnet_realloc(vm, output, 0);
                    if (!successful) {
                        jsonnet_destroy(vm);
                        return EXIT_FAILURE;
                    }
                }
            }
            break;

            case FMT: {
                std::string output_file = config.outputFile;
                if (config.fmtInPlace) {
                    if (config.inputFile == "-") {
                        std::cerr << "ERROR: Cannot use --in-place with stdin" << std::endl;
                        jsonnet_destroy(vm);
                        return EXIT_FAILURE;
                    }
                    if (config.filenameIsCode) {
                        std::cerr << "ERROR: Cannot use --in-place with --exec" << std::endl;
                        jsonnet_destroy(vm);
                        return EXIT_FAILURE;
                    }
                    output_file = config.inputFile;
                }

                output = jsonnet_fmt_snippet(vm, config.inputFile.c_str(), input.c_str(), &error);

                if (error) {
                    std::cerr << output;
                    std::cerr.flush();
                    jsonnet_realloc(vm, output, 0);
                    jsonnet_destroy(vm);
                    return EXIT_FAILURE;
                }

                if (config.fmtTest) {
                    // Check the output matches the input.
                    bool ok = output == input;
                    jsonnet_realloc(vm, output, 0);
                    jsonnet_destroy(vm);
                    return ok ? EXIT_SUCCESS : 2;

                } else {
                    // Write output Jsonnet.
                    bool successful = write_output_file(output, output_file);
                    jsonnet_realloc(vm, output, 0);
                    if (!successful) {
                        jsonnet_destroy(vm);
                        return EXIT_FAILURE;
                    }
                }
            }
            break;
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

