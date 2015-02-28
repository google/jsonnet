/*
Copyright 2015 Google Inc. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

namespace {

    /** Mark & sweep: advanced by 1 each GC cycle.
     */
    typedef unsigned char GarbageCollectionMark;

    /** Supertype of everything that is allocated on the heap.
     */
    struct HeapEntity {
        GarbageCollectionMark mark;
        virtual ~HeapEntity() { }
    };

    /** Tagged union of all values.
     *
     * Primitives (<= 8 bytes) are copied by value.  Otherwise a pointer to a HeapEntity is used.
     */
    struct Value {
        enum Type {
            NULL_TYPE = 0x0,  // Unfortunately NULL is a macro in C.
            BOOLEAN = 0x1,
            DOUBLE = 0x2,

            ARRAY = 0x10,
            FUNCTION = 0x11,
            OBJECT = 0x12,
            STRING = 0x13
        };
        Type t;
        union {
            HeapEntity *h;
            double d;
            bool b;
        } v;
        bool isHeap(void) const
        {
            return t & 0x10;
        }
    };

    /** Convert the type into a string, for error messages. */
    std::string type_str(Value::Type t)
    {
        switch (t) {
            case Value::NULL_TYPE: return "null";
            case Value::BOOLEAN: return "boolean";
            case Value::DOUBLE: return "double";
            case Value::ARRAY: return "array";
            case Value::FUNCTION: return "function";
            case Value::OBJECT: return "object";
            case Value::STRING: return "string";
            default:
            std::cerr << "INTERNAL ERROR: Unknown type: " << t << std::endl;
            std::abort();
            return "";  // Quiet, compiler.
        }
    }

    /** Convert the value's type into a string, for error messages. */
    std::string type_str(const Value &v)
    {
        return type_str(v.t);
    }

    struct HeapThunk;

    /** Stores the values bound to variables.
     *
     * Each nested local statement, function call, and field access has its own binding frame to
     * give the values for the local variable, function parameters, or upValues.
     */
    typedef std::map<const Identifier*, HeapThunk*> BindingFrame;

    /** Supertype of all objects.  Types of Value::OBJECT will point at these.  */
    struct HeapObject : public HeapEntity {
    };

    /** Hold an unevaluated expression.  This implements lazy semantics.
     */
    struct HeapThunk : public HeapEntity {
        /** Whether or not the thunk was forced. */
        bool filled;

        /** The result when the thunk was forced, if filled == true. */
        Value content;

        /** Used in error tracebacks. */
        const Identifier *name;

        /** The captured environment.
         *
         * Note, this is non-const because we have to add cyclic references to it.
         */
        BindingFrame upValues;

        /** The captured self variable, or nullptr if there was none.  \see CallFrame. */
        HeapObject *self;

        /** The offset from the captured self variable. \see CallFrame. */
        unsigned offset;

        /** Evaluated to force the thunk. */
        const AST *body;

        HeapThunk(const Identifier *name, HeapObject *self, unsigned offset, const AST *body)
          : filled(false), name(name), self(self), offset(offset), body(body)
        { }

        void fill(const Value &v)
        {
            content = v;
            filled = true;
            self = nullptr;
            upValues.clear();
        }
    };

    struct HeapArray : public HeapEntity {
        // It is convenient for this to not be const, so that we can add elements to it one at a
        // time after creation.  Thus, elements are not GCed as the array is being
        // created.
        std::vector<HeapThunk*> elements;
        HeapArray(const std::vector<HeapThunk*> &elements)
          : elements(elements)
        { }
    };

    /** Supertype of all objects that are not super objects or extended objects.  */
    struct HeapLeafObject : public HeapObject {
    };

    /** Objects created via the simple object constructor construct. */
    struct HeapSimpleObject : public HeapLeafObject {
        /** The captured environment. */
        const BindingFrame upValues;

        struct Field {
            /** Will the field appear in output? */
            Object::Field::Hide hide;
            /** Expression that is evaluated when indexing this field. */
            AST *body;
        };

        /** The fields.
         *
         * These are evaluated in the captured environment and with self and super bound
         * dynamically.
         */
        const std::map<const Identifier*, Field> fields;

        HeapSimpleObject(const BindingFrame &up_values,
                         const std::map<const Identifier*, Field> fields)
          : upValues(up_values), fields(fields)
        { }
    };

    /** Objects created by the extendby construct. */
    struct HeapExtendedObject : public HeapObject {
        /** The left hand side of the construct. */
        HeapObject *left;

        /** The right hand side of the construct. */
        HeapObject *right;

        HeapExtendedObject(HeapObject *left, HeapObject *right)
          : left(left), right(right)
        { }
    };

    /** Objects created by the super construct. */
    struct HeapSuperObject : public HeapObject {
        /** The object to bind self when evaluating field bodies when indexing me. */
        HeapObject *root;

        /** The object whose field definitions are used when indexing me. */
        unsigned offset;

        HeapSuperObject(HeapObject *root, unsigned offset)
          : root(root), offset(offset)
        { }
    };

    struct HeapComprehensionObject : public HeapLeafObject {

        /** The captured environment. */
        const BindingFrame upValues;

        /** The expression used to compute the field values.  */
        const AST* value;

        /** The identifier of bound variable in that construct.  */
        const Identifier * const id;

        /** Binding for id.
         *
         * For each field, holds the value that should be bound to id.  This is the corresponding
         * array element from the original array used to define this object.  This should not really
         * be a thunk, but it makes the implementation easier.
         */
        const std::map<const Identifier*, HeapThunk*> compValues;

        HeapComprehensionObject(const BindingFrame &up_values, const AST *value,
                                const Identifier *id,
                                const std::map<const Identifier*, HeapThunk*> &comp_values)
          : upValues(up_values), value(value), id(id), compValues(comp_values)
        { }
    };

    /** Stores the function itself and also the captured environment.
     *
     * Either body is non-null and builtin is 0, or body is null and builtin refers to a built-in
     * function.  In the former case, the closure represents a user function, otherwise calling it
     * will trigger the builtin function to execute.  Params is empty when the function is a
     * builtin.
     */
    struct HeapClosure : public HeapEntity {
        /** The captured environment. */
        const BindingFrame upValues;
        /** The captured self variable, or nullptr if there was none.  \see CallFrame. */
        HeapObject *self;
        /** The offset from the captured self variable.  \see CallFrame.*/
        unsigned offset;
        const std::vector<const Identifier*> params;
        const AST *body;
        const unsigned long builtin;
        HeapClosure(const BindingFrame &up_values,
                     HeapObject *self,
                     unsigned offset,
                     const std::vector<const Identifier*> &params,
                     const AST *body, unsigned long builtin)
          : upValues(up_values), self(self), offset(offset),
            params(params), body(body), builtin(builtin)
        { }
    };

    /** Stores a simple string on the heap. */
    struct HeapString : public HeapEntity {
        const std::string value;
        HeapString(const std::string &value)
          : value(value)
        { }
    };

    /** The heap does memory management, i.e. garbage collection. */
    class Heap {

        /** How many objects must exist in the heap before we bother doing garbage collection?
         */
        unsigned gcTuneMinObjects;

        /** How much must the heap have grown since the last cycle to trigger a collection?
         */
        double gcTuneGrowthTrigger;

        /** Value used to mark entities at the last garbage collection cycle. */
        GarbageCollectionMark lastMark;

        /** The heap entities (strings, arrays, objects, functions, etc).
         *
         * Not all may be reachable, all should have o->mark == this->lastMark.  Entities are
         * removed from the heap via O(1) swap with last element, so the ordering of entities is
         * arbitrary and changes every garbage collection cycle.
         */
        std::vector<HeapEntity*> entities;

        /** A stash of entities that are intermediate values during computation.
         *
         * These are still live but are not reachable from the stack or heap, because they are the
         * result of evaluating sub-expressions.  The garbage collector looks here as well as the
         * stack and heap.  The stash is popped via the ScopedStashCleanUp class, which uses the
         * RAII pattern to ensure the stash operations are balanced during execution.
         */
        std::vector<HeapEntity*> stash;

        /** The number of heap entities at the last garbage collection cycle. */
        unsigned long lastNumEntities;

        /** The number of heap entities now. */
        unsigned long numEntities;

        public:

        Heap(unsigned gc_tune_min_objects, double gc_tune_growth_trigger)
          : gcTuneMinObjects(gc_tune_min_objects), gcTuneGrowthTrigger(gc_tune_growth_trigger),
            lastMark(0), lastNumEntities(0), numEntities(0)
        {
        }

        ~Heap(void)
        {
            // Nothing is marked, everything will be collected.
            sweep();
        }

        /** Garbage collection: Mark v, and entities reachable from v. */
        void markFrom(Value v)
        {
            if (v.isHeap()) markFrom(v.v.h);
        }

        /** Garbage collection: Mark heap entities reachable from the given heap entity. */
        void markFrom(HeapEntity *from)
        {
            assert(from != nullptr);
            GarbageCollectionMark thisMark = lastMark + 1;
            if (from->mark == thisMark) return;
            from->mark = thisMark;
            if (auto *obj = dynamic_cast<HeapSimpleObject*>(from)) {
                for (auto upv : obj->upValues)
                    markFrom(upv.second);
            } else if (auto *obj = dynamic_cast<HeapExtendedObject*>(from)) {
                markFrom(obj->left);
                markFrom(obj->right);
            } else if (auto *obj = dynamic_cast<HeapComprehensionObject*>(from)) {
                for (auto upv : obj->upValues)
                    markFrom(upv.second);
                for (auto upv : obj->compValues)
                    markFrom(upv.second);
            } else if (auto *obj = dynamic_cast<HeapSuperObject*>(from)) {
                markFrom(obj->root);
            } else if (auto *arr = dynamic_cast<HeapArray*>(from)) {
                for (auto el : arr->elements)
                    markFrom(el);
            } else if (auto *func = dynamic_cast<HeapClosure*>(from)) {
                for (auto upv : func->upValues)
                    markFrom(upv.second);
                if (func->self) markFrom(func->self);
            } else if (auto *thunk = dynamic_cast<HeapThunk*>(from)) {
                if (thunk->filled) {
                    if (thunk->content.isHeap())
                        markFrom(thunk->content.v.h);
                } else {
                    for (auto upv : thunk->upValues)
                        markFrom(upv.second);
                    if (thunk->self) markFrom(thunk->self);
                }
            }
        }

        /** Avoid garbage collecting the given value.
         *
         * This is to be used in conjunction with ScopedStashCleanUp.
         */
        void stashIfIsHeap(const Value &v)
        {
            if (!v.isHeap()) return;
            stash.push_back(v.v.h);
        }

        /** Avoid garbage collecting the given value.
         *
         * This is to be used in conjunction with ScopedStashCleanUp.
         */
        void stashIfIsHeap(HeapEntity *h)
        {
            stash.push_back(h);
        }

        /** Mark all objects reachable from the stash. */
        void markFromStash(void)
        {
            for (HeapEntity *l : stash) {
                markFrom(l);
            }
        }

        /** Delete everything that was not marked since the last collection. */
        void sweep(void)
        {
            lastMark++;
            // Heap shrinks during this loop.  Do not cache entities.size().
            for (unsigned long i=0 ; i<entities.size() ; ++i) {
                HeapEntity *x = entities[i];
                if (x->mark != lastMark) {
                    delete x;
                    if (i != entities.size() - 1) {
                        // Swap it with the back.
                        entities[i] = entities[entities.size()-1];
                    }
                    entities.pop_back();
                    --i;
                }
            }
            lastNumEntities = numEntities = entities.size();
        }

        /** Returns the stash stack to the size it was earlier. */
        class ScopedStashCleanUp {
            Heap &heap;
            unsigned long sz;

            public:
            ScopedStashCleanUp(Heap &heap)
              : heap(heap), sz(heap.stash.size())
            { }
            ~ScopedStashCleanUp(void)
            {
                heap.stash.resize(sz);
            }
        };

        /** Is it time to initiate a GC cycle? */
        bool checkHeap(void)
        {
            return numEntities > gcTuneMinObjects
                && numEntities > gcTuneGrowthTrigger * lastNumEntities;
        }

        /** Allocate a heap entity.
         *
         * If the heap is large enough (\see gcTuneMinObjects) and has grown by enough since the
         * last collection cycle (\see gcTuneGrowthTrigger), a collection cycle is performed.
        */
        template <class T, class... Args> T* makeEntity(Args... args)
        {
            T *r = new T(args...);
            entities.push_back(r);
            r->mark = lastMark;
            numEntities = entities.size();
            return r;
        }

    };

}
