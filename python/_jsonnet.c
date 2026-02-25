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

#if PY_MAJOR_VERSION < 3
#error "Python 2 is no longer supported."
#endif

struct StrRefAndObj {
    /* Since Python 3.10, we have PyUnicode_AsUTF8AndSize
     * which doesn't need its result to be deallocated, and
     * therefore doesn't need to track the extra Bytes object. */
#if Py_LIMITED_API < 0x030A0000
    PyObject *bytes;
#endif
    const char *cstr;
    Py_ssize_t len;
};

static struct StrRefAndObj get_py_utf8_string(PyObject *unicode_obj) {
    Py_ssize_t len = 0;
    struct StrRefAndObj ret = {0};

    if (!unicode_obj) {
        return ret;
    }

#if Py_LIMITED_API < 0x030A0000
    PyObject *bytes = PyUnicode_AsUTF8String(unicode_obj);
    if (!bytes) {
        return ret;
    }
    char *s = 0;
    if (PyBytes_AsStringAndSize(bytes, &s, &len)) {
        // What to do if it fails??
        Py_XDECREF(bytes);
        return ret;
    }
    ret.bytes = bytes;
#else
    const char *s = PyUnicode_AsUTF8AndSize(unicode_obj, &len);
#endif
    ret.cstr = s;
    ret.len = len;
    return ret;
}

static void release_py_utf8_string(struct StrRefAndObj *utf8_obj) {
#if Py_LIMITED_API < 0x030A0000
    Py_XDECREF(utf8_obj->bytes);
    utf8_obj->bytes = NULL;
#endif
    utf8_obj->cstr = NULL;
    utf8_obj->len = 0;
}

static char *jsonnet_str(struct JsonnetVm *vm, const char *str)
{
    size_t size = strlen(str) + 1;
    char *out = jsonnet_realloc(vm, NULL, size);
    memcpy(out, str, size);
    return out;
}

static char *jsonnet_str_nonull(struct JsonnetVm *vm, const char *str, size_t *buflen)
{
    *buflen = strlen(str);
    char *out = jsonnet_realloc(vm, NULL, *buflen);
    memcpy(out, str, *buflen);
    return out;
}

static struct StrRefAndObj exc_to_str(void)
{
    PyObject *ptype, *pvalue, *ptraceback;
    PyErr_Fetch(&ptype, &pvalue, &ptraceback);
    PyObject *exc_str = PyObject_Str(pvalue);
    return get_py_utf8_string(exc_str);
}

struct NativeCtx {
    struct JsonnetVm *vm;
    PyThreadState **py_thread;
    PyObject *callback;
    size_t argc;
};

