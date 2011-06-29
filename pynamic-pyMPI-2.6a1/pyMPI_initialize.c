/**************************************************************************/
/* FILE   **************     pyMPI_initialize.c    ************************/
/**************************************************************************/
/* Author: Patrick Miller April 15 2003                                   */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* All things related to startup.                                         */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************           dummy           ************************/
/**************************************************************************/
/* Do NOT remove this dummy assignment!  It forces our library to load    */
/* with the locally defined isatty() and not the system one!!!            */
/**************************************************************************/
static void* dummy = (void*)isatty; /* Keep this! */

/**************************************************************************/
/* GLOBAL **************     world_communicator    ************************/
/**************************************************************************/
/* This is the world communicator (normally MPI_COMM_WORLD)               */
/**************************************************************************/
MPI_Comm pyMPI_world_communicator = MPI_COMM_NULL;

/**************************************************************************/
/* GLOBAL **************        pyMPI_color        ************************/
/**************************************************************************/
/* This is the color associated with a MPMD run                           */
/**************************************************************************/
int pyMPI_color = MPI_UNDEFINED;

/**************************************************************************/
/* GLOBAL **************       pyMPI_owns_MPI      ************************/
/**************************************************************************/
/* Flag to indicate whether or not pyMPI is responsible for MPI shutdown  */
/**************************************************************************/
int pyMPI_owns_MPI = 0;

/**************************************************************************/
/* GLOBAL **************      pyMPI_initialize     ************************/
/**************************************************************************/
/*  Add items to Python startup table and otherwise get ready to run      */
/**************************************************************************/
void pyMPI_initialize(int Python_owns_MPI,MPI_Comm world_communicator,int color) {
  if ( color != MPI_UNDEFINED ) {
    if ( MPI_Comm_split(world_communicator,color,0,&pyMPI_world_communicator)
         != MPI_SUCCESS ) {
      fprintf(stderr,"Failure on MPI_Comm_split()\n");
      MPI_Abort(MPI_COMM_WORLD,1);
    }
  } else {
    pyMPI_world_communicator = world_communicator;
  }
  pyMPI_color = color;

  pyMPI_owns_MPI = Python_owns_MPI;

  /* This is the MPI module */
  PyImport_AppendInittab("mpi",pyMPI_init);

  /* User is free to add stuff too */
  pyMPI_user_startup();

}


END_CPLUSPLUS
