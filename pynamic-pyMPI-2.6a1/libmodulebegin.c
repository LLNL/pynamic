#include <Python.h>

volatile int v;

void begin_break_here()
{
    v++;
}

static PyObject *py_libmodulebegin_break_here(PyObject *self, PyObject *args)
{
    begin_break_here();
    Py_RETURN_NONE;
}

static PyMethodDef libmodulebegin_importMethods[] = {
    {"begin_break_here", py_libmodulebegin_break_here, METH_VARARGS, "a function."},
    {NULL, NULL, 0, NULL}
};

void initlibmodulebegin()
{
    Py_InitModule("libmodulebegin", libmodulebegin_importMethods);
}