static struct JsonnetJsonValue *python_to_jsonnet_json(struct JsonnetVm *vm, PyObject *v,
                                                       const char **err_msg)
{
    if (PyUnicode_Check(v)) {
        struct JsonnetJsonValue *r;
        struct StrRefAndObj v_utf8 = get_py_utf8_string(v);
        r = jsonnet_json_make_string(vm, v_utf8.cstr);
        release_py_utf8_string(&v_utf8);
        return r;
    } else if (PyBool_Check(v)) {
        return jsonnet_json_make_bool(vm, PyObject_IsTrue(v));
    } else if (PyFloat_Check(v)) {
        return jsonnet_json_make_number(vm, PyFloat_AsDouble(v));
    } else if (PyLong_Check(v)) {
        return jsonnet_json_make_number(vm, (double)(PyLong_AsLong(v)));
    } else if (v == Py_None) {
        return jsonnet_json_make_null(vm);
    } else if (PySequence_Check(v)) {
        Py_ssize_t len, i;
        struct JsonnetJsonValue *arr;
        // Convert it to a O(1) indexable form if necessary.
        PyObject *fast = PySequence_Fast(v, "python_to_jsonnet_json internal error: not sequence");
        len = PySequence_Size(fast);
        arr = jsonnet_json_make_array(vm);
        for (i = 0; i < len; ++i) {
            struct JsonnetJsonValue *json_el;
            PyObject *el = PySequence_GetItem(fast, i);
            json_el = python_to_jsonnet_json(vm, el, err_msg);
            if (json_el == NULL) {
                Py_DECREF(fast);
                jsonnet_json_destroy(vm, arr);
                return NULL;
            }
            jsonnet_json_array_append(vm, arr, json_el);
        }
        Py_DECREF(fast);
        return arr;
    } else if (PyDict_Check(v)) {
        struct JsonnetJsonValue *obj;
        PyObject *key, *val;
        Py_ssize_t pos = 0;
        obj = jsonnet_json_make_object(vm);
        while (PyDict_Next(v, &pos, &key, &val)) {
            struct JsonnetJsonValue *json_val;
            struct StrRefAndObj key_utf8 = get_py_utf8_string(key);
            if (!key_utf8.cstr) {
                release_py_utf8_string(&key_utf8);
                *err_msg = "Non-string key in dict returned from Python Jsonnet native extension.";
                jsonnet_json_destroy(vm, obj);
                return NULL;
            }
            json_val = python_to_jsonnet_json(vm, val, err_msg);
            if (json_val == NULL) {
                release_py_utf8_string(&key_utf8);
                jsonnet_json_destroy(vm, obj);
                return NULL;
            }
            jsonnet_json_object_append(vm, obj, key_utf8.cstr, json_val);
            release_py_utf8_string(&key_utf8);
        }
        return obj;
    } else {
        *err_msg = "Unrecognized type return from Python Jsonnet native extension.";
        return NULL;
    }
}

/* This function is bound for every native callback, but with a different
 * context.
 */
static struct JsonnetJsonValue *cpython_native_callback(
    void *ctx_, const struct JsonnetJsonValue * const *argv, int *succ)
{
    const struct NativeCtx *ctx = ctx_;

    PyEval_RestoreThread(*ctx->py_thread);

    PyObject *arglist;  // Will hold a tuple of strings.
    PyObject *result;  // Will hold a string.

    // Populate python function args.
    arglist = PyTuple_New(ctx->argc);
    for (size_t i = 0; i < ctx->argc; ++i) {
        double d;
        const char *param_str = jsonnet_json_extract_string(ctx->vm, argv[i]);
        int param_null = jsonnet_json_extract_null(ctx->vm, argv[i]);
        int param_bool = jsonnet_json_extract_bool(ctx->vm, argv[i]);
        int param_num = jsonnet_json_extract_number(ctx->vm, argv[i], &d);
        PyObject *pyobj;
        if (param_str != NULL) {
            pyobj = PyUnicode_FromString(param_str);
        } else if (param_null) {
            pyobj = Py_None;
        } else if (param_bool != 2) {
            pyobj = PyBool_FromLong(param_bool);
        } else if (param_num) {
            pyobj = PyFloat_FromDouble(d);
        } else {
            // TODO(dcunnin): Support arrays (to tuples).
            // TODO(dcunnin): Support objects (to dicts).
            Py_DECREF(arglist);
            *succ = 0;
            *ctx->py_thread = PyEval_SaveThread();
            return jsonnet_json_make_string(ctx->vm, "Non-primitive param.");
        }
        PyTuple_SetItem(arglist, i, pyobj);
    }

    // Call python function.
    result = PyObject_CallObject(ctx->callback, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
        // Get string from exception.
        struct StrRefAndObj err_utf8 = exc_to_str();
        struct JsonnetJsonValue *r = jsonnet_json_make_string(ctx->vm, err_utf8.cstr);
        release_py_utf8_string(&err_utf8);
        *succ = 0;
        PyErr_Clear();
        *ctx->py_thread = PyEval_SaveThread();
        return r;
    }

    const char *err_msg;
    struct JsonnetJsonValue *r = python_to_jsonnet_json(ctx->vm, result, &err_msg);
    if (r != NULL) {
        *succ = 1;
    } else {
        *succ = 0;
        r = jsonnet_json_make_string(ctx->vm, err_msg);
    }
    *ctx->py_thread = PyEval_SaveThread();
    return r;
}


struct ImportCtx {
    struct JsonnetVm *vm;
    PyThreadState **py_thread;
    PyObject *callback;
};

