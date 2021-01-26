#ifndef _C4_YML_EMIT_DEF_HPP_
#define _C4_YML_EMIT_DEF_HPP_

#ifndef _C4_YML_EMIT_HPP_
#include "./emit.hpp"
#endif
#include "./detail/parser_dbg.hpp"

namespace c4 {
namespace yml {

template<class Writer>
substr Emitter<Writer>::emit(EmitType_e type, Tree const& t, size_t id, bool error_on_excess)
{
    if(type == YAML)
    {
        _do_visit(t, id, 0);
    }
    else if(type == JSON)
    {
        _do_visit_json(t, id);
    }
    else
    {
        c4::yml::error("unknown emit type");
    }
    substr result = this->Writer::_get(error_on_excess);
    return result;
}

/** @todo this function is too complex. break it down into manageable
 * pieces */
template<class Writer>
void Emitter<Writer>::_do_visit(Tree const& t, size_t id, size_t ilevel, size_t do_indent)
{
    RepC ind = indent_to(do_indent * ilevel);
    RYML_ASSERT(t.is_root(id) || (t.parent_is_map(id) || t.parent_is_seq(id)));

    if(t.is_doc(id))
    {
        this->Writer::_do_write("---");
        if(t.has_val(id))
        {
            RYML_ASSERT(!t.has_key(id));
            this->Writer::_do_write(' ');
            _writev(t, id, ilevel);
        }
        else
        {
            if(t.has_val_tag(id))
            {
                this->Writer::_do_write(' ');
                this->Writer::_do_write(t.val_tag(id));
            }
            if(t.has_val_anchor(id))
            {
                this->Writer::_do_write(' ');
                this->Writer::_do_write('&');
                this->Writer::_do_write(t.val_anchor(id));
            }
        }
        this->Writer::_do_write('\n');
    }
    else if(t.is_keyval(id))
    {
        RYML_ASSERT(t.has_parent(id));
        this->Writer::_do_write(ind);
        _writek(t, id, ilevel);
        this->Writer::_do_write(": ");
        _writev(t, id, ilevel);
        this->Writer::_do_write('\n');
        return;
    }
    else if(t.has_key(id) && t.is_val_ref(id))
    {
        RYML_ASSERT(t.has_parent(id));
        this->Writer::_do_write(ind);
        _writek(t, id, ilevel);
        this->Writer::_do_write(": ");
        this->Writer::_do_write('*');
        this->Writer::_do_write(t.val_ref(id));
        this->Writer::_do_write('\n');
        return;
    }
    else if(t.is_val(id))
    {
        RYML_ASSERT(t.has_parent(id));
        this->Writer::_do_write(ind);
        this->Writer::_do_write("- ");
        _writev(t, id, ilevel);
        this->Writer::_do_write('\n');
        return;
    }
    else if(t.is_val_ref(id))
    {
        RYML_ASSERT(t.has_parent(id));
        this->Writer::_do_write(ind);
        this->Writer::_do_write("- ");
        this->Writer::_do_write('*');
        this->Writer::_do_write(t.val_ref(id));
        this->Writer::_do_write('\n');
        return;
    }
    else if(t.is_container(id))
    {
        RYML_ASSERT(t.is_map(id) || t.is_seq(id));

        bool spc = false; // write a space
        bool nl = false;  // write a newline

        if(t.has_key(id))
        {
            this->Writer::_do_write(ind);
            _writek(t, id, ilevel);
            this->Writer::_do_write(':');
            spc = true;
        }
        else if(!t.is_root(id))
        {
            this->Writer::_do_write(ind);
            this->Writer::_do_write('-');
            spc = true;
        }

        if(t.has_val_tag(id))
        {
            if(spc) this->Writer::_do_write(' ');
            this->Writer::_do_write(t.val_tag(id));
            spc = true;
            nl = true;
        }

        if(t.has_val_anchor(id))
        {
            if(spc) this->Writer::_do_write(' ');
            this->Writer::_do_write('&');
            this->Writer::_do_write(t.val_anchor(id));
            spc = true;
            nl = true;
        }

        if(t.has_children(id))
        {
            if(t.has_key(id))
            {
                nl = true;
            }
            else
            {
                if(!t.is_root(id) && !nl)
                {
                    spc = true;
                }
            }
        }
        else
        {
            if(t.is_seq(id))
            {
                this->Writer::_do_write(" []\n");
            }
            else if(t.is_map(id))
            {
                this->Writer::_do_write(" {}\n");
            }
            return;
        }

        if(spc && !nl)
        {
            this->Writer::_do_write(' ');
        }

        do_indent = 0;
        if(nl)
        {
            this->Writer::_do_write('\n');
            do_indent = 1;
        }
    } // container

    size_t next_level = ilevel + 1;
    if(t.is_stream(id) || t.is_doc(id) || t.is_root(id))
    {
        next_level = ilevel; // do not indent at top level
    }

    for(size_t ich = t.first_child(id); ich != NONE; ich = t.next_sibling(ich))
    {
        _do_visit(t, ich, next_level, do_indent);
        do_indent = true;
    }
}
template<class Writer>
void Emitter<Writer>::_do_visit_json(Tree const& t, size_t id)
{
    if(t.is_doc(id))
    {
        c4::yml::error("no doc processing for JSON");
    }
    else if(t.is_keyval(id))
    {
        _writek_json(t, id);
        this->Writer::_do_write(": ");
        _writev_json(t, id);
    }
    else if(t.is_val(id))
    {
        _writev_json(t, id);
    }
    else if(t.is_container(id))
    {
        if(t.has_key(id))
        {
            _writek_json(t, id);
            this->Writer::_do_write(": ");
        }

        if(t.is_seq(id))
        {
            this->Writer::_do_write('[');
        }
        else if(t.is_map(id))
        {
            this->Writer::_do_write('{');
        }
    } // container
    for(size_t ich = t.first_child(id); ich != NONE; ich = t.next_sibling(ich))
    {
        if(ich != t.first_child(id)) this->Writer::_do_write(',');
        _do_visit_json(t, ich);
    }
    if(t.is_container(id))
    {
        if(t.is_seq(id))
        {
            this->Writer::_do_write(']');
        }
        else if(t.is_map(id))
        {
            this->Writer::_do_write('}');
        }
    }
}

template<class Writer>
void Emitter<Writer>::_write(NodeScalar const& sc, NodeType flags, size_t ilevel)
{
    if( ! sc.tag.empty())
    {
        this->Writer::_do_write(sc.tag);
        this->Writer::_do_write(' ');
    }
    if(flags.has_anchor())
    {
        RYML_ASSERT(flags.is_ref() != flags.has_anchor());
        RYML_ASSERT( ! sc.anchor.empty());
        this->Writer::_do_write('&');
        this->Writer::_do_write(sc.anchor);
        this->Writer::_do_write(' ');
    }

    const bool has_newlines = sc.scalar.first_of('\n') != npos;
    if(!has_newlines || (sc.scalar.triml(" \t") != sc.scalar))
    {
        _write_scalar(sc.scalar);
    }
    else
    {
        _write_scalar_block(sc.scalar, ilevel, flags.has_key());
    }
}
template<class Writer>
void Emitter<Writer>::_write_json(NodeScalar const& sc, NodeType flags)
{
    if( ! sc.tag.empty())
    {
        c4::yml::error("no tag processing for JSON");
    }
    if(flags.has_anchor())
    {
        c4::yml::error("no anchor processing for JSON");
    }
    _write_scalar_json(sc.scalar, flags.has_key());
}

template<class Writer>
void Emitter<Writer>::_write_scalar_block(csubstr s, size_t ilevel, bool as_key)
{
    #define _rymlindent_nextline() for(size_t lv = 0; lv < ilevel+1; ++lv) { this->Writer::_do_write("  "); }
    if(as_key)
    {
        this->Writer::_do_write("? ");
    }
    RYML_ASSERT(s.find("\r") == csubstr::npos);
    csubstr trimmed = s.trimr(" \t\n");
    RYML_ASSERT(trimmed.len <= s.len);
    size_t numnewlines_at_end = s.len - trimmed.len;
    _c4dbgpf("numnl=%zu s=[%zu]'%.*s' trimmed=[%zu]'%.*s'", numnewlines_at_end, s.len, _c4prsp(s), trimmed.len, _c4prsp(trimmed));
    if(numnewlines_at_end == 0)
    {
        this->Writer::_do_write("|-\n");
    }
    else if(numnewlines_at_end == 1)
    {
        this->Writer::_do_write("|\n");
    }
    else if(numnewlines_at_end > 1)
    {
        this->Writer::_do_write("|+\n");
        if(!as_key)
        {
            RYML_ASSERT(s.back() == '\n');
            s = s.offs(0, 1); // do not write the last newline
        }
    }
    _rymlindent_nextline()
    size_t pos = 0; // tracks the last character that was already written
    for(size_t i = 0; i < s.len; ++i)
    {
        if(s[i] != '\n') continue;
        // write everything up to this point
        csubstr sub = s.range(pos, i+1); // include the newline
        pos = i+1; // because of the newline
        this->Writer::_do_write(sub);
        if(i+1 != s.len)
        {
            _rymlindent_nextline();
        }
    }
    if(pos < s.len)
    {
        csubstr sub = s.sub(pos);
        this->Writer::_do_write(sub);
    }
    if(as_key && numnewlines_at_end == 0)
    {
        this->Writer::_do_write('\n');
    }
    #undef _rymlindent_nextline
}

template<class Writer>
void Emitter<Writer>::_write_scalar(csubstr s)
{
    // this block of code needed to be moved to before the needs_quotes
    // assignment to workaround a g++ optimizer bug where (s.str != nullptr)
    // was evaluated as true even if s.str was actually a nullptr (!!!)
    if(s.len == 0)
    {
        if(s.str != nullptr)
        {
            this->Writer::_do_write("''");
        }
        else
        {
            this->Writer::_do_write('~');
        }
        return;
    }

    const bool needs_quotes = (
        !s.is_number() // is not a number
        &&
        (
            (s != s.trim(" \t\n\r")) // has leading or trailing whitespace
            ||
            s.first_of("#:-?,\n{}[]'\"") != npos // has special chars
            )
        );

    if(!needs_quotes)
    {
        this->Writer::_do_write(s);
    }
    else
    {
        const bool has_dquotes = s.first_of( '"') != npos;
        const bool has_squotes = s.first_of('\'') != npos;
        if(!has_squotes && has_dquotes)
        {
            this->Writer::_do_write('\'');
            this->Writer::_do_write(s);
            this->Writer::_do_write('\'');
        }
        else if(has_squotes && !has_dquotes)
        {
            this->Writer::_do_write('"');
            this->Writer::_do_write(s);
            this->Writer::_do_write('"');
        }
        else
        {
            size_t pos = 0; // tracks the last character that was already written
            this->Writer::_do_write('\'');
            for(size_t i = 0; i < s.len; ++i)
            {
                if(s[i] == '\'' || s[i] == '\n')
                {
                    csubstr sub = s.range(pos, i);
                    pos = i;
                    this->Writer::_do_write(sub); // write everything up to this point
                    this->Writer::_do_write(s[i]); // write the character twice
                }
            }
            if(pos < s.len)
            {
                csubstr sub = s.sub(pos);
                this->Writer::_do_write(sub);
            }
            this->Writer::_do_write('\'');
        }
    }
}
template<class Writer>
void Emitter<Writer>::_write_scalar_json(csubstr s, bool as_key)
{
    if(!as_key && s.is_number())
    {
        this->Writer::_do_write(s);
    }
    else if(!as_key && (s == "true" || s == "null" || s == "false"))
    {
        this->Writer::_do_write(s);
    }
    else
    {
        size_t pos = 0;
        this->Writer::_do_write('"');
        for(size_t i = 0; i < s.len; ++i)
        {
            if(s[i] == '"')
            {
                if(i > 0)
                {
                    csubstr sub = s.range(pos, i);
                    this->Writer::_do_write(sub);
                }
                pos = i + 1;
                this->Writer::_do_write("\\\"");
            }
        }
        if(pos < s.len)
        {
            csubstr sub = s.sub(pos);
            this->Writer::_do_write(sub);
        }
        this->Writer::_do_write('"');
    }
}

} // namespace yml
} // namespace c4

#endif /* _C4_YML_EMIT_DEF_HPP_ */
