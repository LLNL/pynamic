/**************************************************************************/
/* FILE   **************        pyMPI_rco.c        ************************/
/**************************************************************************/
/* Author: Patrick Miller June  8 2004                                    */
/* Copyright (C) 2004 University of California Regents                    */
/**************************************************************************/
/*                                                                        */
/* Build some infrastructure for remote objects that communicate via      */
/* message queues                                                         */
/*                                                                        */
/* >>> import mpi                                                         */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************        sends_posted       ************************/
/**************************************************************************/
/* List of sends that have posted, but have not yet cleared machine       */
/**************************************************************************/
static PyObject* sends_posted = 0;
static PyObject* recvs_posted = 0;
static PyObject* object_cache = 0; /*TODO: Cache should be a dictionary */

/**************************************************************************/
/* LOCAL  **************      partial_dealloc      ************************/
/**************************************************************************/
/* When we kill the remote partial, we must release the true object       */
/**************************************************************************/
static void partial_dealloc(PyObject* pySelf) {
  PyMPI_RemotePartial* self = (PyMPI_RemotePartial*)pySelf;

  Assert(self);
  Py_XDECREF(self->actor);
  Py_XDECREF(self->destinations);
  Py_XDECREF(self->method);
}

/**************************************************************************/
/* LOCAL  **************       partial_slice       ************************/
/**************************************************************************/
/* When we slice or index, we are really just specifying which ranks are  */
/* going to be involved in the send                                       */
/**************************************************************************/
static PyObject* partial_slice(PyObject* pySelf, PyObject* index) {
  PyMPI_RemotePartial* self = (PyMPI_RemotePartial*)pySelf;

  Py_XDECREF(self->destinations);
  self->destinations = index;
  Py_XINCREF(index);

  Py_INCREF(pySelf);
  return pySelf;
}

/**************************************************************************/
/* LOCAL  **************      partial_mapping      ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static PyMappingMethods partial_mapping = {
  0,                            /* mp_length */
  partial_slice,                /* mp_subscript */
  0                             /* mp_ass_subscript */
};


/**************************************************************************/
/* LOCAL  **************     partial_message_to    ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static void partial_message_to(PyObject* comm, int comm_len, long index, PyObject* tag, PyObject* message) {
  PyObject* request = 0;
  if ( index < 0 || index >= comm_len ) {
    PyErr_Format(PyExc_IndexError,"Index %ld is out of range for communicator of size %d",index,comm_len);
    return;
  }

  Assert(sends_posted);
  PYCHECK( request/*owned*/ = PyObject_CallMethod(comm,"isend","OiO",message,index,tag) );
  PYCHECK( PyList_Append(sends_posted,request) );
  Py_DECREF(request); request = 0;

  return;
 pythonError:
  Py_XDECREF(request);
  return;
}

/**************************************************************************/
/* LOCAL  **************      partial_message      ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static void partial_message(PyObject* comm, int comm_len, PyObject* destination, PyObject* tag, PyObject* message) {
  long i;
  Py_ssize_t start;
  Py_ssize_t stop;
  Py_ssize_t step;
  PyObject* index = 0;

  if ( PyInt_Check(destination) ) {
    PYCHECK( i = PyInt_AsLong(destination) );
    PYCHECK( partial_message_to(comm,comm_len,i,tag,message) );
  }
  
  else if ( PySequence_Check(destination) && !PyString_Check(destination) ) {
    stop = PySequence_Size(destination); PyErr_Clear();
    for(i=0;i<stop;++i) {
      PYCHECK( index /*owned*/ = PySequence_GetItem(destination,i) );
      partial_message(comm,comm_len,index,tag,message);
      Py_XDECREF(index);
      index = 0;
    }
  }

  else if ( PySlice_Check(destination) ) {
    PYCHECK( PySlice_GetIndices((PySliceObject*)destination,comm_len,&start,&stop,&step) );
    for(i=start; i != stop; i += step) {
      if ( i < 0 || i >= comm_len ) continue;
      PYCHECK( partial_message_to(comm,comm_len,i,tag,message) );
    }
  }

  else {
    PyErr_SetString(PyExc_ValueError,"destination must be an integer, sequence, or slice");
  }
 pythonError:
  Py_XDECREF(index);
  return;
}

