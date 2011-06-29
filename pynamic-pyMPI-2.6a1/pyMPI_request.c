/**************************************************************************/
/* FILE   **************      pyMPI_request.c      ************************/
/**************************************************************************/
/* Author: Chris Murray                                                   */
/* Author: Patrick Miller May 22 2002                                     */
/**************************************************************************/
/*                                                                        */
/* >>> import mpi                                                         */
/* >>> from mpi import *                                                  */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************     abandoned_requests    ************************/
/**************************************************************************/
/* This is a list of non-cancelable send requests */
/**************************************************************************/
static PyMPI_Request* abandoned_requests = 0;

/**************************************************************************/
/* LOCAL  **************   request_second_message  ************************/
/**************************************************************************/
/* Handle short message and request second part if not already requested  */
/**************************************************************************/
static void request_second_message(PyMPI_Request* self) {

  assert(self);
  if (self->postedMessage) return;

  if ( self->firstBufMsg.header.bytes_in_second_message == 0 ) {
    PYCHECK( self->postedMessage = pyMPI_unpack(&self->firstBufMsg,0) );
  } else {
    if ( !self->buffer ) {
      self->buffer = (char*)malloc(self->firstBufMsg.header.bytes_in_second_message);
      assert(self->buffer);
      self->firstBufMsg.header.free_buffer = 1; /* This malloc'd space needs to be freed */

      MPICHECK(self->communicator,
               MPI_Irecv(self->buffer,
                         self->firstBufMsg.header.bytes_in_second_message,
                         MPI_BYTE, self->status.MPI_SOURCE, self->status.MPI_TAG,
                         self->communicator, &self->bufferRequest));
    }
  }
  return;

 pythonError:
  pyMPI_message_free(&self->firstBufMsg,&self->buffer);
  return;
}

/**************************************************************************/
/* METHOD **************        request_init       ************************/
/**************************************************************************/
/* Initialization of request objects                                      */
/**************************************************************************/
/* Fill in the request object                                             */
/**************************************************************************/
static int request_init(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  COVERAGE();
  Assert(self);
  return 0;
}
 
/**************************************************************************/
/* LOCAL  **************        request_test       ************************/
/**************************************************************************/
/* Purpose: Test to see if a nonblocking operation has finished           */
/* NOTE: edited by Chris to work with new message model                   */
/*                                                                        */
/* Note: From MPICH man page                                              */
/*                                                                        */
/* "For send operations, the only use of status is for MPI_Test_cancelled */
/*  or in the case that there is an error, in which case the MPI_ERROR    */
/*  field of status will be set."                                         */
/*                                                                        */
/* Returns: tuple (bool HasOperationFinished?,status)                     */
/*                                                                        */
/* Note: return value is dynamically allocated so it must be DECREFed     */
/*       when not in use                                                  */
/**************************************************************************/
static PyObject* request_test(PyMPI_Request* pySelf  ) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;
  PyObject* result = 0;
  int isReady = 0;
  PyObject* pyReady = 0;


  /* ----------------------------------------------- */
  /* Deal with send objects                          */
  /* ----------------------------------------------- */
  if (self->iAmASendObject != 0) {

    /* for right now lets just check the bufferRequest */
    MPICHECK(self->communicator,
             MPI_Test(&self->bufferRequest,
                      &isReady,
                      &self->status));

    PYCHECK( pyReady/*owned*/ = PyInt_FromLong(isReady) );
    PYCHECK( result/*owned*/ = pyMPI_resultstatus(/*consumes*/pyReady,self->status) );
    return result;
  }

  /* ----------------------------------------------- */
  /* For recv, maybe we already received it...       */
  /* ----------------------------------------------- */
  if (self->postedMessage != 0) {
    PYCHECK( pyReady/*owned*/ = PyInt_FromLong(1) );
    PYCHECK(result /*owned*/ = pyMPI_resultstatus(/*consumes*/pyReady,self->status));
    pyReady = 0;
    return result;
  }

  /* ----------------------------------------------- */
  /* See if first part request has arrived           */
  /* ----------------------------------------------- */
  MPICHECK(self->communicator,
           MPI_Test(&self->descriptionRequest,
                    &isReady,
                    &self->status));
    
  /* Not here yet... */
  if ( !isReady ) {
    PYCHECK( pyReady/*owned*/ = PyInt_FromLong(0) );
    PYCHECK(result/*owned*/ = pyMPI_resultstatus(/*consumes*/pyReady,self->status));
    pyReady = 0;
    return result;
  }

  /* ----------------------------------------------- */
  /* Do we have everything?                          */
  /* ----------------------------------------------- */
  PYCHECK( request_second_message(self) );
  if ( self->postedMessage ) {
    PYCHECK( pyReady/*owned*/ = PyInt_FromLong(1) );
    PYCHECK(result/*owned*/ = pyMPI_resultstatus(/*consumes*/pyReady,self->status));
    pyReady = 0;
    return result;
  }

  /* ----------------------------------------------- */
  /* OK, the first part is here, but not second      */
  /* ----------------------------------------------- */
  MPICHECK(self->communicator,
           MPI_Test(&self->bufferRequest,
                    &isReady,
                    &self->status));

  if (isReady) {
    PYCHECK( self->postedMessage = pyMPI_unpack(&self->firstBufMsg,&self->buffer) );
  }

  PYCHECK( pyReady/*owned*/ = PyInt_FromLong(isReady) );
  PYCHECK( result/*owned*/ = pyMPI_resultstatus(/*consumes*/pyReady,self->status) );

  return result;
 pythonError:
  Py_XDECREF(result);
  Py_XDECREF(pyReady);
  return 0;
}

