#ifndef _LIBYAML_HPP_
#define _LIBYAML_HPP_

#include <yaml.h>
#include <c4/yml/std/std.hpp>
#include <c4/yml/yml.hpp>
#include <c4/yml/detail/stack.hpp>

#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace c4 {
namespace yml {


namespace detail {
class Event;
} // detail

class detail::Event
{
public:
    yaml_event_t m_event;
public:
    Event() {}
    ~Event()
    {
        yaml_event_delete(&m_event);
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class LibyamlParser
{
public:

    yaml_parser_t m_parser;
    csubstr       m_input;
    Tree        * m_tree;
    c4::yml::detail::stack<size_t> m_stack;

public:

    LibyamlParser() : m_parser(), m_input(), m_tree(), m_stack()
    {
        memset(&m_parser, 0, sizeof(decltype(m_parser)));
        yaml_parser_initialize(&m_parser);
    }

    ~LibyamlParser()
    {
        yaml_parser_delete(&m_parser);
        memset(&m_parser, 0, sizeof(decltype(m_parser)));
    }

    void parse(Tree *t, const csubstr sp)
    {
        m_input = sp;
        m_tree = t;
        m_tree->clear();
        m_stack.clear();
        yaml_parser_set_input_string(&m_parser, (const unsigned char*)m_input.str, m_input.len);
        _do_parse();
    }

    void _do_parse()
    {
        bool done = false;
        //bool doc_had_scalars = false;
        csubstr prev_scalar;
        while( ! done)
        {
            detail::Event ev;
            if( ! yaml_parser_parse(&m_parser, &ev.m_event))
            {
                _handle_error();
                break;
            }

#if defined(RYML_DBG)
#define _c4_handle_case(_ev)                            \
case YAML_ ## _ev ## _EVENT:                            \
    printf(#_ev " val=%.*s\n",                          \
           /*(int)prev_scalar.len, prev_scalar.str,*/   \
           (int)val.len, val.str);
#else
#define _c4_handle_case(_ev)                            \
case YAML_ ## _ev ## _EVENT:                            \
    (void)val;
#endif

            csubstr val = get_scalar_val(ev);
            switch(ev.m_event.type)
            {
            _c4_handle_case(MAPPING_START)
            {
                /*if(( ! s->stack_top_is_type(DOC) || doc_had_scalars)
                   &&
                   ( ! m_load_root))
                {
                    C4_ASSERT( ! prev_scalar.empty());
                    s->begin_map(prev_scalar, s->top_last());
                    prev_scalar.clear();
                }
                */
                begin_container(MAP, prev_scalar);
                prev_scalar.clear();
                break;
            }
            _c4_handle_case(MAPPING_END)
            {
                end_container();
                break;
            }
            _c4_handle_case(SEQUENCE_START)
            {
                begin_container(SEQ, prev_scalar);
                prev_scalar.clear();
                break;
            }
            _c4_handle_case(SEQUENCE_END)
            {
                end_container();
                break;
            }
            _c4_handle_case(SCALAR)
            {
                size_t parent = m_stack.top();
                if(m_tree->is_seq(parent))
                {
                    C4_ASSERT(prev_scalar.empty());
                    append_val(val);
                }
                else if(m_tree->is_map(parent))
                {
                    if( ! prev_scalar.empty())
                    {
                        append_keyval(prev_scalar, val);
                        prev_scalar.clear();
                    }
                    else
                    {
                        prev_scalar = val;
                    }
                }
                //doc_had_scalars = true;
                break;
            }
            _c4_handle_case(DOCUMENT_START)
            {
                auto r = m_tree->root_id();
                m_tree->to_doc(r);
                //m_stack.push(r);
                break;
            }
            _c4_handle_case(DOCUMENT_END)
            {
                //m_stack.pop();
                break;
            }
            _c4_handle_case(STREAM_START)
            {
                //s->begin_stream();
                break;
            }
            _c4_handle_case(STREAM_END)
            {
                //s->end_stream();
                done = true;
                break;
            }
            _c4_handle_case(ALIAS)
            {
                C4_NOT_IMPLEMENTED();
                break;
            }
            _c4_handle_case(NO)
            {
                break;
            }

#undef _c4_handle_case
            default:
                break;
            };
        }
    }

private:

    void begin_container(NodeType type, csubstr key)
    {
        size_t parent, elm;
        if( ! m_stack.empty())
        {
            parent = m_stack.top();
            elm = m_tree->append_child(parent);
        }
        else
        {
            parent = m_tree->root_id();
            C4_ASSERT(m_tree->is_container(parent));
            if(type.is_map())
            {
                m_tree->to_map(parent);
            }
            else
            {
                m_tree->to_seq(parent);
            }
            elm = parent;
        }
        m_stack.push(elm);
        if(type.is_map())
        {
            if(key.not_empty())
            {
                //printf("append map! key=%.*s elm=%zu parent=%zu\n", (int)key.len, key.str, elm, parent);
                m_tree->to_map(elm, key);
            }
            else
            {
                //printf("append map! elm=%zu parent=%zu\n", elm, parent);
                m_tree->to_map(elm);
            }
        }
        else if(type.is_seq())
        {
            if(key.not_empty())
            {
                //printf("append seq! key=%.*s elm=%zu parent=%zu\n", (int)key.len, key.str, elm, parent);
                m_tree->to_seq(elm, key);
            }
            else
            {
                //printf("append seq! elm=%zu parent=%zu\n", elm, parent);
                m_tree->to_seq(elm);
            }
        }
        else
        {
            C4_ERROR("");
        }
    }

    void end_container()
    {
        if( ! m_stack.empty())
        {
            m_stack.pop();
        }
    }

    void append_val(csubstr val)
    {
        size_t elm = m_tree->append_child(m_stack.top());
        m_tree->to_val(elm, val);
        //printf("append val! %.*s  elm=%zu parent=%zu\n", (int)val.len, val.str, elm, m_tree->parent(elm));
    }

    void append_keyval(csubstr key, csubstr val)
    {
        size_t elm = m_tree->append_child(m_stack.top());
        m_tree->to_keyval(elm, key, val);
        //printf("append keyval! %.*s: %.*s  elm=%zu parent=%zu\n", (int)key.len, key.str, (int)val.len, val.str, elm, m_tree->parent(elm));
    }

    csubstr get_scalar_val(detail::Event const &ev) const
    {
        // the memory in data.scalar is allocated anew, so don't do this
        //auto const& scalar = e.m_event.data.scalar;
        //csubstr val((const char*)scalar.value, scalar.length);
        //return val;
        // ... but the event tells us where in the string the value is
        auto const& e = ev.m_event;
        size_t len = e.end_mark.index - e.start_mark.index;
        csubstr val(m_input.str + e.start_mark.index, len);
        return val;
    }

    void _handle_error()
    {
        Location problem_loc, context_loc;
        if(m_parser.problem)
        {
            auto const& m = m_parser.problem_mark;
            problem_loc = Location(m_parser.problem, m.index, m.line+1, m.column+1);
        }
        if(m_parser.context)
        {
            auto const& m = m_parser.context_mark;
            context_loc = Location(m_parser.context, m.index, m.line+1, m.column+1);
        }

        switch(m_parser.error)
        {

        case YAML_MEMORY_ERROR:
            error("Memory error: Not enough memory for parsing");
            break;

        case YAML_READER_ERROR:
            if (m_parser.problem_value != -1)
            {
                char buf[32];
                int ret = snprintf(buf, sizeof(buf), "Reader error: #%X", m_parser.problem_value);
                C4_CHECK(ret >= 0);
                error(buf, static_cast<size_t>(ret), &problem_loc);
            }
            else
            {
                error("Reader error", &problem_loc);
            }
            break;

        case YAML_SCANNER_ERROR:
            error("Scanner error", &context_loc, &problem_loc);
            break;

        case YAML_PARSER_ERROR:
            error("Parser error", &context_loc, &problem_loc);
            break;

        default:
            /* Couldn't happen. */
            error("Internal error");
            break;
        };
    }

    static void error(const char* msg, size_t length, Location *loc1 = nullptr, Location *loc2 = nullptr)
    {
        fprintf(stderr, "%.*s\n", (int)length, msg);
        if(loc1 && *loc1)
        {
            fprintf(stderr, "    : %.*s at %zd:%zd (offset %zd)\n", (int)loc1->name.len, loc1->name.str, loc1->line, loc1->col, loc1->offset);
        }
        if(loc2 && *loc2)
        {
            fprintf(stderr, "    : %.*s at %zd:%zd (offset %zd)\n", (int)loc2->name.len, loc2->name.str, loc2->line, loc2->col, loc2->offset);
        }
        throw std::runtime_error(std::string(msg, msg+length));
    }
    template< size_t N >
    static void error(char const (&msg)[N], Location *loc1 = nullptr, Location *loc2 = nullptr)
    {
        error(&msg[0], N-1, loc1, loc2);
    }

};

} // namespace yml
} // namespace c4

#endif // _LIBYAML_HPP_
