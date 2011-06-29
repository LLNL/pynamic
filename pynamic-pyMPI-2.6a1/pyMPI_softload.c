/**************************************************************************/
/* FILE   **************      pyMPI_softload.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller February 25 2004                                */
/* Copyright (C) 2004 Patrick J. Miller                                   */
/**************************************************************************/
/* Some MPI combos will let you start as Python and dynamically load MPI  */
/* Your mileage may vary                                                  */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

/**************************************************************************/
/* LOCAL  **************            argv           ************************/
/**************************************************************************/
/* This holds the reconstructed argv set. We free it when we finalize MPI  */
/**************************************************************************/
static int    argc = 0;
static char** argv = 0;
static char** before_argv = 0;

/**************************************************************************/
/* LOCAL  **************          shutdown         ************************/
/**************************************************************************/
/* This is cleanup work that has to be done as Python shuts down          */
/**************************************************************************/
static void shutdown(void) {
  int i;
  int MPI_is_running = 0;
  int MPI_is_finalized = 0;

  MPI_Initialized(&MPI_is_running);
  if ( MPI_is_running ) {
#ifdef HAVE_MPI_FINALIZED
    MPI_Finalized(&MPI_is_finalized);
#endif
    if ( ! MPI_is_finalized ) MPI_Finalize();
  }
  if ( argv ) free(argv);
  if ( before_argv ) {
    for(i=0;i<argc;++i) {
      if (before_argv[i]) free(before_argv[i]);
    }
    free(before_argv);
  }
}

/**************************************************************************/
/* GLOBAL **************          initmpi          ************************/
/**************************************************************************/
/* While the module is actually inited by pyMPI_init(), this is the setup */
/* needed to get MPI on line                                              */
/**************************************************************************/
void initmpi(void) {
  PyObject* pyArgv = 0;
  PyObject* pyElement = 0;
  char*  element = 0;
  int i;
  int MPI_is_running = 0;

  Py_AtExit(shutdown);
  if ( PyErr_Occurred() ) {
    PyErr_SetString(PyExc_SystemError,"cannot register shutdown to close MPI");
    return;
  }

  /* The first thing we must do is to reconstruct argv */
  pyArgv = PySys_GetObject("argv");
  if ( PyErr_Occurred() ) {
    PyErr_SetString(PyExc_SystemError,"sys.argv doesn't exist to initialize mpi");
    return;
  }

  argc = PyList_Size(pyArgv);  
  if ( PyErr_Occurred() || argc < 0 ) {
    PyErr_SetString(PyExc_SystemError,"sys.argv isn't a list");
    return;
  }

  argv = (char**)(malloc((argc+1)*sizeof(char*)));
  if ( !argv ) {
    PyErr_SetString(PyExc_MemoryError,"out of memory");
    return;
  }
  before_argv = malloc((argc+1)*sizeof(char*));
  if ( !before_argv ) {
    PyErr_SetString(PyExc_MemoryError,"out of memory");
    return;
  }
  for(i=0;i<argc;++i) {
    pyElement = PyList_GetItem(pyArgv,i);
    if ( PyErr_Occurred() ) return;
    
    element = PyString_AsString(pyElement);
    if ( PyErr_Occurred() ) return;

    before_argv[i] = (char*)(malloc(strlen(element)+1));
    if ( !before_argv[i] ) {
      PyErr_SetString(PyExc_MemoryError,"out of memory");
      return;
    }
    strcpy(before_argv[i],element);

    argv[i] = before_argv[i]; /* Borrows other char* */
  }
  argv[argc] = 0;

  /* May already be initialized... */
  MPI_Initialized(&MPI_is_running);
  if ( ! MPI_is_running ) {
    MPI_Init(&argc,&argv);
  }
  PySys_SetArgv(argc,argv);

  /* Turn on pyMPI module */
  pyMPI_world_communicator = MPI_COMM_WORLD;
  pyMPI_owns_MPI = 0;
  pyMPI_init();
  pyMPI_user_startup();
}
