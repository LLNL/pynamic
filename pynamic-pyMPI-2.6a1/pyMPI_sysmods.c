/**************************************************************************/
/* FILE   *************        pyMPI_sysmods.c       **********************/
/**************************************************************************/
/* Author: Patrick Miller January 15 2003                                 */
/**************************************************************************/
/* Modify sys.exit to work better with MPI                                */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************    original_excepthook    ************************/
/**************************************************************************/
/* Holds original excepthook function from system                         */
/**************************************************************************/
static PyObject* original_excepthook = 0;

/**************************************************************************/
/* LOCAL  **************         excepthook        ************************/
/**************************************************************************/
/* Just notices that excepthook was just called so that pyMPI_exit_status */
/* can be properly set.  Note that the status is set to 0 in readline so  */
/* that interactive works properly.                                       */
/**************************************************************************/
static PyObject* excepthook(PyObject* self, PyObject* args) {

  pyMPI_exit_status = 1;

  if ( original_excepthook ) {
    return PyObject_Call(original_excepthook,args,0);
  }

  Py_INCREF(Py_None);
  return Py_None;
}

/**************************************************************************/
/* LOCAL  **************     excepthook_method     ************************/
/**************************************************************************/
/* Method entry to build excepthook function to sit in between pyMPI and  */
/* python.                                                                */
/**************************************************************************/
static PyMethodDef excepthook_method = {"excepthook",excepthook,METH_VARARGS,
                              "excepthook(exctype, value, traceback) -> None\n\nHandle an exception by displaying it with a traceback on sys.stderr.\n"};

#ifndef PYMPI_MACOSX
/* TODO: MAC linker has an issue with redefining this... skip it for now */

/**************************************************************************/
/* GLOBAL **************       Py_GetVersion       ************************/
/**************************************************************************/
/* Replacement for Python's getversion                                    */
/**************************************************************************/
const char* Py_GetVersion(void) {
  static char version[250];
  PyOS_snprintf(version, sizeof(version), "%.80s (%.80s %.80s)", 
                PY_VERSION,PACKAGE,VERSION);
  pyMPI_banner(version+strlen(PY_VERSION)+1,(int)(sizeof(version)-strlen(PY_VERSION)+1));
  return version;
}
#endif


/**************************************************************************/
/* LOCAL  **************      sys_exit_called      ************************/
/**************************************************************************/
/* Flag to indicate that we are exiting via sys.exit()                    */
/**************************************************************************/
static int sys_exit_called = 0;

/**************************************************************************/
/* LOCAL  **************     original_sys_exit     ************************/
/**************************************************************************/
/* A reference to the original sys.exit()                                 */
/**************************************************************************/
static PyObject* original_sys_exit = 0;

/**************************************************************************/
/* GLOBAL **************     pyMPI_exit_status     ************************/
/**************************************************************************/
/* Hold the exit status for user controlled finalize                      */
/**************************************************************************/
int pyMPI_exit_status = 0;

/**************************************************************************/
/* LOCAL  **************       modified_exit       ************************/
/**************************************************************************/
/* A new exit function which sets our flag and calls the original         */
/**************************************************************************/
static PyObject* modified_exit(PyObject* self, PyObject* args) {
  PyObject* result = 0;
  
  pyMPI_exit_status = 1;

  // We may have an integer status here... ignore it
  // if we don't, it just means we accept the default
  // exit status integer value.
  PyArg_ParseTuple(args,"|i",&pyMPI_exit_status);
  PyErr_Clear();

  /* ----------------------------------------------- */
  /* Tell the _fini() routine to finalize.  Please   */
  /* note that this isn't exactly right.  One could  */
  /* throw SystemExit and then catch it.  Mostly the */
  /* right thing to do.                              */
  /* ----------------------------------------------- */
  sys_exit_called = 1;

  /* ----------------------------------------------- */
  /* This will always return an exception...         */
  /* ----------------------------------------------- */
  result = PyObject_CallObject(original_sys_exit,args);
  return result;

 pythonError:
  pyMPI_exit_status = 0;
  return 0;
}

/**************************************************************************/
/* LOCAL  **************       exit_function       ************************/
/**************************************************************************/
/* The exit function.  NOTE:  The ml_doc field is copied from the real    */
/* exit function before we make our version.                              */
/**************************************************************************/
static PyMethodDef exit_function = {"exit",modified_exit,METH_VARARGS,0};

/**************************************************************************/
/* GLOBAL **************     pyMPI_sysmods_init    ************************/
/**************************************************************************/
/* Update sys.exit() to call MPI_Finalize()                               */
/**************************************************************************/
void pyMPI_sysmods_init(PyObject** docstring_pointer) {
  PyObject* modified = 0;
  PyObject* docstring = 0;
  PyObject* emergency = 0;

  /* ----------------------------------------------- */
  /* Get the original sys.exit function              */
  /* ----------------------------------------------- */
  PYCHECK( original_sys_exit /*borrowed*/ = PySys_GetObject("exit") );
  Py_INCREF(original_sys_exit);

  /* ----------------------------------------------- */
  /* Copy the docstring into the new exit function   */
  /* ----------------------------------------------- */
  PYCHECK( docstring /*owned*/ = PyObject_GetAttrString(original_sys_exit,"__doc__") );
  if ( docstring && PyString_Check(docstring) ) {
    exit_function.ml_doc = PyString_AS_STRING(docstring);
  }
  docstring = 0; /* LEAKS TO PRESERVE INTEGREGITY OF ml_doc */

  /* ----------------------------------------------- */
  /* Put the new one back in sys module              */
  /* ----------------------------------------------- */
  PYCHECK( modified /*owned*/ = PyCFunction_New(&exit_function,0) );
  PYCHECK( PySys_SetObject("exit",/*incs*/modified) );
  Py_XDECREF(modified);
  modified = 0;

  /* ----------------------------------------------- */
  /* Override the excepthook method to set the exit  */
  /* status flag                                     */
  /* ----------------------------------------------- */
  emergency = PyCFunction_New(&excepthook_method,0);
  original_excepthook/*borrow*/= PySys_GetObject("excepthook");
  Py_INCREF(original_excepthook);
  PySys_SetObject("excepthook",emergency);

  return;

 pythonError:
  Py_XDECREF(modified);
  return;
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_sysmods_fini    ************************/
/**************************************************************************/
/* Maybe I have to finalize MPI...                                        */
/**************************************************************************/
void pyMPI_sysmods_fini(void) {
  if ( original_excepthook ) {
    Py_DECREF(original_excepthook);
  }
  if ( sys_exit_called && pyMPI_owns_MPI) {
    MPI_Finalize();
  }
}

END_CPLUSPLUS
