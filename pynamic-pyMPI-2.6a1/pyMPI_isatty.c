/**************************************************************************/
/* FILE   **************       pyMPI_isatty.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller April 15 2003                                   */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* Hook on isatty() to force initialization and fix IBM/AIX/POE issues    */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

#ifdef HAVE_MPC_ISATTY
#include <pm_util.h>
#endif

#ifndef __THROW
#define __THROW
#endif

START_CPLUSPLUS

#ifdef HAVE_MPC_ISATTY
/**************************************************************************/
/* GLOBAL **************           isatty          ************************/
/**************************************************************************/
/* Replacement for isatty() with correct results under AIX's POE          */
/**************************************************************************/
int isatty(int filedes) __THROW {
  int status;

  /* ----------------------------------------------- */
  /* Do the isatty() work                            */
  /* ----------------------------------------------- */
  status = (mpc_isatty(filedes) == 1);

  return status;
}
#else
#if PYMPI_ISATTY
#ifndef __THROW
#define __THROW /*nothing*/
#endif
/**************************************************************************/
/* GLOBAL **************           isatty          ************************/
/**************************************************************************/
/* Assume stdin,stdout,stderr are attached to a tty                       */
/**************************************************************************/
int isatty(int filedes)  {
  return (filedes == 0 || filedes == 1 || filedes == 2);
}
#endif
#endif

END_CPLUSPLUS

