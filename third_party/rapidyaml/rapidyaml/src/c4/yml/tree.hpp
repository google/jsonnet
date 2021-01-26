#ifndef _C4_YML_TREE_HPP_
#define _C4_YML_TREE_HPP_


#include "c4/types.hpp"
#ifndef _C4_YML_COMMON_HPP_
#include "c4/yml/common.hpp"
#endif

#include <c4/charconv.hpp>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4251/*needs to have dll-interface to be used by clients of struct*/)
#   pragma warning(disable: 4296/*expression is always 'boolean_value'*/)
#elif defined(__clang__)
#   pragma clang diagnostic push
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wtype-limits"
#endif


namespace c4 {
namespace yml {

struct NodeScalar;
struct NodeInit;
struct NodeData;
class NodeRef;
class Tree;


/** the integral type necessary to cover all the bits marking node types */
using tag_bits = uint16_t;

/** the integral type necessary to cover all the bits marking node types */
using type_bits = uint64_t;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** a bit mask for marking tags for types */
typedef enum : uint8_t {
    // container types
    TAG_NONE      =  0,
    TAG_MAP       =  1, /**< !!map   Unordered set of key: value pairs without duplicates. @see https://yaml.org/type/map.html */
    TAG_OMAP      =  2, /**< !!omap  Ordered sequence of key: value pairs without duplicates. @see https://yaml.org/type/omap.html */
    TAG_PAIRS     =  3, /**< !!pairs Ordered sequence of key: value pairs allowing duplicates. @see https://yaml.org/type/pairs.html */
    TAG_SET       =  4, /**< !!set   Unordered set of non-equal values. @see https://yaml.org/type/set.html */
    TAG_SEQ       =  5, /**< !!seq   Sequence of arbitrary values. @see https://yaml.org/type/seq.html */
    // scalar types
    TAG_BINARY    =  6, /**< !!binary A sequence of zero or more octets (8 bit values). @see https://yaml.org/type/binary.html */
    TAG_BOOL      =  7, /**< !!bool   Mathematical Booleans. @see https://yaml.org/type/bool.html */
    TAG_FLOAT     =  8, /**< !!float  Floating-point approximation to real numbers. https://yaml.org/type/float.html */
    TAG_INT       =  9, /**< !!float  Mathematical integers. https://yaml.org/type/int.html */
    TAG_MERGE     = 10, /**< !!merge  Specify one or more mapping to be merged with the current one. https://yaml.org/type/merge.html */
    TAG_NULL      = 11, /**< !!null   Devoid of value. https://yaml.org/type/null.html */
    TAG_STR       = 12, /**< !!str    A sequence of zero or more Unicode characters. https://yaml.org/type/str.html */
    TAG_TIMESTAMP = 13, /**< !!timestamp A point in time https://yaml.org/type/timestamp.html */
    TAG_VALUE     = 14, /**< !!value  Specify the default value of a mapping https://yaml.org/type/value.html */
    TAG_YAML      = 15, /**< !!yaml   Specify the default value of a mapping https://yaml.org/type/yaml.html */
} YamlTag_e;


YamlTag_e to_tag(csubstr tag);



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------


/** the integral type necessary to cover all the bits marking node types */
using type_bits = uint64_t;


/** a bit mask for marking node types */
typedef enum : type_bits {
    // a convenience define, undefined below
    #define c4bit(v) (type_bits(1) << v)
    NOTYPE  = 0,            ///< no node type is set
    VAL     = c4bit(0),     ///< a leaf node, has a (possibly empty) value
    KEY     = c4bit(1),     ///< is member of a map, must have non-empty key
    MAP     = c4bit(2),     ///< a map: a parent of keyvals
    SEQ     = c4bit(3),     ///< a seq: a parent of vals
    DOC     = c4bit(4),     ///< a document
    STREAM  = c4bit(5)|SEQ, ///< a stream: a seq of docs
    KEYREF  = c4bit(6),     ///< a *reference: the key references an &anchor
    VALREF  = c4bit(7),     ///< a *reference: the val references an &anchor
    KEYANCH = c4bit(8),     ///< the key has an &anchor
    VALANCH = c4bit(9),     ///< the val has an &anchor
    _TYMASK = c4bit(10)-1,
    KEYTAG  = c4bit(10),    ///< the key has an explicit tag/type
    VALTAG  = c4bit(11),    ///< the val has an explicit tag/type
    KEYVAL  = KEY|VAL,
    KEYSEQ  = KEY|SEQ,
    KEYMAP  = KEY|MAP,
    DOCMAP  = DOC|MAP,
    DOCSEQ  = DOC|SEQ,
    DOCVAL  = DOC|VAL,

#ifdef C4_WORK_IN_PROGRESS_
    // https://yaml.org/type/
    _NUM_TAG_TYPES = 15,
    _KTAG_SHIFT = type_bits(12),
    _VTAG_SHIFT = type_bits(_KTAG_SHIFT + _NUM_TAG_TYPES),
    _TAG_END_SHIFT = type_bits(_VTAG_SHIFT + _NUM_TAG_TYPES + 1), // the first non-tagtype bit after 
    _KVTAG_MASK = (~(c4bit(_KTAG_SHIFT)-1))/*zeros to the left of tagtype bits */
                  & (c4bit(_TAG_END_SHIFT)-1)/*ones until the end of tagtype bits*/,
    // key: collection types
    KTAG_MAP    = c4bit( 0+_KTAG_SHIFT), /**< !!map   Unordered set of key: value pairs without duplicates. @see https://yaml.org/type/map.html */
    KTAG_OMAP   = c4bit( 1+_KTAG_SHIFT), /**< !!omap  Ordered sequence of key: value pairs without duplicates. @see https://yaml.org/type/omap.html */
    KTAG_PAIRS  = c4bit( 2+_KTAG_SHIFT), /**< !!pairs Ordered sequence of key: value pairs allowing duplicates. @see https://yaml.org/type/pairs.html */
    KTAG_SET    = c4bit( 3+_KTAG_SHIFT), /**< !!set   Unordered set of non-equal values. @see https://yaml.org/type/set.html */
    KTAG_SEQ    = c4bit( 4+_KTAG_SHIFT), /**< !!set   Sequence of arbitrary values. @see https://yaml.org/type/seq.html */
    // key: scalar types
    KTAG_BINARY = c4bit( 5+_KTAG_SHIFT), /**< !!binary A sequence of zero or more octets (8 bit values). @see https://yaml.org/type/binary.html */
    KTAG_BOOL   = c4bit( 6+_KTAG_SHIFT), /**< !!bool   Mathematical Booleans. @see https://yaml.org/type/bool.html */
    KTAG_FLOAT  = c4bit( 7+_KTAG_SHIFT), /**< !!float  Floating-point approximation to real numbers. https://yaml.org/type/float.html */
    KTAG_INT    = c4bit( 8+_KTAG_SHIFT), /**< !!float  Mathematical integers. https://yaml.org/type/int.html */
    KTAG_MERGE  = c4bit( 9+_KTAG_SHIFT), /**< !!merge  Specify one or more mapping to be merged with the current one. https://yaml.org/type/merge.html */
    KTAG_NULL   = c4bit(10+_KTAG_SHIFT), /**< !!null   Devoid of value. https://yaml.org/type/null.html */
    KTAG_STR    = c4bit(11+_KTAG_SHIFT), /**< !!str    A sequence of zero or more Unicode characters. https://yaml.org/type/str.html */
    KTAG_TIME   = c4bit(12+_KTAG_SHIFT), /**< !!timestamp A point in time https://yaml.org/type/timestamp.html */
    KTAG_VALUE  = c4bit(13+_KTAG_SHIFT), /**< !!value  Specify the default value of a mapping https://yaml.org/type/value.html */
    KTAG_YAML   = c4bit(14+_KTAG_SHIFT), /**< !!yaml   Specify the default value of a mapping https://yaml.org/type/yaml.html */
    // key: collection types
    VTAG_MAP    = c4bit( 0+_VTAG_SHIFT), /**< !!map   Unordered set of key: value pairs without duplicates. @see https://yaml.org/type/map.html */
    VTAG_OMAP   = c4bit( 1+_VTAG_SHIFT), /**< !!omap  Ordered sequence of key: value pairs without duplicates. @see https://yaml.org/type/omap.html */
    VTAG_PAIRS  = c4bit( 2+_VTAG_SHIFT), /**< !!pairs Ordered sequence of key: value pairs allowing duplicates. @see https://yaml.org/type/pairs.html */
    VTAG_SET    = c4bit( 3+_VTAG_SHIFT), /**< !!set   Unordered set of non-equal values. @see https://yaml.org/type/set.html */
    VTAG_SEQ    = c4bit( 4+_VTAG_SHIFT), /**< !!set   Sequence of arbitrary values. @see https://yaml.org/type/seq.html */
    // key: scalar types
    VTAG_BINARY = c4bit( 5+_VTAG_SHIFT), /**< !!binary A sequence of zero or more octets (8 bit values). @see https://yaml.org/type/binary.html */
    VTAG_BOOL   = c4bit( 6+_VTAG_SHIFT), /**< !!bool   Mathematical Booleans. @see https://yaml.org/type/bool.html */
    VTAG_FLOAT  = c4bit( 7+_VTAG_SHIFT), /**< !!float  Floating-point approximation to real numbers. https://yaml.org/type/float.html */
    VTAG_INT    = c4bit( 8+_VTAG_SHIFT), /**< !!float  Mathematical integers. https://yaml.org/type/int.html */
    VTAG_MERGE  = c4bit( 9+_VTAG_SHIFT), /**< !!merge  Specify one or more mapping to be merged with the current one. https://yaml.org/type/merge.html */
    VTAG_NULL   = c4bit(10+_VTAG_SHIFT), /**< !!null   Devoid of value. https://yaml.org/type/null.html */
    VTAG_STR    = c4bit(11+_VTAG_SHIFT), /**< !!str    A sequence of zero or more Unicode characters. https://yaml.org/type/str.html */
    VTAG_TIME   = c4bit(12+_VTAG_SHIFT), /**< !!timestamp A point in time https://yaml.org/type/timestamp.html */
    VTAG_VALUE  = c4bit(13+_VTAG_SHIFT), /**< !!value  Specify the default value of a mapping https://yaml.org/type/value.html */
    VTAG_YAML   = c4bit(14+_VTAG_SHIFT), /**< !!yaml   Specify the default value of a mapping https://yaml.org/type/yaml.html */
#endif // C4_WORK_IN_PROGRESS_

#undef c4bit

} NodeType_e;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** wraps a NodeType_e element with some syntactic sugar and predicates */
struct NodeType
{
public:

