/**************************************************************************/
/* FILE   **************        pyMPI_site.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller April 29 2003                                   */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* Add some paths like the "site" module does....                         */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

/**************************************************************************/
/* GLOBAL **************         pyMPI_site        ************************/
/**************************************************************************/
/* Adjust the work done by the ''site'' module.  We have to insert some   */
/* special paths to prefer pyMPI directories over Python ones.            */
/**************************************************************************/
void pyMPI_site(void) {
  PyObject* site = 0;
  PyObject* ignore = 0;
  char* user_directory = 0;

  PYCHECK( site = PyImport_ImportModule("site") );

  /* ----------------------------------------------- */
  /* If the extender provides a directory, add it in */
  /* here.                                           */
  /* ----------------------------------------------- */
  user_directory/*borrow*/ = pyMPI_user_directory();
  if ( user_directory ) {
    PYCHECK( ignore/*owned*/ =
             PyObject_CallMethod(site,
                                 "addsitedir",
                                 "s", 
                                 user_directory) );
    Py_DECREF(ignore);
    ignore = 0;
  }

  /* ----------------------------------------------- */
  /* Similarly, we add in the pyMPI site packages    */
  /* ----------------------------------------------- */
  PYCHECK( ignore/*owned*/ = 
           PyObject_CallMethod(
                               site,
                               "addsitedir",
                               "s",
                               PYMPI_SITEDIR) );
  Py_DECREF(ignore);
  ignore = 0;

  /* Fall through */
 pythonError:
  Py_XDECREF(site);
  Py_XDECREF(ignore);
  return;
}
