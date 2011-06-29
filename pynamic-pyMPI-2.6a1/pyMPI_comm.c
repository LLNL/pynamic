/**************************************************************************/
/* FILE   **************        pyMPI_comm.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/**************************************************************************/
/* Top level communicator object description.  It is so large and complex */
/* that it further broken into a series of smaller files (pyMPI_comm_xxx) */
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/* >>> comm = WORLD.comm_dup()                                            */
/* >>> handle = int(WORLD)                                                */
/* >>> def serial_work(n):                                                */
/* ...    return                                                          */
/* >>> def deep_thought():                                               */
/* ...    return                                                          */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************      pyMPI_COMM_NULL      ************************/
/**************************************************************************/
/* Python wrapper on NULL communicator.                                   */
/**************************************************************************/
PyObject* pyMPI_COMM_NULL = 0;

/**************************************************************************/
/* GLOBAL **************        pyMPI_world        ************************/
/**************************************************************************/
/* Python wrapper on MPI_COMM_WORLD                                       */
/**************************************************************************/
PyObject* pyMPI_world = 0;


/**************************************************************************/
/* METHOD **************         comm_init         ************************/
/**************************************************************************/
/* Initialization of communicator and derived instances                   */
/**************************************************************************/
/* Create communicator object from communicator or handle                 */
/*                                                                        */
/* communicator(communicator=COMM_NULL,  # Communicator to wrap           */
/*              persistent=1)            # If false, release MPI comm     */
/*  --> <communicator instance>                                           */
/*                                                                        */
/* Build instance of a communicator interface.  The persistent flag (by   */
/* default on) means that Python WILL NOT release the MPI communicator on */
/* delete.                                                                */
/*                                                                        */
/* >>> null = communicator()      # returns a (not the) NULL communicator */
/* >>> c = communicator(WORLD)    # a new interface to WORLD communicator */
/* >>> my_world = communicator(handle,0) # Python version of handle       */
/*                                       # MPI_Comm_free() will be called */
/*                                                                        */
/**************************************************************************/
static int comm_init(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  long comm_as_long = (long)MPI_COMM_NULL;
  int persistent = 1;
  MPI_Comm communicator;
  static char* kwlist[] = {"communicator","persist",0};

  COVERAGE();
  Assert(self);
  Assert(sizeof(long) >= sizeof(MPI_Comm));
  Assert( PyObject_IsInstance(pySelf, (PyObject*)(&pyMPI_comm_type)) );

  /* ----------------------------------------------- */
  /* Grab the constructor arguements                  */
  /* ----------------------------------------------- */
  PYCHECK(PyArg_ParseTupleAndKeywords(args,kw,"|li", kwlist,&comm_as_long,&persistent));
  communicator = (MPI_Comm)comm_as_long;

  /* ----------------------------------------------- */
  /* Fill in the important values                    */
  /* ----------------------------------------------- */
  self->communicator = communicator; /* MPI Handle */
  self->persistent = persistent; /* False to call MPI_Comm_Free() on delete */
  self->time0 = MPI_Wtime();    /* Start timer for stopwatch() */
  self->rank = 0;               /* set below */
  self->size = 0;               /* set below */

  /* ----------------------------------------------- */
  /* Make sure it looks seems to be a real comm.  We */
  /* have to use MPI_COMM_WORLD as the "test" comm   */
  /* so that we can guarantee err handling later     */
  /* ----------------------------------------------- */
  if ( communicator != MPI_COMM_NULL ) {
    MPICHECK(MPI_COMM_WORLD, MPI_Comm_rank(communicator,&self->rank) );
    MPICHECK(MPI_COMM_WORLD, MPI_Comm_size(communicator,&self->size) );
  }
  return 0;

 pythonError:
  return -1;
}

