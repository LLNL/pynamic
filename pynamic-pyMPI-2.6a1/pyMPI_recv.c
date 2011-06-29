/**************************************************************************/
/* FILE   **************        pyMPI_recv.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller February  4 2003                                */
/**************************************************************************/
/* This is a common utility routine.                                      */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************         pyMPI_recv        ************************/
/**************************************************************************/
/* Receive a message on a communicator object given source and tag        */
/**************************************************************************/
PyObject* pyMPI_recv(PyMPI_Comm* self, int source, int tag) {
  PyMPI_Message buffer1;
  char*     buffer2 = 0;
  PyObject* result = 0;
  PyObject* result_status = 0;
  MPI_Status status;

  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  RAISEIFNOMPI();

  /* ----------------------------------------------- */
  /* Initial message                                 */ 
  /* ----------------------------------------------- */
  Assert(self);
  MPICHECK( self->communicator,
            MPI_Recv(&buffer1,
                     1, pyMPI_message_datatype,
                     source,tag,
                     self->communicator,&status)  );

  /* ----------------------------------------------- */
  /* Update the actual source/tag if we used         */
  /* wildcards                                       */
  /* ----------------------------------------------- */
  if ( source == MPI_ANY_SOURCE ) source = status.MPI_SOURCE;
  if ( tag == MPI_ANY_TAG ) tag = status.MPI_TAG;

  if ( buffer1.header.bytes_in_second_message ) {
    buffer2 = (char*)malloc(buffer1.header.bytes_in_second_message);
    Assert(buffer2);
    MPI_Recv(buffer2,
             buffer1.header.bytes_in_second_message,
             MPI_BYTE,source,tag,
             self->communicator,&status);
    buffer1.header.free_buffer = 1; /* This malloc'd space needs to be freed */
  }

  PYCHECK(result /*owned*/ = pyMPI_unpack(&buffer1,&buffer2));
  PYCHECK(result_status/*owned*/ = pyMPI_resultstatus(/*consumes*/result,status) );
  return result_status;

 pythonError:
  pyMPI_message_free(&buffer1,&buffer2);
  return 0;
}


END_CPLUSPLUS

