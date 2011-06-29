#include "Python.h"

#ifdef HAVE_MPI
#include "mpi.h"
#endif

static char buf[33554432];
/**************************************************************************/
/* LOCAL  **************         floattime         ************************/
/**************************************************************************/
/*  */
/**************************************************************************/
static double floattime(void) {
  static PyObject* timemodule = 0;
  PyObject* py_result = 0;
  double result = 0.0;

  if ( timemodule == 0 ) {
    timemodule = PyImport_ImportModule("time");
    if ( PyErr_Occurred() ) return 0.0;
  }

  py_result = PyObject_CallMethod(timemodule,"time","");
  if ( PyErr_Occurred() ) return 0.0;
  result = PyFloat_AsDouble(py_result);
  if ( PyErr_Occurred() ) return 0.0;

  return result;
}


static PyObject* timefunction(PyObject* ignore, PyObject* args, PyObject* kw) {
  int n;
  PyObject* func = 0;
  PyObject* answer = 0;
  PyObject* func_args = 0;
  int i;
  double t;
  double t0;
  

  if ( PyTuple_GET_SIZE(args) < 2 ) {
    PyErr_SetString(PyExc_ValueError,"need 2 or more args");
    return 0;
  }

  n = PyInt_AsLong(PyTuple_GET_ITEM(args,0));
  if ( PyErr_Occurred() ) return 0;
  func = PyTuple_GET_ITEM(args,1);
  if ( !PyCallable_Check(func) ) {
    PyErr_SetString(PyExc_ValueError,"2nd arg must be function");
    return 0;
  }

  func_args /*owned*/= PySequence_GetSlice(args,2,PyTuple_GET_SIZE(args));

  /* Make one call outside the timer to "warm" it up */
  answer = PyObject_Call(func,func_args,kw);

  t0 = floattime();
  for(i=0;i<n;++i) {
    Py_XDECREF(answer);
    answer = PyObject_Call(func,func_args,kw);
  }
  t = floattime()-t0;

  Py_DECREF(func_args);
  return Py_BuildValue("dO",t/n,answer);
  
}

static PyObject* allreduce_doublesum(PyObject* ignore, PyObject* args) {
  double x = 0.0;
  double y = 0.0;
  int istat;
  long handle;
  MPI_Comm comm;

  PyArg_ParseTuple(args,"dl",&x,&handle);
  comm = (MPI_Comm)handle;

  istat = MPI_Allreduce(&x,&y,1,MPI_DOUBLE,MPI_SUM,comm);

  return PyFloat_FromDouble(y);
}
  
static PyObject* allreduce_doublemin(PyObject* ignore, PyObject* args) {
  double x = 0.0;
  double y = 0.0;
  int istat;
  long handle;
  MPI_Comm comm;

  PyArg_ParseTuple(args,"dl",&x,&handle);
  comm = (MPI_Comm)handle;

  istat = MPI_Allreduce(&x,&y,1,MPI_DOUBLE,MPI_MIN,comm);

  return PyFloat_FromDouble(y);
}
  