static int cpython_import_callback(void *ctx_, const char *base, const char *rel,
                                   char **found_here, char **buf, size_t *buflen)
{
    const struct ImportCtx *ctx = ctx_;
    PyObject *arglist, *result;
    int success;

    PyEval_RestoreThread(*ctx->py_thread);
    arglist = Py_BuildValue("(s, s)", base, rel);
    result = PyObject_CallObject(ctx->callback, arglist);
    Py_DECREF(arglist);

    if (result == NULL) {
        // Get string from exception
        struct StrRefAndObj err_utf8 = exc_to_str();
        *buf = jsonnet_str_nonull(ctx->vm, err_utf8.cstr, buflen);
        release_py_utf8_string(&err_utf8);
        PyErr_Clear();
        *ctx->py_thread = PyEval_SaveThread();
        return 1; // failure
    }

    if (!PyTuple_Check(result)) {
        *buf = jsonnet_str_nonull(ctx->vm, "import_callback did not return a tuple", buflen);
        success = 0;
    } else if (PyTuple_Size(result) != 2) {
        *buf = jsonnet_str_nonull(ctx->vm, "import_callback did not return a tuple (size 2)", buflen);
        success = 0;
    } else {
        PyObject *file_name = PyTuple_GetItem(result, 0);
        PyObject *file_content = PyTuple_GetItem(result, 1);
        if (!PyUnicode_Check(file_name) || !PyBytes_Check(file_content)) {
            *buf = jsonnet_str_nonull(ctx->vm, "import_callback did not return (string, bytes). Since 0.19.0 imports should be returned as bytes instead of as a string.  You may want to call .encode() on your string.", buflen);
            success = 0;
        } else {
            char *content_buf;
            Py_ssize_t content_len;
            struct StrRefAndObj found_here_utf8 = get_py_utf8_string(file_name);
            PyBytes_AsStringAndSize(file_content, &content_buf, &content_len);
            *found_here = jsonnet_str(ctx->vm, found_here_utf8.cstr);
            release_py_utf8_string(&found_here_utf8);
            *buflen = content_len;
            *buf = jsonnet_realloc(ctx->vm, NULL, *buflen);
            memcpy(*buf, content_buf, *buflen);
            success = 1;
        }
    }

    Py_DECREF(result);
    *ctx->py_thread = PyEval_SaveThread();

    return success ? 0 : 1;
}

static PyObject *handle_result(struct JsonnetVm *vm, char *out, int error)
{
    if (error) {
        PyErr_SetString(PyExc_RuntimeError, out);
        jsonnet_realloc(vm, out, 0);
        jsonnet_destroy(vm);
        return NULL;
    } else {
        PyObject *ret = PyUnicode_FromString(out);
        jsonnet_realloc(vm, out, 0);
        jsonnet_destroy(vm);
        return ret;
    }
}

int handle_vars(struct JsonnetVm *vm, PyObject *map, int code, int tla)
{
    if (map == NULL) return 1;

    PyObject *key, *val;
    Py_ssize_t pos = 0;

    while (PyDict_Next(map, &pos, &key, &val)) {
        struct StrRefAndObj key_utf8 = get_py_utf8_string(key);
        if (!key_utf8.cstr) {
            release_py_utf8_string(&key_utf8);
            jsonnet_destroy(vm);
            return 0;
        }
        struct StrRefAndObj val_utf8 = get_py_utf8_string(val);
        if (!val_utf8.cstr) {
            release_py_utf8_string(&val_utf8);
            release_py_utf8_string(&key_utf8);
            jsonnet_destroy(vm);
            return 0;
        }
        if (!tla && !code) {
            jsonnet_ext_var(vm, key_utf8.cstr, val_utf8.cstr);
        } else if (!tla && code) {
            jsonnet_ext_code(vm, key_utf8.cstr, val_utf8.cstr);
        } else if (tla && !code) {
            jsonnet_tla_var(vm, key_utf8.cstr, val_utf8.cstr);
        } else {
            jsonnet_tla_code(vm, key_utf8.cstr, val_utf8.cstr);
        }
        release_py_utf8_string(&val_utf8);
        release_py_utf8_string(&key_utf8);
    }
    return 1;
}