/**************************************************************************/
/* METHOD **************         comm_free         ************************/
/**************************************************************************/
/* Implements MPI_Comm_free                                               */
/**************************************************************************/
/* Release the MPI communicator                                           */
/*                                                                        */
/* release() --> None                                                     */
/*                                                                        */
/* This tells MPI that the communicator is no longer in use.  The WORLD   */
/* and NULL communicators are not freed and the method returns silently.  */
/*                                                                        */
/* >>> comm.comm_free()                                                   */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_free(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  COVERAGE();

  Assert(pySelf);
  Assert( PyObject_IsInstance(pySelf, (PyObject*)(&pyMPI_comm_type)) );

  /* ----------------------------------------------- */
  /* Never release the WORLD and NULL communicators  */
  /* The pyMPI variants are released on shutdown */
  /* ----------------------------------------------- */
  if ( self->communicator != MPI_COMM_WORLD &&
       self->communicator != MPI_COMM_NULL &&
       self->communicator != pyMPI_COMM_WORLD &&
       self->communicator != pyMPI_COMM_INPUT
       ) {
    MPICHECK( MPI_COMM_WORLD, MPI_Comm_free(&self->communicator) );
    self->communicator = MPI_COMM_NULL;
    self->rank = 0;
    self->size = 0;
  }

  Py_INCREF(Py_None);
  return Py_None;

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************        comm_dealloc       ************************/
/**************************************************************************/
/* Returns Python allocated memory.  Note: This frees the memory, but it  */
/* only releases the communicator if the persistent flag was cleared by   */
/* the user                                                               */
/**************************************************************************/
static void comm_dealloc(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  COVERAGE();

  Assert(pySelf);
  Assert( PyObject_IsInstance(pySelf, (PyObject*)(&pyMPI_comm_type)) );

  /* ----------------------------------------------- */
  /* Release the communicator and the storage        */
  /* ----------------------------------------------- */
  if ( ! self->persistent ) comm_free(pySelf);
  PYCHECK( PyObject_FREE(pySelf) );

 pythonError:
  return;
}


/**************************************************************************/
/* METHOD *****************    comm_stopwatch    **************************/
/**************************************************************************/
/* Time delta                                                             */
/**************************************************************************/
/* Returns time in seconds since last call (or communicator creation)     */
/*                                                                        */
/* stopwatch() --> float    # Double precision time                       */
/*                                                                        */
/* >>> mpi.stopwatch() # Start the timer                                  */
/* >>> deep_thought()                                                     */
/* >>> print 'Deep thought took',mpi.stopwatch(),'seconds'                */
/*                                                                        */
/* A separate timer is maintained for every Python communicator.          */
/* >>> comm.stopwatch()                                                   */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_stopwatch(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  double theTime;
  double delta;

  Assert(self);

  COVERAGE();

  theTime = MPI_Wtime();
  delta = theTime - self->time0;
  self->time0 = theTime;
  return PyFloat_FromDouble(delta);
}

/**************************************************************************/
/* METHOD *****************       comm_tick      **************************/
/**************************************************************************/
/* Timer tick size (resolution)                                           */
/**************************************************************************/
/* Return the ticksize of the high-resolution timer (Python Float)        */
/*                                                                        */
/* tick() --> float                                                       */
/*                                                                        */
/* The smallest tick for the MPI high resolution timer.  See time() to    */
/* get time in seconds since the epoch.                                   */
/*                                                                        */
/* >>> tick = mpi.tick()                                                  */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_tick(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);
  COVERAGE();

  return PyFloat_FromDouble(MPI_Wtick());
}

/**************************************************************************/
/* METHOD **************         comm_time         ************************/
/**************************************************************************/
/* Timer value                                                            */
/**************************************************************************/
/* Returns the time in seconds (as Python Float) since epoch.             */
/*                                                                        */
/* time() --> float                                                       */
/*                                                                        */
/* Uses the MPI high resolution timer, not system clock.  See tick()      */
/* for clock resolution                                                   */
/*                                                                        */
/* >>> mpi.time()                                                         */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_time(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);
  COVERAGE();

  return PyFloat_FromDouble(MPI_Wtime());
}


