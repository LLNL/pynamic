import sys
import os
import glob
import mpi

from distutils.core import setup,Extension

open('extensiontest.c','w').write("""
#include "Python.h"
#ifdef HAVE_MPI
#include "mpi.h"
#endif

static PyObject* extensiontest_test(PyObject* self,PyObject* args) {
  int rank;
  /* Note this will fail if MPI isn't included, but this   */
  /* does help insure that the -DHAVE_MPI flag is included */
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  return PyInt_FromLong(rank);
}

static PyMethodDef methods[] = {
  {"test",extensiontest_test,METH_VARARGS,"simple MPI test function"},
  {0,0}
};

void initextensiontest(void) {
  Py_InitModule("extensiontest",methods);
}
""")

cwd = os.getcwd()
sys.path.insert(0,cwd)
setup(ext_modules=[Extension('extensiontest',['extensiontest.c'])],script_args=['install','--install-lib=%s'%cwd])

import extensiontest
assert extensiontest.test() == mpi.rank

# Clean up files
print cwd
files = (
    glob.glob(os.path.join(cwd,'extensiontest*'))+
    glob.glob(os.path.join(cwd,'build','*','*'))+
    glob.glob(os.path.join(cwd,'build','*'))+
    glob.glob(os.path.join(cwd,'build'))
    )
for file in files:
    if os.path.isdir(file):
        print '%% rmdir %s'%file
        os.rmdir(file)
    else:
        print '%% rm %s'%file
        try:
            os.unlink(file)
        except:
            pass