/**************************************************************************/
/* METHOD  *************        request_wait        ***********************/
/**************************************************************************/
/* Purpose: wait for the pending operation to complete.                   */
/*                                                                        */
/* Note: From MPICH man page:                                             */
/* "For send operations, the only use of status is for MPI_Test_cancelled */
/*  or in the case that there is an error, in which case the MPI_ERROR    */
/*  field of status will be set."                                         */
/*                                                                        */
/*  On receives, will block and wait for description (if not already      */
/*  received) and data to arrive.                                         */
/*                                                                        */
/*  Return: Status object of request (always)                             */
/**************************************************************************/
/* wait()                                                                 */
/*                                                                        */
/* Wait for a request to complete                                         */
/*                                                                        */
/* >>> req = mpi.isend('hello',0)                                         */
/* >>> req.wait()                                                         */
/* >>> print 'send completed'                                             */
/*                                                                        */
/**************************************************************************/
static PyObject* request_wait(PyObject* pySelf, PyObject* args) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;
  PyObject* result = 0;

  COVERAGE();
  NOARGUMENTS();


  /* ----------------------------------------------- */
  /* Deal with send objects by waiting on first part */
  /* and second part                                 */
  /* ----------------------------------------------- */
  if ( self->iAmASendObject) {
    if ( self->descriptionRequest != MPI_REQUEST_NULL ) {
      MPICHECK(self->communicator, MPI_Wait(&self->descriptionRequest, &self->status));
    }
    if ( self->bufferRequest != MPI_REQUEST_NULL ) {
      MPICHECK(self->communicator, MPI_Wait( &self->bufferRequest, &self->status ));
    }
    PYCHECK(result = pyMPI_status(self->status));
    return result;
  }

  /* ----------------------------------------------- */
  /* If the object is here, nothing to wait on...    */
  /* ----------------------------------------------- */
  if ( self->postedMessage ) {
    PYCHECK( result = pyMPI_status(self->status) );
    return result;
  }

  /* ----------------------------------------------- */
  /* Wait on the first part (may already be here)    */
  /* ----------------------------------------------- */
  if ( self->descriptionRequest != MPI_REQUEST_NULL ) {
    MPICHECK(self->communicator, MPI_Wait(&self->descriptionRequest, &self->status));
  }
  
  /* ----------------------------------------------- */
  /* Do we have everything? (won't ask more than     */
  /* once).                                          */
  /* ----------------------------------------------- */
  PYCHECK( request_second_message(self) );
  if ( self->postedMessage ) {
    PYCHECK( result = pyMPI_status(self->status) );
    return result;
  }

  /* ----------------------------------------------- */
  /* Wait on the second part                         */
  /* ----------------------------------------------- */
  if ( self->bufferRequest != MPI_REQUEST_NULL ) {
    MPICHECK(self->communicator, MPI_Wait( &self->bufferRequest, &self->status ));
  }

  /* ----------------------------------------------- */
  /* Convert back to python object                   */
  /* ----------------------------------------------- */
  PYCHECK( self->postedMessage = pyMPI_unpack(&self->firstBufMsg,&self->buffer) );

  PYCHECK(result = pyMPI_status(self->status));
  return result;

 pythonError:
  Py_XDECREF(result);
  return 0;
}

