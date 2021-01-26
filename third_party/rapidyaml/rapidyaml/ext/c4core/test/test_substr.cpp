#include "c4/std/std.hpp"
#include "c4/substr.hpp"

#include <c4/test.hpp>

#include "c4/libtest/supprwarn_push.hpp"
#include <iostream>

namespace c4 {

TEST_CASE("substr.ctor_from_char")
{
    char buf1[] = "{foo: 1}";
    char buf2[] = "{foo: 2}";
    substr s(buf1);
    CHECK_EQ(s, "{foo: 1}");
    s = buf2;
    CHECK_EQ(s, "{foo: 2}");
}

TEST_CASE("csubstr.ctor_from_char")
{
    char buf1[] = "{foo: 1}";
    char buf2[] = "{foo: 2}";
    csubstr s(buf1);
    CHECK_EQ(s, "{foo: 1}");
    s = buf2;
    CHECK_EQ(s, "{foo: 2}");
}

TEST_CASE("csubstr.empty_vs_null")
{
    csubstr s;
    CHECK_UNARY(s.empty());
    CHECK_UNARY(s.len == 0);
    CHECK_UNARY(s.str == nullptr);
    CHECK_UNARY(s == nullptr);

    s = "";
    CHECK_UNARY(s.empty());
    CHECK_UNARY(s.len == 0);
    CHECK_UNARY(s.str != nullptr);
    CHECK_UNARY(s != nullptr);

    s = nullptr;
    CHECK_UNARY(s.empty());
    CHECK_UNARY(s.len == 0);
    CHECK_UNARY(s.str == nullptr);
    CHECK_UNARY(s == nullptr);

    s = "";
    CHECK_UNARY(s.empty());
    CHECK_UNARY(s.len == 0);
    CHECK_UNARY(s.str != nullptr);
    CHECK_UNARY(s != nullptr);

    s = {};
    CHECK_UNARY(s.empty());
    CHECK_UNARY(s.len == 0);
    CHECK_UNARY(s.str == nullptr);
    CHECK_UNARY(s == nullptr);

    csubstr pp(nullptr);
    CHECK_UNARY(pp.empty());
    CHECK_UNARY(pp.len == 0);
    CHECK_UNARY(pp.str == nullptr);
    CHECK_UNARY(pp == nullptr);
}

TEST_CASE("substr.is_sub")
{
    csubstr buf = "0123456789";

    // ref
    csubstr s;
    csubstr ref = buf.select("345");
    CHECK_EQ(ref, "345");
    CHECK_UNARY(buf.is_super(ref));
    CHECK_UNARY(ref.is_sub(buf));
    CHECK_FALSE(ref.is_super(buf));
    CHECK_FALSE(buf.is_sub(ref));

    buf.clear();
    ref.clear();
    CHECK_FALSE(buf.is_super(ref));
    CHECK_FALSE(ref.is_super(buf));
    CHECK_FALSE(ref.is_sub(buf));
    CHECK_FALSE(buf.is_sub(ref));

    buf = "";
    ref = buf;
    CHECK_FALSE(buf.is_super("a"));
    CHECK_UNARY(buf.is_super(ref));
}

TEST_CASE("substr.overlaps")
{
    csubstr buf = "0123456789";

    // ref
    csubstr s;
    csubstr ref = buf.select("345");
    CHECK_EQ(ref.len, 3);
    CHECK_EQ(ref, "345");

    // all_left
    s = buf.sub(0, 2);
    CHECK_EQ(s, "01");
    CHECK_FALSE(ref.overlaps(s));
    CHECK_FALSE(s.overlaps(ref));

    // all_left_tight
    s = buf.sub(0, 3);
    CHECK_EQ(s, "012");
    CHECK_FALSE(ref.overlaps(s));
    CHECK_FALSE(s.overlaps(ref));

    // overlap_left
    s = buf.sub(0, 4);
    CHECK_EQ(s, "0123");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));

    // inside_tight_left
    s = buf.sub(3, 1);
    CHECK_EQ(s, "3");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));
    s = buf.sub(3, 2);
    CHECK_EQ(s, "34");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));

    // all_inside_tight
    s = buf.sub(4, 1);
    CHECK_EQ(s, "4");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));
    s = buf.sub(3, 3);
    CHECK_EQ(s, "345");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));

    // inside_tight_right
    s = buf.sub(4, 2);
    CHECK_EQ(s, "45");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));
    s = buf.sub(5, 1);
    CHECK_EQ(s, "5");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));

    // overlap_right
    s = buf.sub(5, 2);
    CHECK_EQ(s, "56");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));
    s = buf.sub(5, 3);
    CHECK_EQ(s, "567");
    CHECK_UNARY(ref.overlaps(s));
    CHECK_UNARY(s.overlaps(ref));

    // all_right_tight
    s = buf.sub(6, 1);
    CHECK_EQ(s, "6");
    CHECK_FALSE(ref.overlaps(s));
    CHECK_FALSE(s.overlaps(ref));
    s = buf.sub(6, 2);
    CHECK_EQ(s, "67");
    CHECK_FALSE(ref.overlaps(s));
    CHECK_FALSE(s.overlaps(ref));

    // all_right
    s = buf.sub(7, 1);
    CHECK_EQ(s, "7");
    CHECK_FALSE(ref.overlaps(s));
    CHECK_FALSE(s.overlaps(ref));
    s = buf.sub(7, 2);
    CHECK_EQ(s, "78");
    CHECK_FALSE(ref.overlaps(s));
    CHECK_FALSE(s.overlaps(ref));

    // null vs null
    csubstr n1, n2;
    CHECK_EQ(n1.str, nullptr);
    CHECK_EQ(n2.str, nullptr);
    CHECK_EQ(n1.len, 0);
    CHECK_EQ(n2.len, 0);
    CHECK_FALSE(n1.overlaps(n2));
    CHECK_FALSE(n2.overlaps(n1));
}

TEST_CASE("substr.sub")
{
    CHECK_EQ(csubstr("10]").sub(0, 2), "10");
}

TEST_CASE("substr.range")
{
    csubstr s = "0123456789";
    CHECK_EQ(s.range(0, 10), "0123456789");
    CHECK_EQ(s.range(0    ), "0123456789");
    CHECK_EQ(s.range(1, 10), "123456789");
    CHECK_EQ(s.range(1    ), "123456789");
    CHECK_EQ(s.range(2, 10), "23456789");
    CHECK_EQ(s.range(2    ), "23456789");
    CHECK_EQ(s.range(3, 10), "3456789");
    CHECK_EQ(s.range(3    ), "3456789");
    CHECK_EQ(s.range(4, 10), "456789");
    CHECK_EQ(s.range(4    ), "456789");
    CHECK_EQ(s.range(5, 10), "56789");
    CHECK_EQ(s.range(5    ), "56789");
    CHECK_EQ(s.range(6, 10), "6789");
    CHECK_EQ(s.range(6    ), "6789");
    CHECK_EQ(s.range(7, 10), "789");
    CHECK_EQ(s.range(7    ), "789");
    CHECK_EQ(s.range(8, 10), "89");
    CHECK_EQ(s.range(8    ), "89");
    CHECK_EQ(s.range(9, 10), "9");
    CHECK_EQ(s.range(9    ), "9");
    CHECK_EQ(s.range(10, 10), "");
    CHECK_EQ(s.range(10    ), "");

    CHECK_EQ(s.range(0 , 9), "012345678");
    CHECK_EQ(s.range(1 , 9), "12345678");
    CHECK_EQ(s.range(2 , 9), "2345678");
    CHECK_EQ(s.range(3 , 9), "345678");
    CHECK_EQ(s.range(4 , 9), "45678");
    CHECK_EQ(s.range(5 , 9), "5678");
    CHECK_EQ(s.range(6 , 9), "678");
    CHECK_EQ(s.range(7 , 9), "78");
    CHECK_EQ(s.range(8 , 9), "8");
    CHECK_EQ(s.range(9 , 9), "");

    CHECK_EQ(s.range(0 , 7), "0123456");
    CHECK_EQ(s.range(1 , 7), "123456");
    CHECK_EQ(s.range(2 , 7), "23456");
    CHECK_EQ(s.range(3 , 7), "3456");
    CHECK_EQ(s.range(4 , 7), "456");
    CHECK_EQ(s.range(5 , 7), "56");
    CHECK_EQ(s.range(6 , 7), "6");
    CHECK_EQ(s.range(7 , 7), "");

    CHECK_EQ(s.range(0 , 5), "01234");
    CHECK_EQ(s.range(1 , 5), "1234");
    CHECK_EQ(s.range(2 , 5), "234");
    CHECK_EQ(s.range(3 , 5), "34");
    CHECK_EQ(s.range(4 , 5), "4");
    CHECK_EQ(s.range(5 , 5), "");

    CHECK_EQ(s.range(0 , 3), "012");
    CHECK_EQ(s.range(1 , 3), "12");
    CHECK_EQ(s.range(2 , 3), "2");
    CHECK_EQ(s.range(3 , 3), "");

    CHECK_EQ(s.range(0 , 2), "01");
    CHECK_EQ(s.range(1 , 2), "1");
    CHECK_EQ(s.range(2 , 2), "");

    CHECK_EQ(s.range(0 , 1), "0");
    CHECK_EQ(s.range(1 , 1), "");
}

TEST_CASE("substr.first")
{
    csubstr s = "0123456789";

    CHECK_EQ(s.first(csubstr::npos), s);

    CHECK_EQ(s.first(10), "0123456789");
    CHECK_EQ(s.first(9), "012345678");
    CHECK_EQ(s.first(8), "01234567");
    CHECK_EQ(s.first(7), "0123456");
    CHECK_EQ(s.first(6), "012345");
    CHECK_EQ(s.first(5), "01234");
    CHECK_EQ(s.first(4), "0123");
    CHECK_EQ(s.first(3), "012");
    CHECK_EQ(s.first(2), "01");
    CHECK_EQ(s.first(1), "0");
    CHECK_EQ(s.first(0), "");
}

TEST_CASE("substr.last")
{
    csubstr s = "0123456789";

    CHECK_EQ(s.last(csubstr::npos), s);

    CHECK_EQ(s.last(10), "0123456789");
    CHECK_EQ(s.last(9), "123456789");
    CHECK_EQ(s.last(8), "23456789");
    CHECK_EQ(s.last(7), "3456789");
    CHECK_EQ(s.last(6), "456789");
    CHECK_EQ(s.last(5), "56789");
    CHECK_EQ(s.last(4), "6789");
    CHECK_EQ(s.last(3), "789");
    CHECK_EQ(s.last(2), "89");
    CHECK_EQ(s.last(1), "9");
    CHECK_EQ(s.last(0), "");
}

TEST_CASE("substr.offs")
{
    csubstr s = "0123456789";

    CHECK_EQ(s.offs(0, 0), s);

    CHECK_EQ(s.offs(1, 0), "123456789");
    CHECK_EQ(s.offs(0, 1), "012345678");
    CHECK_EQ(s.offs(1, 1), "12345678");

    CHECK_EQ(s.offs(1, 2), "1234567");
    CHECK_EQ(s.offs(2, 1), "2345678");
    CHECK_EQ(s.offs(2, 2), "234567");

    CHECK_EQ(s.offs(2, 3), "23456");
    CHECK_EQ(s.offs(3, 2), "34567");
    CHECK_EQ(s.offs(3, 3), "3456");

    CHECK_EQ(s.offs(3, 4), "345");
    CHECK_EQ(s.offs(4, 3), "456");
    CHECK_EQ(s.offs(4, 4), "45");

    CHECK_EQ(s.offs(4, 5), "4");
    CHECK_EQ(s.offs(5, 4), "5");
    CHECK_EQ(s.offs(5, 5), "");
}

TEST_CASE("substr.count")
{
    csubstr buf = "0123456789";

    CHECK_EQ(buf.count('0'), 1);
    CHECK_EQ(buf.count('0', 0), 1);
    CHECK_EQ(buf.count('0', 1), 0);
    CHECK_EQ(buf.count('0', buf.len), 0);

    CHECK_EQ(buf.count('1'), 1);
    CHECK_EQ(buf.count('1', 0), 1);
    CHECK_EQ(buf.count('1', 1), 1);
    CHECK_EQ(buf.count('1', 2), 0);
    CHECK_EQ(buf.count('1', buf.len), 0);

    CHECK_EQ(buf.count('2'), 1);
    CHECK_EQ(buf.count('2', 0), 1);
    CHECK_EQ(buf.count('2', 1), 1);
    CHECK_EQ(buf.count('2', 2), 1);
    CHECK_EQ(buf.count('2', 3), 0);
    CHECK_EQ(buf.count('2', buf.len), 0);

    CHECK_EQ(buf.count('2'), 1);
    CHECK_EQ(buf.count('2', 0), 1);
    CHECK_EQ(buf.count('2', 1), 1);
    CHECK_EQ(buf.count('2', 2), 1);
    CHECK_EQ(buf.count('2', 3), 0);
    CHECK_EQ(buf.count('2', buf.len), 0);

    CHECK_EQ(buf.count('3'), 1);
    CHECK_EQ(buf.count('3', 0), 1);
    CHECK_EQ(buf.count('3', 1), 1);
    CHECK_EQ(buf.count('3', 2), 1);
    CHECK_EQ(buf.count('3', 3), 1);
    CHECK_EQ(buf.count('3', 4), 0);
    CHECK_EQ(buf.count('3', buf.len), 0);

    CHECK_EQ(buf.count('4'), 1);
    CHECK_EQ(buf.count('4', 0), 1);
    CHECK_EQ(buf.count('4', 1), 1);
    CHECK_EQ(buf.count('4', 2), 1);
    CHECK_EQ(buf.count('4', 3), 1);
    CHECK_EQ(buf.count('4', 4), 1);
    CHECK_EQ(buf.count('4', 5), 0);
    CHECK_EQ(buf.count('4', buf.len), 0);

    CHECK_EQ(buf.count('5'), 1);
    CHECK_EQ(buf.count('5', 0), 1);
    CHECK_EQ(buf.count('5', 1), 1);
    CHECK_EQ(buf.count('5', 2), 1);
    CHECK_EQ(buf.count('5', 3), 1);
    CHECK_EQ(buf.count('5', 4), 1);
    CHECK_EQ(buf.count('5', 5), 1);
    CHECK_EQ(buf.count('5', 6), 0);
    CHECK_EQ(buf.count('5', buf.len), 0);

    CHECK_EQ(buf.count('a'), 0);
    CHECK_EQ(buf.count('a', 0), 0);
    CHECK_EQ(buf.count('a', 1), 0);
    CHECK_EQ(buf.count('a', 2), 0);
    CHECK_EQ(buf.count('a', 3), 0);
    CHECK_EQ(buf.count('a', 4), 0);
    CHECK_EQ(buf.count('a', 5), 0);
    CHECK_EQ(buf.count('a', 6), 0);
    CHECK_EQ(buf.count('a', buf.len), 0);

    buf = "00110022003300440055";
    CHECK_EQ(buf.count('0', 0), 10);
    CHECK_EQ(buf.count('0', 1), 9);
    CHECK_EQ(buf.count('0', 2), 8);
    CHECK_EQ(buf.count('0', 3), 8);
    CHECK_EQ(buf.count('0', 4), 8);
    CHECK_EQ(buf.count('0', 5), 7);
    CHECK_EQ(buf.count('0', 6), 6);
    CHECK_EQ(buf.count('0', 7), 6);
    CHECK_EQ(buf.count('0', 8), 6);
    CHECK_EQ(buf.count('0', 9), 5);
    CHECK_EQ(buf.count('0', 10), 4);
    CHECK_EQ(buf.count('0', 11), 4);
    CHECK_EQ(buf.count('0', 12), 4);
    CHECK_EQ(buf.count('0', 13), 3);
    CHECK_EQ(buf.count('0', 14), 2);
    CHECK_EQ(buf.count('0', 15), 2);
    CHECK_EQ(buf.count('0', 16), 2);
    CHECK_EQ(buf.count('0', 17), 1);
    CHECK_EQ(buf.count('0', 18), 0);
    CHECK_EQ(buf.count('0', 19), 0);
    CHECK_EQ(buf.count('0', 20), 0);

    CHECK_EQ(buf.count('1', 0), 2);
    CHECK_EQ(buf.count('1', 1), 2);
    CHECK_EQ(buf.count('1', 2), 2);
    CHECK_EQ(buf.count('1', 3), 1);
    CHECK_EQ(buf.count('1', 4), 0);
    CHECK_EQ(buf.count('1', 5), 0);
}

