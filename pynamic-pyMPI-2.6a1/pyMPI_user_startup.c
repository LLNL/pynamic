/**************************************************************************/
/* FILE   **************    pyMPI_user_startup.c   ************************/
/**************************************************************************/
/* Author: Patrick Miller May  1 2003                                     */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* Startup code for extender                                              */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************     pyMPI_user_startup    ************************/
/**************************************************************************/
/* Runs before Py_Initialize()                                            */
/* An example implementation would look like                              */
/* void pyMPI_user_startup(void) {                                        */
/*   extern void initfoo(void);                                           */
/*   PyImport_AppendInittab("foo",initfoo);                               */
/* }                                                                      */
/**************************************************************************/
void pyMPI_user_startup(void) {
}

END_CPLUSPLUS
