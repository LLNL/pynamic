/**************************************************************************/
/* FILE   **************    pyMPI_comm_message.c   ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/* Copyright (C) 2002 University of California Regents                    */
/**************************************************************************/
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/* >>> message = "Message from rank %d"%mpi.rank                          */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS


/**************************************************************************/
/* GMETHOD **************    pyMPI_message_send    ************************/
/**************************************************************************/
/* MPI_Send()                                                             */
/**************************************************************************/
/* Send a message to another process on a communicator.                   */
/*                                                                        */
/* send(message,            # Serializable Python object                  */
/*      destination,        # Rank to send object to                      */
/*      tag=0)              # Integer message tag                         */
/* --> None                                                               */
/*                                                                        */
/* The message is serialized with the pickle module and sent to the       */
/* specified process.  This routine may block.                            */
/*                                                                        */
/* >>> assert WORLD.size >= 2,"running in parallel"                       */
/* >>> if mpi.rank == 0:                                                  */
/* ...    send("a message from zero",1,33)                                */
/* ... elif mpi.rank == 1:                                                */
/* ...    msg,status = recv()                                             */
/* ...    print 'Got',msg,'from',status.source,'tag',status.tag           */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_message_send(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  int       destination = 0;
  int       tag    = 0;
  PyObject* message = 0;
  static char *kwlist[] = {"message", "destination", "tag", 0};

  COVERAGE();

  /* ----------------------------------------------- */
  /* msg = send(message,destination,tag=0)           */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"Oi|i",kwlist,&message,&destination,&tag) );
  Assert(message);
  return pyMPI_send(self,message,destination,tag);

 pythonError:
  return 0;
}

/**************************************************************************/
/* GMETHOD **************    pyMPI_message_recv    ************************/
/**************************************************************************/
/* MPI_recv()                                                             */
/**************************************************************************/
/* Receive a message.  Returns a list consisting of (data, status)        */
/*                                                                        */
/* recv(source=ANY_SOURCE,         # expected rank of sender              */
/*      tag=ANY_TAG)               # expected tag associated with message */
/* --> message,status                                                     */
/*                                                                        */
/* Get a message from another process on communicator.  The result is in  */
/* two parts: the message and the status object.  Use the status object   */
/* to find out the actual sender and tag value.                           */
/*                                                                        */
/* >>> assert WORLD.size >=2,"running in parallel"                        */
/* >>> if mpi.rank == 0:                                                  */
/* ...    msg,status = recv(1,tag=0)                                      */
/* ... elif mpi.rank == 1:                                                */
/* ...    send("hello from rank 1",0)                                     */
/*                                                                        */
/* >>> assert WORLD.size >=2,"running in parallel"                        */
/* >>> if mpi.rank == 0:                                                  */
/* ...    msg,status = recv() # defaults to ANY_SOURCE, ANY_TAG           */
/* ...    print msg,'from',status.source,'tag',status.tag                 */
/* ... elif mpi.rank == 1:                                                */
/* ...    send("hello from rank 1",0)                                     */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_message_recv(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  int source = MPI_ANY_SOURCE;
  int tag = MPI_ANY_TAG;
  static char *kwlist[] = {"source", "tag", 0};

  COVERAGE();

  /* ----------------------------------------------- */
  /* msg,stat =                                      */
  /*   Recv(source=mpi.ANY_SOURCE,tag=mpi.ANY_TAG)   */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|ii",kwlist,&source,&tag) );
  return pyMPI_recv(self,source,tag);

 pythonError:
  return 0;
}

