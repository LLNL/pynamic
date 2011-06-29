/**************************************************************************/
/* FILE   **************       pyMPI_Types.h       ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/**************************************************************************/
/* The base types for MPI Python                                          */
/**************************************************************************/

#ifndef PYMPI_TYPES_H
#define PYMPI_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/* TYPE   **************         Py_ssize_t        ************************/
/**************************************************************************/
/* Pre-Python2.5 used int for "sizish" things...  We need to live in both */
/* worlds.                                                                */
/**************************************************************************/
#ifndef HAVE_PY_SSIZE_T
#ifndef PYMPI_SKIP_SSIZE_DEF
  typedef int Py_ssize_t;
#endif
#endif

/**************************************************************************/
/* TYPE   **************    pyMPI_void_function    ************************/
/**************************************************************************/
/* A type for void functions of no arguments                              */
/**************************************************************************/
typedef void (*pyMPI_void_function)(void);

#ifndef PYMPI_EAGER_LIMIT
#define PYMPI_EAGER_LIMIT (512)
#endif

/**************************************************************************/
/* TYPE **************   PyMPI_MessageProtocol_t  *************************/
/**************************************************************************/
/* We support a few message protocols beyond the obvious pickling         */
/**************************************************************************/
typedef enum {
  PyMPI_AS_LONG,
  PyMPI_AS_DOUBLE,
  PyMPI_AS_STRING,
  PyMPI_AS_NUMPY,
  PyMPI_AS_NUMARRAY,
  PyMPI_AS_NUMERIC,
  PyMPI_AS_PICKLE
} PyMPI_MessageProtocol_t;

/**************************************************************************/
/* TYPE   **************       PyMPI_Protocol       ***********************/
/**************************************************************************/
/* Protocol overhead for sending packed message                           */
/**************************************************************************/
typedef struct {
  double double_payload;
  long long_payload;
  int  bytes_in_second_message;
  int message_type; /* really PyMPI_MessageProtocol_t */
  short free_buffer;
  short bytes_in_prefix;
} PyMPI_Protocol;

#define PYMPI_PREFIXCHARS (PYMPI_EAGER_LIMIT-sizeof(PyMPI_Protocol))

/**************************************************************************/
/* TYPE ****************       PyMPI_Message       ***********************/
/**************************************************************************/
/* Small message, hopefully inside eager limit                            */
/**************************************************************************/
typedef struct {
  PyMPI_Protocol header;
  char prefix[PYMPI_PREFIXCHARS];
} PyMPI_Message; 

/**************************************************************************/
/* MACRO  **************      PYMPI_COMM_BODY      ************************/
/**************************************************************************/
/* Basic communicator body.  We hold a handle to the MPI comm, a flag to  */
/* specify if we should MPI_Comm_free() on destruction, a time for the    */
/* stopwatch, and we cache rank and size since we use them a lot.         */
/**************************************************************************/
#define PYMPI_COMM_BODY \
  PyObject_HEAD \
  MPI_Comm      communicator; \
  int           persistent;     /* If false, call MPI_free on delete */ \
  double        time0;          /* Time for deltaT timer */ \
  int           rank;           /* Cached rank info */ \
  int           size;           /* Cached size info */


/**************************************************************************/
/* TYPE   **************         PyMPI_Comm        ************************/
/**************************************************************************/
/* Here we define the real type.  We use the PYMPI_COMM_BODY macro so we  */
/* can reliably define derived types (e.g. the cartesian communicator)    */
/**************************************************************************/
typedef struct {
  PYMPI_COMM_BODY
} PyMPI_Comm;

/**************************************************************************/
/* TYPE   **************      PyMPI_Cart_Comm      ************************/
/**************************************************************************/
/* A cartesian communicator simply extends the communicator object        */
/**************************************************************************/
typedef struct {
  PYMPI_COMM_BODY

  int        ndims;
  PyObject*  dims;
  PyObject*  periods;
} PyMPI_Cart_Comm;

/**************************************************************************/
/* TYPE   **************        PyMPI_Remote       ************************/
/**************************************************************************/
/* An object with remote procedure invocation                             */
/**************************************************************************/
typedef struct {
  PyObject_HEAD
  PyObject* comm;               /* Communicator connecting objects */
  PyObject* tag;                /* Tag on which to isend() message */
  PyObject* hash;               /* Unique identifier */
} PyMPI_Remote;

/**************************************************************************/
/* TYPE   **************    PyMPI_RemotePartial    ************************/
/**************************************************************************/
/* An object with remote procedure invocation                             */
/**************************************************************************/
typedef struct {
  PyObject_HEAD
  PyObject* actor;
  PyObject* destinations;
  PyObject* method;
} PyMPI_RemotePartial;

/**************************************************************************/
/* TYPE   **************        PyMPI_Status       ************************/
/**************************************************************************/
/* A Python binding to the MPI status type                                */
/**************************************************************************/
typedef struct {
   PyObject_HEAD
   MPI_Status   status;
} PyMPI_Status;

/**************************************************************************/
/* TYPE   **************     PyMPI_Description     ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
typedef struct {
  long count;
  long extent;
  long datatype;
} PyMPI_Description;

/**************************************************************************/
/* TYPE   **************       PyMPI_Request       ************************/
/**************************************************************************/
/* The request type holds data for the async */
/**************************************************************************/
typedef struct PyMPI_Request {
  PyObject_HEAD
  int              iAmASendObject;    /* associated w/ a send operation?*/
  PyMPI_Description description;      /* what are we sending/receiving?*/
  MPI_Request      descriptionRequest;/* for description isend/irecv    */
  char*            buffer;            /* allocated buffer for isend()/irecv() 
                                         must NOT be touched until operations
                                         have been completed!!          */
  MPI_Request      bufferRequest;     /* for data                       */
  MPI_Status       status;            /* Use for status (after receive) */
  PyObject*        postedMessage;     /* data as a python object        */
  MPI_Comm         communicator;      /* which communicator are we on?  */

  PyMPI_Message firstBufMsg; /* Stores the result of the first part of the
                                send in case the send was broken in two */

  struct PyMPI_Request* abandoned_link; /* Chain of abandoned send requests */
} PyMPI_Request;

/**************************************************************************/
/* TYPE   **************        PyMPI_Group        ************************/
/**************************************************************************/
/* Interface to the MPI_Group type                                        */
/**************************************************************************/
typedef struct {
   PyObject_HEAD
   MPI_Group    group;
} PyMPI_Group;

/**************************************************************************/
/* TYPE   **************     PyMPI_Shared_File     ************************/
/**************************************************************************/
/* This defines a file-like object that reimplements a file for the input */
/* objects that share a read or write file handle on one process          */
/**************************************************************************/
typedef struct {
   PyObject_HEAD
   int master;
   int rank;
  int style;
  PyMPI_Comm* communicator;
  PyObject* file;
  PyObject* filename;
  PyObject* filemode;
  PyObject* local_buffer;
} PyMPI_Shared_File;


/**************************************************************************/
/* TYPE   **************      PyMPI_Parameters     ************************/
/**************************************************************************/
/* I prefer enumerated constants to #define parameters                    */
/**************************************************************************/
typedef enum {
  PyMPI_STRING_BUFFER,
  PyMPI_FILE_BUFFER,
  PyMPI_NULL_BUFFER,
  PyMPI_NUMPARAMS
} PyMPI_Parameters;


#ifdef __cplusplus
}
#endif

#endif
