/**************************************************************************/
/* FILE   **************        pyMPI_main.c       ************************/
/**************************************************************************/
/* Author: Patrick Miller May 15 2002                                     */
/* Copyright (C) 2002 Patrick J. Miller                                   */
/**************************************************************************/
/* Simple startup for MPI and Python                                      */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************         pyMPI_Main        ************************/
/**************************************************************************/
/* The "main" program.  It will initialize MPI, python, and pyMPI         */
/**************************************************************************/
int pyMPI_Main_with_communicator(int Python_owns_MPI, int *argc, char ***argv,MPI_Comm comm) {
  int status;
  int finalized;
  int rank;
  int i;
  int color = MPI_UNDEFINED;

  Assert(argc);
  Assert(argv);

  /* ----------------------------------------------- */
  /* In this simple main, we just call MPI_Init and  */
  /* go.                                             */
  /* ----------------------------------------------- */
  if (Python_owns_MPI && MPI_Init(argc,argv) != MPI_SUCCESS) {
    fprintf(stderr, "%s: Failure on MPI_Init()\n",
            (*argv)[0]?(*argv)[0]:"???");
    MPI_Abort(MPI_COMM_WORLD,1);
  }

  /* ----------------------------------------------- */
  /* See if we have a color or key assigned to us */
  /* ----------------------------------------------- */
  for(i=1;i<(*argc);++i) {
    if ( (*argv)[i][0] != '-' ) break; /* Skip any real args */
    if ( strncmp((*argv)[i],"-color=",7) == 0 ) {
      color = atoi((*argv)[i]+7);
      /* Shift args over 1 position to the left */
      for(;i<(*argc);++i) {
        (*argv)[i] = (*argv)[i+1];
      }
      (*argc)--;
    }
  }

  /* ----------------------------------------------- */
  /* Call Python normally after activating pyMPI     */
  /* ----------------------------------------------- */
  pyMPI_initialize(Python_owns_MPI,comm,color);
  status =  Py_Main(*argc, *argv);

  /* ----------------------------------------------- */
  /* Shut down MPI                                   */
  /* ----------------------------------------------- */
#ifdef HAVE_MPI_FINALIZED
  MPI_Finalized(&finalized);
#else
  finalized = 0;
#endif

  if ( Python_owns_MPI && !finalized ) {
    MPI_Finalize();
  }

  return status;
}

/**************************************************************************/
/* GLOBAL **************         pyMPI_Main        ************************/
/**************************************************************************/
/* This is a simpler version of the pyMPI main that defaults to the WORLD */
/* communicator                                                           */
/**************************************************************************/
int pyMPI_Main(int Python_owns_MPI, int *argc, char ***argv) {
  char* COLOR = 0;
  int color = -1;

  return pyMPI_Main_with_communicator(Python_owns_MPI,argc,argv,MPI_COMM_WORLD);
}

END_CPLUSPLUS