    NodeType_e type;

public:

    inline operator NodeType_e      & C4_RESTRICT ()       { return type; }
    inline operator NodeType_e const& C4_RESTRICT () const { return type; }

    NodeType() : type(NOTYPE) {}
    NodeType(NodeType_e t) : type(t) {}
    NodeType(type_bits t) : type((NodeType_e)t) {}

    const char *type_str() const { return type_str(type); }
    static const char* type_str(NodeType_e t);

    void set(NodeType_e t) { type = t; }
    void set(type_bits  t) { type = (NodeType_e)t; }

    void add(NodeType_e t) { type = (NodeType_e)(type|t); }
    void add(type_bits  t) { type = (NodeType_e)(type|t); }

    void rem(NodeType_e t) { type = (NodeType_e)(type & ~t); }
    void rem(type_bits  t) { type = (NodeType_e)(type & ~t); }

    void clear() { type = NOTYPE; }

public:

    #if defined(__clang__)
    #   pragma clang diagnostic push
    #   pragma clang diagnostic ignored "-Wnull-dereference"
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic push
    #   if __GNUC__ >= 6
    #       pragma GCC diagnostic ignored "-Wnull-dereference"
    #   endif
    #endif

    bool is_stream() const { return ((type & STREAM) == STREAM) != 0; }
    bool is_doc() const { return (type & DOC) != 0; }
    bool is_container() const { return (type & (MAP|SEQ|STREAM|DOC)) != 0; }
    bool is_map() const { return (type & MAP) != 0; }
    bool is_seq() const { return (type & SEQ) != 0; }
    bool has_val() const { return (type & VAL) != 0; }
    bool has_key() const { return (type & KEY) != 0; }
    bool is_val() const { return (type & KEYVAL) == VAL; }
    bool is_keyval() const { return (type & KEYVAL) == KEYVAL; }
    bool has_key_tag() const { return (type & (KEY|KEYTAG)) == (KEY|KEYTAG); }
    bool has_val_tag() const { return ((type & (VALTAG)) && (type & (VAL|MAP|SEQ))); }
    bool has_key_anchor() const { return (type & KEYANCH) != 0; }
    bool has_val_anchor() const { return (type & VALANCH) != 0; }
    bool has_anchor() const { return (type & (KEYANCH|VALANCH)) != 0; }
    bool is_key_ref() const { return (type & KEYREF) != 0; }
    bool is_val_ref() const { return (type & VALREF) != 0; }
    bool is_ref() const { return (type & (KEYREF|VALREF)) != 0; }

