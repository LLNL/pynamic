#include <Python.h>

volatile int v;

void break_here()
{
    v++;
}

static PyObject *py_libmodulefinal_break_here(PyObject *self, PyObject *args)
{
    break_here();
    Py_RETURN_NONE;
}

static PyMethodDef libmodulefinal_importMethods[] = {
    {"break_here", py_libmodulefinal_break_here, METH_VARARGS, "a function."},
    {NULL, NULL, 0, NULL}
};

void initlibmodulefinal()
{
    Py_InitModule("libmodulefinal", libmodulefinal_importMethods);
}
