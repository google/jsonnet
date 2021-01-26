#ifndef _C4_YML_EMIT_HPP_
#define _C4_YML_EMIT_HPP_

#ifndef _C4_YML_WRITER_HPP_
#include "./writer.hpp"
#endif

#ifndef _C4_YML_TREE_HPP_
#include "./tree.hpp"
#endif

#ifndef _C4_YML_NODE_HPP_
#include "./node.hpp"
#endif

namespace c4 {
namespace yml {

template<class Writer> class Emitter;

template<class OStream>
using EmitterOStream = Emitter<WriterOStream<OStream>>;
using EmitterFile = Emitter<WriterFile>;
using EmitterBuf  = Emitter<WriterBuf>;

typedef enum {
    YAML = 0,
    JSON = 1
} EmitType_e;


/** mark a tree or node to be emitted as json */
struct as_json
{
    Tree const* tree;
    size_t node;
    as_json(Tree const& t) : tree(&t), node(t.root_id()) {}
    as_json(Tree const& t, size_t id) : tree(&t), node(id) {}
    as_json(NodeRef const& n) : tree(n.tree()), node(n.id()) {}
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template<class Writer>
class Emitter : public Writer
{
public:

    using Writer::Writer;

    /** emit!
     *
     * When writing to a buffer, returns a substr of the emitted YAML.
     * If the given buffer has insufficient space, the returned span will
     * be null and its size will be the needed space. No writes are done
     * after the end of the buffer.
     *
     * When writing to a file, the returned substr will be null, but its
     * length will be set to the number of bytes written. */
    substr emit(EmitType_e type, Tree const& t, size_t id, bool error_on_excess);
    /** @overload */
    substr emit(EmitType_e type, Tree const& t, bool error_on_excess=true) { return emit(type, t, t.root_id(), error_on_excess); }
    /** @overload */
    substr emit(EmitType_e type, NodeRef const& n, bool error_on_excess=true) { return emit(type, *n.tree(), n.id(), error_on_excess); }

private:

    void _do_visit(Tree const& t, size_t id, size_t ilevel=0, size_t do_indent=1);
    void _do_visit_json(Tree const& t, size_t id);

private:

    void _write(NodeScalar const& sc, NodeType flags, size_t level);
    void _write_json(NodeScalar const& sc, NodeType flags);

    void _write_scalar(csubstr s);
    void _write_scalar_json(csubstr s, bool as_key);
    void _write_scalar_block(csubstr s, size_t level, bool as_key);

    void _indent(size_t ilevel)
    {
        this->Writer::_do_write(indent_to(ilevel));
    }

    enum {
        _keysc =  (KEY|KEYREF|KEYANCH)  | ~(VAL|VALREF|VALANCH),
        _valsc = ~(KEY|KEYREF|KEYANCH)  |  (VAL|VALREF|VALANCH),
        _keysc_json =  (KEY)  | ~(VAL),
        _valsc_json = ~(KEY)  |  (VAL),
    };

    C4_ALWAYS_INLINE void _writek(Tree const& t, size_t id, size_t level) { _write(t.keysc(id), t.type(id) & ~(VAL|VALREF|VALANCH), level); }
    C4_ALWAYS_INLINE void _writev(Tree const& t, size_t id, size_t level) { _write(t.valsc(id), t.type(id) & ~(KEY|KEYREF|KEYANCH), level); }

