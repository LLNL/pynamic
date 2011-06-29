/**************************************************************************/
/* FILE   **************     pyMPI_comm_misc.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/* Copyright (C) 2002 University of California Regents                    */
/**************************************************************************/
/*                                                                        */
/* Setup for micro tests                                                  */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/* >>> comm = WORLD.comm_dup()                                            */
/* >>> def fake(*args): return                                            */
/* >>> mpi.abort = fake                                                   */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GMETHOD **************     pyMPI_misc_create     ***********************/
/**************************************************************************/
/* In MPI, this is created from a GROUP object.                           */
/**************************************************************************/
/* Create sub-communicators from a communicator                           */
/*                                                                        */
/* comm_create(group) --> communicator                                    */
/*                                                                        */
/* We create new communicators from a list or range of ranks to include.  */
/* This is a bit different from MPI which uses a GROUP object.  When we   */
/* combine this functionality with the slice ability of communicators, it */
/* makes more sense.                                                      */
/*                                                                        */
/* Using the default communicator...                                      */
/* >>> even = mpi.comm_create( mpi.WORLD[0::2] )                          */
/* >>> assert mpi.size > 1,"running in parallel"                          */
/* >>> odd = mpi.comm_create( mpi.WORLD[1::2] )                           */
/*                                                                        */
/* You can use a list or range object as the argument too...              */
/* >>> first = mpi.comm_create( range(0,mpi.size/2) )                     */
/* >>> last  = mpi.comm_create( range(mpi.size/2,mpi.size) )              */
/* >>> assert mpi.size > 1,"running in parallel"                          */
/* >>> two = mpi.comm_create([0,1])                                       */
/*                                                                        */
/* Can be a communicator if you want a bad version of comm_dup...         */
/* This works because communicators are sequences [of their ranks]        */
/* >>> clone = mpi.comm_create(WORLD)                                     */
/*                                                                        */
/* You can use either a list or a single integer                          */
/* >>> comm2 = comm.comm_create([0])                                      */
/* >>> comm3 = comm.comm_create(0)                                        */
/*                                                                        */
/*                                                                        */
/* Note that the operation is synchronous requiring all processes on the  */
/* communicator to call it.                                               */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_misc_create(PyObject* pySelf, PyObject* args, PyObject* kw) {
   PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
   static char* kwlist[] = {"group",0};
   PyObject* inclusions = 0;
   MPI_Comm newComm = MPI_COMM_NULL;
   PyObject* newPyComm = 0;
   MPI_Group worldGroup = MPI_GROUP_NULL;
   MPI_Group newGroup = MPI_GROUP_NULL;
   int killWorld = 0;
   int killNew = 0;
   int i,j,len;
   PyObject* element = 0;
   int elementValue = 0;
   int* ranks = 0;
   int mySize = -1;
  
   COVERAGE();
   Assert(self);
   RAISEIFNOMPI();

   /* ----------------------------------------------- */
   /* We have an argument from which we build the new */
   /* group.                                          */
   /* ----------------------------------------------- */
   PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"O",kwlist,&inclusions));
   Assert( inclusions );

   /* ----------------------------------------------- */
   /* Mike Steder 11/18/04                            */
   /* Made changes to allow a singleton communicator  */
   /* to be created by passing a single integer to    */
   /* comm_create, as well as forcing None arguments  */
   /* to comm_create to return None                   */
   /* ----------------------------------------------- */
   /* Believed Correct Behavior:                      */
   /* comm_create( indexable object ) ->              */
   /*   intersect object with range(size) are "in"    */
   /*   rest return None                              */
   /* comm_create( castable_as_integer ) -> includes  */
   /*   matching rank, rest are None                  */
   /* comm_create( None ) --> All return None         */
   /*                                                 */
   /* The If(Indexable), else if(none), else(castable)*/
   /* should implement the above behavior.            */
   /* ----------------------------------------------- */
   /* If inclusions is a single element:              */
   /* We can grab this element and skip over the      */
   /* following loop.                                 */
   /* returns 1 if the object provides a sequence     */
   /* protocol: is a sequence 0 otherwise             */
   /* ----------------------------------------------- */
   if( PySequence_Check( inclusions ) ) {
     /* ----------------------------------------------- */
     /* Start pulling integers out of inclusion list    */
     /* ----------------------------------------------- */
     mySize = self->size;
     ranks = (int*)(malloc(sizeof(int)*mySize));
     Assert(ranks);
     for(len=0; ; ++len) {
        /* ----------------------------------------------- */
        /* Get value (may cause an index error (that's OK) */
        /* ----------------------------------------------- */
        element = PySequence_GetItem(inclusions,len);
        if ( PyErr_Occurred() ) {
           if ( PyErr_ExceptionMatches(PyExc_IndexError) ) {
              PyErr_Clear(); /* This was expected */
              Py_XDECREF(element);
              element = 0;
              break;
           } else {
              goto pythonError;
           }
        }

        /* ----------------------------------------------- */
        /* If we have too many values, something's wrong   */
        /* ----------------------------------------------- */
        if ( len >= mySize ) {
           PYCHECK( PyErr_SetString(PyExc_ValueError,"Inclusion list larger than communicator") );
        }
        /* ----------------------------------------------- */
        /* Pull value out as an integer                    */
        /* ----------------------------------------------- */
        PYCHECK( elementValue = PyInt_AsLong(element) );
        Py_XDECREF(element);
        element = 0;
        ranks[len] = elementValue;
     }
   }
   /* ----------------------------------------------- */
   /* If inclusions is a None type object             */
   /* ----------------------------------------------- */
   else if ( inclusions == Py_None ) {
      newPyComm = Py_None;
      Py_INCREF(Py_None);
      return newPyComm;
   }
   /* ----------------------------------------------- */
   /* Case in which inclusions is a single integer    */
   /* ----------------------------------------------- */
   else {
     mySize = self->size;
     len = 1;
     ranks = (int*)(malloc(sizeof(int)));
     element = PyNumber_Long( inclusions );
     PYCHECK( elementValue = PyInt_AsLong(element) );
     ranks[0] = elementValue;
   }
   /* ----------------------------------------------- */
   /* The group may be empty.  If so, we have to be   */
   /* careful since some MPI implementations allow an */
   /* empty group and others do not.  We just make a  */
   /* null communicator.                              */
   /* ----------------------------------------------- */
   if ( len == 0 ) {
     Py_INCREF(pyMPI_COMM_NULL);
     return pyMPI_COMM_NULL;
   }

   /* ----------------------------------------------- */
   /* Make sure each is in range and not duplicated   */
   /* ----------------------------------------------- */
   for(i=0;i<len;++i) {
      /* ----------------------------------------------- */
      /* Make sure it's in range                         */
      /* ----------------------------------------------- */
      if ( ranks[i] < 0 || ranks[i] >= mySize ) {
         char buf[256];
         sprintf(buf,"Out of range.  Rank %d in position %d",ranks[i],i);
         PYCHECK( PyErr_SetString(PyExc_ValueError,buf) );
      }

      /* ----------------------------------------------- */
      /* Make sure there are no duplicates               */
      /* ----------------------------------------------- */
      for(j=0; j < i; ++j) {
         if ( ranks[i] == ranks[j] ) {
            char buf[256];
            sprintf(buf,"Duplicated rank.  Rank %d in position %d",ranks[i],i);
            PYCHECK( PyErr_SetString(PyExc_ValueError,buf) );
         }
      }
   }

   /* ----------------------------------------------- */
   /* Build a new group that is build to match world  */
   /* (except with only the specified members).       */
   /* ----------------------------------------------- */
   MPICHECK( self->communicator,
             MPI_Comm_group(self->communicator,&worldGroup);
             killWorld = 1;
             );

   MPICHECKCOMMLESS(MPI_Group_incl(worldGroup,len,ranks,&newGroup);
                    killNew = 1;
                    );

   /* ----------------------------------------------- */
   /* Build a new communicator (or null if not in the */
   /* group).                                         */
   /* ----------------------------------------------- */
   MPICHECK( self->communicator,
             MPI_Comm_create(self->communicator, newGroup, &newComm));
   if ( newComm != MPI_COMM_NULL ) {
      PYCHECK( newPyComm = pyMPI_comm(newComm) );
   } else {
      newPyComm = Py_None;
      Py_INCREF(Py_None);
   }
   /* FALL THROUGH */

 pythonError:
   if ( ranks ) free(ranks);
   Py_XDECREF(element);
   if ( killWorld ) {
      killWorld = 0;
      MPICHECKCOMMLESS(MPI_Group_free(&worldGroup));
   }
   if ( killNew ) {
      killNew = 0;
      MPICHECKCOMMLESS(MPI_Group_free(&newGroup));
   }
   return newPyComm;
}

