/**************************************************************************/
/* FILE   **************       pyMPI_linker.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller April 22 2003					  */
/* Copyright (C) 2003 University of California Regents			  */
/**************************************************************************/
/*  */
/**************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "pyMPI_Config.h"

#ifdef STDC_HEADERS
#include <stdlib.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

  char* flags[] = { PYMPI_LINKER_FLAGS, 0 };

int main(int argc, char** argv) {
  char** new_argv = 0;
  char buffer[4096];
  char *p;
  char **pp;
  char *q;
  char *arg;
  int i;
  int state;

  new_argv = (char**)malloc((argc+1000)*sizeof(char**));
  for(i=0;i<argc-1;++i) {
    new_argv[i] = argv[i+1];
  }
  new_argv[0] = PYMPI_LINKER; /* Use the mpi compiler here */

  /* ----------------------------------------------- */
  /* Add in the -L<directory> where -lpyMPI is       */
  /* ----------------------------------------------- */
  new_argv[i] = (char*)malloc(strlen(PYMPI_LIBDIR)+3);
  strcpy(new_argv[i],"-L");
  strcat(new_argv[i],PYMPI_LIBDIR);
  i++;

  /* ----------------------------------------------- */
  /* Walk through the link flags and pick up all the */
  /* -l flags to stick them on the end...            */
  /* ----------------------------------------------- */
  for(pp=flags; *pp; ++pp) {
    p = *pp;
  state = 0;
  for(q=buffer;*p;++p) {
    if (p[0] == '-' && (p[1] == 'l' || p[1] == 'L')) state = 1;
    if ( p[0] == ' ' ) state = 0;

    if ( state && (q-buffer)<4095 ) {
      *q++ = *p;
    } else {
      *q = 0;
      if ( q != buffer ) {
        arg = (char*)malloc(strlen(buffer)+1);
        strcpy(arg,buffer);
        new_argv[i++] = arg;
        q = buffer;
        buffer[0] = 0;
      }
    }
    }
  }

  new_argv[i++] = 0;

  printf("Running: ");
  for(i=0;i<argc-1;++i) {
    printf("%s ",new_argv[i]);
  }
  putchar('\n');
  if ( execvp(new_argv[0],new_argv) ) {
    perror(new_argv[0]);
  }

  exit(1);
}

#ifdef __cplusplus
}
#endif

