
%module ryml


//-----------------------------------------------------------------------------
// this block will be pasted verbatim in the generated C++ source file

%{
// specifies that the resulting C file should be built as a python
// extension, inserting the module init code
#define SWIG_FILE_WITH_INIT

#include "ryml.hpp"

namespace c4 {
namespace yml {

using substr = c4::substr;
using csubstr = c4::csubstr;

} /* namespace yml */
} /* namespace c4 */

%}

//-----------------------------------------------------------------------------


%apply (const char *STRING, size_t LENGTH) { (const char *str, size_t len) };
%apply (char *STRING, size_t LENGTH) { (char *str, size_t len) };

%typemap(in) c4::substr {
#if defined(SWIGPYTHON)
  Py_buffer view;
  int ok = PyObject_CheckBuffer($input);
  if(ok)
  {
      ok = (0 == PyObject_GetBuffer($input, &view, PyBUF_SIMPLE|PyBUF_WRITABLE));
  }
  if(ok)
  {
      $1 = c4::substr((char*)view.buf, view.len);
      PyBuffer_Release(&view);
  }
  else
  {
      PyErr_SetString(PyExc_TypeError, "could not get mutable memory for c4::csubstr - have you passed a str?");
      SWIG_fail;
  }
#else
#error no "in" typemap defined for this export language
#endif
};

%typemap(in) c4::csubstr {
#if defined(SWIGPYTHON)
  Py_buffer view;
  view.buf = nullptr;
  int ok = PyObject_CheckBuffer($input);
  if(ok)
  {
      ok = (0 == PyObject_GetBuffer($input, &view, PyBUF_CONTIG_RO));
  }
  if(ok)
  {
      $1 = c4::csubstr((const char*)view.buf, view.len);
      PyBuffer_Release(&view);
  }
  else
  {
      // https://stackoverflow.com/questions/36098984/python-3-3-c-api-and-utf-8-strings
      Py_ssize_t sz = 0;
      const char *buf = PyUnicode_AsUTF8AndSize($input, &sz);
      if(buf || sz == 0)
      {
          $1 = c4::csubstr(buf, sz);
      }
      else
      {
          PyErr_SetString(PyExc_TypeError, "c4::csubstr: could not get readonly memory from python object");
          SWIG_fail;
      }
  }
#else
#error no "in" typemap defined for this export language
#endif
};
// Copy the typecheck code for "char *".
%typemap(typecheck) c4::substr = char *;
%typemap(typecheck) c4::csubstr = const char *;


%typemap(out) c4::csubstr {
#if defined(SWIGPYTHON)
  PyObject *obj = PyMemoryView_FromMemory((char*)$1.str, $1.len, PyBUF_READ);
  if( ! obj)
  {
      PyErr_SetString(PyExc_TypeError, "could not get readonly memory from c4::csubstr - have you passed a str?");
      SWIG_fail;
  }
  $result = obj;
#else
#error no "out" typemap defined for this export language
#endif
};


%inline %{

void parse_csubstr(c4::csubstr s, c4::yml::Tree *t)
{
    c4::yml::parse(s, t);
}

void parse_substr(c4::substr s, c4::yml::Tree *t)
{
    c4::yml::parse(s, t);
}


// force a roundtrip to C++, which triggers a conversion to csubstr and returns it as a memoryview
c4::csubstr _get_as_csubstr(c4::csubstr s)
{
    //printf("_get_as_csubstr: %p[%zu]'%.*s'\n", s.str, s.len, (int)s.len, s.str);
    return s;
}

c4::csubstr  _get_as_substr(c4::substr s)
{
    //printf("_get_as_substr: %p[%zu]'%.*s'\n", s.str, s.len, (int)s.len, s.str);
    return s;
}


// utilities for testing
bool _same_ptr(c4::csubstr l, c4::csubstr r)
{
    return l.str == r.str;
}

bool _same_mem(c4::csubstr l, c4::csubstr r)
{
    return l.str == r.str && l.len == r.len;
}


%}


//-----------------------------------------------------------------------------