/**************************************************************************/
/* LOCAL  **************      request_dealloc      ************************/
/**************************************************************************/
/* Destroy a pyMPI request object.  We may be able to cancel the two      */
/* internal requests (for receives), but for sends we must put them in a  */
/* queue until we get a chance to actually kill them after the message    */
/* sent.                                                                  */
/**************************************************************************/
static void request_dealloc(PyObject* pySelf) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  /* If we have an unallocated object, we can just continue */
  if ( self->descriptionRequest == MPI_REQUEST_NULL &&
       self->buffer == 0 &&
       self->bufferRequest == MPI_REQUEST_NULL ) {
    /* TODO: Release python storage */
    return; /* No work to do */
  }

  /* ----------------------------------------------- */
  /* Either cancel or wait for request               */
  /* ----------------------------------------------- */
  if ( self->iAmASendObject ) {
    self->abandoned_link = abandoned_requests;
    abandoned_requests = self;
    return;
  } else {
    /* On recv, if we don't have a message, we need to cancel */
    if ( !self->postedMessage ) {
      MPI_Cancel(&self->descriptionRequest);
      if ( self->bufferRequest != MPI_REQUEST_NULL ) {
        MPI_Cancel(&self->bufferRequest);
      }
    }
  }

  pyMPI_message_free(&self->firstBufMsg,&self->buffer);

  Py_XDECREF(self->postedMessage);
}


/**************************************************************************/
/* METHOD   *************       request_cancel      ***********************/
/**************************************************************************/
/* Cancel requests                                                        */
/**************************************************************************/
/* Cancel a receive request.                                              */
/*                                                                        */
/* cancel()                                                               */
/*                                                                        */
/* Note: From MPICH man page:                                             */
/*                                                                        */
/* "Cancel has only been implemented for receive requests; it is a no-op  */
/*  for send requests.  The primary expected use of MPI_Cancel is in multi*/
/*  buffering schemes, where speculative MPI_Irecvs are made.  When the   */
/*  computation completes, some of these receive requests may remain;     */
/*  using MPI_Cancel allows the user to cancel these unsatisfied requests.*/
/*                                                                        */
/* >>> req = mpi.irecv(0)                                                 */
/* >>> req.cancel()                                                       */
/*                                                                        */
/**************************************************************************/
static PyObject* request_cancel(PyObject* pySelf, PyObject* args) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  COVERAGE();
  NOARGUMENTS();

  if (self->iAmASendObject != 0) {
#ifndef PYMPI_BADCANCEL
    MPICHECK(self->communicator, MPI_Cancel(&self->descriptionRequest));
    if ( self->bufferRequest != MPI_REQUEST_NULL ) {
      MPICHECK(self->communicator, MPI_Cancel(&self->bufferRequest));
    }
#endif
  } else {
    /* ----------------------------------------------- */
    /* if buffer has been allocated, description has   */
    /* been received and we don't want to cancel a      */
    /* request we have already received.               */
    /* ----------------------------------------------- */
    if (self->buffer == 0) {
      MPICHECK(self->communicator, MPI_Cancel(&self->descriptionRequest));
    }
    /* NOTE: I don't know what happens when you call cancel on a receive
     * request that has already come in :-/   - MC
     */
    if ( self->bufferRequest != MPI_REQUEST_NULL ) {
      MPICHECK(self->communicator, MPI_Cancel(&self->bufferRequest));
    }
  }

  Py_INCREF(Py_None);
  return Py_None;

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************       requestMethods      ************************/
/**************************************************************************/
/* Requests have only two simple methods.                                 */
/**************************************************************************/
static PyMethodDef requestMethods[] = 
{
  {"wait",request_wait,METH_VARARGS,PYMPI_REQUEST_WAIT_DOC},
  {"cancel",request_cancel,METH_VARARGS,PYMPI_REQUEST_CANCEL_DOC},
  {0,0}
};