TEST_CASE("substr.select")
{
    csubstr buf = "0123456789";

    CHECK_EQ(buf.select('0'), "0");
    CHECK_EQ(buf.select('1'), "1");
    CHECK_EQ(buf.select('2'), "2");
    CHECK_EQ(buf.select('8'), "8");
    CHECK_EQ(buf.select('9'), "9");

    CHECK_EQ(buf.select('a').str, nullptr);
    CHECK_EQ(buf.select('a').len, 0);
    CHECK_EQ(buf.select('a'), "");

    CHECK_EQ(buf.select("a").str, nullptr);
    CHECK_EQ(buf.select("a").len, 0);
    CHECK_EQ(buf.select("a"), "");

    CHECK_EQ(buf.select("0"), "0");
    CHECK_EQ(buf.select("0").str, buf.str+0);
    CHECK_EQ(buf.select("0").len, 1);

    CHECK_EQ(buf.select("1"), "1");
    CHECK_EQ(buf.select("1").str, buf.str+1);
    CHECK_EQ(buf.select("1").len, 1);

    CHECK_EQ(buf.select("2"), "2");
    CHECK_EQ(buf.select("2").str, buf.str+2);
    CHECK_EQ(buf.select("2").len, 1);

    CHECK_EQ(buf.select("9"), "9");
    CHECK_EQ(buf.select("9").str, buf.str+9);
    CHECK_EQ(buf.select("9").len, 1);

    CHECK_EQ(buf.select("012"), "012");
    CHECK_EQ(buf.select("012").str, buf.str+0);
    CHECK_EQ(buf.select("012").len, 3);

    CHECK_EQ(buf.select("345"), "345");
    CHECK_EQ(buf.select("345").str, buf.str+3);
    CHECK_EQ(buf.select("345").len, 3);

    CHECK_EQ(buf.select("789"), "789");
    CHECK_EQ(buf.select("789").str, buf.str+7);
    CHECK_EQ(buf.select("789").len, 3);

    CHECK_EQ(buf.select("89a"), "");
    CHECK_EQ(buf.select("89a").str, nullptr);
    CHECK_EQ(buf.select("89a").len, 0);
}

TEST_CASE("substr.begins_with")
{
    CHECK (csubstr(": ").begins_with(":" ));
    CHECK (csubstr(": ").begins_with(':' ));
    CHECK_FALSE(csubstr(":") .begins_with(": "));

    CHECK (csubstr(    "1234").begins_with('0', 0));
    CHECK (csubstr(   "01234").begins_with('0', 1));
    CHECK_FALSE(csubstr(   "01234").begins_with('0', 2));
    CHECK (csubstr(  "001234").begins_with('0', 1));
    CHECK (csubstr(  "001234").begins_with('0', 2));
    CHECK_FALSE(csubstr(  "001234").begins_with('0', 3));
    CHECK (csubstr( "0001234").begins_with('0', 1));
    CHECK (csubstr( "0001234").begins_with('0', 2));
    CHECK (csubstr( "0001234").begins_with('0', 3));
    CHECK_FALSE(csubstr( "0001234").begins_with('0', 4));
    CHECK (csubstr("00001234").begins_with('0', 1));
    CHECK (csubstr("00001234").begins_with('0', 2));
    CHECK (csubstr("00001234").begins_with('0', 3));
    CHECK (csubstr("00001234").begins_with('0', 4));
    CHECK_FALSE(csubstr("00001234").begins_with('0', 5));
}

TEST_CASE("substr.ends_with")
{
    CHECK_UNARY(csubstr("{% if foo %}bar{% endif %}").ends_with("{% endif %}"));

    CHECK (csubstr("1234"    ).ends_with('0', 0));
    CHECK (csubstr("12340"   ).ends_with('0', 1));
    CHECK_FALSE(csubstr("12340"   ).ends_with('0', 2));
    CHECK (csubstr("123400"  ).ends_with('0', 1));
    CHECK (csubstr("123400"  ).ends_with('0', 2));
    CHECK_FALSE(csubstr("123400"  ).ends_with('0', 3));
    CHECK (csubstr("1234000" ).ends_with('0', 1));
    CHECK (csubstr("1234000" ).ends_with('0', 2));
    CHECK (csubstr("1234000" ).ends_with('0', 3));
    CHECK_FALSE(csubstr("1234000" ).ends_with('0', 4));
    CHECK (csubstr("12340000").ends_with('0', 1));
    CHECK (csubstr("12340000").ends_with('0', 2));
    CHECK (csubstr("12340000").ends_with('0', 3));
    CHECK (csubstr("12340000").ends_with('0', 4));
    CHECK_FALSE(csubstr("12340000").ends_with('0', 5));
}

TEST_CASE("substr.first_of")
{
    size_t npos = csubstr::npos;

    CHECK_EQ(csubstr("012345").first_of('a'), npos);
    CHECK_EQ(csubstr("012345").first_of("ab"), npos);

    CHECK_EQ(csubstr("012345").first_of('0'), 0u);
    CHECK_EQ(csubstr("012345").first_of("0"), 0u);
    CHECK_EQ(csubstr("012345").first_of("01"), 0u);
    CHECK_EQ(csubstr("012345").first_of("10"), 0u);
    CHECK_EQ(csubstr("012345").first_of("012"), 0u);
    CHECK_EQ(csubstr("012345").first_of("210"), 0u);
    CHECK_EQ(csubstr("012345").first_of("0123"), 0u);
    CHECK_EQ(csubstr("012345").first_of("3210"), 0u);
    CHECK_EQ(csubstr("012345").first_of("01234"), 0u);
    CHECK_EQ(csubstr("012345").first_of("43210"), 0u);
    CHECK_EQ(csubstr("012345").first_of("012345"), 0u);
    CHECK_EQ(csubstr("012345").first_of("543210"), 0u);

    CHECK_EQ(csubstr("012345").first_of('5'), 5u);
    CHECK_EQ(csubstr("012345").first_of("5"), 5u);
    CHECK_EQ(csubstr("012345").first_of("45"), 4u);
    CHECK_EQ(csubstr("012345").first_of("54"), 4u);
    CHECK_EQ(csubstr("012345").first_of("345"), 3u);
    CHECK_EQ(csubstr("012345").first_of("543"), 3u);
    CHECK_EQ(csubstr("012345").first_of("2345"), 2u);
    CHECK_EQ(csubstr("012345").first_of("5432"), 2u);
    CHECK_EQ(csubstr("012345").first_of("12345"), 1u);
    CHECK_EQ(csubstr("012345").first_of("54321"), 1u);
    CHECK_EQ(csubstr("012345").first_of("012345"), 0u);
    CHECK_EQ(csubstr("012345").first_of("543210"), 0u);

    CHECK_EQ(csubstr("012345").first_of('0', 6u), npos);
    CHECK_EQ(csubstr("012345").first_of('5', 6u), npos);
    CHECK_EQ(csubstr("012345").first_of("012345", 6u), npos);
}

TEST_CASE("substr.last_of")
{
    size_t npos = csubstr::npos;

    CHECK_EQ(csubstr("012345").last_of('a'), npos);
    CHECK_EQ(csubstr("012345").last_of("ab"), npos);

    CHECK_EQ(csubstr("012345").last_of('0'), 0u);
    CHECK_EQ(csubstr("012345").last_of("0"), 0u);
    CHECK_EQ(csubstr("012345").last_of("01"), 1u);
    CHECK_EQ(csubstr("012345").last_of("10"), 1u);
    CHECK_EQ(csubstr("012345").last_of("012"), 2u);
    CHECK_EQ(csubstr("012345").last_of("210"), 2u);
    CHECK_EQ(csubstr("012345").last_of("0123"), 3u);
    CHECK_EQ(csubstr("012345").last_of("3210"), 3u);
    CHECK_EQ(csubstr("012345").last_of("01234"), 4u);
    CHECK_EQ(csubstr("012345").last_of("43210"), 4u);
    CHECK_EQ(csubstr("012345").last_of("012345"), 5u);
    CHECK_EQ(csubstr("012345").last_of("543210"), 5u);

    CHECK_EQ(csubstr("012345").last_of('5'), 5u);
    CHECK_EQ(csubstr("012345").last_of("5"), 5u);
    CHECK_EQ(csubstr("012345").last_of("45"), 5u);
    CHECK_EQ(csubstr("012345").last_of("54"), 5u);
    CHECK_EQ(csubstr("012345").last_of("345"), 5u);
    CHECK_EQ(csubstr("012345").last_of("543"), 5u);
    CHECK_EQ(csubstr("012345").last_of("2345"), 5u);
    CHECK_EQ(csubstr("012345").last_of("5432"), 5u);
    CHECK_EQ(csubstr("012345").last_of("12345"), 5u);
    CHECK_EQ(csubstr("012345").last_of("54321"), 5u);
    CHECK_EQ(csubstr("012345").last_of("012345"), 5u);
    CHECK_EQ(csubstr("012345").last_of("543210"), 5u);

    CHECK_EQ(csubstr("012345").last_of('0', 6u), 0u);
    CHECK_EQ(csubstr("012345").last_of('5', 6u), 5u);
    CHECK_EQ(csubstr("012345").last_of("012345", 6u), 5u);
}