%pythoncode %{


def as_csubstr(s):
    return _get_as_csubstr(s)

def as_substr(s):
    return _get_as_substr(s)

def u(memview):
    return str(memview, "utf8")


def children(tree, node=None):
    assert tree is not None
    if node is None: node = tree.root_id()
    ch = tree.first_child(node)
    while ch != NONE:
       yield ch
       ch = tree.next_sibling(ch)


def siblings(tree, node):
    assert tree is not None
    if node is None: return
    ch = tree.first_sibling(node)
    while ch != NONE:
       yield ch
       ch = tree.next_sibling(ch)


def walk(tree, node=None, indentation_level=0):
    assert tree is not None
    if node is None: node = tree.root_id()
    yield node, indentation_level
    ch = tree.first_child(node)
    while ch != NONE:
       for gc, il in walk(tree, ch, indentation_level + 1):
           yield gc, il
       ch = tree.next_sibling(ch)


def parse_in_situ(buf, **kwargs):
    _check_valid_for_in_situ(buf)
    return _call_parse(parse_substr, buf, **kwargs)


def parse(buf, **kwargs):
    return _call_parse(parse_csubstr, buf, **kwargs)


def _call_parse(parse_fn, buf, **kwargs):
    tree = kwargs.get("tree", Tree())
    parse_fn(buf, tree)
    return tree


def _check_valid_for_in_situ(obj):
    if type(obj) in (str, bytes):
        raise TypeError("cannot parse in situ: " + type(obj).__name__)

%}

//-----------------------------------------------------------------------------

namespace c4 {
namespace yml {

constexpr const size_t NONE = (size_t)-1;

typedef enum {
    NOTYPE  = 0,          ///< no type is set
    VAL     = (1<<0),     ///< a leaf node, has a (possibly empty) value
    KEY     = (1<<1),     ///< is member of a map, must have non-empty key
    MAP     = (1<<2),     ///< a map: a parent of keyvals
    SEQ     = (1<<3),     ///< a seq: a parent of vals
    DOC     = (1<<4),     ///< a document
    STREAM  = (1<<5)|SEQ, ///< a stream: a seq of docs
    KEYREF  = (1<<6),     ///< a *reference: the key references an &anchor
    VALREF  = (1<<7),     ///< a *reference: the val references an &anchor
    KEYANCH = (1<<8),     ///< the key has an &anchor
    VALANCH = (1<<9),     ///< the val has an &anchor
    KEYTAG  = (1<<10),    ///< the key has an explicit tag/type
    VALTAG  = (1<<11),    ///< the val has an explicit tag/type
} NodeType_e;


struct NodeType
{
    NodeType_e type;

    NodeType();
    NodeType(NodeType_e t);
    ~NodeType();

    const char *type_str();
    static const char* type_str(NodeType_e t);

    void set(NodeType_e t);
    void add(NodeType_e t);
    void rem(NodeType_e t);

    bool is_stream() const;
    bool is_doc() const;
    bool is_container() const;
    bool is_map() const;
    bool is_seq() const;
    bool has_val() const;
    bool has_key() const;
    bool is_val() const;
    bool is_keyval() const;
    bool has_key_tag() const;
    bool has_val_tag() const;
    bool has_key_anchor() const;
    bool has_val_anchor() const;
    bool has_anchor() const;
    bool is_key_ref() const;
    bool is_val_ref() const;
    bool is_ref() const;
};


struct Tree
{
    Tree();
    ~Tree();

    void reserve(size_t node_capacity);
    void reserve_arena(size_t node_capacity);
    void clear();
    void clear_arena();

    size_t size() const;
    size_t capacity() const;
    size_t slack() const;

    size_t arena_size() const;
    size_t arena_capacity() const;
    size_t arena_slack() const;

    void resolve();

public:

    // getters

    NodeType_e  type(size_t node) const;
    const char* type_str(size_t node) const;

    c4::csubstr key       (size_t node) const;
    c4::csubstr key_tag   (size_t node) const;
    c4::csubstr key_ref   (size_t node) const;
    c4::csubstr key_anchor(size_t node) const;
    c4::yml::NodeScalar keysc(size_t node) const;