/**************************************************************************/
/* GMETHOD **************      pyMPI_misc_dup      ************************/
/**************************************************************************/
/* Implement MPI_Comm_dup()                                               */
/**************************************************************************/
/* Create a communicator the same size as the original                    */
/*                                                                        */
/* comm_dup() --> communicator                                            */
/* This creates a new MPI communicator handle that is the same size as    */
/* the original one.                                                      */
/*                                                                        */
/* >>> another_world = mpi.WORLD.comm_dup()                               */
/*                                                                        */
/* >>> another_world = mpi.comm_dup()                                     */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_misc_dup(PyObject* pySelf) {
   PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
   MPI_Comm newComm = MPI_COMM_NULL;
   PyObject* newPyComm = 0;

   COVERAGE();
   Assert(self);
   RAISEIFNOMPI();

   MPICHECK( self->communicator,
             MPI_Comm_dup(self->communicator, &newComm)
             );
   PYCHECK( newPyComm = pyMPI_comm(newComm) );
   return newPyComm;

 pythonError:
   return 0;
}

/**************************************************************************/
/* GMETHOD **************  pyMPI_misc_cart_create  ************************/
/**************************************************************************/
/* MPI_Cart_create()                                                      */
/**************************************************************************/
/* Create a cartesian grid or torus                                       */
/*                                                                        */
/* cartesian_communicator(dims=[old_comm.size], # default to 1D           */
/*                        periods=[1,...]       # default to torus        */
/*                        reorder=1)            # can reorder ranks       */
/* --> new cartesisan communicator                                        */
/*                                                                        */
/* This creates a cartesian communicator that may yeild optimized         */
/* messaging for some architectures.  It also activates methods to        */
/* compute neighbor ranks on a grid.  See also the cartesian_communicator */
/* type.                                                                  */
/*                                                                        */
/* >>> torus = mpi.WORLD.cart_create()                                    */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_misc_cart_create(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyObject* new_arguments = 0;
  PyObject* result = 0;
  PyObject* element = 0;
  int arg_count;
  int i;

  Assert(pySelf);

  /* ----------------------------------------------- */
  /* We add the communicator to the arguments        */
  /* ----------------------------------------------- */
  PYCHECK( arg_count = args?PyObject_Size(args):0 );
  PYCHECK( new_arguments /*owned*/ = PyTuple_New(arg_count+1) );

  Py_INCREF(pySelf);
  PyTuple_SET_ITEM(new_arguments,0,pySelf);

  for(i=0;i<arg_count;++i) {
    PYCHECK( element/*owned*/ = PySequence_GetItem(args,i) );
    PyTuple_SET_ITEM(new_arguments,i+1,element);
    element = 0;
  }

  /* ----------------------------------------------- */
  /* Now, call the normal cartesian communicator.    */
  /* ----------------------------------------------- */
  PYCHECK( result = PyObject_Call((PyObject*)(&pyMPI_cart_comm_type), new_arguments, kw) );

  /* fall through so we can reclaim new_arguments */
 pythonError:
  Py_XDECREF(new_arguments);
  return result;
}