/**************************************************************************/
/* LOCAL  **************        partial_call       ************************/
/**************************************************************************/
/* Here is where the rubber meets the road... we bundle a message that we */
/* ship off to the requested ranks.                                       */
/**************************************************************************/
static PyObject* partial_call(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_RemotePartial* self = (PyMPI_RemotePartial*)pySelf;
  PyObject* message = 0;
  PyObject* comm = 0;
  PyObject* hash = 0;
  PyObject* tag = 0;
  int comm_len;

  Assert(self);
  Assert(self->actor);

  /* ----------------------------------------------- */
  /* Get the communicator and hash from the actor    */
  /* ----------------------------------------------- */
  PYCHECK( comm/*owned*/ = PyObject_GetAttrString(self->actor,"__remote_comm__") );
  PYCHECK( hash/*owned*/ = PyObject_GetAttrString(self->actor,"__remote_id__") );
  PYCHECK( tag/*owned*/ = PyObject_GetAttrString(self->actor,"__remote_tag__") );
  PYCHECK( comm_len = PyObject_Size(comm) );

  /* ----------------------------------------------- */
  /* Build a message with object unique ID, method   */
  /* name, args, and kw                              */
  /* TODO: Don't send the hash                       */
  /* ----------------------------------------------- */
  if ( kw ) {
    PYCHECK( message = PyTuple_New(4) );
    PyTuple_SET_ITEM(message,0,hash);
    PyTuple_SET_ITEM(message,1,self->method);
    PyTuple_SET_ITEM(message,2,args);
    PyTuple_SET_ITEM(message,3,kw);
  } else {
    PYCHECK( message = PyTuple_New(3) );
    PyTuple_SET_ITEM(message,0,hash);
    PyTuple_SET_ITEM(message,1,self->method);
    PyTuple_SET_ITEM(message,2,args);
  }
  Py_XINCREF(self->method);
  Py_XINCREF(args);
  Py_XINCREF(kw);
  hash = 0;

  /* ----------------------------------------------- */
  /* OK, now we loop through the args and send the   */
  /* message.                                        */
  /* ----------------------------------------------- */
  PYCHECK( partial_message(comm,comm_len,self->destinations,tag,message) );

  Py_INCREF(pySelf);
  return pySelf;

 pythonError:
  Py_XDECREF(hash);
  Py_XDECREF(message);
  Py_XDECREF(comm);
  return 0;
}

/**************************************************************************/
/* LOCAL  **************      partial_getattro     ************************/
/**************************************************************************/
/* Sets the remote method name                                            */
/**************************************************************************/
static PyObject* partial_getattro(PyObject* pySelf, PyObject* attrname) {
  PyMPI_RemotePartial* self = (PyMPI_RemotePartial*)pySelf;

  Py_XDECREF(self->method);
  self->method = attrname;
  Py_XINCREF(attrname);

  Py_INCREF(pySelf);
  return pySelf;
}