    c4::csubstr val       (size_t node) const;
    c4::csubstr val_tag   (size_t node) const;
    c4::csubstr val_ref   (size_t node) const;
    c4::csubstr val_anchor(size_t node) const;
    c4::yml::NodeScalar valsc(size_t node) const;

public:

    // node predicates

    bool is_root(size_t node) const;
    bool is_stream(size_t node) const;
    bool is_doc(size_t node) const;
    bool is_container(size_t node) const;
    bool is_map(size_t node) const;
    bool is_seq(size_t node) const;
    bool has_val(size_t node) const;
    bool has_key(size_t node) const;
    bool is_val(size_t node) const;
    bool is_keyval(size_t node) const;
    bool has_key_tag(size_t node) const;
    bool has_val_tag(size_t node) const;
    bool has_key_anchor(size_t node) const;
    bool has_val_anchor(size_t node) const;
    bool is_key_ref(size_t node) const;
    bool is_val_ref(size_t node) const;
    bool is_ref(size_t node) const;
    bool is_anchor(size_t node) const;
    bool parent_is_seq(size_t node) const;
    bool parent_is_map(size_t node) const;
    bool empty(size_t node) const;
    bool has_anchor(size_t node, c4::csubstr a) const;

public:

    // hierarchy predicates

    bool has_parent(size_t node) const;
    bool has_child(size_t node, c4::csubstr key) const;
    //bool has_child(size_t node, size_t ch) const;
    bool has_children(size_t node) const;
    bool has_sibling(size_t node, c4::csubstr key) const;
    //bool has_sibling(size_t node, size_t sib) const;
    bool has_siblings(size_t node) const;
    bool has_other_siblings(size_t node) const;

public:

    // hierarchy getters

    size_t root_id() const;

    size_t parent(size_t node) const;
    size_t prev_sibling(size_t node) const;
    size_t next_sibling(size_t node) const;
    size_t num_children(size_t node) const;
    size_t child_pos(size_t node, size_t ch) const;
    size_t first_child(size_t node) const;
    size_t last_child(size_t node) const;
    size_t child(size_t node, size_t pos) const;
    size_t find_child(size_t node, c4::csubstr key) const;
    size_t num_siblings(size_t node) const;
    size_t num_other_siblings(size_t node) const;
    size_t sibling_pos(size_t node, size_t sib) const;
    size_t first_sibling(size_t node) const;
    size_t last_sibling(size_t node) const;
    size_t sibling(size_t node, size_t pos) const;
    size_t find_sibling(size_t node, c4::csubstr key) const;

public:

    void to_keyval(size_t node, c4::csubstr key, c4::csubstr val, int more_flags=0);
    void to_map(size_t node, c4::csubstr key, int more_flags=0);
    void to_seq(size_t node, c4::csubstr key, int more_flags=0);
    void to_val(size_t node, c4::csubstr val, int more_flags=0);
    void to_stream(size_t node, int more_flags=0);
    void to_map(size_t node, int more_flags=0);
    void to_seq(size_t node, int more_flags=0);
    void to_doc(size_t node, int more_flags=0);

    void set_key_tag(size_t node, c4::csubstr tag);
    void set_key_anchor(size_t node, c4::csubstr anchor);
    void set_val_anchor(size_t node, c4::csubstr anchor);
    void set_key_ref   (size_t node, c4::csubstr ref   );
    void set_val_ref   (size_t node, c4::csubstr ref   );

    void set_val_tag(size_t node, c4::csubstr tag);
    void rem_key_anchor(size_t node);
    void rem_val_anchor(size_t node);
    void rem_key_ref   (size_t node);
    void rem_val_ref   (size_t node);
    void rem_anchor_ref(size_t node);

public:

    /** create and insert a new child of "parent". insert after the (to-be)
     * sibling "after", which must be a child of "parent". To insert as the
     * first child, set after to NONE */
    size_t insert_child(size_t parent, size_t after);
    size_t prepend_child(size_t parent);
    size_t  append_child(size_t parent);

public:

    //! create and insert a new sibling of n. insert after "after"
    size_t insert_sibling(size_t node, size_t after);
    size_t prepend_sibling(size_t node);
    size_t  append_sibling(size_t node);

public:

    //! remove an entire branch at once: ie remove the children and the node itself
    void remove(size_t node);

    //! remove all the node's children, but keep the node itself
    void remove_children(size_t node);

public:

    /** change the node's position in the parent */
    void move(size_t node, size_t after);

    /** change the node's parent and position */
    void move(size_t node, size_t new_parent, size_t after);
    /** change the node's parent and position */
    size_t move(Tree * src, size_t node, size_t new_parent, size_t after);

    /** recursively duplicate the node */
    size_t duplicate(size_t node, size_t new_parent, size_t after);
    /** recursively duplicate a node from a different tree */
    size_t duplicate(Tree const* src, size_t node, size_t new_parent, size_t after);

    /** recursively duplicate the node's children (but not the node) */
    void duplicate_children(size_t node, size_t parent, size_t after);
    /** recursively duplicate the node's children (but not the node), where the node is from a different tree */
    void duplicate_children(Tree const* src, size_t node, size_t parent, size_t after);

    void duplicate_contents(size_t node, size_t where);

    /** duplicate the node's children (but not the node) in a new parent, but
     * omit repetitions where a duplicated node has the same key (in maps) or
     * value (in seqs). If one of the duplicated children has the same key
     * (in maps) or value (in seqs) as one of the parent's children, the one
     * that is placed closest to the end will prevail. */
    void duplicate_children_no_rep(size_t node, size_t parent, size_t after);

};

/*
%extend Tree {

    bool has_anchor(size_t node, const char *str, size_t len) const
    {
        return $self->has_anchor(node, c4::csubstr(str, len));
    }

    bool has_child(size_t node, const char *str, size_t len) const
    {
        return $self->has_child(node, c4::csubstr(str, len));
    }

    bool has_sibling(size_t node, const char *str, size_t len) const
    {
        return $self->has_sibling(node, c4::csubstr(str, len));
    }

    size_t find_child(size_t node, const char *str, size_t len) const
    {
        return $self->find_child(node, c4::csubstr(str, len));
    }

    size_t find_sibling(size_t node, const char *str, size_t len) const
    {
        return $self->find_sibling(node, c4::csubstr(str, len));
    }

    void to_keyval(size_t node, const char *keystr, size_t keylen, const char *valstr, size_t vallen, int more_flags=0)
    {
        return $self->to_keyval(node, c4::csubstr(keystr, keylen), c4::csubstr(valstr, vallen), more_flags);
    }

    void to_map(size_t node, const char *keystr, size_t keylen, int more_flags=0)
    {
        return $self->to_map(node, c4::csubstr(keystr, keylen), more_flags);
    }

    void to_seq(size_t node, const char *keystr, size_t keylen, int more_flags=0)
    {
        return $self->to_seq(node, c4::csubstr(keystr, keylen), more_flags);
    }

    void to_val(size_t node, const char *valstr, size_t vallen, int more_flags=0)
    {
        return $self->to_val(node, c4::csubstr(valstr, vallen), more_flags);
    }

    void set_key_tag(size_t node, const char *str, size_t len)
    {
        return $self->set_key_tag(node, c4::csubstr(str, len));
    }
    void set_val_tag(size_t node, const char *str, size_t len)
    {
        return $self->set_val_tag(node, c4::csubstr(str, len));
    }

    void set_key_anchor(size_t node, const char *str, size_t len)
    {
        return $self->set_key_anchor(node, c4::csubstr(str, len));
    }
    void set_val_anchor(size_t node, const char *str, size_t len)
    {
        return $self->set_val_anchor(node, c4::csubstr(str, len));
    }

    void set_key_ref(size_t node, const char *str, size_t len)
    {
        return $self->set_key_ref(node, c4::csubstr(str, len));
    }
    void set_val_ref(size_t node, const char *str, size_t len)
    {
        return $self->set_val_ref(node, c4::csubstr(str, len));
    }

};
*/

} // namespace yml
} // namespace c4

//-----------------------------------------------------------------------------