/**************************************************************************/
/* LOCAL  **************       comm_methods        ************************/
/**************************************************************************/
/* Method table describing communicator methods exposed to Python.  Some  */
/* of the methods are defined in other files.  Use the name of the func   */
/* to figure out the file.  For instance, pyMPI_message_send is in the    */
/* file "pyMPI_comm_message."                                             */
/**************************************************************************/
static PyMethodDef comm_methods[] = {

  /* ===================   Point to Point ================================== */
  {"send",(PyCFunction)pyMPI_message_send,METH_VARARGS|METH_KEYWORDS,PYMPI_MESSAGE_SEND_DOC},
  {"recv",(PyCFunction)pyMPI_message_recv,METH_VARARGS|METH_KEYWORDS,PYMPI_MESSAGE_RECV_DOC},
  {"sendrecv",(PyCFunction)pyMPI_message_sendrecv,METH_VARARGS|METH_KEYWORDS,PYMPI_MESSAGE_SENDRECV_DOC},

  /* ===================   Asynchronous  Point to Point ==================== */
  {"isend",(PyCFunction)pyMPI_asynchronous_isend,METH_VARARGS|METH_KEYWORDS,PYMPI_ASYNCHRONOUS_ISEND_DOC},
  {"irecv",(PyCFunction)pyMPI_asynchronous_irecv,METH_VARARGS|METH_KEYWORDS,PYMPI_ASYNCHRONOUS_IRECV_DOC},

  /* ===================   Collective Communication========================= */
  {"barrier",(PyCFunction)pyMPI_collective_barrier,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_BARRIER_DOC},
  {"bcast",(PyCFunction)pyMPI_collective_bcast,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_BCAST_DOC},
  {"scatter",(PyCFunction)pyMPI_collective_scatter,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_SCATTER_DOC},
  {"reduce",(PyCFunction)pyMPI_collective_reduce,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_REDUCE_DOC},
  {"allreduce",(PyCFunction)pyMPI_collective_allreduce,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_ALLREDUCE_DOC},
  {"scan",(PyCFunction)pyMPI_collective_scan,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_SCAN_DOC},
  {"gather",(PyCFunction)pyMPI_collective_gather,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_GATHER_DOC},
  {"allgather",(PyCFunction)pyMPI_collective_allgather,METH_VARARGS|METH_KEYWORDS,PYMPI_COLLECTIVE_ALLGATHER_DOC},

  /* ==========================   I/O  ===================================== */
  {"synchronizeQueuedOutput",(PyCFunction)pyMPI_io_synchronizeQueuedOutput,METH_VARARGS|METH_KEYWORDS,PYMPI_IO_SYNCHRONIZEQUEUEDOUTPUT_DOC},
  {"synchronizedWrite",(PyCFunction)pyMPI_io_synchronizedWrite,METH_VARARGS|METH_KEYWORDS,PYMPI_IO_SYNCHRONIZEDWRITE_DOC},
  {"synchronizedWriteln",(PyCFunction)pyMPI_io_synchronizedWriteln,METH_VARARGS|METH_KEYWORDS,PYMPI_IO_SYNCHRONIZEDWRITELN_DOC},

  /* ==================   Non Communication ================================ */
  {"comm_create",(PyCFunction)pyMPI_misc_create,METH_VARARGS|METH_KEYWORDS,PYMPI_MISC_CREATE_DOC},
  {"comm_dup",(PyCFunction)pyMPI_misc_dup,METH_NOARGS,PYMPI_MISC_DUP_DOC},
  {"cart_create",(PyCFunction)pyMPI_misc_cart_create,METH_VARARGS|METH_KEYWORDS,PYMPI_MISC_CART_CREATE_DOC},
  {"comm_rank",(PyCFunction)pyMPI_misc_rank,METH_NOARGS,PYMPI_MISC_RANK_DOC},
  {"comm_size",(PyCFunction)pyMPI_misc_size,METH_NOARGS,PYMPI_MISC_SIZE_DOC},
  {"comm_free",(PyCFunction)pyMPI_misc_free,METH_NOARGS,PYMPI_MISC_FREE_DOC},
  {"abort",(PyCFunction)pyMPI_misc_abort,METH_VARARGS|METH_KEYWORDS,PYMPI_MISC_ABORT_DOC},
  {"time",(PyCFunction)comm_time,METH_NOARGS,PYMPI_COMM_TIME_DOC},
  {"tick",(PyCFunction)comm_tick,METH_NOARGS,PYMPI_COMM_TICK_DOC},
  {"stopwatch",(PyCFunction)comm_stopwatch,METH_NOARGS,PYMPI_COMM_STOPWATCH_DOC},

  /* ==========================  map  ====================================== */
  {"map",(PyCFunction)pyMPI_map_map,METH_VARARGS|METH_KEYWORDS,PYMPI_MAP_MAP_DOC},
  {"mapserver",(PyCFunction)pyMPI_map_mapserver,METH_VARARGS|METH_KEYWORDS,PYMPI_MAP_MAPSERVER_DOC},
  {"mapstats",(PyCFunction)pyMPI_map_mapstats,METH_VARARGS|METH_KEYWORDS,PYMPI_MAP_MAPSTATS_DOC},
  {0,0}
};