/**************************************************************************/
/* LOCAL  **************      request_getattr      ************************/
/**************************************************************************/
/* TODO: Convert request.getattr() to a new style type with properties    */
/**************************************************************************/
static PyObject* request_getattr(PyObject* pySelf, char* name) 
{
  PyMPI_Request* self = (PyMPI_Request*)pySelf;
  PyObject* attr = 0;
  MPI_Status status;
  int flag;
  PyObject* testresult = 0;


  COVERAGE();

  status.MPI_SOURCE = 0;
  status.MPI_TAG = 0;
  status.MPI_ERROR = 0;

  Assert(self);
  if (strcmp(name,"ready") == 0){
    COVERAGE();

    /* if message is ready return 1, otherwise return 0 */      
    if ( self->iAmASendObject ) {
      COVERAGE();
      PyErr_SetString(PyExc_AttributeError,"No ready on send request.");
    } else {
      COVERAGE();
      PYCHECK( testresult = request_test(self) );
      Py_DECREF(testresult);
      PYCHECK( attr = PyInt_FromLong(self->postedMessage != 0) );
    }

  } else if ( strcmp(name,"message") == 0 ) {
    COVERAGE();

    /* If we have message, return reference, otherwise wait for it */
    if ( self->postedMessage ) {
      COVERAGE();
      Py_INCREF(self->postedMessage);
      attr = self->postedMessage;
    } else {
      PyObject* wait = 0;
      COVERAGE();

      PYCHECK( wait = request_wait((PyObject*) self,0) );
      Py_XDECREF(wait);
      wait = 0;
      /* Now the message should be there */
      Assert(self->postedMessage);
      Py_INCREF(self->postedMessage);
      attr = self->postedMessage;
    }
  } else if ( strcmp(name,"test") == 0 ) {
    COVERAGE();

    attr = request_test(self);
  } else if ( strcmp(name,"status") == 0 ) {
    if ( self->postedMessage ) {
      COVERAGE();
      PYCHECK( attr = pyMPI_status(self->status) );
    } else {
      PyObject* wait = 0;
      COVERAGE();

      PYCHECK( wait = request_wait((PyObject*) self,0) );
      Py_XDECREF(wait);
      wait = 0;
      /* Now the message should be there */
      Assert(self->postedMessage);
      PYCHECK( attr = pyMPI_status(self->status) );
    }
  } else if ( strcmp(name,"test_cancelled") == 0){
    COVERAGE();

    MPICHECK(self->communicator,
             MPI_Test(&self->bufferRequest,&flag,&status));
    MPICHECK(self->communicator,
             MPI_Test_cancelled(&status,&flag));
    PYCHECK(attr = PyInt_FromLong(flag));
  } else {
    COVERAGE();

    PYCHECK( attr = Py_FindMethod(requestMethods,pySelf,name) );
  }

  return attr;

 pythonError:
  Py_XDECREF(attr);
  return 0;
}

