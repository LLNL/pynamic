/**************************************************************************/
/* FILE   **************        pyMPI_util.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller May 15 2002                                     */
/* Copyright (C) 2002 Patrick J. Miller                                   */
/**************************************************************************/
/* Utility routines like pack/unpack                                      */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"

#define PYMPI_SKIP_SSIZE_DEF
#include "pyMPI.h"
#include "pyMPI_Macros.h"

#ifdef PYMPI_HAS_NUMPY
#undef PYMPI_HAS_NUMERIC
#include "numpy/arrayobject.h"
static int NUMPY_IS_LOADED = 0;
#endif

#ifdef PYMPI_HAS_NUMERIC
#include "Numeric/arrayobject.h"
static int NUMERIC_IS_LOADED = 0;
#endif


START_CPLUSPLUS

static PyObject* minus_one = 0;

/**************************************************************************/
/* GLOBAL **************   pyMPI_header_datatype   ************************/
/**************************************************************************/
/* This describes the header of a pyMPI_message structure                 */
/**************************************************************************/
MPI_Datatype pyMPI_header_datatype = MPI_DATATYPE_NULL;

/**************************************************************************/
/* GLOBAL **************   pyMPI_message_datatype  ************************/
/**************************************************************************/
/* This describes the PyMPI_Message structure                             */
/**************************************************************************/
MPI_Datatype pyMPI_message_datatype = MPI_DATATYPE_NULL;

/**************************************************************************/
/* GLOBAL **************      pyMPI_util_ready     ************************/
/**************************************************************************/
/* In many parts of the code, we need to know that MPI is ready to accept */
/* requests.  This implies that it is initialized but not finalized.      */
/**************************************************************************/
int pyMPI_util_ready(void) {
   int ready = 1;               /* We assume these status in case we don't */
   int finalized = 0;           /* have MPI_Initialized() and Finalized()  */
   int status;

#ifdef HAVE_MPI_INITIALIZED
   {
     int ierr;
     ierr = MPI_Initialized(&ready);
     Assert(ierr == 0);
   }
#endif

#ifdef HAVE_MPI_FINALIZED
   {
     int ierr;
     ierr = MPI_Finalized(&finalized);
     Assert(ierr == 0);
   }
#endif

   status = ready && !finalized;
   return status;
}

