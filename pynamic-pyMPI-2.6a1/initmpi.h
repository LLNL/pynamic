/**************************************************************************/
/* FILE   **************         initmpi.h         ************************/
/************************************************************************ **/
/* Author: Patrick Miller May  9 2002                                     */
/**************************************************************************/
/*  */
/**************************************************************************/
#ifndef INITMPI_H
#define INITMPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pyMPI_Config.h"
#include <stdio.h>
#include <stdlib.h>

#include "Python.h"
#include "mpi.h"

#define PYTHONBROADCAST_EOT -1
#define PYTHONBROADCAST_EOF -2

extern int pyMPI_packedIntSize;
extern int pyMPI_packedCharSize;

extern MPI_Comm PyMPI_COMM_WORLD;
extern MPI_Comm PyMPI_COMM_INPUT;
extern PyObject* PyMPI_pickleLoaderFunction;
extern PyObject* PyMPI_pickleDumperFunction;
extern MPI_Datatype PyMPI_pythonPickleType;
extern MPI_Datatype PyMPI_pythonFuncPickleType;
extern int (*PyMPI_finalizeMPI)(void);
extern void PyMPI_IHaveFinalizedMPI(void);
extern int PyMPI_Finalized(int*);
extern PyObject* PyMPI_dictionary;
extern PyMethodDef PyMPIMethods_Comm[];
extern PyMethodDef PyMPIMethods_Request[];
extern PyTypeObject PyMPIObject_Communicator_Type;
extern void PyMPI_UnrestrictedOutput(int);
extern PyObject* pyMPI_tupleToSpacedString(PyObject*);

/* mpicomm.c globals */
extern PyObject* bundleWithStatus(PyObject* result, MPI_Status status);
extern PyObject* PyMPIObject_Communicator_getattr(PyObject* pySelf, char*
		name);

/* EXPERIMENTAL */
#define INITMESGLEN 64

/**************************************************************************/
/* MACRO  **************          COVERAGE         ************************/
/**************************************************************************/
/* A macro used to make coverage checks of the code                       */
/**************************************************************************/
#ifdef COVERAGECHECK
#  define COVERAGEFILE(fp,rank) { char coverage[64]; sprintf(coverage,"COVERAGE.%04d"); fp = fopen(coverage,"w"); }
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
/* PyErr has been set.  You must explicitly clear  */
/* the error if you don't want it.                 */
/* ----------------------------------------------- */
#define MPICHECKCOMMLESS(a) {\
  int ierr = a; \
  if ( ierr != MPI_SUCCESS) { \
    char errorMessage[MPI_MAX_ERROR_STRING]; \
    int errorMessageLen = 0; \
    MPI_Error_string(ierr,errorMessage,&errorMessageLen); \
    PyErr_SetString(PyExc_RuntimeError,errorMessage); \
    goto pythonError; \
  } \
}
#define MPICHECK(comm,a) {\
  int ierr; \
  MPI_Errhandler handler; \
  if ( comm != PyMPI_COMM_WORLD && comm != PyMPI_COMM_INPUT ) {\
    MPI_Errhandler_get(comm,&handler); \
    MPI_Errhandler_set(comm,MPI_ERRORS_RETURN); \
    ierr = a; \
    MPI_Errhandler_set(comm,handler); \
  } else {\
    ierr = a; \
  }\
  if ( ierr != MPI_SUCCESS) { \
    char errorMessage[MPI_MAX_ERROR_STRING]; \
    int errorMessageLen = 0; \
    MPI_Error_string(ierr,errorMessage,&errorMessageLen); \
    PyErr_SetString(PyExc_RuntimeError,errorMessage); \
    goto pythonError; \
  } \
}
#define RAISEIFNOMPI() {\
  if ( !mpiReady() ) { \
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

#define PYCHECK(a) { a; if ( PyErr_Occurred() ) goto pythonError; }

/**************************************************************************/
/* TYPE   **************       pyCommunicator      ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
typedef struct {
   PyObject_HEAD
   MPI_Comm	communicator;
   double       time0; /* Time for deltaT timer */
  int		rank; /* Cached rank info */
  int		size; /* Cached size info */
} PyMPIObject_Communicator;


