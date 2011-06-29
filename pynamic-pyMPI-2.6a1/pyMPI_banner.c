/**************************************************************************/
/* FILE   **************       pyMPI_banner.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller April 22 2003                                   */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* This is the stub for user defined banners                              */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************        pyMPI_banner       ************************/
/**************************************************************************/
/* Can change the interactive banner.  Write a null terminated string     */
/* into buffer (n chars including NULL).                                  */
/**************************************************************************/
void pyMPI_banner(char* buffer, int n) {
}

END_CPLUSPLUS
