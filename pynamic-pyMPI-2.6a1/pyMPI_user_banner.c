/**************************************************************************/
/* FILE   **************    pyMPI_user_banner.c    ************************/
/**************************************************************************/
/* Author: Patrick Miller May  1 2003                                     */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* Modify part of the startup banner                                      */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS
/**************************************************************************/
/* GLOBAL **************     pyMPI_user_banner     ************************/
/**************************************************************************/
/* Can change the interactive banner.  Write a null terminated string     */
/* into buffer (n chars including NULL).                                  */
/*                                                                        */
/* void pyMPI_user_banner(char* buffer, int n) {                          */
/*  strncpy(buffer,"FOOP7.3",n);                                          */
/* }                                                                      */
/**************************************************************************/
void pyMPI_user_banner(char* buffer, int n) {
}
END_CPLUSPLUS
