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

#include <stdlib.h>
#include <stdio.h>

#include <Python.h>

#include "libjsonnet.h"

static char *jsonnet_str(struct JsonnetVm *vm, const char *str)
{
    char *out = jsonnet_realloc(vm, NULL, strlen(str) + 1);
    memcpy(out, str, strlen(str) + 1);
    return out;
}

struct ImportCtx {
    struct JsonnetVm *vm;
    PyObject *callback;
};

static char *cpython_import_callback(void *ctx_, const char *base, const char *rel,
                                     char **found_here, int *success)
{
    const struct ImportCtx *ctx = ctx_;
    PyObject *arglist, *result;
    char *out;

    arglist = Py_BuildValue("(s, s)", base, rel);
    result = PyEval_CallObject(ctx->callback, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
        // Get string from exception
        PyObject *ptype;
        PyObject *pvalue;
        PyObject *ptraceback;
        PyErr_Fetch(&ptype, &pvalue, &ptraceback);
        PyObject *exc_str = PyObject_Str(pvalue);
        const char *exc_cstr = PyString_AsString(exc_str);
        char *out = jsonnet_str(ctx->vm, exc_cstr);
        *success = 0;
        PyErr_Clear();
        return out;
    }

    if (!PyTuple_Check(result)) {
        out = jsonnet_str(ctx->vm, "import_callback did not return a tuple");
        *success = 0;
    } else if (PyTuple_Size(result) != 2) {
        out = jsonnet_str(ctx->vm, "import_callback did not return a tuple (size 2)");
        *success = 0;
    } else {
        PyObject *file_name = PyTuple_GetItem(result, 0);
        PyObject *file_content = PyTuple_GetItem(result, 1);
        if (!PyString_Check(file_name) || !PyString_Check(file_content)) {
            out = jsonnet_str(ctx->vm, "import_callback did not return a pair of strings");
            *success = 0;
        } else {
            const char *found_here_cstr = PyString_AsString(file_name);
            const char *content_cstr = PyString_AsString(file_content);
            *found_here = jsonnet_str(ctx->vm, found_here_cstr);
            out = jsonnet_str(ctx->vm, content_cstr);
            *success = 1;
        }
    }

    Py_DECREF(result);

    return out;
}

static PyObject *handle_result(struct JsonnetVm *vm, char *out, int error)
{
    if (error) {
        PyErr_SetString(PyExc_RuntimeError, out);
        jsonnet_realloc(vm, out, 0);
        jsonnet_destroy(vm);
        return NULL;
    } else {
        PyObject *ret = PyString_FromString(out);
        jsonnet_realloc(vm, out, 0);
        jsonnet_destroy(vm);
        return ret;
    }
}

int handle_ext_vars(struct JsonnetVm *vm, PyObject *ext_vars)
{
    if (ext_vars == NULL) return 1;

    PyObject *key, *val;
    Py_ssize_t pos = 0;
    
    while (PyDict_Next(ext_vars, &pos, &key, &val)) {
        const char *key_ = PyString_AsString(key);
        if (key_ == NULL) {
            jsonnet_destroy(vm);
            return 0;
        }
        const char *val_ = PyString_AsString(val);
        if (val_ == NULL) {
            jsonnet_destroy(vm);
            return 0;
        }
        jsonnet_ext_var(vm, key_, val_);
    }
    return 1;
}


int handle_import_callback(struct ImportCtx *ctx, PyObject *import_callback)
{
    if (import_callback == NULL) return 1;

    if (!PyCallable_Check(import_callback)) {
        PyErr_SetString(PyExc_TypeError, "import_callback must be callable");
        return 0;
    }

    jsonnet_import_callback(ctx->vm, cpython_import_callback, ctx);

    return 1;
}


static PyObject* evaluate_file(PyObject* self, PyObject* args, PyObject *keywds)
{
    const char *filename;
    char *out;
    unsigned max_stack = 500, gc_min_objects = 1000, max_trace = 20;
    double gc_growth_trigger = 2;
    int debug_ast = 0, error;
    PyObject *ext_vars = NULL, *import_callback = NULL;
    struct JsonnetVm *vm;
    static char *kwlist[] = {"filename", "max_stack", "gc_min_objects", "gc_growth_trigger", "ext_vars", "debug_ast", "max_trace", "import_callback", NULL};

    (void) self;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "s|IIdOiIO", kwlist,
                                     &filename,
                                     &max_stack, &gc_min_objects, &gc_growth_trigger, &ext_vars,
                                     &debug_ast, &max_trace, &import_callback)) {
        return NULL;
    }

    vm = jsonnet_make();
    jsonnet_max_stack(vm, max_stack);
    jsonnet_gc_min_objects(vm, gc_min_objects);
    jsonnet_max_trace(vm, max_trace);
    jsonnet_gc_growth_trigger(vm, gc_growth_trigger);
    jsonnet_debug_ast(vm, debug_ast);
    if (!handle_ext_vars(vm, ext_vars)) {
        return NULL;
    }
    struct ImportCtx ctx = { vm, import_callback };
    if (!handle_import_callback(&ctx, import_callback)) {
        return NULL;
    }

    out = jsonnet_evaluate_file(vm, filename, &error);
    return handle_result(vm, out, error);
}

static PyObject* evaluate_snippet(PyObject* self, PyObject* args, PyObject *keywds)
{
    const char *filename, *src;
    char *out;
    unsigned max_stack = 500, gc_min_objects = 1000, max_trace = 20;
    double gc_growth_trigger = 2;
    int debug_ast = 0, error;
    PyObject *ext_vars = NULL, *import_callback = NULL;
    struct JsonnetVm *vm;
    static char *kwlist[] = {"filename", "src", "max_stack", "gc_min_objects", "gc_growth_trigger", "ext_vars", "debug_ast", "max_trace", "import_callback", NULL};

    (void) self;

    if (!PyArg_ParseTupleAndKeywords(args, keywds, "ss|IIdOiIO", kwlist,
                                     &filename, &src,
                                     &max_stack, &gc_min_objects, &gc_growth_trigger, &ext_vars,
                                     &debug_ast, &max_trace, &import_callback)) {
        return NULL;
    }

    vm = jsonnet_make();
    jsonnet_max_stack(vm, max_stack);
    jsonnet_gc_min_objects(vm, gc_min_objects);
    jsonnet_max_trace(vm, max_trace);
    jsonnet_gc_growth_trigger(vm, gc_growth_trigger);
    jsonnet_debug_ast(vm, debug_ast);
    if (!handle_ext_vars(vm, ext_vars)) {
        return NULL;
    }
    struct ImportCtx ctx = { vm, import_callback };
    if (!handle_import_callback(&ctx, import_callback)) {
        return NULL;
    }

    out = jsonnet_evaluate_snippet(vm, filename, src, &error);
    return handle_result(vm, out, error);
}

static PyMethodDef module_methods[] = {
    {"evaluate_file", (PyCFunction)evaluate_file, METH_VARARGS | METH_KEYWORDS,
     "Interpret the given Jsonnet file."},
    {"evaluate_snippet", (PyCFunction)evaluate_snippet, METH_VARARGS | METH_KEYWORDS,
     "Interpret the given Jsonnet code."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_jsonnet(void)
{
    Py_InitModule3("_jsonnet", module_methods, "A Python interface to Jsonnet.");
}

