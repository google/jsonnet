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

#include "vm.h"

#include <cassert>
#include <cmath>

#include <set>
#include <string>

#include "parser.h"
#include "state.h"
#include "static_analysis.h"

namespace {

    /** Turn a path e.g. "/a/b/c" into a dir, e.g. "/a/b/".  If there is no path returns "".
     */
    std::string dir_name(const std::string &path)
    {
        size_t last_slash = path.rfind('/');
        if (last_slash != std::string::npos) {
            return path.substr(0, last_slash+1);
        }
        return "";
    }

    enum FrameKind {
        FRAME_APPLY_TARGET,
        FRAME_BINARY_LEFT,
        FRAME_BINARY_RIGHT,
        FRAME_BUILTIN_FILTER,
        FRAME_BUILTIN_FORCE_THUNKS,
        FRAME_CALL,
        FRAME_ERROR,
        FRAME_EQUALITY_MANIFEST,
        FRAME_IF,
        FRAME_INDEX_TARGET,
        FRAME_INDEX_INDEX,
        FRAME_LOCAL,
        FRAME_OBJECT,
        FRAME_OBJECT_COMP_ARRAY,
        FRAME_OBJECT_COMP_ELEMENT,
        FRAME_STRING_CONCAT,
        FRAME_UNARY
    };

    /** A frame on the stack.
     *
     * Every time a subterm is evaluated, we first push a new stack frame to
     * store the continuation.
     *
     * The stack frame is a bit like a tagged union, except not as memory
     * efficient.  The set of member variables that are actually used depends on
     * the value of the member varaible kind.
     *
     * If the stack frame is of kind FRAME_CALL, then it counts towards the
     * maximum number of stack frames allowed.  Other stack frames are not
     * counted.  This is because FRAME_CALL exists where there is a branch in
     * the code, e.g. the forcing of a thunk, evaluation of a field, calling a
     * function, etc.
     *
     * The stack is used to mark objects during garbage
     * collection, so HeapObjects not referred to from the stack may be
     * prematurely collected.
     */
    struct Frame {

        /** Tag (tagged union). */
        FrameKind kind;

        /** The code we were executing before. */
        const AST *ast;

        /** The location of the code we were executing before.
         *
         * loc == ast->location when ast != nullptr
         */
        LocationRange location;

        /** Reuse this stack frame for the purpose of tail call optimization. */
        bool tailCall;

        /** Used for a variety of purposes. */
        Value val;

        /** Used for a variety of purposes. */
        Value val2;

        /** Used for a variety of purposes. */
        Object::Fields::const_iterator fit;

        /** Used for a variety of purposes. */
        std::map<const Identifier *, HeapSimpleObject::Field> objectFields;

        /** Used for a variety of purposes. */
        unsigned elementId;

        /** Used for a variety of purposes. */
        std::map<const Identifier *, HeapThunk*> elements;

        /** Used for a variety of purposes. */
        std::vector<HeapThunk*> thunks;

        /** The context is used in error messages to attempt to find a reasonable name for the
         * object, function, or thunk value being executed.
         */
        HeapEntity *context;

        /** The lexically nearest object we are in, or nullptr.  Note
         * that this is not the same as context, because we could be inside a function,
         * inside an object and then context would be the function, but self would still point
         * to the object.
         */
        HeapObject *self;

        /** The "super" level of self.  Sometimes, we look upwards in the
         * inheritance tree, e.g. via an explicit use of super, or because a given field
         * has been inherited.  When evaluating a field from one of these super objects,
         * we need to bind self to the concrete object (so self must point
         * there) but uses of super should be resolved relative to the object whose
         * field we are evaluating.  Thus, we keep a second field for that.  This is
         * usually 0, unless we are evaluating a super object's field.
         */
        unsigned offset;

        /** A set of variables introduced at this point. */
        BindingFrame bindings;

        Frame(const FrameKind &kind, const AST *ast)
          : kind(kind), ast(ast), location(ast->location), tailCall(false), elementId(0),
            context(NULL), self(NULL), offset(0)
        {
            val.t = Value::NULL_TYPE;
            val2.t = Value::NULL_TYPE;
        }

        Frame(const FrameKind &kind, const LocationRange &location)
          : kind(kind), ast(nullptr), location(location), tailCall(false), elementId(0),
            context(NULL), self(NULL), offset(0)
        {
            val.t = Value::NULL_TYPE;
            val2.t = Value::NULL_TYPE;
        }

        /** Mark everything visible from this frame. */
        void mark(Heap &heap) const
        {
            heap.markFrom(val);
            heap.markFrom(val2);
            if (context) heap.markFrom(context);
            if (self) heap.markFrom(self);
            for (const auto &bind : bindings)
                heap.markFrom(bind.second);
            for (const auto &el : elements)
                heap.markFrom(el.second);
            for (const auto &th : thunks)
                heap.markFrom(th);
        }

        bool isCall(void) const
        {
            return kind == FRAME_CALL;
        }

    };

    /** The stack holds all the stack frames and manages the stack frame limit. */
    class Stack {

        /** How many call frames are on the stack. */
        unsigned calls;

        /** How many call frames should be allowed before aborting the program. */
        unsigned limit;

        /** The stack frames. */
        std::vector<Frame> stack;

        public:

        Stack(unsigned limit)
          : calls(0), limit(limit)
        {
        }

        ~Stack(void) { }

        unsigned size(void)
        {
            return stack.size();
        }

        /** Search for the closest variable in scope that matches the given name. */
        HeapThunk *lookUpVar(const Identifier *id)
        {
            for (int i=stack.size()-1 ; i>=0 ; --i) {
                const auto &binds = stack[i].bindings;
                auto it = binds.find(id);
                if (it != binds.end()) {
                    return it->second;
                }
                if (stack[i].isCall()) break;
            }
            return nullptr;
        }

        /** Mark everything visible from the stack (any frame). */
        void mark(Heap &heap)
        {
            for (const auto &f : stack) {
                f.mark(heap);
            }
        }

        Frame &top(void)
        {
            return stack.back();
        }

        const Frame &top(void) const
        {
            return stack.back();
        }

        void pop(void)
        {
            if (top().isCall()) calls--;
            stack.pop_back();
        }

        /** Attempt to find a name for a given heap entity.  This may not be possible, but we try
         * reasonably hard.  We look in the bindings for a variable in the closest scope that
         * happens to point at the entity in question.  Otherwise, the best we can do is use its
         * type.
         */
        std::string getName(unsigned from_here, const HeapEntity *e)
        {
            std::string name;
            for (int i=from_here-1 ; i>=0; --i) {
                const auto &f = stack[i];
                for (const auto &pair : f.bindings) {
                    HeapThunk *thunk = pair.second;
                    if (!thunk->filled) continue;
                    if (!thunk->content.isHeap()) continue;
                    if (e != thunk->content.v.h) continue;
                    name = pair.first->name;
                }
                // Do not go into the next call frame, keep local reasoning.
                if (f.isCall()) break;
            }

            if (name == "") name = "anonymous";
            if (dynamic_cast<const HeapObject*>(e)) {
                return "object <" + name + ">";
            } else if (auto *thunk = dynamic_cast<const HeapThunk*>(e)) {
                return "thunk <" + thunk->name->name + ">";
            } else {
                const auto *func = static_cast<const HeapClosure *>(e);
                if (func->body == nullptr) {
                    name = jsonnet_builtin_decl(func->builtin).name;
                    return "builtin function <" + name + ">";
                }
                return "function <" + name + ">";
            }
        }

        /** Dump the stack.
         *
         * This is useful to help debug the VM in gdb.  It is virtual to stop it
         * being removed by the compiler.
         */
        virtual void dump(void)
        {
            for (unsigned i=0 ; i<stack.size() ; ++i) {
                std::cout << "stack[" << i << "] = " << stack[i].location
                          << " (" << stack[i].kind << ")"
                          << std::endl;
            }
            std::cout << std::endl;
        }

        /** Creates the error object for throwing, and also populates it with the stack trace.
         */
        RuntimeError makeError(const LocationRange &loc, const std::string &msg)
        {
            std::vector<TraceFrame> stack_trace;
            stack_trace.push_back(TraceFrame(loc));
            for (int i=stack.size()-1 ; i>=0 ; --i) {
                const auto &f = stack[i];
                if (f.isCall()) {
                    if (f.context != nullptr) {
                        // Give the last line a name.
                        stack_trace[stack_trace.size()-1].name = getName(i, f.context);
                    }
                    stack_trace.push_back(TraceFrame(f.location));
                }
            }
            return RuntimeError(stack_trace, msg);
        }

        /** New (non-call) frame. */
        template <class... Args> void newFrame(Args... args)
        {
            stack.emplace_back(args...);
        }

