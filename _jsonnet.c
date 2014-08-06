/*
Copyright 2014 Google Inc. All rights reserved.

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

static PyObject* evaluate_file(PyObject* self, PyObject* args)
{
    const char *filename, *out, *error;

    (void) self;

    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    out = jsonnet_evaluate_file(filename, &error);
    if (out == NULL) {
        PyErr_SetString(PyExc_RuntimeError, error);
        jsonnet_delete(error);
        return NULL;
    } else {
        PyObject *ret = PyString_FromString(out);
        jsonnet_delete(out);
        return ret;
    }
}

static PyObject* evaluate_snippet(PyObject* self, PyObject* args)
{
    const char *filename, *src, *out, *error;

    (void) self;

    if (!PyArg_ParseTuple(args, "ss", &filename, &src))
        return NULL;

    out = jsonnet_evaluate_snippet(filename, src, &error);
    if (out == NULL) {
        PyErr_SetString(PyExc_RuntimeError, error);
        jsonnet_delete(error);
        return NULL;
    } else {
        PyObject *ret = PyString_FromString(out);
        jsonnet_delete(out);
        return ret;
    }
}

static PyMethodDef module_methods[] = {
    {"evaluate_file", evaluate_file, METH_VARARGS, "Interpret the given Jsonnet file."},
    {"evaluate_snippet", evaluate_snippet, METH_VARARGS, "Interpret the given Jsonnet code."},
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC init_jsonnet(void)
{
    Py_InitModule3("_jsonnet", module_methods, "A Python interface to Jsonnet.");
}

