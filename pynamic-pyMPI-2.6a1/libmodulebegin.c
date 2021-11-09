#include <Python.h>

static volatile int v;

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

#if PY_MAJOR_VERSION == 2
void initlibmodulebegin()
{
   Py_InitModule("libmodulebegin", libmodulebegin_importMethods);
}
#else
PyMODINIT_FUNC PyInit_libmodulebegin()
{
   static struct PyModuleDef beginmodule = {
      PyModuleDef_HEAD_INIT,
      "libmodulebeign",
      "",
      -1,
      libmodulebegin_importMethods
   };
   return PyModule_Create(&beginmodule);
}
#endif