TEST_CASE("substr.first_not_of")
{
    size_t npos = csubstr::npos;

    CHECK_EQ(csubstr("012345").first_not_of('a'), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("ab"), 0u);

    CHECK_EQ(csubstr("012345").first_not_of('0'), 1u);
    CHECK_EQ(csubstr("012345").first_not_of("0"), 1u);
    CHECK_EQ(csubstr("012345").first_not_of("01"), 2u);
    CHECK_EQ(csubstr("012345").first_not_of("10"), 2u);
    CHECK_EQ(csubstr("012345").first_not_of("012"), 3u);
    CHECK_EQ(csubstr("012345").first_not_of("210"), 3u);
    CHECK_EQ(csubstr("012345").first_not_of("0123"), 4u);
    CHECK_EQ(csubstr("012345").first_not_of("3210"), 4u);
    CHECK_EQ(csubstr("012345").first_not_of("01234"), 5u);
    CHECK_EQ(csubstr("012345").first_not_of("43210"), 5u);
    CHECK_EQ(csubstr("012345").first_not_of("012345"), npos);
    CHECK_EQ(csubstr("012345").first_not_of("543210"), npos);

    CHECK_EQ(csubstr("012345").first_not_of('5'), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("5"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("45"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("54"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("345"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("543"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("2345"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("5432"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("12345"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("54321"), 0u);
    CHECK_EQ(csubstr("012345").first_not_of("012345"), npos);
    CHECK_EQ(csubstr("012345").first_not_of("543210"), npos);

    CHECK_EQ(csubstr("").first_not_of('0', 0u), npos);
    CHECK_EQ(csubstr("012345").first_not_of('5', 6u), npos);
    CHECK_EQ(csubstr("012345").first_not_of("012345", 6u), npos);
}

TEST_CASE("substr.last_not_of")
{
    size_t npos = csubstr::npos;

    CHECK_EQ(csubstr("012345").last_not_of('a'), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("ab"), 5u);

    CHECK_EQ(csubstr("012345").last_not_of('5'), 4u);
    CHECK_EQ(csubstr("012345").last_not_of("5"), 4u);
    CHECK_EQ(csubstr("012345").last_not_of("45"), 3u);
    CHECK_EQ(csubstr("012345").last_not_of("54"), 3u);
    CHECK_EQ(csubstr("012345").last_not_of("345"), 2u);
    CHECK_EQ(csubstr("012345").last_not_of("543"), 2u);
    CHECK_EQ(csubstr("012345").last_not_of("2345"), 1u);
    CHECK_EQ(csubstr("012345").last_not_of("5432"), 1u);
    CHECK_EQ(csubstr("012345").last_not_of("12345"), 0u);
    CHECK_EQ(csubstr("012345").last_not_of("54321"), 0u);
    CHECK_EQ(csubstr("012345").last_not_of("012345"), npos);
    CHECK_EQ(csubstr("012345").last_not_of("543210"), npos);

    CHECK_EQ(csubstr("012345").last_not_of('0'), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("0"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("01"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("10"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("012"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("210"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("0123"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("3210"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("01234"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("43210"), 5u);
    CHECK_EQ(csubstr("012345").last_not_of("012345"), npos);
    CHECK_EQ(csubstr("012345").last_not_of("543210"), npos);

    CHECK_EQ(csubstr("").last_not_of('0', 0u), npos);
    CHECK_EQ(csubstr("012345").last_not_of('5', 6u), 4);
}

TEST_CASE("substr.left_of")
{
    csubstr s = "012345";

    CHECK_EQ(s.left_of(csubstr::npos, /*include_pos*/false), s);
    CHECK_EQ(s.left_of(csubstr::npos, /*include_pos*/true), s);


    CHECK_EQ(s.left_of(0, /*include_pos*/false), "");
    CHECK_EQ(s.left_of(1, /*include_pos*/false), "0");
    CHECK_EQ(s.left_of(2, /*include_pos*/false), "01");
    CHECK_EQ(s.left_of(3, /*include_pos*/false), "012");
    CHECK_EQ(s.left_of(4, /*include_pos*/false), "0123");
    CHECK_EQ(s.left_of(5, /*include_pos*/false), "01234");
    CHECK_EQ(s.left_of(6, /*include_pos*/false), "012345");

    CHECK_UNARY(s.is_super(s.left_of(0, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.left_of(1, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.left_of(2, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.left_of(3, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.left_of(4, /*include_pos*/false)));


    CHECK_EQ(s.left_of(0, /*include_pos*/true), "0");
    CHECK_EQ(s.left_of(1, /*include_pos*/true), "01");
    CHECK_EQ(s.left_of(2, /*include_pos*/true), "012");
    CHECK_EQ(s.left_of(3, /*include_pos*/true), "0123");
    CHECK_EQ(s.left_of(4, /*include_pos*/true), "01234");
    CHECK_EQ(s.left_of(5, /*include_pos*/true), "012345");

    CHECK_UNARY(s.is_super(s.left_of(0, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.left_of(1, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.left_of(2, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.left_of(3, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.left_of(4, /*include_pos*/true)));


    CHECK_EQ(s.sub(5), "5");
    CHECK_EQ(s.sub(4), "45");
    CHECK_EQ(s.sub(3), "345");
    CHECK_EQ(s.sub(2), "2345");
    CHECK_EQ(s.sub(1), "12345");
    CHECK_EQ(s.sub(0), "012345");

    CHECK_EQ(s.left_of(s.sub(5)), "01234");
    CHECK_EQ(s.left_of(s.sub(4)), "0123");
    CHECK_EQ(s.left_of(s.sub(3)), "012");
    CHECK_EQ(s.left_of(s.sub(2)), "01");
    CHECK_EQ(s.left_of(s.sub(1)), "0");
    CHECK_EQ(s.left_of(s.sub(0)), "");

    CHECK_UNARY(s.is_super(s.left_of(s.sub(5))));
    CHECK_UNARY(s.is_super(s.left_of(s.sub(4))));
    CHECK_UNARY(s.is_super(s.left_of(s.sub(3))));
    CHECK_UNARY(s.is_super(s.left_of(s.sub(2))));
    CHECK_UNARY(s.is_super(s.left_of(s.sub(1))));
    CHECK_UNARY(s.is_super(s.left_of(s.sub(0))));
}

TEST_CASE("substr.right_of")
{
    csubstr s = "012345";

    CHECK_EQ(s.right_of(csubstr::npos, /*include_pos*/false), "");
    CHECK_EQ(s.right_of(csubstr::npos, /*include_pos*/true), "");


    CHECK_EQ(s.right_of(0, /*include_pos*/false), "12345");
    CHECK_EQ(s.right_of(1, /*include_pos*/false), "2345");
    CHECK_EQ(s.right_of(2, /*include_pos*/false), "345");
    CHECK_EQ(s.right_of(3, /*include_pos*/false), "45");
    CHECK_EQ(s.right_of(4, /*include_pos*/false), "5");
    CHECK_EQ(s.right_of(5, /*include_pos*/false), "");

    CHECK_UNARY(s.is_super(s.right_of(0, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.right_of(1, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.right_of(2, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.right_of(3, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.right_of(4, /*include_pos*/false)));
    CHECK_UNARY(s.is_super(s.right_of(5, /*include_pos*/false)));


    CHECK_EQ(s.right_of(0, /*include_pos*/true), "012345");
    CHECK_EQ(s.right_of(1, /*include_pos*/true), "12345");
    CHECK_EQ(s.right_of(2, /*include_pos*/true), "2345");
    CHECK_EQ(s.right_of(3, /*include_pos*/true), "345");
    CHECK_EQ(s.right_of(4, /*include_pos*/true), "45");
    CHECK_EQ(s.right_of(5, /*include_pos*/true), "5");
    CHECK_EQ(s.right_of(6, /*include_pos*/true), "");

    CHECK_UNARY(s.is_super(s.right_of(0, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.right_of(1, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.right_of(2, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.right_of(3, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.right_of(4, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.right_of(5, /*include_pos*/true)));
    CHECK_UNARY(s.is_super(s.right_of(6, /*include_pos*/true)));


    CHECK_EQ(s.sub(0, 0), "");
    CHECK_EQ(s.sub(0, 1), "0");
    CHECK_EQ(s.sub(0, 2), "01");
    CHECK_EQ(s.sub(0, 3), "012");
    CHECK_EQ(s.sub(0, 4), "0123");
    CHECK_EQ(s.sub(0, 5), "01234");
    CHECK_EQ(s.sub(0, 6), "012345");

    CHECK_EQ(s.right_of(s.sub(0, 0)), "012345");
    CHECK_EQ(s.right_of(s.sub(0, 1)), "12345");
    CHECK_EQ(s.right_of(s.sub(0, 2)), "2345");
    CHECK_EQ(s.right_of(s.sub(0, 3)), "345");
    CHECK_EQ(s.right_of(s.sub(0, 4)), "45");
    CHECK_EQ(s.right_of(s.sub(0, 5)), "5");
    CHECK_EQ(s.right_of(s.sub(0, 6)), "");

    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 0))));
    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 1))));
    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 2))));
    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 3))));
    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 4))));
    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 5))));
    CHECK_UNARY(s.is_super(s.right_of(s.sub(0, 6))));
}

TEST_CASE("substr.compare_different_length")
{
    const char s1[] = "one empty doc";
    const char s2[] = "one empty doc, explicit termination";
    csubstr c1(s1), c2(s2);
    CHECK_NE(c1, c2);
    CHECK_NE(c1, s2);
    CHECK_NE(s1, c2);
    CHECK_LT(c1, c2);
    CHECK_LT(c1, s2);
    CHECK_LT(s1, c2);
    CHECK_GT(c2, c1);
    CHECK_GT(c2, s1);
    CHECK_GT(s2, c1);
    CHECK_UNARY((c1 > c2) != (c1 < c2));
    CHECK_UNARY((c1 > s2) != (c1 < s2));
    CHECK_UNARY((s1 > c2) != (s1 < c2));
    CHECK_UNARY((c2 > c1) != (c2 < c1));
    CHECK_UNARY((c2 > s1) != (c2 < s1));
    CHECK_UNARY((s2 > c1) != (s2 < c1));
}

TEST_CASE("substr.compare_null")
{
    csubstr s1, s2, sp(" ");
    CHECK_EQ(s1, "");
    CHECK_EQ(s1, s2);
    CHECK(!(s1 > s2));
    CHECK(!(s1 < s2));
    CHECK((s1 <= s2));
    CHECK((s1 >= s2));
    CHECK(!(s1 != s2));
    CHECK_EQ(s1.compare('-'), -1);
    CHECK_EQ(sp.compare(' '), 0);
    CHECK_EQ(s1.compare("-", 1u), -1);
    CHECK_EQ(s1.compare("-", 0u), 0);
    CHECK_EQ(s1.compare((const char*)0, 0u), 0);
    CHECK_EQ(sp.compare((const char*)0, 0u), 1);
    CHECK_EQ(sp.compare(" ", 0u), 1);
    CHECK_EQ(sp.compare(" ", 1u), 0);
}

TEST_CASE("substr.compare_vs_char")
{
    CHECK_EQ(csubstr("-"), '-');
    CHECK_NE(csubstr("+"), '-');

    CHECK_NE(csubstr("---"), '-');
    CHECK_NE(csubstr("---"), "-");

    CHECK_NE(csubstr("aaa"), 'a');
    CHECK_NE(csubstr("aaa"), "a");

    CHECK_NE(csubstr("aaa"), 'b');
    CHECK_NE(csubstr("aaa"), "b");

    CHECK_LT(csubstr("aaa"), 'b');
    CHECK_LT(csubstr("aaa"), "b");

    CHECK_LE(csubstr("aaa"), 'b');
    CHECK_LE(csubstr("aaa"), "b");

    CHECK_NE(csubstr("bbb"), 'a');
    CHECK_NE(csubstr("bbb"), "a");

    CHECK_GT(csubstr("bbb"), 'a');
    CHECK_GT(csubstr("bbb"), "a");

    CHECK_GE(csubstr("bbb"), 'a');
    CHECK_GE(csubstr("bbb"), "a");
}

TEST_CASE("substr.mixed_cmp")
{
    // c++20 introduced new comparison rules and clang10 fails:
    //
    // error: ISO C++20 considers use of overloaded operator '==' (with operand
    // types 'const c4::basic_substring<char>' and 'const
    // c4::basic_substring<char>') to be ambiguous despite there being a unique
    // best viable function [-Werror,-Wambiguous-reversed-operator]

    char sa_[] = "a";
    char sb_[] = "b";
    csubstr csa = "a"; substr sa = sa_;
    csubstr csb = "b"; substr sb = sb_;

    CHECK_EQ(csa, csa);
    CHECK_EQ(sa, sa); // this fails
    CHECK_EQ(csa, sa);
    CHECK_EQ(sa, csa);

    CHECK_NE(sa, sb);
    CHECK_NE(csa, csb);
    CHECK_NE(csa, sb);
    CHECK_NE(sa, csb);

    CHECK_LT(sa,  sb);
    CHECK_LT(csa, csb);
    CHECK_LT(csa,  sb);
    CHECK_LT(sa, csb);

    CHECK_LE(sa, sb);
    CHECK_LE(csa, csb);
    CHECK_LE(csa,  sb);
    CHECK_LE(sa, csb);

    CHECK_LE(sa, sa);
    CHECK_LE(csa, csa);
    CHECK_LE(csa, sa);
    CHECK_LE(sa, csa);

    CHECK_GT(sb, sa);
    CHECK_GT(csb, csa);
    CHECK_GT(csb,  sa);
    CHECK_GT( sb, csa);

    CHECK_GE(sb,  sa);
    CHECK_GE(csb, csa);
    CHECK_GE(csb,  sa);
    CHECK_GE( sb, csa);

    CHECK_GE(sb,  sb);
    CHECK_GE(csb, csb);
    CHECK_GE(csb,  sb);
    CHECK_GE( sb, csb);
}

TEST_CASE("substr.eqne")
{
    char buf[128];
    for(size_t i = 0; i < 5; ++i) buf[i] = (char)('0' + i);
    csubstr cmp(buf, 5);

    CHECK_EQ(csubstr("01234"), cmp);
    CHECK_EQ(        "01234" , cmp);
    CHECK_EQ(             cmp, "01234");
    CHECK_NE(csubstr("0123"), cmp);
    CHECK_NE(        "0123" , cmp);
    CHECK_NE(            cmp, "0123");
    CHECK_NE(csubstr("012345"), cmp);
    CHECK_NE(        "012345" , cmp);
    CHECK_NE(              cmp, "012345");
}

TEST_CASE("substr.substr2csubstr")
{
    char b[] = "some string";
    substr s(b);
    csubstr sc = s;
    CHECK_EQ(sc, s);
    const substr cs(b);
    const csubstr csc(b);
}

template <class ...Args>
void test_first_of_any(csubstr input, bool true_or_false, size_t which, size_t pos, Args... args)
{
    csubstr::first_of_any_result r = input.first_of_any(to_csubstr(args)...);
    //std::cout << input << ": " << (bool(r) ? "true" : "false") << "/which:" << r.which << "/pos:" << r.pos << "\n";
    CHECK_EQ(r, true_or_false);
    if(true_or_false)
    {
        CHECK_UNARY(r);
    }
    else
    {
        CHECK_FALSE(r);
    }
    CHECK_EQ(r.which, which);
    CHECK_EQ(r.pos, pos);
}

TEST_CASE("substr.first_of_any")
{
    size_t NONE = csubstr::NONE;
    size_t npos = csubstr::npos;

    test_first_of_any("foobar"               , true , 0u  ,   3u, "bar", "barbell", "bark", "barff");
    test_first_of_any("foobar"               , false, NONE, npos,        "barbell", "bark", "barff");
    test_first_of_any("foobart"              , false, NONE, npos,        "barbell", "bark", "barff");

    test_first_of_any("10"                   , false, NONE, npos, "0x", "0X", "-0x", "-0X");
    test_first_of_any("10]"                  , false, NONE, npos, "0x", "0X", "-0x", "-0X");
    test_first_of_any(csubstr("10]").first(2), false, NONE, npos, "0x", "0X", "-0x", "-0X");


    test_first_of_any("baz{% endif %}", true, 0u, 3u, "{% endif %}", "{% if "         , "{% elif bar %}" , "{% else %}" );
    test_first_of_any("baz{% endif %}", true, 1u, 3u, "{% if "     , "{% endif %}"    , "{% elif bar %}" , "{% else %}" );
    test_first_of_any("baz{% endif %}", true, 2u, 3u, "{% if "     , "{% elif bar %}" , "{% endif %}"    , "{% else %}" );
    test_first_of_any("baz{% endif %}", true, 3u, 3u, "{% if "     , "{% elif bar %}" , "{% else %}"     , "{% endif %}");

    test_first_of_any("baz{% e..if %}", false, NONE, npos, "{% endif %}", "{% if "         , "{% elif bar %}" , "{% else %}" );
    test_first_of_any("baz{% e..if %}", false, NONE, npos, "{% if "     , "{% endif %}"    , "{% elif bar %}" , "{% else %}" );
    test_first_of_any("baz{% e..if %}", false, NONE, npos, "{% if "     , "{% elif bar %}" , "{% endif %}"    , "{% else %}" );
    test_first_of_any("baz{% e..if %}", false, NONE, npos, "{% if "     , "{% elif bar %}" , "{% else %}"     , "{% endif %}");


    test_first_of_any("bar{% else %}baz{% endif %}", true, 0u, 3u, "{% else %}" , "{% if "         , "{% elif bar %}" , "{% endif %}");
    test_first_of_any("bar{% else %}baz{% endif %}", true, 1u, 3u, "{% if "     , "{% else %}"     , "{% elif bar %}" , "{% endif %}");
    test_first_of_any("bar{% else %}baz{% endif %}", true, 2u, 3u, "{% if "     , "{% elif bar %}" , "{% else %}"     , "{% endif %}");
    test_first_of_any("bar{% else %}baz{% endif %}", true, 3u, 3u, "{% if "     , "{% elif bar %}" , "{% endif %}"    , "{% else %}" );

    test_first_of_any("bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% else %}" , "{% if "         , "{% elif bar %}" , "{% endif %}");
    test_first_of_any("bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "     , "{% else %}"     , "{% elif bar %}" , "{% endif %}");
    test_first_of_any("bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "     , "{% elif bar %}" , "{% else %}"     , "{% endif %}");
    test_first_of_any("bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "     , "{% elif bar %}" , "{% endif %}"    , "{% else %}" );


    test_first_of_any("foo{% elif bar %}bar{% else %}baz{% endif %}", true, 0u, 3u, "{% elif bar %}" , "{% if "         , "{% else %}"     , "{% endif %}"   );
    test_first_of_any("foo{% elif bar %}bar{% else %}baz{% endif %}", true, 1u, 3u, "{% if "         , "{% elif bar %}" , "{% else %}"     , "{% endif %}"   );
    test_first_of_any("foo{% elif bar %}bar{% else %}baz{% endif %}", true, 2u, 3u, "{% if "         , "{% else %}"     , "{% elif bar %}" , "{% endif %}"   );
    test_first_of_any("foo{% elif bar %}bar{% else %}baz{% endif %}", true, 3u, 3u, "{% if "         , "{% else %}"     , "{% endif %}"    , "{% elif bar %}");

    test_first_of_any("foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% elif bar %}" , "{% if "         , "{% else %}"     , "{% endif %}"   );
    test_first_of_any("foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "         , "{% elif bar %}" , "{% else %}"     , "{% endif %}"   );
    test_first_of_any("foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "         , "{% else %}"     , "{% elif bar %}" , "{% endif %}"   );
    test_first_of_any("foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "         , "{% else %}"     , "{% endif %}"    , "{% elif bar %}");


    test_first_of_any("{% if foo %}foo{% elif bar %}bar{% else %}baz{% endif %}", true, 0u, 0u, "{% if "         , "{% elif bar %}" , "{% else %}" , "{% endif %}" );
    test_first_of_any("{% if foo %}foo{% elif bar %}bar{% else %}baz{% endif %}", true, 1u, 0u, "{% elif bar %}" , "{% if "         , "{% else %}" , "{% endif %}" );
    test_first_of_any("{% if foo %}foo{% elif bar %}bar{% else %}baz{% endif %}", true, 2u, 0u, "{% elif bar %}" , "{% else %}"     , "{% if "     , "{% endif %}" );
    test_first_of_any("{% if foo %}foo{% elif bar %}bar{% else %}baz{% endif %}", true, 3u, 0u, "{% elif bar %}" , "{% else %}"     , "{% endif %}", "{% if "      );

    test_first_of_any("{% .. foo %}foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% if "         , "{% elif bar %}" , "{% else %}" , "{% endif %}" );
    test_first_of_any("{% .. foo %}foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% elif bar %}" , "{% if "         , "{% else %}" , "{% endif %}" );
    test_first_of_any("{% .. foo %}foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% elif bar %}" , "{% else %}"     , "{% if "     , "{% endif %}" );
    test_first_of_any("{% .. foo %}foo{% e..f bar %}bar{% e..e %}baz{% e..if %}", false, NONE, npos, "{% elif bar %}" , "{% else %}"     , "{% endif %}", "{% if "      );
}


TEST_CASE("substr.pair_range_esc")
{
    const char q = '\'';
    CHECK_EQ(csubstr("").pair_range_esc(q), "");
    CHECK_EQ(csubstr("'").pair_range_esc(q), "");
    CHECK_EQ(csubstr("''").pair_range_esc(q), "''");
    CHECK_EQ(csubstr("'\\'\\''").pair_range_esc(q), "'\\'\\''");
    CHECK_EQ(csubstr("asdasdasd''asdasd").pair_range_esc(q), "''");
    CHECK_EQ(csubstr("asdasdasd'abc'asdasda").pair_range_esc(q), "'abc'");
}

TEST_CASE("substr.pair_range")
{
    CHECK_EQ(csubstr("").pair_range('{', '}'), "");
    CHECK_EQ(csubstr("{").pair_range('{', '}'), "");
    CHECK_EQ(csubstr("}").pair_range('{', '}'), "");
    CHECK_EQ(csubstr("{}").pair_range('{', '}'), "{}");
    CHECK_EQ(csubstr("{abc}").pair_range('{', '}'), "{abc}");
    CHECK_EQ(csubstr("123{abc}456").pair_range('{', '}'), "{abc}");
}

TEST_CASE("substr.pair_range_nested")
{
    CHECK_EQ(csubstr("").pair_range_nested('{', '}'), "");
    CHECK_EQ(csubstr("{").pair_range_nested('{', '}'), "");
    CHECK_EQ(csubstr("}").pair_range_nested('{', '}'), "");
    CHECK_EQ(csubstr("{}").pair_range_nested('{', '}'), "{}");
    CHECK_EQ(csubstr("{abc}").pair_range_nested('{', '}'), "{abc}");
    CHECK_EQ(csubstr("123{abc}456").pair_range_nested('{', '}'), "{abc}");
    CHECK_EQ(csubstr("123{abc}456{def}").pair_range_nested('{', '}'), "{abc}");
    CHECK_EQ(csubstr(   "{{}}").pair_range_nested('{', '}'), "{{}}");
    CHECK_EQ(csubstr("123{{}}456").pair_range_nested('{', '}'), "{{}}");
    CHECK_EQ(csubstr(   "{a{}b{}c}").pair_range_nested('{', '}'), "{a{}b{}c}");
    CHECK_EQ(csubstr("123{a{}b{}c}456").pair_range_nested('{', '}'), "{a{}b{}c}");
    CHECK_EQ(csubstr(    "{a{{}}b{{}}c}").pair_range_nested('{', '}'), "{a{{}}b{{}}c}");
    CHECK_EQ(csubstr("123{a{{}}b{{}}c}456").pair_range_nested('{', '}'), "{a{{}}b{{}}c}");
    CHECK_EQ(csubstr(   "{{{}}a{{}}b{{}}c{{}}}").pair_range_nested('{', '}'), "{{{}}a{{}}b{{}}c{{}}}");
    CHECK_EQ(csubstr("123{{{}}a{{}}b{{}}c{{}}}456").pair_range_nested('{', '}'), "{{{}}a{{}}b{{}}c{{}}}");
}

TEST_CASE("substr.unquoted")
{
    CHECK_EQ(csubstr("").unquoted(), "");

    CHECK_EQ(csubstr("''").unquoted(), "");
    CHECK_EQ(csubstr("\"\"").unquoted(), "");

    CHECK_EQ(csubstr("'\''").unquoted(), "'");

    CHECK_EQ(csubstr("aa").unquoted(), "aa");
    CHECK_EQ(csubstr("'aa'").unquoted(), "aa");
    CHECK_EQ(csubstr("\"aa\"").unquoted(), "aa");
    CHECK_EQ(csubstr("'aa\''").unquoted(), "aa'");
}


TEST_CASE("substr.first_non_empty_span")
{
    CHECK_EQ(csubstr("foo bar").first_non_empty_span(), "foo");
    CHECK_EQ(csubstr("       foo bar").first_non_empty_span(), "foo");
    CHECK_EQ(csubstr("\n   \r  \t  foo bar").first_non_empty_span(), "foo");
    CHECK_EQ(csubstr("\n   \r  \t  foo\n\r\t bar").first_non_empty_span(), "foo");
    CHECK_EQ(csubstr("\n   \r  \t  foo\n\r\t bar").first_non_empty_span(), "foo");
    CHECK_EQ(csubstr(",\n   \r  \t  foo\n\r\t bar").first_non_empty_span(), ",");
}

TEST_CASE("substr.first_uint_span")
{
    CHECK_EQ(csubstr("1234").first_uint_span(), "1234");
    CHECK_EQ(csubstr("+1234").first_uint_span(), "+1234");
    CHECK_EQ(csubstr("-1234").first_uint_span(), "");
    CHECK_EQ(csubstr("1234 asdkjh").first_uint_span(), "1234");
    CHECK_EQ(csubstr("1234\rasdkjh").first_uint_span(), "1234");
    CHECK_EQ(csubstr("1234\tasdkjh").first_uint_span(), "1234");
    CHECK_EQ(csubstr("1234\nasdkjh").first_uint_span(), "1234");
    CHECK_EQ(csubstr("1234]asdkjh").first_uint_span(), "1234");
    CHECK_EQ(csubstr("1234)asdkjh").first_uint_span(), "1234");
    CHECK_EQ(csubstr("1234gasdkjh").first_uint_span(), "");
    CHECK_EQ(csubstr("1").first_uint_span(), "1");
    CHECK_EQ(csubstr("+1").first_uint_span(), "+1");
    CHECK_EQ(csubstr("-1").first_uint_span(), "");
    CHECK_EQ(csubstr("-0").first_uint_span(), "");
    CHECK_EQ(csubstr("0").first_uint_span(), "0");
    CHECK_EQ(csubstr("+0").first_uint_span(), "+0");
    CHECK_EQ(csubstr("-0").first_uint_span(), "");
    CHECK_EQ(csubstr("1234 abc").first_uint_span(), "1234");
    CHECK_EQ(csubstr("abc 1234 abc").first_uint_span(), "");
    CHECK_EQ(csubstr("+0x1234 abc").first_uint_span(), "+0x1234");
    CHECK_EQ(csubstr("-0x1234 abc").first_uint_span(), "");
    CHECK_EQ(csubstr("0x1234 abc").first_uint_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\rabc").first_uint_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\nabc").first_uint_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\tabc").first_uint_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234]abc").first_uint_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234)abc").first_uint_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234g").first_uint_span(), "");
    CHECK_EQ(csubstr("0b01").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("+0b01").first_uint_span(), "+0b01");
    CHECK_EQ(csubstr("-0b01").first_uint_span(), "");
    CHECK_EQ(csubstr("0b01 asdasd").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("0b01\rasdasd").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("0b01\tasdasd").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("0b01\nasdasd").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("0b01]asdasd").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("0b01)asdasd").first_uint_span(), "0b01");
    CHECK_EQ(csubstr("0b01hasdasd").first_uint_span(), "");
    CHECK_EQ(csubstr("+").first_uint_span(), "");
    CHECK_EQ(csubstr("-").first_uint_span(), "");
}

TEST_CASE("substr.first_int_span")
{
    CHECK_EQ(csubstr("1234").first_int_span(), "1234");
    CHECK_EQ(csubstr("+1234").first_int_span(), "+1234");
    CHECK_EQ(csubstr("-1234").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234 asdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234\rasdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234\tasdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234\nasdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234]asdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234)asdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234gasdkjh").first_int_span(), "");
    CHECK_EQ(csubstr("1").first_int_span(), "1");
    CHECK_EQ(csubstr("+1").first_int_span(), "+1");
    CHECK_EQ(csubstr("-1").first_int_span(), "-1");
    CHECK_EQ(csubstr("-0").first_int_span(), "-0");
    CHECK_EQ(csubstr("0").first_int_span(), "0");
    CHECK_EQ(csubstr("+0").first_int_span(), "+0");
    CHECK_EQ(csubstr("-0").first_int_span(), "-0");
    CHECK_EQ(csubstr("1234 abc").first_int_span(), "1234");
    CHECK_EQ(csubstr("abc 1234 abc").first_int_span(), "");
    CHECK_EQ(csubstr("+0x1234 abc").first_int_span(), "+0x1234");
    CHECK_EQ(csubstr("-0x1234 abc").first_int_span(), "-0x1234");
    CHECK_EQ(csubstr("0x1234 abc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\rabc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\nabc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\tabc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234]abc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234)abc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234gabc").first_int_span(), "");
    CHECK_EQ(csubstr("0b01").first_int_span(), "0b01");
    CHECK_EQ(csubstr("+0b01").first_int_span(), "+0b01");
    CHECK_EQ(csubstr("-0b01").first_int_span(), "-0b01");
    CHECK_EQ(csubstr("0b01 asdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01\rasdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01\tasdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01\nasdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01]asdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01)asdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01gasdasd").first_int_span(), "");
}

TEST_CASE("substr.first_real_span")
{
    CHECK_EQ(csubstr("1234").first_int_span(), "1234");
    CHECK_EQ(csubstr("+1234").first_int_span(), "+1234");
    CHECK_EQ(csubstr("-1234").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234 asdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234\rasdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234\tasdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234\nasdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234]asdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234)asdkjh").first_int_span(), "-1234");
    CHECK_EQ(csubstr("-1234gasdkjh").first_int_span(), "");
    CHECK_EQ(csubstr("1").first_int_span(), "1");
    CHECK_EQ(csubstr("+1").first_int_span(), "+1");
    CHECK_EQ(csubstr("-1").first_int_span(), "-1");
    CHECK_EQ(csubstr("-0").first_int_span(), "-0");
    CHECK_EQ(csubstr("0").first_int_span(), "0");
    CHECK_EQ(csubstr("+0").first_int_span(), "+0");
    CHECK_EQ(csubstr("-0").first_int_span(), "-0");
    CHECK_EQ(csubstr("1234 abc").first_int_span(), "1234");
    CHECK_EQ(csubstr("abc 1234 abc").first_int_span(), "");
    CHECK_EQ(csubstr("+0x1234 abc").first_int_span(), "+0x1234");
    CHECK_EQ(csubstr("-0x1234 abc").first_int_span(), "-0x1234");
    CHECK_EQ(csubstr("0x1234 abc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\rabc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\nabc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234\tabc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234]abc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234)abc").first_int_span(), "0x1234");
    CHECK_EQ(csubstr("0x1234gabc").first_int_span(), "");
    CHECK_EQ(csubstr("0b01").first_int_span(), "0b01");
    CHECK_EQ(csubstr("+0b01").first_int_span(), "+0b01");
    CHECK_EQ(csubstr("-0b01").first_int_span(), "-0b01");
    CHECK_EQ(csubstr("0b01 asdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01\rasdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01\tasdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01\nasdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01]asdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01)asdasd").first_int_span(), "0b01");
    CHECK_EQ(csubstr("0b01gasdasd").first_int_span(), "");
    CHECK_EQ(csubstr("+").first_int_span(), "");
    CHECK_EQ(csubstr("-").first_int_span(), "");
}

typedef enum : uint8_t { kIsNone = 0, kIsUint = 1, kIsInt = 3, kIsReal = 7 } NumberClass;
struct number
{
    csubstr num;
    NumberClass cls;

    template<size_t N>
    number(const char (&n)[N], NumberClass c) : num(n), cls(c) {}
    number(csubstr n, NumberClass c) : num(n), cls(c) {}


    void test(csubstr ref={})
    {
        if(ref.empty()) ref = num;
        switch(cls)
        {
        case kIsUint:
        {
            CHECK_EQ(num.first_uint_span(), ref);
            CHECK_EQ(num.first_int_span(), ref);
            CHECK_EQ(num.first_real_span(), ref);
            CHECK_UNARY(num.first_uint_span().is_integer());
            CHECK_UNARY(num.first_uint_span().is_number());
            break;
        }
        case kIsInt:
        {
            CHECK_EQ(num.first_uint_span(), "");
            CHECK_EQ(num.first_int_span(), ref);
            CHECK_EQ(num.first_real_span(), ref);
            CHECK_UNARY(num.first_int_span().is_integer());
            CHECK_UNARY(num.first_int_span().is_number());
            break;
        }
        case kIsReal:
        {
            CHECK_EQ(num.first_uint_span(), "");
            CHECK_EQ(num.first_int_span(), "");
            CHECK_EQ(num.first_real_span(), ref);
            CHECK_FALSE(num.first_real_span().is_integer());
            CHECK_UNARY(num .first_real_span().is_number());
            break;
        }
        case kIsNone:
        {
            CHECK_EQ(num.first_uint_span(), "");
            CHECK_EQ(num.first_int_span(), "");
            CHECK_EQ(num.first_real_span(), "");
            CHECK_FALSE(num.is_integer());
            CHECK_FALSE(num.is_number());
            break;
        }
        default:
        {
            CHECK_UNARY(false);//FAIL();
            break;
        }
        }
    }
};

const number numbers[] = {
    {"", kIsNone},
    // TODO: {".", kIsNone},
    {".0", kIsReal},
    {"0.", kIsReal},
    {"0.0", kIsReal},
    {"1234", kIsUint},
    {"+1234", kIsUint},
    {"-1234", kIsInt},
    {"1234.0", kIsReal},
    {"+1234.0", kIsReal},
    {"-1234.0", kIsReal},
    {"1", kIsUint},
    {"+1", kIsUint},
    {"-1", kIsInt},
    {"1.", kIsReal},
    {"+1.", kIsReal},
    {"-1.", kIsReal},
    {"0", kIsUint},
    {"+0", kIsUint},
    {"-0", kIsInt},
    {"1.", kIsReal},
    {"+1.", kIsReal},
    {"-1.", kIsReal},
    {".1", kIsReal},
    {"+.1", kIsReal},
    {"-.1", kIsReal},
    {"0x1234", kIsUint},
    {"+0x1234", kIsUint},
    {"-0x1234", kIsInt},
    {"0b01", kIsUint},
    {"+0b01", kIsUint},
    {"-0b01", kIsInt},
    {"1e1", kIsReal},
    {"1e+1", kIsReal},
    {"1e-1", kIsReal},
    {"1.e1", kIsReal},
    {"1.e-1", kIsReal},
    {"1.e+1", kIsReal},
    {"1.0e1", kIsReal},
    {"1.0e-1", kIsReal},
    {"1.0e+1", kIsReal},
    {"+1e1", kIsReal},
    {"+1e+1", kIsReal},
    {"+1e-1", kIsReal},
    {"+1.e1", kIsReal},
    {"+1.e-1", kIsReal},
    {"+1.e+1", kIsReal},
    {"+1.0e1", kIsReal},
    {"+1.0e-1", kIsReal},
    {"+1.0e+1", kIsReal},
    {"-1e1", kIsReal},
    {"-1e+1", kIsReal},
    {"-1e-1", kIsReal},
    {"-1.e1", kIsReal},
    {"-1.e-1", kIsReal},
    {"-1.e+1", kIsReal},
    {"-1.0e1", kIsReal},
    {"-1.0e-1", kIsReal},
    {"-1.0e+1", kIsReal},
    {"1e123", kIsReal},
    {"1e+123", kIsReal},
    {"1e-123", kIsReal},
    {"1.e123", kIsReal},
    {"1.e-123", kIsReal},
    {"1.e+123", kIsReal},
    {"1.0e123", kIsReal},
    {"1.0e-123", kIsReal},
    {"1.0e+123", kIsReal},
    {"+1e123", kIsReal},
    {"+1e+123", kIsReal},
    {"+1e-123", kIsReal},
    {"+1.e123", kIsReal},
    {"+1.e-123", kIsReal},
    {"+1.e+123", kIsReal},
    {"+1.0e123", kIsReal},
    {"+1.0e-123", kIsReal},
    {"+1.0e+123", kIsReal},
    {"-1e123", kIsReal},
    {"-1e+123", kIsReal},
    {"-1e-123", kIsReal},
    {"-1.e123", kIsReal},
    {"-1.e-123", kIsReal},
    {"-1.e+123", kIsReal},
    {"-1.0e123", kIsReal},
    {"-1.0e-123", kIsReal},
    {"-1.0e+123", kIsReal},
};

TEST_CASE("substr.is_number")
{
    for(number n : numbers)
    {
        n.test();
    }
    char buf[128];
    csubstr garbage = "sdkjhsdfkju";
    // adding anything before the number will make it not be a number
    for(number n : numbers)
    {
        for(int i = 0; i < 127; ++i)
        {
            char c = (char)i;
            csubstr fmtd = cat_sub(buf, garbage, c, n.num);
            number cp(fmtd, kIsNone);
            cp.test();
        }
    }
    // adding after may or may not make it a number
    for(number const& n : numbers)
    {
        for(int i = 0; i < 127; ++i)
        {
            number cp = n;
            char c = (char)i;
            cp.num = cat_sub(buf, n.num, c, garbage);
            if(!csubstr::_is_delim_char(c))
            {
                cp.cls = kIsNone;
            }
            cp.test(n.num);
        }
    }
}

TEST_CASE("substr.triml")
{
    using S = csubstr;

    CHECK_EQ(S("aaabbb"   ).triml('a' ), "bbb");
    CHECK_EQ(S("aaabbb"   ).triml('b' ), "aaabbb");
    CHECK_EQ(S("aaabbb"   ).triml('c' ), "aaabbb");
    CHECK_EQ(S("aaabbb"   ).triml("ab"), "");
    CHECK_EQ(S("aaabbb"   ).triml("ba"), "");
    CHECK_EQ(S("aaabbb"   ).triml("cd"), "aaabbb");
    CHECK_EQ(S("aaa...bbb").triml('a' ), "...bbb");
    CHECK_EQ(S("aaa...bbb").triml('b' ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").triml('c' ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").triml("ab"), "...bbb");
    CHECK_EQ(S("aaa...bbb").triml("ba"), "...bbb");
    CHECK_EQ(S("aaa...bbb").triml("ab."), "");
    CHECK_EQ(S("aaa...bbb").triml("a."), "bbb");
    CHECK_EQ(S("aaa...bbb").triml(".a"), "bbb");
    CHECK_EQ(S("aaa...bbb").triml("b."), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").triml(".b"), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").triml("cd"), "aaa...bbb");

    CHECK_EQ(S("ab"   ).triml('a' ), "b");
    CHECK_EQ(S("ab"   ).triml('b' ), "ab");
    CHECK_EQ(S("ab"   ).triml('c' ), "ab");
    CHECK_EQ(S("ab"   ).triml("ab"), "");
    CHECK_EQ(S("ab"   ).triml("ba"), "");
    CHECK_EQ(S("ab"   ).triml("cd"), "ab");
    CHECK_EQ(S("a...b").triml('a' ), "...b");
    CHECK_EQ(S("a...b").triml('b' ), "a...b");
    CHECK_EQ(S("a...b").triml('c' ), "a...b");
    CHECK_EQ(S("a...b").triml("ab"), "...b");
    CHECK_EQ(S("a...b").triml("ba"), "...b");
    CHECK_EQ(S("a...b").triml("ab."), "");
    CHECK_EQ(S("a...b").triml("a."), "b");
    CHECK_EQ(S("a...b").triml(".a"), "b");
    CHECK_EQ(S("a...b").triml("b."), "a...b");
    CHECK_EQ(S("a...b").triml(".b"), "a...b");
    CHECK_EQ(S("a...b").triml("cd"), "a...b");
}

TEST_CASE("substr.trimr")
{
    using S = csubstr;

    CHECK_EQ(S("aaabbb"   ).trimr('a' ), "aaabbb");
    CHECK_EQ(S("aaabbb"   ).trimr('b' ), "aaa");
    CHECK_EQ(S("aaabbb"   ).trimr('c' ), "aaabbb");
    CHECK_EQ(S("aaabbb"   ).trimr("ab"), "");
    CHECK_EQ(S("aaabbb"   ).trimr("ba"), "");
    CHECK_EQ(S("aaabbb"   ).trimr("cd"), "aaabbb");
    CHECK_EQ(S("aaa...bbb").trimr('a' ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trimr('b' ), "aaa...");
    CHECK_EQ(S("aaa...bbb").trimr('c' ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trimr("ab"), "aaa...");
    CHECK_EQ(S("aaa...bbb").trimr("ba"), "aaa...");
    CHECK_EQ(S("aaa...bbb").trimr("ab."), "");
    CHECK_EQ(S("aaa...bbb").trimr("a."), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trimr(".a"), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trimr("b."), "aaa");
    CHECK_EQ(S("aaa...bbb").trimr(".b"), "aaa");
    CHECK_EQ(S("aaa...bbb").trimr("cd"), "aaa...bbb");

    CHECK_EQ(S("ab"   ).trimr('a' ), "ab");
    CHECK_EQ(S("ab"   ).trimr('b' ), "a");
    CHECK_EQ(S("ab"   ).trimr('c' ), "ab");
    CHECK_EQ(S("ab"   ).trimr("ab"), "");
    CHECK_EQ(S("ab"   ).trimr("ba"), "");
    CHECK_EQ(S("ab"   ).trimr("cd"), "ab");
    CHECK_EQ(S("a...b").trimr('a' ), "a...b");
    CHECK_EQ(S("a...b").trimr('b' ), "a...");
    CHECK_EQ(S("a...b").trimr('c' ), "a...b");
    CHECK_EQ(S("a...b").trimr("ab"), "a...");
    CHECK_EQ(S("a...b").trimr("ba"), "a...");
    CHECK_EQ(S("a...b").trimr("ab."), "");
    CHECK_EQ(S("a...b").trimr("a."), "a...b");
    CHECK_EQ(S("a...b").trimr(".a"), "a...b");
    CHECK_EQ(S("a...b").trimr("b."), "a");
    CHECK_EQ(S("a...b").trimr(".b"), "a");
    CHECK_EQ(S("a...b").trimr("cd"), "a...b");
}

TEST_CASE("substr.trim")
{
    using S = csubstr;

    CHECK_EQ(S("aaabbb"   ).trim('a' ), "bbb");
    CHECK_EQ(S("aaabbb"   ).trim('b' ), "aaa");
    CHECK_EQ(S("aaabbb"   ).trim('c' ), "aaabbb");
    CHECK_EQ(S("aaabbb"   ).trim("ab"), "");
    CHECK_EQ(S("aaabbb"   ).trim("ba"), "");
    CHECK_EQ(S("aaabbb"   ).trim("cd"), "aaabbb");
    CHECK_EQ(S("aaa...bbb").trim('a' ), "...bbb");
    CHECK_EQ(S("aaa...bbb").trim('b' ), "aaa...");
    CHECK_EQ(S("aaa...bbb").trim('c' ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trim("ab"), "...");
    CHECK_EQ(S("aaa...bbb").trim("ba"), "...");
    CHECK_EQ(S("aaa...bbb").trim('c' ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trim("ab."), "");
    CHECK_EQ(S("aaa...bbb").trim("." ), "aaa...bbb");
    CHECK_EQ(S("aaa...bbb").trim("a."), "bbb");
    CHECK_EQ(S("aaa...bbb").trim(".a"), "bbb");
    CHECK_EQ(S("aaa...bbb").trim("b."), "aaa");
    CHECK_EQ(S("aaa...bbb").trim(".b"), "aaa");
    CHECK_EQ(S("aaa...bbb").trim("cd"), "aaa...bbb");

    CHECK_EQ(S("ab"   ).trim('a' ), "b");
    CHECK_EQ(S("ab"   ).trim('b' ), "a");
    CHECK_EQ(S("ab"   ).trim('c' ), "ab");
    CHECK_EQ(S("ab"   ).trim("ab"), "");
    CHECK_EQ(S("ab"   ).trim("ba"), "");
    CHECK_EQ(S("ab"   ).trim("cd"), "ab");
    CHECK_EQ(S("a...b").trim('a' ), "...b");
    CHECK_EQ(S("a...b").trim('b' ), "a...");
    CHECK_EQ(S("a...b").trim('c' ), "a...b");
    CHECK_EQ(S("a...b").trim("ab"), "...");
    CHECK_EQ(S("a...b").trim("ba"), "...");
    CHECK_EQ(S("a...b").trim('c' ), "a...b");
    CHECK_EQ(S("a...b").trim("ab."), "");
    CHECK_EQ(S("a...b").trim("." ), "a...b");
    CHECK_EQ(S("a...b").trim("a."), "b");
    CHECK_EQ(S("a...b").trim(".a"), "b");
    CHECK_EQ(S("a...b").trim("b."), "a");
    CHECK_EQ(S("a...b").trim(".b"), "a");
    CHECK_EQ(S("a...b").trim("cd"), "a...b");
}

TEST_CASE("substr.pop_right")
{
    using S = csubstr;

    CHECK_EQ(S("0/1/2"    ).pop_right('/'      ), "2");
    CHECK_EQ(S("0/1/2"    ).pop_right('/', true), "2");
    CHECK_EQ(S("0/1/2/"   ).pop_right('/'      ), "");
    CHECK_EQ(S("0/1/2/"   ).pop_right('/', true), "2/");
    CHECK_EQ(S("0/1/2///" ).pop_right('/'      ), "");
    CHECK_EQ(S("0/1/2///" ).pop_right('/', true), "2///");

    CHECK_EQ(S("0/1//2"    ).pop_right('/'      ), "2");
    CHECK_EQ(S("0/1//2"    ).pop_right('/', true), "2");
    CHECK_EQ(S("0/1//2/"   ).pop_right('/'      ), "");
    CHECK_EQ(S("0/1//2/"   ).pop_right('/', true), "2/");
    CHECK_EQ(S("0/1//2///" ).pop_right('/'      ), "");
    CHECK_EQ(S("0/1//2///" ).pop_right('/', true), "2///");

    CHECK_EQ(S("0/1///2"    ).pop_right('/'      ), "2");
    CHECK_EQ(S("0/1///2"    ).pop_right('/', true), "2");
    CHECK_EQ(S("0/1///2/"   ).pop_right('/'      ), "");
    CHECK_EQ(S("0/1///2/"   ).pop_right('/', true), "2/");
    CHECK_EQ(S("0/1///2///" ).pop_right('/'      ), "");
    CHECK_EQ(S("0/1///2///" ).pop_right('/', true), "2///");

    CHECK_EQ(S("/0/1/2"   ).pop_right('/'      ), "2");
    CHECK_EQ(S("/0/1/2"   ).pop_right('/', true), "2");
    CHECK_EQ(S("/0/1/2/"  ).pop_right('/'      ), "");
    CHECK_EQ(S("/0/1/2/"  ).pop_right('/', true), "2/");
    CHECK_EQ(S("/0/1/2///").pop_right('/'      ), "");
    CHECK_EQ(S("/0/1/2///").pop_right('/', true), "2///");

    CHECK_EQ(S("0"        ).pop_right('/'      ), "0");
    CHECK_EQ(S("0"        ).pop_right('/', true), "0");
    CHECK_EQ(S("0/"       ).pop_right('/'      ), "");
    CHECK_EQ(S("0/"       ).pop_right('/', true), "0/");
    CHECK_EQ(S("0///"     ).pop_right('/'      ), "");
    CHECK_EQ(S("0///"     ).pop_right('/', true), "0///");

    CHECK_EQ(S("/0"       ).pop_right('/'      ), "0");
    CHECK_EQ(S("/0"       ).pop_right('/', true), "0");
    CHECK_EQ(S("/0/"      ).pop_right('/'      ), "");
    CHECK_EQ(S("/0/"      ).pop_right('/', true), "0/");
    CHECK_EQ(S("/0///"    ).pop_right('/'      ), "");
    CHECK_EQ(S("/0///"    ).pop_right('/', true), "0///");

    CHECK_EQ(S("/"        ).pop_right('/'      ), "");
    CHECK_EQ(S("/"        ).pop_right('/', true), "");
    CHECK_EQ(S("///"      ).pop_right('/'      ), "");
    CHECK_EQ(S("///"      ).pop_right('/', true), "");

    CHECK_EQ(S(""         ).pop_right('/'      ), "");
    CHECK_EQ(S(""         ).pop_right('/', true), "");

    CHECK_EQ(S("0-1-2"    ).pop_right('-'      ), "2");
    CHECK_EQ(S("0-1-2"    ).pop_right('-', true), "2");
    CHECK_EQ(S("0-1-2-"   ).pop_right('-'      ), "");
    CHECK_EQ(S("0-1-2-"   ).pop_right('-', true), "2-");
    CHECK_EQ(S("0-1-2---" ).pop_right('-'      ), "");
    CHECK_EQ(S("0-1-2---" ).pop_right('-', true), "2---");

    CHECK_EQ(S("0-1--2"    ).pop_right('-'      ), "2");
    CHECK_EQ(S("0-1--2"    ).pop_right('-', true), "2");
    CHECK_EQ(S("0-1--2-"   ).pop_right('-'      ), "");
    CHECK_EQ(S("0-1--2-"   ).pop_right('-', true), "2-");
    CHECK_EQ(S("0-1--2---" ).pop_right('-'      ), "");
    CHECK_EQ(S("0-1--2---" ).pop_right('-', true), "2---");

    CHECK_EQ(S("0-1---2"    ).pop_right('-'      ), "2");
    CHECK_EQ(S("0-1---2"    ).pop_right('-', true), "2");
    CHECK_EQ(S("0-1---2-"   ).pop_right('-'      ), "");
    CHECK_EQ(S("0-1---2-"   ).pop_right('-', true), "2-");
    CHECK_EQ(S("0-1---2---" ).pop_right('-'      ), "");
    CHECK_EQ(S("0-1---2---" ).pop_right('-', true), "2---");

    CHECK_EQ(S("-0-1-2"   ).pop_right('-'      ), "2");
    CHECK_EQ(S("-0-1-2"   ).pop_right('-', true), "2");
    CHECK_EQ(S("-0-1-2-"  ).pop_right('-'      ), "");
    CHECK_EQ(S("-0-1-2-"  ).pop_right('-', true), "2-");
    CHECK_EQ(S("-0-1-2---").pop_right('-'      ), "");
    CHECK_EQ(S("-0-1-2---").pop_right('-', true), "2---");

    CHECK_EQ(S("0"        ).pop_right('-'      ), "0");
    CHECK_EQ(S("0"        ).pop_right('-', true), "0");
    CHECK_EQ(S("0-"       ).pop_right('-'      ), "");
    CHECK_EQ(S("0-"       ).pop_right('-', true), "0-");
    CHECK_EQ(S("0---"     ).pop_right('-'      ), "");
    CHECK_EQ(S("0---"     ).pop_right('-', true), "0---");

    CHECK_EQ(S("-0"       ).pop_right('-'      ), "0");
    CHECK_EQ(S("-0"       ).pop_right('-', true), "0");
    CHECK_EQ(S("-0-"      ).pop_right('-'      ), "");
    CHECK_EQ(S("-0-"      ).pop_right('-', true), "0-");
    CHECK_EQ(S("-0---"    ).pop_right('-'      ), "");
    CHECK_EQ(S("-0---"    ).pop_right('-', true), "0---");

    CHECK_EQ(S("-"        ).pop_right('-'      ), "");
    CHECK_EQ(S("-"        ).pop_right('-', true), "");
    CHECK_EQ(S("---"      ).pop_right('-'      ), "");
    CHECK_EQ(S("---"      ).pop_right('-', true), "");

    CHECK_EQ(S(""         ).pop_right('-'      ), "");
    CHECK_EQ(S(""         ).pop_right('-', true), "");
}

TEST_CASE("substr.pop_left")
{
    using S = csubstr;

    CHECK_EQ(S("0/1/2"    ).pop_left('/'      ), "0");
    CHECK_EQ(S("0/1/2"    ).pop_left('/', true), "0");
    CHECK_EQ(S("0/1/2/"   ).pop_left('/'      ), "0");
    CHECK_EQ(S("0/1/2/"   ).pop_left('/', true), "0");
    CHECK_EQ(S("0/1/2///" ).pop_left('/'      ), "0");
    CHECK_EQ(S("0/1/2///" ).pop_left('/', true), "0");

    CHECK_EQ(S("0//1/2"    ).pop_left('/'      ), "0");
    CHECK_EQ(S("0//1/2"    ).pop_left('/', true), "0");
    CHECK_EQ(S("0//1/2/"   ).pop_left('/'      ), "0");
    CHECK_EQ(S("0//1/2/"   ).pop_left('/', true), "0");
    CHECK_EQ(S("0//1/2///" ).pop_left('/'      ), "0");
    CHECK_EQ(S("0//1/2///" ).pop_left('/', true), "0");

    CHECK_EQ(S("0///1/2"    ).pop_left('/'      ), "0");
    CHECK_EQ(S("0///1/2"    ).pop_left('/', true), "0");
    CHECK_EQ(S("0///1/2/"   ).pop_left('/'      ), "0");
    CHECK_EQ(S("0///1/2/"   ).pop_left('/', true), "0");
    CHECK_EQ(S("0///1/2///" ).pop_left('/'      ), "0");
    CHECK_EQ(S("0///1/2///" ).pop_left('/', true), "0");

    CHECK_EQ(S("/0/1/2"   ).pop_left('/'      ), "");
    CHECK_EQ(S("/0/1/2"   ).pop_left('/', true), "/0");
    CHECK_EQ(S("/0/1/2/"  ).pop_left('/'      ), "");
    CHECK_EQ(S("/0/1/2/"  ).pop_left('/', true), "/0");
    CHECK_EQ(S("/0/1/2///").pop_left('/'      ), "");
    CHECK_EQ(S("/0/1/2///").pop_left('/', true), "/0");
    CHECK_EQ(S("///0/1/2" ).pop_left('/'      ), "");
    CHECK_EQ(S("///0/1/2" ).pop_left('/', true), "///0");
    CHECK_EQ(S("///0/1/2/").pop_left('/'      ), "");
    CHECK_EQ(S("///0/1/2/").pop_left('/', true), "///0");
    CHECK_EQ(S("///0/1/2/").pop_left('/'      ), "");
    CHECK_EQ(S("///0/1/2/").pop_left('/', true), "///0");

    CHECK_EQ(S("0"        ).pop_left('/'      ), "0");
    CHECK_EQ(S("0"        ).pop_left('/', true), "0");
    CHECK_EQ(S("0/"       ).pop_left('/'      ), "0");
    CHECK_EQ(S("0/"       ).pop_left('/', true), "0");
    CHECK_EQ(S("0///"     ).pop_left('/'      ), "0");
    CHECK_EQ(S("0///"     ).pop_left('/', true), "0");

    CHECK_EQ(S("/0"       ).pop_left('/'      ), "");
    CHECK_EQ(S("/0"       ).pop_left('/', true), "/0");
    CHECK_EQ(S("/0/"      ).pop_left('/'      ), "");
    CHECK_EQ(S("/0/"      ).pop_left('/', true), "/0");
    CHECK_EQ(S("/0///"    ).pop_left('/'      ), "");
    CHECK_EQ(S("/0///"    ).pop_left('/', true), "/0");
    CHECK_EQ(S("///0///"  ).pop_left('/'      ), "");
    CHECK_EQ(S("///0///"  ).pop_left('/', true), "///0");

    CHECK_EQ(S("/"        ).pop_left('/'      ), "");
    CHECK_EQ(S("/"        ).pop_left('/', true), "");
    CHECK_EQ(S("///"      ).pop_left('/'      ), "");
    CHECK_EQ(S("///"      ).pop_left('/', true), "");

    CHECK_EQ(S(""         ).pop_left('/'      ), "");
    CHECK_EQ(S(""         ).pop_left('/', true), "");

    CHECK_EQ(S("0-1-2"    ).pop_left('-'      ), "0");
    CHECK_EQ(S("0-1-2"    ).pop_left('-', true), "0");
    CHECK_EQ(S("0-1-2-"   ).pop_left('-'      ), "0");
    CHECK_EQ(S("0-1-2-"   ).pop_left('-', true), "0");
    CHECK_EQ(S("0-1-2---" ).pop_left('-'      ), "0");
    CHECK_EQ(S("0-1-2---" ).pop_left('-', true), "0");

    CHECK_EQ(S("0--1-2"    ).pop_left('-'      ), "0");
    CHECK_EQ(S("0--1-2"    ).pop_left('-', true), "0");
    CHECK_EQ(S("0--1-2-"   ).pop_left('-'      ), "0");
    CHECK_EQ(S("0--1-2-"   ).pop_left('-', true), "0");
    CHECK_EQ(S("0--1-2---" ).pop_left('-'      ), "0");
    CHECK_EQ(S("0--1-2---" ).pop_left('-', true), "0");

    CHECK_EQ(S("0---1-2"    ).pop_left('-'      ), "0");
    CHECK_EQ(S("0---1-2"    ).pop_left('-', true), "0");
    CHECK_EQ(S("0---1-2-"   ).pop_left('-'      ), "0");
    CHECK_EQ(S("0---1-2-"   ).pop_left('-', true), "0");
    CHECK_EQ(S("0---1-2---" ).pop_left('-'      ), "0");
    CHECK_EQ(S("0---1-2---" ).pop_left('-', true), "0");

    CHECK_EQ(S("-0-1-2"   ).pop_left('-'      ), "");
    CHECK_EQ(S("-0-1-2"   ).pop_left('-', true), "-0");
    CHECK_EQ(S("-0-1-2-"  ).pop_left('-'      ), "");
    CHECK_EQ(S("-0-1-2-"  ).pop_left('-', true), "-0");
    CHECK_EQ(S("-0-1-2---").pop_left('-'      ), "");
    CHECK_EQ(S("-0-1-2---").pop_left('-', true), "-0");
    CHECK_EQ(S("---0-1-2" ).pop_left('-'      ), "");
    CHECK_EQ(S("---0-1-2" ).pop_left('-', true), "---0");
    CHECK_EQ(S("---0-1-2-").pop_left('-'      ), "");
    CHECK_EQ(S("---0-1-2-").pop_left('-', true), "---0");
    CHECK_EQ(S("---0-1-2-").pop_left('-'      ), "");
    CHECK_EQ(S("---0-1-2-").pop_left('-', true), "---0");

    CHECK_EQ(S("0"        ).pop_left('-'      ), "0");
    CHECK_EQ(S("0"        ).pop_left('-', true), "0");
    CHECK_EQ(S("0-"       ).pop_left('-'      ), "0");
    CHECK_EQ(S("0-"       ).pop_left('-', true), "0");
    CHECK_EQ(S("0---"     ).pop_left('-'      ), "0");
    CHECK_EQ(S("0---"     ).pop_left('-', true), "0");

    CHECK_EQ(S("-0"       ).pop_left('-'      ), "");
    CHECK_EQ(S("-0"       ).pop_left('-', true), "-0");
    CHECK_EQ(S("-0-"      ).pop_left('-'      ), "");
    CHECK_EQ(S("-0-"      ).pop_left('-', true), "-0");
    CHECK_EQ(S("-0---"    ).pop_left('-'      ), "");
    CHECK_EQ(S("-0---"    ).pop_left('-', true), "-0");
    CHECK_EQ(S("---0---"  ).pop_left('-'      ), "");
    CHECK_EQ(S("---0---"  ).pop_left('-', true), "---0");

    CHECK_EQ(S("-"        ).pop_left('-'      ), "");
    CHECK_EQ(S("-"        ).pop_left('-', true), "");
    CHECK_EQ(S("---"      ).pop_left('-'      ), "");
    CHECK_EQ(S("---"      ).pop_left('-', true), "");

    CHECK_EQ(S(""         ).pop_left('-'      ), "");
    CHECK_EQ(S(""         ).pop_left('-', true), "");
}

TEST_CASE("substr.gpop_left")
{
    using S = csubstr;

    CHECK_EQ(S("0/1/2"      ).gpop_left('/'      ), "0/1");
    CHECK_EQ(S("0/1/2"      ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1/2/"     ).gpop_left('/'      ), "0/1/2");
    CHECK_EQ(S("0/1/2/"     ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1/2//"    ).gpop_left('/'      ), "0/1/2/");
    CHECK_EQ(S("0/1/2//"    ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1/2///"   ).gpop_left('/'      ), "0/1/2//");
    CHECK_EQ(S("0/1/2///"   ).gpop_left('/', true), "0/1");

    CHECK_EQ(S("0/1//2"     ).gpop_left('/'      ), "0/1/");
    CHECK_EQ(S("0/1//2"     ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1//2/"    ).gpop_left('/'      ), "0/1//2");
    CHECK_EQ(S("0/1//2/"    ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1//2//"   ).gpop_left('/'      ), "0/1//2/");
    CHECK_EQ(S("0/1//2//"   ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1//2///"  ).gpop_left('/'      ), "0/1//2//");
    CHECK_EQ(S("0/1//2///"  ).gpop_left('/', true), "0/1");

    CHECK_EQ(S("0/1///2"    ).gpop_left('/'      ), "0/1//");
    CHECK_EQ(S("0/1///2"    ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1///2/"   ).gpop_left('/'      ), "0/1///2");
    CHECK_EQ(S("0/1///2/"   ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1///2//"  ).gpop_left('/'      ), "0/1///2/");
    CHECK_EQ(S("0/1///2//"  ).gpop_left('/', true), "0/1");
    CHECK_EQ(S("0/1///2///" ).gpop_left('/'      ), "0/1///2//");
    CHECK_EQ(S("0/1///2///" ).gpop_left('/', true), "0/1");

    CHECK_EQ(S("/0/1/2"     ).gpop_left('/'      ), "/0/1");
    CHECK_EQ(S("/0/1/2"     ).gpop_left('/', true), "/0/1");
    CHECK_EQ(S("/0/1/2/"    ).gpop_left('/'      ), "/0/1/2");
    CHECK_EQ(S("/0/1/2/"    ).gpop_left('/', true), "/0/1");
    CHECK_EQ(S("/0/1/2//"   ).gpop_left('/'      ), "/0/1/2/");
    CHECK_EQ(S("/0/1/2//"   ).gpop_left('/', true), "/0/1");
    CHECK_EQ(S("/0/1/2///"  ).gpop_left('/'      ), "/0/1/2//");
    CHECK_EQ(S("/0/1/2///"  ).gpop_left('/', true), "/0/1");

    CHECK_EQ(S("//0/1/2"    ).gpop_left('/'      ), "//0/1");
    CHECK_EQ(S("//0/1/2"    ).gpop_left('/', true), "//0/1");
    CHECK_EQ(S("//0/1/2/"   ).gpop_left('/'      ), "//0/1/2");
    CHECK_EQ(S("//0/1/2/"   ).gpop_left('/', true), "//0/1");
    CHECK_EQ(S("//0/1/2//"  ).gpop_left('/'      ), "//0/1/2/");
    CHECK_EQ(S("//0/1/2//"  ).gpop_left('/', true), "//0/1");
    CHECK_EQ(S("//0/1/2///" ).gpop_left('/'      ), "//0/1/2//");
    CHECK_EQ(S("//0/1/2///" ).gpop_left('/', true), "//0/1");

    CHECK_EQ(S("///0/1/2"   ).gpop_left('/'      ), "///0/1");
    CHECK_EQ(S("///0/1/2"   ).gpop_left('/', true), "///0/1");
    CHECK_EQ(S("///0/1/2/"  ).gpop_left('/'      ), "///0/1/2");
    CHECK_EQ(S("///0/1/2/"  ).gpop_left('/', true), "///0/1");
    CHECK_EQ(S("///0/1/2//" ).gpop_left('/'      ), "///0/1/2/");
    CHECK_EQ(S("///0/1/2//" ).gpop_left('/', true), "///0/1");
    CHECK_EQ(S("///0/1/2///").gpop_left('/'      ), "///0/1/2//");
    CHECK_EQ(S("///0/1/2///").gpop_left('/', true), "///0/1");


    CHECK_EQ(S("0/1"      ).gpop_left('/'      ), "0");
    CHECK_EQ(S("0/1"      ).gpop_left('/', true), "0");
    CHECK_EQ(S("0/1/"     ).gpop_left('/'      ), "0/1");
    CHECK_EQ(S("0/1/"     ).gpop_left('/', true), "0");
    CHECK_EQ(S("0/1//"    ).gpop_left('/'      ), "0/1/");
    CHECK_EQ(S("0/1//"    ).gpop_left('/', true), "0");
    CHECK_EQ(S("0/1///"   ).gpop_left('/'      ), "0/1//");
    CHECK_EQ(S("0/1///"   ).gpop_left('/', true), "0");

    CHECK_EQ(S("0//1"     ).gpop_left('/'      ), "0/");
    CHECK_EQ(S("0//1"     ).gpop_left('/', true), "0");
    CHECK_EQ(S("0//1/"    ).gpop_left('/'      ), "0//1");
    CHECK_EQ(S("0//1/"    ).gpop_left('/', true), "0");
    CHECK_EQ(S("0//1//"   ).gpop_left('/'      ), "0//1/");
    CHECK_EQ(S("0//1//"   ).gpop_left('/', true), "0");
    CHECK_EQ(S("0//1///"  ).gpop_left('/'      ), "0//1//");
    CHECK_EQ(S("0//1///"  ).gpop_left('/', true), "0");

    CHECK_EQ(S("0///1"    ).gpop_left('/'      ), "0//");
    CHECK_EQ(S("0///1"    ).gpop_left('/', true), "0");
    CHECK_EQ(S("0///1/"   ).gpop_left('/'      ), "0///1");
    CHECK_EQ(S("0///1/"   ).gpop_left('/', true), "0");
    CHECK_EQ(S("0///1//"  ).gpop_left('/'      ), "0///1/");
    CHECK_EQ(S("0///1//"  ).gpop_left('/', true), "0");
    CHECK_EQ(S("0///1///" ).gpop_left('/'      ), "0///1//");
    CHECK_EQ(S("0///1///" ).gpop_left('/', true), "0");

    CHECK_EQ(S("/0/1"      ).gpop_left('/'      ), "/0");
    CHECK_EQ(S("/0/1"      ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0/1/"     ).gpop_left('/'      ), "/0/1");
    CHECK_EQ(S("/0/1/"     ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0/1//"    ).gpop_left('/'      ), "/0/1/");
    CHECK_EQ(S("/0/1//"    ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0/1///"   ).gpop_left('/'      ), "/0/1//");
    CHECK_EQ(S("/0/1///"   ).gpop_left('/', true), "/0");

    CHECK_EQ(S("/0//1"     ).gpop_left('/'      ), "/0/");
    CHECK_EQ(S("/0//1"     ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0//1/"    ).gpop_left('/'      ), "/0//1");
    CHECK_EQ(S("/0//1/"    ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0//1//"   ).gpop_left('/'      ), "/0//1/");
    CHECK_EQ(S("/0//1//"   ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0//1///"  ).gpop_left('/'      ), "/0//1//");
    CHECK_EQ(S("/0//1///"  ).gpop_left('/', true), "/0");

    CHECK_EQ(S("/0///1"    ).gpop_left('/'      ), "/0//");
    CHECK_EQ(S("/0///1"    ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0///1/"   ).gpop_left('/'      ), "/0///1");
    CHECK_EQ(S("/0///1/"   ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0///1//"  ).gpop_left('/'      ), "/0///1/");
    CHECK_EQ(S("/0///1//"  ).gpop_left('/', true), "/0");
    CHECK_EQ(S("/0///1///" ).gpop_left('/'      ), "/0///1//");
    CHECK_EQ(S("/0///1///" ).gpop_left('/', true), "/0");

    CHECK_EQ(S("//0/1"      ).gpop_left('/'      ), "//0");
    CHECK_EQ(S("//0/1"      ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0/1/"     ).gpop_left('/'      ), "//0/1");
    CHECK_EQ(S("//0/1/"     ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0/1//"    ).gpop_left('/'      ), "//0/1/");
    CHECK_EQ(S("//0/1//"    ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0/1///"   ).gpop_left('/'      ), "//0/1//");
    CHECK_EQ(S("//0/1///"   ).gpop_left('/', true), "//0");

    CHECK_EQ(S("//0//1"     ).gpop_left('/'      ), "//0/");
    CHECK_EQ(S("//0//1"     ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0//1/"    ).gpop_left('/'      ), "//0//1");
    CHECK_EQ(S("//0//1/"    ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0//1//"   ).gpop_left('/'      ), "//0//1/");
    CHECK_EQ(S("//0//1//"   ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0//1///"  ).gpop_left('/'      ), "//0//1//");
    CHECK_EQ(S("//0//1///"  ).gpop_left('/', true), "//0");

    CHECK_EQ(S("//0///1"    ).gpop_left('/'      ), "//0//");
    CHECK_EQ(S("//0///1"    ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0///1/"   ).gpop_left('/'      ), "//0///1");
    CHECK_EQ(S("//0///1/"   ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0///1//"  ).gpop_left('/'      ), "//0///1/");
    CHECK_EQ(S("//0///1//"  ).gpop_left('/', true), "//0");
    CHECK_EQ(S("//0///1///" ).gpop_left('/'      ), "//0///1//");
    CHECK_EQ(S("//0///1///" ).gpop_left('/', true), "//0");

    CHECK_EQ(S("0"      ).gpop_left('/'      ), "");
    CHECK_EQ(S("0"      ).gpop_left('/', true), "");
    CHECK_EQ(S("0/"     ).gpop_left('/'      ), "0");
    CHECK_EQ(S("0/"     ).gpop_left('/', true), "");
    CHECK_EQ(S("0//"    ).gpop_left('/'      ), "0/");
    CHECK_EQ(S("0//"    ).gpop_left('/', true), "");
    CHECK_EQ(S("0///"   ).gpop_left('/'      ), "0//");
    CHECK_EQ(S("0///"   ).gpop_left('/', true), "");

    CHECK_EQ(S("/0"      ).gpop_left('/'      ), "");
    CHECK_EQ(S("/0"      ).gpop_left('/', true), "");
    CHECK_EQ(S("/0/"     ).gpop_left('/'      ), "/0");
    CHECK_EQ(S("/0/"     ).gpop_left('/', true), "");
    CHECK_EQ(S("/0//"    ).gpop_left('/'      ), "/0/");
    CHECK_EQ(S("/0//"    ).gpop_left('/', true), "");
    CHECK_EQ(S("/0///"   ).gpop_left('/'      ), "/0//");
    CHECK_EQ(S("/0///"   ).gpop_left('/', true), "");

    CHECK_EQ(S("//0"      ).gpop_left('/'      ), "/");
    CHECK_EQ(S("//0"      ).gpop_left('/', true), "");
    CHECK_EQ(S("//0/"     ).gpop_left('/'      ), "//0");
    CHECK_EQ(S("//0/"     ).gpop_left('/', true), "");
    CHECK_EQ(S("//0//"    ).gpop_left('/'      ), "//0/");
    CHECK_EQ(S("//0//"    ).gpop_left('/', true), "");
    CHECK_EQ(S("//0///"   ).gpop_left('/'      ), "//0//");
    CHECK_EQ(S("//0///"   ).gpop_left('/', true), "");

    CHECK_EQ(S("///0"      ).gpop_left('/'      ), "//");
    CHECK_EQ(S("///0"      ).gpop_left('/', true), "");
    CHECK_EQ(S("///0/"     ).gpop_left('/'      ), "///0");
    CHECK_EQ(S("///0/"     ).gpop_left('/', true), "");
    CHECK_EQ(S("///0//"    ).gpop_left('/'      ), "///0/");
    CHECK_EQ(S("///0//"    ).gpop_left('/', true), "");
    CHECK_EQ(S("///0///"   ).gpop_left('/'      ), "///0//");
    CHECK_EQ(S("///0///"   ).gpop_left('/', true), "");

    CHECK_EQ(S("/"        ).gpop_left('/'      ), "");
    CHECK_EQ(S("/"        ).gpop_left('/', true), "");
    CHECK_EQ(S("//"       ).gpop_left('/'      ), "/");
    CHECK_EQ(S("//"       ).gpop_left('/', true), "");
    CHECK_EQ(S("///"      ).gpop_left('/'      ), "//");
    CHECK_EQ(S("///"      ).gpop_left('/', true), "");

    CHECK_EQ(S(""         ).gpop_left('/'      ), "");
    CHECK_EQ(S(""         ).gpop_left('/', true), "");
}

TEST_CASE("substr.gpop_right")
{
    using S = csubstr;

    CHECK_EQ(S("0/1/2"      ).gpop_right('/'      ), "1/2");
    CHECK_EQ(S("0/1/2"      ).gpop_right('/', true), "1/2");
    CHECK_EQ(S("0/1/2/"     ).gpop_right('/'      ), "1/2/");
    CHECK_EQ(S("0/1/2/"     ).gpop_right('/', true), "1/2/");
    CHECK_EQ(S("0/1/2//"    ).gpop_right('/'      ), "1/2//");
    CHECK_EQ(S("0/1/2//"    ).gpop_right('/', true), "1/2//");
    CHECK_EQ(S("0/1/2///"   ).gpop_right('/'      ), "1/2///");
    CHECK_EQ(S("0/1/2///"   ).gpop_right('/', true), "1/2///");

    CHECK_EQ(S("0//1/2"     ).gpop_right('/'      ), "/1/2");
    CHECK_EQ(S("0//1/2"     ).gpop_right('/', true),  "1/2");
    CHECK_EQ(S("0//1/2/"    ).gpop_right('/'      ), "/1/2/");
    CHECK_EQ(S("0//1/2/"    ).gpop_right('/', true),  "1/2/");
    CHECK_EQ(S("0//1/2//"   ).gpop_right('/'      ), "/1/2//");
    CHECK_EQ(S("0//1/2//"   ).gpop_right('/', true),  "1/2//");
    CHECK_EQ(S("0//1/2///"  ).gpop_right('/'      ), "/1/2///");
    CHECK_EQ(S("0//1/2///"  ).gpop_right('/', true),  "1/2///");

    CHECK_EQ(S("0///1/2"     ).gpop_right('/'      ), "//1/2");
    CHECK_EQ(S("0///1/2"     ).gpop_right('/', true),   "1/2");
    CHECK_EQ(S("0///1/2/"    ).gpop_right('/'      ), "//1/2/");
    CHECK_EQ(S("0///1/2/"    ).gpop_right('/', true),   "1/2/");
    CHECK_EQ(S("0///1/2//"   ).gpop_right('/'      ), "//1/2//");
    CHECK_EQ(S("0///1/2//"   ).gpop_right('/', true),   "1/2//");
    CHECK_EQ(S("0///1/2///"  ).gpop_right('/'      ), "//1/2///");
    CHECK_EQ(S("0///1/2///"  ).gpop_right('/', true),   "1/2///");


    CHECK_EQ(S("/0/1/2"      ).gpop_right('/'      ), "0/1/2");
    CHECK_EQ(S("/0/1/2"      ).gpop_right('/', true),   "1/2");
    CHECK_EQ(S("/0/1/2/"     ).gpop_right('/'      ), "0/1/2/");
    CHECK_EQ(S("/0/1/2/"     ).gpop_right('/', true),   "1/2/");
    CHECK_EQ(S("/0/1/2//"    ).gpop_right('/'      ), "0/1/2//");
    CHECK_EQ(S("/0/1/2//"    ).gpop_right('/', true),   "1/2//");
    CHECK_EQ(S("/0/1/2///"   ).gpop_right('/'      ), "0/1/2///");
    CHECK_EQ(S("/0/1/2///"   ).gpop_right('/', true),   "1/2///");

    CHECK_EQ(S("/0//1/2"     ).gpop_right('/'      ), "0//1/2");
    CHECK_EQ(S("/0//1/2"     ).gpop_right('/', true),    "1/2");
    CHECK_EQ(S("/0//1/2/"    ).gpop_right('/'      ), "0//1/2/");
    CHECK_EQ(S("/0//1/2/"    ).gpop_right('/', true),    "1/2/");
    CHECK_EQ(S("/0//1/2//"   ).gpop_right('/'      ), "0//1/2//");
    CHECK_EQ(S("/0//1/2//"   ).gpop_right('/', true),    "1/2//");
    CHECK_EQ(S("/0//1/2///"  ).gpop_right('/'      ), "0//1/2///");
    CHECK_EQ(S("/0//1/2///"  ).gpop_right('/', true),    "1/2///");

    CHECK_EQ(S("/0///1/2"     ).gpop_right('/'      ), "0///1/2");
    CHECK_EQ(S("/0///1/2"     ).gpop_right('/', true),     "1/2");
    CHECK_EQ(S("/0///1/2/"    ).gpop_right('/'      ), "0///1/2/");
    CHECK_EQ(S("/0///1/2/"    ).gpop_right('/', true),     "1/2/");
    CHECK_EQ(S("/0///1/2//"   ).gpop_right('/'      ), "0///1/2//");
    CHECK_EQ(S("/0///1/2//"   ).gpop_right('/', true),     "1/2//");
    CHECK_EQ(S("/0///1/2///"  ).gpop_right('/'      ), "0///1/2///");
    CHECK_EQ(S("/0///1/2///"  ).gpop_right('/', true),     "1/2///");


    CHECK_EQ(S("//0/1/2"      ).gpop_right('/'      ), "/0/1/2");
    CHECK_EQ(S("//0/1/2"      ).gpop_right('/', true),    "1/2");
    CHECK_EQ(S("//0/1/2/"     ).gpop_right('/'      ), "/0/1/2/");
    CHECK_EQ(S("//0/1/2/"     ).gpop_right('/', true),    "1/2/");
    CHECK_EQ(S("//0/1/2//"    ).gpop_right('/'      ), "/0/1/2//");
    CHECK_EQ(S("//0/1/2//"    ).gpop_right('/', true),    "1/2//");
    CHECK_EQ(S("//0/1/2///"   ).gpop_right('/'      ), "/0/1/2///");
    CHECK_EQ(S("//0/1/2///"   ).gpop_right('/', true),    "1/2///");

    CHECK_EQ(S("//0//1/2"     ).gpop_right('/'      ), "/0//1/2");
    CHECK_EQ(S("//0//1/2"     ).gpop_right('/', true),     "1/2");
    CHECK_EQ(S("//0//1/2/"    ).gpop_right('/'      ), "/0//1/2/");
    CHECK_EQ(S("//0//1/2/"    ).gpop_right('/', true),     "1/2/");
    CHECK_EQ(S("//0//1/2//"   ).gpop_right('/'      ), "/0//1/2//");
    CHECK_EQ(S("//0//1/2//"   ).gpop_right('/', true),     "1/2//");
    CHECK_EQ(S("//0//1/2///"  ).gpop_right('/'      ), "/0//1/2///");
    CHECK_EQ(S("//0//1/2///"  ).gpop_right('/', true),     "1/2///");

    CHECK_EQ(S("//0///1/2"     ).gpop_right('/'      ), "/0///1/2");
    CHECK_EQ(S("//0///1/2"     ).gpop_right('/', true),     "1/2");
    CHECK_EQ(S("//0///1/2/"    ).gpop_right('/'      ), "/0///1/2/");
    CHECK_EQ(S("//0///1/2/"    ).gpop_right('/', true),     "1/2/");
    CHECK_EQ(S("//0///1/2//"   ).gpop_right('/'      ), "/0///1/2//");
    CHECK_EQ(S("//0///1/2//"   ).gpop_right('/', true),     "1/2//");
    CHECK_EQ(S("//0///1/2///"  ).gpop_right('/'      ), "/0///1/2///");
    CHECK_EQ(S("//0///1/2///"  ).gpop_right('/', true),      "1/2///");


    CHECK_EQ(S("0/1"      ).gpop_right('/'      ), "1");
    CHECK_EQ(S("0/1"      ).gpop_right('/', true), "1");
    CHECK_EQ(S("0/1/"     ).gpop_right('/'      ), "1/");
    CHECK_EQ(S("0/1/"     ).gpop_right('/', true), "1/");
    CHECK_EQ(S("0/1//"    ).gpop_right('/'      ), "1//");
    CHECK_EQ(S("0/1//"    ).gpop_right('/', true), "1//");
    CHECK_EQ(S("0/1///"   ).gpop_right('/'      ), "1///");
    CHECK_EQ(S("0/1///"   ).gpop_right('/', true), "1///");

    CHECK_EQ(S("0//1"     ).gpop_right('/'      ), "/1");
    CHECK_EQ(S("0//1"     ).gpop_right('/', true),  "1");
    CHECK_EQ(S("0//1/"    ).gpop_right('/'      ), "/1/");
    CHECK_EQ(S("0//1/"    ).gpop_right('/', true),  "1/");
    CHECK_EQ(S("0//1//"   ).gpop_right('/'      ), "/1//");
    CHECK_EQ(S("0//1//"   ).gpop_right('/', true),  "1//");
    CHECK_EQ(S("0//1///"  ).gpop_right('/'      ), "/1///");
    CHECK_EQ(S("0//1///"  ).gpop_right('/', true),  "1///");

    CHECK_EQ(S("0///1"    ).gpop_right('/'      ), "//1");
    CHECK_EQ(S("0///1"    ).gpop_right('/', true),   "1");
    CHECK_EQ(S("0///1/"   ).gpop_right('/'      ), "//1/");
    CHECK_EQ(S("0///1/"   ).gpop_right('/', true),   "1/");
    CHECK_EQ(S("0///1//"  ).gpop_right('/'      ), "//1//");
    CHECK_EQ(S("0///1//"  ).gpop_right('/', true),   "1//");
    CHECK_EQ(S("0///1///" ).gpop_right('/'      ), "//1///");
    CHECK_EQ(S("0///1///" ).gpop_right('/', true),   "1///");


    CHECK_EQ(S("/0/1"      ).gpop_right('/'      ), "0/1");
    CHECK_EQ(S("/0/1"      ).gpop_right('/', true),   "1");
    CHECK_EQ(S("/0/1/"     ).gpop_right('/'      ), "0/1/");
    CHECK_EQ(S("/0/1/"     ).gpop_right('/', true),   "1/");
    CHECK_EQ(S("/0/1//"    ).gpop_right('/'      ), "0/1//");
    CHECK_EQ(S("/0/1//"    ).gpop_right('/', true),   "1//");
    CHECK_EQ(S("/0/1///"   ).gpop_right('/'      ), "0/1///");
    CHECK_EQ(S("/0/1///"   ).gpop_right('/', true),   "1///");

    CHECK_EQ(S("/0//1"     ).gpop_right('/'      ), "0//1");
    CHECK_EQ(S("/0//1"     ).gpop_right('/', true),    "1");
    CHECK_EQ(S("/0//1/"    ).gpop_right('/'      ), "0//1/");
    CHECK_EQ(S("/0//1/"    ).gpop_right('/', true),    "1/");
    CHECK_EQ(S("/0//1//"   ).gpop_right('/'      ), "0//1//");
    CHECK_EQ(S("/0//1//"   ).gpop_right('/', true),    "1//");
    CHECK_EQ(S("/0//1///"  ).gpop_right('/'      ), "0//1///");
    CHECK_EQ(S("/0//1///"  ).gpop_right('/', true),    "1///");

    CHECK_EQ(S("/0///1"    ).gpop_right('/'      ), "0///1");
    CHECK_EQ(S("/0///1"    ).gpop_right('/', true),     "1");
    CHECK_EQ(S("/0///1/"   ).gpop_right('/'      ), "0///1/");
    CHECK_EQ(S("/0///1/"   ).gpop_right('/', true),     "1/");
    CHECK_EQ(S("/0///1//"  ).gpop_right('/'      ), "0///1//");
    CHECK_EQ(S("/0///1//"  ).gpop_right('/', true),     "1//");
    CHECK_EQ(S("/0///1///" ).gpop_right('/'      ), "0///1///");
    CHECK_EQ(S("/0///1///" ).gpop_right('/', true),     "1///");


    CHECK_EQ(S("//0/1"      ).gpop_right('/'      ), "/0/1");
    CHECK_EQ(S("//0/1"      ).gpop_right('/', true),    "1");
    CHECK_EQ(S("//0/1/"     ).gpop_right('/'      ), "/0/1/");
    CHECK_EQ(S("//0/1/"     ).gpop_right('/', true),    "1/");
    CHECK_EQ(S("//0/1//"    ).gpop_right('/'      ), "/0/1//");
    CHECK_EQ(S("//0/1//"    ).gpop_right('/', true),    "1//");
    CHECK_EQ(S("//0/1///"   ).gpop_right('/'      ), "/0/1///");
    CHECK_EQ(S("//0/1///"   ).gpop_right('/', true),    "1///");

    CHECK_EQ(S("//0//1"     ).gpop_right('/'      ), "/0//1");
    CHECK_EQ(S("//0//1"     ).gpop_right('/', true),     "1");
    CHECK_EQ(S("//0//1/"    ).gpop_right('/'      ), "/0//1/");
    CHECK_EQ(S("//0//1/"    ).gpop_right('/', true),     "1/");
    CHECK_EQ(S("//0//1//"   ).gpop_right('/'      ), "/0//1//");
    CHECK_EQ(S("//0//1//"   ).gpop_right('/', true),     "1//");
    CHECK_EQ(S("//0//1///"  ).gpop_right('/'      ), "/0//1///");
    CHECK_EQ(S("//0//1///"  ).gpop_right('/', true),     "1///");

    CHECK_EQ(S("//0///1"    ).gpop_right('/'      ), "/0///1");
    CHECK_EQ(S("//0///1"    ).gpop_right('/', true),      "1");
    CHECK_EQ(S("//0///1/"   ).gpop_right('/'      ), "/0///1/");
    CHECK_EQ(S("//0///1/"   ).gpop_right('/', true),      "1/");
    CHECK_EQ(S("//0///1//"  ).gpop_right('/'      ), "/0///1//");
    CHECK_EQ(S("//0///1//"  ).gpop_right('/', true),      "1//");
    CHECK_EQ(S("//0///1///" ).gpop_right('/'      ), "/0///1///");
    CHECK_EQ(S("//0///1///" ).gpop_right('/', true),      "1///");


    CHECK_EQ(S("0"      ).gpop_right('/'      ), "");
    CHECK_EQ(S("0"      ).gpop_right('/', true), "");
    CHECK_EQ(S("0/"     ).gpop_right('/'      ), "");
    CHECK_EQ(S("0/"     ).gpop_right('/', true), "");
    CHECK_EQ(S("0//"    ).gpop_right('/'      ), "/");
    CHECK_EQ(S("0//"    ).gpop_right('/', true), "");
    CHECK_EQ(S("0///"   ).gpop_right('/'      ), "//");
    CHECK_EQ(S("0///"   ).gpop_right('/', true), "");

    CHECK_EQ(S("/0"      ).gpop_right('/'      ), "0");
    CHECK_EQ(S("/0"      ).gpop_right('/', true), "");
    CHECK_EQ(S("/0/"     ).gpop_right('/'      ), "0/");
    CHECK_EQ(S("/0/"     ).gpop_right('/', true), "");
    CHECK_EQ(S("/0//"    ).gpop_right('/'      ), "0//");
    CHECK_EQ(S("/0//"    ).gpop_right('/', true), "");
    CHECK_EQ(S("/0///"   ).gpop_right('/'      ), "0///");
    CHECK_EQ(S("/0///"   ).gpop_right('/', true), "");

    CHECK_EQ(S("//0"      ).gpop_right('/'      ), "/0");
    CHECK_EQ(S("//0"      ).gpop_right('/', true), "");
    CHECK_EQ(S("//0/"     ).gpop_right('/'      ), "/0/");
    CHECK_EQ(S("//0/"     ).gpop_right('/', true), "");
    CHECK_EQ(S("//0//"    ).gpop_right('/'      ), "/0//");
    CHECK_EQ(S("//0//"    ).gpop_right('/', true), "");
    CHECK_EQ(S("//0///"   ).gpop_right('/'      ), "/0///");
    CHECK_EQ(S("//0///"   ).gpop_right('/', true), "");

    CHECK_EQ(S("///0"      ).gpop_right('/'      ), "//0");
    CHECK_EQ(S("///0"      ).gpop_right('/', true), "");
    CHECK_EQ(S("///0/"     ).gpop_right('/'      ), "//0/");
    CHECK_EQ(S("///0/"     ).gpop_right('/', true), "");
    CHECK_EQ(S("///0//"    ).gpop_right('/'      ), "//0//");
    CHECK_EQ(S("///0//"    ).gpop_right('/', true), "");
    CHECK_EQ(S("///0///"   ).gpop_right('/'      ), "//0///");
    CHECK_EQ(S("///0///"   ).gpop_right('/', true), "");

    CHECK_EQ(S("/"        ).gpop_right('/'      ), "");
    CHECK_EQ(S("/"        ).gpop_right('/', true), "");
    CHECK_EQ(S("//"       ).gpop_right('/'      ), "/");
    CHECK_EQ(S("//"       ).gpop_right('/', true), "");
    CHECK_EQ(S("///"      ).gpop_right('/'      ), "//");
    CHECK_EQ(S("///"      ).gpop_right('/', true), "");

    CHECK_EQ(S(""         ).gpop_right('/'      ), "");
    CHECK_EQ(S(""         ).gpop_right('/', true), "");
}

TEST_CASE("substr.basename")
{
    using S = csubstr;
    CHECK_EQ(S("0/1/2").basename(), "2");
    CHECK_EQ(S("0/1/2/").basename(), "2");
    CHECK_EQ(S("0/1/2///").basename(), "2");
    CHECK_EQ(S("/0/1/2").basename(), "2");
    CHECK_EQ(S("/0/1/2/").basename(), "2");
    CHECK_EQ(S("/0/1/2///").basename(), "2");
    CHECK_EQ(S("///0/1/2").basename(), "2");
    CHECK_EQ(S("///0/1/2/").basename(), "2");
    CHECK_EQ(S("///0/1/2///").basename(), "2");
    CHECK_EQ(S("/").basename(), "");
    CHECK_EQ(S("//").basename(), "");
    CHECK_EQ(S("///").basename(), "");
    CHECK_EQ(S("////").basename(), "");
    CHECK_EQ(S("").basename(), "");
}

TEST_CASE("substr.dirname")
{
    using S = csubstr;
    CHECK_EQ(S("0/1/2").dirname(), "0/1/");
    CHECK_EQ(S("0/1/2/").dirname(), "0/1/");
    CHECK_EQ(S("/0/1/2").dirname(), "/0/1/");
    CHECK_EQ(S("/0/1/2/").dirname(), "/0/1/");
    CHECK_EQ(S("///0/1/2").dirname(), "///0/1/");
    CHECK_EQ(S("///0/1/2/").dirname(), "///0/1/");
    CHECK_EQ(S("/0").dirname(), "/");
    CHECK_EQ(S("/").dirname(), "/");
    CHECK_EQ(S("//").dirname(), "//");
    CHECK_EQ(S("///").dirname(), "///");
    CHECK_EQ(S("////").dirname(), "////");
    CHECK_EQ(S("").dirname(), "");
}

TEST_CASE("substr.extshort")
{
    using S = csubstr;
    CHECK_EQ(S("filename.with.ext").extshort(), "ext");
    CHECK_EQ(S("filename.with.ext.").extshort(), "");
    CHECK_EQ(S(".a.b").extshort(), "b");
    CHECK_EQ(S(".a.b.").extshort(), "");
    CHECK_EQ(S(".b..").extshort(), "");
    CHECK_EQ(S("..b.").extshort(), "");
}

TEST_CASE("substr.extlong")
{
    using S = csubstr;
    CHECK_EQ(S("filename.with.ext").extlong(), "with.ext");
    CHECK_EQ(S("filename.with.ext.").extlong(), "with.ext.");
    CHECK_EQ(S(".a.b").extlong(), "a.b");
    CHECK_EQ(S(".a.b.").extlong(), "a.b.");
    CHECK_EQ(S(".b..").extlong(), "b..");
    CHECK_EQ(S("..b.").extlong(), ".b.");
}

TEST_CASE("substr.next_split")
{
    using S = csubstr;

    {
        S const n;
        typename S::size_type pos = 0;
        S ss;
        CHECK_EQ(n.next_split(':', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
        CHECK_EQ(n.next_split(':', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
        pos = 0;
        CHECK_EQ(n.next_split(',', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
        CHECK_EQ(n.next_split(',', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
    }

    {
        S const n("0");
        typename S::size_type pos = 0;
        S ss;
        CHECK_EQ(n.next_split(':', &pos, &ss), true);
        CHECK_EQ(ss.empty(), false);
        CHECK_EQ(n.next_split(':', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
        CHECK_EQ(n.next_split(':', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
        pos = 0;
        CHECK_EQ(n.next_split(',', &pos, &ss), true);
        CHECK_EQ(ss.empty(), false);
        CHECK_EQ(n.next_split(',', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
        CHECK_EQ(n.next_split(',', &pos, &ss), false);
        CHECK_EQ(ss.empty(), true);
    }

    {
        S const n;
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            ++count;
        }
        CHECK_EQ(count, 0);
    }

    {
        S const n("0123456");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), n.size());
                CHECK_EQ(ss.empty(), false);
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 1);
    }

    {
        S const n("0123456:");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), n.size()-1);
                CHECK_EQ(ss.empty(), false);
                break;
            case 1:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 2);
    }

    {
        S const n(":0123456:");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            case 1:
                CHECK_EQ(ss.size(), n.size()-2);
                CHECK_EQ(ss.empty(), false);
                break;
            case 2:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 3);
    }

    {
        S const n(":");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            case 1:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 2);
    }

    {
        S const n("01:23:45:67");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "01");
                CHECK_NE(ss, "01:");
                CHECK_NE(ss, ":01:");
                CHECK_NE(ss, ":01");
                break;
            case 1:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "23");
                CHECK_NE(ss, "23:");
                CHECK_NE(ss, ":23:");
                CHECK_NE(ss, ":23");
                break;
            case 2:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "45");
                CHECK_NE(ss, "45:");
                CHECK_NE(ss, ":45:");
                CHECK_NE(ss, ":45");
                break;
            case 3:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "67");
                CHECK_NE(ss, "67:");
                CHECK_NE(ss, ":67:");
                CHECK_NE(ss, ":67");
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            count++;
        }
        CHECK_EQ(count, 4);
    }

    {
        const S n(":01:23:45:67:");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            case 1:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "01");
                CHECK_NE(ss, "01:");
                CHECK_NE(ss, ":01:");
                CHECK_NE(ss, ":01");
                break;
            case 2:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "23");
                CHECK_NE(ss, "23:");
                CHECK_NE(ss, ":23:");
                CHECK_NE(ss, ":23");
                break;
            case 3:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "45");
                CHECK_NE(ss, "45:");
                CHECK_NE(ss, ":45:");
                CHECK_NE(ss, ":45");
                break;
            case 4:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "67");
                CHECK_NE(ss, "67:");
                CHECK_NE(ss, ":67:");
                CHECK_NE(ss, ":67");
                break;
            case 5:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            count++;
        }
        CHECK_EQ(count, 6);
    }

    {
        const S n("::::01:23:45:67::::");
        typename S::size_type pos = 0;
        typename S::size_type count = 0;
        S ss;
        while(n.next_split(':', &pos, &ss))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 1:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 2:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 3:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 4:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "01");
                CHECK_NE(ss, "01:");
                CHECK_NE(ss, ":01:");
                CHECK_NE(ss, ":01");
                break;
            case 5:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "23");
                CHECK_NE(ss, "23:");
                CHECK_NE(ss, ":23:");
                CHECK_NE(ss, ":23");
                break;
            case 6:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "45");
                CHECK_NE(ss, "45:");
                CHECK_NE(ss, ":45:");
                CHECK_NE(ss, ":45");
                break;
            case 7:
                CHECK_EQ(ss.size(), 2);
                CHECK_EQ(ss, "67");
                CHECK_NE(ss, "67:");
                CHECK_NE(ss, ":67:");
                CHECK_NE(ss, ":67");
                break;
            case 8:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 9:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 10:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 11:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            default:
                CHECK_UNARY(false);//GTEST_FAIL();
                break;
            }
            count++;
        }
        CHECK_EQ(count, 12);
    }
}

TEST_CASE("substr.split")
{
    using S = csubstr;

    {
        S const n;
        {
            auto spl = n.split(':');
            auto beg = spl.begin();
            auto end = spl.end();
            CHECK_UNARY(beg == end);
        }
    }

    {
        S const n("foo:bar:baz");
        auto spl = n.split(':');
        auto beg = spl.begin();
        auto end = spl.end();
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(end->size(), 0);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        auto it = beg;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "foo");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it == beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "bar");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "baz");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 0);
        CHECK_UNARY(it == end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it = beg;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "foo");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it == beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "bar");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "baz");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 0);
        CHECK_UNARY(it == end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
    }

    {
        S const n("foo:bar:baz:");
        auto spl = n.split(':');
        auto beg = spl.begin();
        auto end = spl.end();
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(end->size(), 0);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        auto it = beg;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "foo");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it == beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "bar");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "baz");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 0);
        CHECK_EQ(*it, "");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        ++it;
        CHECK_EQ(it->size(), 0);
        CHECK_UNARY(it == end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        //--------------------------
        it = beg;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "foo");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it == beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "bar");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 3);
        CHECK_EQ(*it, "baz");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 0);
        CHECK_EQ(*it, "");
        CHECK_UNARY(it != end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
        it++;
        CHECK_EQ(it->size(), 0);
        CHECK_UNARY(it == end);
        CHECK_UNARY(it != beg);
        CHECK_EQ(beg->size(), 3);
        CHECK_EQ(*beg, "foo");
        CHECK_UNARY(beg != end);
    }

    {
        S const n;
        auto s = n.split(':');
        // check that multiple calls to begin() always yield the same result
        CHECK_EQ(*s.begin(), "");
        CHECK_EQ(*s.begin(), "");
        CHECK_EQ(*s.begin(), "");
        // check that multiple calls to end() always yield the same result
        auto e = s.end();
        CHECK_UNARY(s.end() == e);
        CHECK_UNARY(s.end() == e);
        //
        auto it = s.begin();
        CHECK_EQ(*it, "");
        CHECK_EQ(it->empty(), true);
        CHECK_EQ(it->size(), 0);
        ++it;
        CHECK_UNARY(it == e);
    }

    {
        S const n("01:23:45:67");
        auto s = n.split(':');
        // check that multiple calls to begin() always yield the same result
        CHECK_EQ(*s.begin(), "01");
        CHECK_EQ(*s.begin(), "01");
        CHECK_EQ(*s.begin(), "01");
        // check that multiple calls to end() always yield the same result
        auto e = s.end();
        CHECK_UNARY(s.end() == e);
        CHECK_UNARY(s.end() == e);
        CHECK_UNARY(s.end() == e);
        //
        auto it = s.begin();
        CHECK_EQ(*it, "01");
        CHECK_EQ(it->size(), 2);
        ++it;
        CHECK_EQ(*it, "23");
        CHECK_EQ(it->size(), 2);
        ++it;
        CHECK_EQ(*it, "45");
        CHECK_EQ(it->size(), 2);
        ++it;
        CHECK_EQ(*it, "67");
        CHECK_EQ(it->size(), 2);
        ++it;
        CHECK_UNARY(it == s.end());
    }

    {
        S const n;
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            ++count;
        }
        CHECK_EQ(count, 0);
    }

    {
        S const n("0123456");
        {
            auto spl = n.split(':');
            auto beg = spl.begin();
            auto end = spl.end();
            CHECK_EQ(beg->size(), n.size());
            CHECK_EQ(end->size(), 0);
        }
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), n.size());
                CHECK_EQ(ss.empty(), false);
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 1);
    }

    {
        S const n("foo:bar");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 3);
                CHECK_EQ(ss.empty(), false);
                CHECK_EQ(ss, "foo");
                break;
            case 1:
                CHECK_EQ(ss.size(), 3);
                CHECK_EQ(ss.empty(), false);
                CHECK_EQ(ss, "bar");
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 2);
    }

    {
        S const n("0123456:");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), n.size()-1);
                CHECK_EQ(ss.empty(), false);
                break;
            case 1:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 2);
    }

    {
        S const n(":0123456:");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            case 1:
                CHECK_EQ(ss.size(), n.size()-2);
                CHECK_EQ(ss.empty(), false);
                break;
            case 2:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 3);
    }

    {
        S const n(":");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            case 1:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            }
            ++count;
        }
        CHECK_EQ(count, 2);
    }

    {
        S const n("01:23:45:67");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss, "01");
                CHECK_NE(ss, "01:");
                CHECK_NE(ss, ":01:");
                CHECK_NE(ss, ":01");
                break;
            case 1:
                CHECK_EQ(ss, "23");
                CHECK_NE(ss, "23:");
                CHECK_NE(ss, ":23:");
                CHECK_NE(ss, ":23");
                break;
            case 2:
                CHECK_EQ(ss, "45");
                CHECK_NE(ss, "45:");
                CHECK_NE(ss, ":45:");
                CHECK_NE(ss, ":45");
                break;
            case 3:
                CHECK_EQ(ss, "67");
                CHECK_NE(ss, "67:");
                CHECK_NE(ss, ":67:");
                CHECK_NE(ss, ":67");
                break;
            }
            count++;
        }
        CHECK_EQ(count, 4);
    }

    {
        const S n(":01:23:45:67:");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            case 1:
                CHECK_EQ(ss, "01");
                CHECK_NE(ss, "01:");
                CHECK_NE(ss, ":01:");
                CHECK_NE(ss, ":01");
                break;
            case 2:
                CHECK_EQ(ss, "23");
                CHECK_NE(ss, "23:");
                CHECK_NE(ss, ":23:");
                CHECK_NE(ss, ":23");
                break;
            case 3:
                CHECK_EQ(ss, "45");
                CHECK_NE(ss, "45:");
                CHECK_NE(ss, ":45:");
                CHECK_NE(ss, ":45");
                break;
            case 4:
                CHECK_EQ(ss, "67");
                CHECK_NE(ss, "67:");
                CHECK_NE(ss, ":67:");
                CHECK_NE(ss, ":67");
                break;
            case 5:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                break;
            }
            count++;
        }
        CHECK_EQ(count, 6);
    }

    {
        const S n("::::01:23:45:67::::");
        typename S::size_type count = 0;
        for(auto &ss : n.split(':'))
        {
            switch(count)
            {
            case 0:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 1:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 2:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 3:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 4:
                CHECK_EQ(ss, "01");
                CHECK_NE(ss, "01:");
                CHECK_NE(ss, ":01:");
                CHECK_NE(ss, ":01");
                break;
            case 5:
                CHECK_EQ(ss, "23");
                CHECK_NE(ss, "23:");
                CHECK_NE(ss, ":23:");
                CHECK_NE(ss, ":23");
                break;
            case 6:
                CHECK_EQ(ss, "45");
                CHECK_NE(ss, "45:");
                CHECK_NE(ss, ":45:");
                CHECK_NE(ss, ":45");
                break;
            case 7:
                CHECK_EQ(ss, "67");
                CHECK_NE(ss, "67:");
                CHECK_NE(ss, ":67:");
                CHECK_NE(ss, ":67");
                break;
            case 8:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 9:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 10:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            case 11:
                CHECK_EQ(ss.size(), 0);
                CHECK_EQ(ss.empty(), true);
                CHECK_NE(ss, "::");
                break;
            }
            count++;
        }
        CHECK_EQ(count, 12);
    }
}


//-----------------------------------------------------------------------------
TEST_CASE("substr.copy_from")
{
    char buf[128] = {0};
    substr s = buf;
    CHECK_EQ(s.size(), sizeof(buf)-1);
    CHECK_NE(s.first(3), "123");
    s.copy_from("123");
    CHECK_EQ(s.first(3), "123");
    CHECK_EQ(s.first(6), "123\0\0\0");
    s.copy_from("+++", 3);
    CHECK_EQ(s.first(6), "123+++");
    CHECK_EQ(s.first(9), "123+++\0\0\0");
    s.copy_from("456", 6);
    CHECK_EQ(s.first(9), "123+++456");
    CHECK_EQ(s.first(12), "123+++456\0\0\0");
    s.copy_from("***", 3);
    CHECK_EQ(s.first(9), "123***456");
    CHECK_EQ(s.first(12), "123***456\0\0\0");

    // make sure that it's safe to pass source strings that don't fit
    // in the remaining destination space
    substr ss = s.first(9);
    ss.copy_from("987654321", 9); // should be a no-op
    CHECK_EQ(s.first(12), "123***456\0\0\0");
    ss.copy_from("987654321", 6);
    CHECK_EQ(s.first(12), "123***987\0\0\0");
    ss.copy_from("987654321", 3);
    CHECK_EQ(s.first(12), "123987654\0\0\0");
    ss.first(3).copy_from("987654321");
    CHECK_EQ(s.first(12), "987987654\0\0\0");
}


//-----------------------------------------------------------------------------
void do_test_reverse(substr s, csubstr orig, csubstr expected)
{
    CHECK_EQ(s, orig);
    s.reverse();
    CHECK_EQ(s, expected);
    s.reverse();
    CHECK_EQ(s, orig);
}

TEST_CASE("substr.reverse")
{
    char buf[] = "0123456789";
    do_test_reverse(buf, "0123456789", "9876543210");
    do_test_reverse(buf, "0123456789", "9876543210");

    // in the middle
    substr s = buf;
    s.sub(2, 2).reverse();
    CHECK_EQ(s, "0132456789");
    s.sub(2, 2).reverse();
    CHECK_EQ(s, "0123456789");

    s.sub(4, 2).reverse();
    CHECK_EQ(s, "0123546789");
    s.sub(4, 2).reverse();
    CHECK_EQ(s, "0123456789");

    // at the beginning
    s.first(3).reverse();
    CHECK_EQ(s, "2103456789");
    s.first(3).reverse();
    CHECK_EQ(s, "0123456789");

    // at the end
    s.last(3).reverse();
    CHECK_EQ(s, "0123456987");
    s.last(3).reverse();
    CHECK_EQ(s, "0123456789");
}


//-----------------------------------------------------------------------------
TEST_CASE("substr.erase")
{
    char buf[] = "0123456789";

    substr s = buf;
    CHECK_EQ(s.len, s.size());
    CHECK_EQ(s.len, 10);
    CHECK_EQ(s, "0123456789");

    substr ss = s.first(6);
    CHECK_EQ(ss.len, 6);
    for(size_t i = 0; i <= ss.len; ++i)
    {
        ss.erase(i, 0); // must be a no-op
        CHECK_EQ(s, "0123456789");
        ss.erase_range(i, i); // must be a no-op
        CHECK_EQ(s, "0123456789");
        ss.erase(ss.len-i, i); // must be a no-op
        CHECK_EQ(s, "0123456789");
    }

    substr r;
    ss = ss.erase(0, 1);
    CHECK_EQ(ss.len, 5);
    CHECK_EQ(ss, "12345");
    CHECK_EQ(s, "1234556789");
    ss = ss.erase(0, 2);
    CHECK_EQ(ss.len, 3);
    CHECK_EQ(ss, "345");
    CHECK_EQ(s, "3454556789");

    csubstr s55 = s.sub(4, 2);
    ss = s.erase(s55);
    CHECK_EQ(s, "3454678989");
}


//-----------------------------------------------------------------------------
TEST_CASE("substr.replace")
{
    char buf[] = "0.1.2.3.4.5.6.7.8.9";

    substr s = buf;

    auto ret = s.replace('+', '.');
    CHECK_EQ(ret, 0);

    ret = s.replace('.', '.', s.len);
    CHECK_EQ(s, "0.1.2.3.4.5.6.7.8.9");
    CHECK_EQ(ret, 0);
    ret = s.replace('.', '.');
    CHECK_EQ(ret, 9);
    CHECK_EQ(s, "0.1.2.3.4.5.6.7.8.9");

    ret = s.replace('.', '+', s.len);
    CHECK_EQ(s, "0.1.2.3.4.5.6.7.8.9");
    CHECK_EQ(ret, 0);
    ret = s.replace('.', '+');
    CHECK_EQ(ret, 9);
    CHECK_EQ(s, "0+1+2+3+4+5+6+7+8+9");

    ret = s.replace("16", '.', s.len);
    CHECK_EQ(s, "0+1+2+3+4+5+6+7+8+9");
    CHECK_EQ(ret, 0);
    ret = s.replace("16", '.');
    CHECK_EQ(ret, 2);
    CHECK_EQ(s, "0+.+2+3+4+5+.+7+8+9");
    ret = s.replace("3+2", '_');
    CHECK_EQ(ret, 11);
    CHECK_EQ(s, "0_._____4_5_._7_8_9");

    // must accept empty string
    ret = s.sub(0, 0).replace('0', '1');
    CHECK_EQ(ret, 0);
    CHECK_EQ(s, "0_._____4_5_._7_8_9");
    ret = s.sub(0, 0).replace("0", '1');
    CHECK_EQ(ret, 0);
    CHECK_EQ(s, "0_._____4_5_._7_8_9");
}

TEST_CASE("substr.replace_all")
{
    char buf[] = "0.1.2.3.4.5.6.7.8.9";
    std::string tmp, out("0+1+2+3+4+5+6+7+8+9");

    // must accept empty string
    substr(buf).sub(0, 0).replace_all(to_substr(tmp), "0", "X");
    CHECK_EQ(csubstr(buf), "0.1.2.3.4.5.6.7.8.9");

    substr r;
    auto replall = [&](csubstr pattern, csubstr repl) -> substr {
                       tmp = out;
                       csubstr rtmp = to_csubstr(tmp);
                       out.resize(128);
                       substr dst = to_substr(out);
                       size_t sz = rtmp.replace_all(dst, pattern, repl);
                       CHECK_LE(sz, out.size());
                       out.resize(sz);
                       return dst.first(sz);
                   };
    r = replall("0+1", "0+++++1");
    // the result must be a view of out
    CHECK_FALSE(r.empty());
    CHECK_FALSE(out.empty());
    CHECK_EQ(r.size(), out.size());
    CHECK_EQ(r.front(), out.front());
    CHECK_EQ(r.back(), out.back());
    CHECK_EQ(r, "0+++++1+2+3+4+5+6+7+8+9");

    r = replall("+", "");
    CHECK_EQ(r, "0123456789");

    r = replall("+", "");
    CHECK_EQ(r, "0123456789"); // must not change

    r = replall("0123456789", "9876543210");
    CHECK_EQ(r, "9876543210");

    r = replall("987", ".");
    CHECK_EQ(r, ".6543210");

    r = replall("210", ".");
    CHECK_EQ(r, ".6543.");

    r = replall("6543", ":");
    CHECK_EQ(r, ".:.");

    r = replall(".:.", "");
    CHECK_EQ(r, "");
}

TEST_CASE("substr.short_integer")
{
    char buf[] = "-";
    CHECK_FALSE(substr(buf).is_integer());
    CHECK_FALSE(csubstr("-").is_integer());
    CHECK_FALSE(csubstr("+").is_integer());
}

} // namespace c4

#include "c4/libtest/supprwarn_pop.hpp"
