/**************************************************************************/
/* FILE   **************      pyMPI_readline.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/**************************************************************************/
/* Replacement readline functionality so that interactive input will work */
/* properly. The rank 0 process uses the original readline pointer to     */
/* read a processed line of data.  The data are then sent to other ranks  */
/* which are waiting on a MPI_Bcast().                                    */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************      pyMPI_COMM_INPUT     ************************/
/**************************************************************************/
/* A communicator for handling readline broadcast                         */
/**************************************************************************/
MPI_Comm pyMPI_COMM_INPUT = MPI_COMM_NULL;


/**************************************************************************/
/* LOCAL  ********** original_python_readline_function ********************/
/**************************************************************************/
/* What serial Python uses as the readline pointer                        */
/**************************************************************************/
static char *(*original_python_readline_function)
#if PY_VERSION_HEX < ((2 << 24) | (3 << 16))
     Py_PROTO((char *)) = 0;
#else
     Py_PROTO((FILE *, FILE *, char *)) = 0;
#endif

/**************************************************************************/
/* LOCAL  **************      parallelReadline     ************************/
/**************************************************************************/
/* PURPOSE:  This handles interactive output using the standard mechanism */
/*           on the master which gets the buffer to broadcast to slaves.  */
/*                                                                        */
/**************************************************************************/
static char *parallelReadline
#if PY_VERSION_HEX < ((2 << 24) | (3 << 16))
#define sys_stdin stdin
#define sys_stdout stdout
(char *prompt)
#else
(FILE *sys_stdin, FILE *sys_stdout, char *prompt)
#endif
{
  char *result = 0;
  int stringSize = 0;
  int finalized = 0;
  
  COVERAGE();

  /* ----------------------------------------------- */
  /* This is a bit of a hack, but it allows proper   */
  /* recovery of the pyMPI exit status when running  */
  /* in the interpreter after getting an exception.  */
  /* ----------------------------------------------- */
  pyMPI_exit_status = 0;

  /* ----------------------------------------------- */
  /* Prepare for prompt/readline                     */
  /* Maybe need sys.stdout.flush()?                  */
  /* ----------------------------------------------- */
  fflush(sys_stdout);
  fflush(stderr);
  MPICHECK(pyMPI_COMM_INPUT, MPI_Barrier(pyMPI_COMM_INPUT));

  /* ----------------------------------------------- */
  /* When finalized, the slaves are cut off from the */
  /* input stream.                                   */
  /* ----------------------------------------------- */
#ifdef HAVE_MPI_FINALIZED
  MPICHECKCOMMLESS( MPI_Finalized(&finalized) );
#endif

#ifdef HAVE_MPC_FLUSH
  /* ----------------------------------------------- */
  /* We handle the prompt here so we can deal with   */
  /* flushing issues.                                */
  /* ----------------------------------------------- */
  if ( pyMPI_rank == 0 ) fputs(prompt,sys_stdout);
  fflush(sys_stdout);
  mpc_flush(1); /* Flush stdout.  POE call synchronizes */
  prompt = "";
#endif

#if PYMPI_PROMPT_NL
  /* ----------------------------------------------- */
  /* Force a newline to make naughty MPI's flush     */
  /* ----------------------------------------------- */
  if ( pyMPI_rank == 0 ) {
    if ( prompt && prompt[0] ) fputs(prompt,sys_stdout);
    prompt = "";
    fputs("\n",sys_stdout);
    fflush(sys_stdout);
  }
#endif

  /* ----------------------------------------------- */
  /* Master calls original function                  */
  /* ----------------------------------------------- */
  Assert(original_python_readline_function);
  Assert(prompt);
  if (pyMPI_rank == 0) {

#if PY_VERSION_HEX < ((2 << 24) | (3 << 16))
    result /*owned*/ = original_python_readline_function(prompt);
#else
    result /*owned*/ = original_python_readline_function(sys_stdin,sys_stdout,prompt);
#endif

    if ( finalized ) {
      return result;
    }

    if (!result) {
      stringSize = PYMPI_BROADCAST_EOT; /* i.e. No Data */
      MPICHECK(pyMPI_COMM_INPUT,
               MPI_Bcast(&stringSize, 1, MPI_INT, 0, pyMPI_COMM_INPUT));
    } else {
      stringSize = strlen(result);
      MPICHECK(pyMPI_COMM_INPUT,
               MPI_Bcast(&stringSize, 1, MPI_INT, 0, pyMPI_COMM_INPUT));
      MPICHECK(pyMPI_COMM_INPUT,
               MPI_Bcast(result, stringSize + 1, MPI_CHAR, 0,
                         pyMPI_COMM_INPUT));
    }
  }

  /* ----------------------------------------------- */
  /* Slaves will be making calls 1-1 with master     */
  /* ----------------------------------------------- */
  else {
    if ( finalized ) {
      return 0;
    }

    /* ----------------------------------------------- */
    /* If an input hook is registered (gist, tk), we   */
    /* need to invoke it because we otherwise do not   */
    /* call the normal readline function here.         */
    /* ----------------------------------------------- */
    if (PyOS_InputHook != NULL) {
      (void)(PyOS_InputHook)();
    }

    /* Figure out the buffer size */
    MPICHECK(pyMPI_COMM_INPUT,
             MPI_Bcast(&stringSize, 1, MPI_INT, 0, pyMPI_COMM_INPUT));
    if (stringSize < 0) {
      result = 0;               /* Error of some sort */
    } else {
      /* ----------------------------------------------- */
      /* Original 'readline.c' from Python says...       */
      /* We must return a buffer allocated with          */
      /* PyMem_Malloc.                                   */
      /* ----------------------------------------------- */
      result = (char *) PyMem_Malloc(stringSize + 1);
      if ( !result ) {
        return 0;
      }
      MPICHECK(pyMPI_COMM_INPUT,
               MPI_Bcast(result, stringSize + 1, MPI_CHAR, 0,
                         pyMPI_COMM_INPUT));
    }
  }
  return result;

pythonError:
  if (result) {
    PyMem_Free(result);
  }
  return 0;
}

