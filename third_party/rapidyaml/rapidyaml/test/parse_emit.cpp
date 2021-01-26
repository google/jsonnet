#include <c4/yml/std/std.hpp>
#include <c4/yml/parse.hpp>
#include <c4/yml/emit.hpp>
#include <c4/fs/fs.hpp>

#include <cstdio>
#include <chrono>
#include <algorithm>


using namespace c4;


//-----------------------------------------------------------------------------

struct timed_section
{
    using myclock = std::chrono::high_resolution_clock;
    using msecs = std::chrono::duration<double, std::milli>;

    csubstr name;
    myclock::time_point start;

    msecs since() const { return myclock::now() - start; }
    timed_section(csubstr n) : name(n), start(myclock::now()) {}
    ~timed_section()
    {
        fprintf(stderr, "%.2lgms: %.*s\n", since().count(), (int)name.len, name.str);
        fflush(stderr);
    }
};

#define TS(name) timed_section name##__##__LINE__(#name)


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

int main(int argc, const char *argv[])
{
    if(argc != 2)
    {
        printf("usage: %s <path/to/file.yaml>\n", argv[0]);
        return 1;
    }

    TS(TOTAL);

    csubstr file = to_csubstr(argv[1]);
    C4_CHECK_MSG(fs::path_exists(file.str), "cannot find file: %s (cwd=%s)", file.str, fs::cwd<std::string>().c_str());

    {
        TS(objects);
        std::string contents, output;
        yml::Tree tree;
        {
            TS(read_file);
            fs::file_get_contents(file.str, &contents);
        }
        {
            TS(tree_reserve);
            size_t nlines;
            {
                TS(count_lines);
                C4_CHECK(contents.begin() <= contents.end());
                nlines = static_cast<size_t>(std::count(contents.begin(), contents.end(), '\n'));
            }
            fprintf(stderr, "reserving #lines=%zu\n", nlines);
            tree.reserve(nlines);
        }
        {
            TS(parse_yml);
            yml::parse(file, to_substr(contents), &tree);
        }
        {
            TS(emit_to_buffer);
            output.resize(contents.size()); // resize, not just reserve
            yml::emitrs(tree, &output);
        }
        {
            TS(print_stdout);
            fwrite(output.data(), 1, output.size(), stdout);
            putchar('\n');
        }
    }

    return 0;
}
