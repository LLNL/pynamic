/**************************************************************************/
/* FILE   **************          pyMPI.h          ************************/
/**************************************************************************/
/* Author: Patrick Miller May 15 2002                                     */
/* Copyright (C) 2002 Patrick J. Miller                                   */
/**************************************************************************/
/*  */
/**************************************************************************/
#ifndef PYMPI_H
#define PYMPI_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/* GLOBAL **************     PyOS_StdioReadline    ************************/
/**************************************************************************/
/* This extern is not found in Python.h .. We have to steal it            */
/**************************************************************************/
#if PY_VERSION_HEX < ((2 << 24) | (3 << 16))
extern DL_IMPORT(char *) PyOS_StdioReadline Py_PROTO((char*));
#else
extern DL_IMPORT(char *) PyOS_StdioReadline Py_PROTO((FILE*,FILE*,char*));
#endif

/**************************************************************************/
/* GLOBAL **************          Py_Main          ************************/
/**************************************************************************/
/* This extern is not found in Python.h .. We have to steal it            */
/**************************************************************************/
extern int Py_Main( int argc, char *argv[] );

#include "pyMPI_Config.h"
#include "pyMPI_Types.h"
#include "pyMPI_Externals.h"

/* POE utilities for AIX */
#ifdef HAVE_PM_UTIL_H
#include <pm_util.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN (1024)
#endif

#ifdef __cplusplus
}
#endif

#endif
