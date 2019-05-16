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

#ifndef JSONNET_CMD_UTILS_H
#define JSONNET_CMD_UTILS_H

#include <string>
#include <vector>

/** Like std::strtol with base 10 but exits the program if str is not a number.
 */
long strtol_check(const std::string &str);

/** Advances i and returns args[i]. 
 * Exits the program if args is not large enough.
 */
std::string next_arg(unsigned &i, const std::vector<std::string> &args);

/** Collect commandline args into a vector of strings, and expand -foo to -f -o -o. */
std::vector<std::string> simplify_args(int argc, const char **argv);

/** Reads from the input file or stdin into the input buffer. */
bool read_input_content(std::string filename, std::string *input);

/** Changes filename for stdin and filename_is_code case. */
void change_special_filename(bool filename_is_code, std::string *filename);

/** Gets Jsonnet code from any source into the input buffer and changes
 * the filename if it's not an actual filename (e.g. "-"). */
bool read_input(
    bool filename_is_code, std::string *filename, std::string *input);

/** Writes the output text to the specified output file.
 */
bool write_output_file(const char *output, const std::string &output_file);

#endif