/**************************************************************************/
/* LOCAL  **************        comm_nonzero       ************************/
/**************************************************************************/
/* This computes the "truth" value of a communicator when used in the     */
/* boolean sense (e.g. if comm: ... )                                     */
/**************************************************************************/
static int comm_nonzero(PyObject* pySelf) {
   PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

   Assert(self);
   COVERAGE();

   return self->communicator != MPI_COMM_NULL;
}

/**************************************************************************/
/* LOCAL  **************          comm_int         ************************/
/**************************************************************************/
/* Converting to an integer will return the MPI handle.  This allows you  */
/* to pass a communicator into a C or FORTRAN routine that wants a comm   */
/**************************************************************************/
static PyObject* comm_int(PyObject* pySelf) {
   PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

   Assert(self);
   COVERAGE();

   return PyInt_FromLong((long)self->communicator);
}

/**************************************************************************/
/* LOCAL  **************        comm_length        ************************/
/**************************************************************************/
/* The length of a communicator (because of sequence like implementation) */
/* is the number of processes associated with it (will be zero for the    */
/* COMM_NULL communicator).                                               */
/**************************************************************************/
static Py_ssize_t comm_length(PyObject* pySelf) {
   PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

   Assert(self);
   COVERAGE();

   return self->size;
}

/**************************************************************************/
/* LOCAL  **************         comm_item         ************************/
/**************************************************************************/
/* A communicator looks like a sequence of "rank" numbers [0,1,...].      */
/*                                                                        */
/* This allows code such as:                                              */
/* >>> for rank in comm:                                                  */
/* ...     print rank                                                     */
/* ...                                                                    */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_item(PyObject* pySelf, Py_ssize_t position) {
   PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
   PyObject* result = 0;

   Assert(self);

   if ( position < 0 || position >= self->size ) {
     PYCHECK( PyErr_Format(PyExc_IndexError,"list index %d out of range",
                           (int)position) );
   }

   PYCHECK( result = PyInt_FromLong((long)position) );
   return result;

 pythonError:
   Assert(PyErr_Occurred());
   return 0;
}

