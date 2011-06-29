/**************************************************************************/
/* FILE   **************       pyMPI_pickle.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller January 14 2003                                 */
/**************************************************************************/
/* Serialization support                                                  */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************        pyMPI_pickle       ************************/
/**************************************************************************/
/* Owned reference to cPickle module object                               */
/**************************************************************************/
PyObject* pyMPI_pickle = 0;

/**************************************************************************/
/* GLOBAL **************     pyMPI_pickle_loads    ************************/
/**************************************************************************/
/* Owned reference to loads() function for pickling                       */
/**************************************************************************/
PyObject* pyMPI_pickle_loads = 0;

/**************************************************************************/
/* GLOBAL **************     pyMPI_pickle_dumps    ************************/
/**************************************************************************/
/* Owned reference to dumps() function for pickling                       */
/**************************************************************************/
PyObject* pyMPI_pickle_dumps = 0;

/**************************************************************************/
/* GLOBAL **************     pyMPI_pickle_init     ************************/
/**************************************************************************/
/* We use the "pickle" module to serialize data for sending               */
/**************************************************************************/
void pyMPI_pickle_init(PyObject** docStringP) {
  COVERAGE();

  PYCHECK( pyMPI_pickle /*owned*/ = PyImport_ImportModule("cPickle") );
  PYCHECK( pyMPI_pickle_loads /*owned*/ = PyObject_GetAttrString(pyMPI_pickle,"loads") );
  PYCHECK( pyMPI_pickle_dumps /*owned*/ = PyObject_GetAttrString(pyMPI_pickle,"dumps") );
  return;

 pythonError:
  Py_XDECREF(pyMPI_pickle);
  pyMPI_pickle = 0;

  Py_XDECREF(pyMPI_pickle_loads);
  pyMPI_pickle_loads = 0;

  Py_XDECREF(pyMPI_pickle_dumps);
  pyMPI_pickle_dumps = 0;
  return;
}


/**************************************************************************/
/* GLOBAL **************     pyMPI_pickle_fini     ************************/
/**************************************************************************/
/* Here, we release our PyObjects...                                      */
/**************************************************************************/
void pyMPI_pickle_fini(void) {
  Py_XDECREF( pyMPI_pickle );
  pyMPI_pickle = 0;

  Py_XDECREF( pyMPI_pickle_loads );
  pyMPI_pickle_loads = 0;

  Py_XDECREF( pyMPI_pickle_dumps );
  pyMPI_pickle_dumps = 0;
}

END_CPLUSPLUS

