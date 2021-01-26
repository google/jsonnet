#include "c4/yml/common.hpp"

#ifndef RYML_NO_DEFAULT_CALLBACKS
#   include <stdlib.h>
#   include <stdio.h>
#endif // RYML_NO_DEFAULT_CALLBACKS

namespace c4 {
namespace yml {

#ifndef RYML_NO_DEFAULT_CALLBACKS

void report_error_impl(const char* msg, size_t length, Location loc, FILE *f)
{
    if(!f)
    {
        f = stderr;
    }
    if(loc)
    {
        if(!loc.name.empty())
        {
            fprintf(f, "%.*s:", (int)loc.name.len, loc.name.str);
        }
        fprintf(f, "%zu:%zu:", loc.line, loc.col);
        if(loc.offset)
        {
            fprintf(f, " (%zuB):", loc.offset);
        }
    }
    fprintf(f, "ERROR: %.*s\n", (int)length, msg);
    fflush(f);
}

namespace {

void error_impl(const char* msg, size_t length, Location loc, void * /*user_data*/)
{
    report_error_impl(msg, length, loc, nullptr);
    ::abort();
}

void* allocate_impl(size_t length, void * /*hint*/, void * /*user_data*/)
{
    void *mem = ::malloc(length);
    if(mem == nullptr)
    {
        const char msg[] = "could not allocate memory";
        error_impl(msg, sizeof(msg)-1, {}, nullptr);
    }
    return mem;
}

void free_impl(void *mem, size_t /*length*/, void * /*user_data*/)
{
    ::free(mem);
}
} // empty namespace


Callbacks::Callbacks()
    :
    m_user_data(nullptr),
    m_allocate(allocate_impl),
    m_free(free_impl),
    m_error(error_impl)
{
}

Callbacks::Callbacks(void *user_data, pfn_allocate alloc_, pfn_free free_, pfn_error error_)
    :
    m_user_data(user_data),
    m_allocate(alloc_ ? alloc_ : allocate_impl),
    m_free(free_ ? free_ : free_impl),
    m_error(error_ ? error_ : error_impl)
{
    C4_CHECK(m_allocate);
    C4_CHECK(m_free);
    C4_CHECK(m_error);
}

#else // RYML_NO_DEFAULT_CALLBACKS

Callbacks::Callbacks()
    :
    m_user_data(nullptr),
    m_allocate(nullptr),
    m_free(nullptr),
    m_error(nullptr)
{
}

Callbacks::Callbacks(void *user_data, pfn_allocate alloc_, pfn_free free_, pfn_error error_)
    :
    m_user_data(user_data),
    m_allocate(alloc_ ? alloc_ : allocate_impl),
    m_free(free_ ? free_ : free_impl),
    m_error(error_ ? error_ : error_impl)
{
    C4_CHECK(m_allocate);
    C4_CHECK(m_free);
    C4_CHECK(m_error);
}

#endif // RYML_NO_DEFAULT_CALLBACKS

namespace {
Callbacks s_default_callbacks;
MemoryResourceCallbacks s_default_memory_resource;
MemoryResource* s_memory_resource = &s_default_memory_resource;
}

void set_callbacks(Callbacks const& c)
{
    s_default_callbacks = c;
}

Callbacks const& get_callbacks()
{
    return s_default_callbacks;
}

MemoryResource* get_memory_resource()
{
    return s_memory_resource;
}

void set_memory_resource(MemoryResource* r)
{
    s_memory_resource = r ? r : &s_default_memory_resource;
}

void error(const char *msg, size_t msg_len, Location loc)
{
    s_default_callbacks.error(msg, msg_len, loc);
}

} // namespace yml
} // namespace c4
