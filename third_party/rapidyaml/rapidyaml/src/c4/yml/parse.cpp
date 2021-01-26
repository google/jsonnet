#include "c4/yml/parse.hpp"
#include "c4/error.hpp"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

#include "c4/yml/detail/parser_dbg.hpp"
#ifdef RYML_DBG
#include "c4/yml/detail/print.hpp"
#endif


#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4296/*expression is always 'boolean_value'*/)
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wtype-limits" // to remove a warning on an assertion that a size_t >= 0. Later on, this size_t will turn into a template argument, and then it can become < 0.
#   pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wtype-limits" // to remove a warning on an assertion that a size_t >= 0. Later on, this size_t will turn into a template argument, and then it can become < 0.
#   pragma GCC diagnostic ignored "-Wformat-nonliteral"
#   if __GNUC__ >= 7
#       pragma GCC diagnostic ignored "-Wduplicated-branches"
#   endif
#endif

namespace c4 {
namespace yml {

static bool _is_scalar_next__runk(csubstr s)
{
    if(s.begins_with(": ") || s.begins_with_any("#,:{}[]%&") || s.begins_with("? ") || s == "-" || s.begins_with("- "))
    {
        return false;
    }
    return true;
}

static bool _is_scalar_next__rseq_rval(csubstr s)
{
    if(s.begins_with_any("[{!&") || s.begins_with("? "))
    {
        return false;
    }
    else if(s.begins_with("- ") || s == "-")
    {
        return false;
    }
    return true;
}

static bool _is_scalar_next__rseq_rnxt(csubstr s)
{
    if(s.begins_with("- "))
    {
        return false;
    }
    else if(s == "-")
    {
        return false;
    }

    return true;
}

static bool _is_scalar_next__rmap(csubstr s)
{
    if(s.begins_with(": ") || s.begins_with_any("#,!&") || s.begins_with("? "))
    {
        return false;
    }
    return true;
}

static bool _is_scalar_next__rmap_val(csubstr s)
{
    if(s.begins_with("- ") || s.begins_with_any("{[") || s == "-")
    {
        return false;
    }
    return true;
}

static bool _is_doc_sep(csubstr s)
{
    constexpr const csubstr dashes = "---";
    constexpr const csubstr ellipsis = "...";
    constexpr const csubstr whitesp = " \t";
    if(s.begins_with(dashes))
    {
        return s == dashes || s.sub(3).begins_with_any(whitesp);
    }
    else if(s.begins_with(ellipsis))
    {
        return s == ellipsis || s.sub(3).begins_with_any(whitesp);
    }
    return false;
}


//-----------------------------------------------------------------------------
Parser::Parser(Allocator const& a)
    : m_file()
    , m_buf()
    , m_root_id(NONE)
    , m_tree()
    , m_stack(a)
    , m_state()
    , m_key_tag()
    , m_val_tag()
    , m_key_anchor()
    , m_val_anchor()
{
    State st{};
    m_stack.push(st);
    m_state = &m_stack.top();
}

//-----------------------------------------------------------------------------
void Parser::_reset()
{
    while(m_stack.size() > 1)
    {
        m_stack.pop();
    }
    RYML_ASSERT(m_stack.size() == 1);
    m_stack.clear();
    m_stack.push({});
    m_state = &m_stack.top();
    m_state->reset(m_file.str, m_root_id);

    m_key_tag.clear();
    m_val_tag.clear();
    m_key_anchor.clear();
    m_val_anchor.clear();
}

//-----------------------------------------------------------------------------
bool Parser::_finished_file() const
{
    bool ret = m_state->pos.offset >= m_buf.len;
    if(ret)
    {
        _c4dbgp("finished file!!!");
    }
    return ret;
}

//-----------------------------------------------------------------------------
bool Parser::_finished_line() const
{
    bool ret = m_state->line_contents.rem.empty();
    return ret;
}

//-----------------------------------------------------------------------------
void Parser::parse(csubstr file, substr buf, Tree *t, size_t node_id)
{
    m_file = file;
    m_buf = buf;
    m_root_id = node_id;
    m_tree = t;

    _reset();

    while( ! _finished_file())
    {
        _scan_line();
        while( ! _finished_line())
        {
            _handle_line();
        }
        if(_finished_file()) break; // it may have finished because of multiline blocks
        _line_ended();
    }

    _handle_finished_file();
}

//-----------------------------------------------------------------------------
void Parser::_handle_finished_file()
{
    _end_stream();
}

//-----------------------------------------------------------------------------
void Parser::_handle_line()
{
    _c4dbgq("\n-----------");
    _c4dbgt("handling line=%zu, offset=%zuB", m_state->pos.line, m_state->pos.offset);

    RYML_ASSERT( ! m_state->line_contents.rem.empty());

    if(has_any(RSEQ))
    {
        if(has_any(EXPL))
        {
            if(_handle_seq_expl())
            {
                return;
            }
        }
        else
        {
            if(_handle_seq_impl())
            {
                return;
            }
        }
    }
    else if(has_any(RMAP))
    {
        if(has_any(EXPL))
        {
            if(_handle_map_expl())
            {
                return;
            }
        }
        else
        {
            if(_handle_map_impl())
            {
                return;
            }
        }
    }
    else if(has_any(RUNK))
    {
        if(_handle_unk())
        {
            return;
        }
    }

    if(_handle_top())
    {
        return;
    }
}


//-----------------------------------------------------------------------------
bool Parser::_handle_unk()
{
    _c4dbgp("handle_unk");

    csubstr rem = m_state->line_contents.rem;
    const bool start_as_child = (node(m_state) == nullptr);

    if(C4_UNLIKELY(has_any(NDOC)))
    {
        if(rem == "---" || rem.begins_with("--- "))
        {
            _start_new_doc(rem);
            return true;
        }
        auto trimmed = rem.triml(' ');
        if(trimmed == "---" || trimmed.begins_with("--- "))
        {
            RYML_ASSERT(rem.len >= trimmed.len);
            _line_progressed(rem.len - trimmed.len);
            _start_new_doc(trimmed);
            _save_indentation();
            return true;
        }
        else if(trimmed.empty())
        {
            _line_progressed(rem.len);
            return true;
        }
        else if(trimmed.begins_with("..."))
        {
            _end_stream();
        }
        else
        {
            _c4dbgpf("starting implicit doc to accomodate unexpected tokens: '%.*s'", _c4prsp(rem));
            size_t indref = m_state->indref;
            _push_level();
            _start_doc();
            _set_indentation(indref);
        }
    }

    RYML_ASSERT(has_none(RNXT|RSEQ|RMAP));
    if(m_state->indref > 0)
    {
        csubstr ws = rem.left_of(rem.first_not_of(' '));
        if(m_state->indref <= ws.len)
        {
            _c4dbgpf("skipping base indentation of %zd", m_state->indref);
            _line_progressed(m_state->indref);
            rem = rem.sub(m_state->indref);
        }
    }

    if(rem.begins_with("- "))
    {
        _c4dbgpf("it's a seq (as_child=%d)", start_as_child);
        _push_level();
        _start_seq(start_as_child);
        _save_indentation();
        _line_progressed(2);
        return true;
    }
    else if(rem == '-')
    {
        _c4dbgpf("it's a seq (as_child=%d)", start_as_child);
        _push_level();
        _start_seq(start_as_child);
        _save_indentation();
        _line_progressed(1);
        return true;
    }
    else if(rem.begins_with('['))
    {
        _c4dbgpf("it's a seq, explicit (as_child=%d)", start_as_child);
        _push_level(/*explicit flow*/true);
        _start_seq(start_as_child);
        add_flags(EXPL);
        _line_progressed(1);
        return true;
    }
    else if(rem.begins_with('{'))
    {
        _c4dbgpf("it's a map, explicit (as_child=%d)", start_as_child);
        _push_level(/*explicit flow*/true);
        _start_map(start_as_child);
        addrem_flags(EXPL|RKEY, RVAL);
        _line_progressed(1);
        return true;
    }
    else if(rem.begins_with("? "))
    {
        _c4dbgpf("it's a map (as_child=%d) + this key is complex", start_as_child);
        _push_level();
        _start_map(start_as_child);
        addrem_flags(RKEY|CPLX, RVAL);
        _save_indentation();
        _line_progressed(2);
        return true;
    }
    else if(rem.begins_with(": ") && !has_all(SSCL))
    {
        _c4dbgp("it's a map with an empty key");
        _push_level();
        _start_map(start_as_child);
        _store_scalar("");
        addrem_flags(RVAL, RKEY);
        _save_indentation();
        _line_progressed(2);
        return true;
    }
    else if(rem == ':' && !has_all(SSCL))
    {
        _c4dbgp("it's a map with an empty key");
        _push_level();
        _start_map(start_as_child);
        _store_scalar("");
        addrem_flags(RVAL, RKEY);
        _save_indentation();
        _line_progressed(1);
        return true;
    }
    else if(_handle_types())
    {
        return true;
    }
    else if(!rem.begins_with('*') && _handle_key_anchors_and_refs())
    {
        return true;
    }
    else if(has_all(SSCL))
    {
        _c4dbgpf("there's a stored scalar: '%.*s'", _c4prsp(m_state->scalar));

        csubstr saved_scalar;
        if(_scan_scalar(&saved_scalar))
        {
            rem = m_state->line_contents.rem;
            _c4dbgpf("... and there's also a scalar next! '%.*s'", _c4prsp(saved_scalar));
            if(rem.begins_with_any(" \t"))
            {
                size_t n = rem.first_not_of(" \t");
                _c4dbgpf("skipping %zu spaces/tabs", n);
                rem = rem.sub(n);
                _line_progressed(n);
            }
        }

        _c4dbgpf("rem='%.*s'", _c4prsp(rem));

        if(rem.begins_with(", "))
        {
            _c4dbgpf("got a ',' -- it's a seq (as_child=%d)", start_as_child);
            _start_seq(start_as_child);
            add_flags(EXPL);
            _append_val(_consume_scalar());
            _line_progressed(2);
        }
        else if(rem.begins_with(','))
        {
            _c4dbgpf("got a ',' -- it's a seq (as_child=%d)", start_as_child);
            _start_seq(start_as_child);
            add_flags(EXPL);
            _append_val(_consume_scalar());
            _line_progressed(1);
        }
        else if(rem.begins_with(": "))
        {
            _c4dbgpf("got a \": \" -- it's a map (as_child=%d)", start_as_child);
            _start_map(start_as_child); // wait for the val scalar to append the key-val pair
            _line_progressed(2);
            /*if(rem == ": ")
            {
                _c4dbgp("map key opened a new line -- starting val scope as unknown");
                _start_unk();
            }*/
        }
        else if(rem == ":")
        {
            _c4dbgpf("got a ':' -- it's a map (as_child=%d)", start_as_child);
            _start_map(start_as_child); // wait for the val scalar to append the key-val pair
            _line_progressed(1);
            //_c4dbgp("map key opened a new line -- starting val scope as unknown");
            //_start_unk();
        }
        else if(rem.begins_with('}'))
        {
            if(!has_all(RMAP|EXPL))
            {
                _c4err("invalid token: not reading a map");
            }
            if(!has_all(SSCL))
            {
                _c4err("no scalar stored");
            }
            _append_key_val(saved_scalar);
            _stop_map();
            _line_progressed(1);
        }
        else if(rem.begins_with("..."))
        {
            _c4dbgp("got stream end '...'");
            _end_stream();
            _line_progressed(3);
        }
        else if(rem.begins_with('#'))
        {
            _c4dbgpf("it's a comment: '%.*s'", _c4prsp(rem));
            _scan_comment();
            return true;
        }
        else if(_handle_key_anchors_and_refs())
        {
            return true;
        }
        else if(rem.begins_with(" ") || rem.begins_with("\t"))
        {
            auto n = rem.first_not_of(" \t");
            _c4dbgpf("has %zu spaces/tabs, skip...", n);
            _line_progressed(n);
            return true;
        }
        else if(rem.empty())
        {
            // nothing to do
        }
        else if(rem == "---" || rem.begins_with("--- "))
        {
            _c4dbgp("caught ---: starting doc");
            _start_new_doc(rem);
            _save_indentation();
            return true;
        }
        else if(rem.begins_with('%'))
        {
            _c4dbgp("caught a directive: ignoring...");
            _line_progressed(rem.len);
            return true;
        }
        else
        {
            _c4err("parse error");
        }

        if( ! saved_scalar.empty())
        {
            _store_scalar(saved_scalar);
        }

        return true;
    }
    else
    {
        RYML_ASSERT( ! has_any(SSCL));
        csubstr scalar;
        size_t indentation = m_state->line_contents.indentation; // save
        if(_scan_scalar(&scalar))
        {
            _c4dbgp("got a scalar");
            rem = m_state->line_contents.rem;
            _store_scalar(scalar);
            if(rem.begins_with(": "))
            {
                _c4dbgpf("got a ': ' next -- it's a map (as_child=%d)", start_as_child);
                _push_level();
                _start_map(start_as_child); // wait for the val scalar to append the key-val pair
                _set_indentation(indentation);
                _line_progressed(2); // call this AFTER saving the indentation
            }
            else if(rem == ":")
            {
                _c4dbgpf("got a ':' next -- it's a map (as_child=%d)", start_as_child);
                _push_level();
                _start_map(start_as_child); // wait for the val scalar to append the key-val pair
                _set_indentation(indentation);
                _line_progressed(1); // call this AFTER saving the indentation
            }
            else
            {
                // we still don't know whether it's a seq or a map
                // so just store the scalar
            }
            return true;
        }
        else if(rem.begins_with(' '))
        {
            csubstr ws = rem.left_of(rem.first_not_of(' '));
            rem = rem.right_of(ws);
            if(has_all(RTOP) && rem.begins_with("---"))
            {
                _c4dbgp("there's a doc starting, and it's indented");
                _set_indentation(ws.len);
            }
            else
            {
                _c4dbgpf("skipping %zd spaces", ws.len);
            }
            _line_progressed(ws.len);
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_seq_expl()
{
    _c4dbgpf("handle_seq_expl: node_id=%zd level=%zd", m_state->node_id, m_state->level);
    csubstr rem = m_state->line_contents.rem;

    RYML_ASSERT(has_none(RKEY));
    RYML_ASSERT(has_all(RSEQ|EXPL));

    if(rem.begins_with(' '))
    {
        // with explicit flow, indentation does not matter
        _c4dbgp("starts with spaces");
        rem = rem.left_of(rem.first_not_of(' '));
        _c4dbgpf("skip %zd spaces", rem.len);
        _line_progressed(rem.len);
        return true;
    }
    else if(rem.begins_with('#'))
    {
        _c4dbgp("it's a comment");
        rem = _scan_comment(); // also progresses the line
        return true;
    }
    else if(rem.begins_with(']'))
    {
        _c4dbgp("end the sequence");
        _pop_level();
        _line_progressed(1);
        if(has_all(RSEQIMAP))
        {
            _stop_seqimap();
            _pop_level();
        }
        return true;
    }

    if(has_any(RVAL))
    {
        RYML_ASSERT(has_none(RNXT));
        if(_scan_scalar(&rem))
        {
            _c4dbgp("it's a scalar");
            addrem_flags(RNXT, RVAL);
            _append_val(rem);
            return true;
        }
        else if(rem.begins_with('['))
        {
            _c4dbgp("val is a child seq");
            addrem_flags(RNXT, RVAL); // before _push_level!
            _push_level(/*explicit flow*/true);
            _start_seq();
            add_flags(EXPL);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with('{'))
        {
            _c4dbgp("val is a child map");
            addrem_flags(RNXT, RVAL); // before _push_level!
            _push_level(/*explicit flow*/true);
            _start_map();
            addrem_flags(EXPL|RKEY, RVAL);
            _line_progressed(1);
            return true;
        }
        else if(rem == ':')
        {
            _c4dbgpf("found ':' -- there's an implicit map in the seq node[%zu]", m_state->node_id);
            _start_seqimap();
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with(": "))
        {
            _c4dbgpf("found ': ' -- there's an implicit map in the seq node[%zu]", m_state->node_id);
            _start_seqimap();
            _line_progressed(2);
            return true;
        }
        else if(rem.begins_with("? "))
        {
            _c4dbgpf("found '? ' -- there's an implicit map in the seq node[%zu]", m_state->node_id);
            _start_seqimap();
            _line_progressed(2);
            RYML_ASSERT(has_any(SSCL) && m_state->scalar == "");
            addrem_flags(CPLX|RKEY, RVAL|SSCL);
            return true;
        }
        else if(_handle_types())
        {
            return true;
        }
        else if(_handle_val_anchors_and_refs())
        {
            return true;
        }
        else if(rem.begins_with(", "))
        {
            _c4dbgp("found ',' -- the value was null");
            _append_val_null();
            _line_progressed(2);
            return true;
        }
        else if(rem.begins_with(','))
        {
            _c4dbgp("found ',' -- the value was null");
            _append_val_null();
            _line_progressed(1);
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }
    else if(has_any(RNXT))
    {
        RYML_ASSERT(has_none(RVAL));
        if(rem.begins_with(", "))
        {
            RYML_ASSERT(has_all(EXPL));
            _c4dbgp("seq: expect next val");
            addrem_flags(RVAL, RNXT);
            _line_progressed(2);
            return true;
        }
        else if(rem.begins_with(','))
        {
            RYML_ASSERT(has_all(EXPL));
            _c4dbgp("seq: expect next val");
            addrem_flags(RVAL, RNXT);
            _line_progressed(1);
            return true;
        }
        else if(rem == ':')
        {
            _c4dbgpf("found ':' -- there's an implicit map in the seq node[%zu]", m_state->node_id);
            _start_seqimap();
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with(": "))
        {
            _c4dbgpf("found ': ' -- there's an implicit map in the seq node[%zu]", m_state->node_id);
            _start_seqimap();
            _line_progressed(2);
            return true;
        }
        else
        {
            _c4err("was expecting a comma");
        }
    }
    else
    {
        _c4err("internal error");
    }

    return true;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_seq_impl()
{
    _c4dbgpf("handle_seq_impl: node_id=%zd level=%zd", m_state->node_id, m_state->level);
    csubstr rem = m_state->line_contents.rem;

    RYML_ASSERT(has_all(RSEQ));
    RYML_ASSERT(has_none(RKEY));
    RYML_ASSERT(has_none(EXPL));

    if(rem.begins_with('#'))
    {
        _c4dbgp("it's a comment");
        rem = _scan_comment();
        return true;
    }

    if(has_any(RNXT))
    {
        RYML_ASSERT(has_none(RVAL));

        if(_handle_indentation())
        {
            return true;
        }

        if(rem.begins_with("- "))
        {
            _c4dbgp("expect another val");
            addrem_flags(RVAL, RNXT);
            _line_progressed(2);
            return true;
        }
        else if(rem == '-')
        {
            _c4dbgp("expect another val");
            addrem_flags(RVAL, RNXT);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with(' '))
        {
            RYML_ASSERT( ! _at_line_begin());
            rem = rem.left_of(rem.first_not_of(' '));
            _c4dbgpf("skipping %zd spaces", rem.len);
            _line_progressed(rem.len);
            return true;
        }
        else if(rem.begins_with("..."))
        {
            _c4dbgp("got stream end '...'");
            _end_stream();
            _line_progressed(3);
            return true;
        }
        else if(rem.begins_with("---"))
        {
            _c4dbgp("got document start '---'");
            _start_new_doc(rem);
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }
    else if(has_any(RVAL))
    {
        // there can be empty values
        if(_handle_indentation())
        {
            return true;
        }

        csubstr s;
        if(_scan_scalar(&s)) // this also progresses the line
        {
            _c4dbgp("it's a scalar");

            rem = m_state->line_contents.rem;
            if(rem.begins_with(' '))
            {
                _c4dbgp("skipping whitespace...");
                size_t skip = rem.first_not_of(' ');
                if(skip == csubstr::npos) skip = rem.len; // maybe the line is just whitespace
                _line_progressed(skip);
                rem = rem.sub(skip);
            }

            if(rem.begins_with(": ") || rem.ends_with(':'))
            {
                _c4dbgp("actually, the scalar is the first key of a map, and it opens a new scope");
                addrem_flags(RNXT, RVAL); // before _push_level! This prepares the current level for popping by setting it to RNXT
                _push_level();
                _start_map();
                _store_scalar(s);
                _set_indentation(m_state->scalar_col); // this is the column where the scalar starts
                addrem_flags(RVAL, RKEY);
                _line_progressed(1);
            }
            else
            {
                _c4dbgp("appending val to current seq");
                _append_val(s);
                addrem_flags(RNXT, RVAL);
            }
            return true;
        }
        else if(rem.begins_with("- "))
        {
            if(_rval_dash_start_or_continue_seq())
            {
                _line_progressed(2);
            }
            return true;
        }
        else if(rem == '-')
        {
            if(_rval_dash_start_or_continue_seq())
            {
                _line_progressed(1);
            }
            return true;
        }
        else if(rem.begins_with('['))
        {
            _c4dbgp("val is a child seq, explicit");
            addrem_flags(RNXT, RVAL); // before _push_level!
            _push_level(/*explicit flow*/true);
            _start_seq();
            add_flags(EXPL);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with('{'))
        {
            _c4dbgp("val is a child map, explicit");
            addrem_flags(RNXT, RVAL); // before _push_level!
            _push_level(/*explicit flow*/true);
            _start_map();
            addrem_flags(EXPL|RKEY, RVAL);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with("? "))
        {
            _c4dbgp("val is a child map + this key is complex");
            addrem_flags(RNXT, RVAL); // before _push_level!
            _push_level();
            _start_map();
            addrem_flags(CPLX|RKEY, RVAL);
            _save_indentation();
            _line_progressed(2);
            return true;
        }
        else if(rem.begins_with(' '))
        {
            csubstr spc = rem.left_of(rem.first_not_of(' '));
            if(_at_line_begin())
            {
                _c4dbgpf("skipping value indentation: %zd spaces", spc.len);
                _line_progressed(spc.len);
                return true;
            }
            else
            {
                _c4dbgpf("skipping %zd spaces", spc.len);
                _line_progressed(spc.len);
                return true;
            }
        }
        else if(_handle_types())
        {
            return true;
        }
        else if(_handle_val_anchors_and_refs())
        {
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }

    return false;
}

//-----------------------------------------------------------------------------

bool Parser::_rval_dash_start_or_continue_seq()
{
    size_t ind = m_state->line_contents.current_col();
    RYML_ASSERT(ind >= m_state->indref);
    size_t delta_ind = ind - m_state->indref;
    if( ! delta_ind)
    {
        _c4dbgp("prev val was empty");
        addrem_flags(RNXT, RVAL);
        _append_val_null();
        return false;
    }
    _c4dbgp("val is a nested seq, indented");
    addrem_flags(RNXT, RVAL); // before _push_level!
    _push_level();
    _start_seq();
    _save_indentation();
    return true;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_map_expl()
{
    // explicit flow, ie, inside {}, separated by commas
    _c4dbgpf("handle_map_expl: node_id=%zd  level=%zd", m_state->node_id, m_state->level);
    csubstr rem = m_state->line_contents.rem;

    RYML_ASSERT(has_all(RMAP|EXPL));

    if(rem.begins_with(' '))
    {
        // with explicit flow, indentation does not matter
        _c4dbgp("starts with spaces");
        rem = rem.left_of(rem.first_not_of(' '));
        _c4dbgpf("skip %zd spaces", rem.len);
        _line_progressed(rem.len);
        return true;
    }
    else if(rem.begins_with('#'))
    {
        _c4dbgp("it's a comment");
        rem = _scan_comment(); // also progresses the line
        return true;
    }
    else if(rem.begins_with('}'))
    {
        _c4dbgp("end the map");
        if(has_all(SSCL))
        {
            _c4dbgp("the last val was null");
            _append_key_val_null();
            rem_flags(RVAL);
        }
        _pop_level();
        _line_progressed(1);
        if(has_all(RSEQIMAP))
        {
            _c4dbgp("stopping implicitly nested 1x map");
            _stop_seqimap();
            _pop_level();
        }
        return true;
    }

    if(has_any(RNXT))
    {
        RYML_ASSERT(has_none(RKEY));
        RYML_ASSERT(has_none(RVAL));
        RYML_ASSERT(has_none(RSEQIMAP));

        if(rem.begins_with(", "))
        {
            _c4dbgp("seq: expect next keyval");
            addrem_flags(RKEY, RNXT);
            _line_progressed(2);
            return true;
        }
        else if(rem.begins_with(','))
        {
            _c4dbgp("seq: expect next keyval");
            addrem_flags(RKEY, RNXT);
            _line_progressed(1);
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }
    else if(has_any(RKEY))
    {
        RYML_ASSERT(has_none(RNXT));
        RYML_ASSERT(has_none(RVAL));

        if(has_none(SSCL) && _scan_scalar(&rem))
        {
            _c4dbgp("it's a scalar");
            _store_scalar(rem);
            rem = m_state->line_contents.rem;
            csubstr trimmed = rem.triml(" \t");
            if(trimmed.len && (trimmed.begins_with(": ") || trimmed.begins_with_any(":,}")))
            {
                RYML_ASSERT(trimmed.str >= rem.str);
                size_t num = static_cast<size_t>(trimmed.str - rem.str);
                _c4dbgpf("trimming %zu whitespace after the scalar: '%.*s' --> '%.*s'", num, _c4prsp(rem), _c4prsp(rem.sub(num)));
                rem = rem.sub(num);
                _line_progressed(num);
            }
        }

        if(rem.begins_with(": "))
        {
            _c4dbgp("wait for val");
            addrem_flags(RVAL, RKEY|CPLX);
            _line_progressed(2);
            if(!has_all(SSCL))
            {
                _c4dbgp("no key was found, defaulting to empty key ''");
                _store_scalar("");
            }
            return true;
        }
        else if(rem == ':')
        {
            _c4dbgp("wait for val");
            addrem_flags(RVAL, RKEY|CPLX);
            _line_progressed(1);
            if(!has_all(SSCL))
            {
                _c4dbgp("no key was found, defaulting to empty key ''");
                _store_scalar("");
            }
            return true;
        }
        else if(rem.begins_with('?'))
        {
            _c4dbgp("complex key");
            add_flags(CPLX);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with(','))
        {
            _c4dbgp("prev scalar was a key with null value");
            _append_key_val_null();
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with('}'))
        {
            _c4dbgp("map terminates after a key...");
            RYML_ASSERT(has_all(SSCL));
            _c4dbgp("the last val was null");
            _append_key_val_null();
            rem_flags(RVAL);
            if(has_all(RSEQIMAP))
            {
                _c4dbgp("stopping implicitly nested 1x map");
                _stop_seqimap();
                _pop_level();
            }
            _pop_level();
            _line_progressed(1);
            return true;
        }
        else if(_handle_types())
        {
            return true;
        }
        else if(_handle_key_anchors_and_refs())
        {
            return true;
        }
        else if(rem == "")
        {
            return true;
        }
        else if(rem.begins_with('}'))
        {
            _c4dbgp("the last val was null");
            _append_key_val_null();
            _line_progressed(1);
            return true;
        }
        else
        {
            size_t pos = rem.first_not_of(" \t");
            if(pos == csubstr::npos) pos = 0;
            rem = rem.sub(pos);
            if(rem.begins_with(':'))
            {
                _c4dbgp("wait for val");
                addrem_flags(RVAL, RKEY|CPLX);
                _line_progressed(pos + 1);
                if(!has_all(SSCL))
                {
                    _c4dbgp("no key was found, defaulting to empty key ''");
                    _store_scalar("");
                }
                return true;
            }
            else if(rem.begins_with('#'))
            {
                _c4dbgp("it's a comment");
                _line_progressed(pos);
                rem = _scan_comment(); // also progresses the line
                return true;
            }
            else
            {
                _c4err("parse error");
            }
        }
    }
    else if(has_any(RVAL))
    {
        RYML_ASSERT(has_none(RNXT));
        RYML_ASSERT(has_none(RKEY));
        RYML_ASSERT(has_all(SSCL));
        if(_scan_scalar(&rem))
        {
            _c4dbgp("it's a scalar");
            addrem_flags(RNXT, RVAL|RKEY);
            _append_key_val(rem);
            if(has_all(RSEQIMAP))
            {
                _c4dbgp("stopping implicitly nested 1x map");
                _stop_seqimap();
                _pop_level();
            }
            return true;
        }
        else if(rem.begins_with('['))
        {
            _c4dbgp("val is a child seq");
            addrem_flags(RNXT, RVAL|RKEY); // before _push_level!
            _push_level(/*explicit flow*/true);
            _move_scalar_from_top();
            _start_seq();
            add_flags(EXPL);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with('{'))
        {
            _c4dbgp("val is a child map");
            addrem_flags(RNXT, RVAL|RKEY); // before _push_level!
            _push_level(/*explicit flow*/true);
            _move_scalar_from_top();
            _start_map();
            addrem_flags(EXPL|RKEY, RNXT|RVAL);
            _line_progressed(1);
            return true;
        }
        else if(_handle_types())
        {
            return true;
        }
        else if(_handle_val_anchors_and_refs())
        {
            return true;
        }
        else if(rem.begins_with(','))
        {
            _c4dbgp("appending empty val");
            _append_key_val_null();
            addrem_flags(RKEY, RVAL);
            _line_progressed(1);
            if(has_any(RSEQIMAP))
            {
                _c4dbgp("stopping implicitly nested 1x map");
                _stop_seqimap();
                _pop_level();
            }
            return true;
        }
        else if(has_any(RSEQIMAP) && rem.begins_with(']'))
        {
            _c4dbgp("stopping implicitly nested 1x map");
            if(has_any(SSCL))
            {
                _append_key_val_null();
            }
            _stop_seqimap();
            _pop_level();
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }
    else
    {
        _c4err("internal error");
    }

    return false;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_map_impl()
{
    _c4dbgpf("handle_map_impl: node_id=%zd  level=%zd", m_state->node_id, m_state->level);
    csubstr rem = m_state->line_contents.rem;

    RYML_ASSERT(has_all(RMAP));
    RYML_ASSERT(has_none(EXPL));

    if(rem.begins_with('#'))
    {
        _c4dbgp("it's a comment");
        rem = _scan_comment();
        return true;
    }

    if(has_any(RNXT))
    {
        RYML_ASSERT(has_none(RKEY));
        RYML_ASSERT(has_none(RVAL));
        // actually, we don't need RNXT in indent-based maps.
        addrem_flags(RKEY, RNXT);
    }

    if(_handle_indentation())
    {
        //rem = m_state->line_contents.rem;
        return true;
    }

    if(has_any(RKEY))
    {
        RYML_ASSERT(has_none(RNXT));
        RYML_ASSERT(has_none(RVAL));

        _c4dbgp("read scalar?");
        if(_scan_scalar(&rem)) // this also progresses the line
        {
            _c4dbgp("it's a scalar");
            _store_scalar(rem);
            if(has_all(CPLX|RSET))
            {
                _c4dbgp("it's a complex key, so use null value '~'");
                _append_key_val_null();
            }
            rem = m_state->line_contents.rem;

            if(rem.begins_with(':'))
            {
                _c4dbgp("wait for val");
                addrem_flags(RVAL, RKEY|CPLX);
                _line_progressed(1);
                rem = m_state->line_contents.rem;
                if(rem.begins_with(' '))
                {
                    RYML_ASSERT( ! _at_line_begin());
                    rem = rem.left_of(rem.first_not_of(' '));
                    _c4dbgpf("skip %zd spaces", rem.len);
                    _line_progressed(rem.len);
                }
            }
            return true;
        }
        else if(rem.begins_with(' '))
        {
            //RYML_ASSERT( ! _at_line_begin());
            rem = rem.left_of(rem.first_not_of(' '));
            _c4dbgpf("skip %zd spaces", rem.len);
            _line_progressed(rem.len);
            return true;
        }
        else if(rem.begins_with("? "))
        {
            _c4dbgp("it's a complex key");
            add_flags(CPLX);
            _line_progressed(2);
            if(has_all(SSCL))
            {
                _append_key_val_null();
            }
            return true;
        }
        else if(has_all(CPLX) && rem.begins_with(':'))
        {
            _c4dbgp("complex key finished");
            addrem_flags(RVAL, RKEY|CPLX);
            _line_progressed(1);
            rem = m_state->line_contents.rem;
            if(rem.begins_with(' '))
            {
                RYML_ASSERT( ! _at_line_begin());
                rem = rem.left_of(rem.first_not_of(' '));
                _c4dbgpf("skip %zd spaces", rem.len);
                _line_progressed(rem.len);
            }
            return true;
        }
        else if(rem.begins_with(": "))
        {
            _c4dbgp("key finished");
            if(!has_all(SSCL))
            {
                _c4dbgp("key was empty...");
                _store_scalar("");
            }
            addrem_flags(RVAL, RKEY);
            _line_progressed(2);
            return true;
        }
        else if(rem == ':')
        {
            _c4dbgp("key finished");
            if(!has_all(SSCL))
            {
                _c4dbgp("key was empty...");
                _store_scalar("");
            }
            addrem_flags(RVAL, RKEY);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with("..."))
        {
            _c4dbgp("end current document");
            _end_stream();
            _line_progressed(3);
            return true;
        }
        else if(rem.begins_with("---"))
        {
            _c4dbgp("start new document '---'");
            _start_new_doc(rem);
            return true;
        }
        else if(_handle_types())
        {
            return true;
        }
        else if(_handle_key_anchors_and_refs())
        {
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }
    else if(has_any(RVAL))
    {
        RYML_ASSERT(has_none(RNXT));
        RYML_ASSERT(has_none(RKEY));

        csubstr s;
        if(_scan_scalar(&s)) // this also progresses the line
        {
            _c4dbgp("it's a scalar");

            rem = m_state->line_contents.rem;

            if(rem.begins_with(": "))
            {
                _c4dbgp("actually, the scalar is the first key of a map");
                addrem_flags(RKEY, RVAL); // before _push_level! This prepares the current level for popping by setting it to RNXT
                _push_level();
                _move_scalar_from_top();
                _start_map();
                _save_indentation(m_state->scalar_col);
                addrem_flags(RVAL, RKEY);
                _line_progressed(2);
            }
            else if(rem.begins_with(':'))
            {
                _c4dbgp("actually, the scalar is the first key of a map, and it opens a new scope");
                addrem_flags(RKEY, RVAL); // before _push_level! This prepares the current level for popping by setting it to RNXT
                _push_level();
                _move_scalar_from_top();
                _start_map();
                _save_indentation(/*behind*/s.len);
                addrem_flags(RVAL, RKEY);
                _line_progressed(1);
            }
            else
            {
                _c4dbgp("appending keyval to current map");
                _append_key_val(s);
                addrem_flags(RKEY, RVAL);
            }
            return true;
        }
        else if(rem.begins_with("- "))
        {
            _c4dbgp("val is a nested seq, indented");
            addrem_flags(RKEY, RVAL); // before _push_level!
            _push_level();
            _move_scalar_from_top();
            _start_seq();
            _save_indentation();
            _line_progressed(2);
            return true;
        }
        else if(rem == '-')
        {
            _c4dbgp("maybe a seq. start unknown, indented");
            _start_unk();
            _save_indentation();
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with('['))
        {
            _c4dbgp("val is a child seq, explicit");
            addrem_flags(RKEY, RVAL); // before _push_level!
            _push_level(/*explicit flow*/true);
            _move_scalar_from_top();
            _start_seq();
            add_flags(EXPL);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with('{'))
        {
            _c4dbgp("val is a child map, explicit");
            addrem_flags(RKEY, RVAL); // before _push_level!
            _push_level(/*explicit flow*/true);
            _move_scalar_from_top();
            _start_map();
            addrem_flags(EXPL|RKEY, RVAL);
            _line_progressed(1);
            return true;
        }
        else if(rem.begins_with(' '))
        {
            csubstr spc = rem.left_of(rem.first_not_of(' '));
            if(_at_line_begin())
            {
                _c4dbgpf("skipping value indentation: %zd spaces", spc.len);
                _line_progressed(spc.len);
                return true;
            }
            else
            {
                _c4dbgpf("skipping %zd spaces", spc.len);
                _line_progressed(spc.len);
                return true;
            }
        }
        else if(_handle_types())
        {
            return true;
        }
        else if(_handle_val_anchors_and_refs())
        {
            return true;
        }
        else
        {
            _c4err("parse error");
        }
    }
    else
    {
        _c4err("internal error");
    }

    return false;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_top()
{
    _c4dbgp("handle_top");
    csubstr rem = m_state->line_contents.rem;

    if(rem.begins_with('#'))
    {
        _c4dbgp("a comment line");
        _scan_comment();
        return true;
    }

    csubstr trimmed = rem.triml(' ');

    if(trimmed.begins_with('%'))
    {
        _c4dbgpf("%% directive! ignoring...: '%.*s'", _c4prsp(rem));
        _line_progressed(rem.len);
        return true;
    }
    else if(trimmed.begins_with("--- ") || trimmed == "---" || trimmed.begins_with("---\t"))
    {
        _start_new_doc(rem);
        if(trimmed.len < rem.len)
        {
            _line_progressed(rem.len - trimmed.len);
            _save_indentation();
        }
        return true;
    }
    else if(trimmed.begins_with("..."))
    {
        _c4dbgp("end current document");
        _end_stream();
        if(trimmed.len < rem.len)
        {
            _line_progressed(rem.len - trimmed.len);
        }
        _line_progressed(3);
        return true;
    }
    else
    {
        _c4err("parse error");
    }

    return false;
}

//-----------------------------------------------------------------------------
csubstr Parser::_scan_ref()
{
    csubstr rem = m_state->line_contents.rem;
    RYML_ASSERT(rem.begins_with("<<"));

    size_t pos = rem.find(": ");
    // for now we require the target anchor to be in the same line
    RYML_ASSERT(pos != npos);
    _line_progressed(pos + 2);

    csubstr ref = rem.right_of(pos);
    pos = ref.first_of('*');
    RYML_ASSERT(pos != npos);
    ref = ref.right_of(pos);
    _line_progressed(pos);
    ref = ref.left_of(ref.first_of(' '));
    _line_progressed(ref.len);

    _c4dbgpf("scanned ref value: '%.*s'", _c4prsp(ref));
    return ref;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_key_anchors_and_refs()
{
    RYML_ASSERT(!has_any(RVAL));
    csubstr rem = m_state->line_contents.rem;
    if(rem.begins_with('&'))
    {
        _c4dbgp("found a key anchor!!!");
        csubstr anchor = rem.left_of(rem.first_of(' '));
        _line_progressed(anchor.len);
        anchor = anchor.sub(1); // skip the first character
        if(!m_key_anchor.empty())
        {
            _c4dbgpf("move current key anchor to val slot: '%.*s'", _c4prsp(m_key_anchor));
            if(!m_val_anchor.empty())
            {
                _c4err("triple-pending anchor");
            }
            m_val_anchor = m_key_anchor;
        }
        _c4dbgpf("key anchor value: '%.*s'", _c4prsp(anchor));
        m_key_anchor = anchor;
        return true;
    }
    else if(rem.begins_with('*'))
    {
        _c4err("not implemented - this should have been catched elsewhere");
        C4_NEVER_REACH();
        return false;
    }

    return false;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_val_anchors_and_refs()
{
    RYML_ASSERT(!has_any(RKEY));
    csubstr rem = m_state->line_contents.rem;
    if(rem.begins_with('&'))
    {
        _c4dbgp("found a val anchor!!!");
        if(!m_val_anchor.empty())
        {
            _c4dbgpf("anchor value: '%.*s' empty=%d", _c4prsp(m_val_anchor), m_val_anchor.empty());
            _c4err("there's a pending anchor");
        }
        csubstr anchor = rem.left_of(rem.first_of(' '));
        _line_progressed(anchor.len);
        anchor = anchor.sub(1); // skip the first character
        _c4dbgpf("val anchor value: '%.*s'", _c4prsp(anchor));
        m_val_anchor = anchor;
        return true;
    }
    else if(rem.begins_with('*'))
    {
        _c4err("not implemented - this should have been catched elsewhere");
        C4_NEVER_REACH();
        return false;
    }

    return false;
}

//-----------------------------------------------------------------------------
bool Parser::_handle_types()
{
    csubstr rem = m_state->line_contents.rem.triml(' ');
    csubstr t;

    if(rem.begins_with("!!"))
    {
        _c4dbgp("begins with '!!'");
        t = rem.left_of(rem.first_of(" ,"));
        RYML_ASSERT(t.len >= 2);
        //t = t.sub(2);
        if(t == "!!set")
        {
            add_flags(RSET);
        }
    }
    else if(rem.begins_with("!<"))
    {
        _c4dbgp("begins with '!<'");
        t = rem.left_of(rem.first_of(' '));
        RYML_ASSERT(t.len >= 2);
        //t = t.sub(2, t.len-1);
    }
    else if(rem.begins_with("!h!"))
    {
        _c4dbgp("begins with '!h!'");
        t = rem.left_of(rem.first_of(' '));
        RYML_ASSERT(t.len >= 3);
        //t = t.sub(3);
    }
    else if(rem.begins_with('!'))
    {
        _c4dbgp("begins with '!'");
        t = rem.left_of(rem.first_of(' '));
        RYML_ASSERT(t.len >= 1);
        //t = t.sub(1);
    }

    if(t.empty())
    {
        return false;
    }

    _c4dbgpf("there was a tag: '%.*s'", _c4prsp(t));
    RYML_ASSERT(t.end() > m_state->line_contents.rem.begin());
    _line_progressed(static_cast<size_t>(t.end() - m_state->line_contents.rem.begin()));

    if(has_all(RMAP|RKEY))
    {
        _c4dbgpf("saving map key tag '%.*s'", _c4prsp(t));
        RYML_ASSERT(m_key_tag.empty());
        m_key_tag = t;
    }
    else if(has_all(RMAP|RVAL))
    {
        /* foo: !!str
         * !!str : bar  */
        rem = m_state->line_contents.rem;
        rem = rem.left_of(rem.find("#"));
        rem = rem.trim(" \t");
        _c4dbgpf("rem='%.*s'", _c4prsp(rem));
        if(rem == ':' || rem.begins_with(": "))
        {
            _c4dbgp("the last val was null, and this is a tag from a null key");
            _append_key_val_null();
            _store_scalar_null();
            // do not change the flag to key, it is ~
            RYML_ASSERT(rem.begin() > m_state->line_contents.rem.begin());
            size_t token_len = rem == ':' ? 1 : 2;
            _line_progressed(static_cast<size_t>(token_len + rem.begin() - m_state->line_contents.rem.begin()));
        }
        RYML_ASSERT(m_val_tag.empty());
        m_val_tag = t;
    }
    else if(has_all(RSEQ|RVAL))
    {
        _c4dbgpf("saving seq val tag '%.*s'", _c4prsp(t));
        RYML_ASSERT(m_val_tag.empty());
        m_val_tag = t;
    }
    else if(has_all(RTOP|RUNK|NDOC))
    {
        _c4dbgpf("saving doc tag '%.*s'", _c4prsp(t));
        RYML_ASSERT(m_val_tag.empty());
        m_val_tag = t;
    }
    else if(has_all(RTOP|RUNK))
    {
        rem = m_state->line_contents.rem;
        rem = rem.left_of(rem.find("#"));
        rem = rem.trim(" \t");
        if(rem.empty())
        {
            _c4dbgpf("saving val tag '%.*s'", _c4prsp(t));
            RYML_ASSERT(m_val_tag.empty());
            m_val_tag = t;
        }
        else
        {
            _c4dbgpf("saving key tag '%.*s'", _c4prsp(t));
            RYML_ASSERT(m_key_tag.empty());
            m_key_tag = t;
        }
    }
    else
    {
        _c4err("internal error");
    }

    if(!m_val_tag.empty())
    {
        YamlTag_e tag = to_tag(t);
        if(tag == TAG_MAP || tag == TAG_OMAP || tag == TAG_PAIRS || tag == TAG_SET)
        {
            _c4dbgpf("tag '%.*s' is a map-type tag", _c4prsp(t));
            //_push_level();
            //_start_map(/*as_child*/false);
            //_save_indentation(m_state->line_contents.indentation);
            //addrem_flags(RKEY, RVAL);
        }
        else if(tag == TAG_SEQ)
        {
            _c4dbgpf("tag '%.*s' is a seq-type tag", _c4prsp(t));
            //_push_level();
            //_start_seq(/*as_child*/false);
            //_save_indentation(0);
            //addrem_flags(RNXT, RVAL);
        }
        else if(tag == TAG_STR)
        {
            _c4dbgpf("tag '%.*s' is a str-type tag", _c4prsp(t));
            if(has_all(RTOP|RUNK|NDOC))
            {
                _c4dbgpf("docval. slurping the string. pos=%zu", m_state->pos.offset);
                csubstr scalar = _slurp_doc_scalar();
                _c4dbgpf("docval. after slurp: %zu, at node %zu: '%.*s'", m_state->pos.offset, m_state->node_id, _c4prsp(scalar));
                m_tree->to_val(m_state->node_id, scalar, DOC);
                m_tree->set_val_tag(m_state->node_id, m_val_tag);
                m_val_tag.clear();
                if(!m_val_anchor.empty())
                {
                    _c4dbgpf("setting val anchor[%zu]='%.*s'", m_state->node_id, _c4prsp(m_val_anchor));
                    m_tree->set_val_anchor(m_state->node_id, m_val_anchor);
                    m_val_anchor.clear();
                }
                _end_stream();
            }
        }
    }
    return true;
}

//-----------------------------------------------------------------------------
csubstr Parser::_slurp_doc_scalar()
{
    csubstr s = m_state->line_contents.rem;
    size_t pos = m_state->pos.offset;
    RYML_ASSERT(m_state->line_contents.full.find("---") != csubstr::npos);
    _c4dbgpf("CRL 0 '%.*s'. REM='%.*s'", _c4prsp(s), _c4prsp(m_buf.sub(m_state->pos.offset)));
    if(s.len == 0)
    {
        _line_ended();
        _scan_line();
        s = m_state->line_contents.rem;
        pos = m_state->pos.offset;
    }

    size_t skipws = s.first_not_of(" \t");
    _c4dbgpf("CRL 1 '%.*s'. REM='%.*s'", _c4prsp(s), _c4prsp(m_buf.sub(m_state->pos.offset)));
    if(skipws != npos)
    {
        _line_progressed(skipws);
        s = m_state->line_contents.rem;
        pos = m_state->pos.offset;
        _c4dbgpf("CRL 2 '%.*s'. REM='%.*s'", _c4prsp(s), _c4prsp(m_buf.sub(m_state->pos.offset)));
    }

    RYML_ASSERT(m_val_anchor.empty());
    _handle_val_anchors_and_refs();
    if(!m_val_anchor.empty())
    {
        s = m_state->line_contents.rem;
        skipws = s.first_not_of(" \t");
        if(skipws != npos)
        {
            _line_progressed(skipws);
        }
        s = m_state->line_contents.rem;
        pos = m_state->pos.offset;
        _c4dbgpf("CRL 3 '%.*s'. REM='%.*s'", _c4prsp(s), _c4prsp(m_buf.sub(m_state->pos.offset)));
    }

    if(s.begins_with('\''))
    {
        m_state->scalar_col = m_state->line_contents.current_col(s);
        return _scan_quoted_scalar('\'');
    }
    else if(s.begins_with('"'))
    {
        m_state->scalar_col = m_state->line_contents.current_col(s);
        return _scan_quoted_scalar('"');
    }
    else if(s.begins_with('|') || s.begins_with('>'))
    {
        return _scan_block();
    }

    _c4dbgpf("CRL 4 '%.*s'. REM='%.*s'", _c4prsp(s), _c4prsp(m_buf.sub(m_state->pos.offset)));

    m_state->scalar_col = m_state->line_contents.current_col(s);
    RYML_ASSERT(s.end() >= m_buf.begin() + pos);
    _line_progressed(static_cast<size_t>(s.end() - (m_buf.begin() + pos)));

    _c4dbgpf("CRL 5 '%.*s'. REM='%.*s'", _c4prsp(s), _c4prsp(m_buf.sub(m_state->pos.offset)));

    if(_at_line_end())
    {
        _c4dbgpf("at line end. curr='%.*s'", _c4prsp(s));
        s = _extend_scanned_scalar(s);
    }

    _c4dbgpf("scalar was '%.*s'", _c4prsp(s));

    return s;
}

//-----------------------------------------------------------------------------
bool Parser::_scan_scalar(csubstr *scalar)
{
    csubstr s = m_state->line_contents.rem;
    if(s.len == 0) return false;
    s = s.trim(' ');
    if(s.len == 0) return false;

    if(s.begins_with('\''))
    {
        m_state->scalar_col = m_state->line_contents.current_col(s);
        *scalar = _scan_quoted_scalar('\'');
        return true;
    }
    else if(s.begins_with('"'))
    {
        m_state->scalar_col = m_state->line_contents.current_col(s);
        *scalar = _scan_quoted_scalar('"');
        return true;
    }
    else if(s.begins_with('|') || s.begins_with('>'))
    {
        *scalar = _scan_block();
        return true;
    }
    else if(has_any(RTOP) && _is_doc_sep(s))
    {
        return false;
    }
    else if(has_any(RSEQ))
    {
        RYML_ASSERT( ! has_all(RKEY));
        if(has_all(RVAL))
        {
            _c4dbgp("RSEQ|RVAL");
            if( ! _is_scalar_next__rseq_rval(s))
            {
                return false;
            }
            s = s.left_of(s.find(" #")); // is there a comment?
            s = s.left_of(s.find(": ")); // is there a key-value?
            if(s.ends_with(':')) s = s.left_of(s.len-1);
            if(has_all(EXPL))
            {
                _c4dbgp("RSEQ|RVAL|EXPL");
                s = s.left_of(s.first_of(",]"));
            }
            s = s.trimr(' ');
        }
        else if(has_all(RNXT))
        {
            if( ! _is_scalar_next__rseq_rnxt(s))
            {
                return false;
            }
            _c4err("internal error");
        }
        else
        {
            _c4err("internal error");
        }
    }
    else if(has_any(RMAP))
    {
        if( ! _is_scalar_next__rmap(s))
        {
            return false;
        }
        size_t colon_space = s.find(": ");
        if(colon_space == npos)
        {
            colon_space = s.find(":");
            RYML_ASSERT(s.len > 0);
            if(colon_space != s.len-1)
            {
                colon_space = npos;
            }
        }

        if(has_all(RKEY))
        {
            if(has_any(CPLX))
            {
                _c4dbgp("RMAP|RKEY|CPLX");
                RYML_ASSERT(has_any(RMAP));
                s = s.left_of(colon_space);
                if(s.begins_with("---"))
                {
                    return false;
                }
                else if(s.begins_with("..."))
                {
                    return false;
                }
            }
            else
            {
                _c4dbgp("RMAP|RKEY");
                s = s.triml(' ');
                s = s.left_of(colon_space);
                s = s.trimr(' ');
                if(has_any(EXPL))
                {
                    _c4dbgp("RMAP|RVAL|EXPL");
                    s = s.left_of(s.first_of(",}"));
                }
                else if(s.begins_with("---"))
                {
                    return false;
                }
                else if(s.begins_with("..."))
                {
                    return false;
                }
            }
        }
        else if(has_all(RVAL))
        {
            _c4dbgp("RMAP|RVAL");
            RYML_ASSERT(has_none(CPLX));
            if( ! _is_scalar_next__rmap_val(s))
            {
                return false;
            }
            s = s.left_of(s.find(" #")); // is there a comment?
            if(has_any(EXPL))
            {
                _c4dbgp("RMAP|RVAL|EXPL");
                if(has_none(RSEQIMAP))
                {
                    s = s.left_of(s.first_of(",}"));
                }
                else
                {
                    s = s.left_of(s.first_of(",]"));
                }
            }
            s = s.trim(' ');
        }
        else
        {
            _c4err("parse error");
        }
    }
    else if(has_all(RUNK))
    {
        _c4dbgp("RUNK");
        if( ! _is_scalar_next__runk(s))
        {
            _c4dbgp("RUNK: no scalar next");
            return false;
        }
        s = s.left_of(s.find(" #"));
        size_t pos = s.find(": ");
        if(pos != npos)
        {
            s = s.left_of(pos);
        }
        else if(s.ends_with(':'))
        {
            s = s.left_of(s.len-1);
        }
        else
        {
            s = s.left_of(s.first_of(','));
        }
        s = s.trim(' ');
        _c4dbgpf("RUNK: scalar='%.*s'", _c4prsp(s));
    }
    else
    {
        _c4err("not implemented");
    }

    if(s.empty()) return false;

    m_state->scalar_col = m_state->line_contents.current_col(s);
    RYML_ASSERT(s.str >= m_state->line_contents.rem.str);
    _line_progressed(static_cast<size_t>(s.str - m_state->line_contents.rem.str) + s.len);

    if(_at_line_end())
    {
        _c4dbgpf("at line end. curr='%.*s'", _c4prsp(s));
        s = _extend_scanned_scalar(s);
    }

    _c4dbgpf("scalar was '%.*s'", _c4prsp(s));

    if(s == '~')
    {
        _c4dbgp("scalar was '~', so use null");
        s = {};
    }
    else if(s == "null")
    {
        _c4dbgp("scalar was null");
        s = {};
    }

    *scalar = s;
    return true;
}

//-----------------------------------------------------------------------------

csubstr Parser::_extend_scanned_scalar(csubstr s)
{
    if(has_all(RMAP|RKEY|CPLX))
    {
        size_t scalar_indentation = has_any(EXPL) ? 0 : m_state->indref;
        _c4dbgp("complex key!");
        csubstr n = _scan_to_next_nonempty_line(scalar_indentation);
        if(!n.empty())
        {
            substr full = _scan_complex_key(s, n).trimr(" \t\r\n");
            if(full != s)
            {
                s = _filter_plain_scalar(full, scalar_indentation);
            }
        }
    }
    // deal with plain (unquoted) scalars that continue to the next line
    else if(!s.begins_with_any("*")) // cannot be a plain scalar if it starts with * (that's an anchor reference)
    {
        _c4dbgpf("reading plain scalar: line ended, scalar='%.*s'", _c4prsp(s));
        if(has_none(EXPL))
        {
            size_t scalar_indentation = m_state->indref + 1;
            csubstr n = _scan_to_next_nonempty_line(scalar_indentation);
            if(!n.empty())
            {
                RYML_ASSERT(m_state->line_contents.full.is_super(n));
                _c4dbgpf("rscalar[IMPL]: state_indref=%zu state_indentation=%zu scalar_indentation=%zu", m_state->indref, m_state->line_contents.indentation, scalar_indentation);
                substr full = _scan_plain_scalar_impl(s, n, scalar_indentation);
                if(full != s)
                {
                    s = _filter_plain_scalar(full, scalar_indentation);
                }
            }
        }
        else
        {
            RYML_ASSERT(has_all(EXPL));
            csubstr n = _scan_to_next_nonempty_line(/*indentation*/0);
            if(!n.empty())
            {
                _c4dbgp("rscalar[EXPL]");
                substr full = _scan_plain_scalar_expl(s, n);
                s = _filter_plain_scalar(full, /*indentation*/0);
            }
        }
    }

    return s;
}


//-----------------------------------------------------------------------------

substr Parser::_scan_plain_scalar_expl(csubstr currscalar, csubstr peeked_line)
{
    static constexpr const csubstr chars = "[]{}?#,";
    size_t pos = peeked_line.first_of(chars);
    bool first = true;
    while(pos != 0)
    {
        if(has_any(RMAP|RUNK))
        {
            csubstr tpkl = peeked_line.triml(' ').trimr("\r\n");
            if(tpkl.begins_with(": ") || tpkl == ':')
            {
                _c4dbgpf("rscalar[EXPL]: map value starts on the peeked line: '%.*s'", _c4prsp(peeked_line));
                peeked_line = peeked_line.first(0);
                break;
            }
        }
        if(pos != npos)
        {
            _c4dbgpf("rscalar[EXPL]: found special character '%c' at %zu, stopping: '%.*s'", peeked_line[pos], pos, _c4prsp(peeked_line.left_of(pos).trimr("\r\n")));
            peeked_line = peeked_line.left_of(pos);
            RYML_ASSERT(peeked_line.end() >= m_state->line_contents.rem.begin());
            _line_progressed(static_cast<size_t>(peeked_line.end() - m_state->line_contents.rem.begin()));
            break;
        }
        _c4dbgpf("rscalar[EXPL]: append another line, full: '%.*s'", _c4prsp(peeked_line.trimr("\r\n")));
        if(!first)
        {
            RYML_CHECK(_advance_to_peeked());
        }
        peeked_line = _scan_to_next_nonempty_line(/*indentation*/0);
        if(peeked_line.empty())
        {
            _c4err("expected token or continuation");
        }
        pos = peeked_line.first_of(chars);
        first = false;
    }
    substr full(m_buf.str + (currscalar.str - m_buf.str), m_buf.begin() + m_state->pos.offset);
    full = full.trimr("\r\n ");
    return full;
}


//-----------------------------------------------------------------------------

substr Parser::_scan_plain_scalar_impl(csubstr currscalar, csubstr peeked_line, size_t indentation)
{
    RYML_ASSERT(m_buf.is_super(currscalar));
    // NOTE. there's a problem with _scan_to_next_nonempty_line(), as it counts newlines twice
    // size_t offs = m_state->pos.offset;   // so we workaround by directly counting from the end of the given scalar
    RYML_ASSERT(currscalar.end() >= m_buf.begin());
    size_t offs = static_cast<size_t>(currscalar.end() - m_buf.begin());
    RYML_ASSERT(peeked_line.begins_with(' ', indentation));
    while(true)
    {
        _c4dbgpf("rscalar[IMPL]: continuing... ref_indentation=%zu", indentation);
        if(peeked_line.begins_with("...") || peeked_line.begins_with("---"))
        {
            _c4dbgpf("rscalar[IMPL]: document termination next -- bail now '%.*s'", _c4prsp(peeked_line.trimr("\r\n")));
            break;
        }
        else if(( ! peeked_line.begins_with(' ', indentation))) // is the line deindented?
        {
            if(!peeked_line.trim(" \r\n\t").empty()) // is the line not blank?
            {
                _c4dbgpf("rscalar[IMPL]: deindented line, not blank -- bail now '%.*s'", _c4prsp(peeked_line.trimr("\r\n")));
                break;
            }
            _c4dbgpf("rscalar[IMPL]: line is blank and has less indentation: ref=%zu line=%zu: '%.*s'", indentation, peeked_line.first_not_of(' ') == csubstr::npos ? 0 : peeked_line.first_not_of(' '), _c4prsp(peeked_line.trimr("\r\n")));
            _c4dbgpf("rscalar[IMPL]: ... searching for a line starting at indentation %zu", indentation);
            csubstr next_peeked = _scan_to_next_nonempty_line(indentation);
            if(next_peeked.empty())
            {
                _c4dbgp("rscalar[IMPL]: ... finished.");
                break;
            }
            _c4dbgp("rscalar[IMPL]: ... continuing.");
            peeked_line = next_peeked;
        }

        _c4dbgpf("rscalar[IMPL]: line contents: '%.*s'", _c4prsp(peeked_line.right_of(indentation, true).trimr("\r\n")));
        if(peeked_line.find(": ") != npos)
        {
            _line_progressed(peeked_line.find(": "));
            _c4err("': ' is not a valid token in plain flow (unquoted) scalars");
        }
        else if(peeked_line.ends_with(':'))
        {
            _line_progressed(peeked_line.find(':'));
            _c4err("lines cannot end with ':' in plain flow (unquoted) scalars");
        }
        else if(peeked_line.find(" #") != npos)
        {
            _line_progressed(peeked_line.find(" #"));
            _c4err("' #' is not a valid token in plain flow (unquoted) scalars");
        }

        _c4dbgpf("rscalar[IMPL]: append another line: (len=%zu)'%.*s'", peeked_line.len, _c4prsp(peeked_line.trimr("\r\n")));
        if(!_advance_to_peeked())
        {
            _c4dbgp("rscalar[IMPL]: file finishes after the scalar");
            break;
        }
        peeked_line = m_state->line_contents.rem;
    }
    RYML_ASSERT(m_state->pos.offset >= offs);
    substr full(m_buf.str + (currscalar.str - m_buf.str),
                currscalar.len + (m_state->pos.offset - offs));
    return full;
}

substr Parser::_scan_complex_key(csubstr currscalar, csubstr peeked_line)
{
    RYML_ASSERT(m_buf.is_super(currscalar));
    // NOTE. there's a problem with _scan_to_next_nonempty_line(), as it counts newlines twice
    // size_t offs = m_state->pos.offset;   // so we workaround by directly counting from the end of the given scalar
    RYML_ASSERT(currscalar.end() >= m_buf.begin());
    size_t offs = static_cast<size_t>(currscalar.end() - m_buf.begin());
    while(true)
    {
        _c4dbgp("rcplxkey: continuing...");
        if(peeked_line.begins_with("...") || peeked_line.begins_with("---"))
        {
            _c4dbgpf("rcplxkey: document termination next -- bail now '%.*s'", _c4prsp(peeked_line.trimr("\r\n")));
            break;
        }
        else
        {
            size_t pos = peeked_line.first_of("?:[]{}");
            if(pos == csubstr::npos)
            {
                pos = peeked_line.find("- ");
            }
            if(pos != csubstr::npos)
            {
                _c4dbgpf("rcplxkey: found special characters at pos=%zu: '%.*s'", pos, _c4prsp(peeked_line.trimr("\r\n")));
                _line_progressed(pos);
                break;
            }
        }

        _c4dbgpf("rcplxkey: no special chars found '%.*s'", _c4prsp(peeked_line.trimr("\r\n")));
        csubstr next_peeked = _scan_to_next_nonempty_line(0);
        if(next_peeked.empty())
        {
            _c4dbgp("rcplxkey: empty ... finished.");
            break;
        }
        _c4dbgp("rcplxkey: ... continuing.");
        peeked_line = next_peeked;

        _c4dbgpf("rcplxkey: line contents: '%.*s'", _c4prsp(peeked_line.trimr("\r\n")));
        if(peeked_line.find(": ") != npos)
        {
            _c4dbgp("rcplxkey: found ': ', stopping.");
            _line_progressed(peeked_line.find(": "));
            break;
        }
        else if(peeked_line.ends_with(':'))
        {
            _c4dbgp("rcplxkey: ends with ':', stopping.");
            _line_progressed(peeked_line.find(':'));
            break;
        }

        _c4dbgpf("rcplxkey: append another line: (len=%zu)'%.*s'", peeked_line.len, _c4prsp(peeked_line.trimr("\r\n")));
        if(!_advance_to_peeked())
        {
            _c4dbgp("rcplxkey: file finishes after the scalar");
            break;
        }
        peeked_line = m_state->line_contents.rem;
    }
    RYML_ASSERT(m_state->pos.offset >= offs);
    substr full(m_buf.str + (currscalar.str - m_buf.str),
                currscalar.len + (m_state->pos.offset - offs));
    return full;
}

//! scans to the next non-blank line starting with the given indentation
csubstr Parser::_scan_to_next_nonempty_line(size_t indentation)
{
    csubstr next_peeked;
    while(true)
    {
        _c4dbgpf("rscalar: ... curr offset: %zu indentation=%zu", m_state->pos.offset, indentation);
        next_peeked = _peek_next_line(m_state->pos.offset);
        _c4dbgpf("rscalar: ... next peeked line='%.*s'", _c4prsp(next_peeked.trimr("\r\n")));
        if(next_peeked.triml(' ').begins_with('#'))
        {
            ; // nothing to do
        }
        else if(next_peeked.begins_with(' ', indentation))
        {
            _c4dbgpf("rscalar: ... begins at same indentation %zu, assuming continuation", indentation);
            _advance_to_peeked();
            return next_peeked;
        }
        else   // check for de-indentation
        {
            csubstr trimmed = next_peeked.triml(' ').trimr("\t\r\n");
            _c4dbgpf("rscalar: ... deindented! trimmed='%.*s'", _c4prsp(trimmed));
            if(!trimmed.empty())
            {
                _c4dbgp("rscalar: ... and not empty. bailing out.");
                return {};
            }
        }
        if(!_advance_to_peeked())
        {
            _c4dbgp("rscalar: file finished");
            return {};
        }
    }
    return {};
}

// returns false when the file finished
bool Parser::_advance_to_peeked()
{
    _line_progressed(m_state->line_contents.rem.len);
    _line_ended(); // advances to the peeked-at line, consuming all remaining (probably newline) characters on the current line
    RYML_ASSERT(m_state->line_contents.rem.first_of("\r\n") == csubstr::npos);
    _c4dbgpf("advance to peeked: scan more... pos=%zu len=%zu", m_state->pos.offset, m_buf.len);
    _scan_line();  // puts the peeked-at line in the buffer
    if(_finished_file())
    {
        _c4dbgp("rscalar: finished file!");
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------

C4_ALWAYS_INLINE size_t _extend_from_combined_newline(char nl, char following)
{
    return (nl == '\n' && following == '\r') || (nl == '\r' && following == '\n');
}

//! look for the next newline chars, and jump to the right of those
csubstr from_next_line(csubstr rem)
{
    size_t nlpos = rem.first_of("\r\n");
    if(nlpos == csubstr::npos)
    {
        return {};
    }
    const char nl = rem[nlpos];
    rem = rem.right_of(nlpos);
    if(rem.empty())
    {
        return {};
    }
    if(_extend_from_combined_newline(nl, rem.front()))
    {
        rem = rem.sub(1);
    }
    return rem;
}

csubstr Parser::_peek_next_line(size_t pos) const
{
    csubstr rem{}; // declare here because of the goto
    size_t nlpos{}; // declare here because of the goto
    pos = pos == npos ? m_state->pos.offset : pos;
    if(pos >= m_buf.len)
    {
        goto next_is_empty;
    }

    // look for the next newline chars, and jump to the right of those
    rem = from_next_line(m_buf.sub(pos));
    if(rem.empty())
    {
        goto next_is_empty;
    }

    // now get everything up to and including the following newline chars
    nlpos = rem.first_of("\r\n");
    if((nlpos != csubstr::npos) && (nlpos + 1 < rem.len))
    {
        nlpos += _extend_from_combined_newline(rem[nlpos], rem[nlpos+1]);
    }
    rem = rem.left_of(nlpos, /*include_pos*/true);

    _c4dbgpf("peek next line @ %zu: (len=%zu)'%.*s'", pos, rem.len, _c4prsp(rem.trimr("\r\n")));
    return rem;
next_is_empty:
    _c4dbgpf("peek next line @ %zu: (len=0)''", pos);
    return {};
}

//-----------------------------------------------------------------------------
void Parser::_scan_line()
{
    if(m_state->pos.offset >= m_buf.len) return;

    char const* b = &m_buf[m_state->pos.offset];
    char const* e = b;

    // get the line stripped of newline chars
    while(e < m_buf.end() && (*e != '\n' && *e != '\r'))
    {
        ++e;
    }
    RYML_ASSERT(e >= b);
    csubstr stripped = m_buf.sub(m_state->pos.offset, static_cast<size_t>(e - b));

    // advance pos to include the first line ending
    if(e != m_buf.end() && *e == '\r') ++e;
    if(e != m_buf.end() && *e == '\n') ++e;
    RYML_ASSERT(e >= b);
    csubstr full = m_buf.sub(m_state->pos.offset, static_cast<size_t>(e - b));

    m_state->line_contents.reset(full, stripped);
}

//-----------------------------------------------------------------------------
void Parser::_line_progressed(size_t ahead)
{
    _c4dbgpf("line[%zu] (%zu cols) progressed by %zu:  col %zu --> %zu   offset %zu --> %zu", m_state->pos.line, m_state->line_contents.full.len, ahead, m_state->pos.col, m_state->pos.col+ahead, m_state->pos.offset, m_state->pos.offset+ahead);
    m_state->pos.offset += ahead;
    m_state->pos.col += ahead;
    RYML_ASSERT(m_state->pos.col <= m_state->line_contents.stripped.len+1);
    m_state->line_contents.rem = m_state->line_contents.rem.sub(ahead);
}

void Parser::_line_ended()
{
    _c4dbgpf("line[%zu] (%zu cols) ended! offset %zu --> %zu", m_state->pos.line, m_state->line_contents.full.len, m_state->pos.offset, m_state->pos.offset+m_state->line_contents.full.len - m_state->line_contents.stripped.len);
    RYML_ASSERT(m_state->pos.col == m_state->line_contents.stripped.len+1);
    m_state->pos.offset += m_state->line_contents.full.len - m_state->line_contents.stripped.len;
    ++m_state->pos.line;
    m_state->pos.col = 1;
}

//-----------------------------------------------------------------------------
void Parser::_set_indentation(size_t indentation)
{
    m_state->indref = indentation;
    _c4dbgpf("state[%zd]: saving indentation: %zd", m_state-m_stack.begin(), m_state->indref);
}
void Parser::_save_indentation(size_t behind)
{
    RYML_ASSERT(m_state->line_contents.rem.begin() >= m_state->line_contents.full.begin());
    m_state->indref = static_cast<size_t>(m_state->line_contents.rem.begin() - m_state->line_contents.full.begin());
    RYML_ASSERT(behind <= m_state->indref);
    m_state->indref -= behind;
    _c4dbgpf("state[%zd]: saving indentation: %zd", m_state-m_stack.begin(), m_state->indref);
}

//-----------------------------------------------------------------------------
void Parser::_write_key_anchor(size_t node_id)
{
    RYML_ASSERT(m_tree->has_key(node_id));
    if( ! m_key_anchor.empty())
    {
        _c4dbgpf("node=%zd: set key anchor to '%.*s'", node_id, _c4prsp(m_key_anchor));
        m_tree->set_key_anchor(node_id, m_key_anchor);
        m_key_anchor.clear();
    }
    else
    {
        csubstr r = m_tree->key(node_id);
        if(r.begins_with('*'))
        {
            _c4dbgpf("node=%zd: set key reference: '%.*s'", node_id, _c4prsp(r));
            m_tree->set_key_ref(node_id, r.sub(1));
        }
        else if(r == "<<")
        {
            _c4dbgpf("node=%zd: it's an inheriting reference", node_id);
            if(m_tree->is_seq(node_id))
            {
                _c4dbgpf("node=%zd: inheriting from seq of %zd", node_id, m_tree->num_children(node_id));
                for(size_t i = m_tree->first_child(node_id); i != NONE; i = m_tree->next_sibling(i))
                {
                    if( ! (m_tree->val(i).begins_with('*')))
                    {
                        _c4err("malformed reference: '%.*s'", _c4prsp(m_tree->val(i)));
                    }
                }
            }
            else if( ! (m_tree->val(node_id).begins_with('*') ))
            {
                 _c4err("malformed reference: '%.*s'", _c4prsp(m_tree->val(node_id)));
            }
            //m_tree->set_key_ref(node_id, r);
        }
    }
}

//-----------------------------------------------------------------------------
void Parser::_write_val_anchor(size_t node_id)
{
    if( ! m_val_anchor.empty())
    {
        _c4dbgpf("node=%zd: set val anchor to '%.*s'", node_id, _c4prsp(m_val_anchor));
        m_tree->set_val_anchor(node_id, m_val_anchor);
        m_val_anchor.clear();
    }
    csubstr r = m_tree->has_val(node_id) ? m_tree->val(node_id) : "";
    if(r.begins_with('*'))
    {
        _c4dbgpf("node=%zd: set val reference: '%.*s'", node_id, _c4prsp(r));
        m_tree->set_val_ref(node_id, r.sub(1));
    }
}

//-----------------------------------------------------------------------------
void Parser::_push_level(bool explicit_flow_chars)
{
    _c4dbgpf("pushing level! currnode=%zd  currlevel=%zd", m_state->node_id, m_state->level);
    RYML_ASSERT(m_state == &m_stack.top());
    if(node(m_state) == nullptr)
    {
        _c4dbgp("pushing level! actually no, current node is null");
        //RYML_ASSERT( ! explicit_flow_chars);
        return;
    }
    size_t st = RUNK;
    if(explicit_flow_chars || has_all(EXPL))
    {
        st |= EXPL;
    }
    m_stack.push(*m_state);
    m_state = &m_stack.top();
    set_flags(st);
    m_state->node_id = (size_t)NONE;
    m_state->indref = (size_t)NONE;
    ++m_state->level;
    _c4dbgpf("pushing level: now, currlevel=%zd", m_state->level);
}

void Parser::_pop_level()
{
    _c4dbgpf("popping level! currnode=%zd currlevel=%zd", m_state->node_id, m_state->level);
    if(has_any(RMAP) || node(m_state)->is_map())
    {
        _stop_map();
    }
    if(has_any(RSEQ) || node(m_state)->is_seq())
    {
        _stop_seq();
    }
    if(node(m_state)->is_doc())
    {
        _stop_doc();
    }
    RYML_ASSERT(m_stack.size() > 1);
    _prepare_pop();
    m_stack.pop();
    m_state = &m_stack.top();
    /*if(has_any(RMAP))
    {
        _toggle_key_val();
    }*/
    if(m_state->line_contents.indentation == 0)
    {
        //RYML_ASSERT(has_none(RTOP));
        add_flags(RTOP);
    }
    _c4dbgpf("popping level: now, currnode=%zd currlevel=%zd", m_state->node_id, m_state->level);
}

//-----------------------------------------------------------------------------
void Parser::_start_unk(bool /*as_child*/)
{
    _c4dbgp("start_unk");
    _push_level();
    _move_scalar_from_top();
}

//-----------------------------------------------------------------------------
void Parser::_start_doc(bool as_child)
{
    _c4dbgpf("start_doc (as child=%d)", as_child);
    RYML_ASSERT(node(m_stack.bottom()) == node(m_root_id));
    size_t parent_id = m_stack.size() < 2 ? m_root_id : m_stack.top(1).node_id;
    RYML_ASSERT(parent_id != NONE);
    RYML_ASSERT(m_tree->is_root(parent_id));
    RYML_ASSERT(node(m_state) == nullptr || node(m_state) == node(m_root_id));
    if(as_child)
    {
        _c4dbgpf("start_doc: parent=%zu", parent_id);
        if( ! m_tree->is_stream(parent_id))
        {
            for(size_t ch = m_tree->first_child(parent_id); ch != NONE; ch = m_tree->next_sibling(ch))
            {
                _c4dbgpf("start_doc: setting %zu->DOC", ch);
                m_tree->_add_flags(ch, DOC);
            }
            m_tree->_add_flags(parent_id, STREAM);
        }
        m_state->node_id = m_tree->append_child(parent_id);
        m_tree->to_doc(m_state->node_id);
    }
    else
    {
        RYML_ASSERT(m_tree->is_seq(parent_id) || m_tree->empty(parent_id));
        m_state->node_id = parent_id;
        if( ! m_tree->is_doc(parent_id))
        {
            m_tree->to_doc(parent_id, DOC);
        }
    }
    _c4dbgpf("start_doc: id=%zd", m_state->node_id);
    add_flags(RUNK|RTOP|NDOC);
    _handle_types();
    rem_flags(NDOC);
}

void Parser::_stop_doc()
{
    _c4dbgp("stop_doc");
    RYML_ASSERT(node(m_state)->is_doc());
}

void Parser::_end_stream()
{
    _c4dbgpf("end_stream, level=%zu node_id=%zu", m_state->level, m_state->node_id);
    RYML_ASSERT( ! m_stack.empty());
    NodeData *added = nullptr;
    if(has_any(SSCL))
    {
        if(m_tree->is_seq(m_state->node_id))
        {
            _c4dbgp("append val...");
            added = _append_val(_consume_scalar());
        }
        else if(m_tree->is_map(m_state->node_id))
        {
            _c4dbgp("append null key val...");
            added = _append_key_val_null();
            if(has_any(RSEQIMAP))
            {
                _stop_seqimap();
                _pop_level();
            }
        }
        else if(m_tree->type(m_state->node_id) == NOTYPE)
        {
            _c4dbgp("to seq...");
            m_tree->to_seq(m_state->node_id);
            added = _append_val(_consume_scalar());
        }
        else if(m_tree->is_doc(m_state->node_id))
        {
            _c4dbgp("to docval...");
            m_tree->to_val(m_state->node_id, _consume_scalar(), DOC);
            added = m_tree->get(m_state->node_id);
        }
        else
        {
            _c4err("internal error");
        }
    }
    else if(has_all(RSEQ|RVAL) && has_none(EXPL))
    {
        added = _append_val_null();
    }

    if(added)
    {
        size_t added_id = m_tree->id(added);
        if(m_tree->is_seq(m_state->node_id) || m_tree->is_doc(m_state->node_id))
        {
            if(!m_key_anchor.empty())
            {
                _c4dbgpf("node[%zu]: move key to val anchor: '%.*s'", added_id, _c4prsp(m_key_anchor));
                m_val_anchor = m_key_anchor;
                m_key_anchor = "";
            }
            if(!m_key_tag.empty())
            {
                _c4dbgpf("node[%zu]: move key to val tag: '%.*s'", added_id, _c4prsp(m_key_tag));
                m_val_tag = m_key_tag;
                m_key_tag = "";
            }
        }
        if(!m_key_anchor.empty())
        {
            _c4dbgpf("node[%zu]: set key anchor='%.*s'", m_tree->id(added), _c4prsp(m_key_anchor));
            m_tree->set_key_anchor(added_id, m_key_anchor);
            m_key_anchor = "";
        }
        if(!m_val_anchor.empty())
        {
            _c4dbgpf("node[%zu]: set val anchor='%.*s'", m_tree->id(added), _c4prsp(m_val_anchor));
            m_tree->set_val_anchor(added_id, m_val_anchor);
            m_val_anchor = "";
        }
        if(!m_key_tag.empty())
        {
            _c4dbgpf("node[%zu]: set key tag='%.*s'", m_tree->id(added), _c4prsp(m_key_tag));
            m_tree->set_key_tag(added_id, m_key_tag);
            m_key_tag = "";
        }
        if(!m_val_tag.empty())
        {
            _c4dbgpf("node[%zu]: set val tag='%.*s'", m_tree->id(added), _c4prsp(m_val_tag));
            m_tree->set_val_tag(added_id, m_val_tag);
            m_val_tag = "";
        }
    }

    while(m_stack.size() > 1)
    {
        _c4dbgpf("popping level: %zu (stack sz=%zu)", m_state->level, m_stack.size());
        RYML_ASSERT( ! has_any(SSCL, &m_stack.top()));
        _pop_level();
    }
    add_flags(NDOC);
}

void Parser::_start_new_doc(csubstr rem)
{
    _c4dbgp("_start_new_doc");
    RYML_ASSERT(rem.begins_with("---"));
    C4_UNUSED(rem);

    _end_stream();

    // start a document
    _c4dbgp("start a document");
    size_t indref = m_state->indref;
    _line_progressed(3);
    _push_level();
    _start_doc();
    _set_indentation(indref);
}

//-----------------------------------------------------------------------------
void Parser::_start_map(bool as_child)
{
    _c4dbgpf("start_map (as child=%d)", as_child);
    addrem_flags(RMAP|RVAL, RKEY|RUNK);
    RYML_ASSERT(node(m_stack.bottom()) == node(m_root_id));
    size_t parent_id = m_stack.size() < 2 ? m_root_id : m_stack.top(1).node_id;
    RYML_ASSERT(parent_id != NONE);
    RYML_ASSERT(node(m_state) == nullptr || node(m_state) == node(m_root_id));
    if(as_child)
    {
        m_state->node_id = m_tree->append_child(parent_id);
        if(has_all(SSCL))
        {
            csubstr key = _consume_scalar();
            m_tree->to_map(m_state->node_id, key);
            _c4dbgpf("start_map: id=%zd key='%.*s'", m_state->node_id, _c4prsp(node(m_state)->key()));
            _write_key_anchor(m_state->node_id);
        }
        else
        {
            m_tree->to_map(m_state->node_id);
            _c4dbgpf("start_map: id=%zd", m_state->node_id);
        }
        _write_val_anchor(m_state->node_id);
    }
    else
    {
        if(!(m_tree->is_map(parent_id) || m_tree->empty(parent_id)))
        {
            _c4err("parse error");
        }
        m_state->node_id = parent_id;
        _c4dbgpf("start_map: id=%zd", m_state->node_id);
        std::underlying_type<NodeType_e>::type as_doc = 0;
        if(node(m_state)->is_doc()) as_doc |= DOC;
        m_tree->to_map(parent_id, as_doc);
        _move_scalar_from_top();
        _write_val_anchor(parent_id);
        if(parent_id != NONE)
        {
            if(m_stack.size() >= 2)
            {
                State const& parent_state = m_stack.top(1);
                if(parent_state.flags & RSET)
                {
                    add_flags(RSET);
                }
            }
        }
    }
    if( ! m_val_tag.empty())
    {
        _c4dbgpf("start_map[%zu]: set val tag to '%.*s'", m_state->node_id, _c4prsp(m_val_tag));
        m_tree->set_val_tag(m_state->node_id, m_val_tag);
        m_val_tag.clear();
    }
}

void Parser::_stop_map()
{
    _c4dbgp("stop_map");
    RYML_ASSERT(node(m_state)->is_map());
}

//-----------------------------------------------------------------------------
void Parser::_start_seq(bool as_child)
{
    _c4dbgpf("start_seq (as child=%d)", as_child);
    if(has_all(RTOP|RUNK))
    {
        _c4dbgpf("start_seq: moving key tag to val tag: '%.*s'", _c4prsp(m_key_tag));
        m_val_tag = m_key_tag;
        m_key_tag.clear();
    }
    addrem_flags(RSEQ|RVAL, RUNK);
    RYML_ASSERT(node(m_stack.bottom()) == node(m_root_id));
    size_t parent_id = m_stack.size() < 2 ? m_root_id : m_stack.top(1).node_id;
    RYML_ASSERT(parent_id != NONE);
    RYML_ASSERT(node(m_state) == nullptr || node(m_state) == node(m_root_id));
    if(as_child)
    {
        m_state->node_id = m_tree->append_child(parent_id);
        if(has_all(SSCL))
        {
            RYML_ASSERT(node(parent_id)->is_map());
            csubstr name = _consume_scalar();
            m_tree->to_seq(m_state->node_id, name);
            _c4dbgpf("start_seq: id=%zd name='%.*s'", m_state->node_id, _c4prsp(node(m_state)->key()));
            _write_key_anchor(m_state->node_id);
        }
        else
        {
            std::underlying_type<NodeType_e>::type as_doc = 0;
            if(node(m_state)->is_doc()) as_doc |= DOC;
            m_tree->to_seq(m_state->node_id, as_doc);
            _c4dbgpf("start_seq: id=%zd%s", m_state->node_id, as_doc ? " as doc" : "");
        }
        _write_val_anchor(m_state->node_id);
    }
    else
    {
        RYML_ASSERT(m_tree->is_seq(parent_id) || m_tree->empty(parent_id));
        m_state->node_id = parent_id;
        std::underlying_type<NodeType_e>::type as_doc = 0;
        if(node(m_state)->is_doc()) as_doc |= DOC;
        m_tree->to_seq(parent_id, as_doc);
        _move_scalar_from_top();
        _c4dbgpf("start_seq: id=%zd%s", m_state->node_id, as_doc ? " as_doc" : "");
        _write_val_anchor(parent_id);
    }
    if( ! m_val_tag.empty())
    {
        _c4dbgpf("start_seq: set val tag to '%.*s'", _c4prsp(m_val_tag));
        m_tree->set_val_tag(m_state->node_id, m_val_tag);
        m_val_tag.clear();
    }
}

void Parser::_stop_seq()
{
    _c4dbgp("stop_seq");
    RYML_ASSERT(node(m_state)->is_seq());
}

//-----------------------------------------------------------------------------
void Parser::_start_seqimap()
{
    _c4dbgpf("start_seqimap at node=%zu. has_children=%d", m_state->node_id, m_tree->has_children(m_state->node_id));
    RYML_ASSERT(has_all(RSEQ|EXPL));
    // create a map, and turn the last scalar of this sequence
    // into the key of the map's first child.
    //
    // Yep, YAML is crazy.
    if(m_tree->has_children(m_state->node_id) && m_tree->has_val(m_tree->last_child(m_state->node_id)))
    {
        auto prev = m_tree->last_child(m_state->node_id);
        _c4dbgpf("has children and last child=%zu has val. saving the scalars, val='%.*s', val='%.*s'", prev, _c4prsp(m_tree->val(prev)), _c4prsp(m_tree->valsc(prev).scalar));
        NodeScalar tmp = m_tree->valsc(prev);
        m_tree->remove(prev);
        _push_level();
        _start_map();
        _store_scalar(tmp.scalar);
        m_key_anchor = tmp.anchor;
        m_key_tag = tmp.tag;
    }
    else
    {
        _c4dbgpf("node %zu has no children yet, using empty key", m_state->node_id);
        _push_level();
        _start_map();
        _store_scalar("");
    }
    add_flags(RSEQIMAP|EXPL);
}

void Parser::_stop_seqimap()
{
    _c4dbgp("stop_seqimap");
    RYML_ASSERT(has_all(RSEQIMAP));
}

//-----------------------------------------------------------------------------
NodeData* Parser::_append_val(csubstr val)
{
    RYML_ASSERT( ! has_all(SSCL));
    RYML_ASSERT(node(m_state) != nullptr);
    RYML_ASSERT(node(m_state)->is_seq());
    _c4dbgpf("append val: '%.*s' to parent id=%zd (level=%zd)", _c4prsp(val), m_state->node_id, m_state->level);
    size_t nid = m_tree->append_child(m_state->node_id);
    m_tree->to_val(nid, val);
    _c4dbgpf("append val: id=%zd key='%.*s' val='%.*s'", nid, _c4prsp(m_tree->get(nid)->m_key.scalar), _c4prsp(m_tree->get(nid)->m_val.scalar));
    if( ! m_val_tag.empty())
    {
        _c4dbgpf("append val: set tag to '%.*s'", _c4prsp(m_val_tag));
        m_tree->set_val_tag(nid, m_val_tag);
        m_val_tag.clear();
    }
    _write_val_anchor(nid);
    return m_tree->get(nid);
}

NodeData* Parser::_append_key_val(csubstr val)
{
    RYML_ASSERT(node(m_state)->is_map());
    csubstr key = _consume_scalar();
    _c4dbgpf("append keyval: '%.*s' '%.*s' to parent id=%zd (level=%zd)", _c4prsp(key), _c4prsp(val), m_state->node_id, m_state->level);
    size_t nid = m_tree->append_child(m_state->node_id);
    m_tree->to_keyval(nid, key, val);
    _c4dbgpf("append keyval: id=%zd key='%.*s' val='%.*s'", nid, _c4prsp(m_tree->get(nid)->key()), _c4prsp(m_tree->get(nid)->val()));
    if( ! m_key_tag.empty())
    {
        _c4dbgpf("append keyval: set key tag to '%.*s'", _c4prsp(m_key_tag));
        m_tree->set_key_tag(nid, m_key_tag);
        m_key_tag.clear();
    }
    if( ! m_val_tag.empty())
    {
        _c4dbgpf("append keyval: set val tag to '%.*s'", _c4prsp(m_val_tag));
        m_tree->set_val_tag(nid, m_val_tag);
        m_val_tag.clear();
    }
    _write_key_anchor(nid);
    _write_val_anchor(nid);
    return m_tree->get(nid);
}

//-----------------------------------------------------------------------------
void Parser::_store_scalar(csubstr const& s)
{
    _c4dbgpf("state[%zd]: storing scalar '%.*s' (flag: %zd) (old scalar='%.*s')", m_state-m_stack.begin(), _c4prsp(s), m_state->flags & SSCL, _c4prsp(m_state->scalar));
    RYML_ASSERT(has_none(SSCL));
    add_flags(SSCL);
    m_state->scalar = s;
}

csubstr Parser::_consume_scalar()
{
    _c4dbgpf("state[%zd]: consuming scalar '%.*s' (flag: %zd))", m_state-m_stack.begin(), _c4prsp(m_state->scalar), m_state->flags & SSCL);
    RYML_ASSERT(m_state->flags & SSCL);
    csubstr s = m_state->scalar;
    rem_flags(SSCL);
    m_state->scalar.clear();
    return s;
}

void Parser::_move_scalar_from_top()
{
    if(m_stack.size() < 2) return;
    State &prev = m_stack.top(1);
    RYML_ASSERT(m_state == &m_stack.top());
    RYML_ASSERT(m_state != &prev);
    if(prev.flags & SSCL)
    {
        _c4dbgpf("moving scalar '%.*s' from state[%zd] to state[%zd] (overwriting '%.*s')", _c4prsp(prev.scalar), &prev-m_stack.begin(), m_state-m_stack.begin(), _c4prsp(m_state->scalar));
        add_flags(prev.flags & SSCL);
        m_state->scalar = prev.scalar;
        rem_flags(SSCL, &prev);
        prev.scalar.clear();
    }
}

//-----------------------------------------------------------------------------
bool Parser::_handle_indentation()
{
    RYML_ASSERT(has_none(EXPL));
    if( ! _at_line_begin()) return false;

    size_t ind = m_state->line_contents.indentation;
    csubstr rem = m_state->line_contents.rem;
    /** @todo instead of trimming, we should use the indentation index from above */
    csubstr remt = rem.triml(' ');

    if(remt.empty() || remt.begins_with('#')) // this is a blank or comment line
    {
        _line_progressed(rem.size());
        return true;
    }

    if(ind == m_state->indref)
    {
        if(has_all(SSCL|RVAL) && ! rem.sub(ind).begins_with('-'))
        {
            if(has_all(RMAP))
            {
                _append_key_val_null();
                addrem_flags(RKEY, RVAL);
            }
            else if(has_all(RSEQ))
            {
                _append_val(_consume_scalar());
                addrem_flags(RNXT, RVAL);
            }
            else
            {
                _c4err("internal error");
            }
        }
        else if(has_all(RSEQ|RNXT) && ! rem.sub(ind).begins_with('-'))
        {
            if(m_stack.size() > 2) // do not pop to root level
            {
                _c4dbgp("end the indentless seq");
                _pop_level();
                return true;
            }
        }
        else
        {
            _c4dbgpf("same indentation (%zd) -- nothing to see here", ind);
        }
        _line_progressed(ind);
        return ind > 0;
    }
    else if(ind < m_state->indref)
    {
        _c4dbgpf("smaller indentation (%zd < %zd)!!!", ind, m_state->indref);
        if(has_all(RVAL))
        {
            _c4dbgp("there was an empty val -- appending");
            if(has_all(RMAP))
            {
                RYML_ASSERT(has_all(SSCL));
                _append_key_val_null();
            }
            else if(has_all(RSEQ))
            {
                RYML_ASSERT(has_none(SSCL));
                _append_val_null();
            }
        }
        // search the stack frame to jump to based on its indentation
        State const* popto = nullptr;
        RYML_ASSERT(m_stack.is_contiguous()); // this search relies on the stack being contiguous
        for(State const* s = m_state-1; s >= m_stack.begin(); --s)
        {
            _c4dbgpf("searching for state with indentation %zu. curr=%zu (level=%zu,node=%zu)", ind, s->indref, s->level, s->node_id);
            if(s->indref == ind)
            {
                _c4dbgpf("gotit!!! level=%zu node=%zu", s->level, s->node_id);
                popto = s;
                // while it may be tempting to think we're done at this
                // point, we must still determine whether we're jumping to a
                // parent with the same indentation. Consider this case with
                // an indentless sequence:
                //
                // product:
                // - sku: BL394D
                //   quantity: 4
                //   description: Basketball
                //   price: 450.00
                // - sku: BL4438H
                //   quantity: 1
                //   description: Super Hoop
                //   price: 2392.00  # jumping one level here would be wrong.
                // tax: 1234.5       # we must jump two levels
                if(popto > m_stack.begin())
                {
                    auto parent = popto - 1;
                    if(parent->indref == popto->indref)
                    {
                        _c4dbgpf("the parent (level=%zu,node=%zu) has the same indentation (%zu). is this in an indentless sequence?",
                                 parent->level, parent->node_id, popto->indref);
                        _c4dbgpf("isseq(popto)=%d ismap(parent)=%d", m_tree->is_seq(popto->node_id), m_tree->is_map(parent->node_id));
                        if(m_tree->is_seq(popto->node_id) && m_tree->is_map(parent->node_id))
                        {
                            if( ! remt.begins_with('-'))
                            {
                                _c4dbgp("this is an indentless sequence");
                                popto = parent;
                            }
                            else
                            {
                                _c4dbgp("not an indentless sequence");
                            }
                        }
                    }
                }
                break;
            }
        }
        if(!popto || popto >= m_state || popto->level >= m_state->level)
        {
            _c4err("parse error: incorrect indentation?");
        }
        _c4dbgpf("popping %zd levels: from level %zd to level %zd", m_state->level-popto->level, m_state->level, popto->level);
        while(m_state != popto)
        {
            _c4dbgpf("popping level %zd (indentation=%zd)", m_state->level, m_state->indref);
            _pop_level();
        }
        RYML_ASSERT(ind == m_state->indref);
        _line_progressed(ind);
        return true;
    }
    else
    {
        _c4dbgpf("larger indentation (%zd > %zd)!!!", ind, m_state->indref);
        RYML_ASSERT(ind > m_state->indref);
        if(has_all(RMAP|RVAL))
        {
            if(_is_scalar_next__rmap_val(remt) && remt.first_of(":?") == npos)
            {
                _c4dbgpf("actually it seems a value: '%.*s'", _c4prsp(remt));
            }
            else
            {
                addrem_flags(RKEY, RVAL);
                _start_unk();
                //_move_scalar_from_top();
                _line_progressed(ind);
                _save_indentation();
                return true;
            }
        }
        else if(has_all(RSEQ|RVAL))
        {
            // nothing to do here
        }
        else if(rem.triml(' ').begins_with("#"))
        {
            C4_NEVER_REACH(); // this should have been handled earlier
        }
        else
        {
            _c4err("parse error - indentation should not increase at this point");
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
csubstr Parser::_scan_comment()
{
    csubstr s = m_state->line_contents.rem;
    RYML_ASSERT(s.begins_with('#'));
    _line_progressed(s.len);
    // skip the # character
    s = s.sub(1);
    // skip leading whitespace
    s = s.right_of(s.first_not_of(' '), /*include_pos*/true);
    _c4dbgpf("comment was '%.*s'", _c4prsp(s));
    return s;
}

//-----------------------------------------------------------------------------
csubstr Parser::_scan_quoted_scalar(const char q)
{
    RYML_ASSERT(q == '\'' || q == '"');

    // quoted scalars can spread over multiple lines!
    // nice explanation here: http://yaml-multiline.info/

    bool needs_filter = false;

    // a span to the end of the file
    size_t b = m_state->pos.offset;
    substr s = m_buf.sub(b);
    if(s.begins_with(' '))
    {
        s = s.triml(' ');
        RYML_ASSERT(m_buf.sub(b).is_super(s));
        RYML_ASSERT(s.begin() >= m_buf.sub(b).begin());
        _line_progressed((size_t)(s.begin() - m_buf.sub(b).begin()));
    }
    b = m_state->pos.offset; // take this into account
    RYML_ASSERT(s.begins_with(q));

    // skip the opening quote
    _line_progressed(1);
    s = s.sub(1);

    // find the pos of the matching quote
    size_t pos = npos;
    while( ! _finished_file())
    {
        const csubstr line = m_state->line_contents.rem;
        bool line_is_blank = true;

        if(q == '\'') // scalars with single quotes
        {
            _c4dbgpf("scanning single quoted scalar @ line[%zd]:  line=\"%.*s\"", m_state->pos.line, _c4prsp(line));
            for(size_t i = 0; i < line.len; ++i)
            {
                const char curr = line.str[i];
                if(curr == '\'') // single quotes are escaped with two single quotes
                {
                    const char next = i+1 < line.len ? line.str[i+1] : '~';
                    if(next != '\'') // so just look for the first quote
                    {                // without another after it
                        pos = i;
                        break;
                    }
                    else
                    {
                        needs_filter = true; // needs filter to remove escaped quotes
                        ++i; // skip the escaped quote
                    }
                }
                else if(curr != ' ')
                {
                    line_is_blank = false;
                }
            }
        }
        else // scalars with double quotes
        {
            _c4dbgpf("scanning double quoted scalar @ line[%zd]:  line='%.*s'", m_state->pos.line, _c4prsp(line));
            for(size_t i = 0; i < line.len; ++i)
            {
                const char curr = line.str[i];
                if(curr != ' ')
                {
                    line_is_blank = false;
                }
                // every \ is an escape
                if(curr == '\\')
                {
                    const char next = i+1 < line.len ? line.str[i+1] : '~';
                    needs_filter = true;
                    if(next == '"' || next == '\\')
                    {
                        ++i;
                    }
                }
                else if(curr == '"')
                {
                    pos = i;
                    break;
                }
            }
        }

        // leading whitespace also needs filtering
        needs_filter = needs_filter
            || line_is_blank
            || (_at_line_begin() && line.begins_with(' '))
            || (m_state->line_contents.full.last_of('\r') != csubstr::npos);

        if(pos == npos)
        {
            _line_progressed(line.len);
            _c4dbgpf("scanning scalar @ line[%zd]: sofar=\"%.*s\"", m_state->pos.line, _c4prsp(s.sub(0, m_state->pos.offset-b)));
        }
        else
        {
            RYML_ASSERT(pos >= 0 && pos < m_buf.len);
            RYML_ASSERT(m_buf[m_state->pos.offset + pos] == q);
            _line_progressed(pos + 1); // progress beyond the quote
            pos = m_state->pos.offset - b - 1; // but we stop before it
            break;
        }

        _line_ended();
        _scan_line();
    }

    if(pos == npos)
    {
        _c4err("reached end of file while looking for closing quote");
    }
    else if(pos == 0)
    {
        s.clear();
        RYML_ASSERT( ! needs_filter);
    }
    else
    {
        RYML_ASSERT(s.end() >= m_buf.begin() && s.end() <= m_buf.end());
        RYML_ASSERT(s.end() == m_buf.end() || *s.end() == q);
        s = s.sub(0, pos-1);
    }

    if(needs_filter)
    {
        csubstr ret;
        if(q == '\'')
        {
            ret = _filter_squot_scalar(s);
        }
        else if(q == '"')
        {
            ret = _filter_dquot_scalar(s);
        }
        RYML_ASSERT(ret.len <= s.len || s.empty() || s.trim(' ').empty());
        _c4dbgpf("final scalar: \"%.*s\"", _c4prsp(ret));
        return ret;
    }

    _c4dbgpf("final scalar: \"%.*s\"", _c4prsp(s));

    return s;
}

//-----------------------------------------------------------------------------
csubstr Parser::_scan_block()
{
    // nice explanation here: http://yaml-multiline.info/
    csubstr s = m_state->line_contents.rem;
    csubstr trimmed = s.triml(" ");
    if(trimmed.str > s.str)
    {
        _c4dbgp("skipping whitespace");
        RYML_ASSERT(trimmed.str >= s.str);
        _line_progressed(static_cast<size_t>(trimmed.str - s.str));
        s = trimmed;
    }
    RYML_ASSERT(s.begins_with('|') || s.begins_with('>'));

    _c4dbgpf("scanning block: specs=\"%.*s\"", _c4prsp(s));

    // parse the spec
    BlockStyle_e newline = s.begins_with('>') ? BLOCK_FOLD : BLOCK_LITERAL;
    BlockChomp_e chomp = CHOMP_CLIP; // default to clip unless + or - are used
    size_t indentation = npos; // have to find out if no spec is given
    csubstr digits;
    if(s.len > 1)
    {
        csubstr t = s.sub(1);
        RYML_ASSERT(t.len >= 1);
        if(t[0] == '-')
        {
            chomp = CHOMP_STRIP;
            t = t.sub(1);
        }
        else if(t[0] == '+')
        {
            chomp = CHOMP_KEEP;
            t = t.sub(1);
        }

        // from here to the end, only digits are considered
        digits = t.left_of(t.first_not_of("0123456789"));
        if( ! digits.empty())
        {
            if( ! _read_decimal(digits, &indentation))
            {
                _c4err("parse error: could not read decimal");
            }
        }
    }

    // finish the current line
    _line_progressed(s.len);
    _line_ended();
    _scan_line();

    if(indentation == npos)
    {
        indentation = m_state->line_contents.indentation;
    }

    _c4dbgpf("scanning block:  style=%s", newline==BLOCK_FOLD ? "fold" : "literal");
    _c4dbgpf("scanning block:  chomp=%s", chomp==CHOMP_CLIP ? "clip" : (chomp==CHOMP_STRIP ? "strip" : "keep"));
    _c4dbgpf("scanning block: indent=%zd (digits='%.*s')", indentation, _c4prsp(digits));

    // start with a zero-length block, already pointing at the right place
    substr raw_block(m_buf.data() + m_state->pos.offset, size_t(0));// m_state->line_contents.full.sub(0, 0);
    RYML_ASSERT(raw_block.begin() == m_state->line_contents.full.begin());

    // read every full line into a raw block,
    // from which newlines are to be stripped as needed
    size_t num_lines = 0, first = m_state->pos.line;
    auto &lc = m_state->line_contents;
    while(( ! _finished_file()))
    {
        _scan_line();
        if(lc.indentation < indentation)
        {
            // stop when the line is deindented and not empty
            if( ! lc.rem.trim(" \t\r\n").empty())
            {
                break;
            }
        }
        raw_block.len += m_state->line_contents.full.len;
        _c4dbgpf("scanning block: append '%.*s'", _c4prsp(m_state->line_contents.rem));
        _line_progressed(m_state->line_contents.rem.len);
        _line_ended();
        ++num_lines;
    }
    RYML_ASSERT(m_state->pos.line == (first + num_lines));
    C4_UNUSED(num_lines);
    C4_UNUSED(first);

    _c4dbgpf("scanning block: raw='%.*s'", _c4prsp(raw_block));

    // ok! now we strip the newlines and spaces according to the specs
    s = _filter_block_scalar(raw_block, newline, chomp, indentation);

    _c4dbgpf("scanning block: final='%.*s'", _c4prsp(s));

    return s;
}

//-----------------------------------------------------------------------------
csubstr Parser::_filter_plain_scalar(substr s, size_t indentation)
{
    _c4dbgpf("filtering plain scalar: indentation=%zu before='%.*s'", indentation, _c4prsp(s));

    // do a first sweep to clean leading whitespace from the indentation
    substr r = _filter_whitespace(s, indentation);

    // now another sweep for newlines
    for(size_t i = 0; i < r.len; ++i)
    {
        const char curr = r[i];
        const char next = i+1 < r.len ? r[i+1] : '\0';
        RYML_ASSERT(curr != '\r' && next != '\r');
        if(curr == '\n')
        {
            _c4dbgpf("filtering plain scalar: i=%zu: looked at: '%.*s'", i, _c4prsp(r.first(i)));
            if(next != '\n')
            {
                _c4dbgpf("filtering plain scalar: filter single newline at %zu", i);
                if(i + 1 < r.len)
                {
                    r[i] = ' '; // a single unix newline: turn it into a space
                }
                else
                {
                    --r.len;
                }
            }
            else
            {
                 // multiple new lines
                RYML_ASSERT(next == '\n');
                r = r.erase(i, 1);  // erase one
                RYML_ASSERT(r[i] == '\n');
                size_t nl = r.sub(i).first_not_of('\n');
                if(nl == csubstr::npos)
                {
                    _c4dbgpf("filtering plain scalar: newlines starting at %zu go up to the end at %zu", i, r.len);
                    break;
                }
                _c4dbgpf("filtering plain scalar: erasing one of %zu newlines found at %zu", nl, i);
                RYML_ASSERT(nl > 0);
                i += nl; // and skip the rest
            }
        }
        else if(curr == '\r')
        {
            _c4dbgpf("filtering plain scalar: i=%zu: removing carriage return \\r", i);
            r = r.erase(i, 1);
        }
    }

    RYML_ASSERT(s.len >= r.len);
    _c4dbgpf("filtering plain scalar: num filtered chars=%zd", s.len - r.len);
    _c4dbgpf("filtering plain scalar: after='%.*s'", _c4prsp(r));

#ifdef RYML_DBG
    for(size_t i = r.len; i < s.len; ++i)
    {
        s[i] = '~';
    }
#endif

    return r;
}

//-----------------------------------------------------------------------------
csubstr Parser::_filter_squot_scalar(substr s)
{
    _c4dbgpf("filtering single-quoted scalar: before=\"%.*s\"", _c4prsp(s));

    // do a first sweep to clean leading whitespace
    substr r = _filter_whitespace(s);

    // now another sweep for quotes and newlines
    for(size_t i = 0; i < r.len; ++i)
    {
        const char curr = r[i];
        //const char prev = i   > 0     ? r[i-1] : '\0';
        const char next = i+1 < r.len ? r[i+1] : '\0';
        if(curr == '\'' && (curr == next))
        {
            r = r.erase(i+1, 1); // turn two consecutive single quotes into one
        }
        else if(curr == '\n')
        {
            if(next != '\n')
            {
                r[i] = ' '; // a single unix newline: turn it into a space
            }
            else if(curr == '\n' && next == '\n')
            {
                r = r.erase(i+1, 1); // keep only one of consecutive newlines
            }
        }
    }

    RYML_ASSERT(s.len >= r.len);
    _c4dbgpf("filtering single-quoted scalar: num filtered chars=%zd", s.len - r.len);
    _c4dbgpf("filtering single-quoted scalar: after=\"%.*s\"", _c4prsp(r));

#ifdef RYML_DBG
    for(size_t i = r.len; i < s.len; ++i)
    {
        s[i] = '~';
    }
#endif

    return r;
}

//-----------------------------------------------------------------------------
csubstr Parser::_filter_dquot_scalar(substr s)
{
    _c4dbgpf("filtering double-quoted scalar: before='%.*s'", _c4prsp(s));

    // do a first sweep to clean leading whitespace
    substr r = _filter_whitespace(s);

    for(size_t i = 0; i < r.len; ++i)
    {
        const char curr = r[i];
        //const char prev = i   > 0     ? r[i-1] : '\0';
        const char next = i+1 < r.len ? r[i+1] : '\0';
        if(curr == '\\')
        {
            if(next == curr)
            {
                r = r.erase(i+1, 1); // turn two consecutive backslashes into one
            }
            else if(next == '\n')
            {
                r = r.erase(i, 2);  // newlines are escaped with \ -- delete both
            }
            else if(next == '"')
            {
                r = r.erase(i, 1);  // fix escaped double quotes
            }
            else if(next == 'n')
            {
                r = r.erase(i+1, 1);
                r[i] = '\n';
            }
        }
        else if(curr == '\n')
        {
            if(next != '\n')
            {
                r[i] = ' '; // a single unix newline: turn it into a space
            }
            else if(curr == '\n' && next == '\n')
            {
                r = r.erase(i+1, 1); // keep only one of consecutive newlines
            }
        }
    }

    RYML_ASSERT(s.len >= r.len);
    _c4dbgpf("filtering double-quoted scalar: num filtered chars=%zd", s.len - r.len);
    _c4dbgpf("filtering double-quoted scalar: after='%.*s'", _c4prsp(r));

#ifdef RYML_DBG
    for(size_t i = r.len; i < s.len; ++i)
    {
        s[i] = '~';
    }
#endif

    return r;
}

//-----------------------------------------------------------------------------
/** @p leading_whitespace when true, remove every leading spaces from the
 * beginning of each line */
substr Parser::_filter_whitespace(substr r, size_t indentation, bool leading_whitespace)
{
    _c4dbgpf("filtering whitespace: indentation=%zu leading=%d before=\"%.*s\"", indentation, leading_whitespace, _c4prsp(r));

    for(size_t i = 0; i < r.len; ++i)
    {
        const char curr = r[i];
        const char prev = i   > 0     ? r[i-1] : '\0';
        if(curr == ' ' && prev == '\n')
        {
            _c4dbgpf("filtering whitespace: removing indentation i=%zu len=%zu. curr=~%.*s~", i, r.len, _c4prsp(r.first(i)));
            csubstr ss = r.sub(i);
            ss = ss.left_of(ss.first_not_of(' '));
            RYML_ASSERT(ss.len >= 1);
            size_t num = ss.len;
            // RYML_ASSERT(num >= indentation); // empty lines are allowed
            _c4dbgpf("                    : line has %zu spaces", num);
            if(leading_whitespace)
            {
                num = ss.len;
            }
            else if(indentation != csubstr::npos)
            {
                num = num < indentation ? num : indentation;
            }
            _c4dbgpf("                    : removing %zu spaces", num);
            r = r.erase(i, num);
            if(i < r.len && r[i] != ' ') --i; // i is incremented on the next iteration
        }
        // erase \r --- https://stackoverflow.com/questions/1885900
        else if(curr == '\r')
        {
            _c4dbgpf("filtering whitespace: remove \\r: i=%zu len=%zu. curr=~%.*s~", i, r.len, _c4prsp(r.first(i)));
            r = r.erase(i, 1);
            --i; // i is incremented on the next iteration
        }
    }

    _c4dbgpf("filtering whitespace: after=\"%.*s\"", _c4prsp(r));

    return r;
}

//-----------------------------------------------------------------------------
csubstr Parser::_filter_block_scalar(substr s, BlockStyle_e style, BlockChomp_e chomp, size_t indentation)
{
    _c4dbgpf("filtering block: '%.*s'", _c4prsp(s));

    substr r = s;

    r = _filter_whitespace(s, indentation, /*leading whitespace*/false);
    if(r.begins_with(' ', indentation))
    {
        r = r.erase(0, indentation);
    }

    _c4dbgpf("filtering block: after whitespace='%.*s'", _c4prsp(r));

    RYML_ASSERT(r.find('\r') == csubstr::npos); // filter whitespace must remove this

    switch(chomp)
    {
    case CHOMP_KEEP: // nothing to do, keep everything
        _c4dbgp("filtering block: chomp=KEEP (+)");
        break;
    case CHOMP_STRIP: // strip all newlines from the end
    {
        _c4dbgp("filtering block: chomp=STRIP (-)");
        auto pos = r.last_not_of("\n");
        if(pos != npos)
        {
            r = r.left_of(pos, /*include_pos*/true);
        }
        break;
    }
    case CHOMP_CLIP: // clip to a single newline
    {
        _c4dbgp("filtering block: chomp=CLIP");
        auto pos = r.last_not_of("\n");
        if(pos != npos && pos+1 < r.len)
        {
            ++pos;
            r = r.left_of(pos, /*include_pos*/true);
        }
        break;
    }
    default:
        _c4err("unknown chomp style");
    }

    _c4dbgpf("filtering block: after chomp='%.*s'", _c4prsp(r));

    switch(style)
    {
    case BLOCK_LITERAL:
        break;
    case BLOCK_FOLD:
        {
            auto pos = r.last_not_of('\n'); // do not fold trailing newlines
            if((pos != npos) && (pos < r.len))
            {
                ++pos; // point pos at the first newline char
                substr t = r.sub(0, pos);
                for(size_t i = 0; i < t.len; ++i)
                {
                    const char curr = t[i];
                    if(curr != '\n') continue;
                    size_t nextl = t.first_not_of('\n', i+1);
                    if(nextl == i+1)
                    {
                        _c4dbgpf("filtering block[fold]: %zu: single newline, replace with space. curr=~%.*s~", i, _c4prsp(r.first(i)));
                        t[i] = ' ';
                    }
                    else if(nextl != csubstr::npos)
                    {
                        _c4dbgpf("filtering block[fold]: %zu: %zu newlines, remove first. curr=~%.*s~", i, nextl-1, _c4prsp(r.first(i)));
                        RYML_ASSERT(nextl >= 1);
                        t = t.erase(i, 1);
                        i = nextl-1;
                        if(i) --i;
                    }
                    else
                    {
                        _c4err("crl");
                        break;
                    }
                }
                // copy over the trailing newlines
                substr nl = r.sub(pos);
                RYML_ASSERT(t.len + nl.len <= r.len);
                for(size_t i = 0; i < nl.len; ++i)
                {
                    r[t.len + i] = nl[i];
                }
                // now trim r
                r = r.sub(0, t.len + nl.len);
            }
        }
        break;
    default:
        _c4err("unknown block style");
    }

    _c4dbgpf("filtering block: final='%.*s'", _c4prsp(r));

#ifdef RYML_DBG
    for(size_t i = r.len; i < s.len; ++i)
    {
        s[i] = '~';
    }
#endif

    return r;
}

//-----------------------------------------------------------------------------
bool Parser::_read_decimal(csubstr const& str, size_t *decimal)
{
    RYML_ASSERT(str.len >= 1);
    size_t n = 0;
    for(size_t i = 0; i < str.len; ++i)
    {
        char c = str.str[i];
        if(c < '0' || c > '9') return false;
        n = n*10 + size_t(c-'0');
    }
    *decimal = n;
    return true;
}

//-----------------------------------------------------------------------------
size_t Parser::_count_nlines(csubstr src)
{
    size_t n = (src.len > 0);
    while(src.len > 0)
    {
        n += (src.begins_with('\n') || src.begins_with('\r'));
        src = src.sub(1);
    }
    return n;
}

//-----------------------------------------------------------------------------
void Parser::set_flags(size_t f, State * s)
{
#ifdef RYML_DBG
    char buf1[64], buf2[64];
    int len1 = _prfl(buf1, sizeof(buf1), f);
    int len2 = _prfl(buf2, sizeof(buf2), s->flags);
    _c4dbgpf("state[%zd]: setting flags to %.*s: before=%.*s", s-m_stack.begin(), len1, buf1, len2, buf2);
#endif
    s->flags = f;
}

void Parser::add_flags(size_t on, State * s)
{
#ifdef RYML_DBG
    char buf1[64], buf2[64], buf3[64];
    int len1 = _prfl(buf1, sizeof(buf1), on);
    int len2 = _prfl(buf2, sizeof(buf2), s->flags);
    int len3 = _prfl(buf3, sizeof(buf3), s->flags|on);
    _c4dbgpf("state[%zd]: adding flags %.*s: before=%.*s after=%.*s", s-m_stack.begin(), len1, buf1, len2, buf2, len3, buf3);
#endif
    s->flags |= on;
}

void Parser::addrem_flags(size_t on, size_t off, State * s)
{
#ifdef RYML_DBG
    char buf1[64], buf2[64], buf3[64], buf4[64];
    int len1 = _prfl(buf1, sizeof(buf1), on);
    int len2 = _prfl(buf2, sizeof(buf2), off);
    int len3 = _prfl(buf3, sizeof(buf3), s->flags);
    int len4 = _prfl(buf4, sizeof(buf4), ((s->flags|on)&(~off)));
    _c4dbgpf("state[%zd]: adding flags %.*s / removing flags %.*s: before=%.*s after=%.*s", s-m_stack.begin(), len1, buf1, len2, buf2, len3, buf3, len4, buf4);
#endif
    s->flags |= on;
    s->flags &= ~off;
}

void Parser::rem_flags(size_t off, State * s)
{
#ifdef RYML_DBG
    char buf1[64], buf2[64], buf3[64];
    int len1 = _prfl(buf1, sizeof(buf1), off);
    int len2 = _prfl(buf2, sizeof(buf2), s->flags);
    int len3 = _prfl(buf3, sizeof(buf3), s->flags&(~off));
    _c4dbgpf("state[%zd]: removing flags %.*s: before=%.*s after=%.*s", s-m_stack.begin(), len1, buf1, len2, buf2, len3, buf3);
#endif
    s->flags &= ~off;
}

//-----------------------------------------------------------------------------
void Parser::_err(const char *fmt, ...) const
{
#ifndef RYML_ERRMSG_SIZE
    #define RYML_ERRMSG_SIZE 1024
#endif
    char errmsg[RYML_ERRMSG_SIZE];
    int len = sizeof(errmsg);

    va_list args;
    va_start(args, fmt);
    len = _fmt_msg(errmsg, len, fmt, args);
    va_end(args);
    c4::yml::error(errmsg, static_cast<size_t>(len), m_state->pos);
}

//-----------------------------------------------------------------------------
#ifdef RYML_DBG
void Parser::_dbg(const char *fmt, ...) const
{
    char errmsg[RYML_ERRMSG_SIZE];
    int len = sizeof(errmsg);

    va_list args;
    va_start(args, fmt);
    len = _fmt_msg(errmsg, len, fmt, args);
    va_end(args);
    printf("%.*s", len, errmsg);
}
#endif

//-----------------------------------------------------------------------------
#define _wrapbuf() pos += del; len -= del; if(len < 0) { pos = 0; len = buflen; }

int Parser::_fmt_msg(char *buf, int buflen, const char *fmt, va_list args) const
{
    int len = buflen;
    int pos = 0;
    auto const& lc = m_state->line_contents;


    // first line: print the message
    int del = vsnprintf(buf + pos, static_cast<size_t>(len), fmt, args);
    _wrapbuf();
    del = snprintf(buf + pos, static_cast<size_t>(len), "\n");
    _wrapbuf();

    // next line: print the yaml src line
    if( ! m_file.empty())
    {
        del = snprintf(buf + pos, static_cast<size_t>(len), "%.*s:%zd: '", (int)m_file.len, m_file.str, m_state->pos.line);
    }
    else
    {
        del = snprintf(buf + pos, static_cast<size_t>(len), "line %zd: '", m_state->pos.line);
    }
    int offs = del;
    _wrapbuf();
    del = snprintf(buf + pos, static_cast<size_t>(len), "%.*s' (sz=%zd)\n",
                   (int)lc.stripped.len, lc.stripped.str, lc.stripped.len);
    _wrapbuf();

    // next line: highlight the remaining portion of the previous line
    if(lc.rem.len)
    {
        size_t firstcol = static_cast<size_t>(lc.rem.begin() - lc.full.begin());
        size_t lastcol = firstcol + lc.rem.len;
        del = snprintf(buf + pos, static_cast<size_t>(len), "%*s", (offs+(int)firstcol), ""); // this works only for spaces....
        _wrapbuf();
        // the %*s technique works only for spaces, so put the characters directly
        del = (int)lc.rem.len;
        for(int i = 0; i < del && i < len; ++i) { buf[pos + i] = (i ? '~' : '^'); }
        _wrapbuf();
        del = snprintf(buf + pos, static_cast<size_t>(len), "  (cols %zd-%zd)\n", firstcol+1, lastcol+1);
        _wrapbuf();
    }
    else
    {
        del = snprintf(buf + pos, static_cast<size_t>(len), "\n");
        _wrapbuf();
    }

#ifdef RYML_DBG
    // next line: print the state flags
    {
        del = snprintf(buf + pos, static_cast<size_t>(len), "top state: ");
        _wrapbuf();

        del = _prfl(buf + pos, len, m_state->flags);
        _wrapbuf();

        del = snprintf(buf + len, static_cast<size_t>(len), "\n");
        _wrapbuf();
    }
#endif

    return pos;
}

int Parser::_prfl(char *buf, int buflen, size_t v)
{
    int len = buflen;
    int pos = 0, del = 0;

    bool gotone = false;
#define _prflag(fl)                                 \
    if((v & (fl)) == (fl))                          \
    {                                               \
        if(!gotone)                                 \
        {                                           \
            gotone = true;                          \
        }                                           \
        else                                        \
        {                                           \
            del = snprintf(buf + pos, static_cast<size_t>(len), "|");    \
            _wrapbuf();                             \
        }                                           \
        del = snprintf(buf + pos, static_cast<size_t>(len), #fl);        \
        _wrapbuf();                                 \
    }

    _prflag(RTOP);
    _prflag(RUNK);
    _prflag(RMAP);
    _prflag(RSEQ);
    _prflag(EXPL);
    _prflag(CPLX);
    _prflag(RKEY);
    _prflag(RVAL);
    _prflag(RNXT);
    _prflag(SSCL);
    _prflag(RSET);
    _prflag(NDOC);
    _prflag(RSEQIMAP);
#undef _prflag

    return pos;
}

#undef _wrapbuf

} // namespace yml
} // namespace c4


#if defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
