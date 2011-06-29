/**************************************************************************/
/* FILE   **************      pyMPI_signals.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller May 12 2003                                     */
/* Copyright (C) 2003 University of California Regents                    */
/**************************************************************************/
/* Handle untrapped fatal signals by calling MPI_Abort()                  */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "compile.h"
#include "frameobject.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

#include <signal.h>

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************         fix_signal        ************************/
/**************************************************************************/
/* This examines the current signal handler and replaces only if it uses  */
/* the from handler                                                       */
/**************************************************************************/
static void fix_signal(int signal,void (*from)(int,siginfo_t*,void*),void (*to)(int,siginfo_t*,void*)) {
  struct sigaction handler;
  if ( sigaction(signal,0,&handler) == 0 && handler.sa_sigaction == from ) {
    handler.sa_sigaction = to;
    if ( sigaction(signal,&handler,0) != 0 ) {
      perror("sigaction");
    }
  }
}

/**************************************************************************/
/* LOCAL  **************      change_handlers      ************************/
/**************************************************************************/
/* Change signals that may indicate process demise                        */
/**************************************************************************/
static void change_handlers(void (*from)(int,siginfo_t*,void*),void (*to)(int,siginfo_t*,void*)) {
#ifdef SIGILL
  fix_signal(SIGILL,from,to);
#endif
#ifdef SIGFPE
  fix_signal(SIGFPE,from,to);
#endif
#ifdef SIGSEGV
  fix_signal(SIGSEGV,from,to);
#endif
#ifdef SIGBUS
  fix_signal(SIGBUS,from,to);
#endif
#ifdef SIGXCPU
  fix_signal(SIGXCPU,from,to);
#endif
#ifdef SIGXFSZ
  fix_signal(SIGXFSZ,from,to);
#endif
}

/**************************************************************************/
/* LOCAL  **************        fatal_signal       ************************/
/**************************************************************************/
/* Signal handler replaces default handler for certain UNIX signals       */
/**************************************************************************/
static void fatal_signal(int signal, siginfo_t* info, void* extra) {
  int initialized = 1;
  int finalized = 0;
  struct sigaction handler;
  char* signal_name = 0;
  PyThreadState* state = 0;
  PyFrameObject* frame = 0;

  switch( signal ) {
#ifdef SIGILL
  case SIGILL: signal_name = "SIGILL"; break;
#endif
#ifdef SIGFPE
  case SIGFPE: signal_name = "SIGFPE"; break;
#endif
#ifdef SIGSEGV
  case SIGSEGV: signal_name = "SIGSEGV"; break;
#endif
#ifdef SIGBUS
  case SIGBUS: signal_name = "SIGBUS"; break;
#endif
#ifdef SIGXCPU
  case SIGXCPU: signal_name = "SIGXCPU"; break;
#endif
#ifdef SIGXFSZ
  case SIGXFSZ: signal_name = "SIGXFSZ"; break;
#endif
  default: signal_name = "???";
  }

  fprintf(stderr,"pyMPI aborting on untrapped fatal signal %d (%s) on rank %d\n",signal,signal_name,pyMPI_rank);
  state = PyThreadState_Get();
  
  if ( state ) {
    for(frame=state->frame; frame; frame = frame->f_back) {
      char* filename = "???.py";
      char* funcname = "?";
      int lineno = 0;

      if ( frame->f_code && frame->f_code->co_filename && PyString_Check(frame->f_code->co_filename)) {
        filename = PyString_AS_STRING(frame->f_code->co_filename);
      }
      if ( frame->f_code && frame->f_code->co_name && PyString_Check(frame->f_code->co_name)) {
        funcname = PyString_AS_STRING(frame->f_code->co_name);
      }
      if ( frame->f_code && ( frame->f_lasti > 0 ) ) {
        lineno = PyCode_Addr2Line(frame->f_code,frame->f_lasti);
      }
      if ( frame->f_back ) {
        fprintf(stderr,"%s:%d: function %s()\n", filename, lineno, funcname);
      } else {
        fprintf(stderr,"%s:%d: __main__\n", filename, lineno);
      }
    }
  }
  fflush(stderr);

  /* Uninstall handlers */
  change_handlers(fatal_signal,(void(*)(int,siginfo_t*,void*))SIG_DFL);

#ifdef HAVE_MPI_INITIALIZED
  MPI_Initialized(&initialized);
#endif
#ifdef HAVE_MPI_FINALIZED
  MPI_Finalized(&finalized);
#endif
  if ( initialized && !finalized ) MPI_Abort(MPI_COMM_WORLD,signal);
  abort();
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_signals_init     ***********************/
/**************************************************************************/
/* Set up default handlers for fatal signals                              */
/**************************************************************************/
void pyMPI_signals_init(PyObject** docStringP) {
  change_handlers((void(*)(int,siginfo_t*,void*))SIG_DFL,fatal_signal);
}

/**************************************************************************/
/* GLOBAL **************     pyMPI_signals_fini     ***********************/
/**************************************************************************/
/* No cleanup work                                                        */
/**************************************************************************/
void pyMPI_signals_fini(void) {
}

END_CPLUSPLUS