        /** If there is a tailstrict annotated frame followed by some locals, pop them all. */
        void tailCallTrimStack (void)
        {
            for (int i=stack.size()-1 ; i>=0 ; --i) {
                switch (stack[i].kind) {
                    case FRAME_CALL: {
                        if (!stack[i].tailCall || stack[i].thunks.size() > 0) {
                            return;
                        }
                        // Remove all stack frames including this one.
                        while (stack.size() > unsigned(i)) stack.pop_back();
                        calls--;
                        return;
                    } break;

                    case FRAME_LOCAL: break;

                    default: return;
                }
            }
        }

        /** New call frame. */
        void newCall(const LocationRange &loc, HeapEntity *context, HeapObject *self,
                     unsigned offset, const BindingFrame &up_values)
        {
            tailCallTrimStack();
            if (calls >= limit) {
                throw makeError(loc, "Max stack frames exceeded.");
            }
            stack.emplace_back(FRAME_CALL, loc);
            calls++;
            top().context = context;
            top().self = self;
            top().offset = offset;
            top().bindings = up_values;
            top().tailCall = false;

            #ifndef NDEBUG
            for (const auto &bind : up_values) {
                assert(bind.second != nullptr);
            }
            #endif
        }

        /** Look up the stack to find the self binding. */
        void getSelfBinding(HeapObject *&self, unsigned &offset)
        {
            self = nullptr;
            offset = 0;
            for (int i=stack.size() - 1 ; i>=0 ; --i) {
                if (stack[i].isCall()) {
                    self = stack[i].self;
                    offset = stack[i].offset;
                    return;
                }
            }
        }
    };

    /** Typedef to save some typing. */
    typedef std::map<std::string, std::string> StrMap;


    /** Holds the intermediate state during execution and implements the necessary functions to
     * implement the semantics of the language.
     *
     * The garbage collector used is a simple stop-the-world mark and sweep collector.  It runs upon
     * memory allocation if the heap is large enough and has grown enough since the last collection.
     * All reachable entities have their mark field incremented.  Then all entities with the old
     * mark are removed from the heap.
     */
    class Interpreter {

        /** The heap. */
        Heap heap;

        /** The value last computed. */
        Value scratch;

        /** The stack. */
        Stack stack;

        /** Used to create ASTs if needed.
         *
         * This is used at import time, and in a few other cases.
         */
        Allocator *alloc;

        /** Used to "name" thunks crated on the inside of an array. */
        const Identifier *idArrayElement;

        /** Cache for imported Jsonnet files. */
        std::map<std::pair<std::string, std::string>, const std::string *> cachedImports;

        /** External variables for std.extVar. */
        StrMap externalVars;

        /** The callback used for loading imported files. */
        JsonnetImportCallback *importCallback;

        /** User context pointer for the import callback. */
        void *importCallbackContext;

        RuntimeError makeError(const LocationRange &loc, const std::string &msg)
        {
            return stack.makeError(loc, msg);
        }

        /** Create an object on the heap, maybe collect garbage.
         * \param T Something under HeapEntity
         * \returns The new object
         */
        template <class T, class... Args> T* makeHeap(Args... args)
        {
            T *r = heap.makeEntity<T, Args...>(args...);
            if (heap.checkHeap()) {  // Do a GC cycle?
                // Avoid the object we just made being collected.
                heap.markFrom(r);

                // Mark from the stack.
                stack.mark(heap);

                // Mark from the scratch register
                heap.markFrom(scratch);

                // Delete unreachable objects.
                heap.sweep();
            }
            return r;
        }

        Value makeBoolean(bool v)
        {
            Value r;
            r.t = Value::BOOLEAN;
            r.v.b = v;
            return r;
        }

        Value makeDouble(double v)
        {
            Value r;
            r.t = Value::DOUBLE;
            r.v.d = v;
            return r;
        }

        Value makeDoubleCheck(const LocationRange &loc, double v)
        {
            if (std::isnan(v)) {
                throw makeError(loc, "Not a number");
            }
            if (std::isinf(v)) {
                throw makeError(loc, "Overflow");
            }
            return makeDouble(v);
        }

        Value makeNull(void)
        {
            Value r;
            r.t = Value::NULL_TYPE;
            return r;
        }

        Value makeArray(const std::vector<HeapThunk*> &v)
        {
            Value r;
            r.t = Value::ARRAY;
            r.v.h = makeHeap<HeapArray>(v);
            return r;
        }

        Value makeClosure(const BindingFrame &env,
                           HeapObject *self,
                           unsigned offset,
                           const std::vector<const Identifier *> &params,
                           AST *body)
        {
            Value r;
            r.t = Value::FUNCTION;
            r.v.h = makeHeap<HeapClosure>(env, self, offset, params, body, 0);
            return r;
        }

        Value makeBuiltin(unsigned long builtin_id, const std::vector<const Identifier *> &params)
        {
            AST *body = nullptr;
            Value r;
            r.t = Value::FUNCTION;
            r.v.h = makeHeap<HeapClosure>(BindingFrame(), nullptr, 0,
                                          params, body, builtin_id);
            return r;
        }

        template <class T, class... Args> Value makeObject(Args... args)
        {
            Value r;
            r.t = Value::OBJECT;
            r.v.h = makeHeap<T>(args...);
            return r;
        }

        Value makeString(const std::string &v)
        {
            Value r;
            r.t = Value::STRING;
            r.v.h = makeHeap<HeapString>(v);
            return r;
        }

        /** Auxiliary function of objectIndex.
         *
         * Traverse the object's tree from right to left, looking for an object
         * with the given field.  Call with offset initially set to 0.
         *
         * \param f The field we're looking for.
         * \param curr The root object.
         * \param start_from Step over this many leaves first.
         * \param counter Return the level of "super" that contained the field.
         * \param self Return the new root.
         * \returns The first object with the field, or nullptr if it could not be found.
         */
        HeapLeafObject *findObject(const Identifier *f, HeapObject *root, HeapObject *curr,
                                   unsigned start_from, unsigned &counter,
                                   HeapObject *&self)
        {
            if (auto *ext = dynamic_cast<HeapExtendedObject*>(curr)) {
                auto *r = findObject(f, root, ext->right, start_from, counter, self);
                if (r) return r;
                auto *l = findObject(f, root, ext->left, start_from, counter, self);
                if (l) return l;
            } else if (auto *super = dynamic_cast<HeapSuperObject*>(curr)) {
                unsigned counter2 = 0;
                auto *needle = findObject(f, super->root, super->root, super->offset, counter2, self);
                if (needle != nullptr) {
                    counter = counter2;
                    return needle;
                }
            } else {
                if (counter >= start_from) {
                    if (auto *simp = dynamic_cast<HeapSimpleObject*>(curr)) {
                        auto it = simp->fields.find(f);
                        if (it != simp->fields.end()) {
                            self = root;
                            return simp;
                        }
                    } else if (auto *comp = dynamic_cast<HeapComprehensionObject*>(curr)) {
                        auto it = comp->compValues.find(f);
                        if (it != comp->compValues.end()) {
                            self = root;
                            return comp;
                        }
                    }
                }
                counter++;
            }
            return nullptr;
        }

        typedef std::map<const Identifier*, Object::Field::Hide> IdHideMap;

        /** Auxiliary function.
         */
         IdHideMap objectFields(const HeapObject *obj_,
                                unsigned &counter, unsigned skip,
                                bool manifesting)
        {
            IdHideMap r;
            if (auto *obj = dynamic_cast<const HeapSimpleObject*>(obj_)) {
                counter++;
                if (counter <= skip) return r;
                for (const auto &f : obj->fields) {
                    r[f.first] = !manifesting ? Object::Field::VISIBLE : f.second.hide;
                }

            } else if (auto *obj = dynamic_cast<const HeapExtendedObject*>(obj_)) {
                r = objectFields(obj->right, counter, skip, manifesting);
                for (const auto &pair : objectFields(obj->left, counter, skip, manifesting)) {
                    auto it = r.find(pair.first);
                    if (it == r.end()) {
                        // First time it is seen
                        r[pair.first] = pair.second;
                    } else if (it->second == Object::Field::INHERIT) {
                        // Seen before, but with inherited visibility so use new visibility
                        r[pair.first] = pair.second;
                    }
                }

            } else if (auto *obj = dynamic_cast<const HeapSuperObject*>(obj_)) {
                unsigned counter2 = 0;
                return objectFields(obj->root, counter2, obj->offset, manifesting);

            } else if (auto *obj = dynamic_cast<const HeapComprehensionObject*>(obj_)) {
                counter++;
                if (counter <= skip) return r;
                for (const auto &f : obj->compValues)
                    r[f.first] = Object::Field::VISIBLE;
            }
            return r;
        }

        /** Auxiliary function.
         */
        std::set<const Identifier*> objectFields(const HeapObject *obj_, bool manifesting)
        {
            unsigned counter = 0;
            std::set<const Identifier*> r;
            for (const auto &pair : objectFields(obj_, counter, 0, manifesting)) {
                if (pair.second != Object::Field::HIDDEN) r.insert(pair.first);
            }
            return r;
        }