/**************************************************************************/
/* LOCAL  **************         comm_slice        ************************/
/**************************************************************************/
/* Slices from a communicator return the list of "rank" numbers in the    */
/* range. Suppose we have a communicator of size 7...                     */
/*                                                                        */
/* >>> x1 = comm[...]   # returns [i for i in range(comm.size)]           */
/* >>> x2 = comm[:]     # returns [i for i in range(comm.size)]           */
/* >>> x3 = comm[:]                                                       */
/* >>> x4 = comm[::2]                                                     */
/* >>> x5 = comm[:-1]                                                     */
/*                                                                        */
/* >>> assert comm.size > 1, "running in parallel"                        */
/* >>> x6 = comm[1::2]                                                    */
/* >>> x7 = comm[1::2]                                                    */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_slice(PyObject* pySelf,PyObject* index) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  Py_ssize_t start;
  Py_ssize_t stop;
  Py_ssize_t step;
  Py_ssize_t size;
  Py_ssize_t i;
  PyObject* result = 0;
  PyObject* element = 0;
 
  COVERAGE();
  Assert(self);
  Assert(index);

  /* ----------------------------------------------- */
  /* If the index is a simple integer, we just call  */
  /* the comm_index function. We get here because    */
  /* the MAPPING table overrides the SEQUENCE table  */
  /* ----------------------------------------------- */
  if ( PyInt_Check(index) ) {
    return comm_item(pySelf,PyInt_AS_LONG(index));
  }

  /* ----------------------------------------------- */
  /* Slices specify the low, high, and step          */
  /* ----------------------------------------------- */
  if ( PySlice_Check(index) ) {
    if ( PySlice_GetIndices((PySliceObject*)index, self->size, &start,&stop,&step) ) {
      PYCHECK( PyErr_SetString(PyExc_TypeError,"invalid sequence slice") );
    }
  } else if ( index == Py_Ellipsis ) { /* Note: Py_Ellipsis is a singleton */
    start = 0;
    stop = self->size;
    step = 1;
  } else {
    Assert(index->ob_type && index->ob_type->tp_name);
    PYCHECK( PyErr_Format(PyExc_IndexError,"invalid index type - %s",index->ob_type->tp_name) );
  }

  /* ----------------------------------------------- */
  /* PySlice_GetIndices adjusts for negative values, */
  /* ----------------------------------------------- */
  if ( step == 0 ) {
      PYCHECK( PyErr_SetString(PyExc_IndexError,"invalid slice") );
  } if ( step < 0 ) {
    size = ( start - (stop+1) - step ) / (-step);
  } else {
    size = ( (stop-1) - start + step ) / step;
  }

  /* ----------------------------------------------- */
  /* Fill in a standard sequence                     */
  /* ----------------------------------------------- */
  PYCHECK( result = PyList_New(size) );
  for(i=0;i<size;++i) {
    PYCHECK( element/*owned*/ = PyInt_FromLong((long)(start+i*step)) );
    PYCHECK( PyList_SetItem(result,i,/*noinc*/ element) );
    element = 0;
  }
  return result;
  
 pythonError:
   Py_XDECREF(result);
   Py_XDECREF(element);
   return 0;
}

/**************************************************************************/
/* METHOD **************       comm_get_rank       ************************/
/**************************************************************************/
/* Returns local rank saved for this communicator.                        */
/**************************************************************************/
/* Return the local rank of a communicator.  See also mpi.get_rank()      */
/*                                                                        */
/* rank --> int                                                           */
/*                                                                        */
/* >>> rank = mpi.rank          # returns mpi.WORLD.get_rank()            */
/* >>> rank2 = comm.rank         # returns comm.get_rank()                */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_get_rank(PyObject* pySelf, void* closure) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);
  COVERAGE();

  return PyInt_FromLong(self->rank);
}

/**************************************************************************/
/* METHOD **************       comm_get_size       ************************/
/**************************************************************************/
/* Return the size (number of processes) associated with a communicator   */
/**************************************************************************/
/* Return the size of a communicator.  See also get_size()                */
/*                                                                        */
/* size --> int                                                           */
/*                                                                        */
/* >>> nprocs = mpi.size    # returns mpi.WORLD.size in reality           */
/* >>> cprocs = comm.size   # returns comm.get_size() as an attribute     */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_get_size(PyObject* pySelf, void* closure) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);
  COVERAGE();

  return PyInt_FromLong(self->size);
}

/**************************************************************************/
/* METHOD **************    comm_get_persistent    ************************/
/**************************************************************************/
/* Control MPI release of communicators                                   */
/*                                                                        */
/* >>> # Make sure we can't delete the attribute                          */
/* >>> try:                                                               */
/* ...    del comm.persistent                                             */
/* ... except AttributeError:                                             */
/* ...    pass                                                            */
/*                                                                        */
/**************************************************************************/
/* Persistence controls the MPI release of a communicator on destruction  */
/*                                                                        */
/* persistent --> int                                                     */
/*                                                                        */
/* >>> flag = comm.persistent  # returns 1 by default                     */
/* >>> comm.persistent = 0     # can also be cleared in c'tor             */
/*                                                                        */
/**************************************************************************/
static PyObject* comm_get_persistent(PyObject* pySelf, void* closure) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);
  COVERAGE();

  return PyInt_FromLong(self->persistent);
}

