#include "c4/log.hpp"

#include "c4/libtest/supprwarn_push.hpp"
#include "c4/test.hpp"

namespace c4 {

TEST(LogBuffer, basic)
{
#define _CHECK(s, str) \
    EXPECT_EQ(strncmp(s.rd(), str, s.pos), 0) << " string was '" << s.rd() << "'"; \
    s.clear(); \
    EXPECT_EQ(s.pos, 0);\
    EXPECT_EQ(s.buf[0], '\0')

    LogBuffer b;
    const char *foo = "Foo";
    const char *bars_str = "123";
    int bars = 123;

    // raw writing
    b.write("hello world I am ");
    b.write(foo);
    b.write(" and I frobnicate ");
    b.write(bars_str); // only accepts const char*
    b.write(" Bars");
    _CHECK(b, "hello world I am Foo and I frobnicate 123 Bars");

    // chevron-style AKA iostream-style
    b << "hello world I am " << foo << " and I frobnicate " << bars << " Bars";
    _CHECK(b, "hello world I am Foo and I frobnicate 123 Bars");

    // c-style, not type safe
    b.printf("hello world I am %s and I frobnicate %d Bars", foo, bars);
    _CHECK(b, "hello world I am Foo and I frobnicate 123 Bars");

    // python-style, type safe
    b.print("hello world I am {} and I frobnicate {} Bars", foo, bars);
    _CHECK(b, "hello world I am Foo and I frobnicate 123 Bars");

    // r-style, type safe
    b.cat("hello world I am ", foo, " and I frobnicate ", bars, " Bars");
    _CHECK(b, "hello world I am Foo and I frobnicate 123 Bars");

    // using separators: this is unpractical...
    const char *s[] = {"now", "we", "have", "11", "strings", "to", "cat", "one", "after", "the", "other"};
    b.cat(s[0], ' ', s[1], ' ', s[2], ' ', s[3], ' ', s[4], ' ',
          s[5], ' ', s[6], ' ', s[7], ' ', s[8], ' ', s[9], ' ', s[10]);
    _CHECK(b, "now we have 11 strings to cat one after the other");

    // ... and this resolves it
    b.catsep(' ', s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10]);
    _CHECK(b, "now we have 11 strings to cat one after the other");

    b.catsep('_', s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10]);
    _CHECK(b, "now_we_have_11_strings_to_cat_one_after_the_other");

    // can be a full string
    b.catsep("____", s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10]);
    _CHECK(b, "now____we____have____11____strings____to____cat____one____after____the____other");

    // or just a general object
    b.catsep(22, s[0], s[1], s[2], s[3], s[4], s[5], s[6], s[7], s[8], s[9], s[10]);
    _CHECK(b, "now22we22have221122strings22to22cat22one22after22the22other");

}

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