static PyObject* send_ping(PyObject* ignore, PyObject* args) {
  int n;
  int destination;
  MPI_Status status;
  int istat;

  PyArg_ParseTuple(args,"ii",&n,&destination);

#ifdef HAVE_MPI
  istat = MPI_Send(buf,n,MPI_CHAR,destination,0,MPI_COMM_WORLD);
  istat = MPI_Recv(buf,n,MPI_CHAR,destination,0,MPI_COMM_WORLD,&status);
#endif


  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* recv_ping(PyObject* ignore, PyObject* args) {
  int n;
  int source;
  MPI_Status status;
  int istat;

  PyArg_ParseTuple(args,"ii",&n,&source);

#ifdef HAVE_MPI
  istat = MPI_Recv(buf,n,MPI_CHAR,source,0,MPI_COMM_WORLD,&status);
  istat = MPI_Send(buf,n,MPI_CHAR,source,0,MPI_COMM_WORLD);
#endif


  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* c_integrate(PyObject* self, PyObject* args) {
  MPI_Comm comm;
  int n;

  int i,rank,size;
  double h,sum,x,myAnswer,answer;

  PyArg_ParseTuple(args,"li",&comm,&n);
  if ( PyErr_Occurred() ) return 0;

  MPI_Comm_rank(comm,&rank);
  MPI_Comm_size(comm,&size);

  h = 1.0/n;
  sum = 0.0;
  for(i=rank+1; i<n+1; i += size) {
    x = h*(i-0.5);
    sum += 4.0/(1.0+x*x);
  }
  myAnswer = h*sum;

  MPI_Reduce(&myAnswer,&answer,1,MPI_DOUBLE,MPI_SUM,0,comm);


  return PyFloat_FromDouble(answer);
}

static PyObject* c_barrier(PyObject* self, PyObject* args) {
  MPI_Comm comm;

  PyArg_ParseTuple(args,"l",&comm);
  if ( PyErr_Occurred() ) return 0;
  
  MPI_Barrier(comm);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* c_ping(PyObject* self, PyObject* args) {
  MPI_Comm comm;
  int s,r,rank,msg_len;
  char* msg,*buf;
  MPI_Status status;

  PyArg_ParseTuple(args,"liis#",&comm,&s,&r,&msg,&msg_len);
  if ( PyErr_Occurred() ) return 0;
  
  buf = (char*)(malloc(msg_len+1));

  MPI_Comm_rank(comm,&rank);

  if ( rank == s ) {
    MPI_Send(msg,msg_len,MPI_CHAR,r,99,comm);
    MPI_Recv(buf,msg_len,MPI_CHAR,r,99,comm,&status);
  } else {
    MPI_Recv(buf,msg_len,MPI_CHAR,s,99,comm,&status);
    MPI_Send(msg,msg_len,MPI_CHAR,s,99,comm);
  }    
  free(buf);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* c_sr(PyObject* self, PyObject* args) {
  MPI_Comm comm;
  int s,r,rank,msg_len;
  char* msg,*buf;
  MPI_Status status;

  PyArg_ParseTuple(args,"liis#",&comm,&s,&r,&msg,&msg_len);
  if ( PyErr_Occurred() ) return 0;
  
  buf = (char*)(malloc(msg_len+1));

  MPI_Comm_rank(comm,&rank);

  if ( rank == s ) {
    MPI_Sendrecv(msg,msg_len,MPI_CHAR,r,99,buf,msg_len,MPI_CHAR,r,99,comm,&status);
  } else {
    MPI_Sendrecv(msg,msg_len,MPI_CHAR,s,99,buf,msg_len,MPI_CHAR,s,99,comm,&status);
  }    
  free(buf);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyObject* c_gather(PyObject* self, PyObject* args) {
  MPI_Comm comm;
  int rank,size;
  int* buf = 0;

  PyArg_ParseTuple(args,"li",&comm,&rank);
  if ( PyErr_Occurred() ) return 0;

  MPI_Comm_size(comm,&size);
  if ( rank == 0 ) buf = (int*)(malloc(size*sizeof(int)));
  MPI_Gather ( &rank, 1, MPI_INT,
	       buf, 1, MPI_INT,
	       0, comm );
  if ( rank == 0 ) free(buf);

  Py_INCREF(Py_None);
  return Py_None;

}

static PyObject* c_bcast(PyObject* self, PyObject* args) {
  MPI_Comm comm;
  int msg;

  PyArg_ParseTuple(args,"li",&comm,&msg);
  if ( PyErr_Occurred() ) return 0;

  MPI_Bcast(&msg,1,MPI_INT,0,comm);

  Py_INCREF(Py_None);
  return Py_None;
}

static PyMethodDef methods[] = {
  {"send",send_ping,METH_VARARGS,"send/recv ping"},
  {"recv",recv_ping,METH_VARARGS,"recv/send ping"},
  {"c_integrate",c_integrate,METH_VARARGS,"calcpi"},
  {"c_barrier",c_barrier,METH_VARARGS,"barrier"},
  {"c_ping",c_ping,METH_VARARGS,"ping"},
  {"c_sr",c_sr,METH_VARARGS,"ping via sendrecv"},
  {"c_gather",c_gather,METH_VARARGS,"gather"},
  {"c_bcast",c_bcast,METH_VARARGS,"bcast"},
  {"allreduce_doublesum",allreduce_doublesum,METH_VARARGS,"allreduce w/MPI_SUM"},
  {"allreduce_doublemin",allreduce_doublemin,METH_VARARGS,"allreduce w/MPI_MIN"},
  {"timefunction",(PyCFunction)timefunction,METH_KEYWORDS,"timefunction(n,f,args...)"},
  {0,0}
};

void initpingpong(void) {
  Py_InitModule("pingpong",methods);
}