int handle_import_callback(struct ImportCtx *ctx, PyObject *import_callback)
{
    if (import_callback == NULL) return 1;

    if (!PyCallable_Check(import_callback)) {
        jsonnet_destroy(ctx->vm);
        PyErr_SetString(PyExc_TypeError, "import_callback must be callable");
        return 0;
    }

    jsonnet_import_callback(ctx->vm, cpython_import_callback, ctx);

    return 1;
}


/** Register native callbacks with Jsonnet VM.
 *
 * Example native_callbacks = { 'name': (('p1', 'p2', 'p3'), func) }
 *
 * May set *ctxs, in which case it should be free()'d by caller.
 *
 * \returns 1 on success, 0 with exception set upon failure.
 */
static int handle_native_callbacks(struct JsonnetVm *vm, PyObject *native_callbacks,
                                   struct NativeCtx **ctxs, PyThreadState **py_thread)
{
    size_t num_natives = 0;
    PyObject *key, *val;
    Py_ssize_t pos = 0;

    if (native_callbacks == NULL) return 1;

    /* Verify the input before we allocate memory, throw all errors at this point.
     * Also, count the callbacks to see how much memory we need.
     */
    while (PyDict_Next(native_callbacks, &pos, &key, &val)) {
        Py_ssize_t i;
        Py_ssize_t num_params;
        PyObject *params;
        if (!PyUnicode_Check(key)) {
            PyErr_SetString(PyExc_TypeError, "native callback dict keys must be string");
            goto bad;
        }
        if (!PyTuple_Check(val)) {
            PyErr_SetString(PyExc_TypeError, "native callback dict values must be tuples");
            goto bad;
        } else if (PyTuple_Size(val) != 2) {
            PyErr_SetString(PyExc_TypeError, "native callback tuples must have size 2");
            goto bad;
        }
        params = PyTuple_GetItem(val, 0);
        if (!PyTuple_Check(params)) {
            PyErr_SetString(PyExc_TypeError, "native callback params must be a tuple");
            goto bad;
        }
        /* Check the params are all strings */
        num_params = PyTuple_Size(params);
        for (i = 0; i < num_params ; ++i) {
            PyObject *param = PyTuple_GetItem(params, 0);
            if (!PyUnicode_Check(param)) {
                PyErr_SetString(PyExc_TypeError, "native callback param must be string");
                goto bad;
            }
        }
        if (!PyCallable_Check(PyTuple_GetItem(val, 1))) {
            PyErr_SetString(PyExc_TypeError, "native callback must be callable");
            goto bad;
        }

        num_natives++;
        continue;

        bad:
        jsonnet_destroy(vm);
        return 0;
    }

    if (num_natives == 0) {
        return 1;
    }

    *ctxs = malloc(sizeof(struct NativeCtx) * num_natives);

    /* Re-use num_natives but just as a counter this time. */
    num_natives = 0;
    pos = 0;
    while (PyDict_Next(native_callbacks, &pos, &key, &val)) {
        Py_ssize_t i;
        Py_ssize_t num_params;
        PyObject *params;
        struct StrRefAndObj key_utf8 = get_py_utf8_string(key);
        params = PyTuple_GetItem(val, 0);
        num_params = PyTuple_Size(params);
        /* Include space for terminating NULL. */
        const char **params_c = malloc(sizeof(const char*) * (num_params + 1));
        for (i = 0; i < num_params ; ++i) {
            struct StrRefAndObj param_c_utf8 = get_py_utf8_string(PyTuple_GetItem(params, i));
            params_c[i] = param_c_utf8.cstr;
            release_py_utf8_string(&param_c_utf8);
        }
        params_c[num_params] = NULL;
        (*ctxs)[num_natives].vm = vm;
        (*ctxs)[num_natives].py_thread = py_thread;
        (*ctxs)[num_natives].callback = PyTuple_GetItem(val, 1);
        (*ctxs)[num_natives].argc = num_params;
        jsonnet_native_callback(vm, key_utf8.cstr, cpython_native_callback, &(*ctxs)[num_natives],
                                params_c);
        release_py_utf8_string(&key_utf8);
        free(params_c);
        num_natives++;
    }

