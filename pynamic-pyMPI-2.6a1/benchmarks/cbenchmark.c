#include "Python.h"
#include "mpi.h"

static int rank;
static int master;
static int slave;

static double time0;
static double time1;
#define TIMER_START time0 = MPI_Wtime()
#define TIMER_STOP  time1 = MPI_Wtime()
#define TIMER_ELAPSED (1e6*(time1-time0))

static PyObject* latency(PyObject* self, PyObject* args) {
  int cnt;
  int bytes;
  int i;
  double total;
  double result;
  MPI_Status stat;
  char* send;
  char* receive;
 
  PyArg_ParseTuple(args,"ii",&cnt,&bytes);
  if ( PyErr_Occurred() ) return 0;
    
  if ( rank == master ) {
    send = malloc(bytes);
    receive = malloc(4);
    TIMER_START;
    for (i=0; i<cnt; i++){
      MPI_Send(send, bytes, MPI_CHAR, slave, 22, MPI_COMM_WORLD);
    }
    TIMER_STOP;
    MPI_Recv(receive, 4, MPI_CHAR, slave, 22, MPI_COMM_WORLD, &stat);
    total = TIMER_ELAPSED;
    free(send);
    free(receive);
    result = total/cnt;
  } else if ( rank == slave ) {
    receive = malloc(bytes);
    for (i=0; i<cnt; i++){
      MPI_Recv(receive, bytes, MPI_CHAR, master, 22, MPI_COMM_WORLD,&stat);
    }
    MPI_Send(receive, 4, MPI_CHAR, master, 22, MPI_COMM_WORLD);
    free(receive);
    result = 0.0;
  } else {
    result = 0.0;
  }
  return PyFloat_FromDouble(result);
}

static PyObject* roundtrip(PyObject* self, PyObject* args) {
  int cnt;
  int bytes;
  int i;
  double total;
  double result;
  MPI_Status stat;
  char* send;
  char* receive;
 
  PyArg_ParseTuple(args,"ii",&cnt,&bytes);
  if ( PyErr_Occurred() ) return 0;
    
  if ( rank == master ) {
    send = malloc(bytes);
    receive = malloc(bytes);
    TIMER_START;
    for (i=0; i<cnt; i++){
      MPI_Send(send, bytes, MPI_CHAR, slave, 22, MPI_COMM_WORLD);
      MPI_Recv(receive, bytes, MPI_CHAR, slave, 22, MPI_COMM_WORLD,&stat);
    }
    TIMER_STOP;
    total = TIMER_ELAPSED;
    free(send);
    free(receive);
    result = cnt/(total*1e-6);
  } else if ( rank == slave ) {
    receive = malloc(bytes);
    for (i=0; i<cnt; i++){
      MPI_Recv(receive, bytes, MPI_CHAR, master, 22, MPI_COMM_WORLD,&stat);
      MPI_Send(receive, bytes, MPI_CHAR, master, 22, MPI_COMM_WORLD);
    }
    free(receive);
    result = 0.0;
  } else {
    result = 0.0;
  }
  return PyFloat_FromDouble(result);
}

static PyObject* bandwidth(PyObject* self, PyObject* args) {
  int cnt;
  int bytes;
  int i;
  double total;
  double result;
  MPI_Status stat;
  char* send;
  char* receive;
 
  PyArg_ParseTuple(args,"ii",&cnt,&bytes);
  if ( PyErr_Occurred() ) return 0;
    
  if ( rank == master ) {
    send = malloc(bytes);
    receive = malloc(4);
    TIMER_START;
    for (i=0; i<cnt; i++){
      MPI_Send(send, bytes, MPI_CHAR, slave, 22, MPI_COMM_WORLD);
    }
    MPI_Recv(receive, 4, MPI_CHAR, slave, 22, MPI_COMM_WORLD,&stat);
    TIMER_STOP;
    total = TIMER_ELAPSED;
    free(send);
    free(receive);
    result = cnt/(total*1e-6);
  } else if ( rank == slave ) {
    receive = malloc(bytes);
    for (i=0; i<cnt; i++){
      MPI_Recv(receive, bytes, MPI_CHAR, master, 22, MPI_COMM_WORLD,&stat);
    }
    MPI_Send(receive, 4, MPI_CHAR, master, 22, MPI_COMM_WORLD);
    free(receive);
    result = 0.0;
  } else {
    result = 0.0;
  }
  return PyFloat_FromDouble(result);
}

static PyObject* bibandwidth(PyObject* self, PyObject* args) {
  int cnt;
  int bytes;
  int i;
  double total;
  double result;
  MPI_Status stats[2];
  MPI_Request requests[2];
  char* send;
  char* receive;
 
  PyArg_ParseTuple(args,"ii",&cnt,&bytes);
  if ( PyErr_Occurred() ) return 0;
    
  if ( rank == master ) {
    send = malloc(bytes);
    receive = malloc(bytes);
    TIMER_START;
    for (i=0; i<cnt; i++){
      MPI_Irecv(receive, bytes, MPI_CHAR, slave, 22, MPI_COMM_WORLD,&requests[1]);
      MPI_Isend(send, bytes, MPI_CHAR, slave, 22, MPI_COMM_WORLD,&requests[0]);
      MPI_Waitall(2, requests, stats);
    }
    TIMER_STOP;
    total = TIMER_ELAPSED;
    free(send);
    free(receive);
    result = cnt/(total*1e-6);
  } else if ( rank == slave ) {
    send = malloc(bytes);
    receive = malloc(bytes);
    for (i=0; i<cnt; i++){
      MPI_Irecv(receive, bytes, MPI_CHAR, master, 22, MPI_COMM_WORLD,&requests[1]);
      MPI_Isend(send, bytes, MPI_CHAR, master, 22, MPI_COMM_WORLD,&requests[0]);
      MPI_Waitall(2, requests, stats);
    }
    free(send);
    free(receive);
    result = 0.0;
  } else {
    result = 0.0;
  }
  return PyFloat_FromDouble(result);
}

static PyMethodDef cbenchmarkmethods[] = {
  {"latency",latency,METH_VARARGS,"Berkeley gap time (message launch time)"},
  {"roundtrip",roundtrip,METH_VARARGS,"Transactions/second"},
  {"bandwidth",bandwidth,METH_VARARGS,"in Kbytes/second"},
  {"bibandwidth",bibandwidth,METH_VARARGS,"Bidirectional bandwidth in Kbytes/second"},
  {0,0},
};

void initcbenchmark(void) {
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  MPI_Comm_size(MPI_COMM_WORLD,&slave);
  slave -= 1;
  master = 0;

  Py_InitModule("cbenchmark",cbenchmarkmethods);
}
