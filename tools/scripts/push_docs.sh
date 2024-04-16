#!/usr/bin/env bash
# Copyright 2015 Google Inc. All rights reserved.
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

# Publishes a new version of the Jsonnet website.
#
# This script does the following:
#
# 1. Builds an archive containing the Jekyll website
# 2. In a temporary directory or the provided WORKING_DIR, clones the
#    Jsonnet website, and checks out the gh-pages branch
# 3. Replaces the contents of the repository with the new version of the site
#    from the archive
# 4. Commits the new version of the site and pushes to the remote repository.
#
# Usage:
# push_docs.sh [WORKING_DIR]

set -e

readonly JSONNET_REPO="git@github.com.:google/jsonnet.git"
readonly JSONNET_REPO_HTTPS="https://github.com/google/jsonnet.git"

function check {
  which $1 > /dev/null || (echo "$1 not installed. Please install $1."; exit 1)
}

[ -x ./jsonnet ] || (echo "Please build ./jsonnet first."; exit 1)
check git
check jekyll

if [ ! -t 0 ]; then
  echo "Not attached to a TTY. Can't prompt the user for confirmation. Exiting." >&1
  exit 1
fi

if [ ! -r 'doc/_config.yml' ]; then
  echo 'No doc/_config.yml file found.' >&1
  echo 'Are you running this script from the root of the Jsonnet repository?' >&1
  exit 1
fi

working_dir=$1
if [ -z "$working_dir" ]; then
  working_dir=$(mktemp -d "/tmp/jsonnet_gh_pages_XXXX")
  # Clean it up automatically on exit.
  trap "rm -rf ${working_dir}" EXIT
fi

git clone \
  -b gh-pages \
  --origin origin \
  --config "remote.origin.pushUrl=$JSONNET_REPO" \
  "$JSONNET_REPO_HTTPS" \
  "$working_dir"

(
  cd "$working_dir"
  git checkout gh-pages
  rm $(git ls-tree --name-only -r HEAD)
)

tools/scripts/update_web_content.sh
jekyll build -s doc -d "$working_dir"

if [ ! -r "$working_dir/js/libjsonnet.wasm" ]; then
  echo 'We have no js/libjsonnet.wasm; restoring the existing one from git' >&1
  (
    cd "$working_dir"
    git checkout -- js/libjsonnet.wasm
  )
fi

(
  cd "$working_dir"
  git add .
  git commit -am "Update docs."

  if [ -t 0 ]; then
    # We're on a TTY. Prompt the user for input.
    while true; do
      read -p "Push new gh-pages commit? Y/n " yn
      case "$yn" in
        Y* )
          break
          ;;
        [nN]* )
          echo "ok; aborting."
          exit
          ;;
        * )
          echo "please enter 'Y' or 'n'"
          ;;
      esac
    done
  else
    # Not on a TTY. Exit.
    exit 1
  fi

  echo "Pushing gh-pages..."
  git push -u origin gh-pages
)