    return 1;
}


static PyObject* evaluate_file(PyObject* self, PyObject* args, PyObject *keywds)
{
    const char *filename;
    char *out;
    unsigned max_stack = 500, gc_min_objects = 1000, max_trace = 20;
    double gc_growth_trigger = 2;
    int error;
    Py_ssize_t num_jpathdir, i;
    PyObject *jpathdir = NULL;
    PyObject *ext_vars = NULL, *ext_codes = NULL;
    PyObject *tla_vars = NULL, *tla_codes = NULL;
    PyObject *import_callback = NULL;
    PyObject *native_callbacks = NULL;
    struct JsonnetVm *vm;
    static char *kwlist[] = {
        "filename", "jpathdir",
        "max_stack", "gc_min_objects", "gc_growth_trigger", "ext_vars",
        "ext_codes", "tla_vars", "tla_codes", "max_trace", "import_callback",
        "native_callbacks",
        NULL
    };

    (void) self;

    if (!PyArg_ParseTupleAndKeywords(
        args, keywds, "s|OIIdOOOOIOO", kwlist,
        &filename, &jpathdir,
        &max_stack, &gc_min_objects, &gc_growth_trigger, &ext_vars,
        &ext_codes, &tla_vars, &tla_codes, &max_trace, &import_callback,
        &native_callbacks)) {
        return NULL;
    }

    PyThreadState *py_thread;

    vm = jsonnet_make();
    jsonnet_max_stack(vm, max_stack);
    jsonnet_gc_min_objects(vm, gc_min_objects);
    jsonnet_max_trace(vm, max_trace);
    jsonnet_gc_growth_trigger(vm, gc_growth_trigger);

    if (jpathdir != NULL) {
        // Support string for backward compatibility with <= 0.15.0
        if (PyUnicode_Check(jpathdir)) {
            struct StrRefAndObj jpath_utf8 = get_py_utf8_string(jpathdir);
            jsonnet_jpath_add(vm, jpath_utf8.cstr);
            release_py_utf8_string(&jpath_utf8);
        } else if (PyList_Check(jpathdir)) {
            num_jpathdir = PyList_Size(jpathdir);
            for (i = 0; i < num_jpathdir ; ++i) {
                PyObject *jpath = PyList_GetItem(jpathdir, i);
                if (PyUnicode_Check(jpath)) {
                    struct StrRefAndObj jpath_utf8 = get_py_utf8_string(jpath);
                    jsonnet_jpath_add(vm, jpath_utf8.cstr);
                    release_py_utf8_string(&jpath_utf8);
                }
            }
        }
    }

    if (!handle_vars(vm, ext_vars, 0, 0)) return NULL;
    if (!handle_vars(vm, ext_codes, 1, 0)) return NULL;
    if (!handle_vars(vm, tla_vars, 0, 1)) return NULL;
    if (!handle_vars(vm, tla_codes, 1, 1)) return NULL;

    struct ImportCtx ctx = { vm, &py_thread, import_callback };
    if (!handle_import_callback(&ctx, import_callback)) {
        return NULL;
    }
    struct NativeCtx *ctxs = NULL;
    if (!handle_native_callbacks(vm, native_callbacks, &ctxs, &py_thread)) {
        free(ctxs);
        return NULL;
    }
    py_thread = PyEval_SaveThread();
    out = jsonnet_evaluate_file(vm, filename, &error);
    PyEval_RestoreThread(py_thread);
    free(ctxs);
    return handle_result(vm, out, error);
}

