/**************************************************************************/
/* FILE   ************** pyMPI_comm_asynchronous.c ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/* Copyright (C) 2002 University of California Regents                    */
/**************************************************************************/
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/* >>> message = "Message from rank %d"%rank                              */
/* >>> somewhere = rank                                                   */
/* >>> mpi.isend("bogus",rank)                                            */
/* >>> mpi.isend("bogus",rank)                                            */
/* >>> request = mpi.irecv(somewhere)                                     */
/* >>> request0 = request                                                 */
/* >>> request1 = mpi.irecv(somewhere)                                    */
/* >>> def cleanup():                                                     */
/* ...    global request0,request1                                        */
/* ...    request0.wait()                                                 */
/* ...    request1.wait()                                                 */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GMETHOD ************* pyMPI_asynchronous_isend *************************/
/**************************************************************************/
/* Implements MPI_isend().  Builds and returns a Python request object    */
/**************************************************************************/
/* Send a message, but do not block while it is inflight                  */
/*                                                                        */
/* isend(message,           # serializable Python object                  */
/*       destination,       # rank of receiving process                   */
/*       tag=0)             # integer message tag                         */
/* --> request                                                            */
/*                                                                        */
/* Just like send(), but guarantees that the sender will not block.       */
/*                                                                        */
/* >>> right = (WORLD.rank+1)%WORLD.size                                  */
/* >>> request = isend("hello",right)                                     */
/*                                                                        */
/* The request object can be tested to see if it has been received        */
/* See request.test() and request.wait() for information.                 */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_isend(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  PyObject* object = 0;
  int destination = 0;
  int tag = 0;
  char*  buffer2 = 0;
  PyMPI_Request* request = 0;
  static char* kwlist[] = {"message","destination","tag",0};
  

  COVERAGE();
  RAISEIFNOMPI();
  Assert(self);

  /* ----------------------------------------------- */
  /* handle = isend(msg,destination,tag=0            */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"Oi|i",kwlist,&object,&destination,&tag) );
  Assert(object);

  /* ----------------------------------------------- */
  /* Get a PyObject to hold messages */
  /* ----------------------------------------------- */
  PYCHECK( request /*owned*/ = pyMPI_request(self->communicator) );
  request->iAmASendObject = 1;

  /* ----------------------------------------------- */
  /* Get MPI message ready. Note that we directly    */
  /* pass in the buffer stored in the request object */
  /* ----------------------------------------------- */
  PYCHECK( pyMPI_pack( object, self, &request->firstBufMsg,&buffer2) );
  request->postedMessage = object;
  request->buffer = buffer2;

  /* ----------------------------------------------- */
  /* ISend message descriptor & message              */
  /* ----------------------------------------------- */
  MPICHECK( self->communicator, 
            MPI_Isend(&request->firstBufMsg,
                      1,pyMPI_message_datatype,
                      destination,tag,self->communicator,
                      &request->descriptionRequest )  );

  if (request->firstBufMsg.header.bytes_in_second_message) {
    MPICHECK( self->communicator,
              MPI_Isend(buffer2,
                        request->firstBufMsg.header.bytes_in_second_message,
                        MPI_CHAR,destination,
                        tag,self->communicator, &request->bufferRequest););

  }

  return (PyObject*)request;

 pythonError:
  Py_XDECREF(request);
  COVERAGE();
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_asynchronous_irecv ************************/
/**************************************************************************/
/* irecv() only really calls irecv() on the description that should be    */
/* coming in.  The reason for this is that, in order to size the buffer   */
/* for the data-irecv() the description must have already arrived         */
/* (which would block if done in this call).  Therefore the second        */
/* irecv() is called from the Handle's ready/wait/test methods.           */
/**************************************************************************/
/* Nonblocking request for a message                                      */
/*                                                                        */
/* >>> left = (WORLD.size+WORLD.rank-1)%WORLD.size                        */
/* >>> request = irecv(left)                                              */
/*                                                                        */
/* With the default arguments, you can request any message                */
/* >>> request = irecv()                                                  */
/*                                                                        */
/* Or you can request any message with a particular tag                   */
/* >>> request = irecv(tag=33)                                            */
/*                                                                        */
/* The request object can be tested to see if it has been received        */
/* See request.test() and request.wait() for information.                 */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_irecv(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  PyMPI_Request* request = 0;
  int source = MPI_ANY_SOURCE;
  int tag = MPI_ANY_TAG;
  static char* kwlist[] = {"source","tag",0};

  RAISEIFNOMPI();
  COVERAGE();
  Assert(self);

  /* ----------------------------------------------- */
  /* handle   =                                      */
  /*   Irecv(source=mpi.ANY_SOURCE,tag=mpi.ANY_TAG)  */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|ii",kwlist,&source,&tag) );

  /* ----------------------------------------------- */
  /* Set the right type to request object            */
  /* ----------------------------------------------- */
  PYCHECK( request /*owned*/ = pyMPI_request(self->communicator) );
  Assert(request); 
  request->iAmASendObject = 0; 

  /* ----------------------------------------------- */
  /* Get message descriptor                          */
  /* ----------------------------------------------- */
  /* request description data */
  MPICHECK( self->communicator,
            MPI_Irecv(&request->firstBufMsg,
                      1,pyMPI_message_datatype,
                      source,tag,self->communicator, &request->descriptionRequest) );
  return (PyObject*)request;

 pythonError:
  COVERAGE();
  return 0;
}