/**************************************************************************/
/* METHOD **************     request_get_ready     ************************/
/**************************************************************************/
/* TODO: Not implemented                                                  */
/**************************************************************************/
/* .ready                                                                 */
/*                                                                        */
/* Attribute is true iff the message is ready.  Does not block            */
/*                                                                        */
/* >>> req = mpi.irecv()                                                  */
/* >>> if req.ready: print 'ready'                                        */
/*                                                                        */
/**************************************************************************/
static PyObject* request_get_ready(PyObject* pySelf, void* closure) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  Assert(self);
  COVERAGE();

  PyErr_SetString(PyExc_NotImplementedError,"Not implemented");
  return 0;
}
/**************************************************************************/
/* METHOD **************    request_get_message    ************************/
/**************************************************************************/
/* TODO: Not implemented                                                  */
/**************************************************************************/
/* .message                                                               */
/*                                                                        */
/* Attribute holding message (for recv).  Will block if not ready.        */
/*                                                                        */
/* >>> req = mpi.irecv()                                                  */
/* >>> if req.ready: print req.message                                    */
/*                                                                        */
/**************************************************************************/
static PyObject* request_get_message(PyObject* pySelf, void* closure) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  Assert(self);
  COVERAGE();

  PyErr_SetString(PyExc_NotImplementedError,"Not implemented");
  return 0;
}
/**************************************************************************/
/* METHOD **************     request_get_status    ************************/
/**************************************************************************/
/* TODO: Not implemented                                                  */
/**************************************************************************/
/* .status                                                                */
/*                                                                        */
/* Receive the status object related to message.  Will block if not ready */
/*                                                                        */
/* >>> req = mpi.irecv()                                                  */
/* >>> if req.ready: print req.status                                     */
/*                                                                        */
/**************************************************************************/
static PyObject* request_get_status(PyObject* pySelf, void* closure) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  Assert(self);
  COVERAGE();

  PyErr_SetString(PyExc_NotImplementedError,"Not implemented");
  return 0;
}

/**************************************************************************/
/* METHOD **************      request_get_test     ************************/
/**************************************************************************/
/* TODO: Not implemented                                                  */
/**************************************************************************/
/* .test                                                                  */
/*                                                                        */
/* Returns a boolean indicating if a message is the action is complete    */
/* This test does not block.                                              */
/**************************************************************************/
static PyObject* request_get_test(PyObject* pySelf, void* closure) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  Assert(self);
  COVERAGE();

  PyErr_SetString(PyExc_NotImplementedError,"Not implemented");
  return 0;
}

/**************************************************************************/
/* METHOD ************** request_get_test_cancelled ***********************/
/**************************************************************************/
/* TODO: Not implemented                                                  */
/**************************************************************************/
/* .canceled                                                              */
/*                                                                        */
/* True if this message was cancelled.  Does not block.                   */
/**************************************************************************/
static PyObject* request_get_test_cancelled(PyObject* pySelf, void* closure) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;

  Assert(self);
  COVERAGE();

  PyErr_SetString(PyExc_NotImplementedError,"Not implemented");
  return 0;
}

/**************************************************************************/
/* LOCAL  **************       request_getset      ************************/
/**************************************************************************/
/* Here we define property attributes for request objects                 */
/**************************************************************************/
static PyGetSetDef request_getset[] = {
  {"ready",request_get_ready,0,PYMPI_REQUEST_GET_READY_DOC,0},
  {"message",request_get_message,0,PYMPI_REQUEST_GET_MESSAGE_DOC,0},
  {"status",request_get_status,0,PYMPI_REQUEST_GET_STATUS_DOC,0},
  {"test",request_get_test,0,PYMPI_REQUEST_GET_TEST_DOC,0},
  {"test_cancelled",request_get_test_cancelled,0,PYMPI_REQUEST_GET_TEST_CANCELLED_DOC,0},
  {0,0}
};

