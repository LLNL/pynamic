#include "Python.h"

static PyMethodDef methods[] = {
  {0,0}
};

void initsimple(void) {
  PyObject* m;
  PyObject* d;
  m = Py_InitModule("simple",methods);
  d = PyModule_GetDict(m);
#ifdef HAVE_MPI
  PyObject_CallMethod(d,"__setitem__","ss","foom","parallel zoom");
#else
  PyObject_CallMethod(d,"__setitem__","ss","foom","serial zoom");
#endif
}