/**************************************************************************/
/* GMETHOD **************  pyMPI_asynchronous_test  ***********************/
/**************************************************************************/
/* Test a request object (through the request)                            */
/**************************************************************************/
/* Test if a request has been fulfilled.                                  */
/*                                                                        */
/* test(request) --> boolean                                              */
/*                                                                        */
/* Note that this is really just a convenience since the function will    */
/* actually invokes a property of the request.  That is,                  */
/* mpi.test(R) is the same as R.test                                      */
/*                                                                        */
/* This call does not block.                                              */ 
/*                                                                        */
/* >>> is_done = mpi.test(request)                                        */
/* >>> if request:  print 'done'     # Another way to invoke a test       */
/* >>> if request.test: print 'done' # Using request's built in method    */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_test(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyObject* request = 0;
  PyObject* result = 0;
  static char* kwlist[] = {"request",0};

  COVERAGE();
  
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"O",kwlist,&request));
  Assert(request);
  PYCHECK( result = PyObject_GetAttrString(request,"test") );
        
  return result;        
pythonError:
   return 0;
}

/**************************************************************************/
/* GMETHOD ********** pyMPI_asynchronous_test_cancelled *******************/
/**************************************************************************/
/* Implement test_cancelled (through the request)                         */
/**************************************************************************/
/* Test if a request has been cancelled.                                  */
/*                                                                        */
/* test_cancelled(request) --> boolean                                    */
/*                                                                        */
/* Note that this is really just a convenience since the function         */
/* actually invokes a property of the request.                            */
/*                                                                        */
/* Call does not block.                                                   */ 
/*                                                                        */
/* >>> request = mpi.irecv(somewhere)                                     */
/* >>> is_done = mpi.test_cancelled(request)  # or request.test_cancelled */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_test_cancelled(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyObject* request = 0;
  PyObject* result = 0;
  static char* kwlist[] = {"request",0};
  
  RAISEIFNOMPI();
  COVERAGE();

  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"O",kwlist,&request));
  Assert(request);
  PYCHECK( result = PyObject_GetAttrString(request,"test_cancelled") );
        
  return result;        
pythonError:
   return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_asynchronous_testall **********************/