/**************************************************************************/
/* LOCAL  **************      request_nonzero      ************************/
/**************************************************************************/
/* Return true if message is ready (used in things like >>> if req: ...   */
/**************************************************************************/
static int request_nonzero(PyObject* pySelf) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;
  PyObject* result = 0;

  /* ----------------------------------------------- */
  /* If the message is here, then true               */
  /* ----------------------------------------------- */
  if ( self->postedMessage ) return 1;

  /* ----------------------------------------------- */
  /* Otherwise, poll the message once and then check */
  /* ----------------------------------------------- */
  PYCHECK( result = request_test(self) );
  Py_DECREF(result);
  result = 0;

  return (self->postedMessage != 0);

pythonError:
  return -1;
}

/**************************************************************************/
/* LOCAL  **************       request_number      ************************/
/**************************************************************************/
/* This table hooks the "nonzero" test into the type                      */
/**************************************************************************/
static PyNumberMethods request_number = {
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
  request_nonzero, /* inquiry nb_nonzero; */
  0, /* unaryfunc nb_invert; */
  0, /* binaryfunc nb_lshift; */
  0, /* binaryfunc nb_rshift; */
  0, /* binaryfunc nb_and; */
  0, /* binaryfunc nb_xor; */
  0, /* binaryfunc nb_or; */
  0, /* coercion nb_coerce; */
  0, /* unaryfunc nb_int; */
  0, /* unaryfunc nb_long; */
  0, /* unaryfunc nb_float; */
  0, /* unaryfunc nb_oct; */
  0, /* unaryfunc nb_hex; */
};

/**************************************************************************/
/* LOCAL  **************       request_string      ************************/
/**************************************************************************/
/* Render request as a string for <repr> and <str>                        */
/**************************************************************************/
static PyObject* request_string(PyObject* pySelf) {
  PyMPI_Request* self = (PyMPI_Request*)pySelf;
  PyObject* result = 0;

  Assert(self);

  if (self->iAmASendObject) {
    COVERAGE();
    PYCHECK( result /*owned*/ = PyString_FromString("<MPI_Request Send Operation> ") );
  } else {
    /* associated w/ an irecv() */
    if (self->postedMessage == 0) {
      COVERAGE();
      PYCHECK( result /*owned*/ = PyString_FromString("<MPI_Request Recv Operation: pending>") );
    } else {
      COVERAGE();
      PYCHECK( result /*owned*/ = PyString_FromString("<MPI_Request Recv Operation: finished>") );
    }
  }
  return result;

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************        request_type       ************************/
/**************************************************************************/
/* TODO: Convert to new style type                                        */
/**************************************************************************/
static PyTypeObject request_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                            /* Ob_Size */
  "MPI_Request",                /*Tp_Name */
  sizeof(PyMPI_Request),
  0,                            /*Tp_Itemsize*/
  /* Methods */
  request_dealloc,              /*Tp_Dealloc*/
  0,                            /*Tp_Print*/
  request_getattr,              /*Tp_Getattr*/
  0,                            /*Tp_Setattr*/
  0,                            /*Tp_Compare*/
  request_string,               /*Tp_Repr*/
  &request_number,              /*Tp_As_Number*/
  0,                            /*Tp_As_Sequence*/
  0,                            /*Tp_As_Mapping*/
  0,                            /*Tp_Hash*/
  0,                            /*Tp_Call*/
  request_string,               /*Tp_Str*/

  /* Space For Future Expansion */
  0l,0l,0l,0l,
  "MPI_Request\n\nMPI Request interface\n\n",
  0l,0l,0l,0l
};