/**************************************************************************/
/* LOCAL  **************    comm_set_persistent    ************************/
/**************************************************************************/
/* Change persistent value                                                */
/**************************************************************************/
static int comm_set_persistent(PyObject* pySelf, PyObject* value, void* closure) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  long status;

  Assert(self);
  COVERAGE();

  /* ----------------------------------------------- */
  /* We don't support delete here                    */
  /* ----------------------------------------------- */
  if ( ! value ) {
    PYCHECK( PyErr_SetString(PyExc_AttributeError,"cannot delete the 'persistent' attribute") );
  }

  /* ----------------------------------------------- */
  /* We save it as a "boolean" (0 or 1 valued int)   */
  /* ----------------------------------------------- */
  PYCHECK( status = PyInt_AsLong(value) );
  self->persistent = (status != 0);

  return 0;

 pythonError:
  return -1;
}

/**************************************************************************/
/* LOCAL  **************        comm_getset        ************************/
/**************************************************************************/
/* This structure defines the property attributes for communicators       */
/* Note that the DOC strings are automagically processed from the METHOD  */
/* comments above.  Do not attempt to change the #define's directly.      */
/**************************************************************************/
static PyGetSetDef comm_getset[] = {
  {"persistent",comm_get_persistent,comm_set_persistent,PYMPI_COMM_GET_PERSISTENT_DOC,0},
  {"procs",comm_get_size,0,"See the size attribute.  This is a deprecated alias",0},
  {"rank",comm_get_rank,0,PYMPI_COMM_GET_RANK_DOC,0},
  {"size",comm_get_size,0,PYMPI_COMM_GET_SIZE_DOC,0},
  {0,0}
};


/**************************************************************************/
/* LOCAL  **************        comm_compare       ************************/
/**************************************************************************/
/* This enforces a full ordering on the universe of communicators.  This  */
/* is pretty basic (communicators are the same if the handles are the     */
/* same), but it is sufficient for sort and hashing.                      */
/**************************************************************************/
static int comm_compare(PyObject* pyLeft, PyObject* pyRight) {
  PyMPI_Comm* left = (PyMPI_Comm*)pyLeft;
  PyMPI_Comm* right = (PyMPI_Comm*)pyRight;

  COVERAGE();
  Assert(left && right);

  return ((long)(right->communicator) - (long)(left->communicator));
}

/**************************************************************************/
/* LOCAL  **************        comm_number        ************************/
/**************************************************************************/
/* Communicators don't really act like numbers, but we do want to provide */
/* the =0 functionality (for things like >>> if COMM: ....) and for       */
/* converting communicators back into their raw MPI handles to pass into  */
/* C, C++, and FORTRAN routines.                                          */
/**************************************************************************/
static PyNumberMethods comm_number = {
   0, /* binaryfunc nb_add; */
   0, /* binaryfunc nb_subtract; */
   0, /* binaryfunc nb_multiply; */
   0, /* binaryfunc nb_divide; */
   0, /* binaryfunc nb_remainder; */
   0, /* binaryfunc nb_divmod; */
   0, /* ternaryfunc nb_power; */
   0, /* unaryfunc nb_negative; */
   0, /* unaryfunc nb_positive; */
   0, /* unaryfunc nb_absolute; */
   comm_nonzero, /* inquiry nb_nonzero; */
   0, /* unaryfunc nb_invert; */
   0, /* binaryfunc nb_lshift; */
   0, /* binaryfunc nb_rshift; */
   0, /* binaryfunc nb_and; */
   0, /* binaryfunc nb_xor; */
   0, /* binaryfunc nb_or; */
   0, /* coercion nb_coerce; */
   comm_int, /* unaryfunc nb_int; */
   0, /* unaryfunc nb_long; */
   0, /* unaryfunc nb_float; */
   0, /* unaryfunc nb_oct; */
   0, /* unaryfunc nb_hex; */
};

/**************************************************************************/
/* LOCAL  **************       comm_sequence       ************************/
/**************************************************************************/
/* This sequence table provides length (e.g. MPI_Comm_size), and item     */
/* info.  The value returned by the __getitem__ and __getslice__ equivs   */
/* are uniteresting  comm[0] will return 0, comm[1] returns 1, etc...     */
/* This functionality is in place to allow code like:                     */
/*                                                                        */
/* >>> for r in comm:                                                     */
/* ...    print r                                                         */
/*                                                                        */
/* >>> # Serial part                                                      */
/* >>> for r in mpi.WORLD:                                                */
/* ...    serial_work(r)                                                  */
/*                                                                        */
/* Indexing and slicing are covered by the mapping table below            */
/**************************************************************************/
static PySequenceMethods comm_sequence = {
   comm_length, /* inquiry sq_length; */
   0, /* binaryfunc sq_concat; */
   0, /* intargfunc sq_repeat; */
   comm_item, /* intargfunc sq_item; */
   0, /* intintargfunc sq_slice; */
   0, /* intobjargproc sq_ass_item; */
   0, /* intintobjargproc sq_ass_slice; */
};

