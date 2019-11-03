#include <iostream>
#include <locale>
#include <cassert>
#include <libjsonnet++.h>

// Regression test for the follwing issue: https://github.com/google/jsonnet/issues/722

int main() {
    std::string templatedJSONString { "2000" };
    std::locale glocale("en_US.UTF-8"); 
    std::locale::global(glocale);

    jsonnet::Jsonnet jsonnet {};
    jsonnet.init();

    std::string expanded;
    if (!jsonnet.evaluateSnippet("", templatedJSONString, &expanded)) {
        std::cerr << "Error parsing Jsonnet: "+jsonnet.lastError();
        exit(1);
    }
    std::string expected = "2000\n";
    assert(expected == expanded);
    return 0;
}