        /** Import another Jsonnet file.
         *
         * If the file has already been imported, then use that version.  This maintains
         * referential transparency in the case of writes to disk during execution.
         *
         * \param loc Location of the import statement.
         * \param file Path to the filename.
         */
        AST *import(const LocationRange &loc, const std::string &file)
        {
            std::string dir = dir_name(loc.file);
            const std::string *input = importString(loc, file);

            std::string abs_file = file;
            if (dir.length() > 0)
                abs_file = dir + '/' + abs_file;

            auto *expr = jsonnet_parse(alloc, abs_file, input->c_str());
            jsonnet_static_analysis(expr);
            return expr;
        }

        /** Import a file as a string.
         *
         * If the file has already been imported, then use that version.  This maintains
         * referential transparency in the case of writes to disk during execution.
         *
         * \param loc Location of the import statement.
         * \param file Path to the filename.
         */
        const std::string *importString(const LocationRange &loc, const std::string &file)
        {
            std::string dir = dir_name(loc.file);

            std::pair<std::string, std::string> key(dir, file);
            const std::string *str = cachedImports[key];
            if (str != nullptr) return str;


            int success = 0;
            char *content =
                importCallback(importCallbackContext, dir.c_str(), file.c_str(), &success);

            std::string input(content);

            input.assign(content);
            ::free(content);

            if (!success) {
                    std::string msg = "Couldn't open import \"" + file + "\": ";
                    msg += input;
                    throw makeError(loc, msg);
            }

            std::string *input_ptr = new std::string(input);
            cachedImports[key] = input_ptr;
            return input_ptr;
        }

        /** Capture the required variables from the environment. */
        BindingFrame capture(const std::vector<const Identifier*> &free_vars)
        {
            BindingFrame env;
            for (auto fv : free_vars) {
                auto *th = stack.lookUpVar(fv);
                if (th != nullptr) {
                    env[fv] = th;
                }
            }
            return env;
        }

        /** Count the number of leaves in the tree.
         *
         * \param obj The root of the tree.
         * \param counter Initialize to 0, returns the number of leaves.
         */
        unsigned countLeaves(HeapObject *obj)
        {
            if (auto *ext = dynamic_cast<HeapExtendedObject*>(obj)) {
                return countLeaves(ext->left) + countLeaves(ext->right);
            } else if (auto *super = dynamic_cast<HeapSuperObject*>(obj)) {
                return countLeaves(super->root);
            } else {
                return 1;
            }
        }

        public:

        /** Create a new interpreter.
         *
         * \param loc The location range of the file to be executed.
         */
        Interpreter(Allocator *alloc, const StrMap &ext_vars,
                    unsigned max_stack, double gc_min_objects, double gc_growth_trigger,
                    JsonnetImportCallback *import_callback, void *import_callback_context)
          : heap(gc_min_objects, gc_growth_trigger), stack(max_stack), alloc(alloc),
            idArrayElement(alloc->makeIdentifier("array_element")), externalVars(ext_vars),
            importCallback(import_callback), importCallbackContext(import_callback_context)
        {
            scratch = makeNull();
        }

        /** Clean up the heap, stack, stash, and builtin function ASTs. */
        ~Interpreter()
        {
            for (const auto &pair : cachedImports) {
                delete pair.second;
            }
        }

        const Value &getScratchRegister(void)
        {
            return scratch;
        }

        void setScratchRegister(const Value &v)
        {
            scratch = v;
        }





        /** Test two values for equality.
         *
         * When not in strong mode, this will evaluate object fields, so can trigger a garbage
         * collection cycle.  Be sure to stash any objects that aren't reachable via the stack or
         * heap.
         */
        bool equality(const LocationRange &loc, const Value &a, const Value &b)
        {
            if (a.t != b.t) return false;

            switch (a.t) {
                case Value::ARRAY: {
                    auto *arr_a = static_cast<HeapArray*>(a.v.h);
                    auto *arr_b = static_cast<HeapArray*>(b.v.h);
                    if (arr_a->elements.size() != arr_b->elements.size()) return false;
                    for (unsigned long i=0 ; i<arr_a->elements.size() ; ++i) {
                        auto th_a = arr_a->elements[i];
                        if (!th_a->filled) {
                            stack.newCall(loc, th_a, th_a->self, th_a->offset, th_a->upValues);
                            evaluate(th_a->body);
                            stack.pop();
                            th_a->fill(scratch);
                        }

                        auto th_b = arr_b->elements[i];
                        if (!th_b->filled) {
                            stack.newCall(loc, th_b, th_b->self, th_b->offset, th_b->upValues);
                            evaluate(th_b->body);
                            stack.pop();
                            th_b->fill(scratch);
                        }

                        if (!equality(loc, th_a->content, th_b->content))
                                return false;
                    }
                    return true;
                }

                case Value::BOOLEAN:
                return a.v.b == b.v.b;

                case Value::DOUBLE:
                return a.v.d == b.v.d;

                case Value::FUNCTION:
                return false;

                case Value::NULL_TYPE:
                return true;

                case Value::OBJECT: {
                    auto *obj_a = static_cast<HeapObject*>(a.v.h);
                    auto *obj_b = static_cast<HeapObject*>(b.v.h);
                    std::set<const Identifier *> fields_a = objectFields(obj_a, true);
                    std::set<const Identifier *> fields_b = objectFields(obj_b, true);
                    if (fields_a != fields_b) return false;

                    // Put the two intermediate values on the stack so they won't get garbage
                    // collected.
                    stack.newFrame(FRAME_EQUALITY_MANIFEST, loc);
                    bool different = false;
                    for (const auto &f : fields_a) {


                        const AST *body_a = objectIndex(loc, obj_a, f);
                        evaluate(body_a);
                        stack.pop();
                        stack.top().val = scratch;

                        const AST *body_b = objectIndex(loc, obj_b, f);
                        evaluate(body_b);
                        stack.pop();
                        stack.top().val2 = scratch;

                        if (!equality(loc, stack.top().val, stack.top().val2)) {
                            different = true;
                            break;
                        }
                    }
                    stack.pop();
                    return !different;
                }

                case Value::STRING: {
                    auto *str_a = static_cast<HeapString*>(a.v.h);
                    auto *str_b = static_cast<HeapString*>(b.v.h);
                    return str_a->value == str_b->value;
                }

            }

            // Compiler thinks this is reachable, even though it is not.
            return false;
        }

        /** Raise an error if the arguments aren't the expected types. */
        void validateBuiltinArgs(const LocationRange &loc,
                                 unsigned long builtin,
                                 const std::vector<Value> &args,
                                 const std::vector<Value::Type> params)
        {
            if (args.size() == params.size()) {
                for (unsigned i=0 ; i<args.size() ; ++i) {
                    if (args[i].t != params[i]) goto bad;
                }
                return;
            }
            bad:;
            const std::string &name = jsonnet_builtin_decl(builtin).name;
            std::stringstream ss;
            ss << "Builtin function " + name + " expected (";
            const char *prefix = "";
            for (auto p : params) {
                ss << prefix << type_str(p);
                prefix = ", ";
            }
            ss << ") but got (";
            prefix = "";
            for (auto a : args) {
                ss << prefix << type_str(a);
                prefix = ", ";
            }
            ss << ")";
            throw makeError(loc, ss.str());
        }


        std::string toString(const LocationRange &loc)
        {
            return manifestJson(loc, false, "");
        }



        /** Index an object's field.
         *
         * \param loc Location where the e.f occured.
         * \param obj The target
         * \param f The field
         */
        const AST *objectIndex(const LocationRange &loc, HeapObject *obj,
                               const Identifier *f)
        {
            unsigned found_at = 0;
            HeapObject *self = nullptr;
            HeapLeafObject *found = findObject(f, obj, obj, 0, found_at, self);
            if (found == nullptr) {
                throw makeError(loc, "Field does not exist: " + f->name);
            }
            if (auto *simp = dynamic_cast<HeapSimpleObject*>(found)) {
                auto it = simp->fields.find(f);
                const AST *body = it->second.body;

                stack.newCall(loc, simp, self, found_at, simp->upValues);
                return body;
            } else {
                // If a HeapLeafObject is not HeapSimpleObject, it must be HeapComprehensionObject.
                auto *comp = static_cast<HeapComprehensionObject*>(found);
                auto it = comp->compValues.find(f);
                auto *th = it->second;
                BindingFrame binds = comp->upValues;
                binds[comp->id] = th;
                stack.newCall(loc, comp, self, found_at, binds);
                return comp->value;
            }
        }

