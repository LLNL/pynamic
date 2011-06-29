/**************************************************************************/
/* FILE   **************   pyMPI_user_directory.c  ************************/
/**************************************************************************/
/* Author: Patrick Miller May  1 2003                                     */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* Add a directory to the path before pyMPI and site-packages             */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************    pyMPI_user_directory   ************************/
/**************************************************************************/
/* This function returns the full path to the directory where distutils   */
/* should install files.  It is an overrideable library function so that  */
/* standalone programs can change where the files will be stored.         */
/* If you return 0 (NULL pointer), then the directory is ignored.         */
/**************************************************************************/
char* pyMPI_user_directory(void) {
  return 0;
}
END_CPLUSPLUS

