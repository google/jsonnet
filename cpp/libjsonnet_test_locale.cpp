#include <iostream>
#include <sstream>
#include <locale>
#include <cassert>
#include <libjsonnet++.h>

// Regression test for the follwing issue: https://github.com/google/jsonnet/issues/722

// Define a custom numpunct facet, so that we don't rely on any particular locale
// being available on the system where the test is built and run.
class Punctuator : public std::numpunct<char> {
protected:
    virtual char do_decimal_point() const override { return '!'; }
    virtual std::string do_grouping() const override { return "\1\2"; }
    virtual char do_thousands_sep() const override { return '\''; }
};

int main() {
    std::string templatedJSONString { "20000.5" };
    Punctuator punctuator;
    std::locale glocale(std::locale::classic(), &punctuator);
    std::locale::global(glocale);

    // Self-test to make sure the custom global locale is actually being applied.
    std::ostringstream ss;
    ss << 20000.5;
    assert(ss.str() == "20'00'0!5");

    jsonnet::Jsonnet jsonnet {};
    jsonnet.init();

    std::string expanded;
    if (!jsonnet.evaluateSnippet("", templatedJSONString, &expanded)) {
        std::cerr << "Error parsing Jsonnet: "+jsonnet.lastError();
        exit(1);
    }
    std::string expected = "20000.5\n";
    assert(expected == expanded);
    return 0;
}
