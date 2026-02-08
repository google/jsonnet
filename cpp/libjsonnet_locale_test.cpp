#include <iostream>
#include <sstream>
#include <locale>
#include <cassert>
#include <libjsonnet++.h>
#include "gtest/gtest.h"

// Regression test for the follwing issue: https://github.com/google/jsonnet/issues/722

// Define a custom numpunct facet, so that we don't rely on any particular locale
// being available on the system where the test is built and run.
class Punctuator : public std::numpunct<char> {
protected:
    virtual char do_decimal_point() const override { return '!'; }
    virtual std::string do_grouping() const override { return "\1\2"; }
    virtual char do_thousands_sep() const override { return '\''; }
};

class TestWithModifiedGlobalLocale : public testing::Test {
    protected:
        TestWithModifiedGlobalLocale():
            glocale_(std::locale::classic(), new Punctuator()) {}

        void SetUp() override {
            this->previous_global_locale_ = std::locale::global(this->glocale_);
        }
        void TearDown() override {
            std::locale::global(this->previous_global_locale_);
        }
    private:
        std::locale glocale_;
        std::locale previous_global_locale_;
};

TEST_F(TestWithModifiedGlobalLocale, Test) {
    std::string templatedJSONString { "20000.5" };

    // Self-test to make sure the custom global locale is actually being applied.
    std::ostringstream ss;
    ss << 20000.5;
    EXPECT_EQ(ss.str(), "20'00'0!5");

    jsonnet::Jsonnet jsonnet {};
    ASSERT_TRUE(jsonnet.init());

    std::string expanded;
    ASSERT_TRUE(jsonnet.evaluateSnippet("", templatedJSONString, &expanded))
        << "Error parsing Jsonnet: " << jsonnet.lastError();
    std::string expected = "20000.5\n";
    EXPECT_EQ(expected, expanded);
}
