/**************************************************************************/
/* FILE   **************     pyMPI_user_fini.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller May  1 2003                                     */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* User shutdown code                                                     */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************      pyMPI_user_fini      ************************/
/**************************************************************************/
/* This is the last thing called before tearing down the pyMPI structures */
/**************************************************************************/
void pyMPI_user_fini(void) {
}

END_CPLUSPLUS