/**************************************************************************/
/* LOCAL  **************    remote_partial_type    ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static PyTypeObject remote_partial_type = {

  PyObject_HEAD_INIT(&PyType_Type)
  0,                            /*int ob_size*/

  /* For printing, in format "<module>.<name>" */
  "RemotePartial",              /*char* tp_name*/

  /* For allocation */
  sizeof(PyMPI_RemotePartial),  /*int tp_basicsize*/
  0,                            /*int tp_itemsize*/

  /* Methods to implement standard operations */
  partial_dealloc,              /*destructor tp_dealloc*/
  0,                            /*printfunc tp_print*/
  0,                            /*getattrfunc tp_getattr*/
  0,                            /*setattrfunc tp_setattr*/
  0,                            /*cmpfunc tp_compare*/
  0,                            /*reprfunc tp_repr*/

  /* Method suites for standard classes */
  0,                            /*PyNumberMethods* tp_as_number*/
  0,                            /*PySequenceMethods* tp_as_sequence*/
  &partial_mapping,             /*PyMappingMethods* tp_as_mapping*/

  /* More standard operations (here for binary compatibility) */
  0,                            /*hasfunc tp_hash*/
  partial_call,                 /*ternaryfunc tp_call*/
  0,                            /*reprfunc tp_str*/
  partial_getattro,             /*getattrofunc tp_getattro*/
  0,                            /*setattrofunc tp_setattro*/

  /* Functions to access object as input/output buffer */
  0,                            /*PyBufferProcs *tp_as_buffer*/

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT,           /*long tp_flags*/

  /* Doc */
  PYMPI_REMOTEPARTIAL_CTOR_DOC,

  0,                            /*traverseproc tp_traverse*/
  0,                            /*inquiry tp_clear*/
  0,                            /*richcmpfunc tp_richcompare*/
  0,                            /*long tp_weaklistoffset*/

  /* Iterators */
  0,                            /*getiterfunc tp_iter*/
  0,                            /*iternextfunc tp_iternext*/

  /* Attribute descriptor and subclassing stuff */
  0,                            /* PyMethodDef *tp_methods; */
  0,                            /* PyMemberDef *tp_members; */
  0,                            /* PyGetSetDef *tp_getset; */
  0,                            /* _typeobject *tp_base; */
  0,                            /* PyObject *tp_dict; */
  0,                            /* descrgetfunc tp_descr_get; */
  0,                            /* descrsetfunc tp_descr_set; */
  0,                            /* long tp_dictoffset; */
  0,                            /* initproc tp_init; */
  0,                            /* allocfunc tp_alloc; */
  0,                            /* newfunc tp_new; */
  0,                            /* destructor tp_free; Low-level free-memory routine */
  0,                            /* inquiry tp_is_gc;  For PyObject_IS_GC */
  0,                            /* PyObject *tp_bases; */
  0,                            /* PyObject *tp_mro;  method resolution order */
  0,                            /* PyObject *tp_cache; */
  0,                            /* PyObject *tp_subclasses; */
  0                             /* PyObject *tp_weaklist; */
};

