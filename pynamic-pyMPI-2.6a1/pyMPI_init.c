/**************************************************************************/
/* FILE   **************        pyMPI_init.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller May 15 2002                                     */
/* Copyright (C) 2002 Patrick J. Miller                                   */
/**************************************************************************/
/* Generic initialization                                                 */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************        pyMPI_module       ************************/
/**************************************************************************/
/* Reference to the "mpi" module                                          */
/**************************************************************************/
PyObject* pyMPI_module = 0;

/**************************************************************************/
/* GLOBAL **************      pyMPI_dictionary     ************************/
/**************************************************************************/
/* Reference to mpi.__dict__                                              */
/**************************************************************************/
PyObject* pyMPI_dictionary = 0;

/**************************************************************************/
/* GLOBAL **************       pyMPI_MPIError      ************************/
/**************************************************************************/
/* Reference to mpi.MPIError                                              */
/**************************************************************************/
PyObject* pyMPI_MPIError = 0;

/**************************************************************************/
/* GLOBAL **************         pyMPI_rank        ************************/
/**************************************************************************/
/* Global rank (rank on MPI_COMM_WORLD) for this process                  */
/**************************************************************************/
int pyMPI_rank = 0;

/**************************************************************************/
/* GLOBAL **************         pyMPI_size        ************************/
/**************************************************************************/
/* Number of processes known to MPI_COMM_WORLD                            */
/**************************************************************************/
int pyMPI_size = 1;

/**************************************************************************/
/* GLOBAL **************    pyMPI_packedIntSize    ************************/
/**************************************************************************/
/* Size of a packed integer on this process                               */
/**************************************************************************/
int pyMPI_packedIntSize = 0;

/**************************************************************************/
/* GLOBAL **************    pyMPI_packedCharSize   ************************/
/**************************************************************************/
/* Size of a packed character on this process                             */
/**************************************************************************/
int pyMPI_packedCharSize = 0;

/**************************************************************************/
/* GLOBAL **************      pyMPI_COMM_WORLD     ************************/
/**************************************************************************/
/* A clone of MPI_COMM_WORLD for use by Python.                           */
/**************************************************************************/
MPI_Comm pyMPI_COMM_WORLD = MPI_COMM_NULL;

/**************************************************************************/
/* MACRO  **************        PYMPI_PLUGIN       ************************/
/**************************************************************************/
/* Makes sure I remember _init() and _fini() calls AND error checks the   */
/* fini_count.                                                            */
/**************************************************************************/
#define PYMPI_PLUGIN(root) { \
  PYCHECK( root##_init(&doc_string) ); \
  Assert(fini_count < MAX_FINI_SET_COUNT); \
  fini_set[fini_count++] = root##_fini; \
}

/**************************************************************************/
/* LOCAL  **************     MAX_FINI_SET_COUNT    ************************/
/**************************************************************************/
/* Number of fini routines that will be executed as pyMPI plugins         */
/**************************************************************************/
#ifndef MAX_FINI_SET_COUNT
#define MAX_FINI_SET_COUNT 100
#endif

/**************************************************************************/
/* LOCAL  **********     MAX_HOUSEKEEPING_SET_COUNT    ********************/
/**************************************************************************/
/* Number of housekeeping routines that can be cached                     */
/**************************************************************************/
#ifndef MAX_HOUSEKEEPING_SET_COUNT
#define MAX_HOUSEKEEPING_SET_COUNT 100
#endif

/**************************************************************************/
/* LOCAL  **************          fini_set         ************************/
/**************************************************************************/
/* This is simply a list of last wish routines to run.                    */
/**************************************************************************/
static pyMPI_void_function fini_set[MAX_FINI_SET_COUNT];
static int fini_count = 0;

/**************************************************************************/
/* LOCAL  **************      housekeeping_set     ************************/
/**************************************************************************/
/* This is simply a list of periodic routines to run.                     */
/**************************************************************************/
static pyMPI_void_function occasional_housekeeping_set[MAX_HOUSEKEEPING_SET_COUNT];
static int occasional_housekeeping_count = 0;
static pyMPI_void_function intensive_housekeeping_set[MAX_HOUSEKEEPING_SET_COUNT];
static int intensive_housekeeping_count = 0;

