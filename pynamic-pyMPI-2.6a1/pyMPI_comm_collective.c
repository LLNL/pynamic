/**************************************************************************/
/* FILE   **************  pyMPI_comm_collective.c  ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/* Copyright (C) 2002 University of California Regents                    */
/**************************************************************************/
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/* >>> local_dt = 1.125                                                   */
/* >>> comm = WORLD.comm_dup()                                            */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

typedef enum { REDUCE_TO_ROOT = 0, REDUCE_AND_BROADCAST = 1, REDUCE_VIA_SCAN = 2 } reduction_type;


START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************          sequence         ************************/
/**************************************************************************/
/* Return an owned reference to an object (possibly the same object) as   */
/* a Python [list].   Makes a copy if the object is not a Sequence, but   */
/* still supports len() and __getitem__()                                 */
/**************************************************************************/
static PyObject* sequence(PyObject* object) {
  int n;
  PyObject* element = 0;
  PyObject* result = 0;
  int i;

  COVERAGE();

  /* ----------------------------------------------- */
  /* Already a list is fine....                      */
  /* ----------------------------------------------- */
  Assert(object);
  if ( PyList_Check(object) ) {
    Py_INCREF(object);
    return object;
  }

  /* ----------------------------------------------- */
  /* Otherwise, it better look like a sequence       */
  /* ----------------------------------------------- */
  if ( !PySequence_Check(object) ) {
    PYCHECK( PyErr_SetString(PyExc_ValueError,"Argument is not a sequence") );
  }

  /* ----------------------------------------------- */
  /* Gather into a [] list                           */
  /* ----------------------------------------------- */
  PYCHECK( n = PySequence_Size(object) );
  PYCHECK( result/*owned*/ = PyList_New(n) );

  for(i=0;i<n;++i) {
    PYCHECK( element/*owned*/ = PySequence_GetItem(object,i) );
    PyList_SET_ITEM(result,i,element);
    element = 0;
  }

  return result;

 pythonError:
  Py_XDECREF(element);
  Py_XDECREF(result);
  return 0;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_collective     ************************/
/**************************************************************************/
/* Receive a message from all processes (except a declared root process's */
/* value which is passed in). Returns a Python tuple of all values        */
/**************************************************************************/
PyObject* pyMPI_collective( PyMPI_Comm *self, int root, PyObject* localValue, int includeRank) {
  PyObject* result = 0;
  PyObject* status = 0;
  PyObject* messageAndStatus = 0;
  PyObject* message = 0;
  PyObject* tuple = 0;
  PyObject* rank = 0;
  int i;
  int TAGTOUSE = 987;

  Assert(self);
  Assert(localValue);

  /* ----------------------------------------------- */
  /* On slaves, send data                            */
  /* ----------------------------------------------- */
  if ( self->rank != root ) {
    PYCHECK( status /*owned*/ = pyMPI_send(self,localValue,root,TAGTOUSE) );
    Py_XDECREF(status);

    /* ----------------------------------------------- */
    /* In the case of an error, we're gonna be hosed.  */
    /* The root was expecting values that will never   */
    /* be delivered.                                   */
    /* ----------------------------------------------- */
    PyErr_Clear();
    return 0;
  }

  /* ----------------------------------------------- */
  /* On root, build a tuple to hold all values       */
  /* ----------------------------------------------- */
  PYCHECK( result = PyTuple_New(self->size) );

  /* ----------------------------------------------- */
  /* Grab an item from every sending process         */
  /* ----------------------------------------------- */
  for(i=0; i<self->size; ++i) {
    if ( i == root ) {
      message /*borrowed*/ = localValue;
      Py_INCREF(localValue); /*owned*/
    } else {
      PYCHECK( messageAndStatus /*owned*/ = pyMPI_recv(self,i,TAGTOUSE) );
      Assert( PyTuple_Check(messageAndStatus) );
      message /*borrowed*/ = PyTuple_GET_ITEM(messageAndStatus,0); /* no checks */
      Py_INCREF(message); /*owned*/

      Py_DECREF(messageAndStatus);
      messageAndStatus = 0;
    }

    /* build tuple with rank for minloc maxloc */
    if ( includeRank ) {
      PYCHECK( tuple /*owned*/ = PyTuple_New(2) );
      PYCHECK( rank /*owned*/ = PyInt_FromLong(i) );
      PyTuple_SET_ITEM(tuple, 0,message);
      PyTuple_SET_ITEM(tuple, 1,rank);
      message = tuple;
      tuple = 0;
    }

    PyTuple_SET_ITEM(result,i,message); /* no checks */
  }

  return result;
pythonError:
  Py_XDECREF(result);
  Py_XDECREF(messageAndStatus);
  Py_XDECREF(message);
  return 0;
}

/**************************************************************************/
/* LOCAL  **************           bcast           ************************/
/**************************************************************************/
/* Use MPI_Bcast() to send a Python object from root to other processes   */
/* This is the basis for mpi.bcast() and is also used as a utility in a   */
/* couple of places in the collective implementation.                     */
/**************************************************************************/
static PyObject* bcast(PyMPI_Comm * self, PyObject* message, int root) {
  PyMPI_Message buffer1;
  char *buffer2 = 0;
  PyObject*result = 0;

  /* ----------------------------------------------- */
  /* Check for valid root to bcast                   */
  /* ----------------------------------------------- */
  if (root < 0 || root >= self->size) {
    PYCHECK(PyErr_SetString(PyExc_ValueError, "Invalid root rank"));
  }
  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  RAISEIFNOMPI();

  /* ----------------------------------------------- */
  /* Message is optional (and ignored) on slaves     */
  /* ----------------------------------------------- */
  if ((self->rank == root) && !(message)) {
    PYCHECK(PyErr_SetString(PyExc_ValueError,
                    "Broadcast needs a message on the root process\n."));
  }
  /* ----------------------------------------------- */
  /* Get MPI message ready                           */
  /* ----------------------------------------------- */
  if (self->rank == root) {
    buffer2 = 0;
    PYCHECK(pyMPI_pack(message, self, &buffer1, &buffer2) );

    /* ----------------------------------------------- */
    /* Broadcast initial buffer                        */
    /* ----------------------------------------------- */
    MPICHECK(self->communicator,
             MPI_Bcast(&buffer1, 1, pyMPI_message_datatype,
                       root, self->communicator));

    if (buffer2 != 0) {
      MPICHECK(self->communicator,
               MPI_Bcast(buffer2, buffer1.header.bytes_in_second_message,
                         MPI_CHAR, root, self->communicator));
    }
    result = message;
    Py_XINCREF(result);
  } else {
    /* ----------------------------------------------- */
    /* Receive broadcast of initial buffer             */
    /* ----------------------------------------------- */
    MPICHECK(self->communicator,
             MPI_Bcast(&buffer1, 1, pyMPI_message_datatype,
                       root, self->communicator));

    /* ----------------------------------------------- */
    /* If the message didn't fit in the first buffer,  */
    /* receive a second                                */
    /* ----------------------------------------------- */
    if (buffer1.header.bytes_in_second_message) {
      buffer2 = (char *) malloc(buffer1.header.bytes_in_second_message);
      Assert(buffer2);
      MPICHECK(self->communicator,
               MPI_Bcast(buffer2,buffer1.header.bytes_in_second_message,
                         MPI_CHAR,root,self->communicator));
      buffer1.header.free_buffer = 1; /* This malloc'd space needs to be freed */
    }
    PYCHECK(result = pyMPI_unpack(&buffer1,&buffer2));
  }

  pyMPI_message_free(&buffer1,&buffer2);
  return result;

pythonError:
  pyMPI_message_free(&buffer1,&buffer2);
  Py_XDECREF(result);
  return 0;
}

/**************************************************************************/
/* LOCAL  ************ specialized_double_reduction **********************/
/**************************************************************************/
/* This handles the 4 special kinds of double reduction.  The kinds are  */
/* ( broadcast | no broadcast ) cross ( XXXLOC | normal reduction )       */
/**************************************************************************/
static PyObject* specialized_double_reduction(PyObject* message, MPI_Op op, int root, MPI_Comm communicator, int rank, reduction_type methodology) {
  struct { double value; int rank; } in_pair;
  struct { double value; int rank; } out_pair;

  char *dl = "(dl)";
  char *d = "d";
  /* ----------------------------------------------- */
  /* Set up value for call.  The rank is only used   */
  /* for MINLOC and MAXLOC calls                     */
  /* ----------------------------------------------- */
  PYCHECK( in_pair.value = PyFloat_AsDouble(message) );
  in_pair.rank = rank;

  /* ----------------------------------------------- */
  /* Use Allreduce or reduce as appropriate.  Return */
  /* a tuple for MINLOC and MAXLOC                   */
  /* ----------------------------------------------- */
  switch(methodology) {
  case REDUCE_TO_ROOT:
    COVERAGE();
    MPICHECK( communicator,
              MPI_Reduce(&in_pair,&out_pair,1,
                         (op == MPI_MINLOC || op == MPI_MAXLOC)?MPI_DOUBLE_INT:MPI_DOUBLE,
                         op,root,communicator) );
    if ( rank == root ) {
      return Py_BuildValue( (op == MPI_MINLOC || op == MPI_MAXLOC)?dl:d,
                           out_pair.value,out_pair.rank);
    } else {
      Py_INCREF(Py_None);
      return Py_None;
    }
    break;
  case REDUCE_AND_BROADCAST:
    COVERAGE();
    MPICHECK( communicator,
              MPI_Allreduce(&in_pair,&out_pair,1,
                            (op == MPI_MINLOC || op == MPI_MAXLOC)?MPI_DOUBLE_INT:MPI_DOUBLE,
                            op,communicator) );
                    
    return Py_BuildValue((op == MPI_MINLOC || op == MPI_MAXLOC)?dl:d,
                         out_pair.value,out_pair.rank);
    break;
  case REDUCE_VIA_SCAN:
    COVERAGE();
    COVERAGE();
    COVERAGE();
    MPICHECK( communicator,
              MPI_Scan(&in_pair,&out_pair,1,
                       (op == MPI_MINLOC || op == MPI_MAXLOC)?MPI_DOUBLE_INT:MPI_DOUBLE,
                       op,communicator) );
                    
    return Py_BuildValue((op == MPI_MINLOC || op == MPI_MAXLOC)?dl:d,
                         out_pair.value,out_pair.rank);
  default:
    Assert(0);
  }

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  ************ specialized_integer_reduction **********************/
/**************************************************************************/
/* This handles the 4 special kinds of integer reduction.  The kinds are  */
/* ( broadcast | no broadcast ) cross ( XXXLOC | normal reduction )       */
/**************************************************************************/
static PyObject* specialized_integer_reduction(PyObject* message, MPI_Op op, int root, MPI_Comm communicator, int rank, reduction_type methodology) {
  struct { long value; int rank; } in_pair;
  struct { long value; int rank; } out_pair;

  char *ll = "(ll)";
  char *l = "l";
  
  /* ----------------------------------------------- */
  /* Set up value for call.  The rank is only used   */
  /* for MINLOC and MAXLOC calls                     */
  /* ----------------------------------------------- */
  PYCHECK( in_pair.value = PyInt_AsLong(message) );
  in_pair.rank = rank;

  /* ----------------------------------------------- */
  /* Use Allreduce or reduce as appropriate.  Return */
  /* a tuple for MINLOC and MAXLOC                   */
  /* ----------------------------------------------- */
  switch(methodology) {
  case REDUCE_TO_ROOT:
    COVERAGE();
    MPICHECK( communicator,
              MPI_Reduce(&in_pair,&out_pair,1,
                         (op == MPI_MINLOC || op == MPI_MAXLOC)?MPI_LONG_INT:MPI_LONG,
                         op,root,communicator) );
    if ( rank == root ) {
      return Py_BuildValue((op == MPI_MINLOC || op == MPI_MAXLOC)?ll:l,
                           out_pair.value,out_pair.rank);
    } else {
      Py_INCREF(Py_None);
      return Py_None;
    }
  case REDUCE_AND_BROADCAST:
    COVERAGE();
    MPICHECK( communicator,
              MPI_Allreduce(&in_pair,&out_pair,1,
                            (op == MPI_MINLOC || op == MPI_MAXLOC)?MPI_LONG_INT:MPI_LONG,
                            op,communicator) );
                    
    return Py_BuildValue((op == MPI_MINLOC || op == MPI_MAXLOC)?ll:l,
                         out_pair.value,out_pair.rank);
  case REDUCE_VIA_SCAN:
    COVERAGE();
    COVERAGE();
    MPICHECK( communicator,
              MPI_Scan(&in_pair,&out_pair,1,
                       (op == MPI_MINLOC || op == MPI_MAXLOC)?MPI_LONG_INT:MPI_LONG,
                       op,communicator) );
                    
    return Py_BuildValue((op == MPI_MINLOC || op == MPI_MAXLOC)?ll:l,
                         out_pair.value,out_pair.rank);
  default:
    Assert(0);
  }

 pythonError:
  return 0;
}


/**************************************************************************/
/* LOCAL  **************         reduction         ************************/
/**************************************************************************/
/* This implements the reduction for BOTH mpi.reduce and mpi.allreduce    */
/* The "broadcast" flag determines if it acts like reduce or allreduce.   */
/* The basic strategy is (after checking a couple of special cases), to   */
/* bundle the data off to the root which performs a serial reduction and  */
/* then broadcasts a message back to others.                              */
/**************************************************************************/
static PyObject* reduction(PyMPI_Comm* self, PyObject* args, PyObject* kw, reduction_type methodology) {
  PyObject* message     = 0;
  PyObject* op          = 0;
  PyObject* ground      = 0;
  PyObject* data        = 0;
  PyObject* result      = 0;
  PyObject* final       = 0;
  PyObject* builtin     = 0;
  PyObject* error_type  = 0;
  PyObject* error_value = 0;
  PyObject* error_trace = 0;
  PyTypeObject* value_type  = 0;
  int treeorder = 0; /* Evaluate in normal order */
  int root = 0;
  int TAGTOUSE = 987;
  int i;
  PyObject* partial = 0;
  PyObject* new_partial = 0;
  PyObject* status = 0;
  PyObject* messageAndStatus = 0;
  MPI_Op operation;
  int need_pair_data_flag = 0; /* Only MINLOC and MAXLOC need pair data */
  static char* keys[] = {"message","operator","root","ground","type", "treeorder",NULL};


  /* ----------------------------------------------- */
  /* Get data ( note: broadcast does not need root ) */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args, kw, "OO|iOOi",keys,
                                       &message /*borrowed*/,
                                       &op      /*borrowed*/,
                                       &root    /*borrowed*/,
                                       &ground  /*borrowed*/,
                                       &value_type /*borrowed*/,
                                       &treeorder) );
  Assert(message);
  Assert(op);

  /* ----------------------------------------------- */
  /* Valid root inside communicator                  */
  /* ----------------------------------------------- */
  if ( root < 0 || root >= self->size ) {
    COVERAGE();
    PYCHECK( PyErr_Format(PyExc_ValueError,"Invalid rank for root [%d]",root) );
  }

  /* ----------------------------------------------- */
  /* Operation must be a valid MPI reduction OR a    */
  /* callable object.                                */
  /* ----------------------------------------------- */
  if ( PyInt_Check(op) ) {
    COVERAGE();
    operation = (MPI_Op)(PyInt_AS_LONG(op)); /* No need for integer type check */

    /* Special ops MINLOC and MAXLOC operate on rank/data pairs */
    if ( operation == MPI_MINLOC || operation == MPI_MAXLOC ) {
      need_pair_data_flag = 1;
    }

    /* Find the Python code that implements (also checks if legal!) */
    op /*borrowed*/ = pyMPI_reductions_python(operation); 
    if ( !op ) {
      COVERAGE();
      PYCHECK( PyErr_Format( PyExc_ValueError,
                             "Called reduction with invalid or unsupported operator [%ld].",(long)operation); );
    }

    /* ----------------------------------------------- */
    /* If we specified a type like Python float or int */
    /* then we use the built in reduction operation so */
    /* that we maintain maximum precision.             */
    /* ----------------------------------------------- */
    if ( value_type == &PyFloat_Type && !ground ) {
      COVERAGE();
      PYCHECK( result = specialized_double_reduction(message,operation,root,self->communicator,self->rank,methodology) );
      return result;
    } else if ( value_type == &PyInt_Type && !ground ) {
      COVERAGE();
      PYCHECK( result = specialized_integer_reduction(message,operation,root,self->communicator,self->rank,methodology) );
      return result;
    }
    /* Fall through on other types or null value type */

  } else if ( ! PyCallable_Check(op) ) {
    COVERAGE();
    PYCHECK( PyErr_SetString(PyExc_TypeError,"invalid reduction operator") );
  }

  /* ----------------------------------------------- */
  /* We use explicit messages for tree reduce and    */
  /* allreduce.                                      */
  /* ----------------------------------------------- */
  if (treeorder && (!ground) && (!need_pair_data_flag) && (methodology == REDUCE_TO_ROOT || methodology == REDUCE_AND_BROADCAST)) {
    int size = self->size;
    int wrappedrank = (self->rank + size - root)%size;
    unsigned mask = 1;
    PyObject* newresult = NULL;

    /* If serial case, the result is just our message */
    Py_INCREF(result = message);

    /* Shift over each level of the tree with right subtrees sending to left subtrees */
    for(mask = 1; mask < size; mask = mask << 1) {
      if (wrappedrank & mask) {
        /* I am a sender this round */
        int wrappedreceiver = wrappedrank & ~mask;
        int receiver = (wrappedreceiver+root)%size;
        PYCHECK( status /*owned*/ = pyMPI_send(self,result,receiver,77) );
        Py_DECREF(status);
        status = NULL;
        break;
      } else {
        /* I am a receiver this round */
        int wrappedsender = wrappedrank | mask;
        if (wrappedsender < size) {
          int sender = (wrappedsender+root)%size;

          PyObject* x = pyMPI_recv(self,sender,77);

          /* ----------------------------------------------- */
          /* Execute Python implementation                   */
          /* ----------------------------------------------- */
          Assert(op);
          PYCHECK( newresult /*owned*/ = PyObject_CallFunction(op,"OO", result, PyTuple_GET_ITEM(x,0)) );
          Py_DECREF(result);
          result = newresult;
          newresult = NULL;

        } else {
          /* Rank is orphaned and will send in a later round */
        }
      }
    }

    if (methodology == REDUCE_AND_BROADCAST) {
      PYCHECK( newresult = bcast(self,result,root) ); /* Get allreduce() value */
      Py_DECREF(result);
      result = newresult;
      newresult = NULL;
    } else if (self->rank != root) { /* throw away partial reductions */
      Py_DECREF(result);
      Py_INCREF(result = Py_None);
    }
    
    return result;
  }

  /* ----------------------------------------------- */
  /* We collect data on the master, run a function   */
  /* across the set, and get a result.  For the two  */
  /* built in operations MINLOC and MAXLOC, we have  */
  /* to build rank/data pairs.                       */
  /* ----------------------------------------------- */
  PYCHECK( data /*owned*/ = pyMPI_collective(self,root,message,need_pair_data_flag) );

  /* ----------------------------------------------- */
  /* Call reduce to generate result on root          */
  /* ----------------------------------------------- */
  COVERAGE();
  if ( self->rank == root ) {
    static PyObject* reduce = 0;
    COVERAGE();

    switch(methodology) {
    case REDUCE_TO_ROOT:
    case REDUCE_AND_BROADCAST:
      /* ----------------------------------------------- */
      /* We only fetch the built in reduce once....      */
      /* ----------------------------------------------- */
      if ( reduce == 0 ) {
        PYCHECK( builtin /*borrowed*/ = PyImport_ImportModule("__builtin__") );
        PYCHECK( reduce /*owned*/ = PyObject_GetAttrString(builtin,"reduce") );
      }

      /* ----------------------------------------------- */
      /* Execute Python implementation with or without   */
      /* a ground (initial) value.                       */
      /* ----------------------------------------------- */
      Assert(op);
      Assert(data);
      if ( ground ) {
        result /*owned*/ = PyObject_CallFunction(reduce,"OOO", op,data,ground);
      } else {
        result /*owned*/ = PyObject_CallFunction(reduce,"OO", op,data);
      }
      Py_DECREF(data);
      data = 0;

      /* ----------------------------------------------- */
      /* Handle errors more cleanly...                       */
      /* ----------------------------------------------- */
      if ( PyErr_Occurred() ) {
        /* Clears error flag too */
        PyErr_Fetch(&error_type/*owned*/, &error_value/*owned*/, &error_trace/*owned*/);
        result /*borrowed*/ = error_type;
        Py_INCREF(result);
      }
      break;
    case REDUCE_VIA_SCAN:
      /* Values for slaves produced below... */
      break;

    default:
      Assert(0);
    }
  }

  /* ----------------------------------------------- */
  /* Take care of the slaves' return value           */
  /* ----------------------------------------------- */
  switch (methodology) {
  case REDUCE_TO_ROOT:
    COVERAGE();
    if ( self->rank == root ) {
      final = result;
      result = 0;
    } else {
      final = Py_None;
      Py_INCREF(Py_None);
    }
    break;
  case REDUCE_AND_BROADCAST:
    COVERAGE();
    PYCHECK( final /*owned*/ = bcast(self,result,root) );
    Py_XDECREF(result);
    result  = 0;
    break;
  case REDUCE_VIA_SCAN:
    COVERAGE();
    if ( self->rank == root ) {

      /* Seed with first value (may be NULL) */
      partial = ground;
      Py_XINCREF(partial);

      for(i=0;i<self->size;++i) {
        PyObject* element = 0;

        /* Get i'th gathered value */
        element /*borrow*/ = PyTuple_GET_ITEM(data,i);

        /* Set the ground value */
        if ( partial ) {
          new_partial/*owned*/ = PyObject_CallFunctionObjArgs(op,partial,element,0);

          if ( PyErr_Occurred() ) {
            PyErr_Clear();
            new_partial/*borrowed*/ = Py_None;
            Py_INCREF(Py_None);
          }
          Py_DECREF(element);
          element = 0;
          Py_DECREF(partial);
          partial/*owned*/ = new_partial;
          new_partial = 0;
        } else {
          partial/*owned*/ = element;
          element = 0;
        }

        if ( i == root ) {
          final = partial;
          Py_INCREF(final);
        } else {
          PYCHECK( status /*owned*/ = pyMPI_send(self,partial,i,TAGTOUSE) );
          Py_XDECREF(status);
          status = NULL;
        }
      }
    } else {
      PYCHECK( messageAndStatus /*owned*/ = pyMPI_recv(self,root,TAGTOUSE) );
      Assert( PyTuple_Check(messageAndStatus) );
      PYCHECK( final /*borrowed*/ = PyTuple_GET_ITEM(messageAndStatus,0) );
      Py_INCREF(final); /*owned*/

      Py_DECREF(messageAndStatus);
      messageAndStatus = 0;
    }
    break;
  default:
    Assert(0);
  }

  /* ----------------------------------------------- */
  /* Restore deferred error                        */
  /* ----------------------------------------------- */
  if ( error_type ) {
    PyErr_Restore(error_type,error_value,error_trace);
    final = 0;
  }

  return final;
 pythonError:
  Py_XDECREF(data);
  Py_XDECREF(result);
  Py_XDECREF(status);
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_collective_barrier ************************/
/**************************************************************************/
/* Implement MPI_Barrier()                                                */
/**************************************************************************/
/* Barrier                                                                */
/*                                                                        */
/* barrier() --> None                                                     */
/*                                                                        */
/* >>> mpi.barrier()  # Block the WORLD communicator                      */
/* >>> comm.barrier()  # Block an arbitrary communicator                  */
/*                                                                        */
/* Blocks processing for this processor until all cooperative tasks on    */
/* the communicator have called MPI_Barrier().  Note that the barrier()   */
/* calls can be in different locations in the code:                       */
/*                                                                        */
/* >>> if WORLD.size%2:                                                   */
/* ...    mpi.barrier()  # odd processors block here                      */
/* ... else:                                                              */
/* ...    mpi.barrier()  # even processors block here                     */
/*                                                                        */
/* ANY invocation on a particular communicator (even in C or FORTRAN)     */
/* will release the barrier.                                              */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_barrier(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm *self = (PyMPI_Comm *) pySelf;

  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  COVERAGE();
  RAISEIFNOMPI();

  NOARGUMENTS();
  Assert(self);
  VALIDCOMMUNICATOR(self);

  MPICHECK(self->communicator,
           MPI_Barrier(self->communicator));

  Py_XINCREF(Py_None);
  return Py_None;

pythonError:
  return 0;
}

/**************************************************************************/
/* GMETHOD **************  pyMPI_collective_bcast  ************************/
/**************************************************************************/
/* MPI_bcast()                                                            */
/**************************************************************************/
/* Broadcast value to all processors.                                     */
/*                                                                        */
/* bcast(value,root=0) --> value                                          */
/*                                                                        */
/* This method messages a value from one process (the root) to all others */
/* The value is ignored on the non-root ranks.                            */
/*                                                                        */
/* >>> message = "I am from rank %d"%mpi.rank                             */
/* >>> value = bcast(message)                                             */
/* >>> print value                                                        */
/* I am from rank 0                                                       */
/* I am from rank 0                                                       */
/* I am from rank 0                                                       */
/* I am from rank 0                                                       */
/*                                                                        */
/* >>> assert WORLD.size > 1,"running in parallel"                        */
/* >>> message = "I am from rank %d"%mpi.rank                             */
/* >>> value = bcast(message,root=1)                                      */
/* >>> print value                                                        */
/* I am from rank 1                                                       */
/* I am from rank 1                                                       */
/* I am from rank 1                                                       */
/* I am from rank 1                                                       */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_bcast(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm *self = (PyMPI_Comm *) pySelf;
  PyObject*object = 0;
  int root = 0;

  /* ----------------------------------------------- */
  /* msg = bcast(msg,root)                           */
  /* ----------------------------------------------- */
  COVERAGE();
  PYCHECK(PyArg_ParseTuple(args, "|Oi", &object /* borrowed */ ,
                           &root /* borrowed */ ));
  return bcast(self, object, root);
pythonError:
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_collective_scatter ************************/
/**************************************************************************/
/* MPI_Scatter() like functionality                                       */
/**************************************************************************/
/* Scatter slices of list or sequence across processes.                   */
/*                                                                        */
/* scatter(message,root=0) --> list or None                               */
/*                                                                        */
/* On the root process (rank 0 by default), evenly scatter elements of a  */
/* list or sequence message to all processes on the communicator.         */
/*                                                                        */
/* >>> global_data = None                                                 */
/* >>> if rank == 0:                                                      */
/* ...    global_data = [11,22,33,44,...]                                 */
/* >>> local_data = scatter(global_data)                                  */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_scatter(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm *self = (PyMPI_Comm *) pySelf;
  PyObject*scatterObject = 0, *toSend = 0;
  PyObject*tempObject2 = 0;
  PyObject*status = 0;
  PyObject*result = 0;
  int i = 0;
  int start = 0;
  int end = 0;
  int root = 0;
  int object_length = 0;
  int per_process = 0;
  int extras = 0;
  static char *keys[] = {"message", "root", 0};

  /*Make sure MPI is ready */
  RAISEIFNOMPI();

  Assert(self);

  PYCHECK(PyArg_ParseTupleAndKeywords(args, kw, "O|i", keys,
                                      &scatterObject /* borrowed */ ,
                                      &root     /* borrowed */
          );
      );

  /* ----------------------------------------------- */
  /* Rank should exist on communicator...            */
  /* ----------------------------------------------- */
  if (root >= self->size || root < 0) {
    PYCHECK(PyErr_SetString(PyExc_ValueError, "Tried to call scatter on invalid root process."));
  }

  /* ----------------------------------------------- */
  /* On the root communicator, we split up the seq   */
  /* and send to the slaves...  If this fails, we    */
  /* send the error so that nobody blocks.           */
  /* ----------------------------------------------- */
  if (self->rank == root) {

    /* Get length without raising exception */
    object_length = PyObject_Size(scatterObject);
    if ( PyErr_Occurred() ) {
      object_length = 0;
      PyErr_Clear();
    }

    /* Break into parts of near equal size */
    per_process = object_length / self->size;
    extras = object_length % self->size;

    /* ----------------------------------------------- */
    /* Send a piece of the sequence from the root to   */
    /* all processes except the root doesn't send to   */
    /* itself. If we fail to get the slice, we send    */
    /* Py_None.                                        */
    /* ----------------------------------------------- */
    end = 0;
    for (i = 0; i < self->size; i++) {
      /* Get slice boundaries */
      start = end; /* Start where last left off */
      end = start + per_process;
      if ( extras ) {
        extras--;
        end++;
      }

      /* NO PYCHECK HERE, we map errors to None */
      toSend /*owned*/ = PySequence_GetSlice(scatterObject, start, end);
      if ( PyErr_Occurred() ) {
        PyErr_Clear();
        Py_INCREF(Py_None);
        toSend /*owned*/ = Py_None;
      }

      /* For the root, this is the result */
      if ( i == root ) {
        result /*owned*/ = toSend;
        toSend = 0;
      }

      /* For the slave, we message it */
      else {
        PyObject* ignore = 0;
        PYCHECK(ignore = pyMPI_send(self, toSend, i, 0));
        Py_DECREF(ignore);
        Py_DECREF(toSend);
        toSend = 0;
      }
    }
  }
  /* END ROOT */


  /* ----------------------------------------------- */
  /* The slave processor just receives the scattered */
  /* message and status value.  Note that this does  */
  /* NOT use MPI_Scatter()                           */
  /* ----------------------------------------------- */
  else {                        /* SLAVE */
    /* If this fails we can't receive and we're deadlocked */
    /* Normally, we receive a tuple (myslice,status) */
    PYCHECK(tempObject2 /*owned */  = pyMPI_recv(self, root, 0));
    if ( !PyTuple_Check(tempObject2) ) {
      PYCHECK( PyErr_SetString(PyExc_ValueError,"Internal pyMPI error, scatter recv did not return a tuple"));
    }

    /* Grab just the slice as the result */
    result = PyTuple_GET_ITEM(tempObject2,0);
    Py_INCREF(result);
    Py_XDECREF(tempObject2);    /* Deletes tuple (with status), but keeps result */

  }
  Assert(result);
  return result;

pythonError:
  Py_XDECREF(toSend);
  Py_XDECREF(tempObject2);
  return 0;
}

/**************************************************************************/
/* GMETHOD **************  pyMPI_collective_reduce  ***********************/
/**************************************************************************/
/* MPI_reduce()                                                           */
/**************************************************************************/
/* Reduces values on all processes to a single value on root process      */
/*                                                                        */
/* reduce(message,        # Local value to reduce                         */
/*        operator,       # MPI_Op or a Python function to apply          */
/*        root=0,         # Rank that gets reduced value                  */
/*        ground=None,    # Optional initial value for reduction          */
/*        type=None)      # Optional guaranteed type                      */
/* --> reduced value | None                                               */
/*                                                                        */
/* MPI defines operators for BAND BOR BXOR LAND LOR LXOR MAX MAXLOC       */
/* MIN MINLOC PROD and SUM.                                               */
/*                                                                        */
/* The operator or function is applied across the values from each rank's */
/* process.  The result is:                                               */
/*   op(op(...op(rank0_value,rank1_value),...),rankn-1_value)             */
/*                                                                        */
/* or if the optional ground value is specified,                          */
/*   op(op(op(...op(ground,rank0_value),rank1_value)...),rankn-1_value)   */
/*                                                                        */
/* The MPI implementation may have fast implementations for commutative   */
/* builtin ops. Python functions are assumed non-commutative              */
/*                                                                        */
/* >>> sum = reduce(mpi.rank, mpi.SUM)                                    */
/* >>> print sum    # (((0+1)+2)+3) on 4 processes                        */
/* 6                                                                      */
/* None                                                                   */
/* None                                                                   */
/* None                                                                   */
/*                                                                        */
/* If you guarantee the type to be a basic type (int,float) through the   */
/* optional type argument, Python will guarantee using the MPI built in   */
/* reductions to minimize precision issues.                               */
/* >>> dt = reduce(local_dt,mpi.MIN,type=float)                           */
/*                                                                        */
/* You can use a class or callable object of two arguments as a operator  */
/* (within the limits imposed by pickling)                                */
/* >>> print reduce(str(mpi.rank), os.path.join)                          */
/* 0/1/2/3                                                                */
/* None                                                                   */
/* None                                                                   */
/* None                                                                   */
/*                                                                        */
/* You can specify reductions to go in "TREE" order to get better         */
/* memory balance (and possible speed improvements)                       */
/* >>> mysum = allreduce(mpi.rank,mpi.SUM,treeorder=True)                 */
/* >>> print 'mysum is',mysum                                             */
/* 6                                                                      */
/* None                                                                   */
/* None                                                                   */
/* None                                                                   */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_reduce(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  COVERAGE();
  return reduction(self, args, kw, REDUCE_TO_ROOT);
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_collective_allreduce **********************/
/**************************************************************************/
/* MPI_allreduce()                                                        */
/**************************************************************************/
/* Reduces values on all processes to the same value across all processes */
/* allreduce(message,        # Local value to reduce                      */
/*        operator,       # MPI_Op or a Python function to apply          */
/*        root=0,         # Not applicable in allreduce                   */
/*        ground=None,    # Optional initial value for reduction          */
/*        type=None)      # Optional guaranteed type                      */
/* --> reduced value                                                      */
/*                                                                        */
/* MPI defines operators for BAND BOR BXOR LAND LOR LXOR MAX MAXLOC       */
/* MIN MINLOC PROD and SUM.                                               */
/*                                                                        */
/* See reduce() for more information.  Allreduce is like reduce(), but    */
/* the result is broadcast to all ranks across a communicator.            */
/*                                                                        */
/* >>> dt = allreduce(local_dt,mpi.MIN)                                   */
/* >>> print dt                                                           */
/* 1.125                                                                  */
/* 1.125                                                                  */
/* 1.125                                                                  */
/* 1.125                                                                  */
/*                                                                        */
/* You can specify reductions to go in "TREE" order to get better         */
/* memory balance (and possible speed improvements)                       */
/*                                                                        */
/* >>> mysum = allreduce(mpi.rank,mpi.SUM,treeorder=True)                 */
/* 6                                                                      */
/* 6                                                                      */
/* 6                                                                      */
/* 6                                                                      */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_allreduce(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  COVERAGE();
  return reduction(self, args, kw, REDUCE_AND_BROADCAST);
}

/**************************************************************************/
/* LOCAL  **************  specialized_numpy_gather ************************/
/**************************************************************************/
/* Do the gather on numpy arrays                                          */
/**************************************************************************/
static PyObject* specialized_numpy_gather(PyMPI_Comm* self, PyObject* gatherObject, long count,int root, int do_allgather) {
  void* data;
  Py_ssize_t actual_count;
  MPI_Datatype mpitype;
  int numpytype;
  PyObject* result = NULL;
  void* target = NULL;

  /* ----------------------------------------------- */
  /* We better have a numpy array here.  We do the   */
  /* checks in pyMPI_util since it loaded the API    */
  /* ----------------------------------------------- */
  PYCHECK( pyMPI_crack_numpy_object(gatherObject,&data,&actual_count,&mpitype,&numpytype) );
  if (count < 0) count = actual_count;

  /* ----------------------------------------------- */
  /* Build the target object array                   */
  /* ----------------------------------------------- */
  if (do_allgather || self->rank == root) {
    result = pyMPI_make_numpy(count * self->size,numpytype,&target);
  } else {
    Py_INCREF( result = Py_None );
  }

  /* ----------------------------------------------- */
  /* Do an allgather or gather as appropriate        */
  /* ----------------------------------------------- */
  if (do_allgather) {
    MPICHECK(self->communicator,
             MPI_Allgather(data,count,mpitype, target,count,mpitype, self->communicator));
  } else {
    MPICHECK(self->communicator,
             MPI_Gather(data,count,mpitype, target,count,mpitype, root,self->communicator));
  }

  return result;
  
 pythonError:
    Py_XDECREF(result);
  return NULL;
}


/**************************************************************************/
/* GMETHOD **************   pyMPI_collective_scan   ***********************/
/**************************************************************************/
/* Will implement MPI_Scan()                                              */
/**************************************************************************/
/* Return a running sum, product, etc... across multiple processes        */
/*                                                                        */
/* scan(message,operation) --> varies by message and operation            */
/*                                                                        */
/* Scan is similar to allreduce, except it returns the partial results as */
/* well as the local value.  A process of rank i, will receive a list of  */
/* i values representing the sub-values on ranks 0, 1, 2, ... i           */
/*                                                                        */
/* >>> assert 0,"TODO: Not implemented yet"                               */
/* >>> print scan(rank,mpi.SUM)                                           */
/* 0                                                                      */
/* 1                                                                      */
/* 3                                                                      */
/* 6                                                                      */
/*                                                                        */
/* That is (0,0+1,0+1+2,0+1+2+3) on 4 processors                          */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_scan(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  COVERAGE();
  return reduction(self, args, kw, REDUCE_VIA_SCAN);
}

/**************************************************************************/
/* GMETHOD **************  pyMPI_collective_gather  ***********************/
/**************************************************************************/
/* This is MPI_gather() like                                              */
/**************************************************************************/
/* Gather items from across the communicator                              */
/*                                                                        */
/* gather(message,             # Some sequence object                     */
/*        count=len(message),  # Number of objects to send                */
/*        root=0,              # Rank that gets the list                  */
/*        native=False)        # Assume numpy arrays                      */
/* --> [ list ] | None                                                    */
/*                                                                        */
/* Gather sub-lists to the root process.  The result is a concatenation   */
/* of the sub-lists on the root process and None on the others.  If the   */
/* native flag is set to True, then assume all objects are numpy arrays.  */
/* The result is then a numpy array instead of a list.                    */
/*                                                                        */
/* >>> print mpi.gather([WORLD.rank])                                     */
/* [0,1,2,3]                                                              */
/* None                                                                   */
/* None                                                                   */
/* None                                                                   */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_gather(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm *self = (PyMPI_Comm *) pySelf;
  PyObject* gatherObject = 0;
  Py_ssize_t count = -1;
  int objectSize = 0;
  int native = 0;

  PyObject* gatherList = 0;
  PyObject* tempObject = 0;
  PyObject* result = 0;
  PyObject* catenation = 0;
  PyObject* temporary = 0;
  int i = 0;
  int root = 0;
  static char *keys[] = {"message", "count", "root", "native", 0};
  PyMPI_Message* all = 0;
  char* secondMessages = 0;
  PyMPI_Message myHeader;
  char* myBody = 0;
  PyObject* element = NULL;
  PyObject* newresult = NULL;
  int* recvCounts = NULL;
  int* displacements = NULL;
  char* data = NULL;
  PyObject* nth = NULL;

  /*Make sure MPI is ready */
  RAISEIFNOMPI();

  if (!self) {
    PYCHECK(PyErr_SetString(PyExc_SystemError,
                            "Called gather with invalid Self object.");
        );
  }
  PYCHECK(PyArg_ParseTupleAndKeywords(args, kw, "O|nii", keys,
                                      &gatherObject /* borrowed */ ,
                                      &count,
                                      &root,
                                      &native) );

  /* ----------------------------------------------- */
  /* Special handling when type is set               */
  /* ----------------------------------------------- */
  if (native) {
#ifdef PYMPI_HAS_NUMPY
    return specialized_numpy_gather(self,gatherObject,count,root,0);
#else
    PyErr_SetString(PyExc_RuntimeError,"special native option used, but numpy not available");
    return NULL;
#endif
  }

  /* ----------------------------------------------- */
  /* Only gather over sequences                      */
  /* ----------------------------------------------- */
  Assert(gatherObject);
  PYCHECK(gatherList /*owned*/ = sequence(gatherObject));
  Assert(PySequence_Check(gatherList));

  /* ----------------------------------------------- */
  /* We may default the count to the object size     */
  /* ----------------------------------------------- */
  PYCHECK(objectSize = PySequence_Size(gatherList));
  if (count < 0) {
    count = objectSize;
  }
  Assert(count >= 0);

  /* ----------------------------------------------- */
  /* Valid root inside communicator                */
  /* ----------------------------------------------- */
  if (root < 0 || root >= self->size) {
    PYCHECK(PyErr_SetString(PyExc_ValueError, "Invalid root"));
  }

  /* ----------------------------------------------- */
  /* Make sure gatherList has enough elements      */
  /* ----------------------------------------------- */
  if (objectSize < count) {
    PYCHECK(PyErr_SetString(PyExc_RuntimeError,
                       "Called gather with too-short sequence object\n"););
  }

  /* ----------------------------------------------- */
  /* If too big, cut it down as a slice              */
  /* ----------------------------------------------- */
  else if (objectSize > count) {
    PYCHECK(temporary /*owned */  = PySequence_GetSlice(gatherList, 0, count));
    Py_DECREF(gatherList);
    gatherList = temporary;
    temporary = NULL;
  }

  /* ----------------------------------------------- */
  /* First, we grab an array of the "eager" block    */
  /* ----------------------------------------------- */
  if (self->rank == root) all = (PyMPI_Message*)malloc(sizeof(PyMPI_Message)*self->size);
  PYCHECK( pyMPI_pack(gatherList,self,&myHeader,&myBody) );
  MPICHECK( self->communicator,
            MPI_Gather(&myHeader,sizeof(myHeader),MPI_BYTE,
                       all,sizeof(myHeader),MPI_BYTE,
                       root,self->communicator) );

  /* ----------------------------------------------- */
  /* On the root, we gather the 2nd messages and     */
  /* build our new list elements.                    */
  /* ----------------------------------------------- */
  if (self->rank == root) {
    int total_bytes = 0;

    /* ---------------------------------------------------------------- */
    /* We need to know all the message sizes before the gatherv() below */
    /* ---------------------------------------------------------------- */
    recvCounts = (int*)malloc(self->size*sizeof(int));
    displacements = (int*)malloc(self->size*sizeof(int));
    for(i=0;i<self->size;++i) {
      int nbytes = all[i].header.bytes_in_second_message;
      recvCounts[i] = nbytes;
      displacements[i] = total_bytes;
      total_bytes += nbytes;
    }

    /* ---------------------------------------------------------------- */
    /* Create a buffer to hold all those second messages and get them   */
    /* ---------------------------------------------------------------- */
    secondMessages = (char*)malloc(total_bytes);
    MPICHECK(self->communicator,
             MPI_Gatherv(myBody,myHeader.header.bytes_in_second_message,MPI_BYTE,
                         secondMessages,recvCounts,displacements,MPI_BYTE,
                         root,self->communicator));

    /* ---------------------------------------------------------------- */
    /* Now, unpack each message into a local pyobject and catenate      */
    /* ---------------------------------------------------------------- */
    PYCHECK( catenation /*owned*/ = PyList_New(0) );
    for(i=0;i<self->size;++i) {

      /* ---------------------------------------------------------------- */
      /* We need to pass in a malloc'd pointer for any 2nd message        */
      /* because pyMPI_unpack() is free to hold it or delete it.          */
      /* ---------------------------------------------------------------- */
      if (recvCounts[i]) {
        // We need to make a copy!
        data /*owned*/ = malloc(recvCounts[i]);
        memcpy(data,secondMessages+displacements[i],recvCounts[i]);
      } else {
        data = NULL;
      }
      
      PYCHECK( nth /*owned*/= pyMPI_unpack(all+i,&data) );

      if (data) { free(data); data = NULL; }

      PYCHECK( temporary = PySequence_Concat(catenation,nth) );
      Py_DECREF(catenation); catenation = NULL;
      Py_DECREF(nth); nth = NULL;
      catenation = temporary;
    }

    /* OK, since we survived all the catenations, we can commit this answer */
    result = catenation;
    catenation = NULL;
    
  } else {
    /* ---------------------------------------------------------------- */
    /* The slaves have to send their second messages (even if they are  */
    /* size zero since some other proc may have a non-zero one)         */
    /* The result here is always "None"                                 */
    /* ---------------------------------------------------------------- */
    MPICHECK(self->communicator,
             MPI_Gatherv(myBody,myHeader.header.bytes_in_second_message,MPI_BYTE,
                         NULL,NULL,NULL,MPI_BYTE,
                         root,self->communicator));
    Py_INCREF( result = Py_None );
  }


  /* fall through */
pythonError:
  Py_XDECREF(gatherList);
  Py_XDECREF(element);
  Py_XDECREF(catenation);
  Py_XDECREF(nth);
  pyMPI_message_free(&myHeader,&myBody);
  if (data) free(data);
  if (all) free(all);
  if (secondMessages) free(secondMessages);
  if (recvCounts) free(recvCounts);
  if (displacements) free(displacements);

  return result;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_collective_allgather **********************/
/**************************************************************************/
/* MPI_Allgather()                                                        */
/**************************************************************************/
/* Gather items from across the communicator                              */
/*                                                                        */
/* allgather(message,             # Some sequence object                  */
/*           count=len(message),  # Number of objects to send             */
/*           root=0,              # Rank that gets the list               */
/*           native=False)        # Assume numpy arrays                   */
/* --> [ list ] | None                                                    */
/*                                                                        */
/* Gather sub-lists to the root process.  The result is a concatenation   */
/* of the sub-lists on all processes (unlike gather which only gathers to */
/* the root). When "native" is true, assume that all arrays are numpy     */
/* objects.  The result is then a numpy object.                           */
/*                                                                        */
/* >>> print mpi.allgather([WORLD.rank])                                  */
/* [0,1,2,3]                                                              */
/* [0,1,2,3]                                                              */
/* [0,1,2,3]                                                              */
/* [0,1,2,3]                                                              */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_collective_allgather(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm *self = (PyMPI_Comm *) pySelf;
  PyObject* gatherObject = 0;
  Py_ssize_t count = -1;
  int native = 0;

  PyObject* tempObject = 0;
  PyObject* tempResult = 0;
  PyObject*finalResult = 0;
  static char *keys[] = {"message", "count", "native",0};

  PYCHECK(
           PyArg_ParseTupleAndKeywords(args, kw, "O|ni", keys,
                                       &gatherObject, /* borrowed */
                                       &count,
                                       &native);
      );

  /* ----------------------------------------------- */
  /* Special handling when type is set               */
  /* ----------------------------------------------- */
  if (native) {
#ifdef PYMPI_HAS_NUMPY
    return specialized_numpy_gather(self,gatherObject,count,0,1);
#else
    PyErr_SetString(PyExc_RuntimeError,"special native option used, but numpy not available");
    return NULL;
#endif
  }

  /* ----------------------------------------------- */
  /* We may default the count to the object size     */
  /* ----------------------------------------------- */
  if (count < 0) {
    PYCHECK(count = PyObject_Size(gatherObject));
  }
  /*Start by gathering onto process zero */
  PYCHECK(
  /* owned */ tempObject = Py_BuildValue("Oi", gatherObject, count));

  /*Even if this fails we must broadcast to the other processes */
  tempResult/* owned */ = pyMPI_collective_gather(pySelf, args, kw);

  if (tempResult == 0) {
    tempResult = Py_None;
    Py_INCREF(tempResult);
  }
  Py_XDECREF(tempObject);

  /*Now process zero broadcasts to all others */
  /*Do NOT PYCHECK the build value statement because then a previous */
  /*exception could prevent the broadcast from occuring */
  /* owned */ tempObject = Py_BuildValue("Oi", tempResult, 0);
  if (tempObject == 0) {
    tempObject = Py_None;
    Py_INCREF(tempObject);
  }
  PYCHECK(finalResult = pyMPI_collective_bcast(pySelf, tempObject,0));

  /* Fall through for cleanup code even when result is good */
pythonError:
  Py_XDECREF(tempObject);
  if (tempResult != Py_None) {
    Py_XDECREF(tempResult);
  }
  if (finalResult == Py_None) {
    /*This will set error messages on the non-zero processes */
    /*The zero process should have a meaningful error message already */
    if (!PyErr_Occurred())
      PyErr_SetString(PyExc_SystemError,
                      "allGather failed for unknown reason");
    finalResult = 0;
  }
  return finalResult;
}

END_CPLUSPLUS