        /** Evaluate the given AST to a value.
         *
         * Rather than call itself recursively, this function maintains a separate stack of
         * partially-evaluated constructs.  First, the AST is handled depending on its type.  If
         * this cannot be completed without evaluating another AST (e.g. a sub expression) then a
         * frame is pushed onto the stack containing the partial state, and the code jumps back to
         * the beginning of this function.  Once there are no more ASTs to evaluate, the code
         * executes the second part of the function to unwind the stack.  If the stack cannot be
         * completely unwound without evaluating an AST then it jumps back to the beginning of the
         * function again.  The process terminates when the AST has been processed and the stack is
         * the same size it was at the beginning of the call to evaluate.
         */
        void evaluate(const AST *ast_)
        {
            unsigned initial_stack_size = stack.size();

            recurse:

            switch (ast_->type) {
                case AST_APPLY: {
                    const auto &ast = *static_cast<const Apply*>(ast_);
                    stack.newFrame(FRAME_APPLY_TARGET, ast_);
                    ast_ = ast.target;
                    goto recurse;
                } break;

                case AST_ARRAY: {
                    const auto &ast = *static_cast<const Array*>(ast_);
                    HeapObject *self;
                    unsigned offset;
                    stack.getSelfBinding(self, offset);
                    scratch = makeArray({});
                    auto &elements = static_cast<HeapArray*>(scratch.v.h)->elements;
                    for (const AST *el : ast.elements) {
                        auto *el_th = makeHeap<HeapThunk>(idArrayElement, self, offset, el);
                        el_th->upValues =  capture(el->freeVariables);
                        elements.push_back(el_th);
                    }
                } break;

                case AST_BINARY: {
                    const auto &ast = *static_cast<const Binary*>(ast_);
                    stack.newFrame(FRAME_BINARY_LEFT, ast_);
                    ast_ = ast.left;
                    goto recurse;
                } break;

                case AST_BUILTIN_FUNCTION: {
                    const auto &ast = *static_cast<const BuiltinFunction*>(ast_);
                    scratch = makeBuiltin(ast.id, ast.params);
                } break;

                case AST_CONDITIONAL: {
                    const auto &ast = *static_cast<const Conditional*>(ast_);
                    stack.newFrame(FRAME_IF, ast_);
                    ast_ = ast.cond;
                    goto recurse;
                } break;

                case AST_ERROR: {
                    const auto &ast = *static_cast<const Error*>(ast_);
                    stack.newFrame(FRAME_ERROR, ast_);
                    ast_ = ast.expr;
                    goto recurse;
                } break;

                case AST_FUNCTION: {
                    const auto &ast = *static_cast<const Function*>(ast_);
                    auto env = capture(ast.freeVariables);
                    HeapObject *self;
                    unsigned offset;
                    stack.getSelfBinding(self, offset);
                    scratch = makeClosure(env, self, offset, ast.parameters, ast.body);
                } break;

                case AST_IMPORT: {
                    const auto &ast = *static_cast<const Import*>(ast_);
                    AST *expr = import(ast.location, ast.file);
                    ast_ = expr;
                    stack.newCall(ast.location, nullptr, nullptr, 0, BindingFrame());
                    goto recurse;
                } break;

                case AST_IMPORTSTR: {
                    const auto &ast = *static_cast<const Importstr*>(ast_);
                    const std::string *str = importString(ast.location, ast.file);
                    scratch = makeString(*str);
                } break;

                case AST_INDEX: {
                    const auto &ast = *static_cast<const Index*>(ast_);
                    stack.newFrame(FRAME_INDEX_TARGET, ast_);
                    ast_ = ast.target;
                    goto recurse;
                } break;

                case AST_LOCAL: {
                    const auto &ast = *static_cast<const Local*>(ast_);
                    stack.newFrame(FRAME_LOCAL, ast_);
                    Frame &f = stack.top();
                    // First build all the thunks and bind them.
                    HeapObject *self;
                    unsigned offset;
                    stack.getSelfBinding(self, offset);
                    for (const auto &bind : ast.binds) {
                        // Note that these 2 lines must remain separate to avoid the GC running
                        // when bindings has a nullptr for key bind.first.
                        auto *th = makeHeap<HeapThunk>(bind.first, self, offset, bind.second);
                        f.bindings[bind.first] = th;
                    }
                    // Now capture the environment (including the new thunks, to make cycles).
                    for (const auto &bind : ast.binds) {
                        auto *thunk = f.bindings[bind.first];
                        thunk->upValues = capture(bind.second->freeVariables);
                    }
                    ast_ = ast.body;
                    goto recurse;
                } break;

                case AST_LITERAL_BOOLEAN: {
                    const auto &ast = *static_cast<const LiteralBoolean*>(ast_);
                    scratch = makeBoolean(ast.value);
                } break;

                case AST_LITERAL_NUMBER: {
                    const auto &ast = *static_cast<const LiteralNumber*>(ast_);
                    scratch = makeDoubleCheck(ast_->location, ast.value);
                } break;

                case AST_LITERAL_STRING: {
                    const auto &ast = *static_cast<const LiteralString*>(ast_);
                    scratch = makeString(ast.value);
                } break;

                case AST_LITERAL_NULL: {
                    scratch = makeNull();
                } break;

                case AST_OBJECT: {
                    const auto &ast = *static_cast<const Object*>(ast_);
                    if (ast.fields.empty()) {
                        BindingFrame env;
                        std::map<const Identifier *, HeapSimpleObject::Field> fields;
                        scratch = makeObject<HeapSimpleObject>(env, fields);
                    } else {
                        auto env = capture(ast.freeVariables);
                        stack.newFrame(FRAME_OBJECT, ast_);
                        auto fit = ast.fields.begin();
                        stack.top().fit = fit;
                        ast_ = fit->name;
                        goto recurse;
                    }
                } break;

                case AST_OBJECT_COMPOSITION: {
                    const auto &ast = *static_cast<const ObjectComposition*>(ast_);
                    stack.newFrame(FRAME_OBJECT_COMP_ARRAY, ast_);
                    ast_ = ast.array;
                    goto recurse;
                } break;

                case AST_SELF: {
                    scratch.t = Value::OBJECT;
                    HeapObject *self;
                    unsigned offset;
                    stack.getSelfBinding(self, offset);
                    scratch.v.h = self;
                } break;

                case AST_SUPER: {
                    HeapObject *self;
                    unsigned offset;
                    stack.getSelfBinding(self, offset);
                    offset++;
                    if (offset >= countLeaves(self)) {
                        throw makeError(ast_->location,
                                        "Attempt to use super when there is no super class.");
                    }
                    scratch = makeObject<HeapSuperObject>(self, offset);
                } break;

                case AST_UNARY: {
                    const auto &ast = *static_cast<const Unary*>(ast_);
                    stack.newFrame(FRAME_UNARY, ast_);
                    ast_ = ast.expr;
                    goto recurse;
                } break;

                case AST_VAR: {
                    const auto &ast = *static_cast<const Var*>(ast_);
                    auto *thunk = stack.lookUpVar(ast.id);
                    if (thunk == nullptr) {
                        std::cerr << "INTERNAL ERROR: Could not bind variable: " << ast.id->name
                                  << std::endl;
                        std::abort();
                    }
                    if (thunk->filled) {
                        scratch = thunk->content;
                    } else {
                        stack.newCall(ast.location, thunk,
                                      thunk->self, thunk->offset, thunk->upValues);
                        ast_ = thunk->body;
                        goto recurse;
                    }
                } break;

                default:
                std::cerr << "INTERNAL ERROR: Unknown AST: " << ast_->type << std::endl;
                std::abort();
            }

            // To evaluate another AST, set ast to it, then goto recurse.
            // To pop, exit the switch or goto popframe
            // To change the frame and re-enter the switch, goto replaceframe
            while (stack.size() > initial_stack_size) {
                Frame &f = stack.top();
                switch (f.kind) {
                    case FRAME_APPLY_TARGET: {
                        const auto &ast = *static_cast<const Apply*>(f.ast);
                        if (scratch.t != Value::FUNCTION) {
                            throw makeError(ast.location,
                                            "Only functions can be called, got "
                                            + type_str(scratch));
                        }
                        auto *func = static_cast<HeapClosure*>(scratch.v.h);
                        if (ast.arguments.size() != func->params.size()) {
                            std::stringstream ss;
                            ss << "Expected " << func->params.size() <<
                                  " arguments, got " << ast.arguments.size() << ".";
                            throw makeError(ast.location, ss.str());
                        }

                        // Create thunks for arguments.
                        for (unsigned i=0 ; i<ast.arguments.size() ; ++i) {
                            const auto *arg = ast.arguments[i];
                            HeapObject *self;
                            unsigned offset;
                            stack.getSelfBinding(self, offset);
                            auto *thunk = makeHeap<HeapThunk>(func->params[i], self, offset, arg);
                            thunk->upValues = capture(arg->freeVariables);
                            f.thunks.push_back(thunk);
                        }
                        // Popping stack frame invalidates the f reference.
                        std::vector<HeapThunk*> args = f.thunks;

                        stack.pop();

                        if (func->body == nullptr) {
                            // Built-in function.
                            // Give nullptr for self because noone looking at this frame will
                            // attempt to bind to self (it's native code).
                            stack.newFrame(FRAME_BUILTIN_FORCE_THUNKS, f.ast);
                            stack.top().thunks = args;
                            stack.top().val = scratch;
                            goto replaceframe;
                        } else {
                            // User defined function.
                            BindingFrame bindings = func->upValues;
                            for (unsigned i=0 ; i<func->params.size() ; ++i)
                                bindings[func->params[i]] = args[i];
                            stack.newCall(ast.location, func, func->self, func->offset, bindings);
                            if (ast.tailstrict) {
                                stack.top().thunks = args;
                                stack.top().val = scratch;
                                stack.top().tailCall = true;
                                goto replaceframe;
                            } else {
                                ast_ = func->body;
                                goto recurse;
                            }
                        }
                    } break;

                    case FRAME_BINARY_LEFT: {
                        const auto &ast = *static_cast<const Binary*>(f.ast);
                        const Value &lhs = scratch;
                        if (lhs.t == Value::BOOLEAN) {
                            // Handle short-cut semantics
                            switch (ast.op) {
                                case BOP_AND: {
                                    if (!lhs.v.b) {
                                        scratch = makeBoolean(false);
                                        goto popframe;
                                    }
                                } break;

                                case BOP_OR: {
                                    if (lhs.v.b) {
                                        scratch = makeBoolean(true);
                                        goto popframe;
                                    }
                                } break;

                                default:;
                            }
                        }
                        stack.top().kind = FRAME_BINARY_RIGHT;
                        stack.top().val = lhs;
                        ast_ = ast.right;
                        goto recurse;
                    } break;

                    case FRAME_BINARY_RIGHT: {
                        const auto &ast = *static_cast<const Binary*>(f.ast);
                        const Value &lhs = stack.top().val;
                        const Value &rhs = scratch;
                        if (lhs.t == Value::STRING || rhs.t == Value::STRING) {
                            if (ast.op == BOP_PLUS) {
                                // Handle co-ercions for string processing.
                                stack.top().kind = FRAME_STRING_CONCAT;
                                stack.top().val2 = rhs;
                                goto replaceframe;
                            }
                        }
                        // Equality can be used when the types don't match.
                        switch (ast.op) {
                            case BOP_MANIFEST_EQUAL:
                            stack.top().kind = FRAME_EQUALITY_MANIFEST;
                            stack.top().val2 = rhs;
                            goto replaceframe;

                            case BOP_MANIFEST_UNEQUAL:
                            std::cerr << "INTERNAL ERROR: Inequalities not desugared" << std::endl;
                            abort();

                            default:;
                        }
                        // Everything else requires matching types.
                        if (lhs.t != rhs.t) {
                            throw makeError(ast.location,
                                            "Binary operator " + bop_string(ast.op) + " requires "
                                            "matching types, got " + type_str(lhs) + " and " +
                                            type_str(rhs) + ".");
                        }
                        switch (lhs.t) {
                            case Value::ARRAY:
                            if (ast.op == BOP_PLUS) {
                                auto *arr_l = static_cast<HeapArray*>(lhs.v.h);
                                auto *arr_r = static_cast<HeapArray*>(rhs.v.h);
                                std::vector<HeapThunk*> elements;
                                for (auto *el : arr_l->elements)
                                    elements.push_back(el);
                                for (auto *el : arr_r->elements)
                                    elements.push_back(el);
                                scratch = makeArray(elements);
                            } else {
                                throw makeError(ast.location,
                                                "Binary operator " + bop_string(ast.op)
                                                + " does not operate on arrays.");
                            }
                            break;

                            case Value::BOOLEAN:
                            switch (ast.op) {
                                case BOP_AND:
                                scratch = makeBoolean(lhs.v.b && rhs.v.b);
                                break;

                                case BOP_OR:
                                scratch = makeBoolean(lhs.v.b || rhs.v.b);
                                break;

                                default:
                                throw makeError(ast.location,
                                                "Binary operator " + bop_string(ast.op)
                                                + " does not operate on booleans.");
                            }
                            break;

                            case Value::DOUBLE:
                            switch (ast.op) {
                                case BOP_PLUS:
                                scratch = makeDoubleCheck(ast.location, lhs.v.d + rhs.v.d);
                                break;

                                case BOP_MINUS:
                                scratch = makeDoubleCheck(ast.location, lhs.v.d - rhs.v.d);
                                break;

                                case BOP_MULT:
                                scratch = makeDoubleCheck(ast.location, lhs.v.d * rhs.v.d);
                                break;

                                case BOP_DIV:
                                if (rhs.v.d == 0)
                                    throw makeError(ast.location, "Division by zero.");
                                scratch = makeDoubleCheck(ast.location, lhs.v.d / rhs.v.d);
                                break;

                                // No need to check doubles made from longs

                                case BOP_SHIFT_L: {
                                    long long_l = lhs.v.d;
                                    long long_r = rhs.v.d;
                                    scratch = makeDouble(long_l << long_r);
                                } break;

                                case BOP_SHIFT_R: {
                                    long long_l = lhs.v.d;
                                    long long_r = rhs.v.d;
                                    scratch = makeDouble(long_l >> long_r);
                                } break;

                                case BOP_BITWISE_AND: {
                                    long long_l = lhs.v.d;
                                    long long_r = rhs.v.d;
                                    scratch = makeDouble(long_l & long_r);
                                } break;

                                case BOP_BITWISE_XOR: {
                                    long long_l = lhs.v.d;
                                    long long_r = rhs.v.d;
                                    scratch = makeDouble(long_l ^ long_r);
                                } break;

                                case BOP_BITWISE_OR: {
                                    long long_l = lhs.v.d;
                                    long long_r = rhs.v.d;
                                    scratch = makeDouble(long_l | long_r);
                                } break;

                                case BOP_LESS_EQ:
                                scratch = makeBoolean(lhs.v.d <= rhs.v.d);
                                break;

                                case BOP_GREATER_EQ:
                                scratch = makeBoolean(lhs.v.d >= rhs.v.d);
                                break;

                                case BOP_LESS:
                                scratch = makeBoolean(lhs.v.d < rhs.v.d);
                                break;

                                case BOP_GREATER:
                                scratch = makeBoolean(lhs.v.d > rhs.v.d);
                                break;

                                default:
                                throw makeError(ast.location,
                                                "Binary operator " + bop_string(ast.op)
                                                + " does not operate on numbers.");
                            }
                            break;

                            case Value::FUNCTION:
                            throw makeError(ast.location, "Binary operator " + bop_string(ast.op) +
                                                          " does not operate on functions.");

                            case Value::NULL_TYPE:
                            throw makeError(ast.location, "Binary operator " + bop_string(ast.op) +
                                                          " does not operate on null.");

                            case Value::OBJECT: {
                                if (ast.op != BOP_PLUS) {
                                    throw makeError(ast.location,
                                                    "Binary operator " + bop_string(ast.op) +
                                                    " does not operate on objects.");
                                }
                                auto *lhs_obj = static_cast<HeapObject*>(lhs.v.h);
                                auto *rhs_obj = static_cast<HeapObject*>(rhs.v.h);
                                scratch = makeObject<HeapExtendedObject>(lhs_obj, rhs_obj);
                            }
                            break;

                            case Value::STRING: {
                                const std::string &lhs_str =
                                    static_cast<HeapString*>(lhs.v.h)->value;
                                const std::string &rhs_str =
                                    static_cast<HeapString*>(rhs.v.h)->value;
                                switch (ast.op) {
                                    case BOP_PLUS:
                                    scratch = makeString(lhs_str + rhs_str);
                                    break;

                                    case BOP_LESS_EQ:
                                    scratch = makeBoolean(lhs_str <= rhs_str);
                                    break;

                                    case BOP_GREATER_EQ:
                                    scratch = makeBoolean(lhs_str >= rhs_str);
                                    break;

                                    case BOP_LESS:
                                    scratch = makeBoolean(lhs_str < rhs_str);
                                    break;

                                    case BOP_GREATER:
                                    scratch = makeBoolean(lhs_str > rhs_str);
                                    break;

                                    default:
                                    throw makeError(ast.location,
                                                    "Binary operator " + bop_string(ast.op)
                                                    + " does not operate on strings.");
                                }
                            }
                            break;
                        }
                    } break;

                    case FRAME_BUILTIN_FILTER: {
                        const auto &ast = *static_cast<const Apply*>(f.ast);
                        auto *func = static_cast<HeapClosure*>(f.val.v.h);
                        auto *arr = static_cast<HeapArray*>(f.val2.v.h);
                        if (scratch.t != Value::BOOLEAN) {
                            throw makeError(ast.location,
                                            "filter function must return boolean, got: "
                                            + type_str(scratch));
                        }
                        if (scratch.v.b) f.thunks.push_back(arr->elements[f.elementId]);
                        f.elementId++;
                        // Iterate through arr, calling the function on each.
                        if (f.elementId == arr->elements.size()) {
                            scratch = makeArray(f.thunks);
                        } else {
                            auto *thunk = arr->elements[f.elementId];
                            BindingFrame bindings = func->upValues;
                            bindings[func->params[0]] = thunk;
                            stack.newCall(ast.location, func, func->self, func->offset, bindings);
                            ast_ = func->body;
                            goto recurse;
                        }
                    } break;

                    case FRAME_BUILTIN_FORCE_THUNKS: {
                        const auto &ast = *static_cast<const Apply*>(f.ast);
                        auto *func = static_cast<HeapClosure*>(f.val.v.h);
                        if (f.elementId == f.thunks.size()) {
                            // All thunks forced, now the builtin implementations.
                            const LocationRange &loc = ast.location;
                            unsigned builtin = func->builtin;
                            std::vector<Value> args;
                            for (auto *th : f.thunks) {
                                args.push_back(th->content);
                            }
                            switch (builtin) {
                                case 0: { // makeArray
                                    validateBuiltinArgs(loc, builtin, args,
                                                        {Value::DOUBLE, Value::FUNCTION});
                                    long sz = long(args[0].v.d);
                                    if (sz < 0) {
                                        std::stringstream ss;
                                        ss << "makeArray requires size >= 0, got " << sz;
                                        throw makeError(loc, ss.str());
                                    }
                                    auto *func = static_cast<const HeapClosure*>(args[1].v.h);
                                    std::vector<HeapThunk*> elements;
                                    if (func->params.size() != 1) {
                                        std::stringstream ss;
                                        ss << "makeArray function must take 1 param, got: "
                                           << func->params.size();
                                        throw makeError(loc, ss.str());
                                    }
                                    elements.resize(sz);
                                    for (long i=0 ; i<sz ; ++i) {
                                        auto *th = makeHeap<HeapThunk>(idArrayElement, func->self,
                                                                       func->offset, func->body);
                                        // The next line stops the new thunks from being GCed.
                                        f.thunks.push_back(th);
                                        th->upValues = func->upValues;

                                        auto *el = makeHeap<HeapThunk>(func->params[0], nullptr,
                                                                       0, nullptr);
                                        el->fill(makeDouble(i));  // i guaranteed not to be inf/NaN
                                        th->upValues[func->params[0]] = el;
                                        elements[i] = th;
                                    }
                                    scratch = makeArray(elements);
                                } break;

                                case 1:  // pow
                                validateBuiltinArgs(loc, builtin, args,
                                                    {Value::DOUBLE, Value::DOUBLE});
                                scratch = makeDoubleCheck(loc,
                                                             std::pow(args[0].v.d, args[1].v.d));
                                break;

                                case 2:  // floor
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::floor(args[0].v.d));
                                break;

                                case 3:  // ceil
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::ceil(args[0].v.d));
                                break;

                                case 4:  // sqrt
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::sqrt(args[0].v.d));
                                break;

                                case 5:  // sin
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::sin(args[0].v.d));
                                break;

                                case 6:  // cos
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::cos(args[0].v.d));
                                break;

                                case 7:  // tan
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::tan(args[0].v.d));
                                break;