/**************************************************************************/
/* GLOBAL **************     pyMPI_request_type    ************************/
/**************************************************************************/
/* This implements an interface to MPI_Request.  The request actually has */
/* to deal with the split message model that may cause multiple sends to  */
/* be sent for one user message.                                          */
/**************************************************************************/
PyTypeObject pyMPI_request_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                            /*int ob_size*/

  /* For printing, in format "<module>.<name>" */
  "request",                    /*char* tp_name*/

  /* For allocation */
  sizeof(PyMPI_Request),        /*int tp_basicsize*/
  0,                            /*int tp_itemsize*/

  /* Methods to implement standard operations */
  request_dealloc,              /*destructor tp_dealloc*/
  0,                            /*printfunc tp_print*/
  0,                            /*getattrfunc tp_getattr*/
  0,                            /*setattrfunc tp_setattr*/
  0,                            /*cmpfunc tp_compare*/
  request_string,               /*reprfunc tp_repr*/

  /* Method suites for standard classes */
  &request_number,              /*PyNumberMethods* tp_as_number*/
  0,                            /*PySequenceMethods* tp_as_sequence*/
  0,                            /*PyMappingMethods* tp_as_mapping*/

  /* More standard operations (here for binary compatibility) */
  0,                            /*hasfunc tp_hash*/
  0,                            /*ternaryfunc tp_call*/
  0,                            /*reprfunc tp_str*/
  PyObject_GenericGetAttr,      /*getattrofunc tp_getattro*/
  0,                            /*setattrofunc tp_setattro*/

  /* Functions to access object as input/output buffer */
  0,                            /*PyBufferProcs *tp_as_buffer*/

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*long tp_flags*/

  /* Doc */
  "communicator\n\nInterface to MPI communicator\n\n",

  0,                            /*traverseproc tp_traverse*/
  0,                            /*inquiry tp_clear*/
  0,                            /*richcmpfunc tp_richcompare*/
  0,                            /*long tp_weaklistoffset*/

  /* Iterators */
  0,                            /*getiterfunc tp_iter*/
  0,                            /*iternextfunc tp_iternext*/

  /* Attribute descriptor and subclassing stuff */
  requestMethods,               /* PyMethodDef *tp_methods; */
  0,                            /* PyMemberDef *tp_members; */
  request_getset,               /* PyGetSetDef *tp_getset; */
  0,                            /* _typeobject *tp_base; */
  0,                            /* PyObject *tp_dict; */
  0,                            /* descrgetfunc tp_descr_get; */
  0,                            /* descrsetfunc tp_descr_set; */
  0,                            /* long tp_dictoffset; */
  request_init,                 /* initproc tp_init; */
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
/* GLOBAL **************       pyMPI_request       ************************/
/**************************************************************************/
/* Build a request object.  The caller (internal only) sets up the send   */
/* flag and buffers                                                       */
/**************************************************************************/
PyMPI_Request* pyMPI_request(MPI_Comm comm) {
  PyMPI_Request* result = 0;
  PYCHECK( result = PyObject_NEW(PyMPI_Request,&request_type) );
         
  Assert(result);
  result->iAmASendObject = 0;
  result->description.count = 0;
  result->description.extent = 0;
  result->description.datatype = 0;
  result->descriptionRequest = MPI_REQUEST_NULL;
  result->bufferRequest = MPI_REQUEST_NULL;
  result->buffer = 0;
  result->communicator = comm;
  result->postedMessage = 0;
  result->status.MPI_SOURCE = 0;
  result->status.MPI_TAG    = 0;
  result->status.MPI_ERROR  = 0;

  result->abandoned_link = 0; /* Only used for abandoned sends */
  return result;

 pythonError:
  return 0;
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_request_init    ************************/
/**************************************************************************/
/* TODO: Set up the abandoned request chain                               */
/**************************************************************************/
void pyMPI_request_init(PyObject** docStringP) {
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_request_fini    ************************/
/**************************************************************************/
/* TODO: Close the abandoned request chain                                */
/**************************************************************************/
void pyMPI_request_fini(void) {
  PyMPI_Request* request = 0;

  for(request=abandoned_requests; request; request=request->abandoned_link) {
    /* ----------------------------------------------- */
    /* We cancel ignoring errors?                      */
    /* ----------------------------------------------- */
#ifndef PYMPI_BADCANCEL
    if ( request->descriptionRequest != MPI_REQUEST_NULL ) {
        MPI_Cancel(&request->descriptionRequest);
    }
    if ( request->bufferRequest != MPI_REQUEST_NULL ) {
        MPI_Cancel(&request->bufferRequest);
    }
#endif
  }
}

END_CPLUSPLUS