/**************************************************************************/
/* METHOD  **************     remotepartial_ctor    ************************/
/**************************************************************************/
/* Create partial remote                                                  */
/**************************************************************************/
/* First step in RPC, from here we gather the destinations                */
/**************************************************************************/
static PyObject* remotepartial_ctor(PyObject* actor) {
  PyMPI_RemotePartial* self;

  /* ----------------------------------------------- */
  /* We build a temporary object holding the actor   */
  /* which we will use later in the message          */
  /* ----------------------------------------------- */
  PYCHECK( self = (PyMPI_RemotePartial*)
           PyObject_NEW(PyMPI_RemotePartial,&remote_partial_type) );

  self->actor = actor;
  Py_XINCREF(actor);

  self->destinations = 0;
  self->method = 0;

  return (PyObject*)self;

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************      rco_housekeeping     ************************/
/**************************************************************************/
/* Housekeeping will be very important!  We must process unexpected       */
/* messages as they come in                                               */
/**************************************************************************/
static void rco_housekeeping(void) {
  int i;
  int len;
  int ready;
  PyObject* request = 0;
  PyObject* new_request = 0;
  PyObject* message = 0;
  PyObject* actor = 0;
  PyObject* method = 0;
  PyObject* result = 0;

  /* ----------------------------------------------- */
  /* First we see if any sends are done so we can    */
  /* delete the associated request                   */
  /* ----------------------------------------------- */
  Assert(sends_posted);
  PYCHECK( len = PyList_Size(sends_posted) );
  for(i=0;i<len;) {
    int ready;

    /* ----------------------------------------------- */
    /* If the request is fulfilled, we can delete it   */
    /* ----------------------------------------------- */
    request/*borrow*/ = PyList_GET_ITEM(sends_posted,i);
    PYCHECK( ready = PyObject_IsTrue(request) );
    if ( ready == 1 ) {
      PYCHECK( PySequence_DelItem(sends_posted,i) );
      PYCHECK( len = PyList_Size(sends_posted) );
    } else {
      ++i;
    }
  }

  /* ----------------------------------------------- */
  /* Similarly, we check if a message has come in    */
  /* ----------------------------------------------- */
  Assert(recvs_posted);
  Assert(object_cache);
  PYCHECK( len = PyList_Size(recvs_posted) );
  for(i=0;i<len;++i) {
    PYCHECK( request/*borrow*/ = PyList_GET_ITEM(recvs_posted,i) );
    PYCHECK( ready = PyObject_IsTrue(request) );
    if ( ready == 1 ) {
      PYCHECK( message/*owned*/ = PyObject_GetAttrString(request,"message") );
      PYCHECK( actor/*borrow*/ = PyList_GET_ITEM(object_cache,i) );

      Assert(PyTuple_Check(message) && PyTuple_GET_SIZE(message) > 2 );
      PYCHECK( method/*owned*/ = PyObject_GetAttr(actor,PyTuple_GET_ITEM(message,1)) );

      Assert(PyTuple_GET_SIZE(message) == 3 || PyTuple_GET_SIZE(message) == 4 );
      if ( PyTuple_GET_SIZE(message) == 3 ) {
        PYCHECK( result/*owned*/ = PyObject_CallObject(method,PyTuple_GET_ITEM(message,2)) );
      } else {
        PYCHECK( result/*owned*/ = PyObject_Call(method,PyTuple_GET_ITEM(message,2),PyTuple_GET_ITEM(message,3)) );
      }
      Py_DECREF(method); method = 0;
      Py_DECREF(result); result = 0;


      /* ----------------------------------------------- */
      /* Make a new request for a message                */
      /* ----------------------------------------------- */
      PYCHECK( new_request/*owned*/ = PyObject_CallMethod(actor,"__remote_irecv__","") );
      PYCHECK( PyList_SetItem(recvs_posted,i,/*noinc*/new_request) );
      new_request = 0;
    }
  }

  /* Fall through */
 pythonError:
  Py_XDECREF(new_request);
  Py_XDECREF(message);
  Py_XDECREF(method);
  Py_XDECREF(result);
  return;
}

/**************************************************************************/
/* METHOD **************        remote_init        ************************/
/**************************************************************************/
/* Implements RemoteObject                                                */
/**************************************************************************/
/* RemoteObject(comm=mpi.WORLD,tag=99,receive=true)                       */
/*                                                                        */
/* Mix-in class for remote procedure invocation                           */
/*                                                                        */
/* >>> class foop(mpi.RemoteObject):                                      */
/* ...    def __init__(self):                                             */
/* ...       mpi.RemoteObject.__init__(self)                              */
/* ...    def method(self,x):                                             */
/* ...       print 'Got',x,'on rank',mpi.rank                             */
/* >>> obj = foop()                                                       */
/* >>> if mpi.rank == 1: obj.__remote__[0].method('hello')                */
/*                                                                        */
/* If you want all messages to flow to only one rank, use                 */
/* >>> class goop(mpi.RemoteObject):                                      */
/* ...    def __init__(self):                                             */
/* ...      mpi.RemoteObject.__init__(self,receive=(mpi.rank == 0))       */
/* >>> obj = goop()                                                       */
/*                                                                        */
/**************************************************************************/
static int remote_init(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Remote* self = (PyMPI_Remote*)pySelf;
  static char* kwlist[] = {"comm","tag","receive",0};
  PyObject* comm = 0;
  PyObject* tag = 0;
  static long last_unique = 0;
  long unique;
  int receive = 1;
  PyObject* hash = 0;
  PyObject* request = 0;

  /* ----------------------------------------------- */
  /* Sanity Check                                    */
  /* ----------------------------------------------- */
  Assert(self);

  /* ----------------------------------------------- */
  /* Make sure that the housekeeping is on to        */
  /* service messages!!!                             */
  /* ----------------------------------------------- */
  pyMPI_add_intensive_work(rco_housekeeping);

  /* ----------------------------------------------- */
  /* Fetch the args                                  */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|OOi",kwlist,/*borrow*/&comm,&tag,&receive));
  if ( comm == 0 ) {
    comm/*borrow*/ = pyMPI_world;
    Py_XINCREF(pyMPI_world);
  }

  /* ----------------------------------------------- */
  /* Do a sanity check                               */
  /* ----------------------------------------------- */
  if ( !PyObject_IsInstance(comm,(PyObject*)(&pyMPI_comm_type)) ) {
    PYCHECK( PyErr_SetString(PyExc_ValueError,"Invalid communicator") );
  }

  /* ----------------------------------------------- */
  /* Figure out a unique ID                          */
  /* ----------------------------------------------- */
  MPICHECK(((PyMPI_Comm*)comm)->communicator,
           MPI_Allreduce(&last_unique,&unique,1,MPI_LONG,MPI_MAX,((PyMPI_Comm*)comm)->communicator) );
  unique++;
  last_unique = unique;
  PYCHECK( hash/*owned*/ = PyInt_FromLong(unique) );

  self->comm = comm; Py_INCREF(comm);
  if ( tag ) { 
    self->tag  = tag;  Py_INCREF(tag);
  } else {
    PYCHECK( self->tag/*owned*/ = PyInt_FromLong(99) );
  }
  self->hash = hash;
  /*TODO: Message tag should be unique, not the hash */

  /* ----------------------------------------------- */
  /* Create a request object to wait for messages    */
  /* ----------------------------------------------- */
  if ( receive ) {
    Assert(recvs_posted);
    PYCHECK( request = PyObject_CallMethod(pySelf,"__remote_irecv__","") );
    PYCHECK( PyList_Append(recvs_posted,request) );
    Py_DECREF(request); request = 0;

    Assert(object_cache);
    PYCHECK( PyList_Append(object_cache,pySelf) );
    if ( PyErr_Occurred() ) {
      Assert(0);
      /* TODO: delete matching request */
    }
  }
  return 0;

 pythonError:
  Py_XDECREF(hash);
  return -1;
}

/**************************************************************************/
/* LOCAL  **************         remote_new        ************************/
/**************************************************************************/
/* Basic initialization                                                   */
/**************************************************************************/
static PyObject* remote_new(PyTypeObject* type, PyObject* args, PyObject* kw) {
  PyMPI_Remote* self = (PyMPI_Remote*)PyType_GenericNew(type,args,kw);
  self->comm = 0;
  self->tag = 0;
  self->hash = 0;

  return (PyObject*)self;
}

/**************************************************************************/
/* LOCAL  **************       remote_dealloc      ************************/
/**************************************************************************/
/* Remove references to comm and hash                                     */
/**************************************************************************/
static void remote_dealloc(PyObject* pySelf) {
  PyMPI_Remote* self = (PyMPI_Remote*)pySelf;

  Py_XDECREF(self->comm);
  Py_XDECREF(self->hash);

}


/**************************************************************************/
/* METHOD  **************     remote_get_remote     ***********************/
/**************************************************************************/
/* RPC                                                                    */
/**************************************************************************/
/* Access RPC (remote procedure call) path to remote methods              */
/**************************************************************************/
static PyObject* remote_get_remote(PyObject* pySelf, void* closure) {
  PyMPI_Remote* self = (PyMPI_Remote*)pySelf;
  if ( !self->comm ) {
    PyErr_SetString(PyExc_RuntimeError,"remote object not initialized");
    return 0;
  }

  return remotepartial_ctor(pySelf);
}

/**************************************************************************/
/* METHOD **************   remote_get_remote_id    ************************/
/**************************************************************************/
/* Local unique hash                                                      */
/**************************************************************************/
/* Unique remote identifier (for this comm)                               */
/**************************************************************************/
static PyObject* remote_get_remote_id(PyObject* pySelf, void* closure) {
  PyMPI_Remote* self = (PyMPI_Remote*)pySelf;
  Py_XINCREF(self->hash);
  return self->hash;
}

/**************************************************************************/
/* METHOD **************   remote_get_remote_comm   ***********************/
/**************************************************************************/
/* Local communicator                                                     */
/**************************************************************************/
/* Unique remote identifier (for this comm)                               */
/**************************************************************************/
static PyObject* remote_get_remote_comm(PyObject* pySelf, void* closure) {
  PyMPI_Remote* self = (PyMPI_Remote*)pySelf;
  Py_XINCREF(self->comm);
  return self->comm;
}

/**************************************************************************/
/* METHOD  **************   remote_get_remote_tag   ***********************/
/**************************************************************************/
/* Tag to use for messages                                                */
/**************************************************************************/
/* Tag used by isend() to post messages                                   */
/**************************************************************************/
static PyObject* remote_get_remote_tag(PyObject* pySelf, void* closure) {
  PyMPI_Remote* self = (PyMPI_Remote*)pySelf;
  Py_XINCREF(self->tag);
  return self->tag;
}

/**************************************************************************/
/* LOCAL  **************       remote_getset       ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static PyGetSetDef remote_getset[] = {
  {"__remote__",remote_get_remote,0,PYMPI_REMOTE_GET_REMOTE_DOC,0},
  {"__remote_id__",remote_get_remote_id,0,PYMPI_REMOTE_GET_REMOTE_ID_DOC,0},
  {"__remote_comm__",remote_get_remote_comm,0,PYMPI_REMOTE_GET_REMOTE_COMM_DOC,0},
  {"__remote_tag__",remote_get_remote_tag,0,PYMPI_REMOTE_GET_REMOTE_TAG_DOC,0},
  {0,0}
};

/**************************************************************************/
/* METHOD **************        remote_irecv       ************************/
/**************************************************************************/
/* Implements a irecv call to build a MPI request object for Remote       */
/**************************************************************************/
/* obj.__remote__irecv__() --> request                                    */
/*                                                                        */
/* For internal use in creating a request for pending messagesss          */
/**************************************************************************/
static PyObject* remote_irecv(PyObject* self, PyObject* ignore) {
  PyObject* tag = 0;
  PyObject* comm = 0;
  PyObject* request = 0;

  PYCHECK( tag/*owned*/ = PyObject_GetAttrString(self,"__remote_tag__") );
  PYCHECK( comm/*owned*/ = PyObject_GetAttrString(self,"__remote_comm__") );
  PYCHECK( request/*owned*/ = PyObject_CallMethod(comm,"irecv","iO",MPI_ANY_SOURCE,tag) );

  Py_DECREF(tag);
  Py_DECREF(comm);
  return request;

 pythonError:
  Py_XDECREF(tag);
  Py_XDECREF(comm);
  Py_XDECREF(request);
  return 0;
}

/**************************************************************************/
/* LOCAL  **************       remote_methods      ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static PyMethodDef remote_methods[] = {
  {"__remote_irecv__",remote_irecv,METH_NOARGS,PYMPI_REMOTE_IRECV_DOC},
  {0,0}
};

/**************************************************************************/
/* LOCAL  **************     pyMPI_remote_type     ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static PyTypeObject pyMPI_remote_type = {

  PyObject_HEAD_INIT(&PyType_Type)
  0,                            /*int ob_size*/

  /* For printing, in format "<module>.<name>" */
  "Remote",                     /*char* tp_name*/

  /* For allocation */
  sizeof(PyMPI_Remote),         /*int tp_basicsize*/
  0,                            /*int tp_itemsize*/

  /* Methods to implement standard operations */
  remote_dealloc,                 /*destructor tp_dealloc*/
  0,                            /*printfunc tp_print*/
  0,                            /*getattrfunc tp_getattr*/
  0,                            /*setattrfunc tp_setattr*/
  0,                            /*cmpfunc tp_compare*/
  0,                            /*reprfunc tp_repr*/

  /* Method suites for standard classes */
  0,                            /*PyNumberMethods* tp_as_number*/
  0,                            /*PySequenceMethods* tp_as_sequence*/
  0,                            /*PyMappingMethods* tp_as_mapping*/

  /* More standard operations (here for binary compatibility) */
  0,                            /*hasfunc tp_hash*/
  0,                            /*ternaryfunc tp_call*/
  0,                            /*reprfunc tp_str*/
  0,                            /*getattrofunc tp_getattro*/
  0,                            /*setattrofunc tp_setattro*/

  /* Functions to access object as input/output buffer */
  0,                            /*PyBufferProcs *tp_as_buffer*/

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*long tp_flags*/

  /* Doc */
  PYMPI_REMOTE_INIT_DOC,

  0,                            /*traverseproc tp_traverse*/
  0,                            /*inquiry tp_clear*/
  0,                            /*richcmpfunc tp_richcompare*/
  0,                            /*long tp_weaklistoffset*/

  /* Iterators */
  0,                            /*getiterfunc tp_iter*/
  0,                            /*iternextfunc tp_iternext*/

  /* Attribute descriptor and subclassing stuff */
  remote_methods,               /* PyMethodDef *tp_methods; */
  0,                            /* PyMemberDef *tp_members; */
  remote_getset,                /* PyGetSetDef *tp_getset; */
  0,                            /* _typeobject *tp_base; */
  0,                            /* PyObject *tp_dict; */
  0,                            /* descrgetfunc tp_descr_get; */
  0,                            /* descrsetfunc tp_descr_set; */
  0,                            /* long tp_dictoffset; */
  remote_init,                  /* initproc tp_init; */
  PyType_GenericAlloc,          /* allocfunc tp_alloc; */
  remote_new,                   /* newfunc tp_new; */
  0,                            /* destructor tp_free; Low-level free-memory routine */
  0,                            /* inquiry tp_is_gc;  For PyObject_IS_GC */
  0,                            /* PyObject *tp_bases; */
  0,                            /* PyObject *tp_mro;  method resolution order */
  0,                            /* PyObject *tp_cache; */
  0,                            /* PyObject *tp_subclasses; */
  0                             /* PyObject *tp_weaklist; */
};


/**************************************************************************/
/* GLOBAL **************       pyMPI_rco_init      ************************/
/**************************************************************************/
/* Add the RemoteObject type to pyMPI                                     */
/**************************************************************************/
void pyMPI_rco_init(PyObject** docstringP) {
  PyObject* factory/*borrowed*/ = (PyObject*)&pyMPI_remote_type;
  Py_INCREF(factory);

  PARAMETER(factory,"RemoteObject","A mix-in class to provide remote procedure calls",(PyObject*),pyMPI_dictionary,docstringP);

  PYCHECK( sends_posted = PyList_New(0) );
  PYCHECK( recvs_posted = PyList_New(0) );
  PYCHECK( object_cache = PyList_New(0) );

  return;

 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************       pyMPI_rco_fini      ************************/
/**************************************************************************/
/* Have to cleanup message lists                                          */
/**************************************************************************/
void pyMPI_rco_fini(void) {
  Py_XDECREF(sends_posted); sends_posted = 0;
  Py_XDECREF(recvs_posted); recvs_posted = 0;
  /* TODO: We die killing the dict's in derived objects :-( */
  /* Py_XDECREF(object_cache); object_cache = 0; */
}


END_CPLUSPLUS