                                case 8:  // asin
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::asin(args[0].v.d));
                                break;

                                case 9:  // acos
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::acos(args[0].v.d));
                                break;

                                case 10:  // atan
                                validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                scratch = makeDoubleCheck(loc, std::atan(args[0].v.d));
                                break;

                                case 11: {  // type
                                    switch (args[0].t) {
                                        case Value::NULL_TYPE:
                                        scratch = makeString("null");
                                        break;

                                        case Value::BOOLEAN:
                                        scratch = makeString("boolean");
                                        break;

                                        case Value::DOUBLE:
                                        scratch = makeString("number");
                                        break;

                                        case Value::ARRAY:
                                        scratch = makeString("array");
                                        break;

                                        case Value::FUNCTION:
                                        scratch = makeString("function");
                                        break;

                                        case Value::OBJECT:
                                        scratch = makeString("object");
                                        break;

                                        case Value::STRING:
                                        scratch = makeString("string");
                                        break;

                                    }
                                }
                                break;

                                case 12: {  // filter
                                    validateBuiltinArgs(loc, builtin, args,
                                                        {Value::FUNCTION, Value::ARRAY});
                                    auto *func = static_cast<HeapClosure*>(args[0].v.h);
                                    auto *arr = static_cast<HeapArray*>(args[1].v.h);
                                    if (func->params.size() != 1) {
                                        throw makeError(loc, "filter function takes 1 parameter.");
                                    }
                                    if (arr->elements.size() == 0) {
                                        scratch = makeArray({});
                                    } else {
                                        f.kind = FRAME_BUILTIN_FILTER;
                                        f.val = args[0];
                                        f.val2 = args[1];
                                        f.thunks.clear();
                                        f.elementId = 0;

                                        auto *thunk = arr->elements[f.elementId];
                                        BindingFrame bindings = func->upValues;
                                        bindings[func->params[0]] = thunk;
                                        stack.newCall(loc, func, func->self, func->offset,
                                                      bindings);
                                        ast_ = func->body;
                                        goto recurse;
                                    }
                                } break;

                                case 13: {  // objectHas
                                    validateBuiltinArgs(loc, builtin, args,
                                                        {Value::OBJECT, Value::STRING});
                                    const auto *obj = static_cast<const HeapObject*>(args[0].v.h);
                                    const auto *str = static_cast<const HeapString*>(args[1].v.h);
                                    bool found = false;
                                    for (const auto &field : objectFields(obj, true)) {
                                        if (field->name == str->value) {
                                            found = true;
                                            break;
                                        }
                                    }
                                    scratch = makeBoolean(found);
                                } break;

                                case 14: {  // length
                                    if (args.size() != 1) {
                                        throw makeError(loc, "length takes 1 parameter.");
                                    }
                                    HeapEntity *e = args[0].v.h;
                                    switch (args[0].t) {
                                        case Value::OBJECT: {
                                            auto fields =
                                                objectFields(static_cast<HeapObject*>(e), true);
                                            scratch = makeDouble(fields.size());
                                        } break;

                                        case Value::ARRAY:
                                        scratch = makeDouble(static_cast<HeapArray*>(e)
                                                             ->elements.size());
                                        break;

                                        case Value::STRING:
                                        scratch = makeDouble(static_cast<HeapString*>(e)
                                                             ->value.length());
                                        break;

                                        case Value::FUNCTION:
                                        scratch = makeDouble(static_cast<HeapClosure*>(e)
                                                             ->params.size());
                                        break;

                                        default:
                                        throw makeError(loc,
                                                        "length operates on strings, objects, "
                                                        "and arrays, got " + type_str(args[0]));
                                    }
                                } break;

                                case 15: {  // objectFields
                                    validateBuiltinArgs(loc, builtin, args, {Value::OBJECT});
                                    const auto *obj = static_cast<HeapObject*>(args[0].v.h);
                                    // Stash in a set first to sort them.
                                    std::set<std::string> fields;
                                    for (const auto &field : objectFields(obj, true)) {
                                        fields.insert(field->name);
                                    }
                                    scratch = makeArray({});
                                    auto &elements = static_cast<HeapArray*>(scratch.v.h)->elements;
                                    for (const auto &field : fields) {
                                        auto *th = makeHeap<HeapThunk>(idArrayElement, nullptr,
                                                                       0, nullptr);
                                        elements.push_back(th);
                                        th->fill(makeString(field));
                                    }
                                } break;

                                case 16: { // codepoint
                                    validateBuiltinArgs(loc, builtin, args, {Value::STRING});
                                    const std::string &str =
                                        static_cast<HeapString*>(args[0].v.h)->value;
                                    if (str.length() != 1) {
                                        std::stringstream ss;
                                        ss << "codepoint takes a string of length 1, got length "
                                           << str.length();
                                        throw makeError(loc, ss.str());
                                    }
                                    char c = static_cast<HeapString*>(args[0].v.h)->value[0];
                                    scratch = makeDouble((unsigned char)(c));
                                } break;

                                case 17: { // char
                                    validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                    long l = (unsigned long)(args[0].v.d);
                                    if (l < 0) {
                                        std::stringstream ss;
                                        ss << "Codepoints must be >= 0, got " << l;
                                        throw makeError(ast.location, ss.str());
                                    }
                                    if (l >= 128) {
                                        std::stringstream ss;
                                        ss << "Sorry, only ASCII supported right now.  ";
                                        ss << "Codepoints must be < 128, got " << l;
                                        throw makeError(ast.location, ss.str());
                                    }
                                    char c = l;
                                    scratch = makeString(std::string(&c, 1));
                                } break;

                                case 18: {  // log
                                    validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                    scratch = makeDoubleCheck(loc, std::log(args[0].v.d));
                                } break;

                                case 19: {  // exp
                                    validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                    scratch = makeDoubleCheck(loc, std::exp(args[0].v.d));
                                } break;

                                case 20: {  // mantissa
                                    validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                    int exp;
                                    double m = std::frexp(args[0].v.d, &exp);
                                    scratch = makeDoubleCheck(loc, m);
                                } break;

                                case 21: {  // exponent
                                    validateBuiltinArgs(loc, builtin, args, {Value::DOUBLE});
                                    int exp;
                                    std::frexp(args[0].v.d, &exp);
                                    scratch = makeDoubleCheck(loc, exp);
                                } break;

                                case 22: {  // modulo
                                    validateBuiltinArgs(loc, builtin, args,
                                                        {Value::DOUBLE, Value::DOUBLE});
                                    double a = args[0].v.d;
                                    double b = args[1].v.d;
                                    if (b == 0)
                                        throw makeError(ast.location, "Division by zero.");
                                    scratch = makeDoubleCheck(loc, std::fmod(a, b));
                                } break;

                                case 23: {  // extVar
                                    validateBuiltinArgs(loc, builtin, args, {Value::STRING});
                                    const std::string &var = static_cast<HeapString*>(args[0].v.h)->value;
                                    if (externalVars.find(var) == externalVars.end()) {
                                        throw makeError(ast.location, "Undefined external variable: " + var);
                                    }
                                    scratch = makeString(externalVars[var]);
                                } break;

                                default:
                                std::cerr << "INTERNAL ERROR: Unrecognized builtin: " << builtin
                                          << std::endl;
                                std::abort();
                            }

                        } else {
                            HeapThunk *th = f.thunks[f.elementId++];
                            if (!th->filled) {
                                stack.newCall(ast.location, th, th->self, th->offset, th->upValues);
                                ast_ = th->body;
                                goto recurse;
                            }
                        }
                    } break;

                    case FRAME_CALL: {
                        if (auto *thunk = dynamic_cast<HeapThunk*>(f.context)) {
                            // If we called a thunk, cache result.
                            thunk->fill(scratch);
                        } else if (auto *closure = dynamic_cast<HeapClosure*>(f.context)) {
                            if (f.elementId < f.thunks.size()) {
                                // If tailstrict, force thunks
                                HeapThunk *th = f.thunks[f.elementId++];
                                if (!th->filled) {
                                    stack.newCall(f.location, th,
                                                  th->self, th->offset, th->upValues);
                                    ast_ = th->body;
                                    goto recurse;
                                }
                            } else if (f.thunks.size() == 0) {
                                // Body has now been executed
                            } else {
                                // Execute the body
                                f.thunks.clear();
                                f.elementId = 0;
                                ast_ = closure->body;
                                goto recurse;
                            }
                        }
                        // Result of call is in scratch, just pop.
                    } break;

                    case FRAME_EQUALITY_MANIFEST: {
                        const auto &ast = *static_cast<const Binary*>(f.ast);
                        const Value &lhs = stack.top().val;
                        const Value &rhs = stack.top().val2;
                        bool b = equality(ast.location, lhs, rhs);
                        scratch = makeBoolean(b);
                    } break;

                    case FRAME_ERROR: {
                        const auto &ast = *static_cast<const Error*>(f.ast);
                        if (scratch.t != Value::STRING)
                            throw makeError(ast.location, "Error message must be string, got " +
                                                          type_str(scratch) + ".");
                        throw makeError(ast.location, static_cast<HeapString*>(scratch.v.h)->value);
                    } break;

                    case FRAME_IF: {
                        const auto &ast = *static_cast<const Conditional*>(f.ast);
                        if (scratch.t != Value::BOOLEAN) {
                            throw makeError(ast.location, "If condition must be boolean, got " +
                                                          type_str(scratch) + ".");
                        }
                        ast_ = scratch.v.b ? ast.branchTrue : ast.branchFalse;
                        stack.pop();
                        goto recurse;
                    } break;

                    case FRAME_INDEX_INDEX: {
                        const auto &ast = *static_cast<const Index*>(f.ast);
                        const Value &target = f.val;
                        if (target.t == Value::ARRAY) {
                            const auto *array = static_cast<HeapArray*>(target.v.h);
                            if (scratch.t != Value::DOUBLE) {
                                throw makeError(ast.location, "Array index must be number, got "
                                                              + type_str(scratch) + ".");
                            }
                            long i = long(scratch.v.d);
                            long sz = array->elements.size();
                            if (i < 0 || i >= sz) {
                                std::stringstream ss;
                                ss << "Array bounds error: " << i
                                   << " not within [0, " << sz << ")";
                                throw makeError(ast.location, ss.str());
                            }
                            auto *thunk = array->elements[i];
                            if (thunk->filled) {
                                scratch = thunk->content;
                            } else {
                                stack.pop();
                                stack.newCall(ast.location, thunk,
                                              thunk->self, thunk->offset, thunk->upValues);
                                ast_ = thunk->body;
                                goto recurse;
                            }
                        } else if (target.t == Value::OBJECT) {
                            auto *obj = static_cast<HeapObject*>(target.v.h);
                            assert(obj != nullptr);
                            if (scratch.t != Value::STRING) {
                                throw makeError(ast.location,
                                                "Object index must be string, got "
                                                + type_str(scratch) + ".");
                            }
                            const std::string &index_name =
                                static_cast<HeapString*>(scratch.v.h)->value;
                            auto *fid = alloc->makeIdentifier(index_name);
                            stack.pop();
                            ast_ = objectIndex(ast.location, obj, fid);
                            goto recurse;
                        } else if (target.t == Value::STRING) {
                            auto *obj = static_cast<HeapString*>(target.v.h);
                            assert(obj != nullptr);
                            if (scratch.t != Value::DOUBLE) {
                                throw makeError(ast.location,
                                                "String index must be a number, got "
                                                + type_str(scratch) + ".");
                            }
                            // TODO(dcunnin):  UTF-8 support goes here.
                            long sz = obj->value.length();
                            long i = (long)scratch.v.d;
                            if (i < 0 || i >= sz) {
                                std::stringstream ss;
                                ss << "String bounds error: " << i
                                   << " not within [0, " << sz << ")";
                                throw makeError(ast.location, ss.str());
                            }
                            char ch[] = {obj->value[i], '\0'};
                            scratch = makeString(ch);
                        } else {
                            std::cerr << "INTERNAL ERROR: Not object / array / string."
                                      << std::endl;
                            abort();
                        }
                    } break;

                    case FRAME_INDEX_TARGET: {
                        const auto &ast = *static_cast<const Index*>(f.ast);
                        if (scratch.t != Value::ARRAY
                            && scratch.t != Value::OBJECT
                            && scratch.t != Value::STRING) {
                            throw makeError(ast.location,
                                            "Can only index objects, strings, and arrays, got "
                                            + type_str(scratch) + ".");
                        }
                        ast_ = ast.index;
                        f.kind = FRAME_INDEX_INDEX;
                        f.val = scratch;
                        goto recurse;
                    } break;

                    case FRAME_LOCAL: {
                        // Result of execution is in scratch already.
                    } break;

                    case FRAME_OBJECT: {
                        const auto &ast = *static_cast<const Object*>(f.ast);
                        if (scratch.t != Value::NULL_TYPE) {
                            if (scratch.t != Value::STRING) {
                                throw makeError(ast.location, "Field name was not a string.");
                            }
                            const auto &fname = static_cast<const HeapString*>(scratch.v.h)->value;
                            const Identifier *fid = alloc->makeIdentifier(fname);
                            if (f.objectFields.find(fid) != f.objectFields.end()) {
                                throw makeError(ast.location,
                                                "Duplicate field name: \"" + fname + "\"");
                            }
                            f.objectFields[fid].hide = f.fit->hide;
                            f.objectFields[fid].body = f.fit->body;
                        }
                        f.fit++;
                        if (f.fit != ast.fields.end()) {
                            ast_ = f.fit->name;
                            goto recurse;
                        } else {
                            auto env = capture(ast.freeVariables);
                            scratch = makeObject<HeapSimpleObject>(env, f.objectFields);
                        }
                    } break;

                    case FRAME_OBJECT_COMP_ARRAY: {
                        const auto &ast = *static_cast<const ObjectComposition*>(f.ast);
                        const Value &arr_v = scratch;
                        if (scratch.t != Value::ARRAY) {
                            throw makeError(ast.location,
                                            "Object comprehension needs array, got "
                                            + type_str(arr_v));
                        }
                        const auto *arr = static_cast<const HeapArray*>(arr_v.v.h);
                        if (arr->elements.size() == 0) {
                            // Degenerate case.  Just create the object now.
                            scratch = makeObject<HeapComprehensionObject>(BindingFrame{}, ast.value,
                                                                          ast.id, BindingFrame{});
                        } else {
                            f.kind = FRAME_OBJECT_COMP_ELEMENT;
                            f.val = scratch;
                            f.bindings[ast.id] = arr->elements[0];
                            f.elementId = 0;
                            ast_ = ast.field;
                            goto recurse;
                        }
                    } break;

                    case FRAME_OBJECT_COMP_ELEMENT: {
                        const auto &ast = *static_cast<const ObjectComposition*>(f.ast);
                        const auto *arr = static_cast<const HeapArray*>(f.val.v.h);
                        if (scratch.t != Value::STRING) {
                            std::stringstream ss;
                            ss << "field must be string, got: " << type_str(scratch);
                            throw makeError(ast.location, ss.str());
                        }
                        const auto &fname = static_cast<const HeapString*>(scratch.v.h)->value;
                        const Identifier *fid = alloc->makeIdentifier(fname);
                        if (f.elements.find(fid) != f.elements.end()) {
                            throw makeError(ast.location,
                                            "Duplicate field name: \"" + fname + "\"");
                        }
                        f.elements[fid] = arr->elements[f.elementId];
                        f.elementId++;

                        if (f.elementId == arr->elements.size()) {
                            auto env = capture(ast.freeVariables);
                            scratch = makeObject<HeapComprehensionObject>(env, ast.value,
                                                                          ast.id, f.elements);
                        } else {
                            f.bindings[ast.id] = arr->elements[f.elementId];
                            ast_ = ast.field;
                            goto recurse;
                        }
                    } break;

                    case FRAME_STRING_CONCAT: {
                        const auto &ast = *static_cast<const Binary*>(f.ast);
                        const Value &lhs = stack.top().val;
                        const Value &rhs = stack.top().val2;
                        std::stringstream ss;
                        if (lhs.t == Value::STRING) {
                            ss << static_cast<const HeapString*>(lhs.v.h)->value;
                        } else {
                            scratch = lhs;
                            ss << toString(ast.left->location);
                        }
                        if (rhs.t == Value::STRING) {
                            ss << static_cast<const HeapString*>(rhs.v.h)->value;
                        } else {
                            scratch = rhs;
                            ss << toString(ast.right->location);
                        }
                        scratch = makeString(ss.str());
                    } break;

                    case FRAME_UNARY: {
                        const auto &ast = *static_cast<const Unary*>(f.ast);
                        switch (scratch.t) {

                            case Value::BOOLEAN:
                            if (ast.op == UOP_NOT) {
                                scratch = makeBoolean(!scratch.v.b);
                            } else {
                                throw makeError(ast.location,
                                                "Unary operator " + uop_string(ast.op)
                                                + " does not operate on booleans.");
                            }
                            break;

                            case Value::DOUBLE:
                            switch (ast.op) {
                                case UOP_PLUS:
                                break;

                                case UOP_MINUS:
                                scratch = makeDouble(-scratch.v.d);
                                break;

                                case UOP_BITWISE_NOT:
                                scratch = makeDouble(~(long)(scratch.v.d));
                                break;

                                default:
                                throw makeError(ast.location,
                                                "Unary operator " + uop_string(ast.op)
                                                + " does not operate on numbers.");
                            }
                            break;

                            default:
                                throw makeError(ast.location,
                                                "Unary operator " + uop_string(ast.op) +
                                                " does not operate on type " + type_str(scratch));
                        }
                    } break;

                    default:
                    std::cerr << "INTERNAL ERROR: Unknown FrameKind:  " << f.kind << std::endl;
                    std::abort();
                }

                popframe:;

                stack.pop();

                replaceframe:;
            }
        }

        /** Manifest the scratch value by evaluating any remaining fields, and then convert to JSON.
         *
         * This can trigger a garbage collection cycle.  Be sure to stash any objects that aren't
         * reachable via the stack or heap.
         *
         * \param multiline If true, will print objects and arrays in an indented fashion.
         */
        std::string manifestJson(const LocationRange &loc, bool multiline,
                                 const std::string &indent)
        {
            // Printing fields means evaluating and binding them, which can trigger
            // garbage collection.

            std::stringstream ss;
            switch (scratch.t) {
                case Value::ARRAY: {
                    HeapArray *arr = static_cast<HeapArray*>(scratch.v.h);
                    if (arr->elements.size() == 0) {
                        ss << "[ ]";
                    } else {
                        const char *prefix = multiline ? "[\n" : "[";
                        std::string indent2 = multiline ? indent + "   " : indent;
                        for (auto *thunk : arr->elements) {
                            LocationRange tloc = thunk->body == nullptr
                                               ? loc
                                               : thunk->body->location;
                            if (thunk->filled) {
                                stack.newCall(loc, thunk, nullptr, 0, BindingFrame{});
                                // Keep arr alive when scratch is overwritten
                                stack.top().val = scratch;
                                scratch = thunk->content;
                            } else {
                                stack.newCall(loc, thunk,
                                              thunk->self, thunk->offset, thunk->upValues);
                                // Keep arr alive when scratch is overwritten
                                stack.top().val = scratch;
                                evaluate(thunk->body);
                            }
                            auto element = manifestJson(tloc, multiline, indent2);
                            // Restore scratch
                            scratch = stack.top().val;
                            stack.pop();
                            ss << prefix << indent2 << element;
                            prefix = multiline ? ",\n" : ", ";
                        }
                        ss << (multiline ? "\n" : "") << indent << "]";
                    }
                }
                break;

                case Value::BOOLEAN:
                ss << (scratch.v.b ? "true" : "false");
                break;

                case Value::DOUBLE:
                ss << jsonnet_unparse_number(scratch.v.d);
                break;

                case Value::FUNCTION:
                throw makeError(loc, "Couldn't manifest function in JSON output.");

                case Value::NULL_TYPE:
                ss << "null";
                break;

                case Value::OBJECT: {
                    auto *obj = static_cast<HeapObject*>(scratch.v.h);
                    // Using std::map has the useful side-effect of ordering the fields
                    // alphabetically.
                    std::map<std::string, const Identifier*> fields;
                    for (const auto &f : objectFields(obj, true)) {
                        fields[f->name] = f;
                    }
                    if (fields.size() == 0) {
                        ss << "{ }";
                    } else {
                        std::string indent2 = multiline ? indent + "   " : indent;
                        const char *prefix = multiline ? "{\n" : "{";
                        for (const auto &f : fields) {
                            // pushes FRAME_CALL
                            const AST *body = objectIndex(loc, obj, f.second);
                            stack.top().val = scratch;
                            evaluate(body);
                            auto vstr = manifestJson(body->location, multiline, indent2);
                            // Reset scratch so that the object we're manifesting doesn't
                            // get GC'd.
                            scratch = stack.top().val;
                            stack.pop();
                            ss << prefix << indent2 << "\"" << f.first << "\": " << vstr;
                            prefix = multiline ? ",\n" : ", ";
                        }
                        ss << (multiline ? "\n" : "") << indent << "}";
                    }
                }
                break;

                case Value::STRING: {
                    const std::string &str = static_cast<HeapString*>(scratch.v.h)->value;
                    ss << jsonnet_unparse_escape(str);
                }
                break;
            }
            return ss.str();
        }

        std::string manifestString(const LocationRange &loc)
        {
            if (scratch.t != Value::STRING) {
                std::stringstream ss;
                ss << "Expected string result, got: " << type_str(scratch.t);
                throw makeError(loc, ss.str());
            }
            return static_cast<HeapString*>(scratch.v.h)->value;
        }

        StrMap manifestMulti(bool string)
        {
            StrMap r;
            LocationRange loc("During manifestation");
            if (scratch.t != Value::OBJECT) {
                std::stringstream ss;
                ss << "Multi mode: Top-level object was a " << type_str(scratch.t) << ", "
                   << "should be an object whose keys are filenames and values hold "
                   << "the JSON for that file.";
                throw makeError(loc, ss.str());
            }
            auto *obj = static_cast<HeapObject*>(scratch.v.h);
            std::map<std::string, const Identifier*> fields;
            for (const auto &f : objectFields(obj, true)) {
                fields[f->name] = f;
            }
            for (const auto &f : fields) {
                // pushes FRAME_CALL
                const AST *body = objectIndex(loc, obj, f.second);
                stack.top().val = scratch;
                evaluate(body);
                auto vstr = string ? manifestString(body->location)
                                   : manifestJson(body->location, true, "");
                // Reset scratch so that the object we're manifesting doesn't
                // get GC'd.
                scratch = stack.top().val;
                stack.pop();
                r[f.first] = vstr;
            }
            return r;
        }

    };

}

std::string jsonnet_vm_execute(Allocator *alloc, const AST *ast,
                               const StrMap &ext_vars,
                               unsigned max_stack, double gc_min_objects,
                               double gc_growth_trigger,
                               JsonnetImportCallback *import_callback, void *ctx,
                               bool string_output)
{
    Interpreter vm(alloc, ext_vars, max_stack, gc_min_objects, gc_growth_trigger,
                   import_callback, ctx);
    vm.evaluate(ast);
    if (string_output) {
        return vm.manifestString(LocationRange("During manifestation"));
    } else {
        return vm.manifestJson(LocationRange("During manifestation"), true, "");
    }
}

StrMap jsonnet_vm_execute_multi(Allocator *alloc, const AST *ast, const StrMap &ext_vars,
                                unsigned max_stack, double gc_min_objects, double gc_growth_trigger,
                                JsonnetImportCallback *import_callback, void *ctx,
                                bool string_output)
{
    Interpreter vm(alloc, ext_vars, max_stack, gc_min_objects, gc_growth_trigger,
                   import_callback, ctx);
    vm.evaluate(ast);
    return vm.manifestMulti(string_output);
}

