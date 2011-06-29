/**************************************************************************/
/* FILE   **************     pyMPI_user_init.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller May  1 2003                                     */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* User startup hook.  Will be called after Py_Initialize()               */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS


/**************************************************************************/
/* GLOBAL **************      pyMPI_user_init      ************************/
/**************************************************************************/
/* This is a stub routine that allows users to add their own processing   */
/* to pyMPI startup and shutdown.  An example version follows...          */
/* Note that you do NOT need any special headers (beyond maybe Python's)  */
/*                                                                        */
/* #include "Python.h"                                                    */
/* extern void initfoobar( void);                                          */
/* void pyMPI_user_init(PyObject** docstring_pointer) {                   */
/*    initfoobar();                                                       */
/* }                                                                      */
/*                                                                        */
/* You can use the docstring pointer to modify the mpi.__doc__ docstring  */
/*                                                                        */
/* You should also consider overriding pyMPI_user_directory() to          */
/* put a special directory forward of pyMPI and Python's in sys.path      */
/*                                                                        */
/* Any cleanup work can be put in pyMPI_user_fini().  They can be in the  */
/* same file, but are separated here to simplify user override of one or  */
/* the other                                                              */
/*                                                                        */
/**************************************************************************/
void pyMPI_user_init(PyObject** docstring_pointer) {
}

END_CPLUSPLUS