    C4_ALWAYS_INLINE void _writek_json(Tree const& t, size_t id) { _write_json(t.keysc(id), t.type(id) & ~(VAL)); }
    C4_ALWAYS_INLINE void _writev_json(Tree const& t, size_t id) { _write_json(t.valsc(id), t.type(id) & ~(KEY)); }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** emit YAML to the given file. A null file defaults to stdout.
 * Return the number of bytes written. */
inline size_t emit(Tree const& t, size_t id, FILE *f)
{
    EmitterFile em(f);
    size_t len = em.emit(YAML, t, id, /*error_on_excess*/true).len;
    return len;
}
/** emit JSON to the given file. A null file defaults to stdout.
 * Return the number of bytes written. */
inline size_t emit_json(Tree const& t, size_t id, FILE *f)
{
    EmitterFile em(f);
    size_t len = em.emit(JSON, t, id, /*error_on_excess*/true).len;
    return len;
}

/** emit YAML to the given file. A null file defaults to stdout.
 * Return the number of bytes written.
 * @overload */
inline size_t emit(Tree const& t, FILE *f=nullptr)
{
    return emit(t, t.root_id(), f);
}
/** emit JSON to the given file. A null file defaults to stdout.
 * Return the number of bytes written.
 * @overload */
inline size_t emit_json(Tree const& t, FILE *f=nullptr)
{
    return emit_json(t, t.root_id(), f);
}

/** emit YAML to the given file. A null file defaults to stdout.
 * Return the number of bytes written.
 * @overload */
inline size_t emit(NodeRef const& r, FILE *f=nullptr)
{
    return emit(*r.tree(), r.id(), f);
}
/** emit JSON to the given file. A null file defaults to stdout.
 * Return the number of bytes written.
 * @overload */
inline size_t emit_json(NodeRef const& r, FILE *f=nullptr)
{
    return emit_json(*r.tree(), r.id(), f);
}


//-----------------------------------------------------------------------------

/** emit YAML to an STL-like ostream */
template<class OStream>
inline OStream& operator<< (OStream& s, Tree const& t)
{
    EmitterOStream<OStream> em(s);
    em.emit(YAML, t.rootref());
    return s;
}

/** emit YAML to an STL-like ostream
 * @overload */
template<class OStream>
inline OStream& operator<< (OStream& s, NodeRef const& n)
{
    EmitterOStream<OStream> em(s);
    em.emit(YAML, n);
    return s;
}

/** emit json to the stream */
template<class OStream>
inline OStream& operator<< (OStream& s, as_json const& js)
{
    EmitterOStream<OStream> em(s);
    em.emit(JSON, *js.tree, js.node);
    return s;
}


//-----------------------------------------------------------------------------

/** emit YAML to the given buffer. Return a substr trimmed to the emitted YAML.
 * @param error_on_excess Raise an error if the space in the buffer is insufficient.
 * @overload */
inline substr emit(Tree const& t, size_t id, substr buf, bool error_on_excess=true)
{
    EmitterBuf em(buf);
    substr result = em.emit(YAML, t, id, error_on_excess);
    return result;
}
/** emit JSON to the given buffer. Return a substr trimmed to the emitted JSON.
 * @param error_on_excess Raise an error if the space in the buffer is insufficient.
 * @overload */
inline substr emit_json(Tree const& t, size_t id, substr buf, bool error_on_excess=true)
{
    EmitterBuf em(buf);
    substr result = em.emit(JSON, t, id, error_on_excess);
    return result;
}

/** emit YAML to the given buffer. Return a substr trimmed to the emitted YAML.
 * @param error_on_excess Raise an error if the space in the buffer is insufficient.
 * @overload */
inline substr emit(Tree const& t, substr buf, bool error_on_excess=true)
{
    return emit(t, t.root_id(), buf, error_on_excess);
}
/** emit JSON to the given buffer. Return a substr trimmed to the emitted JSON.
 * @param error_on_excess Raise an error if the space in the buffer is insufficient.
 * @overload */
inline substr emit_json(Tree const& t, substr buf, bool error_on_excess=true)
{
    return emit_json(t, t.root_id(), buf, error_on_excess);
}

/** emit YAML to the given buffer. Return a substr trimmed to the emitted YAML.
 * @param error_on_excess Raise an error if the space in the buffer is insufficient.
 * @overload
 */
inline substr emit(NodeRef const& r, substr buf, bool error_on_excess=true)
{
    return emit(*r.tree(), r.id(), buf, error_on_excess);
}
/** emit JSON to the given buffer. Return a substr trimmed to the emitted JSON.
 * @param error_on_excess Raise an error if the space in the buffer is insufficient.
 * @overload
 */
inline substr emit_json(NodeRef const& r, substr buf, bool error_on_excess=true)
{
    return emit_json(*r.tree(), r.id(), buf, error_on_excess);
}


//-----------------------------------------------------------------------------

/** emit+resize: YAML to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted YAML. */
template<class CharOwningContainer>
substr emitrs(Tree const& t, size_t id, CharOwningContainer * cont)
{
    substr buf = to_substr(*cont);
    substr ret = emit(t, id, buf, /*error_on_excess*/false);
    if(ret.str == nullptr && ret.len > 0)
    {
        cont->resize(ret.len);
        buf = to_substr(*cont);
        ret = emit(t, id, buf, /*error_on_excess*/true);
    }
    return ret;
}
/** emit+resize: JSON to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted JSON. */
template<class CharOwningContainer>
substr emitrs_json(Tree const& t, size_t id, CharOwningContainer * cont)
{
    substr buf = to_substr(*cont);
    substr ret = emit_json(t, id, buf, /*error_on_excess*/false);
    if(ret.str == nullptr && ret.len > 0)
    {
        cont->resize(ret.len);
        buf = to_substr(*cont);
        ret = emit_json(t, id, buf, /*error_on_excess*/true);
    }
    return ret;
}

/** emit+resize: YAML to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted YAML. */
template<class CharOwningContainer>
CharOwningContainer emitrs(Tree const& t, size_t id)
{
    CharOwningContainer c;
    emitrs(t, id, &c);
    return c;
}
/** emit+resize: JSON to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted JSON. */
template<class CharOwningContainer>
CharOwningContainer emitrs_json(Tree const& t, size_t id)
{
    CharOwningContainer c;
    emitrs_json(t, id, &c);
    return c;
}

/** emit+resize: YAML to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted YAML. */
template<class CharOwningContainer>
substr emitrs(Tree const& t, CharOwningContainer * cont)
{
    return emitrs(t, t.root_id(), cont);
}
/** emit+resize: JSON to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted JSON. */
template<class CharOwningContainer>
substr emitrs_json(Tree const& t, CharOwningContainer * cont)
{
    return emitrs_json(t, t.root_id(), cont);
}

/** emit+resize: YAML to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted YAML. */
template<class CharOwningContainer>
CharOwningContainer emitrs(Tree const& t)
{
    CharOwningContainer c;
    emitrs(t, t.root_id(), &c);
    return c;
}
/** emit+resize: JSON to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted JSON. */
template<class CharOwningContainer>
CharOwningContainer emitrs_json(Tree const& t)
{
    CharOwningContainer c;
    emitrs_json(t, t.root_id(), &c);
    return c;
}

/** emit+resize: YAML to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted YAML. */
template<class CharOwningContainer>
substr emitrs(NodeRef const& n, CharOwningContainer * cont)
{
    return emitrs(*n.tree(), n.id(), cont);
}
/** emit+resize: JSON to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted JSON. */
template<class CharOwningContainer>
substr emitrs_json(NodeRef const& n, CharOwningContainer * cont)
{
    return emitrs_json(*n.tree(), n.id(), cont);
}

/** emit+resize: YAML to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted YAML. */
template<class CharOwningContainer>
CharOwningContainer emitrs(NodeRef const& n)
{
    CharOwningContainer c;
    emitrs(*n.tree(), n.id(), &c);
    return c;
}
/** emit+resize: JSON to the given std::string/std::vector-like container,
 * resizing it as needed to fit the emitted JSON. */
template<class CharOwningContainer>
CharOwningContainer emitrs_json(NodeRef const& n)
{
    CharOwningContainer c;
    emitrs_json(*n.tree(), n.id(), &c);
    return c;
}

} // namespace yml
} // namespace c4

#include "./emit.def.hpp"

#endif /* _C4_YML_EMIT_HPP_ */
