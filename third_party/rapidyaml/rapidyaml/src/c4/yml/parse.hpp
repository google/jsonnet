#ifndef _C4_YML_PARSE_HPP_
#define _C4_YML_PARSE_HPP_

#ifndef _C4_YML_TREE_HPP_
#include "c4/yml/tree.hpp"
#endif

#ifndef _C4_YML_NODE_HPP_
#include "c4/yml/node.hpp"
#endif

#ifndef _C4_YML_DETAIL_STACK_HPP_
#include "c4/yml/detail/stack.hpp"
#endif

#include <stdarg.h>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4251/*needs to have dll-interface to be used by clients of struct*/)
#endif

namespace c4 {
namespace yml {


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class RYML_EXPORT Parser
{
public:

    Parser(Allocator const& a={});

public:

    //! create a new YAML tree and parse into its root
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    Tree parse(csubstr filename, csubstr src) { Tree t; t.reserve(_estimate_capacity(src)); parse(filename, t.copy_to_arena(src), &t, t.root_id()); return t; }
    //! create a new YAML tree and parse into its root
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    Tree parse(csubstr filename,  substr src) { Tree t; t.reserve(_estimate_capacity(src)); parse(filename, src, &t, t.root_id()); return t; }


    //! parse with reuse of a YAML tree
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    void parse(csubstr filename,  substr src, Tree *t) { parse(filename, src, t, t->root_id()); }
    //! parse with reuse of a YAML tree
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    void parse(csubstr filename, csubstr src, Tree *t) { parse(filename, t->copy_to_arena(src), t, t->root_id()); }


    //! parse directly into a node
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    void parse(csubstr filename,  substr src, Tree *t, size_t node_id); // this is the workhorse overload; everything else is syntactic candy
    //! parse directly into a node
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    void parse(csubstr filename, csubstr src, Tree *t, size_t node_id) { parse(filename, t->copy_to_arena(src), t, node_id); }


    //! parse directly into a node ref
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    void parse(csubstr filename,  substr src, NodeRef node) { parse(filename, src, node.tree(), node.id()); }
    //! parse directly into a node ref
    //! @note aliases and anchors are not resolved. You
    //! can resolve by calling Tree::resolve() after parsing.
    void parse(csubstr filename, csubstr src, NodeRef node) { parse(filename, node.tree()->copy_to_arena(src), node.tree(), node.id()); }


private:

    typedef enum {
        BLOCK_LITERAL, //!< keep newlines (|)
        BLOCK_FOLD     //!< replace newline with single space (>)
    } BlockStyle_e;

    typedef enum {
        CHOMP_CLIP,    //!< single newline at end (default)
        CHOMP_STRIP,   //!< no newline at end     (-)
        CHOMP_KEEP     //!< all newlines from end (+)
    } BlockChomp_e;

private:

    static size_t _estimate_capacity(csubstr src) { size_t c = _count_nlines(src); c = c >= 16 ? c : 16; return c; }

    void  _reset();

    bool  _finished_file() const;
    bool  _finished_line() const;

    csubstr _peek_next_line(size_t pos=npos) const;
    bool    _advance_to_peeked();
    void    _scan_line();

    csubstr _slurp_doc_scalar();
    bool    _scan_scalar(csubstr *scalar);
    csubstr _scan_comment();
    csubstr _scan_quoted_scalar(const char q);
    csubstr _scan_block();
    csubstr _scan_ref();
    substr  _scan_plain_scalar_impl(csubstr currscalar, csubstr peeked_line, size_t indentation);
    substr  _scan_plain_scalar_expl(csubstr currscalar, csubstr peeked_line);
    substr  _scan_complex_key(csubstr currscalar, csubstr peeked_line);
    csubstr _scan_to_next_nonempty_line(size_t indentation);
    csubstr _extend_scanned_scalar(csubstr currscalar);

    csubstr _filter_squot_scalar(substr s);
    csubstr _filter_dquot_scalar(substr s);
    csubstr _filter_plain_scalar(substr s, size_t indentation);
    csubstr _filter_block_scalar(substr s, BlockStyle_e style, BlockChomp_e chomp, size_t indentation);
    substr  _filter_whitespace(substr s, size_t indentation=0, bool leading_whitespace=true);

    void  _handle_finished_file();
    void  _handle_line();

    bool  _handle_indentation();

    bool  _handle_unk();
    bool  _handle_map_expl();
    bool  _handle_map_impl();
    bool  _handle_seq_expl();
    bool  _handle_seq_impl();
    bool  _handle_top();
    bool  _handle_key_anchors_and_refs();
    bool  _handle_val_anchors_and_refs();
    bool  _handle_types();


    void  _push_level(bool explicit_flow_chars = false);
    void  _pop_level();

    void  _start_unk(bool as_child=true);

    void  _start_map(bool as_child=true);
    void  _stop_map();

    void  _start_seq(bool as_child=true);
    void  _stop_seq();

    void  _start_seqimap();
    void  _stop_seqimap();

    void  _start_doc(bool as_child=true);
    void  _stop_doc();
    void  _start_new_doc(csubstr rem);
    void  _end_stream();

    NodeData* _append_val(csubstr val);
    NodeData* _append_key_val(csubstr val);
    inline NodeData* _append_val_null() { return _append_val({}/*"~"*/); }
    inline NodeData* _append_key_val_null() { return _append_key_val({}/*"~"*/); }
    bool  _rval_dash_start_or_continue_seq();

    void  _store_scalar(csubstr const& s);
    void  _store_scalar_null() { _store_scalar({}/*"~"*/); }
    csubstr _consume_scalar();
    void  _move_scalar_from_top();

    void  _set_indentation(size_t behind);
    void  _save_indentation(size_t behind=0);

    void  _write_key_anchor(size_t node_id);
    void  _write_val_anchor(size_t node_id);


private:

    static bool   _read_decimal(csubstr const& str, size_t *decimal);
    static size_t _count_nlines(csubstr src);

private:

    typedef enum {
        RTOP = 0x01 <<  0,   ///< reading at top level
        RUNK = 0x01 <<  1,   ///< reading an unknown: must determine whether scalar, map or seq
        RMAP = 0x01 <<  2,   ///< reading a map
        RSEQ = 0x01 <<  3,   ///< reading a seq
        EXPL = 0x01 <<  4,   ///< reading is inside explicit flow chars: [] or {}
        CPLX = 0x01 <<  5,   ///< reading a complex key
        RKEY = 0x01 <<  6,   ///< reading a scalar as key
        RVAL = 0x01 <<  7,   ///< reading a scalar as val
        RNXT = 0x01 <<  8,   ///< read next val or keyval
        SSCL = 0x01 <<  9,   ///< there's a scalar stored
        RSET = 0x01 << 10,   ///< the (implicit) map being read is a !!set. @see https://yaml.org/type/set.html
        NDOC = 0x01 << 11,   ///< no document mode. a document has ended and another has not started yet.
        //! reading an implicit map nested in an explicit seq.
        //! eg, {key: [key2: value2, key3: value3]}
        //! is parsed as {key: [{key2: value2}, {key3: value3}]}
        RSEQIMAP = 0x01 << 12,
    } State_e;

    struct LineContents
    {
        csubstr  full;        ///< the full line, including newlines on the right
        csubstr  stripped;    ///< the stripped line, excluding newlines on the right
        csubstr  rem;         ///< the stripped line remainder; initially starts at the first non-space character
        size_t   indentation; ///< the number of spaces on the beginning of the line

        LineContents() : full(), stripped(), rem(), indentation() {}

        void reset(csubstr full_, csubstr stripped_)
        {
            full = full_;
            stripped = stripped_;
            rem = stripped_;
            // find the first column where the character is not a space
            indentation = full.first_not_of(' ');
        }

        size_t current_col() const
        {
            return current_col(rem);
        }

        size_t current_col(csubstr s) const
        {
            RYML_ASSERT(s.str >= full.str);
            RYML_ASSERT(full.is_super(s));
            size_t col = static_cast<size_t>(s.str - full.str);
            return col;
        }
    };

    struct State
    {
        size_t       flags;
        size_t       level;
        size_t       node_id; // don't hold a pointer to the node as it will be relocated during tree resizes
        csubstr      scalar;
        size_t       scalar_col; // the column where the scalar (or its quotes) begin

        Location     pos;
        LineContents line_contents;
        size_t       indref;

        State() : flags(), level(), node_id(), scalar(), scalar_col(), pos(), line_contents(), indref() {}
        
        void reset(const char *file, size_t node_id_)
        {
            flags = RUNK|RTOP;
            level = 0;
            pos.name = to_csubstr(file);
            pos.offset = 0;
            pos.line = 1;
            pos.col = 1;
            node_id = node_id_;
            scalar_col = 0;
            scalar.clear();
            indref = 0;
        }
    };

    void _line_progressed(size_t ahead);
    void _line_ended();

    void _prepare_pop()
    {
        RYML_ASSERT(m_stack.size() > 1);
        State const& curr = m_stack.top();
        State      & next = m_stack.top(1);
        next.pos = curr.pos;
        next.line_contents = curr.line_contents;
        next.scalar = curr.scalar;
    }

    inline bool _at_line_begin() const
    {
        return m_state->line_contents.rem.begin() == m_state->line_contents.full.begin();
    }
    inline bool _at_line_end() const
    {
        csubstr r = m_state->line_contents.rem;
        return r.empty() || r.begins_with(' ', r.len);
    }

    inline NodeData * node(State const* s) const { return m_tree->get(s->node_id); }
    inline NodeData * node(State const& s) const { return m_tree->get(s .node_id); }
    inline NodeData * node(size_t node_id) const { return m_tree->get(   node_id); }

    inline bool has_all(size_t f) const { return (m_state->flags & f) == f; }
    inline bool has_any(size_t f) const { return (m_state->flags & f) != 0; }
    inline bool has_none(size_t f) const { return (m_state->flags & f) == 0; }

    static inline bool has_all(size_t f, State const* s) { return (s->flags & f) == f; }
    static inline bool has_any(size_t f, State const* s) { return (s->flags & f) != 0; }
    static inline bool has_none(size_t f, State const* s) { return (s->flags & f) == 0; }

    inline void set_flags(size_t f) { set_flags(f, m_state); }
    inline void add_flags(size_t on) { add_flags(on, m_state); }
    inline void addrem_flags(size_t on, size_t off) { addrem_flags(on, off, m_state); }
    inline void rem_flags(size_t off) { rem_flags(off, m_state); }

    void set_flags(size_t f, State * s);
    void add_flags(size_t on, State * s);
    void addrem_flags(size_t on, size_t off, State * s);
    void rem_flags(size_t off, State * s);

private:

#ifdef RYML_DBG
    void _dbg(const char *msg, ...) const;
#endif
    void _err(const char *msg, ...) const;
    int  _fmt_msg(char *buf, int buflen, const char *msg, va_list args) const;
    static int  _prfl(char *buf, int buflen, size_t v);

private:

    csubstr m_file;
     substr m_buf;

    size_t  m_root_id;
    Tree *  m_tree;

    detail::stack<State> m_stack;
    State * m_state;

    csubstr m_key_tag;
    csubstr m_val_tag;

    csubstr m_key_anchor;
    csubstr m_val_anchor;

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

inline Tree parse(                   substr buf) { Parser np; return np.parse({}      , buf); } //!< parse in-situ a modifiable YAML source buffer.
inline Tree parse(csubstr filename,  substr buf) { Parser np; return np.parse(filename, buf); } //!< parse in-situ a modifiable YAML source buffer, providing a filename for error messages.
inline Tree parse(                  csubstr buf) { Parser np; return np.parse({}      , buf); } //!< parse a read-only YAML source buffer, copying it first to the tree's source arena.
inline Tree parse(csubstr filename, csubstr buf) { Parser np; return np.parse(filename, buf); } //!< parse a read-only YAML source buffer, copying it first to the tree's source arena, providing a filename for error messages.

inline void parse(                   substr buf, Tree *t) { Parser np; np.parse({}      , buf, t); } //!< reusing the YAML tree, parse in-situ a modifiable YAML source buffer
inline void parse(csubstr filename,  substr buf, Tree *t) { Parser np; np.parse(filename, buf, t); } //!< reusing the YAML tree, parse in-situ a modifiable YAML source buffer, providing a filename for error messages.
inline void parse(                  csubstr buf, Tree *t) { Parser np; np.parse({}      , buf, t); } //!< reusing the YAML tree, parse a read-only YAML source buffer, copying it first to the tree's source arena.
inline void parse(csubstr filename, csubstr buf, Tree *t) { Parser np; np.parse(filename, buf, t); } //!< reusing the YAML tree, parse a read-only YAML source buffer, copying it first to the tree's source arena, providing a filename for error messages.

inline void parse(                   substr buf, Tree *t, size_t node_id) { Parser np; np.parse({}      , buf, t, node_id); } //!< reusing the YAML tree, parse in-situ a modifiable YAML source buffer
inline void parse(csubstr filename,  substr buf, Tree *t, size_t node_id) { Parser np; np.parse(filename, buf, t, node_id); } //!< reusing the YAML tree, parse in-situ a modifiable YAML source buffer, providing a filename for error messages.
inline void parse(                  csubstr buf, Tree *t, size_t node_id) { Parser np; np.parse({}      , buf, t, node_id); } //!< reusing the YAML tree, parse a read-only YAML source buffer, copying it first to the tree's source arena.
inline void parse(csubstr filename, csubstr buf, Tree *t, size_t node_id) { Parser np; np.parse(filename, buf, t, node_id); } //!< reusing the YAML tree, parse a read-only YAML source buffer, copying it first to the tree's source arena, providing a filename for error messages.

inline void parse(                   substr buf, NodeRef node) { Parser np; np.parse({}      , buf, node); } //!< reusing the YAML tree, parse in-situ a modifiable YAML source buffer
inline void parse(csubstr filename,  substr buf, NodeRef node) { Parser np; np.parse(filename, buf, node); } //!< reusing the YAML tree, parse in-situ a modifiable YAML source buffer, providing a filename for error messages.
inline void parse(                  csubstr buf, NodeRef node) { Parser np; np.parse({}      , buf, node); } //!< reusing the YAML tree, parse a read-only YAML source buffer, copying it first to the tree's source arena.
inline void parse(csubstr filename, csubstr buf, NodeRef node) { Parser np; np.parse(filename, buf, node); } //!< reusing the YAML tree, parse a read-only YAML source buffer, copying it first to the tree's source arena, providing a filename for error messages.

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#endif

#endif /* _C4_YML_PARSE_HPP_ */
