#! /bin/bash

set -o pipefail
set -o nounset
set -o errexit

red="$(tput setaf 1)"
green="$(tput setaf 2)"
reset="$(tput sgr0)"

failed=0
for jsonnet_test in $(ls tests/*.jsonnet); do
  echo "${green}Running ${jsonnet_test}...${reset}"
  ./jsonnet \
    --code-file "tests=${jsonnet_test}" \
    --exec \
    --string \
    'local testing = import "stdlib/testing.jsonnet"; testing.run(std.extVar("tests"))' \
    && errno=0 || errno=${?}

  if [[ "${errno}" != "0" ]]; then
    ((failed++)) || :
    echo "${red}${jsonnet_test} failed${reset}"
    echo
  fi
done

if [[ "${failed}" -gt 0 ]]; then
  echo "${red}(${failed}/$(ls tests/*.jsonnet | wc -l)) test packages failed.${reset}"
  exit 1
fi

echo "${green}OK${reset}"
exit 0

