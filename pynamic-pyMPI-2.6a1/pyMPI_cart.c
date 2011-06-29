/**************************************************************************/
/* FILE   **************        pyMPI_cart.c       ************************/
/**************************************************************************/
/* Author: Martin Casado (Jan 2001)                                       */
/* Rewrite: Pat Miller (Jan 2003)                                         */
/**************************************************************************/
/*                                                                        */
/* Build a logical subclass of the communicator type.  This was made a    */
/* lot easier by Python2.2's new class structure.  Methods and attributes */
/* are overridden here, but for the most part, just use those available   */
/* in the standard communicator                                           */
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/* >>> class junk: pass                                                   */
/* >>> one_D = junk()                                                     */
/* >>> one_D.periods = []                                                 */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************     int_array_to_tuple    ************************/
/**************************************************************************/
/* Convert an array of integers into a Python tuple of integers.  Used    */
/* internally to convert the dimension information of cartesian comm obj  */
/* to Python form.                                                        */
/**************************************************************************/
static PyObject* int_array_to_tuple(int n, int* array) {
  PyObject* result = 0;
  int i;
  Assert(n>=0);
  Assert(array);
  PYCHECK(result/*owned*/ = PyTuple_New(n));
  for(i=0;i<n;++i) {
    PYCHECK(PyTuple_SET_ITEM(result,i,/*noinc,owned*/PyInt_FromLong((long)array[i])));
  }
  return result;
 pythonError:
  Py_XDECREF(result);
  return 0;
}

/**************************************************************************/
/* METHOD  **************      cart_get_dims       ************************/
/**************************************************************************/
/* Just return the stashed object we built with                           */
/**************************************************************************/
/* Dims object used to build communicator                                 */
/*                                                                        */
/* dims -> (integer, ...)                                                 */
/*                                                                        */
/* >>> print dir()                                                        */
/* >>> torus = cartesian_communicator()                                   */
/* >>> dim = torus.dims   # returns (WORLD.size,) the default             */
/*                                                                        */
/* >>> assert WORLD.size%2 == 0, "have an even number of processors"      */
/* >>> grid = cartesian_communicator(dims=[WORLD.size/2,2])               */
/* >>> dim2 = grid.dims   # returns (WORLD.size/2,2)                      */
/*                                                                        */
/**************************************************************************/
static PyObject* cart_get_dims(PyObject* pySelf, void* closure) {
  PyMPI_Cart_Comm* self = (PyMPI_Cart_Comm*)pySelf;

  COVERAGE();
  Assert(self);
  Assert(self->dims);
  Py_INCREF(self->dims);
  return self->dims;
}

/**************************************************************************/
/* METHOD  **************     cart_get_ndims       ************************/
/**************************************************************************/
/* We saved the dimensionality                                            */
/**************************************************************************/
/* Dimensionality of the torus, grid, cube, hypercube, etc...             */
/*                                                                        */
/* ndims -> integer                                                       */
/*                                                                        */
/* >>> torus = cartesian_communicator()                                   */
/* >>> n = torus.ndims       # returns 1 because this is a 1D torus       */
/*                                                                        */
/**************************************************************************/
static PyObject* cart_get_ndims(PyObject* pySelf, void* closure) {
  PyMPI_Cart_Comm* self = (PyMPI_Cart_Comm*)pySelf;
  Assert(self);

  return PyInt_FromLong(self->ndims);
}

/**************************************************************************/
/* METHOD  **************    cart_get_periods      ************************/
/**************************************************************************/
/* Just return the stashed object we built with                           */
/**************************************************************************/
/* Period object used to build communicator                               */
/*                                                                        */
/* periods -> (integer, ...)                                              */
/*                                                                        */
/* >>> torus = cartesian_communicator()                                   */
/* >>> periods = torus.periods  # returns (1,) the default                */
/*                                                                        */
/* >>> one_D = cartesian_communicator(periods=[0])  # No wrap around      */
/* >>> periods1 = one_D.periods  # returns (0,) in this case              */
/*                                                                        */
/* >>> assert WORLD.size %2 == 0, "have an even processor count..."       */
/* >>> grid = cartesian_communicator(dims=[WORLD.size/2,2])               */
/* >>> periods2 = one_D.periods  # returns (1,1) here                     */
/*                                                                        */
/**************************************************************************/
static PyObject* cart_get_periods(PyObject* pySelf, void* closure) {
  PyMPI_Cart_Comm* self = (PyMPI_Cart_Comm*)pySelf;

  COVERAGE();

  Assert(self);
  Assert(self->periods);
  Py_INCREF(self->periods);
  return self->periods;
}