/**************************************************************************/
/* LOCAL  **************        comm_mapping       ************************/
/**************************************************************************/
/* This table has the MAPPING methods.  This is used to support ranged    */
/* slicing on communicators. e.g.  comm[::2] and comm[1::2] which return  */
/* the ranks for even and odd processes respectively.  This allows:       */
/*                                                                        */
/* >>> assert comm.size > 1, "running in parallel"                        */
/* >>> odd = comm.comm_create( comm[1::2] )                               */
/* >>> even = comm.comm_create( comm[::2] )                               */
/*                                                                        */
/**************************************************************************/
static PyMappingMethods comm_mapping = {
  comm_length,                  /*mp_length*/
  comm_slice,                   /*mp_subscript*/
  0,                            /*mp_ass_subscript*/
};


/**************************************************************************/
/* LOCAL  **************         comm_hash         ************************/
/**************************************************************************/
/* The hash is simply the address                                         */
/**************************************************************************/
static long comm_hash(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);
  COVERAGE();

  return (long)(self->communicator);
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_comm_type      ************************/
/**************************************************************************/
/* This defines a python interface to the MPI communicators.  Its pretty  */
/* much just the long sized value returned by MPI                         */
/**************************************************************************/
PyTypeObject pyMPI_comm_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                            /*int ob_size*/

  /* For printing, in format "<module>.<name>" */
  "communicator",               /*char* tp_name*/

  /* For allocation */
  sizeof(PyMPI_Comm),           /*int tp_basicsize*/
  0,                            /*int tp_itemsize*/

  /* Methods to implement standard operations */
  comm_dealloc,                 /*destructor tp_dealloc*/
  0,                            /*printfunc tp_print*/
  0,                            /*getattrfunc tp_getattr*/
  0,                            /*setattrfunc tp_setattr*/
  comm_compare,                 /*cmpfunc tp_compare*/
  0,                            /*reprfunc tp_repr*/

  /* Method suites for standard classes */
  &comm_number,                 /*PyNumberMethods* tp_as_number*/
  &comm_sequence,               /*PySequenceMethods* tp_as_sequence*/
  &comm_mapping,                /*PyMappingMethods* tp_as_mapping*/

  /* More standard operations (here for binary compatibility) */
  comm_hash,                    /*hasfunc tp_hash*/
  0,                            /*ternaryfunc tp_call*/
  0,                            /*reprfunc tp_str*/
  PyObject_GenericGetAttr,      /*getattrofunc tp_getattro*/
  0,                            /*setattrofunc tp_setattro*/

  /* Functions to access object as input/output buffer */
  0,                            /*PyBufferProcs *tp_as_buffer*/

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*long tp_flags*/

  /* Doc */
  PYMPI_COMM_INIT_DOC,

  0,                            /*traverseproc tp_traverse*/
  0,                            /*inquiry tp_clear*/
  0,                            /*richcmpfunc tp_richcompare*/
  0,                            /*long tp_weaklistoffset*/

  /* Iterators */
  0,                            /*getiterfunc tp_iter*/
  0,                            /*iternextfunc tp_iternext*/

  /* Attribute descriptor and subclassing stuff */
  comm_methods,                 /* PyMethodDef *tp_methods; */
  0,                            /* PyMemberDef *tp_members; */
  comm_getset,                  /* PyGetSetDef *tp_getset; */
  0,                            /* _typeobject *tp_base; */
  0,                            /* PyObject *tp_dict; */
  0,                            /* descrgetfunc tp_descr_get; */
  0,                            /* descrsetfunc tp_descr_set; */
  0,                            /* long tp_dictoffset; */
  comm_init,                    /* initproc tp_init; */
  PyType_GenericAlloc,          /* allocfunc tp_alloc */
  PyType_GenericNew,            /* newfunc tp_new; */
  0,                            /* destructor tp_free; Low-level free-memory routine */
  0,                            /* inquiry tp_is_gc;  For PyObject_IS_GC */
  0,                            /* PyObject *tp_bases; */
  0,                            /* PyObject *tp_mro;  method resolution order */
  0,                            /* PyObject *tp_cache; */
  0,                            /* PyObject *tp_subclasses; */
  0                             /* PyObject *tp_weaklist; */
};