    #if defined(__clang__)
    #   pragma clang diagnostic pop
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic pop
    #endif

};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** a node scalar is a csubstr, which may be tagged and anchored. */
struct NodeScalar
{
    csubstr tag;
    csubstr scalar;
    csubstr anchor;

public:

    /// initialize as an empty scalar
    inline NodeScalar() noexcept : tag(), scalar(), anchor() {}

    /// initialize as an untagged scalar
    template<size_t N>
    inline NodeScalar(const char (&s)[N]) noexcept : tag(), scalar(s), anchor() {}
    inline NodeScalar(csubstr      s    ) noexcept : tag(), scalar(s), anchor() {}

    /// initialize as a tagged scalar
    template<size_t N, size_t M>
    inline NodeScalar(const char (&t)[N], const char (&s)[N]) noexcept : tag(t), scalar(s), anchor() {}
    inline NodeScalar(csubstr      t    , csubstr      s    ) noexcept : tag(t), scalar(s), anchor() {}

public:

    ~NodeScalar() noexcept = default;
    NodeScalar(NodeScalar &&) noexcept = default;
    NodeScalar(NodeScalar const&) noexcept = default;
    NodeScalar& operator= (NodeScalar &&) noexcept = default;
    NodeScalar& operator= (NodeScalar const&) noexcept = default;

public:

    bool empty() const noexcept { return tag.empty() && scalar.empty() && anchor.empty(); }

    void clear() noexcept { tag.clear(); scalar.clear(); anchor.clear(); }

};
C4_MUST_BE_TRIVIAL_COPY(NodeScalar);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** convenience class to initialize nodes */
struct NodeInit
{

    NodeType   type;
    NodeScalar key;
    NodeScalar val;

public:

    /// initialize as an empty node
    NodeInit() : type(NOTYPE), key(), val() {}
    /// initialize as a typed node
    NodeInit(NodeType_e t) : type(t), key(), val() {}
    /// initialize as a sequence member
    NodeInit(NodeScalar const& v) : type(VAL), key(), val(v) { _add_flags(); }
    /// initialize as a mapping member
    NodeInit(              NodeScalar const& k, NodeScalar const& v) : type(KEYVAL), key(k.tag, k.scalar), val(v.tag, v.scalar) { _add_flags(); }
    /// initialize as a mapping member with explicit type
    NodeInit(NodeType_e t, NodeScalar const& k, NodeScalar const& v) : type(t     ), key(k.tag, k.scalar), val(v.tag, v.scalar) { _add_flags(); }
    /// initialize as a mapping member with explicit type (eg SEQ or MAP)
    NodeInit(NodeType_e t, NodeScalar const& k                     ) : type(t     ), key(k.tag, k.scalar), val(               ) { _add_flags(KEY); }

public:

    void clear()
    {
        type.clear();
        key.clear();
        val.clear();
    }

    void _add_flags(type_bits more_flags=0)
    {
        type = (type|more_flags);
        if( ! key.tag.empty()) type = (type|KEYTAG);
        if( ! val.tag.empty()) type = (type|VALTAG);
        if( ! key.anchor.empty()) type = (type|KEYANCH);
        if( ! val.anchor.empty()) type = (type|VALANCH);
    }

    bool _check() const
    {
        // key cannot be empty
        RYML_ASSERT(key.scalar.empty() == ((type & KEY) == 0));
        // key tag cannot be empty
        RYML_ASSERT(key.tag.empty() == ((type & KEYTAG) == 0));
        // val may be empty even though VAL is set. But when VAL is not set, val must be empty
        RYML_ASSERT(((type & VAL) != 0) || val.scalar.empty());
        // val tag cannot be empty
        RYML_ASSERT(val.tag.empty() == ((type & VALTAG) == 0));
        return true;
    }
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

/** contains the data for each YAML node. */
struct NodeData
{

    NodeType   m_type;

    NodeScalar m_key;
    NodeScalar m_val;

    size_t     m_parent;
    size_t     m_first_child;
    size_t     m_last_child;
    size_t     m_next_sibling;
    size_t     m_prev_sibling;

public:

    NodeType_e type() const { return (NodeType_e)(m_type & _TYMASK); }
    const char* type_str() const { return type_str(m_type); }
    RYML_EXPORT static const char* type_str(NodeType_e ty);

    csubstr const& key() const { RYML_ASSERT(has_key()); return m_key.scalar; }
    csubstr const& key_tag() const { RYML_ASSERT(has_key_tag()); return m_key.tag; }
    csubstr const& key_anchor() const { return m_key.anchor; }
    NodeScalar const& keysc() const { RYML_ASSERT(has_key()); return m_key; }

    csubstr const& val() const { RYML_ASSERT(has_val()); return m_val.scalar; }
    csubstr const& val_tag() const { RYML_ASSERT(has_val_tag()); return m_val.tag; }
    csubstr const& val_anchor() const { RYML_ASSERT(has_val_tag()); return m_val.anchor; }
    NodeScalar const& valsc() const { RYML_ASSERT(has_val()); return m_val; }

public:

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wnull-dereference"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#   if __GNUC__ >= 6
#       pragma GCC diagnostic ignored "-Wnull-dereference"
#   endif
#endif

    bool   is_root() const { return m_parent == NONE; }

    bool   is_stream() const { return m_type.is_stream(); }
    bool   is_doc() const { return m_type.is_doc(); }
    bool   is_container() const { return m_type.is_container(); }
    bool   is_map() const { return m_type.is_map(); }
    bool   is_seq() const { return m_type.is_seq(); }
    bool   has_val() const { return m_type.has_val(); }
    bool   has_key() const { return m_type.has_key(); }
    bool   is_val() const { return m_type.is_val(); }
    bool   is_keyval() const { return m_type.is_keyval(); }
    bool   has_key_tag() const { return m_type.has_key_tag(); }
    bool   has_val_tag() const { return m_type.has_val_tag(); }
    bool   has_key_anchor() const { return ! m_type.has_key_anchor(); }
    bool   has_val_anchor() const { return ! m_type.has_val_anchor(); }
    bool   is_key_ref() const { return m_type.is_key_ref(); }
    bool   is_val_ref() const { return m_type.is_val_ref(); }

#if defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif

};
C4_MUST_BE_TRIVIAL_COPY(NodeData);


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

class Tree
{
public:

    Tree(Allocator const& cb);
    Tree() : Tree(Allocator()) {}
    Tree(size_t node_capacity, size_t arena_capacity=0, Allocator const& cb={});

    ~Tree();

    Tree(Tree const& that) noexcept;
    Tree(Tree     && that) noexcept;

    Tree& operator= (Tree const& that) noexcept;
    Tree& operator= (Tree     && that) noexcept;

public:

    void reserve(size_t node_capacity);

    /** clear the tree and zero every node
     * @note does NOT clear the arena
     * @see clear_arena() */
    void clear();
    inline void clear_arena() { m_arena_pos = 0; }

    inline bool   empty() const { return m_size == 0; }

    inline size_t size () const { return m_size; }
    inline size_t capacity() const { return m_cap; }
    inline size_t slack() const { RYML_ASSERT(m_cap >= m_size); return m_cap - m_size; }

    inline size_t arena_size() const { return m_arena_pos; }
    inline size_t arena_capacity() const { return m_arena.len; }
    inline size_t arena_slack() const { RYML_ASSERT(m_arena.len >= m_arena_pos); return m_arena.len - m_arena_pos; }

    Allocator const& allocator() const { return m_alloc; }

public:

    size_t id(NodeData const* n)
    {
        if( ! n) return NONE;
        RYML_ASSERT(n >= m_buf && n < m_buf + m_cap);
        return static_cast<size_t>(n - m_buf);
    }
    size_t id(NodeData const* n) const
    {
        if( ! n) return NONE;
        RYML_ASSERT(n >= m_buf && n < m_buf + m_cap);
        return static_cast<size_t>(n - m_buf);
    }

    // with the get() method, i can be NONE, in which case a nullptr is returned
    inline NodeData *get(size_t i)
    {
        if(i == NONE) return nullptr;
        RYML_ASSERT(i >= 0 && i < m_cap);
        return m_buf + i;
    }
    inline NodeData const *get(size_t i) const
    {
        if(i == NONE) return nullptr;
        RYML_ASSERT(i >= 0 && i < m_cap);
        return m_buf + i;
    }

    // these next two functions are implementation only; use at your
    // own risk.

    // An if-less form of get() that demands a valid node index
    inline NodeData       * _p(size_t i)       { RYML_ASSERT(i != NONE && i >= 0 && i < m_cap); return m_buf + i; }
    // An if-less form of get() that demands a valid node index
    inline NodeData const * _p(size_t i) const { RYML_ASSERT(i != NONE && i >= 0 && i < m_cap); return m_buf + i; }

public:

    //! Get the id of the root node
    size_t root_id()       { if(m_cap == 0) { reserve(16); } RYML_ASSERT(m_cap > 0 && m_size > 0); return 0; }
    //! Get the id of the root node
    size_t root_id() const {                                 RYML_ASSERT(m_cap > 0 && m_size > 0); return 0; }

    //! Get the root as a NodeRef
    NodeRef       rootref();
    //! Get the root as a NodeRef
    NodeRef const rootref() const;

    //! find a root child by name, return it as a NodeRef
    //! @note requires the root to be a map.
    NodeRef       operator[] (csubstr key);
    //! find a root child by name, return it as a NodeRef
    //! @note requires the root to be a map.
    NodeRef const operator[] (csubstr key) const;

    //! find a root child by index: return the root node's @p i-th child as a NodeRef
    //! @note @i is NOT the node id, but the child's position
    NodeRef       operator[] (size_t i);
    //! find a root child by index: return the root node's @p i-th child as a NodeRef
    //! @note @i is NOT the node id, but the child's position
    NodeRef const operator[] (size_t i) const;

public:

    NodeType_e  type(size_t node) const { return (NodeType_e)(_p(node)->m_type & _TYMASK); }
    const char* type_str(size_t node) const { return NodeType::type_str(_p(node)->m_type); }

    csubstr    const& key       (size_t node) const { RYML_ASSERT(has_key(node)); return _p(node)->m_key.scalar; }
    csubstr    const& key_tag   (size_t node) const { RYML_ASSERT(has_key_tag(node)); return _p(node)->m_key.tag; }
    csubstr    const& key_ref   (size_t node) const { RYML_ASSERT(is_key_ref(node) && ! has_key_anchor(node)); return _p(node)->m_key.anchor; }
    csubstr    const& key_anchor(size_t node) const { RYML_ASSERT( ! is_key_ref(node) && has_key_anchor(node)); return _p(node)->m_key.anchor; }
    NodeScalar const& keysc     (size_t node) const { RYML_ASSERT(has_key(node)); return _p(node)->m_key; }

    csubstr    const& val       (size_t node) const { RYML_ASSERT(has_val(node)); return _p(node)->m_val.scalar; }
    csubstr    const& val_tag   (size_t node) const { RYML_ASSERT(has_val_tag(node)); return _p(node)->m_val.tag; }
    csubstr    const& val_ref   (size_t node) const { RYML_ASSERT(is_val_ref(node) && ! has_val_anchor(node)); return _p(node)->m_val.anchor; }
    csubstr    const& val_anchor(size_t node) const { RYML_ASSERT( ! is_val_ref(node) && has_val_anchor(node)); return _p(node)->m_val.anchor; }
    NodeScalar const& valsc     (size_t node) const { RYML_ASSERT(has_val(node)); return _p(node)->m_val; }

    /** Resolve references (aliases <- anchors) in the tree.
     *
     * Dereferencing is opt-in; after parsing, you have to call
     * Tree::resolve() explicitly if you want resolved references in the
     * tree. This method will resolve all references and substitute the
     * anchored values in place of the reference.
     *
     * This method first does a full traversal of the tree to gather all
     * anchors and references in a separate collection, then it goes through
     * that collection to locate the names, which it does by obeying the YAML
     * standard diktat that "an alias node refers to the most recent node in
     * the serialization having the specified anchor"
     *
     * So, depending on the number of anchor/alias nodes, this is a
     * potentially expensive operation, with a best-case linear complexity
     * (from the initial traversal). This potential cost is the reason for
     * requiring an explicit call.
     */
    void resolve();

public:

    // node predicates

    bool is_root(size_t node) const { RYML_ASSERT(_p(node)->m_parent != NONE || node == 0); return _p(node)->m_parent == NONE; }
    bool is_stream(size_t node) const { return (_p(node)->m_type & STREAM) == STREAM; }
    bool is_doc(size_t node) const { return (_p(node)->m_type & DOC) != 0; }
    bool is_container(size_t node) const { return (_p(node)->m_type & (MAP|SEQ|STREAM|DOC)) != 0; }
    bool is_map(size_t node) const { return (_p(node)->m_type & MAP) != 0; }
    bool is_seq(size_t node) const { return (_p(node)->m_type & SEQ) != 0; }
    bool has_val(size_t node) const { return (_p(node)->m_type & VAL) != 0; }
    bool has_key(size_t node) const { return (_p(node)->m_type & KEY) != 0; }
    bool is_val(size_t node) const { return (_p(node)->m_type & KEYVAL) == VAL; }
    bool is_keyval(size_t node) const { return (_p(node)->m_type & KEYVAL) == KEYVAL; }
    bool has_key_tag(size_t node) const { return (_p(node)->m_type & (KEY|KEYTAG)) == (KEY|KEYTAG); }
    bool has_val_tag(size_t node) const { return ((_p(node)->m_type & (VALTAG)) && (_p(node)->m_type & (VAL|MAP|SEQ))); }
    bool has_key_anchor(size_t node) const { return (_p(node)->m_type & KEYANCH) != 0; }
    bool has_val_anchor(size_t node) const { return (_p(node)->m_type & VALANCH) != 0; }
    bool is_key_ref(size_t node) const { return (_p(node)->m_type & KEYREF) != 0; }
    bool is_val_ref(size_t node) const { return (_p(node)->m_type & VALREF) != 0; }
    bool is_ref(size_t node) const { return (_p(node)->m_type & (KEYREF|VALREF)) != 0; }
    bool is_anchor(size_t node) const { return (_p(node)->m_type & (KEYANCH|VALANCH)) != 0; }

    bool parent_is_seq(size_t node) const { RYML_ASSERT(has_parent(node)); return is_seq(_p(node)->m_parent); }
    bool parent_is_map(size_t node) const { RYML_ASSERT(has_parent(node)); return is_map(_p(node)->m_parent); }

    /** true when name and value are empty, and has no children */
    bool empty(size_t node) const { return ! has_children(node) && _p(node)->m_key.empty() && (( ! (_p(node)->m_type & VAL)) || _p(node)->m_val.empty()); }
    /** true when the node has an anchor named a */
    bool has_anchor(size_t node, csubstr a) const { return _p(node)->m_key.anchor == a || _p(node)->m_val.anchor == a; }

public:

    // hierarchy predicates

    bool has_parent(size_t node) const { return _p(node)->m_parent != NONE; }

    bool has_child(size_t node, csubstr key) const { return find_child(node, key) != npos; }
    bool has_child(size_t node, size_t ch) const { return child_pos(node, ch) != npos; }
    bool has_children(size_t node) const { return _p(node)->m_first_child != NONE; }

    bool has_sibling(size_t node, size_t sib) const { return is_root(node) ? sib==node : child_pos(_p(node)->m_parent, sib) != npos; }
    bool has_sibling(size_t node, csubstr key) const { return find_sibling(node, key) != npos; }
    /** counts with *this */
    bool has_siblings(size_t /*node*/) const { return true; }
    /** does not count with *this */
    bool has_other_siblings(size_t node) const { return is_root(node) ? false : (_p(_p(node)->m_parent)->m_first_child != _p(_p(node)->m_parent)->m_last_child); }

public:

    // hierarchy getters

    size_t parent(size_t node) const { return _p(node)->m_parent; }

    size_t prev_sibling(size_t node) const { return _p(node)->m_prev_sibling; }
    size_t next_sibling(size_t node) const { return _p(node)->m_next_sibling; }

    /** O(#num_children) */
    size_t num_children(size_t node) const;
    size_t child_pos(size_t node, size_t ch) const;
    size_t first_child(size_t node) const { return _p(node)->m_first_child; }
    size_t last_child(size_t node) const { return _p(node)->m_last_child; }
    size_t child(size_t node, size_t pos) const;
    size_t find_child(size_t node, csubstr const& key) const;

    /** O(#num_siblings) */
    /** counts with this */
    size_t num_siblings(size_t node) const { return is_root(node) ? 1 : num_children(_p(node)->m_parent); }
    /** does not count with this */
    size_t num_other_siblings(size_t node) const { size_t ns = num_siblings(node); RYML_ASSERT(ns > 0); return ns-1; }
    size_t sibling_pos(size_t node, size_t sib) const { RYML_ASSERT( ! is_root(node) || node == root_id()); return child_pos(_p(node)->m_parent, sib); }
    size_t first_sibling(size_t node) const { return is_root(node) ? node : _p(_p(node)->m_parent)->m_first_child; }
    size_t last_sibling(size_t node) const { return is_root(node) ? node : _p(_p(node)->m_parent)->m_last_child; }
    size_t sibling(size_t node, size_t pos) const { return child(_p(node)->m_parent, pos); }
    size_t find_sibling(size_t node, csubstr const& key) const { return find_child(_p(node)->m_parent, key); }

public:

    void to_keyval(size_t node, csubstr const& key, csubstr const& val, type_bits more_flags=0);
    void to_map(size_t node, csubstr const& key, type_bits more_flags=0);
    void to_seq(size_t node, csubstr const& key, type_bits more_flags=0);
    void to_val(size_t node, csubstr const& val, type_bits more_flags=0);
    void to_map(size_t node, type_bits more_flags=0);
    void to_seq(size_t node, type_bits more_flags=0);
    void to_doc(size_t node, type_bits more_flags=0);
    void to_stream(size_t node, type_bits more_flags=0);

    void set_key(size_t node, csubstr key) { RYML_ASSERT(has_key(node)); _p(node)->m_key.scalar = key; }
    void set_val(size_t node, csubstr val) { RYML_ASSERT(has_val(node)); _p(node)->m_val.scalar = val; }

