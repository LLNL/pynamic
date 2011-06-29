#include "Python.h"

#ifdef HAVE_MPI
#include "mpi.h"
#endif

static PyObject* advance_diffuse(PyObject* self, PyObject* args, PyObject* kw) {
  int* data = 0;
  PyObject* List;
  int walkers;
  int steps;
  int w,s,x,r,i;
  int radius;
  int count;
  int escapees;
#ifdef HAVE_MPI
  int leftover;
  MPI_Comm comm = MPI_COMM_WORLD;
  int rank;
  int size;
#else
  long comm = 0;
#endif
  static char* keys[] = {"radius","walkers","steps", "comm", 0};

  /* ----------------------------------------------- */
  /* Get a numeric array, the number of walks to     */
  /* take, and an optional communicator (MPI only)   */
  /* ----------------------------------------------- */
  PyArg_ParseTupleAndKeywords(args,kw,"iii|i",keys,
			      &radius,
			      &walkers,
			      &steps,
			      &comm);
  if ( PyErr_Occurred() ) return 0;
  if ( radius <= 0 ) {
    PyErr_SetString(PyExc_ValueError,"Radius must be > 0");
    return 0;
  }

  /* ----------------------------------------------- */
  /* Build a list with the data			     */
  /* ----------------------------------------------- */
  count = 1+2*radius;
  List = PyList_New(count);
  if ( PyErr_Occurred() ) return 0;

  /* ----------------------------------------------- */
  /* Allocate storage for that radius		     */
  /* ----------------------------------------------- */
  data = (int*)malloc(count*sizeof(int));
  for(i=0;i<count;++i) data[i] = 0;

  /* ----------------------------------------------- */
  /* Loop over our walkers			     */
  /* ----------------------------------------------- */
#ifdef HAVE_MPI
  MPI_Comm_rank(comm,&rank);
  MPI_Comm_size(comm,&size);
  leftover = walkers%size;
  walkers = walkers/size;
  if ( rank == 0 ) walkers += leftover;
#endif
  escapees = 0;
  for(w=0; w < walkers; ++w) {
    x = 0;
    for(s=0;s<steps;++s) {
      r = random();
      if ( r < 0 ) r = -r;
      x += (r%3 - 1);
    }
    if ( x > radius || x < -radius) { 
      escapees++;
    } else {
      data[radius+x]++; /* Need to offset since array holds negative */
    }
  }

  /* ----------------------------------------------- */
  /* For MPI, we must sum the data together	     */
  /* ----------------------------------------------- */
#ifdef HAVE_MPI
  MPI_Allreduce(data,data,count,MPI_INT,MPI_SUM,comm);
  MPI_Allreduce(&escapees,&escapees,1,MPI_INT,MPI_SUM,comm);
#endif

  /* ----------------------------------------------- */
  /* Build a list with that data		     */
  /* ----------------------------------------------- */
  for(i=0;i<count;++i) {
    PyObject* element/*owned*/= PyInt_FromLong(data[i]);
    if ( PyErr_Occurred() ) {
      Py_DECREF(List);
      free(data);
      return 0;
    }
    PyList_SET_ITEM(List,i,element);
  }

  free(data);
  return Py_BuildValue("Oi",List,escapees);
}

static PyMethodDef methods[] = {
  {"diffuse",(PyCFunction)advance_diffuse,METH_VARARGS|METH_KEYWORDS,"Simple \"drunkard's walk\" diffusion\ndiffuse(array,steps,communicator=WORLD)"},
  {0,0}
};

void initadvanced(void) {
  Py_InitModule("advanced",methods);
}
