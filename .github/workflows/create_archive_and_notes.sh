#!/usr/bin/env bash
# Copyright 2023 The Bazel Authors. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -o errexit -o nounset -o pipefail

# Set by GH actions, see
# https://docs.github.com/en/actions/learn-github-actions/environment-variables#default-environment-variables
TAG=${GITHUB_REF_NAME}
# A prefix is added to better match the GitHub generated archives.
PREFIX="jsonnet-${TAG}"
ARCHIVE="jsonnet-$TAG.tar.gz"
git archive --format=tar --prefix=${PREFIX}/ ${TAG} | gzip > $ARCHIVE
SHA=$(shasum -a 256 $ARCHIVE | awk '{print $1}')

cat > release_notes.txt << EOF
### Importing Jsonnet in a project that uses Bazel

#### Using Bzlmod with Bazel 6

Add to your \`MODULE.bazel\` file:

\`\`\`starlark
bazel_dep(name = "jsonnet", version = "${TAG}")
\`\`\`

#### Using WORKSPACE

Paste this snippet into your \`WORKSPACE\` file:

\`\`\`starlark
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

http_archive(
    name = "jsonnet",
    sha256 = "${SHA}",
    strip_prefix = "${PREFIX}",
    url = "https://github.com/google/jsonnet/releases/download/${TAG}/jsonnet-${TAG}.tar.gz",
)
\`\`\`

EOF