/**************************************************************************/
/* LOCAL  **************        cart_getset        ************************/
/**************************************************************************/
/* Special attributes of cartesian communicators                          */
/**************************************************************************/
static PyGetSetDef cart_getset[] = {
  {"dims",cart_get_dims,0,PYMPI_CART_GET_DIMS_DOC,0},
  {"periods",cart_get_periods,0,PYMPI_CART_GET_PERIODS_DOC,0},
  {"ndims",cart_get_ndims,0,PYMPI_CART_GET_NDIMS_DOC,0},
  {0,0}
};

/**************************************************************************/
/* METHOD  **************       cart_coords        ************************/
/**************************************************************************/
/* Implements MPI_Cart_coords                                             */
/**************************************************************************/
/* Get local or remote grid coordinate                                    */
/*                                                                        */
/* coords(rank=local_rank)                                                */
/* --> (integer,...)                                                      */
/*                                                                        */
/* Returns an n-dimensional tuple with the coordiantes of the requested   */
/* rank on this communicator.  The rank can be a negative integer where   */
/* -1, for instance, is the last rank                                     */
/*                                                                        */
/* >>> torus = cartesian_communicator()                                   */
/* >>> x = torus.coords()      # returns torus.rank (local 1D position)   */
/*                                                                        */
/* >>> assert WORLD.size%2 == 0, "have an even processor count"           */
/* >>> grid = cartesian_communicator(dims=[WORLD.size/2,2])               */
/* >>> x0,y0 = grid.coords(0)   # Rank 0's x,y position                   */
/* >>> xn,yn = grid.coords(-1)  # Last rank's x,y position                */
/*                                                                        */
/**************************************************************************/
static PyObject* cart_coords(PyObject* pySelf,PyObject* args, PyObject* kw) {
  PyMPI_Cart_Comm* self = (PyMPI_Cart_Comm*)pySelf;
  static char* kwlist[] = { "rank", 0 };
  int rank = 0;                 /* initialized from self below */
  int* coords = 0;
  PyObject* result = 0;

  /* ----------------------------------------------- */
  /* Get the rank (defaults to rank on this comm)    */
  /* ----------------------------------------------- */
  Assert(self);
  rank = self->rank;
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|i",kwlist,&rank) );
  if ( rank < 0 ) rank += self->size; /* Pythonize */

  /* ----------------------------------------------- */
  /* Allocate some space for coordinates and fetch   */
  /* ----------------------------------------------- */
  coords = (int*)malloc(self->ndims * sizeof(int));
  Assert(coords);               /* TODO: Use Python exception on failure */
  MPICHECK(self->communicator,MPI_Cart_coords(self->communicator,rank,self->ndims,coords));  

  /* ----------------------------------------------- */
  /* Stuff into a tuple and return                   */
  /* ----------------------------------------------- */
  PYCHECK( result = int_array_to_tuple(self->ndims, coords) );

  free(coords); coords = 0;
  return result;

 pythonError:
  if(coords) free(coords);
  return 0;
}