static PyObject* evaluate_snippet(PyObject* self, PyObject* args, PyObject *keywds)
{
    const char *filename, *src;
    char *out;
    unsigned max_stack = 500, gc_min_objects = 1000, max_trace = 20;
    double gc_growth_trigger = 2;
    int error;
    Py_ssize_t num_jpathdir, i;
    PyObject *jpathdir = NULL;
    PyObject *ext_vars = NULL, *ext_codes = NULL;
    PyObject *tla_vars = NULL, *tla_codes = NULL;
    PyObject *import_callback = NULL;
    PyObject *native_callbacks = NULL;
    struct JsonnetVm *vm;
    static char *kwlist[] = {
        "filename", "src", "jpathdir",
        "max_stack", "gc_min_objects", "gc_growth_trigger", "ext_vars",
        "ext_codes", "tla_vars", "tla_codes", "max_trace", "import_callback",
        "native_callbacks",
        NULL
    };

    (void) self;

    if (!PyArg_ParseTupleAndKeywords(
        args, keywds, "ss|OIIdOOOOIOO", kwlist,
        &filename, &src, &jpathdir,
        &max_stack, &gc_min_objects, &gc_growth_trigger, &ext_vars,
        &ext_codes, &tla_vars, &tla_codes, &max_trace, &import_callback,
        &native_callbacks)) {
        return NULL;
    }

    PyThreadState *py_thread;

    vm = jsonnet_make();
    jsonnet_max_stack(vm, max_stack);
    jsonnet_gc_min_objects(vm, gc_min_objects);
    jsonnet_max_trace(vm, max_trace);
    jsonnet_gc_growth_trigger(vm, gc_growth_trigger);

    if (jpathdir != NULL) {
        // Support string for backward compatibility with <= 0.15.0
        if (PyUnicode_Check(jpathdir)) {
            struct StrRefAndObj jpath_utf8 = get_py_utf8_string(jpathdir);
            jsonnet_jpath_add(vm, jpath_utf8.cstr);
            release_py_utf8_string(&jpath_utf8);
        } else if (PyList_Check(jpathdir)) {
            num_jpathdir = PyList_Size(jpathdir);
            for (i = 0; i < num_jpathdir ; ++i) {
                PyObject *jpath = PyList_GetItem(jpathdir, i);
                if (PyUnicode_Check(jpath)) {
                    struct StrRefAndObj jpath_utf8 = get_py_utf8_string(jpath);
                    jsonnet_jpath_add(vm, jpath_utf8.cstr);
                    release_py_utf8_string(&jpath_utf8);
                }
            }
        }
    }

    if (!handle_vars(vm, ext_vars, 0, 0)) return NULL;
    if (!handle_vars(vm, ext_codes, 1, 0)) return NULL;
    if (!handle_vars(vm, tla_vars, 0, 1)) return NULL;
    if (!handle_vars(vm, tla_codes, 1, 1)) return NULL;
    struct ImportCtx ctx = { vm, &py_thread, import_callback };
    if (!handle_import_callback(&ctx, import_callback)) {
        return NULL;
    }
    struct NativeCtx *ctxs = NULL;
    if (!handle_native_callbacks(vm, native_callbacks, &ctxs, &py_thread)) {
        free(ctxs);
        return NULL;
    }
    py_thread = PyEval_SaveThread();
    out = jsonnet_evaluate_snippet(vm, filename, src, &error);
    PyEval_RestoreThread(py_thread);
    free(ctxs);
    return handle_result(vm, out, error);
}

static PyMethodDef module_methods[] = {
    {"evaluate_file", (PyCFunction)evaluate_file, METH_VARARGS | METH_KEYWORDS,
     "Interpret the given Jsonnet file."},
    {"evaluate_snippet", (PyCFunction)evaluate_snippet, METH_VARARGS | METH_KEYWORDS,
     "Interpret the given Jsonnet code."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef _module =
{
    PyModuleDef_HEAD_INIT,
    "_jsonnet",
    "A Python interface to Jsonnet.",
    -1,
    module_methods,
};

PyMODINIT_FUNC PyInit__jsonnet(void)
{
    PyObject *module = PyModule_Create(&_module);
    PyObject *version_str = PyUnicode_FromString(LIB_JSONNET_VERSION);
    if (PyModule_AddObject(module, "version", PyUnicode_FromString(LIB_JSONNET_VERSION)) < 0) {
      Py_XDECREF(version_str);
    }
    return module;
}
