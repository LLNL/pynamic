/**************************************************************************/
/* FILE   **************      pyMPI_comm_map.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/* Copyright (C) 2002 University of California Regents                    */
/**************************************************************************/
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/*                                                                        */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GMETHOD **************      pyMPI_map_map       ************************/
/**************************************************************************/
/* The map function iterates across a sequence and applies a function to  */
/* each element returning a list                                          */
/**************************************************************************/
/* SIMD style parallel map. Other processors must be running mapserver    */
/* The idea all the user code is run on the root.  When a map is invoked  */
/* then the function and part of the argument list is sent to each slave  */
/* which does its work which is regathered back to the master.            */
/*                                                                        */
/* >>> assert NotImplementedError,"TODO: implement parallel map"          */
/* >>> mpi.mapserver()                                                    */
/* >>> answers = mpi.map(F,list)                                          */
/**************************************************************************/
PyObject* pyMPI_map_map(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyErr_SetString(PyExc_NotImplementedError,"Not Implemented comm_map");
  return 0;
}

/**************************************************************************/
/* GMETHOD **************    pyMPI_map_mapserver    ***********************/
/**************************************************************************/
/* This puts the root into "server" mode and the other processors into    */
/* "client" mode (to service mpi.map()).                                  */
/**************************************************************************/
/* SIMD style parallel map server routine. Does not return on slave (non- */
/* zero) processors.  Any errors it gets are sent back to the master when */
/* mpi.map calls are made.                                                */
/*                                                                        */
/* >>> assert NotImplementedError,"TODO: implement mapserver"             */
/* >>> mpi.mapserver()                                                    */
/**************************************************************************/
PyObject* pyMPI_map_mapserver(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyErr_SetString(PyExc_NotImplementedError,"Not Implemented comm_mapserver");
  return 0;
}

/**************************************************************************/
/* GMETHOD **************    pyMPI_map_mapstats    ************************/
/**************************************************************************/
/* Get load information from each slave                                   */
/**************************************************************************/
/* This is used to get load balance information.  Each slave sends back a */
/* run-time/idle-time pair.   Assumes slaves are running mapserver()      */
/*                                                                        */
/* >>> assert NotImplementedError,"TODO: implement mapstats"              */
/* >>> stats = mpi.mapstats()                                             */
/**************************************************************************/
PyObject* pyMPI_map_mapstats(PyObject* pySelf, PyObject* args, PyObject* kw) {
  PyErr_SetString(PyExc_NotImplementedError,"Not Implemented comm_mapstats");
  return 0;
}

END_CPLUSPLUS

