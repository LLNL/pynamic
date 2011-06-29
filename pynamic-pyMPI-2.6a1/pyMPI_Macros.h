/**************************************************************************/
/* FILE   **************       pyMPI_macros.h      ************************/
/************************************************************************ **/
/* Author: Patrick Miller May 15 2002                                     */
/* Copyright (C) 2002 Patrick J. Miller                                   */
/**************************************************************************/
/*  */
/**************************************************************************/
#ifndef PYMPI_MACROS_H
#define PYMPI_MACROS_H


#ifdef __cplusplus
#define START_CPLUSPLUS extern "C" {
#define END_CPLUSPLUS }
#else
#define START_CPLUSPLUS /*nothing*/
#define END_CPLUSPLUS /*nothing*/
#endif

/**************************************************************************/
/* MACRO  **************    pyMPI_PyErr_Occurred   ************************/
/**************************************************************************/
/* Have to make safe checks for teardown                                  */
/**************************************************************************/
#define pyMPI_PyErr_Occurred() (_PyThreadState_Current && PyThreadState_GET()->curexc_type)

/**************************************************************************/
/* MACRO  **************    PYMPI_BROADCAST_EOT    ************************/
/**************************************************************************/
/* Used in pyMPI readline                                                 */
/**************************************************************************/
#define PYMPI_BROADCAST_EOT (-1)

/**************************************************************************/
/* GLOBAL **************    PYMPI_BROADCAST_EOF    ************************/
/**************************************************************************/
/* Used in pyMPI readline                                                 */
/**************************************************************************/
#define PYMPI_BROADCAST_EOF (-2)

/**************************************************************************/
/* MACRO ***************       GLOBALVARIABLE      ************************/
/************************************************************************ **/
/*  */
/**************************************************************************/
#define GLOBALVARIABLE(signature,name,value) extern DL_IMPORT(signature) name; signature name = value;

/**************************************************************************/
/* MACRO  **************            PUTS           ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
#define PUTS(x) printf("%s:%d: [%d]\t%s\n",__FILE__,__LINE__,pyMPI_rank,(x))
#define PRINTF printf("%s:%d: [%d]\t",__FILE__,__LINE__,pyMPI_rank); printf

/**************************************************************************/
/* MACRO  **************         PARAMETER         ************************/
/**************************************************************************/
/* The PARAMETER macro adds a value to the dictionary and adds a simple   */
/* doc line to the doc string.  It is assumed that convert will return a  */
/* owned Python object* created from value.                               */
/**************************************************************************/
#define PARAMETER(value,name,doc,convert,dictionary,docStringP) { \
  PyObject* t = 0; \
  Assert(dictionary); \
  Assert(docStringP && *docStringP); \
  PYCHECK( t /* owned */ = convert(value) ); \
  /* printf("Insert %s, ",name); PyObject_Print(t,stdout,0); printf(" rc=%d\n",t->ob_refcnt); */\
  PYCHECK( PyDict_SetItemString(dictionary,name,t) ); \
  PYCHECK( t /* owned */ = PyString_FromString(name) ); \
  PYCHECK( PyString_ConcatAndDel(docStringP,t) ); \
  PYCHECK( t /* owned */ = PyString_FromString(": ") ); \
  PYCHECK( PyString_ConcatAndDel(docStringP,t) ); \
  PYCHECK( t /* owned */ = PyString_FromString(doc) ); \
  PYCHECK( PyString_ConcatAndDel(docStringP,t) ); \
  PYCHECK( t /* owned */ = PyString_FromString("\n\n") ); \
  PYCHECK( PyString_ConcatAndDel(docStringP,t) ); \
}

/**************************************************************************/
/* MACRO  **************          COVERAGE         ************************/
/**************************************************************************/
/* A macro used to make coverage checks of the code                       */
/**************************************************************************/
#ifdef COVERAGECHECK
#  define COVERAGEFILE(fp,rank) { char coverage[64]; sprintf(coverage,"COVERAGE.%06d",rank); fp = fopen(coverage,"w"); }
#  define COVERAGE() { fprintf(stderr,"%s: %d\n",__FILE__,__LINE__); }
#else
#  define COVERAGEFILE(fp,rank) { fp = (FILE*)0; }
#  define COVERAGE() {}
#endif