/**************************************************************************/
/* GLOBAL ************** pyMPI_util_not_implemented************************/
/**************************************************************************/
/* PURPOSE:  Stub for unimplemented features                              */
/**************************************************************************/
PyObject* pyMPI_util_not_implemented(PyObject* PySelf, PyObject* args) {
   PyErr_SetString(PyExc_NotImplementedError,"not implemented");
   return 0;
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_util_varargs    ************************/
/**************************************************************************/
/* Return a sequence of multiple arguments.  If only one item is in the   */
/* tuple and it is a sequence return it.  Otherwise, return all the args  */
/* as the original tuple (after INCREF).                                  */
/**************************************************************************/
PyObject* pyMPI_util_varargs(PyObject* args) {
  COVERAGE();
  Assert(args);
  Assert(PyTuple_Check(args));

  /* ----------------------------------------------- */
  /* If this is a tuple of one arg that's a sequence */
  /* then return an owned reference.                 */
  /* ----------------------------------------------- */
  if ( PyTuple_GET_SIZE(args) == 1 &&
       PySequence_Check(PyTuple_GET_ITEM(args,0)) ) {
    Py_INCREF(PyTuple_GET_ITEM(args,0));
    return PyTuple_GET_ITEM(args,0);
  }

  /* ----------------------------------------------- */
  /* Otherwise, return the arg tuple itself          */
  /* ----------------------------------------------- */
  Py_INCREF(args);
  return args;
}

/**************************************************************************/
/* GLOBAL ********** pyMPI_util_tuple_to_spaced_string ********************/
/**************************************************************************/
/* This function glues together string representations of the elements of */
/* a tuple.                                                               */
/**************************************************************************/
PyObject* pyMPI_util_tuple_to_spaced_string(PyObject* args) {
   PyObject* line = 0;
   PyObject* arg = 0;
   PyObject* newLine = 0;
   PyObject* space = 0;
   PyObject* str = 0;
   int i;

   /* ----------------------------------------------- */
   /* Prepare a string with space separated strings   */
   /* of all of the arguments (may be empty)          */
   /* ----------------------------------------------- */
   PYCHECK( line/*owned*/ = PyString_FromString("") );
   PYCHECK( space/*owned*/ = PyString_FromString(" ") );
   
   Assert( PyTuple_Check(args) );
   for(i=0;i<PyTuple_GET_SIZE(args);++i) {
     arg/*borrow*/ = PyTuple_GET_ITEM(args,i);

     /* ----------------------------------------------- */
     /* Add a space                                     */
     /* ----------------------------------------------- */
     if ( i > 0 ) {
       PYCHECK( newLine/*owned*/ = PySequence_Concat(line,space) );
       Py_DECREF(line);
       line = newLine;
       newLine = 0;
     }

     /* ----------------------------------------------- */
     /* Add a string                                    */
     /* ----------------------------------------------- */
     PYCHECK( str/*owned*/ = PyObject_Str(arg) );
     PYCHECK( newLine/*owned*/ = PySequence_Concat(line,str) );

     Py_DECREF(str);
     Py_DECREF(line);
     line = newLine;
     newLine = 0;
   }

   Py_XDECREF(space);
   return line;

 pythonError:
   Py_XDECREF(line);
   Py_XDECREF(newLine);
   Py_XDECREF(space);
   Py_XDECREF(str);
   return 0;
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_message_free    ************************/
/**************************************************************************/
/* Free message buffers                                                   */
/**************************************************************************/
void pyMPI_message_free(PyMPI_Message* part1, char** part2) {
  /* ----------------------------------------------- */
  /* Nothing to clear...                             */
  /* ----------------------------------------------- */
  if ( !part2 || !(*part2)) return;
  
  /* ----------------------------------------------- */
  /* default is to free memory                       */
  /* ----------------------------------------------- */
  assert(part1);
  if ( part1->header.free_buffer && part2 ) {
    free(*part2);
  }

  /* ----------------------------------------------- */
  /* Clear the pointer anyway                        */
  /* ----------------------------------------------- */
  if ( part2 ) *part2 = 0;
  return;
}

/**************************************************************************/
/* GLOBAL **************        pyMPI_unpack       ************************/
/**************************************************************************/
/* Unpack and reconstitute a serialized message string                    */
/**************************************************************************/
PyObject* pyMPI_unpack(PyMPI_Message* part1, char** part2) {
  PyObject* result = 0;
  PyObject* aux = 0;
  PyObject* shape = 0;

  assert(part1);

  /* ----------------------------------------------- */
  /* First, we see if we have a special unpack..     */
  /* ----------------------------------------------- */
  switch ( part1->header.message_type ) {
    /* ----------------------------------------------- */
    /* Long has its own payload slot                   */
    /* ----------------------------------------------- */
  case PyMPI_AS_LONG:
    PYCHECK( result = PyInt_FromLong(part1->header.long_payload) );
    break;

    /* ----------------------------------------------- */
    /* Double has its own payload slot                 */
    /* ----------------------------------------------- */
  case PyMPI_AS_DOUBLE:
    PYCHECK( result = PyFloat_FromDouble(part1->header.double_payload) );
    break;

    /* ----------------------------------------------- */
    /* Strings are just copied                         */
    /* ----------------------------------------------- */
  case PyMPI_AS_STRING:
    /* String unpack */
    if ( part1->header.bytes_in_prefix ) {
      PYCHECK( result = PyString_FromStringAndSize(part1->prefix,part1->header.bytes_in_prefix) );
    } else {
      part1->header.free_buffer = 1;
      PYCHECK( result = PyString_FromStringAndSize(*part2,part1->header.bytes_in_second_message) );
    }
    break;


  case PyMPI_AS_NUMPY:
#ifndef PYMPI_HAS_NUMPY
    PYCHECK( PyErr_SetString(PyExc_NotImplementedError,"No special Numarray") );
#else
    {
      npy_intp dims[1000];
      int nd;
      int i;
      PYCHECK( aux /*owned*/ = PyString_FromStringAndSize(part1->prefix,part1->header.bytes_in_prefix) );
      PYCHECK( shape /*owned*/= PyObject_CallFunctionObjArgs(pyMPI_pickle_loads,aux,0) );
      Py_DECREF(aux);
      aux = 0;

      nd = PyTuple_Size(shape);
      Assert(nd < sizeof(dims)/sizeof(dims[0]));
      for(i=0;i<nd;++i) {
        PyObject* el = PyTuple_GET_ITEM(shape,i);
        PYCHECK( dims[i] = PyInt_AsLong(el) );
      }
      Py_DECREF(shape);
      shape = 0;

      Assert(part2 && *part2 && part1->header.bytes_in_second_message > 0);
      
      PYCHECK( result = PyArray_SimpleNew(nd,dims,part1->header.long_payload) );
      memcpy(PyArray_DATA(result),*part2,part1->header.bytes_in_second_message);
    }
#endif
    break;

  case PyMPI_AS_NUMARRAY:
    PYCHECK( PyErr_SetString(PyExc_NotImplementedError,"No special Numarray") );
    break;


  case PyMPI_AS_NUMERIC:
#ifndef PYMPI_HAS_NUMERIC
    PYCHECK( PyErr_SetString(PyExc_NotImplementedError,"No special Numeric") );
#else
    {
      int dims[1000];
      int nd;
      int i;
      PYCHECK( aux /*owned*/ = PyString_FromStringAndSize(part1->prefix,part1->header.bytes_in_prefix) );
      PYCHECK( shape /*owned*/= PyObject_CallFunctionObjArgs(pyMPI_pickle_loads,aux,0) );
      Py_DECREF(aux);
      aux = 0;

      nd = PyTuple_Size(shape);
      Assert(nd < sizeof(dims)/sizeof(dims[0]));
      for(i=0;i<nd;++i) {
        PyObject* el = PyTuple_GET_ITEM(shape,i);
        PYCHECK( dims[i] = PyInt_AsLong(el) );
      }
      Py_DECREF(shape);
      shape = 0;
      PYCHECK( result = PyArray_FromDims(nd,dims,part1->header.long_payload) );
      Assert(part2 && *part2 && part1->header.bytes_in_second_message > 0);
      memcpy(((PyArrayObject*)result)->data,*part2,part1->header.bytes_in_second_message);
    }
#endif
    break;

    /* ----------------------------------------------- */
    /* Pickled objects are unpacked from strings       */
    /* ----------------------------------------------- */
  case PyMPI_AS_PICKLE:

    if ( !pyMPI_pickle_loads ) {
      PYCHECK( PyErr_SetString(PyExc_RuntimeError,"No pickle load function") );
    }

    else if ( part1->header.bytes_in_second_message ) {
      assert(part2 && *part2);
      PYCHECK2( result = PyObject_CallFunction(pyMPI_pickle_loads,"s#",*part2,part1->header.bytes_in_second_message),
                PyExc_ValueError,
                "Fatal internal unpickling error");
    }

    else {
      PYCHECK2( result = PyObject_CallFunction(pyMPI_pickle_loads,"s#",part1->prefix,part1->header.bytes_in_prefix),
                PyExc_ValueError,
                "Fatal internal unpickling error");
    }
    break;

  default:
    PYCHECK( PyErr_SetString(PyExc_NotImplementedError,"No special sends") );
  }

 pythonError:
  Py_XDECREF(aux);
  Py_XDECREF(shape);
  pyMPI_message_free(part1,part2);
  return result;
}

/**************************************************************************/
/* GLOBAL **************         pyMPI_pack        ************************/
/**************************************************************************/
/* Serialize an object.  Split into two buffers if larger than the eager  */
/* limit.  If pyMPI_pack() returns a non-null pointer, it has to be held  */
/* through the lifetime of the call.  The reference is "BORROWED"         */
/**************************************************************************/
void pyMPI_pack(PyObject* object,PyMPI_Comm* comm, PyMPI_Message* buffer1, char** buffer2) {
  PyObject* tempObject = 0;
  char* pickledStr = 0;
  int   pickledStrLen = 0;

  Assert(object);
  Assert(buffer2);

  /* ----------------------------------------------- */
  /* Special case for int                            */
  /* ----------------------------------------------- */
  if ( PyInt_CheckExact(object) ) {
    buffer1->header.double_payload = 0.0;
    buffer1->header.long_payload = PyInt_AS_LONG(object);
    buffer1->header.bytes_in_second_message = 0;
    buffer1->header.message_type= PyMPI_AS_LONG;
    buffer1->header.free_buffer = 0;
    buffer1->header.bytes_in_prefix = 0;
  }

  /* ----------------------------------------------- */
  /* Special case for float                          */
  /* ----------------------------------------------- */
  else if ( PyFloat_CheckExact(object) ) {
    buffer1->header.double_payload = PyFloat_AS_DOUBLE(object);
    buffer1->header.long_payload = 0;
    buffer1->header.bytes_in_second_message = 0;
    buffer1->header.message_type= PyMPI_AS_DOUBLE;
    buffer1->header.free_buffer = 0;
    buffer1->header.bytes_in_prefix = 0;
  }

  /* ----------------------------------------------- */
  /* Special case for strings                        */
  /* ----------------------------------------------- */
  else if ( PyString_CheckExact(object) ) {
    buffer1->header.double_payload = 0.0;
    buffer1->header.long_payload = 0;
    buffer1->header.message_type = PyMPI_AS_STRING;

    if ( PyString_GET_SIZE(object) <= PYMPI_PREFIXCHARS ) {
      buffer1->header.bytes_in_prefix = PyString_GET_SIZE(object);
      buffer1->header.bytes_in_second_message = 0;
      buffer1->header.free_buffer = 0; /* We do NOT need to free(buffer) */

      memcpy(buffer1->prefix,PyString_AS_STRING(object),PyString_GET_SIZE(object));

      Assert(buffer2);
      *buffer2 = 0;
    } else {
      buffer1->header.bytes_in_prefix = 0;
      buffer1->header.bytes_in_second_message = PyString_Size(object);
      buffer1->header.free_buffer = 0; /* We do NOT need to free(buffer) */

      Assert(buffer2);
      *buffer2 /*borrow*/ = PyString_AS_STRING(object);
    }
  }

#ifdef PYMPI_HAS_NUMERIC
  /* ----------------------------------------------- */
  /* Continguous arrays are easy to send for basic   */
  /* types                                           */
  /* ----------------------------------------------- */
  else if ( NUMERIC_IS_LOADED && PyArray_Check(object) && ((PyArrayObject*)object)->descr->type != PyArray_OBJECT && PyArray_ISCONTIGUOUS((PyArrayObject*)object) ) {
    /* Note: If array is contiguous, then it will be forward contiguous */
    PyArrayObject* array = (PyArrayObject*)object;
    PyObject* shape = 0;
    PyObject* aux = 0;
    PyObject* saux = 0;

    /* We build a tuple with auxillary info to pass over */
    shape = PyObject_GetAttrString(object,"shape");
    Assert(pyMPI_pickle_dumps);
    PYCHECK( aux = PyObject_CallFunctionObjArgs(pyMPI_pickle_dumps,shape,0) );
    Assert( PyString_Check(aux) );

    buffer1->header.double_payload = 0.0;
    buffer1->header.long_payload = array->descr->type;
    buffer1->header.bytes_in_second_message = PyArray_NBYTES(array);
    buffer1->header.message_type = PyMPI_AS_NUMERIC;
    buffer1->header.free_buffer = 0; /* already owned by array */
    buffer1->header.bytes_in_prefix = PyString_GET_SIZE(aux);
    Assert(buffer1->header.bytes_in_prefix<=PYMPI_PREFIXCHARS);
    memcpy(buffer1->prefix,PyString_AS_STRING(aux),PyString_GET_SIZE(aux));
    Assert(buffer2);
    *buffer2 /*borrow*/ = (char*)(array->data);
  }
#endif

#ifdef PYMPI_HAS_NUMPY
  /* ----------------------------------------------- */
  /* Continguous arrays are easy to send for basic   */
  /* types                                           */
  /* ----------------------------------------------- */
  else if ( NUMPY_IS_LOADED && PyArray_Check(object) && ((PyArrayObject*)object)->descr->type != PyArray_OBJECT && PyArray_ISCONTIGUOUS((PyArrayObject*)object) ) {
    /* Note: If array is contiguous, then it will be forward contiguous */
    PyArrayObject* array = (PyArrayObject*)object;
    PyObject* shape = 0;
    PyObject* aux = 0;
    PyObject* saux = 0;

    /* We build a tuple with auxillary info to pass over */
    shape = PyObject_GetAttrString(object,"shape");
    Assert(pyMPI_pickle_dumps);
    PYCHECK( aux = PyObject_CallFunctionObjArgs(pyMPI_pickle_dumps,shape,0) );
    Assert( PyString_Check(aux) );

    buffer1->header.double_payload = 0.0;
    buffer1->header.long_payload = PyArray_TYPE(array);
    buffer1->header.bytes_in_second_message = PyArray_NBYTES(array);
    buffer1->header.message_type = PyMPI_AS_NUMPY;
    buffer1->header.free_buffer = 0; /* already owned by array */
    buffer1->header.bytes_in_prefix = PyString_GET_SIZE(aux);
    Assert(buffer1->header.bytes_in_prefix<=PYMPI_PREFIXCHARS);
    memcpy(buffer1->prefix,PyString_AS_STRING(aux),PyString_GET_SIZE(aux));
    Assert(buffer2);
    *buffer2 = PyArray_DATA(array);
  }
#endif
  

  /* ----------------------------------------------- */
  /* Fall back on pickle                             */
  /* ----------------------------------------------- */
  else if ( pyMPI_pickle_dumps ) {
    buffer1->header.double_payload = 0.0;
    buffer1->header.long_payload = 0;
    buffer1->header.message_type = PyMPI_AS_PICKLE;

    /* ----------------------------------------------- */
    /* Call the pickle dumps() function to serialize   */
    /* ----------------------------------------------- */
    PYCHECK( tempObject/*owned*/ = PyObject_CallFunctionObjArgs(pyMPI_pickle_dumps,object,minus_one,0) );

    /* ----------------------------------------------- */
    /* We expect cPickle.dump() to return a string     */
    /* ----------------------------------------------- */
    if ( !(PyString_Check(tempObject))) {
      PYCHECK(PyErr_SetString( PyExc_SystemError,
                               "Could not create pickle string on object."));
    }
    PYCHECK( pickledStr/*borrowed*/ = PyString_AsString(tempObject); );
    PYCHECK( pickledStrLen = PyString_Size(tempObject) ); 

    /* ----------------------------------------------- */
    /* Pack the message length into buffer             */
    /* ----------------------------------------------- */
    if ( pickledStrLen <= PYMPI_PREFIXCHARS ) {
      buffer1->header.bytes_in_prefix = pickledStrLen;
      buffer1->header.bytes_in_second_message = 0;
      buffer1->header.free_buffer = 0; /* We do not need to free(buffer) */
      memcpy(buffer1->prefix,pickledStr,pickledStrLen);
    }

    /* ----------------------------------------------- */
    /* It didn't fit, allocate an overflow buffer      */
    /* ----------------------------------------------- */
    else {
      buffer1->header.bytes_in_prefix = 0;
      buffer1->header.bytes_in_second_message = pickledStrLen;
      buffer1->header.free_buffer = 1; /* We need to free(buffer) */

      Assert(buffer2);
      *buffer2 /*owned*/= (char*)malloc(pickledStrLen);

      Assert(*buffer2);
      memcpy(*buffer2,pickledStr,pickledStrLen);
    }

  } 

  else {
    PYCHECK( PyErr_SetString(PyExc_ImportError,"Cannot pickle messages") );
  }

  Py_XDECREF(tempObject);
  return;

 pythonError:
  Py_XDECREF(tempObject);
  pyMPI_message_free(buffer1,buffer2);
  return;
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_resultstatus    ************************/
/**************************************************************************/
/* This is a convenience function used to pack a MPI status result and a  */
/* Python object together in a tuple.  Typically for functions/methods    */
/* that return both.  We assume that result is "owned"                    */
/**************************************************************************/
PyObject* pyMPI_resultstatus(PyObject* result, MPI_Status status) {
   PyObject* pyStat = 0;
   PyObject* pyTuple = 0;

   /* Build a MPI Status thingy */
   PYCHECK( pyStat/*owned*/ = pyMPI_status(status) );

   /* Stick into a tuple with original result */
   PYCHECK( pyTuple /*owned*/ = PyTuple_New(2); );
   PyTuple_SET_ITEM(pyTuple,0,result);
   PyTuple_SET_ITEM(pyTuple,1,pyStat);
   return pyTuple;
   
 pythonError:
   Py_XDECREF(pyStat);
   Py_XDECREF(pyTuple);
   return 0;
}


/**************************************************************************/
/* GLOBAL **************  pyMPI_util_integer_array ************************/
/**************************************************************************/
/* Build a Python tuple from an array of integers                         */
/**************************************************************************/
PyObject* pyMPI_util_integer_array(int n, int* array) {
  PyObject* tuple = 0;
  PyObject* element = 0;
  int i;

  COVERAGE();

  Assert(n >= 0);
  Assert(array);

  tuple = PyTuple_New(n);
  for ( i = 0; i < n; i ++ ) {
    PYCHECK( element /*owned*/ = PyInt_FromLong(array[i]) );
    PYCHECK( PyTuple_SetItem(tuple,i,/*noinc*/element) );
    element = 0;
  }

  return tuple;

pythonError:
  Py_XDECREF(tuple);
  Py_XDECREF(element);
  return 0;

}


/**************************************************************************/
/* GLOBAL ***********  pyMPI_util_sequence_to_int_array  ******************/
/**************************************************************************/
/* Convert a sequence (range,array, whatever) to a malloc'd array of int  */
/**************************************************************************/
int* pyMPI_util_sequence_to_int_array(PyObject* sequence,char* message) {
  int n;
  int* array = 0;
  PyObject* element = 0;
  int i;
  
  Assert(sequence);
  PYCHECK( n = PyObject_Size(sequence) );
  if ( n < 0 ) goto pythonError;

  /* ----------------------------------------------- */
  /* Some malloc don't like size 0 and some return a */
  /* NULL pointer for size 0.  We will want to free  */
  /* something later, and free(0) is often bad too!  */
  /* We just malloc a small thing and return it :-(  */
  /* ----------------------------------------------- */
  if ( n == 0 ) {
    array =  (int*) malloc(sizeof(int));
  } else {
    array = (int*)(malloc(n*sizeof(int)));
  }

  if ( !array ) {
    PYCHECK( PyErr_Format(PyExc_MemoryError,"Could not malloc %d integers",n) );
  }

  for(i=0;i<n;++i) {
    PYCHECK( element = PySequence_GetItem(sequence,i) );
    PYCHECK( array[i] = PyInt_AsLong(element) );
  }
  
  return array;

 pythonError:
  Py_XDECREF(element);
  if ( array ) free(array);
  Assert(message);
  PyErr_SetString(PyExc_ValueError,message);
  return 0;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_make_numpy     ************************/
/**************************************************************************/
/* Build a 1-D numpy block                                                */
/**************************************************************************/
PyObject* pyMPI_make_numpy(Py_ssize_t count, int type, void** data) {
#ifdef PYMPI_HAS_NUMPY
  npy_intp dim = count;
  PyObject* result = PyArray_SimpleNew(1,&dim,type);
  *data = PyArray_DATA(result);
  return result;
#else
  return NULL;
#endif
}

/**************************************************************************/
/* GLOBAL **************  pyMPI_crack_numpy_object ************************/
/**************************************************************************/
/* Get the interesting bits out of a numpy array                          */
/**************************************************************************/
void pyMPI_crack_numpy_object(PyObject* array, void** data, Py_ssize_t* count, MPI_Datatype* mpitype,int* numpytype) {
#ifdef PYMPI_HAS_NUMPY
  /* ----------------------------------------------- */
  /* Object better be an array...                    */
  /* ----------------------------------------------- */
  if (!PyArray_Check(array)) {
    PyErr_SetString(PyExc_TypeError,"message is not a numpy array, cannot use native mode");
    return;
  }
  if (!PyArray_ISCONTIGUOUS(array)) {
    PyErr_SetString(PyExc_TypeError,"array is not contiguous");
    return;
  }

  *data = PyArray_DATA(array);
  *count = PyArray_SIZE(array);
  *numpytype = PyArray_TYPE(array);

  switch(*numpytype) {
  case NPY_BYTE: *mpitype = MPI_BYTE; break;
  case NPY_SHORT: *mpitype = MPI_SHORT; break;
  case NPY_USHORT: *mpitype = MPI_UNSIGNED_SHORT; break;
  case NPY_INT: *mpitype = MPI_INT; break;
  case NPY_UINT: *mpitype = MPI_UNSIGNED; break;
  case NPY_LONG: *mpitype = MPI_LONG; break;
  case NPY_ULONG: *mpitype = MPI_UNSIGNED_LONG; break;
  case NPY_LONGLONG: *mpitype = MPI_LONG_LONG; break;
  case NPY_ULONGLONG: *mpitype = MPI_UNSIGNED_LONG_LONG; break;
  case NPY_FLOAT: *mpitype = MPI_FLOAT; break;
  case NPY_DOUBLE: *mpitype = MPI_DOUBLE; break;
  case NPY_LONGDOUBLE: *mpitype = MPI_LONG_DOUBLE; break;
  default:
    PyErr_SetString(PyExc_TypeError,"Unsupported numpy element type");
    return;
  }
#endif
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_util_init      ************************/
/**************************************************************************/
/* Initialize and commit the message send datatype                        */
/**************************************************************************/
void pyMPI_util_init(PyObject** docStringP) {
  int counts[5];
  MPI_Aint offset[4];
  MPI_Datatype eltype[4];

  MPI_Aint double_extent = 0;
  MPI_Aint long_extent = 0;
  MPI_Aint int_extent = 0;
  MPI_Aint short_extent = 0;
  MPI_Aint header_extent = 0;
  MPI_Aint message_extent = 0;

#ifdef PYMPI_HAS_NUMERIC
  /* "Secret handshake" needed to have active Numeric API */
  if (PyImport_ImportModule("Numeric")) import_array();
  if (!PyErr_Occurred()) {
    NUMERIC_IS_LOADED = 1;
  }
#endif
#ifdef PYMPI_HAS_NUMPY
  /* "Secret handshake" needed to have active Numpy API */
  if (PyImport_ImportModule("numpy")) import_array();
  if (!PyErr_Occurred()) {
    NUMPY_IS_LOADED = 1;
  }
#endif
  PyErr_Clear();

  PYCHECK( minus_one = PyInt_FromLong(-1) );

  MPICHECKCOMMLESS( MPI_Type_extent(MPI_DOUBLE,&double_extent) );
  MPICHECKCOMMLESS( MPI_Type_extent(MPI_LONG,&long_extent) );
  MPICHECKCOMMLESS( MPI_Type_extent(MPI_INT,&int_extent) );
  MPICHECKCOMMLESS( MPI_Type_extent(MPI_SHORT,&short_extent) );

  counts[0] = 1;
  offset[0] = 0;
  eltype[0] = MPI_DOUBLE;

  counts[1] = 1;
  offset[1] = offset[0] + counts[0]*double_extent;
  eltype[1] = MPI_LONG;

  counts[2] = 2;
  offset[2] = offset[1] + counts[1]*long_extent;
  eltype[2] = MPI_INT;

  counts[3] = 2;
  offset[3] = offset[2] + counts[2]*int_extent;
  eltype[3] = MPI_SHORT;

  MPICHECKCOMMLESS( MPI_Type_struct(4,counts,offset,eltype,&pyMPI_header_datatype) );
  MPICHECKCOMMLESS( MPI_Type_commit(&pyMPI_header_datatype) );
  MPICHECKCOMMLESS( MPI_Type_extent(pyMPI_header_datatype,&header_extent) );

  if ((unsigned)header_extent != (unsigned)sizeof(PyMPI_Protocol)) {
    PyErr_Format(PyExc_RuntimeError,"MPI extent of header (%d) is different than C's (%d)",
                 (int)header_extent,(int)sizeof(PyMPI_Protocol));
  }

  counts[0] = 1;
  offset[0] = 0;
  eltype[0] = pyMPI_header_datatype;

  counts[1] = PYMPI_PREFIXCHARS;
  offset[1] = offset[0] + counts[0]*header_extent;
  eltype[1] = MPI_CHAR;

  MPICHECKCOMMLESS( MPI_Type_struct(2,counts,offset,eltype,&pyMPI_message_datatype) );
  MPICHECKCOMMLESS( MPI_Type_commit(&pyMPI_message_datatype) );
  MPICHECKCOMMLESS( MPI_Type_extent(pyMPI_message_datatype,&message_extent) );

  if ((unsigned)message_extent != (unsigned)sizeof(PyMPI_Message)) {
    PyErr_Format(PyExc_RuntimeError,"MPI extent of message (%d) is different than C's (%d)",
                 (int)message_extent,(int)sizeof(PyMPI_Message));
  }

 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_util_fini      ************************/
/**************************************************************************/
/* Release the previously committed datatype                              */
/**************************************************************************/
void pyMPI_util_fini(void) {
  Py_XDECREF(minus_one);

  if ( pyMPI_message_datatype != MPI_DATATYPE_NULL ) {
    /* In teardown, so ignore any error */
    MPI_Type_free(&pyMPI_message_datatype);
    pyMPI_message_datatype = MPI_DATATYPE_NULL;
    MPI_Type_free(&pyMPI_header_datatype);
    pyMPI_header_datatype = MPI_DATATYPE_NULL;
  }
}

END_CPLUSPLUS