/**************************************************************************/
/* GLOBAL **************         pyMPI_comm        ************************/
/**************************************************************************/
/* Build a Python communicator object from the MPI handle                 */
/* This is equivalent to calling the factory represented by the type      */
/* object.                                                                */
/**************************************************************************/
PyObject* pyMPI_comm(MPI_Comm communicator) {
  PyObject* result = 0;
  PyObject* factory = 0;

  COVERAGE();
  factory /*borrowed*/ = (PyObject*)&pyMPI_comm_type;
  PYCHECK( result/*owned*/ = PyObject_CallFunction(factory,"l",(long)communicator) );
  return result;

 pythonError:
  return 0;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_comm_init      ************************/
/**************************************************************************/
/* This is the mini-plugin that sets up the communicator type.  It maps   */
/* the bound methods of the world communicator up to the main mpi module  */
/* so that mpi.send() is really mpi.PYWORLD.send()                        */
/**************************************************************************/
void pyMPI_comm_init(PyObject** docStringP) {
  PyObject* factory = 0;
  PyObject* attribute = 0;
  PyMethodDef* method = 0;
  PyGetSetDef* getset = 0;

  COVERAGE();


  /* ----------------------------------------------- */
  /* Build communicators through the standard        */
  /* interface.                                      */
  /* ----------------------------------------------- */
  factory /*borrowed*/ = (PyObject*)&pyMPI_comm_type;
  Py_INCREF(factory);
  PARAMETER(factory,"communicator","create new communicators",(PyObject*),pyMPI_dictionary,docStringP);
  PARAMETER(pyMPI_COMM_WORLD,"WORLD","WORLD communicator",pyMPI_comm,pyMPI_dictionary,docStringP);
  PARAMETER(MPI_COMM_WORLD,"COMM_WORLD","MPI_COMM_WORLD communicator",pyMPI_comm,pyMPI_dictionary,docStringP);
  pyMPI_COMM_NULL/*owned*/ = pyMPI_comm(MPI_COMM_NULL);
  PARAMETER(pyMPI_COMM_NULL,"COMM_NULL","MPI_COMM_WORLD communicator",(PyObject*),pyMPI_dictionary,docStringP);


  /* ----------------------------------------------- */
  /* Map the bound member functions of WORLD to the  */
  /* module.  We do this to set the value of         */
  /* mpi.wxyz to mean mpi.WORLD.wxyz                 */
  /* ----------------------------------------------- */
  PYCHECK( pyMPI_world = PyDict_GetItemString(pyMPI_dictionary,"WORLD") );
  for(method=comm_methods; method->ml_name; ++method) {
    PYCHECK( attribute /*owned*/ = PyObject_GetAttrString(pyMPI_world,method->ml_name) );
    PYCHECK( PyDict_SetItemString(pyMPI_dictionary,method->ml_name,attribute) );
  }

  /* ----------------------------------------------- */
  /* Similarly, we grab the attributes out of WORLD  */
  /* and stuff them into the dictionary too.         */
  /* ----------------------------------------------- */
  for(getset=comm_getset; getset->name; ++getset) {
    PYCHECK( attribute /*owned*/ = PyObject_GetAttrString(pyMPI_world,getset->name) );
    PARAMETER( attribute, getset->name, getset->doc,(PyObject*), pyMPI_dictionary, docStringP );
  }

  /* Fall through */
 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_comm_fini      ************************/
/**************************************************************************/
/* Clean up work for plugin                                               */
/**************************************************************************/
void pyMPI_comm_fini(void) {
}

END_CPLUSPLUS