    void set_key_tag(size_t node, csubstr tag) { RYML_ASSERT(has_key(node)); _p(node)->m_key.tag = tag; _add_flags(node, KEYTAG); }
    void set_val_tag(size_t node, csubstr tag) { RYML_ASSERT(has_val(node) || is_container(node)); _p(node)->m_val.tag = tag; _add_flags(node, VALTAG); }

    void set_key_anchor(size_t node, csubstr anchor) { RYML_ASSERT( ! is_key_ref(node)); _p(node)->m_key.anchor = anchor; _add_flags(node, KEYANCH); }
    void set_val_anchor(size_t node, csubstr anchor) { RYML_ASSERT( ! is_val_ref(node)); _p(node)->m_val.anchor = anchor; _add_flags(node, VALANCH); }
    void set_key_ref   (size_t node, csubstr ref   ) { RYML_ASSERT( ! has_key_anchor(node)); _p(node)->m_key.anchor = ref; _add_flags(node, KEYREF); }
    void set_val_ref   (size_t node, csubstr ref   ) { RYML_ASSERT( ! has_val_anchor(node)); _p(node)->m_val.anchor = ref; _add_flags(node, VALREF); }

    void rem_key_anchor(size_t node) { _p(node)->m_key.anchor.clear(); _rem_flags(node, KEYANCH); }
    void rem_val_anchor(size_t node) { _p(node)->m_val.anchor.clear(); _rem_flags(node, VALANCH); }
    void rem_key_ref   (size_t node) { _p(node)->m_key.anchor.clear(); _rem_flags(node, KEYREF); }
    void rem_val_ref   (size_t node) { _p(node)->m_val.anchor.clear(); _rem_flags(node, VALREF); }
    void rem_anchor_ref(size_t node) { _p(node)->m_key.anchor.clear(); _p(node)->m_val.anchor.clear(); _rem_flags(node, KEYANCH|VALANCH|KEYREF|VALREF); }

public:

    /** create and insert a new child of "parent". insert after the (to-be)
     * sibling "after", which must be a child of "parent". To insert as the
     * first child, set after to NONE */
    inline size_t insert_child(size_t parent, size_t after)
    {
        RYML_ASSERT(parent != NONE);
        RYML_ASSERT(is_container(parent) || is_root(parent));
        RYML_ASSERT(after == NONE || has_child(parent, after));
        size_t child = _claim();
        _set_hierarchy(child, parent, after);
        return child;
    }
    inline size_t prepend_child(size_t parent) { return insert_child(parent, NONE); }
    inline size_t  append_child(size_t parent) { return insert_child(parent, last_child(parent)); }

public:

    #if defined(__clang__)
    #   pragma clang diagnostic push
    #   pragma clang diagnostic ignored "-Wnull-dereference"
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic push
    #   if __GNUC__ >= 6
    #       pragma GCC diagnostic ignored "-Wnull-dereference"
    #   endif
    #endif

    //! create and insert a new sibling of n. insert after "after"
    inline size_t insert_sibling(size_t node, size_t after)
    {
        RYML_ASSERT(node != NONE);
        RYML_ASSERT( ! is_root(node));
        RYML_ASSERT(parent(node) != NONE);
        RYML_ASSERT(after == NONE || (has_sibling(node, after) && has_sibling(after, node)));
        RYML_ASSERT(get(node) != nullptr);
        return insert_child(get(node)->m_parent, after);
    }
    inline size_t prepend_sibling(size_t node) { return insert_sibling(node, NONE); }
    inline size_t  append_sibling(size_t node) { return insert_sibling(node, last_sibling(node)); }

public:

    //! remove an entire branch at once: ie remove the children and the node itself
    inline void remove(size_t node)
    {
        remove_children(node);
        _release(node);
    }

    //! remove all the node's children, but keep the node itself
    void remove_children(size_t node)
    {
        RYML_ASSERT(get(node) != nullptr);
        size_t ich = get(node)->m_first_child;
        while(ich != NONE)
        {
            remove_children(ich);
            RYML_ASSERT(get(ich) != nullptr);
            size_t next = get(ich)->m_next_sibling;
            _release(ich);
            if(ich == get(node)->m_last_child) break;
            ich = next;
        }
    }
    
    #if defined(__clang__)
    #   pragma clang diagnostic pop
    #elif defined(__GNUC__)
    #   pragma GCC diagnostic pop
    #endif

public:

    /** reorder the tree in memory so that all the nodes are stored
     * in a linear sequence when visited in depth-first order */
    void reorder();

    /** change the node's position in the parent */
    void move(size_t node, size_t after);

    /** change the node's parent and position */
    void move(size_t node, size_t new_parent, size_t after);

    /** change the node's parent and position to a different tree
     * @return the index of the new node in the destination tree */
    size_t move(Tree * src, size_t node, size_t new_parent, size_t after);

public:

    /** recursively duplicate a node from this tree into a new parent,
     * placing it after one of its children
     * @return the index of the copy */
    size_t duplicate(size_t node, size_t new_parent, size_t after);
    /** recursively duplicate a node from a different tree into a new parent,
     * placing it after one of its children
     * @return the index of the copy */
    size_t duplicate(Tree const* src, size_t node, size_t new_parent, size_t after);

    /** recursively duplicate the node's children (but not the node)
     * @return the index of the last duplicated child */
    size_t duplicate_children(size_t node, size_t parent, size_t after);
    /** recursively duplicate the node's children (but not the node), where
     * the node is from a different tree
     * @return the index of the last duplicated child */
    size_t duplicate_children(Tree const* src, size_t node, size_t parent, size_t after);

    void duplicate_contents(size_t node, size_t where);
    void duplicate_contents(Tree const* src, size_t node, size_t where);

    /** duplicate the node's children (but not the node) in a new parent, but
     * omit repetitions where a duplicated node has the same key (in maps) or
     * value (in seqs). If one of the duplicated children has the same key
     * (in maps) or value (in seqs) as one of the parent's children, the one
     * that is placed closest to the end will prevail. */
    size_t duplicate_children_no_rep(size_t node, size_t parent, size_t after);
    size_t duplicate_children_no_rep(Tree const* src, size_t node, size_t parent, size_t after);

public:

    void merge_with(Tree const* src, size_t src_node=NONE, size_t dst_root=NONE);

public:

    substr arena() const { return m_arena.range(0, m_arena_pos); }
    size_t arena_pos() const { return m_arena_pos; }

    template<class T>
    csubstr to_arena(T const& a)
    {
        substr rem(m_arena.sub(m_arena_pos));
        size_t num = to_chars(rem, a);
        if(num > rem.len)
        {
            rem = _grow_arena(num);
            num = to_chars(rem, a);
            RYML_ASSERT(num <= rem.len);
        }
        rem = _request_span(num);
        return rem;
    }

    bool in_arena(csubstr s) const
    {
        return m_arena.is_super(s);
    }

    substr alloc_arena(size_t sz)
    {
        if(sz >= arena_slack())
        {
            _grow_arena(sz - arena_slack());
        }
        substr s = _request_span(sz);
        return s;
    }

    substr copy_to_arena(csubstr s)
    {
        substr cp = alloc_arena(s.len);
        RYML_ASSERT(cp.len == s.len);
        RYML_ASSERT(!s.overlaps(cp));
        #if defined(__clang__)
        #elif defined(__GNUC__)
        #   pragma GCC diagnostic push
        #   if __GNUC__ >= 10
        #       pragma GCC diagnostic ignored "-Wstringop-overflow=" // no need for terminating \0
        #       pragma GCC diagnostic ignored "-Wrestrict" // there's an assert above covering violation of restrict behavior
        #   endif
        #endif
        memcpy(cp.str, s.str, s.len);
        #if defined(__clang__)
        #elif defined(__GNUC__)
        #   pragma GCC diagnostic pop
        #endif
        return cp;
    }

    void reserve_arena(size_t arena_cap)
    {
        if(arena_cap > m_arena.len)
        {
            substr buf;
            buf.str = (char*) m_alloc.allocate(arena_cap, m_arena.str);
            buf.len = arena_cap;
            if(m_arena.str)
            {
                RYML_ASSERT(m_arena.len >= 0);
                _relocate(buf); // does a memcpy and changes nodes using the arena
                m_alloc.free(m_arena.str, m_arena.len);
            }
            m_arena = buf;
        }
    }

public:

    struct lookup_result
    {
        size_t  target;
        size_t  closest;
        size_t  path_pos;
        csubstr path;

        inline operator bool() const { return target != NONE; }

        lookup_result() : target(NONE), closest(NONE), path_pos(0), path() {}
        lookup_result(csubstr path_, size_t start) : target(NONE), closest(start), path_pos(0), path(path_) {}

        csubstr resolved() const;
        csubstr unresolved() const;
    };

    /** for example foo.bar[0].baz */
    lookup_result lookup_path(csubstr path, size_t start=NONE) const;

    /** defaulted lookup: lookup path; if the lookup fails, recursively modify
     * the tree so that the corresponding lookup_path() would return the 
     * default value */
    size_t lookup_path_or_modify(csubstr default_value, csubstr path, size_t start=NONE);

private:

    struct _lookup_path_token
    {
        csubstr value;
        NodeType type;
        _lookup_path_token() : value(), type() {}
        _lookup_path_token(csubstr v, NodeType t) : value(v), type(t) {}
        inline operator bool() const { return type != NOTYPE; }
        bool is_index() const { return value.begins_with('[') && value.ends_with(']'); }
    };

    void _lookup_path(lookup_result *r, bool modify);
    size_t _next_node(lookup_result *r, bool modify, _lookup_path_token *parent);
    _lookup_path_token _next_token(lookup_result *r, _lookup_path_token const& parent);
    void _advance(lookup_result *r, size_t more);

private:

    substr _grow_arena(size_t more)
    {
        size_t cap = m_arena_pos + more;
        cap = cap < 2 * m_arena.len ? 2 * m_arena.len : cap;
        cap = cap < 64 ? 64 : cap;
        reserve_arena(cap);
        return m_arena.sub(m_arena_pos);
    }

    substr _request_span(size_t sz)
    {
        substr s;
        s = m_arena.sub(m_arena_pos, sz);
        m_arena_pos += sz;
        return s;
    }

    substr _relocated(csubstr s, substr next_arena) const
    {
        RYML_ASSERT(m_arena.is_super(s));
        RYML_ASSERT(m_arena.sub(0, m_arena_pos).is_super(s));
        auto pos = (s.str - m_arena.str);
        substr r(next_arena.str + pos, s.len);
        RYML_ASSERT(r.str - next_arena.str == pos);
        RYML_ASSERT(next_arena.sub(0, m_arena_pos).is_super(r));
        return r;
    }

    void _clear();
    void _free();
    void _copy(Tree const& that);
    void _move(Tree      & that);

    void _relocate(substr next_arena);

public:

    #if ! RYML_USE_ASSERT
    #define _check_next_flags(node, f)
    #else
    inline void _check_next_flags(size_t node, type_bits f)
    {
        auto n = _p(node);
        type_bits o = n->m_type; // old
        C4_UNUSED(o);
        if(f & MAP)
        {
            RYML_ASSERT_MSG((f & SEQ) == 0, "cannot mark simultaneously as map and seq");
            RYML_ASSERT_MSG((f & VAL) == 0, "cannot mark simultaneously as map and val");
            RYML_ASSERT_MSG((o & SEQ) == 0, "cannot turn a seq into a map; clear first");
            RYML_ASSERT_MSG((o & VAL) == 0, "cannot turn a val into a map; clear first");
        }
        else if(f & SEQ)
        {
            RYML_ASSERT_MSG((f & MAP) == 0, "cannot mark simultaneously as seq and map");
            RYML_ASSERT_MSG((f & VAL) == 0, "cannot mark simultaneously as seq and val");
            RYML_ASSERT_MSG((o & MAP) == 0, "cannot turn a map into a seq; clear first");
            RYML_ASSERT_MSG((o & VAL) == 0, "cannot turn a val into a seq; clear first");
        }
        if(f & KEY)
        {
            RYML_ASSERT(!is_root(node));
            auto pid = parent(node); C4_UNUSED(pid);
            RYML_ASSERT(is_map(pid));
        }
        if(f & VAL)
        {
            RYML_ASSERT(!is_root(node));
            auto pid = parent(node); C4_UNUSED(pid);
            RYML_ASSERT(is_map(pid) || is_seq(pid));
        }
    }
    #endif