/**************************************************************************/
/* GMETHOD **************     pyMPI_misc_rank      ************************/
/**************************************************************************/
/* Getting rank via a method instead of attribute.                        */
/**************************************************************************/
/* Even though one would typically access a communicator's rank via its   */
/* rank attribute, you can also use the comm_rank() member function. This */
/* is here mostly for textual compatibility with MPI programs.            */
/*                                                                        */
/* >>> mpi.WORLD.comm_rank() == mpi.WORLD.rank                            */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_misc_rank(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  assert(self);
  COVERAGE();
  return PyInt_FromLong(self->rank);
}

/**************************************************************************/
/* GMETHOD **************      pyMPI_misc_size      ***********************/
/**************************************************************************/
/* Getting size via a method instead of attribute                         */
/**************************************************************************/
/* Even though one would typically access a communicator's size via its   */
/* rank attribute, you can also use the comm_size() member function. This */
/* is here mostly for textual compatibility with MPI programs.            */
/*                                                                        */
/* >>> mpi.WORLD.comm_size() == mpi.WORLD.size                            */
/**************************************************************************/
PyObject* pyMPI_misc_size(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  assert(self);
  COVERAGE();
  return PyInt_FromLong(self->size);
}

/**************************************************************************/
/* GMETHOD **************     pyMPI_misc_free      ************************/
/**************************************************************************/
/* MPI_Comm_free()                                                        */
/**************************************************************************/
/* Explicitly release an MPI communicator.  Does not destroy the object   */
/*                                                                        */
/* comm_free() --> None                                                   */
/*                                                                        */
/* This function will release the MPI communicator handle associated with */
/* the Python skeleton.  Python can lose track of handles and delete one  */
/* more than once (if, for example, you created two Python handles with   */
/* the same MPI handle).                                                  */
/*                                                                        */
/* Users likely do not need to call this.  Python will free communicators */
/* that it created when they are destroyed.  Communicators made from user */
/* handles are not MPI_Comm_free()'d by Python unless their `persistent'  */
/* attribute is set to false.  You can, of course, explicitly release it  */
/* using this method.                                                     */
/*                                                                        */
/* >>> comm.comm_free()                                                   */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_misc_free(PyObject* pySelf) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  Assert(self);

  /* ----------------------------------------------- */
  /* Basic sanity checking                           */
  /* ----------------------------------------------- */
  COVERAGE();
  RAISEIFNOMPI();
   
  /* ----------------------------------------------- */
  /* Don't allow MPI_COMM_WORLD or PYTHON_COMM_WORLD */
  /* to be freed.                                    */
  /* ----------------------------------------------- */
  if ( self->communicator == MPI_COMM_WORLD ||
       self->communicator == MPI_COMM_NULL ) {
    PYCHECK( PyErr_SetString(PyExc_RuntimeError,"Cannot free system communicators") );
  }

  /* ----------------------------------------------- */
  /* Free the communicator (void return)             */
  /* ----------------------------------------------- */
  MPICHECK( self->communicator,
            MPI_Comm_free(&(self->communicator)) );
  Py_INCREF(Py_None);
  return Py_None;

 pythonError:
  return 0;
}


/**************************************************************************/
/* GMETHOD **************     pyMPI_misc_abort     ************************/
/**************************************************************************/
/* This is an interface to the MPI_Abort() call                           */
/* We abort with 77 so that automake is happy with result                 */
/**************************************************************************/
/* Emergency kill of MPI and Python                                       */
/*                                                                        */
/* abort(errorcode=1) --> None                                            */
/*                                                                        */
/* Use this to cleanly abort Python and MPI.  You can provide a status    */
/* (default value is 1), but the MPI implementation may ignore it.        */
/*                                                                        */
/* >>> mpi.abort()                                                        */
/*                                                                        */
/* >>> mpi.abort(77)                                                      */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_misc_abort(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;

  static char *kwlist[] = {"errorcode", 0};
  int errorcode = 1;

  Assert(self);

  /* ----------------------------------------------- */
  /* Basic sanity checking                           */
  /* ----------------------------------------------- */
  COVERAGE();
  RAISEIFNOMPI();

  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|i",kwlist,&errorcode) );

  /* We're screwed if MPI_Abort dies.. */
  Assert(MPI_Abort(self->communicator,errorcode) == MPI_SUCCESS);
  exit(errorcode);

 pythonError:
  return 0;
}

END_CPLUSPLUS

