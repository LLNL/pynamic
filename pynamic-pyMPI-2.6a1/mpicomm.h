/**********************************************************************
* file: mpicomm.h
* date: 2001-May-30 14:07:35 
* Author: Martin Casado
* Last Modified:2001-May-30 14:07:28
*
* Description:
*  Contains declerations of all communication helper functions
*
**********************************************************************/

#ifndef _MPICOMM_H_
#define _MPICOMM_H_

#include "initmpi.h"
#ifdef __cplusplus
extern "C" {
#endif
/* ===================== comm_point_to_point.c ====================== */

extern PyObject* pympi_pt_to_pt_send_implementation(PyMPIObject_Communicator *self, PyObject* message, int destination, int tag);
extern PyObject* pympi_pt_to_pt_send(PyObject* pySelf,PyObject* args);
extern PyObject* pympi_pt_to_pt_recv_implementation(PyMPIObject_Communicator *self, int source, int tag);
extern PyObject* pympi_pt_to_pt_recv(PyObject* pySelf,PyObject* args);
extern PyObject* pympi_pt_to_pt_sendrecv(PyObject* pySelf,PyObject* args);

extern PyObject* native_pt_to_pt_send(PyObject* pySelf,PyObject* args);
extern PyObject* native_pt_to_pt_recv(PyObject* pySelf,PyObject* args);
extern PyObject* native_pt_to_pt_sendrecv(PyObject* pySelf,PyObject* args);

/* ===================== comm_collective.c =============================== */
extern PyObject* pympi_collective_bcast( PyObject* pySelf, PyObject* args );

extern PyObject* pympi_collective_barrier  ( PyObject*,PyObject*);
extern PyObject* pympi_collective_scatter  ( PyObject*,PyObject*,PyObject*);
extern PyObject* pympi_collective_allreduce( PyObject*,PyObject*,PyObject*);
extern PyObject* pympi_collective_reduce   ( PyObject*,PyObject*,PyObject*);
extern PyObject* pympi_collective_gather   ( PyObject*,PyObject*,PyObject*);
extern PyObject* pympi_collective_allgather( PyObject*,PyObject*,PyObject*);
extern PyObject* pympi_collective_scan     ( PyObject*,PyObject*,PyObject*);

extern PyObject* native_collective_allreduce(PyObject* pySelf, PyObject* args);
extern PyObject* native_collective_reduce(PyObject* pySelf, PyObject* args) ;
extern PyObject* native_collective_bcast(PyObject* pySelf, PyObject* args);

/* ===================== commhelpers.c =============================== */

extern int isMpiOp(int op);
extern int isSupportedReduceOp( int op );
extern int procsOf(PyMPIObject_Communicator* self);
extern PyObject* InterpretReceivedBuffer(char** bufferPtr, int count);
extern PyObject* notImplemented(PyObject* PySelf, PyObject* args);
extern PyObject* bundleWithStatus(PyObject* result, MPI_Status status) ;
extern PyObject* asList( PyObject* Obj, int doDecRef );

extern PyObject* GetFromAllProcesses( PyMPIObject_Communicator *self, 
                                      int root, PyObject* localValue,
				      int includeRank);

int PackMessage(PyObject* object,PyMPIObject_Communicator* comm,
                char buffer1[INITMESGLEN] ,char** buffer2,int* len); 

extern PyObject* DoReduceOp( PyObject* Obj1, PyObject* Obj2, int op, 
                             int proc2, int* location );

extern PyObject* nativeMakeReadyToSend(PyObject* object, void** buffer, long* count, 
                                       MPI_Datatype* datatype, MPI_Aint* extent);

extern PyObject* nativeInterpretReceivedBuffer(void** bufferPtr, int count, 
                                               MPI_Datatype datatype );

/* ===================== mpicart.c =============================== */
extern PyObject* PyMPI_Cart_create(MPI_Comm comm_old,int ndims, int* dims,int* periods,int reorder ); 


/* Defined in mpiasynchcomm.c                                 */
extern PyObject* comm_wait(PyObject* pySelf, PyObject* args);
extern PyObject* comm_test(PyObject* pySelf, PyObject* args);
extern PyObject* comm_wait_any(PyObject* pySelf, PyObject* args);
extern PyObject* comm_wait_all(PyObject* pySelf, PyObject* args);
extern PyObject* comm_test_cancelled(PyObject* pySelf, PyObject* args);
extern PyObject* comm_cancel(PyObject* pySelf, PyObject* args);
extern PyObject* pympi_isend(PyObject* pySelf,PyObject* args);
extern PyObject* pympi_irecv(PyObject* pySelf,PyObject* args);

extern PyObject* nativeIsend(PyObject* pySelf, PyObject* args); 
extern PyObject* nativeIrecv(PyObject* pySelf, PyObject* args) ;


#ifdef __cplusplus
}
#endif

#endif