    inline void _set_flags(size_t node, NodeType_e f) { _check_next_flags(node, f); _p(node)->m_type = f; }
    inline void _set_flags(size_t node, type_bits  f) { _check_next_flags(node, f); _p(node)->m_type = f; }

    inline void _add_flags(size_t node, NodeType_e f) { NodeData *d = _p(node); type_bits fb = f | d->m_type; _check_next_flags(node, fb); d->m_type = (NodeType_e) fb; }
    inline void _add_flags(size_t node, type_bits  f) { NodeData *d = _p(node);               f |= d->m_type; _check_next_flags(node,  f); d->m_type = f; }

    inline void _rem_flags(size_t node, NodeType_e f) { NodeData *d = _p(node); type_bits fb = d->m_type & ~f; _check_next_flags(node, fb); d->m_type = (NodeType_e) fb; }
    inline void _rem_flags(size_t node, type_bits  f) { NodeData *d = _p(node);            f = d->m_type & ~f; _check_next_flags(node,  f); d->m_type = f; }

    void _set_key(size_t node, csubstr const& key, type_bits more_flags=0)
    {
        _p(node)->m_key.scalar = key;
        _add_flags(node, KEY|more_flags);
    }
    void _set_key(size_t node, NodeScalar const& key, type_bits more_flags=0)
    {
        _p(node)->m_key = key;
        _add_flags(node, KEY|more_flags);
    }

    void _set_val(size_t node, csubstr const& val, type_bits more_flags=0)
    {
        RYML_ASSERT(num_children(node) == 0);
        RYML_ASSERT( ! is_container(node));
        _p(node)->m_val.scalar = val;
        _add_flags(node, VAL|more_flags);
    }
    void _set_val(size_t node, NodeScalar const& val, type_bits more_flags=0)
    {
        RYML_ASSERT(num_children(node) == 0);
        RYML_ASSERT( ! is_container(node));
        _p(node)->m_val = val;
        _add_flags(node, VAL|more_flags);
    }

    void _set(size_t node, NodeInit const& i)
    {
        RYML_ASSERT(i._check());
        NodeData *n = _p(node);
        RYML_ASSERT(n->m_key.scalar.empty() || i.key.scalar.empty() || i.key.scalar == n->m_key.scalar);
        _add_flags(node, i.type);
        if(n->m_key.scalar.empty())
        {
            if( ! i.key.scalar.empty())
            {
                _set_key(node, i.key.scalar);
            }
        }
        n->m_key.tag = i.key.tag;
        n->m_val = i.val;
    }

    void _set_parent_as_container_if_needed(size_t in)
    {
        NodeData const* n = _p(in);
        size_t ip = parent(in);
        if(ip != NONE)
        {
            if( ! (is_seq(ip) || is_map(ip)))
            {
                if((in == first_child(ip)) && (in == last_child(ip)))
                {
                    if( ! n->m_key.empty() || n->has_key())
                    {
                        _add_flags(ip, MAP);
                    }
                    else
                    {
                        _add_flags(ip, SEQ);
                    }
                }
            }
        }
    }

    void _seq2map(size_t node)
    {
        RYML_ASSERT(is_seq(node));
        for(size_t i = first_child(node); i != NONE; i = next_sibling(i))
        {
            NodeData *C4_RESTRICT ch = _p(i);
            if(ch->m_type.is_keyval()) continue;
            ch->m_type.add(KEY);
            ch->m_key = ch->m_val;
        }
        auto *C4_RESTRICT n = _p(node);
        n->m_type.rem(SEQ);
        n->m_type.add(MAP);
    }

    size_t _do_reorder(size_t *node, size_t count);

    void _swap(size_t n_, size_t m_);
    void _swap_props(size_t n_, size_t m_);
    void _swap_hierarchy(size_t n_, size_t m_);
    void _copy_hierarchy(size_t dst_, size_t src_);

    void _copy_props(size_t dst_, size_t src_)
    {
        auto      & C4_RESTRICT dst = *_p(dst_);
        auto const& C4_RESTRICT src = *_p(src_);
        dst.m_type = src.m_type;
        dst.m_key  = src.m_key;
        dst.m_val  = src.m_val;
    }

    void _copy_props_wo_key(size_t dst_, size_t src_)
    {
        auto      & C4_RESTRICT dst = *_p(dst_);
        auto const& C4_RESTRICT src = *_p(src_);
        dst.m_type = src.m_type;
        dst.m_val  = src.m_val;
    }

    void _copy_props(size_t dst_, Tree const* that_tree, size_t src_)
    {
        auto      & C4_RESTRICT dst = *_p(dst_);
        auto const& C4_RESTRICT src = *that_tree->_p(src_);
        dst.m_type = src.m_type;
        dst.m_key  = src.m_key;
        dst.m_val  = src.m_val;
    }

    void _copy_props_wo_key(size_t dst_, Tree const* that_tree, size_t src_)
    {
        auto      & C4_RESTRICT dst = *_p(dst_);
        auto const& C4_RESTRICT src = *that_tree->_p(src_);
        dst.m_type = src.m_type;
        dst.m_val  = src.m_val;
    }

    inline void _clear_type(size_t node)
    {
        _p(node)->m_type = NOTYPE;
    }

    inline void _clear(size_t node)
    {
        auto *C4_RESTRICT n = _p(node);
        n->m_type = NOTYPE;
        n->m_key.clear();
        n->m_val.clear();
        n->m_parent = NONE;
        n->m_first_child = NONE;
        n->m_last_child = NONE;
    }

    inline void _clear_key(size_t node)
    {
        _p(node)->m_key.clear();
        _rem_flags(node, KEY);
    }

    inline void _clear_val(size_t node)
    {
        _p(node)->m_key.clear();
        _rem_flags(node, VAL);
    }

private:

    void _clear_range(size_t first, size_t num);

    size_t _claim();
    void   _claim_root();
    void   _release(size_t node);
    void   _free_list_add(size_t node);
    void   _free_list_rem(size_t node);

    void _set_hierarchy(size_t node, size_t parent, size_t after_sibling);
    void _rem_hierarchy(size_t node);

public:

    // members are exposed, but you should NOT access them directly

    NodeData * m_buf;
    size_t m_cap;

    size_t m_size;

    size_t m_free_head;
    size_t m_free_tail;

    substr m_arena;
    size_t m_arena_pos;

    Allocator m_alloc;

};

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif


#endif /* _C4_YML_TREE_HPP_ */
