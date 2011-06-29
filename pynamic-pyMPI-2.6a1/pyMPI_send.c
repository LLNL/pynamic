/**************************************************************************/
/* FILE   **************        pyMPI_send.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller February  4 2003                                */
/**************************************************************************/
/* This is a common utility to send a PyObject* somewhere on a            */
/* communicator.                                                          */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************         pyMPI_send        ************************/
/**************************************************************************/
/* This implements MPI_send().  It may send in two parts.                 */
/**************************************************************************/
PyObject* pyMPI_send(PyMPI_Comm* self,PyObject* message,int destination,int tag) {
  PyMPI_Message buffer1;
  char*     buffer2 = 0;

  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  COVERAGE();
  RAISEIFNOMPI();

  /* ----------------------------------------------- */
  /* Get MPI message ready                           */
  /* ----------------------------------------------- */
  PYCHECK( pyMPI_pack(message,self,&buffer1,&buffer2) );

  /* ----------------------------------------------- */
  /* Send message descriptor & message               */
  /* ----------------------------------------------- */
  MPICHECK( self->communicator, 
            MPI_Send(&buffer1,
                     1, pyMPI_message_datatype,
                     destination,tag,self->communicator)  );

  /* ----------------------------------------------- */
  /* If we are over the send limit, we send the rest */
  /* of the message with a second MPI_Send()         */
  /* ----------------------------------------------- */
  if ( buffer1.header.bytes_in_second_message ) {
    MPICHECK( self->communicator,
              MPI_Send(buffer2,
                       buffer1.header.bytes_in_second_message,
                       MPI_BYTE,destination,tag,
                       self->communicator) );
    /* buffer2 = 0; */
  }

  pyMPI_message_free(&buffer1,&buffer2);

  Py_XINCREF(Py_None);
  return Py_None;

 pythonError:
  pyMPI_message_free(&buffer1,&buffer2);
  return 0;
}

END_CPLUSPLUS

