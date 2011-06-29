/**************************************************************************/
/* FILE   **************       pyMPI_status.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller May 21 2002                                     */
/**************************************************************************/
/*  */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************       status_dealloc      ************************/
/**************************************************************************/
/* Delete storage behind a status object                                  */
/**************************************************************************/
static void status_dealloc(PyObject* pySelf) {
  PYCHECK( PyObject_FREE(pySelf) );
  /* fall through */
 pythonError:
  return;
}

/**************************************************************************/
/* LOCAL  **************       status_getattr      ************************/
/**************************************************************************/
/* TODO: Convert to new style properties                                  */
/* TODO: Add in new attribute for error_message                           */
/**************************************************************************/
static PyObject* status_getattr(PyObject* pySelf, char* name) {
   PyMPI_Status* self = (PyMPI_Status*)pySelf;
   PyObject* attr = 0;

   Assert(self);
   if ( strcmp(name,"source") == 0 ) {
      attr = PyInt_FromLong(self->status.MPI_SOURCE);
   } else if ( strcmp(name,"tag") == 0 ) {
      attr = PyInt_FromLong(self->status.MPI_TAG);
   } else if ( strcmp(name,"error") == 0 ) {
      attr = PyInt_FromLong(self->status.MPI_ERROR);
   } else {
      PyErr_SetString(PyExc_AttributeError,name);
   }

   return attr;
}

/**************************************************************************/
/* LOCAL  **************       status_length       ************************/
/**************************************************************************/
/* In FORTRAN, status look like mini-arrays of length 3.  Here we do the  */
/* same.                                                                  */
/**************************************************************************/
static Py_ssize_t status_length(PyObject* pySelf) {
   return 3;
}

/**************************************************************************/
/* LOCAL  **************        status_item        ************************/
/**************************************************************************/
/* This __getitem__ method returns the source tag and error integers from */
/* a status object just like in FORTRAN.                                  */
/**************************************************************************/
static PyObject* status_item(PyObject* pySelf, Py_ssize_t position) {
   PyMPI_Status* self = (PyMPI_Status*)pySelf;
   PyObject* result = 0;

   Assert(self);

   if ( position == 0 ) {
      result = PyInt_FromLong(self->status.MPI_SOURCE);
   } else if ( position == 1 ) {
      result = PyInt_FromLong(self->status.MPI_TAG);
   } else if ( position == 2 ) {
      result = PyInt_FromLong(self->status.MPI_ERROR);
   } else {
      PyErr_SetString(PyExc_IndexError,"list index out of bounds");
   }

   return result;
}

/**************************************************************************/
/* LOCAL  **************       status_string       ************************/
/**************************************************************************/
/* The string for a status object shows the important components          */
/**************************************************************************/
static PyObject* status_string(PyObject* pySelf) {
   PyMPI_Status* self = (PyMPI_Status*)pySelf;

   char errorMessage[MPI_MAX_ERROR_STRING];
   int errorMessageLen = 0;
   MPI_Error_string(self->status.MPI_ERROR,errorMessage,&errorMessageLen);
   return PyString_FromFormat("<MPI_Status Source=\"%d\" Tag=\"%d\" Error=\"%d\" %s\\>",
           self->status.MPI_SOURCE,
           self->status.MPI_TAG,
           self->status.MPI_ERROR,
           errorMessage);
}

/**************************************************************************/
/* LOCAL  **************      status_sequence      ************************/
/**************************************************************************/
/* Status objects look like a 3 element array of source, tag, error       */
/**************************************************************************/
static PySequenceMethods status_sequence = {
   status_length, /* inquiry sq_length; */
   0, /* binaryfunc sq_concat; */
   0, /* intargfunc sq_repeat; */
   status_item, /* intargfunc sq_item; */
   0, /* intintargfunc sq_slice; */
   0, /* intobjargproc sq_ass_item; */
   0, /* intintobjargproc sq_ass_slice; */
};

/**************************************************************************/
/* LOCAL  **************       MPI_StatusType      ************************/
/**************************************************************************/
/* TODO: Convert to new style type                                        */
/* This is the type descriptor for MPI status variables.                  */
/**************************************************************************/
static PyTypeObject MPI_StatusType = {
   PyObject_HEAD_INIT(&PyType_Type)
   0, /* Ob_Size */
   "MPI_Status", /* Tp_Name */
   sizeof(PyMPI_Status),
   0,                                  /*Tp_Itemsize*/
   /* Methods */
   status_dealloc,                     /*Tp_Dealloc*/
   0,                                  /*Tp_Print*/
   status_getattr,                     /*Tp_Getattr*/
   0,                                  /*Tp_Setattr*/
   0,                                  /*Tp_Compare*/
   status_string,                      /*Tp_Repr*/
   0,                                  /*Tp_As_Number*/
   &status_sequence,                   /*Tp_As_Sequence*/
   0,                                  /*Tp_As_Mapping*/
   0,                                  /*Tp_Hash*/
   0,                                  /*Tp_Call*/
   status_string,                      /*Tp_Str*/
                
   /* Space For Future Expansion */
   0l,0l,0l,0l,
   "MPI_Status\n\nMPI Status interface\n\n",
   0l,0l,0l,0l
};

/**************************************************************************/
/* GLOBAL **************        pyMPI_status       ************************/
/**************************************************************************/
/* Create a new status object.  Only used internally.                     */
/**************************************************************************/
PyObject* pyMPI_status(MPI_Status status) {
   PyMPI_Status* result = 0;
   PYCHECK( result = PyObject_NEW(PyMPI_Status,&MPI_StatusType) );
   Assert(result);
   result->status = status;
   return (PyObject*)result;
   
 pythonError:
   return 0;
}

END_CPLUSPLUS

