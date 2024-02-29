/*
Copyright 2024 Google Inc. All rights reserved.

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

#ifndef JSONNET_PATH_UTILS_H
#define JSONNET_PATH_UTILS_H

#include <string>

namespace jsonnet::internal {

/** Get everything except the filename from a path. If the path is just a filename, returns "".
 *
 * The result includes the trailing directory separator (if there is one in the input).
 * Exact behaviour is platform-specific (different platforms use different directory separators).
 */
std::string path_dir_with_trailing_separator(const std::string &path);

}  // namespace jsonnet::internal

#endif