/* logical subclass of PyMPIOBJECT_Communicator 
 * therefore all top fields MUST!!! match
 * those of PyMPIObject_Communicator       */
typedef struct{
  PyObject_HEAD
  MPI_Comm communicator;
  double   	time0; /* Time for deltaT timer */
  int		rank; /* Cached rank info */
  int		size; /* Cached size info */

  int      ndims;
  int*     dims;
  int*     periods;
} PyMPIObject_CartComm;

typedef struct {
   PyObject_HEAD
   MPI_Status   status;
} PyMPIObject_Status;

typedef struct {
  long count;
  long extent;
  long datatype;
} PyMPIDescription;

typedef struct {
  PyObject_HEAD
  int              iAmASendObject;    /* associated w/ a send operation?*/
  PyMPIDescription description;       /* what are we sending/recieveing?*/
  MPI_Request      descriptionRequest;/* for description isend/irecv    */
  char*            buffer;            /* allocated buffer for isend()/irecv() 
                                         must NOT be touched until operations
                                         have been completed!!          */
  MPI_Request      bufferRequest;     /* for data                       */
  MPI_Status	   status;	      /* Use for status (after receive) */
  PyObject*        postedMessage;     /* data as a python object        */
  MPI_Comm         communicator;      /* which communicator are we on?  */

  char firstBufMsg[INITMESGLEN]; /* Stores the result of the first part of the
                                   send in case the send was broken in two */
  int pkllen;                   /* Stores the pickeled length of msg being
                                   received*/
} PyMPIObject_Request;

typedef struct {
   PyObject_HEAD
   MPI_Group    group;
} PyMPIObject_Group;

/* ----------------------------------------------- */
/* Exports                                         */
/* ----------------------------------------------- */
extern PyObject* PyMPI_Status(MPI_Status);
extern PyMPIObject_Request* PyMPI_Request(MPI_Comm);

extern int PyMPI_File_Check(PyObject*);
extern PyObject* PyMPI_File(int,PyObject*,MPI_Comm);
extern PyObject* PyMPI_Comm(MPI_Comm);

extern void PyAlternateMPIStartup(int (**start)(int *, char ***),
                                  int (**finish)(void));

extern PyObject* PyMPI_WaitForPostedMessage(PyMPIObject_Request*);

/**************************************************************************/
/* TYPE   **************       mpiPseudoFile       ************************/
/**************************************************************************/
/* This defines a file-like object that reimplements a file for the input */
/* objects that communicate the stdin around the processors               */
/**************************************************************************/
typedef struct {
   PyObject_HEAD
   short closed;
   short softspace;
   short master;
   short EOT;
   char* mode;
   char* buffer;
   char* current;
   char* bufferEnd;
   PyObject* proxyObject;
   MPI_Comm comm;
} PyMPIObject_File;

/**************************************************************************/
/* TYPE   **************       pyMPI_File_Handle   ************************/
/**************************************************************************/
/* This defines a file-like object that lets processes read from and write*/
/* files.                                                                 */
/**************************************************************************/
typedef struct {
   PyObject_HEAD
   FILE* stream;
   char* mode;
   char* fileName;
   MPI_Comm comm;
} PyMPI_File_Handle;

/**************************************************************************/
/* LOCAL  **************          mpiReady         ************************/
/**************************************************************************/
/* In many parts of the code, we need to know that MPI is ready to accept */
/* requests.  This implies that it is initialized but not finalized.      */
/**************************************************************************/
static int mpiReady(void) {
   int ready = 0;
   int finalized = 0;
   int ierr;
   int status;

   ierr = MPI_Initialized(&ready);
   Assert(ierr == 0);

   ierr = PyMPI_Finalized(&finalized);
   Assert(ierr == 0);

   status = ready && !finalized;
   return status;
}

/* This global lookup table stores the values of MPI reduce operations*/
enum eMyReduceOps { eMaxOp, eMinOp, eSumOp, eProdOp, eLandOp, eBandOp, eLorOp, 
    eBorOp, eLxorOp,eBxorOp, eMaxlocOp, eMinlocOp, eNumReduceOps };
extern MPI_Op reduceOpLookup[eNumReduceOps];
extern PyObject* reduceOpFunction[eNumReduceOps];

#ifdef __cplusplus
}
#endif

#endif
