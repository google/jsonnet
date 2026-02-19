#include <iostream>
#include <string>
#include <libjsonnet++.h>

int main(int argc, char **argv) {
    jsonnet::Jsonnet vm;
    vm.init();
    std::string code = "local c=3;{a:1,b:2,c:c}";
    std::string output;
    std::cout << "evaluating " << code << std::endl;
    if (vm.evaluateSnippet("-", code, &output)) {
        std::cout << "result:\n" << output << std::endl;
        return 0;
    } else {
        std::cout << "FAILURE:\n" << vm.lastError() << std::endl;
        return 1;
    }
}
