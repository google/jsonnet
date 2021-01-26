#include <ryml_std.hpp>
#include <ryml.hpp>
#include <c4/format.hpp>
#include <stdexcept>

#if C4_CPP < 17
#error "must be C++17"
#endif


template <class... Args>
void err(c4::csubstr fmt, Args const& ...args)
{
    throw std::runtime_error(c4::formatrs<std::string>(fmt, args...));
}


void check(ryml::Tree const& t, c4::csubstr key, c4::csubstr expected)
{
    c4::csubstr actual = t[key].val();
    if(actual != expected)
    {
        err("expected t[{}]='{}', got '{}'", key, expected, actual);
    }
}


int main()
{
    auto tree = ryml::parse("{foo: 0, bar: 1}");
    check(tree, "foo", "0");
    check(tree, "bar", "1");
    return 0;
}