/**************************************************************************/
/* Implements MPI_Testall() against a list of request objects             */
/**************************************************************************/
/* True if all requests are completed.                                    */
/*                                                                        */
/* testall(request,request,...) --> boolean                               */
/* testall([request,request,...]) --> boolean                             */
/* testall((request,request,...)) --> boolean                             */
/*                                                                        */
/* If some requests are pending, then an empty list is returned.          */
/* Call does not block.                                                   */
/*                                                                        */
/* >>> statuses = testall(request0,request1,...)                          */
/*                                                                        */
/* or you may pass a list/tuple of requests, e.g.                         */
/*                                                                        */
/* >>> statuses = testall([request0,request1,...])                        */
/* >>> statuses = testall((request0,request1,...))                        */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_testall(PyObject* pySelf, PyObject* args) {
  PyObject* varargs = 0;
  PyObject* element = 0;
  PyObject* temp = 0;
  PyObject* status = 0;
  PyObject* result = 0;
  PyObject* result_list = 0;
  int result_flag = 0;
  int i;
  int n;
  int flag;

  PYCHECK( varargs /*owned*/ = pyMPI_util_varargs(args) );
  Assert(PySequence_Check(varargs));
  PYCHECK( n = PySequence_Size(varargs) );
  PYCHECK( result_list = PyList_New(0) );
  for(i=0;i<n;++i) {
    PYCHECK( element /*owned*/ = PySequence_GetItem(varargs,i) );

    /* ----------------------------------------------- */
    /* Test the individual element (get flag,status)   */
    /* ----------------------------------------------- */
    PYCHECK( temp /*owned*/ = PyObject_GetAttrString(element,"test") );
    Py_DECREF(element);
    element = 0;

    flag = 0;
    PYCHECK( PyArg_ParseTuple(temp,"iO",&flag,&status/*borrowed*/) );

    result_flag &= flag;
    Py_INCREF(status);
    PYCHECK( PyList_Append(result_list,status) );

    Py_DECREF(temp);
    temp = 0;
    status = 0;
  }

  /* ----------------------------------------------- */
  /* If all are done, return the list (otherwise an  */
  /* empty list is returned.                         */
  /* ----------------------------------------------- */
  if ( result_flag ) {
    result = result_list;
    result_list = 0;
  } else {
    Py_DECREF(result_list);
    result_list = 0;
    PYCHECK( result = PyList_New(0) );
  }    
  return result;

 pythonError:
  Py_XDECREF(varargs);
  Py_XDECREF(element);
  Py_XDECREF(temp);
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_asynchronous_testany **********************/
/**************************************************************************/
/* Implements MPI_Testany()                                               */
/**************************************************************************/
/* Get index of a fulfilled request                                       */
/*                                                                        */
/* testany(request,request,...) --> int,status | None,None                */
/* testany([request,request,...]) --> int,status | None,None              */
/* testany((request,request,...)) --> int,status | None,None              */
/*                                                                        */
/* This returns the index of a request that is complete and its status.   */
/* If no request has completed, None,None is returned.                    */
/* You can either use multiple arguments or a list of requests.           */
/*                                                                        */
/* >>> index,status = mpi.testany(request0,request1,...)                  */
/* >>> index,status = mpi.testany([request0,request1,...])                */
/* >>> index,status = mpi.testany((request0,request1,...))                */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_testany(PyObject* pySelf, PyObject* args) {
  PyObject* varargs = 0;
  PyObject* element = 0;
  PyObject* temp = 0;
  PyObject* status = 0;
  PyObject* result = 0;

  int i;
  int n;
  int flag;

  PYCHECK( varargs /*owned*/ = pyMPI_util_varargs(args) );
  Assert(PySequence_Check(varargs));
  PYCHECK( n = PySequence_Size(varargs) );

  for(i=0;i<n;++i) {
    PYCHECK( element /*owned*/ = PySequence_GetItem(varargs,i) );

    /* ----------------------------------------------- */
    /* Test the individual element (get flag,status)   */
    /* ----------------------------------------------- */
    PYCHECK( temp /*owned*/ = PyObject_GetAttrString(element,"test") );
    Py_DECREF(element);
    element = 0;

    flag = 0;
    PYCHECK( PyArg_ParseTuple(temp,"iO",&flag,&status/*borrowed*/) );

    if ( flag ) {
      Py_INCREF(status);
      PYCHECK( result = Py_BuildValue("iO",i,status) );
      Py_DECREF(varargs);
      Py_DECREF(temp);
      return result;
    }

    Py_DECREF(temp);
    temp = 0;
    status = 0;
  }

  /* ----------------------------------------------- */
  /* No values were found.                           */
  /* ----------------------------------------------- */
  Py_INCREF(Py_None);
  Py_INCREF(Py_None);
  PYCHECK( result = Py_BuildValue("OO",Py_None,Py_None) );
  return result;

 pythonError:
  Py_XDECREF(varargs);
  Py_XDECREF(element);
  Py_XDECREF(temp);
  return 0;
}


/**************************************************************************/
/* GMETHOD ************* pyMPI_asynchronous_testsome **********************/
/**************************************************************************/
/* Implements MPI_testsome()                                              */
/**************************************************************************/
/* Find all requests that are ready.                                      */
/*                                                                        */
/* testsome(request,request,...) --> [indices],[statuses]                 */
/* testsome([request,request,...]) --> [indices],[statuses]               */
/* testsome((request,request,...)) --> [indices],[statuses]               */
/*                                                                        */
/* This returns the indices of requests that are complete and those       */
/* statues.  If no request has completed, mpi.testsome returns two empty  */
/* lists. You can either use multiple arguments or a list of requests.    */
/*                                                                        */
/* >>> indices,statuses = mpi.testsome(request0,request1,...)             */
/* >>> for i in indices:                                                  */
/* ...     print 'index',i,'is done'                                      */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_testsome(PyObject* pySelf, PyObject* args) {
  PyObject* varargs = 0;
  PyObject* element = 0;
  PyObject* temp = 0;
  PyObject* index = 0;
  PyObject* status = 0;
  PyObject* result = 0;
  PyObject* result_indices = 0;
  PyObject* result_statuses = 0;

  int i;
  int n;
  int flag;

  PYCHECK( varargs /*owned*/ = pyMPI_util_varargs(args) );
  Assert(PySequence_Check(varargs));
  PYCHECK( n = PySequence_Size(varargs) );
  PYCHECK( result_indices = PyList_New(0) );
  PYCHECK( result_statuses = PyList_New(0) );
  for(i=0;i<n;++i) {
    PYCHECK( element /*owned*/ = PySequence_GetItem(varargs,i) );

    /* ----------------------------------------------- */
    /* Test the individual element (get flag,status)   */
    /* ----------------------------------------------- */
    PYCHECK( temp /*owned*/ = PyObject_GetAttrString(element,"test") );
    Py_DECREF(element);
    element = 0;

    flag = 0;
    PYCHECK( PyArg_ParseTuple(temp,"iO",&flag,&status/*borrowed*/) );

    if ( flag ) {
      PYCHECK( index /*owned*/ = PyInt_FromLong(i) );
      PYCHECK( PyList_Append(result_indices,index) );
      index = 0;
      Py_INCREF(status);
      PYCHECK( PyList_Append(result_statuses,status) );
    }

    Py_DECREF(temp);
    temp = 0;
    status = 0;
  }

  PYCHECK( result = Py_BuildValue("OO",result_indices,result_statuses) );
  Py_DECREF(varargs);
  return result;

 pythonError:
  Py_XDECREF(varargs);
  Py_XDECREF(element);
  Py_XDECREF(temp);
  Py_XDECREF(result_indices);
  Py_XDECREF(result_statuses);
  return 0;

}

/**************************************************************************/
/* GMETHOD **************  pyMPI_asynchronous_wait  ***********************/
/**************************************************************************/
/* Implements MPI_Wait(request)                                           */
/**************************************************************************/
/* Wait for nonblocking operation to complete.                            */
/*                                                                        */
/* wait(request) --> status                                               */
/*                                                                        */
/* This function really just invokes the wait method on the request, so   */
/* >>> status = mpi.wait(request)                                         */
/*                                                                        */
/* is really the same as                                                  */
/* >>> status = request.wait()                                            */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_wait(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyObject* request = 0;
  PyObject* result = 0;
  static char* kwlist[] = {"request",0};
  
  RAISEIFNOMPI();
  COVERAGE();

  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"O",kwlist,&request));
  Assert(request);
  PYCHECK( result = PyObject_CallMethod(request,"wait","") );
        
  return result;        
 pythonError:
  COVERAGE();
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_asynchronous_waitall **********************/
/**************************************************************************/
/* Implements MPI_Waitall(request0,request1,....)                         */
/**************************************************************************/
/* Wait for all requests to complete.                                     */
/*                                                                        */
/* waitall(request,request,...) --> [statuses]                            */
/* waitall([request,request,...]) --> [statuses]                          */
/* waitall((request,request,...)) --> [statuses]                          */
/*                                                                        */
/* Returns a tuple of the associated status values.  You can either enter */
/* a list of requests or multiple request arguments.  The length of the   */
/* statuses list is the same as the number of requests (even if the       */
/* number of requests is zero).                                           */
/*                                                                        */
/* This call blocks until all requests are fulfilled                      */
/*                                                                        */
/* >>> statuses=waitall(request0,request1,...)                            */
/* >>> statuses=waitall([request0,request1,...])                          */
/* >>> statuses=waitall((request0,request1,...))                          */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_waitall(PyObject* pySelf, PyObject* args) {
  PyObject* varargs = 0;
  PyObject* element = 0;
  PyObject* status = 0;
  PyObject* result = 0;

  int i;
  int n;

  PYCHECK( varargs /*owned*/ = pyMPI_util_varargs(args) );
  Assert(PySequence_Check(varargs));
  PYCHECK( n = PySequence_Size(varargs) );
  PYCHECK( result /*owned*/ = PyList_New(0) );
  for(i=0;i<n;++i) {
    PYCHECK( element /*owned*/ = PySequence_GetItem(varargs,i) );

    /* ----------------------------------------------- */
    /* Wait for each request to finish                 */
    /* ----------------------------------------------- */
    PYCHECK( status /*owned*/ = PyObject_CallMethod(element,"wait","") );
    PYCHECK( PyList_Append(result,/*noinc*/status) );
    status = 0;
    Py_DECREF(element);
    element = 0;
  }

  return result;

 pythonError:
  Py_XDECREF(varargs);
  Py_XDECREF(element);
  Py_XDECREF(status);
  Py_XDECREF(result);
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_asynchronous_waitany **********************/
/**************************************************************************/
/* Implements MPI_Waitany(request0,request1,...)                          */
/**************************************************************************/
/* Wait for one of a set of requests to complete.                         */
/*                                                                        */
/* waitany(request,request,...) --> integer,status                        */
/* waitany([request,request,...]) --> integer,status                      */
/* waitany((request,request,...)) --> integer,status                      */
/*                                                                        */
/* Wait for one request to complete. Returns index of the request and     */
/* associated status.  Requests can be either multiple arguments or a     */
/* list of request objects.  Will block (or spin) if none are done.       */
/* The routine will throw ValueError if there are no requests.            */
/*                                                                        */
/* This call blocks until all requests are fulfilled                      */
/*                                                                        */
/* >>> index,status = waitany(request0,request1,...)                      */
/* >>> index,status = waitany([request0,request1,...])                    */
/* >>> index,status = waitany((request0,request1,...))                    */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_waitany(PyObject* pySelf, PyObject* args) {
  PyObject* varargs = 0;
  PyObject* element = 0;
  PyObject* temp = 0;
  PyObject* status = 0;
  PyObject* result = 0;

  int i;
  int n;
  int flag;

  PYCHECK( varargs /*owned*/ = pyMPI_util_varargs(args) );
  Assert(PySequence_Check(varargs));
  PYCHECK( n = PySequence_Size(varargs) );

  /* ----------------------------------------------- */
  /* Doesn't make sense to call with no requests     */
  /* ----------------------------------------------- */
  if ( n == 0 ) {
    PYCHECK( PyErr_SetString(PyExc_ValueError,"no request objects specified") );
  }

  /* ----------------------------------------------- */
  /* This polls which isn't really a good way to do  */
  /* the wait, but there are issues in using the MPI */
  /* implementation, namely that each Python request */
  /* is dealing with a (potentially) split message.  */
  /* ----------------------------------------------- */
  while(1) {
    for(i=0;i<n;++i) {
      PYCHECK( element /*owned*/ = PySequence_GetItem(varargs,i) );

      /* ----------------------------------------------- */
      /* Test the individual element (get flag,status)   */
      /* ----------------------------------------------- */
      PYCHECK( temp /*owned*/ = PyObject_GetAttrString(element,"test") );
      Py_DECREF(element);
      element = 0;

      flag = 0;
      PYCHECK( PyArg_ParseTuple(temp,"iO",&flag,&status/*borrowed*/) );

      if ( flag ) {
        Py_INCREF(status);
        PYCHECK( result = Py_BuildValue("iO",i,/*incs*/status) );
        Py_DECREF(varargs);
        Py_DECREF(temp);
        return result;
      }

      Py_DECREF(temp);
      temp = 0;
    }
  }

  /* unreachable */
  PyErr_SetString(PyExc_SystemError,"impossible escape from polling loop");
  return 0;

 pythonError:
  Py_XDECREF(varargs);
  Py_XDECREF(element);
  Py_XDECREF(temp);
  Py_XDECREF(result);
  return 0;
}

/**************************************************************************/
/* GMETHOD ************* pyMPI_asynchronous_waitsome **********************/
/**************************************************************************/
/* Implements MPI_Waitsome(request0,request1,...)                         */
/**************************************************************************/
/* Wait until some of a set of requests are complete.                     */
/*                                                                        */
/* waitsome(request,request,...) --> [integer],[status]                   */
/* waitsome([request,request,...]) --> [integer],[status]                 */
/* waitsome((request,request,...)) --> [integer],[status]                 */
/*                                                                        */
/* Waits for some communications to complete. Accepts a tuple of request  */
/* handles.  Will return a non-zero array of indices and statuses unless  */
/* no requests are entered.                                               */
/*                                                                        */
/* >>> indices,statuses = waitsome(request0,request1,...)                 */
/* >>> indices,statuses = waitsome([request0,request1,...])               */
/* >>> indices,statuses = waitsome((request0,request1,...))               */
/* >>> empty,empty = waitsome()                                           */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_waitsome(PyObject* pySelf, PyObject* args) {
  PyObject* varargs = 0;
  PyObject* element = 0;
  PyObject* temp = 0;
  PyObject* status = 0;
  PyObject* result_indices = 0;
  PyObject* result_statuses = 0;
  PyObject* result = 0;
  PyObject* index = 0;

  int i;
  int n;
  int flag;

  PYCHECK( varargs /*owned*/ = pyMPI_util_varargs(args) );
  Assert(PySequence_Check(varargs));
  PYCHECK( n = PySequence_Size(varargs) );

  PYCHECK( result_indices /*owned*/ = PyList_New(0) );
  PYCHECK( result_statuses /*owned*/ = PyList_New(0) );

  /* ----------------------------------------------- */
  /* If we have no requests, return the empty lists  */
  /* Otherwise, continue polling until we have at    */
  /* least one.                                      */
  /* ----------------------------------------------- */
  while(n > 0 && PySequence_Size(result_indices) == 0 ) {
    for(i=0;i<n;++i) {
      PYCHECK( element /*owned*/ = PySequence_GetItem(varargs,i) );

      /* ----------------------------------------------- */
      /* Test the individual element (get flag,status)   */
      /* ----------------------------------------------- */
      PYCHECK( temp /*owned*/ = PyObject_GetAttrString(element,"test") );
      Py_DECREF(element);
      element = 0;

      flag = 0;
      PYCHECK( PyArg_ParseTuple(temp,"iO",&flag,&status/*borrowed*/) );

      /* ----------------------------------------------- */
      /* We collect any status that have completed.      */
      /* ----------------------------------------------- */
      if ( flag ) {
        PYCHECK( index /*owned*/ = PyInt_FromLong(i) );
        PYCHECK( PyList_Append(result_indices,/*noinc*/index) );
        index = 0;
        Py_INCREF(status);
        PYCHECK( PyList_Append(result_statuses,/*noinc*/status) );
        status = 0;
      }

      Py_DECREF(temp); /* destroys status */
      temp = 0;
    }
  }

  PYCHECK( result = Py_BuildValue("OO",/*incs*/result_indices,/*incs*/result_statuses) );
  Py_DECREF(result_indices);
  Py_DECREF(result_statuses);
  return result;

 pythonError:
  Py_XDECREF(varargs);
  Py_XDECREF(element);
  Py_XDECREF(result);
  Py_XDECREF(result_indices);
  Py_XDECREF(result_statuses);
  return 0;
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_asynchronous_cancel ***********************/
/**************************************************************************/
/* Implements MPI_Cancel().  Note may not work for sends                  */
/**************************************************************************/
/* Cancel a communication request.                                        */
/*                                                                        */
/* cancel() --> None                                                      */
/*                                                                        */
/* The standard indicates that this may have no effect for isends().      */
/*                                                                        */
/* >>> mpi.cancel(request)                                                */
/*                                                                        */
/* Note that this is the same as:                                         */
/* >>> request.cancel()                                                   */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_asynchronous_cancel(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyObject* request = 0;
  PyObject* result = 0;
  static char* kwlist[] = {"request",0};
  
  RAISEIFNOMPI();
  COVERAGE();

  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"O",kwlist,&request));
  Assert(request);
  PYCHECK( result = PyObject_CallMethod(request,"cancel","") );
        
  return result;
        
 pythonError:
  COVERAGE();
  return 0;
}

END_CPLUSPLUS