/**************************************************************************/
/* GLOBAL **************    pyMPI_readline_init    ************************/
/**************************************************************************/
/* Set up for parallel readline on stdin                                  */
/**************************************************************************/
void pyMPI_readline_init(PyObject** docStringP) {
  PyObject* readline = 0;

  COVERAGE();

  /* ----------------------------------------------- */
  /* If readline exists, make sure we import it here */
  /* before we choose our readline implementation.   */
  /* We must ignore errors here                      */
  /* ----------------------------------------------- */
  if ( isatty(fileno(stdin)) ) {
    readline /*owned*/ = PyImport_ImportModule("readline");
    PyErr_Clear();
    Py_XDECREF(readline);
    readline = 0;
  }

  /* ----------------------------------------------- */
  /* We need a communicator dedictated to input      */
  /* ----------------------------------------------- */
  MPICHECK(MPI_COMM_WORLD,
           MPI_Comm_dup(MPI_COMM_WORLD, &pyMPI_COMM_INPUT));

  /* ----------------------------------------------- */
  /* We subvert the input stream to handle broadcast */
  /* ----------------------------------------------- */
  original_python_readline_function = PyOS_ReadlineFunctionPointer;
  if (!original_python_readline_function) {
    COVERAGE();
    original_python_readline_function = PyOS_StdioReadline;
  }

  PyOS_ReadlineFunctionPointer = parallelReadline;
 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************    pyMPI_readline_fini    ************************/
/**************************************************************************/
/* Release the input communicator                                         */
/**************************************************************************/
void pyMPI_readline_fini(void) {
  if ( pyMPI_COMM_INPUT != MPI_COMM_NULL ) {
    MPI_Comm_free(&pyMPI_COMM_INPUT);
  }
}

END_CPLUSPLUS