/**************************************************************************/
/* METHOD  **************        cart_shift        ************************/
/**************************************************************************/
/* Get source/destination with specified shift                            */
/**************************************************************************/
/* Get source/destination with specified shift                            */
/*                                                                        */
/* cart_shift(direction=0,         # 0 <= Dimension to move < ndims       */
/*            displacement=1,      # steps to take in that dimension      */
/* --> integer,integer                                                    */
/*                                                                        */
/* The direction refers to a grid dimension, not a cardinal direction     */
/*                                                                        */
/* >>> torus = cartesian_communicator()                                   */
/* >>> left,right = torus.shift()                                         */
/*                                                                        */
/* >>> assert WORLD.size%2 == 0, "have an even processor count"           */
/* >>> grid = cartesian_communicator(dims=[WORLD.size/2,2])               */
/* >>> left,right = grid.shift(0)                                         */
/* >>> up,down = grid.shift(1)                                            */
/*                                                                        */
/**************************************************************************/
static PyObject* cart_shift(PyObject* pySelf, PyObject* args, PyObject* kw)  {
  PyMPI_Cart_Comm* self = (PyMPI_Cart_Comm*)pySelf;
  static char* kwlist[] = { "direction", "displacement", 0 };
  int direction=0;
  int displacement=1;
  int rank_source;
  int rank_dest;
  PyObject* result = 0;

  Assert(self);

  PYCHECK(PyArg_ParseTupleAndKeywords(args,kw,"|ii",kwlist,&direction,&displacement));
  MPICHECK(self->communicator,
           MPI_Cart_shift(self->communicator,direction,displacement,&rank_source,&rank_dest););

  PYCHECK(result/*owned*/ = Py_BuildValue("ii",rank_source,rank_dest));
  return result;

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************        cart_methods       ************************/
/**************************************************************************/
/* Specialization/addtions from the standard communicator object          */
/**************************************************************************/
static PyMethodDef cart_methods[] =  {
  {"coords",(PyCFunction)cart_coords,METH_VARARGS|METH_KEYWORDS,PYMPI_CART_COORDS_DOC},
  {"shift",(PyCFunction)cart_shift,METH_VARARGS|METH_KEYWORDS,PYMPI_CART_SHIFT_DOC},
  {0,0}
};

/**************************************************************************/
/* METHOD  **************        cart_init         ************************/
/**************************************************************************/
/* Extends mpi.communicator to allow cartesian communicators.  Mostly of  */
/* this code unpacks and error checks arguments.                          */
/**************************************************************************/
/* Create cartesian communicator from another communicator                */
/*                                                                        */
/* cartesian_communicator(old_comm=WORLD,       # Communicator to clone   */
/*                        dims=[old_comm.size], # default to 1D           */
/*                        periods=[1,...]       # default to torus        */
/*                        reorder=1)            # can reorder ranks       */
/* --> new cartesisan communicator                                        */
/*                                                                        */
/* Interface to MPI cartesian communicators which act identically to      */
/* normal communicators, but also embody the idea of a grid or torus on   */
/* top of that normal interface.  For instance, to set up a 1D torus and  */
/* then figure out each of your neighbors in forward direction            */
/* (forward is positive, backwards is negative) on dimension 0:           */
/*                                                                        */
/* >>> torus = cartesian_communicator(WORLD, [WORLD.size], [1])           */
/* >>> torus1 = cartesian_communicator(WORLD, [WORLD.size])               */
/* >>> torus2 = cartesian_communicator(WORLD)                             */
/*                                                                        */
/* >>> assert WORLD.size%2 == 2,"even processor count"                    */
/* >>> grid = cartesian_communicator(dims=[WORLD.size/2,2])               */
/*                                                                        */
/* >>> torus = cartesian_communicator(WORLD, [WORLD.size], [1])           */
/* >>> src,dest = torus.shift(0,1)                                        */
/* >>> msg,stat = torus.sendrecv('I am rank %d'%torus.rank,dest,src)      */
/*                                                                        */
/* You can find your n-dimensional coordinates too:                       */
/* >>> assert WORLD.size%2 == 2,"even processor count"                    */
/* >>> grid = mpi.cartesian_communicator(mpi.WORLD,[mpi.size/2,2],[1,1])  */
/* >>> x,y = grid.coords()                                                */
/*                                                                        */
/* You can also build a cartesian communicator by invoking a method       */
/* directly on any communicator.                                          */
/* >>> torus2 = WORLD.cart_create([WORLD.size])                           */
/*                                                                        */
/**************************************************************************/
static int cart_init(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Cart_Comm* self = (PyMPI_Cart_Comm*)pySelf;
  static char* kwlist[] = {"old_comm","dims","periods","reorder",0};
  long communicator_as_long = (long)pyMPI_COMM_WORLD;
  MPI_Comm communicator;
  PyObject* dims = 0;
  PyObject* periods = 0;
  PyObject* dims_sequence = 0;
  PyObject* periods_sequence = 0;
  int* dims_array = 0;
  int* periods_array = 0;
  int reorder = 1;
  int size = 0;
  int i;
  int ndims;
  int nperiods;
  MPI_Comm new_communicator;
  PyObject* tuple = 0;

  /* ----------------------------------------------- */
  /* Sanity check                                    */
  /* ----------------------------------------------- */
  Assert(self);
  Assert( PyObject_IsInstance(pySelf, (PyObject*)(&pyMPI_cart_comm_type)) );

  /* ----------------------------------------------- */
  /* Fetch the args                                  */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|lOOl",kwlist,&communicator_as_long,/*borrow*/&dims,/*borrow*/&periods,&reorder) );
  communicator = (MPI_Comm)communicator_as_long;

  /* ----------------------------------------------- */
  /* Won't work on null communicator                 */
  /* ----------------------------------------------- */
  if ( communicator == MPI_COMM_NULL ) {
    PYCHECK( PyErr_SetString(PyExc_ValueError,"invalid null communicator") );
  }

  /* ----------------------------------------------- */
  /* The dims argument defaults to a 1D array across */
  /* the whole communicator. Build the array before  */
  /* getting the size so that a bad sequence gives a */
  /* better error message.                           */
  /* ----------------------------------------------- */
  MPICHECK(communicator, MPI_Comm_size(communicator,&size) );
  if ( !dims ) {
    COVERAGE();
    PYCHECK( dims_sequence /*owned*/ = PyTuple_New(1) );
    PYCHECK( PyTuple_SET_ITEM(dims_sequence,0,PyInt_FromLong(size)) );
  } else {
    COVERAGE();
    Py_INCREF(dims);
    dims_sequence /* owned */ = dims;
  }
  PYCHECK( dims_array = pyMPI_util_sequence_to_int_array(dims_sequence,"Invalid dimension") );
  PYCHECK( ndims = PyObject_Size(dims_sequence) );
  if ( ndims < 1 ) {
    COVERAGE();
    PYCHECK( PyErr_Format(PyExc_ValueError, "Invalid dimension size %d",ndims) );
  }

  /* ----------------------------------------------- */
  /* The periods argument defaults to 1 in each dim  */
  /* ----------------------------------------------- */
  if ( !periods ) {
    COVERAGE();
    PYCHECK( periods_sequence /*owned*/ = PyTuple_New(ndims) );
    for(i=0; i<ndims; ++i) {
      PYCHECK( PyTuple_SET_ITEM(periods_sequence,i,PyInt_FromLong(1)) );
    }
  } else {
    COVERAGE();
    Py_INCREF(periods);
    periods_sequence /* owned */ = periods;
  }
  PYCHECK( periods_array = pyMPI_util_sequence_to_int_array(periods_sequence,"Invalid periods") );
  PYCHECK( nperiods = PyObject_Size(periods_sequence) );
  if ( nperiods != ndims ) {
    COVERAGE();
    PYCHECK( PyErr_Format(PyExc_ValueError, "Period count %d doesn't match dimension count %d",nperiods,ndims) );
  }

  /* ----------------------------------------------- */
  /* Finally, create the new communicator            */
  /* ----------------------------------------------- */
  MPICHECK( communicator, MPI_Cart_create(communicator, ndims, dims_array, periods_array, reorder, &new_communicator) );

  /* ----------------------------------------------- */
  /* Initialize in the normal way                    */
  /* ----------------------------------------------- */
  Assert(pyMPI_comm_type.tp_init);
  PYCHECK( tuple/*owned*/ = Py_BuildValue("(l)",(long)new_communicator) );
  PYCHECK( pyMPI_comm_type.tp_init(pySelf,tuple,0) );

  /* ----------------------------------------------- */
  /* Remmeber the dims and periods values            */
  /* ----------------------------------------------- */
  self->ndims = ndims;
  self->dims = dims_sequence; dims_sequence = 0;
  self->periods = periods_sequence; periods_sequence = 0;
  
  Py_XDECREF(tuple);
  if ( dims_array ) free(dims_array);
  if ( periods_array ) free(periods_array);

  return 0;

 pythonError:
  Py_XDECREF(tuple);
  Py_XDECREF(dims_sequence);
  Py_XDECREF(periods_sequence);
  if ( dims_array ) free(dims_array);
  if ( periods_array ) free(periods_array);
  return -1;
}

/**************************************************************************/
/* LOCAL  **************        cart_dealloc       ************************/
/**************************************************************************/
/* The deallocation is no different for cartesian communicators, so we    */
/* use the standard deallocator in the standard communicator.             */
/**************************************************************************/
static void cart_dealloc(PyObject* pySelf) {
  Assert(pyMPI_comm_type.tp_dealloc);
  pyMPI_comm_type.tp_dealloc(pySelf);
}

/**************************************************************************/
/* GLOBAL **************    pyMPI_cart_comm_type   ************************/
/**************************************************************************/
/* Type table describing the cartesian communicator.  Most work is simply */
/* deferred to the standard communicator                                  */
/**************************************************************************/
PyTypeObject pyMPI_cart_comm_type = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,                            /*int ob_size*/

  /* For printing, in format "<module>.<name>" */
  "cartesian_communicator",     /*char* tp_name*/

  /* For allocation */
  sizeof(PyMPI_Cart_Comm),      /*int tp_basicsize*/
  0,                            /*int tp_itemsize*/

  /* Methods to implement standard operations */
  cart_dealloc,                 /*destructor tp_dealloc*/
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
  PyObject_GenericGetAttr,      /*getattrofunc tp_getattro*/
  0,                            /*setattrofunc tp_setattro*/

  /* Functions to access object as input/output buffer */
  0,                            /*PyBufferProcs *tp_as_buffer*/

  /* Flags to define presence of optional/expanded features */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*long tp_flags*/

  /* Doc */
  PYMPI_CART_INIT_DOC,

  0,                            /*traverseproc tp_traverse*/
  0,                            /*inquiry tp_clear*/
  0,                            /*richcmpfunc tp_richcompare*/
  0,                            /*long tp_weaklistoffset*/

  /* Iterators */
  0,                            /*getiterfunc tp_iter*/
  0,                            /*iternextfunc tp_iternext*/

  /* Attribute descriptor and subclassing stuff */
  cart_methods,                 /* PyMethodDef *tp_methods; */
  0,                            /* PyMemberDef *tp_members; */
  cart_getset,                  /* PyGetSetDef *tp_getset; */
  &pyMPI_comm_type,             /* _typeobject *tp_base; */
  0,                            /* PyObject *tp_dict; */
  0,                            /* descrgetfunc tp_descr_get; */
  0,                            /* descrsetfunc tp_descr_set; */
  0,                            /* long tp_dictoffset; */
  cart_init,                    /* initproc tp_init; */
  PyType_GenericAlloc,          /* allocfunc tp_alloc; */
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
/* GLOBAL **************      pyMPI_cart_init      ************************/
/**************************************************************************/
/* Add the cartesian_communicator type to mpi                             */
/**************************************************************************/
void pyMPI_cart_init(PyObject** docstringP) {
  PyObject* factory/*borrowed*/ = (PyObject*)&pyMPI_cart_comm_type;
  Py_INCREF(factory);

  PARAMETER(factory,"cartesian_communicator","create new cartesian communicators",(PyObject*),pyMPI_dictionary,docstringP);
  return;

 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************      pyMPI_cart_fini      ************************/
/**************************************************************************/
/* No special cleanup work                                                */
/**************************************************************************/
void pyMPI_cart_fini(void) {
}

END_CPLUSPLUS

