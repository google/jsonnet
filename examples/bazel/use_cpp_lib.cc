#include <libjsonnet++.h>
#include <iostream>

int main(int argc, char **argv) {
    int ret = 0;
    std::cerr << "Jsonnet version: " << jsonnet::Jsonnet::version() << std::endl;

    jsonnet::Jsonnet vm;
    if (!vm.init()) {
        return 1;
    }

    std::string result;
    bool ok = vm.evaluateFile("example.jsonnet", &result);
    if (ok) {
        std::cout << result << std::endl;
    } else {
        std::cerr << vm.lastError() << std::endl;
        ret = 2;
    }
    return ret;
}
