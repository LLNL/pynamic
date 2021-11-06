#include <Python.h>

static volatile int v;

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

#if PY_MAJOR_VERSION == 2
void initlibmodulefinal()
{
   Py_InitModule("libmodulefinal", libmodulefinal_importMethods);
}
#else
PyMODINIT_FUNC PyInit_libmodulefinal()
{
   static struct PyModuleDef finalmodule = {
      PyModuleDef_HEAD_INIT,
      "libmodulefinal",
      "",
      -1,
      libmodulefinal_importMethods
   };
   return PyModule_Create(&finalmodule);
}
#endif