/**************************************************************************/
/* LOCAL  **************           Assert          ************************/
/**************************************************************************/
/* Define assertion that force quits on failure                           */
/**************************************************************************/
#ifdef NOASSERT
#  define Assert(a) {}
#else
#  define Assert(a) if (!(a)) \
   { fprintf(stderr,"%s: %d Assertion %s failed at line %d\n",\
          __FILE__,__LINE__,#a,__LINE__); \
          fflush(stderr); \
          exit(1); \
   }
#endif

/* ----------------------------------------------- */
/* Several things I do a lot of, like call MPI and */
/* return a Python exception.  Define as macros    */
/* There is an invariant for these MACROS use: ie. */
/* PyErr will be set.  You must explicitly clear   */
/* the error if you don't want it.                 */
/* ----------------------------------------------- */
#define MPICHECKCOMMLESS(a) {\
  int ierr = a; \
  if ( ierr != MPI_SUCCESS) { \
    char errorMessage[MPI_MAX_ERROR_STRING+1024]; \
    int errorMessageLen = 0; \
    MPI_Error_string(ierr,errorMessage,&errorMessageLen); \
    sprintf(errorMessage+errorMessageLen," (%s:%d)",__FILE__,__LINE__); \
    PyErr_SetString(pyMPI_MPIError?pyMPI_MPIError:PyExc_RuntimeError,errorMessage); \
    goto pythonError; \
  } \
}
#define MPICHECK(comm,a) {\
  int ierr; \
  MPI_Errhandler handler; \
  if ( comm != pyMPI_COMM_WORLD && comm != pyMPI_COMM_INPUT && comm != MPI_COMM_NULL ) {\
    MPI_Errhandler_get(comm,&handler); \
    MPI_Errhandler_set(comm,MPI_ERRORS_RETURN); \
    ierr = a; \
    MPI_Errhandler_set(comm,handler); \
  } else {\
    ierr = a; \
  }\
  if ( ierr != MPI_SUCCESS) { \
    char errorMessage[MPI_MAX_ERROR_STRING+1024]; \
    int errorMessageLen = 0; \
    MPI_Error_string(ierr,errorMessage,&errorMessageLen); \
    sprintf(errorMessage+errorMessageLen," (%s:%d)",__FILE__,__LINE__); \
    PyErr_SetString(pyMPI_MPIError?pyMPI_MPIError:PyExc_RuntimeError,errorMessage); \
    goto pythonError; \
  } \
}
#define VALIDCOMMUNICATOR(self) {\
  if ( self->communicator == MPI_COMM_NULL ) { \
    PYCHECK( PyErr_SetString(PyExc_RuntimeError,"invalid use of NULL commuicator") ); \
  }\
}

#define RAISEIFNOMPI() {\
  if ( !pyMPI_util_ready() ) { \
    PyErr_SetString(PyExc_RuntimeError,"MPI not ready or already finalized");\
    goto pythonError; \
  }\
} 
#define NOARGUMENTS() {\
   if ( args && PyObject_Length(args) > 0 ) {\
      PyErr_SetString(PyExc_TypeError,"no arguments expected");\
      goto pythonError; \
   }\
}

#define DEPRECATED(msg) \
  { static int _firsttime = 1; \
    if ( _firsttime ) { \
      _firsttime = 0; \
      if ( PyErr_Warn(PyExc_DeprecationWarning,msg) ) { \
        PYCHECK(PyErr_SetString(PyExc_DeprecationWarning,msg)); \
      } \
    } \
  }

#define PYCHECK(a) { a; if ( pyMPI_PyErr_Occurred() ) {  goto pythonError;} }
#define PYCHECK2(a,b,c) { a; if ( pyMPI_PyErr_Occurred() ) { PyErr_SetString(b,c); goto pythonError; } }
#define PYOUT(x) { printf("%s:%d: [%d]TODO: %s->",__FILE__,__LINE__,pyMPI_rank,#x); PyObject_Print(x,stdout,0); putchar('\n'); }
#define DEBUG(x) { printf("%s:%d: [%d]TODO: ",__FILE__,__LINE__,pyMPI_rank); printf x; }
#define PYERR() { PYOUT(pyMPI_PyErr_Occurred()); }

#endif