/**************************************************************************/
/* GMETHOD **************  pyMPI_message_sendrecv  ************************/
/**************************************************************************/
/* MPI_sendrecv()                                                         */
/**************************************************************************/
/* Send message and receieve a value.                                     */
/*                                                                        */
/* sendrecv(message,            # Any serializable Python object          */
/*          destination,        # Rank of destination process             */
/*          source=ANY_SOURCE,  # Rank of sending process                 */
/*          sendtag=0,          # Message tag                             */
/*          recvtag=ANY_TAG)    # Expected tag from sender                */
/* --> message, status                                                    */
/*                                                                        */
/* Exchange messages with another process.  Actual sender and tag         */
/* information can be recovered from the status object returned with the  */
/* message (status.source, status.tag)                                    */
/*                                                                        */
/* Example:  A big left shift (wrapped on the end)                        */
/* >>> left = (WORLD.size + WORLD.rank - 1)%WORLD.size                    */
/* >>> right = (WORLD.rank + 1)%WORLD.size                                */
/* >>> msg,status = sendrecv("Hello",left,right)                          */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_message_sendrecv(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  int       destination = 0;
  int       source = MPI_ANY_SOURCE;
  int       sendtag = 0;
  int       recvtag = MPI_ANY_TAG;
  PyMPI_Message sendBuffer1;
  PyMPI_Message recvBuffer1;
  char*     sendBuffer2 = 0;
  char*     recvBuffer2 = 0;
  PyObject* object = 0;
  PyObject* result = 0;
  MPI_Status status;
  static char *kwlist[] = {"message", "destination","source","sendtag","recvtag", 0};
  PyObject* result_status = 0;

  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  COVERAGE();
  RAISEIFNOMPI();

  /* ----------------------------------------------- */
  /* msg = sendrecv(msg,destination,                 */
  /* source=destination,destTag = 0,recvtag = mpi.ANY*/
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"Oi|iii",kwlist,&object,&destination,&source,
                            &sendtag, &recvtag) );
  Assert(object);

  /* ----------------------------------------------- */
  /* Get MPI message ready for the send part         */
  /* ----------------------------------------------- */
  PYCHECK(pyMPI_pack(object,self,&sendBuffer1,&sendBuffer2));

  /* ----------------------------------------------- */
  /* Send-recv initial message buffer                */
  /* ----------------------------------------------- */
  MPICHECK( self->communicator, 
            MPI_Sendrecv( &sendBuffer1, 
                          1, pyMPI_message_datatype,
                          destination, sendtag,
                          &recvBuffer1,
                          1, pyMPI_message_datatype,
                          source, recvtag,
                          self->communicator, &status ) );

  /* ----------------------------------------------- */
  /* We now know the real source/recvtag             */
  /* ----------------------------------------------- */
  if ( source == MPI_ANY_SOURCE ) source = status.MPI_SOURCE;
  if ( recvtag == MPI_ANY_TAG ) recvtag = status.MPI_TAG;



  /* ----------------------------------------------- */
  /* If the message being sent must be broken into   */
  /* two, send the second part                       */
  /* ----------------------------------------------- */
  if( sendBuffer1.header.bytes_in_second_message) {
    MPICHECK( self->communicator,
              MPI_Send( sendBuffer2,
                        sendBuffer1.header.bytes_in_second_message,
                        MPI_CHAR,destination,
                        sendtag, self->communicator) );
    pyMPI_message_free(&sendBuffer1,&sendBuffer2);
  }    

  /* ----------------------------------------------- */
  /* If the msg being received is broken into 2, get */
  /* the second part                                 */
  /* ----------------------------------------------- */
  if ( recvBuffer1.header.bytes_in_second_message ) {
    recvBuffer2 = (char*)malloc(recvBuffer1.header.bytes_in_second_message);
    Assert(recvBuffer2);

    MPICHECK( self->communicator, 
              MPI_Recv( recvBuffer2,
                        recvBuffer1.header.bytes_in_second_message,
                        MPI_CHAR,source,recvtag,
                        self->communicator,&status) );                
    recvBuffer1.header.free_buffer = 1; /* This malloc'd space needs to be freed */
  }

  /* ----------------------------------------------- */
  /* Convert the full buffer back into an object.    */
  /* The unpack routine frees the buffer and zeros   */
  /* the pointer.                                    */
  /* ----------------------------------------------- */
  PYCHECK(result /*owned*/ = pyMPI_unpack(&recvBuffer1,&recvBuffer2));
        
  PYCHECK( result_status/*owned*/ = pyMPI_resultstatus(/*consumes*/result,status) );
  return result_status;

 pythonError:
  pyMPI_message_free(&sendBuffer1,&sendBuffer2);
  pyMPI_message_free(&recvBuffer1,&recvBuffer2);
  return 0;
}

END_CPLUSPLUS