/**************************************************************************/
/* LOCAL  **************      occasional_work      ************************/
/**************************************************************************/
/* Things to do every once in a while                                     */
/**************************************************************************/
static int occasional_work(PyObject* obj, struct _frame* frame, int what, PyObject* args) {
  int i;

  for(i=0;i<occasional_housekeeping_count;++i) {
    occasional_housekeeping_set[i]();
  }
  return 0;
}

/**************************************************************************/
/* LOCAL  **************       intensive_work      ************************/
/**************************************************************************/
/* Things to do every pretty often                                        */
/**************************************************************************/
static int intensive_work(PyObject* obj, struct _frame* frame, int what, PyObject* args) {
  int i;

  for(i=0;i<intensive_housekeeping_count;++i) {
    intensive_housekeeping_set[i]();
  }

  return 0;
}

/**************************************************************************/
/* GLOBAL ************** pyMPI_add_occasional_work ************************/
/**************************************************************************/
/* Turn on occasional work trace                                          */
/**************************************************************************/
void pyMPI_add_occasional_work(pyMPI_void_function F) {
  int i;

  PYCHECK( PyEval_SetProfile(occasional_work,0) ); /* Call every block */
  for(i=0;i<occasional_housekeeping_count;++i) {
    if ( F == occasional_housekeeping_set[i] ) return;
  }
  Assert(occasional_housekeeping_count < MAX_HOUSEKEEPING_SET_COUNT);
  occasional_housekeeping_set[occasional_housekeeping_count++] = F;
 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************  pyMPI_add_intensive_work ************************/
/**************************************************************************/
/* Turn on intensive work trace                                           */
/**************************************************************************/
void pyMPI_add_intensive_work(pyMPI_void_function F) {
  int i;

  PYCHECK( PyEval_SetTrace(intensive_work,0) ); /* Call every instruction */

  for(i=0;i<intensive_housekeeping_count;++i) {
    if ( F == intensive_housekeeping_set[i] ) return;
  }
  Assert(intensive_housekeeping_count < MAX_HOUSEKEEPING_SET_COUNT);
  intensive_housekeeping_set[intensive_housekeeping_count++] = F;
 pythonError:
  return;
}

/**************************************************************************/
/* LOCAL  **************         last_wish         ************************/
/**************************************************************************/
/* This routine runs cleanup for all the plugins.  The goal is to more    */
/* fully clean up MPI structures and memory.  This can help cut clutter   */
/* in MPI tools (like UMPIRE) and memory tools looking for leaks.         */
/**************************************************************************/
static void last_wish(void) {
  int i;

  /* ----------------------------------------------- */
  /* Release our communicators...                    */
  /* ----------------------------------------------- */
  if ( pyMPI_COMM_WORLD != MPI_COMM_NULL && pyMPI_COMM_WORLD != MPI_COMM_WORLD ) {
    MPI_Comm_free(&pyMPI_COMM_WORLD);
  }

  /* ----------------------------------------------- */
  /* Allow other plugins to cleanup.  Note that MPI  */
  /* may be finalized here on abnormal exit          */
  /* ----------------------------------------------- */
  for(i=0; i<fini_count; ++i) {
    fini_set[i]();
  }
  fini_count = 0;

  /* TODO: DECREF python objects, but must check ownership issues first */
}


/**************************************************************************/
/* LOCAL  **************          finalize         ************************/
/**************************************************************************/
/* Implements MPI_FINALIZE() --                                           */
/**************************************************************************/
static PyObject *finalize(PyObject * pySelf, PyObject * args)
{
  COVERAGE();

  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  NOARGUMENTS();
  RAISEIFNOMPI();

  MPICHECKCOMMLESS( MPI_Finalize() );

  Py_XINCREF(Py_None);
  return Py_None;

pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************         finalized         ************************/
/**************************************************************************/
/* Implements MPI_Finalized() (may use the builtin version)               */
/**************************************************************************/
static PyObject *finalized(PyObject * pySelf, PyObject * args)
{
  int finalizedFlag = 0;
  PyObject *result = 0;

  COVERAGE();

  /* ----------------------------------------------- */
  /* Does not need MPI to answer                     */
  /* ----------------------------------------------- */
  NOARGUMENTS();

#ifdef HAVE_MPI_FINALIZED
  MPICHECKCOMMLESS(MPI_Finalized(&finalizedFlag));
#endif

  PYCHECK(result = PyInt_FromLong(finalizedFlag));
  return result;

pythonError:
  return 0;

}

/**************************************************************************/
/* LOCAL  **************        initialized        ************************/
/**************************************************************************/
/* Implements MPI_initialized (if available).  Otherwise it reports that  */
/* MPI is ready since it cannot determine precisely.                      */
/**************************************************************************/
static PyObject *initialized(PyObject * pySelf, PyObject * args)
{
  PyObject *result = 0;

  /* ----------------------------------------------- */
  /* Coverage checkpoint                             */
  /* ----------------------------------------------- */
  COVERAGE();

  NOARGUMENTS();

  PYCHECK(result = PyInt_FromLong(pyMPI_util_ready()));
  return result;

pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************           wtick           ************************/
/**************************************************************************/
/* Get the time tick (a double)                                           */
/**************************************************************************/
 /*ARGSUSED*/ static PyObject *wtick(PyObject * pySelf, PyObject * args)
{
  double theTick = 0.0;
  PyObject *result = 0;

  COVERAGE();
  NOARGUMENTS();

  theTick = MPI_Wtick();
  PYCHECK(result = PyFloat_FromDouble(theTick));
  return result;

pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************           wtime           ************************/
/**************************************************************************/
/* Implements MPI_Wtime() to get high fidelity system timer               */
/**************************************************************************/
static PyObject *wtime(PyObject * pySelf, PyObject * args)
{
  double theTime = 0.0;
  PyObject *result = 0;

  COVERAGE();
  NOARGUMENTS();

  theTime = MPI_Wtime();
  PYCHECK(result = PyFloat_FromDouble(theTime));
  return result;

pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************         mpi_trace         ************************/
/**************************************************************************/
/* Implements the helper function mpi.trace().  On the rank 0 process, we */
/* converts all arguments to a space separated string, prints it out to   */
/* stderr, and flushes it.  Rank 0 refers to the MPI_COMM_WORLD rank 0    */
/* processor.                                                             */
/**************************************************************************/
static PyObject* mpi_trace(PyObject* pySelf, PyObject* args) {
  PyObject* line = 0;
  char* representation = 0;
  
  if ( pyMPI_rank == 0 ) {
    PYCHECK( line/*owned*/ = pyMPI_util_tuple_to_spaced_string(args) );
    PYCHECK( representation = PyString_AsString(line) );
  
    fputs(representation,stderr);
    fputc(' ',stderr);
    fflush(stderr);

    Py_XDECREF(line);
  }

  Py_INCREF(Py_None);
  return Py_None;

 pythonError:
  Py_XDECREF(line);
  return 0;
}

/**************************************************************************/
/* LOCAL  **************        mpi_traceln        ************************/
/**************************************************************************/
/* This implements mpi.traceln which is a shortcut to write data to only  */
/* the rank 0 processor.  It is just like trace(), but adds a newline     */
/* Output is always flushed to stderr.                                    */
/**************************************************************************/
static PyObject* mpi_traceln(PyObject* pySelf, PyObject* args) {
  PyObject* trace = 0;
  
  PYCHECK( trace/*owned*/ = mpi_trace(pySelf,args) );

  if ( pyMPI_rank == 0 ) {
    fputc('\n',stderr);
    fflush(stderr);
  }

  return trace;

 pythonError:
  Py_XDECREF(trace);
  return 0;
}

static PyObject* bcast_input_to_slaves(PyObject* pySelf, PyObject* args) {
  char* string = 0;
  int   size = 0;
  
  /* Have to release internal barier in slaves */
  MPICHECK( pyMPI_COMM_INPUT, MPI_Barrier(pyMPI_COMM_INPUT) );


  if (PyTuple_GET_SIZE(args) == 1 && PyTuple_GET_ITEM(args,0) == Py_None) {
    size = PYMPI_BROADCAST_EOT; /* i.e. No Data */
    MPICHECK(pyMPI_COMM_INPUT,
             MPI_Bcast(&size, 1, MPI_INT, 0, pyMPI_COMM_INPUT));
  } else {
    PYCHECK( PyArg_ParseTuple(args,"s#",&string,&size) );

    MPICHECK(pyMPI_COMM_INPUT,
             MPI_Bcast(&size, 1, MPI_INT, 0, pyMPI_COMM_INPUT));
    MPICHECK(pyMPI_COMM_INPUT,
             MPI_Bcast(string, size + 1, MPI_CHAR, 0,
                       pyMPI_COMM_INPUT));
  }
  Py_INCREF(Py_None);
  return Py_None;
 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************        mpiMethods         ************************/
/**************************************************************************/
/* This is the table of normal methods for MPI.  The comm plugin adds a   */
/* dozen more, but these are really bound methods on pyMPI_COMM_WORLD     */
/* that are stuck directly into the dictionary.                           */
/**************************************************************************/
static PyMethodDef mpiMethods[] = {
  {"bcast_input_to_slaves",bcast_input_to_slaves, METH_VARARGS,
   "From the root, broadcast a string to the slaves hung reading stdin"},

  {"finalize", finalize, METH_VARARGS,
   "Turn off MPI.  It is called at shutdown unless already finalized\n  finalize()"},
  {"finalized", finalized, METH_VARARGS,
   "True iff MPI has been finalized.  Equivalent to MPI_Finalized() for MPICH, else best guess\n  finalized()"},
  {"initialized", initialized, METH_VARARGS,
   "True iff MPI has been initialized.  Equivalent to MPI_Initialized()\n  initialized()"},
  {"wtick", wtick, METH_VARARGS,
   "Use MPI_Wtick to get tick time of high resolution timer\n  See also tick\n  wtick()"},
  {"wtime", wtime, METH_VARARGS,
   "Use MPI_Wtime to get value of high resolution timer\n  See also deltaT()\n  wtime()"},
  {"trace",mpi_trace,METH_VARARGS,"Write debug output to stdout on rank 0 only"},
  {"traceln",mpi_traceln,METH_VARARGS,"Write debug output to stdout on rank 0 only with newline"},

  /* ----------------------------------------------- */
  /* Global functions on requests                    */
  /* ----------------------------------------------- */
  {"test",(PyCFunction)pyMPI_asynchronous_test,METH_VARARGS|METH_KEYWORDS,PYMPI_ASYNCHRONOUS_TEST_DOC},
  {"test_cancelled",(PyCFunction)pyMPI_asynchronous_test_cancelled,METH_VARARGS|METH_KEYWORDS,PYMPI_ASYNCHRONOUS_TEST_CANCELLED_DOC},
  {"testall",pyMPI_asynchronous_testall,METH_VARARGS,PYMPI_ASYNCHRONOUS_TESTALL_DOC},
  {"testany",pyMPI_asynchronous_testany,METH_VARARGS,PYMPI_ASYNCHRONOUS_TESTANY_DOC},
  {"testsome",pyMPI_asynchronous_testsome,METH_VARARGS,PYMPI_ASYNCHRONOUS_TESTSOME_DOC},

  {"wait",(PyCFunction)pyMPI_asynchronous_wait,METH_VARARGS|METH_KEYWORDS,PYMPI_ASYNCHRONOUS_WAIT_DOC},
  {"waitall",pyMPI_asynchronous_waitall,METH_VARARGS,PYMPI_ASYNCHRONOUS_WAITALL_DOC},
  {"waitany",pyMPI_asynchronous_waitany,METH_VARARGS,PYMPI_ASYNCHRONOUS_WAITANY_DOC},
  {"waitsome",pyMPI_asynchronous_waitsome,METH_VARARGS,PYMPI_ASYNCHRONOUS_WAITSOME_DOC},


  {"cancel",(PyCFunction)pyMPI_asynchronous_cancel,METH_VARARGS|METH_KEYWORDS,PYMPI_ASYNCHRONOUS_CANCEL_DOC},

  {0,0}
};

/**************************************************************************/
/* GLOBAL **************         pyMPI_init        ************************/
/**************************************************************************/
/* This is the start of the Python MPI interface.  It is either invoked   */
/* directly or is sneakilly invoked when the main routine imports site.py */
/**************************************************************************/
void pyMPI_init(void) {
  int version = 0;
  int subversion = 0;
  PyObject* doc_string = 0;
  char* variety = 0;

  COVERAGE();
  RAISEIFNOMPI();

  /* ----------------------------------------------- */
  /* Version string and version check                */
  /* ----------------------------------------------- */
  MPICHECKCOMMLESS(MPI_Get_version(&version, &subversion));
  Assert(version == MPI_VERSION && subversion == MPI_SUBVERSION);

  /* ----------------------------------------------- */
  /* Set up the Python module and its dictionary     */
  /* ----------------------------------------------- */
  PYCHECK( pyMPI_module /*owned*/ = Py_InitModule("mpi",mpiMethods) );
  PYCHECK( pyMPI_dictionary /*borrowed*/ = PyModule_GetDict(pyMPI_module) );

  /* ----------------------------------------------- */
  /* Site setup (paths mostly)                       */
  /* ----------------------------------------------- */
  PYCHECK( pyMPI_site() );

  /* ----------------------------------------------- */
  /* Setup important communicators & error handlers  */
  /* ----------------------------------------------- */
  Assert(pyMPI_world_communicator != MPI_COMM_NULL);
  if (MPI_Errhandler_set(pyMPI_world_communicator, MPI_ERRORS_RETURN)
      != MPI_SUCCESS) {
    PYCHECK(PyErr_SetString
            (PyExc_SystemError, "MPI Failure -- MPI_Errhandler_set()"));
  }
  if (pyMPI_world_communicator == MPI_COMM_WORLD) {
    MPICHECK(pyMPI_world_communicator,
             MPI_Comm_dup(pyMPI_world_communicator, &pyMPI_COMM_WORLD));
  } else {
    pyMPI_COMM_WORLD = pyMPI_world_communicator;
  }
  /* Note: These are released in last_wish() above */

  /* ----------------------------------------------- */
  /* Some invariant values for this process          */
  /* ----------------------------------------------- */
  MPICHECK( pyMPI_world_communicator, MPI_Comm_rank(pyMPI_world_communicator, &pyMPI_rank) );
  MPICHECK( pyMPI_world_communicator, MPI_Comm_size(pyMPI_world_communicator, &pyMPI_size) );
  MPICHECK( pyMPI_world_communicator, MPI_Pack_size( 1, MPI_INT, pyMPI_world_communicator, &pyMPI_packedIntSize) );
  MPICHECK( pyMPI_world_communicator, MPI_Pack_size( 1, MPI_CHAR, pyMPI_world_communicator, &pyMPI_packedCharSize) );

  /* ----------------------------------------------- */
  /* If we are running in parallel, we suppress .pyc */
  /* file generation (if supported)                  */
  /* ----------------------------------------------- */
#ifdef HAVE_PY_READONLYBYTECODEFLAG
  if ( pyMPI_size > 1 ) Py_ReadOnlyBytecodeFlag++;
#endif

  /* ----------------------------------------------- */
  /* Set up doc string (must be done before PARAM)   */
  /* ----------------------------------------------- */
  PYCHECK( doc_string/*owned*/ = PyString_FromString("mpi\n\nBasic mpi calls\n\n") );

  /* ----------------------------------------------- */
  /* Important parameters                            */
  /* ----------------------------------------------- */
  PYCHECK(pyMPI_MPIError/*owned*/=PyErr_NewException("mpi.MPIError",PyExc_Exception,0));
  PARAMETER(pyMPI_MPIError,"MPIError","MPI Exception",(PyObject*),pyMPI_dictionary,&doc_string);
  PARAMETER((long)MPI_ANY_SOURCE,"ANY_SOURCE","Wildcard for use as a source rank in recv",PyInt_FromLong,pyMPI_dictionary,&doc_string);
  PARAMETER((long)MPI_ANY_TAG,"ANY_TAG","Wildcard for use as a recv tag",PyInt_FromLong,pyMPI_dictionary,&doc_string);
  PARAMETER((long)MPI_UNDEFINED,"UNDEFINED","The undefined color for MPMD",PyInt_FromLong,pyMPI_dictionary,&doc_string);
  PARAMETER((long)pyMPI_color,"COLOR","MPMD color (or UNDEFINED if SPMD)",PyInt_FromLong,pyMPI_dictionary,&doc_string);
  variety/*borrowed*/ =
#ifdef MPI_INCLUDE_STRING
    MPI_INCLUDE_STRING;
#else
#ifdef MPICH_NAME
  "MPICH";
#else
#ifdef LAM_MPI
  "LAM";
#else
  "mpi";
#endif
#endif
#endif
  PARAMETER(variety,"name","Variety of MPI used",PyString_FromString,pyMPI_dictionary,&doc_string);

  /* ----------------------------------------------- */
  /* These "plugins" add the true functionality of   */
  /* communicators and the like... The PLUGIN macro  */
  /* simply calls the init() and sets up the fini()  */
  /* routines, but this way is much easier to read   */
  /* with all the error handling hidden              */
  /* ----------------------------------------------- */
  PYMPI_PLUGIN( pyMPI_util );
  PYMPI_PLUGIN( pyMPI_signals ); /* CALL FIRST */
  PYMPI_PLUGIN( pyMPI_user ); /* CALL SECOND */
  PYMPI_PLUGIN( pyMPI_readline );
  PYMPI_PLUGIN( pyMPI_comm );
  PYMPI_PLUGIN( pyMPI_pipes );
  PYMPI_PLUGIN( pyMPI_pickle );
  PYMPI_PLUGIN( pyMPI_distutils );
  PYMPI_PLUGIN( pyMPI_reductions );
  PYMPI_PLUGIN( pyMPI_configuration );
  PYMPI_PLUGIN( pyMPI_cart );
  PYMPI_PLUGIN( pyMPI_rco ); /* Put before request */
  PYMPI_PLUGIN( pyMPI_request );
  PYMPI_PLUGIN( pyMPI_sysmods ); /* CALL LAST AS IT CAN FINALIZE MPI */

  PYCHECK( Py_AtExit(last_wish) ); /* Cleanup before end-of-program */

  /* ----------------------------------------------- */
  /* Plug the doc string into module                 */
  /* ----------------------------------------------- */
  PYCHECK( PyDict_SetItemString(pyMPI_dictionary,"__doc__",doc_string) );

  return;

 pythonError:
  /* ----------------------------------------------- */
  /* Trap error and map to ImportError               */
  /* ----------------------------------------------- */
  {
    PyObject* exception = 0;
    PyObject* error = 0;
    PyObject* traceback = 0;
    PyObject* message = 0;

    PyErr_Fetch(&exception,&error,&traceback);
    PyErr_Clear();
    
    message /*owned*/ = Py_BuildValue("sOO","Internal failure",exception,error);
    if ( PyErr_Occurred() ) {
      PyErr_SetString(PyExc_ImportError,"Internal error");
    }

    PyErr_SetObject(PyExc_ImportError,message);

  }
  return;
}

END_CPLUSPLUS
