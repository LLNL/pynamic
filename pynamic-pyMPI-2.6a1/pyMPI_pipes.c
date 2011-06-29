/**************************************************************************/
/* FILE   **************       pyMPI_pipes.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller January 14 2003                                 */
/**************************************************************************/
/* We have to modify some Python behavior to make it play nice            */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************      safe_file_close      ************************/
/**************************************************************************/
/* This proxy will call the original file_close, but will block SIGCHLD's */
/* to avoid weird pipe errors.                                            */
/**************************************************************************/
static PyCFunction unsafe_file_close = 0;
static PyObject* safe_file_close(PyObject* self, PyObject* args) {
  sigset_t newset;
  sigset_t signalset;
  PyObject* result = 0;
  void (*old_handler)(int) = 0;
  void (*restored_handler)(int) = 0;

  int status;

  /* ----------------------------------------------- */
  /* Block SIGCHLD for a moment                      */
  /* ----------------------------------------------- */
  status = sigemptyset(&newset);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);
  status = sigaddset(&newset,SIGCHLD);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);
  status = sigprocmask(SIG_BLOCK,&newset,&signalset);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);

  /* ----------------------------------------------- */
  /* Grab the old handler                            */
  /* ----------------------------------------------- */
  old_handler = signal(SIGCHLD,SIG_DFL);
  if ( old_handler == SIG_ERR ) return PyErr_SetFromErrno(PyExc_OSError);

  /* ----------------------------------------------- */
  /* Make the original call                          */
  /* ----------------------------------------------- */
  result/*owned*/ = unsafe_file_close(self,args);

  /* ----------------------------------------------- */
  /* Put the original handler back                   */
  /* ----------------------------------------------- */
  restored_handler = signal(SIGCHLD,old_handler);
  if ( restored_handler == SIG_ERR ) return PyErr_SetFromErrno(PyExc_OSError);

  /* ----------------------------------------------- */
  /* Restore the SIGCHLD signal                      */
  /* ----------------------------------------------- */
  status = sigprocmask(SIG_SETMASK,&signalset,0);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);

  return result;
}

/**************************************************************************/
/* LOCAL  **************      safe_file_read      ************************/
/**************************************************************************/
/* This proxy will call the original file_read, but will block SIGCHLD's */
/* to avoid weird pipe errors.                                            */
/**************************************************************************/
static PyCFunction unsafe_file_read = 0;
static PyObject* safe_file_read(PyObject* self, PyObject* args) {
  PyObject* result = 0;
  sigset_t newset;
  sigset_t signalset;
  void (*old_handler)(int) = 0;
  void (*restored_handler)(int) = 0;
  int status;

  /* ----------------------------------------------- */
  /* Block SIGCHLD for a moment                      */
  /* ----------------------------------------------- */
  status = sigemptyset(&newset);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);
  status = sigaddset(&newset,SIGCHLD);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);
  status = sigprocmask(SIG_BLOCK,&newset,&signalset);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);

  /* ----------------------------------------------- */
  /* Grab the old handler                            */
  /* ----------------------------------------------- */
  old_handler = signal(SIGCHLD,SIG_DFL);
  if ( old_handler == SIG_ERR ) return PyErr_SetFromErrno(PyExc_OSError);

  /* ----------------------------------------------- */
  /* Make the original call                          */
  /* ----------------------------------------------- */
  result/*owned*/ = unsafe_file_read(self,args);

  /* ----------------------------------------------- */
  /* Put the original handler back                   */
  /* ----------------------------------------------- */
  restored_handler = signal(SIGCHLD,old_handler);
  if ( restored_handler == SIG_ERR ) return PyErr_SetFromErrno(PyExc_OSError);

  /* ----------------------------------------------- */
  /* Restore the SIGCHLD signal                      */
  /* ----------------------------------------------- */
  status = sigprocmask(SIG_SETMASK,&signalset,0);
  if ( status == -1 ) return PyErr_SetFromErrno(PyExc_OSError);

  return result;
}

/**************************************************************************/
/* LOCAL  **************        mpi_waitpid        ************************/
/**************************************************************************/
/* Fixes waitpid call in Python to pass SIGCHLD calls                     */
/**************************************************************************/
static PyObject* mpi_waitpid(PyObject* pySelf, PyObject* args) {
  sigset_t newset;
  sigset_t signalset;
  int pid, rpid, options;
#ifdef UNION_WAIT
  union wait status;
#define status_i (status.w_status)
#else
  int status;
#define status_i status
#endif
  status_i = 0;

  if (!PyArg_ParseTuple(args, "ii:waitpid", &pid, &options))
    return NULL;
  Py_BEGIN_ALLOW_THREADS
    {
      sigemptyset(&newset);
      sigaddset(&newset,SIGCHLD);
      sigprocmask(SIG_BLOCK,&newset,&signalset);
      rpid = waitpid(pid, &status, options);
      sigprocmask(SIG_SETMASK,&signalset,0);
    }
  Py_END_ALLOW_THREADS
    if ( rpid == -1 ) {
      return PyErr_SetFromErrno(PyExc_OSError);
    } else {
      return Py_BuildValue("ii", rpid, status_i);
    }
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_pipes_init     ************************/
/**************************************************************************/
/* Some MPI libraries signals interfere with the standard implementations */
/* for pipes.                                                             */
/**************************************************************************/
void pyMPI_pipes_init(PyObject** docStringP) {
  PyTypeObject* file_type_pointer = 0;
  PyMethodDef *file_methods = 0;
  PyObject* os = 0;
  PyObject* py_waitpid = 0;
  
  /* ----------------------------------------------- */
  /* Here, we apply a "fix" to the file_close meth   */
  /* The issue is one of unsafe SIGCHLD handlers for */
  /* MPICH.  Pipe close is only well defined under   */
  /* SIG_DFL for SIGCHLD.                            */
  /* NOTE:  Use (&PyFile_Type) to fix Cygwin DLL bug */
  /* http://www-unix.mcs.anl.gov/mpi/www/www3/MPI_Init.html */
  /* ----------------------------------------------- */
  file_type_pointer/*borrowed*/ = (pyMPI_rank>=0)?(&(PyFile_Type)):(0);
  for(file_methods=file_type_pointer->tp_methods; file_methods && file_methods->ml_name; ++file_methods) {
    if ( strcmp(file_methods->ml_name,"close") == 0 ) {
      unsafe_file_close = file_methods->ml_meth;
      file_methods->ml_meth = safe_file_close;
    }
    if ( strcmp(file_methods->ml_name,"read") == 0 ) {
      unsafe_file_read = file_methods->ml_meth;
      file_methods->ml_meth = safe_file_read;
    }
  }

  /* ----------------------------------------------- */
  /* The waitpid call tends to fail under MPICH      */
  /* since the system call is interrupted.           */
  /* ----------------------------------------------- */
  if ( PyErr_Occurred() ) {
    PyErr_Clear();
  } else {
    static PyMethodDef waitpid = {
      "mpi_waitpid",mpi_waitpid,METH_VARARGS,"mpi variant of os.waitpid"
    };
    os = PyImport_ImportModule("os");
    Assert(os);
    PYCHECK( py_waitpid/*owned*/ = PyCFunction_New(&waitpid,0) );
    PYCHECK( PyObject_SetAttrString(os,"waitpid",py_waitpid) );
    py_waitpid = 0;
  }


  /* Fall through */
 pythonError:
  Py_XDECREF(os);
  return;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_pipes_fini     ************************/
/**************************************************************************/
/* Clean up work                                                          */
/**************************************************************************/
void pyMPI_pipes_fini(void) {
}

END_CPLUSPLUS

