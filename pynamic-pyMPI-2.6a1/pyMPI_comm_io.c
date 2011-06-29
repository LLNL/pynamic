/**************************************************************************/
/* FILE   **************      pyMPI_comm_io.c      ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/**************************************************************************/
/*                                                                        */
/* >>> from mpi import *                                                  */
/* >>> import mpi                                                         */
/*                                                                        */
/**************************************************************************/


#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

typedef enum { NO_ADD_NEWLINE, ADD_NEWLINE } kind_of_write;

/**************************************************************************/
/* LOCAL  **************        empty_tuple        ************************/
/**************************************************************************/
/* An empty Python Tuple object ()                                        */
/**************************************************************************/
static PyObject* empty_tuple = 0;

/**************************************************************************/
/* LOCAL  ************** synchronize_system_stream ************************/
/**************************************************************************/
/* Locally pull any queued data from the file and send to master          */
/**************************************************************************/
static void synchronize_system_stream(PyMPI_Comm* self,char* sys_name,char* __sys_name__,PyObject* reopen_as) {
  PyObject* ospath = 0;
  PyObject* full_name = 0;
  PyObject* sys_stream = 0;
  PyObject* default_stream = 0;
  PyObject* new_stream = 0;
  PyObject* name = 0;
  PyObject* queued = 0;
  PyObject* data = 0;
  PyObject* input = 0;
  char* old_name = 0;
  char* new_name = 0;
  int i;
  char buffer[4096];

  /* ----------------------------------------------- */
  /* Grab appropriate output file                    */
  /* ----------------------------------------------- */
  PYCHECK( sys_stream/*borrow*/ = PySys_GetObject(sys_name) );
  Py_INCREF(sys_stream);/*owned*/

  /* ----------------------------------------------- */
  /* For the default, we have nothing to output      */
  /* ----------------------------------------------- */
  PYCHECK( queued/*owned*/ = PyString_FromString("") );
  if ( self->rank != 0 ) {

    /* ----------------------------------------------- */
    /* If we have queued information, read it          */
    /* ----------------------------------------------- */
    PYCHECK( name/*borrow*/= PyFile_Name(sys_stream) );
    if ( name && PyString_Check(name) && PyString_AS_STRING(name)[0] == '/' ) {
      /* ----------------------------------------------- */
      /* Flush and close old output                      */
      /* ----------------------------------------------- */
      PYCHECK( PyObject_CallMethod(sys_stream,"flush","") );
      PYCHECK( PyObject_CallMethod(sys_stream,"close","") );

      /* ----------------------------------------------- */
      /* Set output to default (changed below if needed) */
      /* so if we fail, output is open                   */
      /* ----------------------------------------------- */
      PYCHECK( default_stream/*borrow*/ = PySys_GetObject(__sys_name__) );
      Py_INCREF(default_stream);
      PYCHECK( PySys_SetObject(sys_name,/*noinc*/default_stream) );
      default_stream = 0;

      /* ----------------------------------------------- */
      /* Read in all the queued information              */
      /* ----------------------------------------------- */
      old_name/*borrow*/ = PyString_AS_STRING(name);
      PYCHECK( input/*owned*/ = PyFile_FromString(old_name,"r") );
      Py_DECREF(queued);
      queued = 0;
      PYCHECK( queued/*owned*/ = PyObject_CallMethod(input,"read","") );
      PYCHECK( PyObject_CallMethod(input,"close","") );
      Py_DECREF(input);
      input = 0;
    }

    /* ----------------------------------------------- */
    /* Here, we either                                 */
    /* 1) Restore to original (reopen_as is None)      */
    /* 2) Reopen to the old file (reopen_as == 0)      */
    /* 3) Reopen to a new file (reopen_as is string)   */
    /* ----------------------------------------------- */
    if ( reopen_as == Py_None ) {
      new_name/*borrow*/ = 0;
      PYCHECK( default_stream/*borrow*/ = PySys_GetObject(__sys_name__) );
      Py_INCREF(default_stream);/*owned*/
      PYCHECK( PySys_SetObject(sys_name,/*noinc*/default_stream) );
      default_stream = 0;
    } else if ( reopen_as ) {
      PYCHECK( ospath/*owned*/ = PyImport_ImportModule("os.path") );
      PYCHECK( full_name/*owned*/ = PyObject_CallMethod(ospath,"abspath","O",reopen_as) );
      PYCHECK( new_name/*borrow*/ = PyString_AsString(full_name) );
      if ( strcmp(new_name,"/dev/null") != 0 ) {
        sprintf(buffer,"%s.%s.%d",new_name,sys_name+3,self->rank);
        new_name/*borrow*/ = buffer;
      }
    } else {
      new_name/*borrow*/ = old_name;
    }

    if ( new_name ) {
      PYCHECK( new_stream/*owned*/ = PyFile_FromString(new_name,"w") );
      PYCHECK( PySys_SetObject(sys_name,/*noincs*/new_stream) );
      new_stream = 0;
    }
  } 

  /* ----------------------------------------------- */
  /* Everyone sends queued data to master            */
  /* ----------------------------------------------- */
  data = pyMPI_collective(self,0, queued,0);
  MPI_Barrier(MPI_COMM_WORLD);

  /* ----------------------------------------------- */
  /* Master writes data to stream                    */
  /* ----------------------------------------------- */
  if ( self->rank == 0 ) {
    Assert(data && PyTuple_Check(data));
    for(i=1;i<self->size;++i) {
      PYCHECK( PyFile_WriteObject(PyTuple_GET_ITEM(data,i),sys_stream,1) );
    }
    Py_DECREF(data);
    data = 0;
  }

  /* fall through */
 pythonError:
  Py_XDECREF(ospath);
  Py_XDECREF(full_name);
  Py_XDECREF(sys_stream);
  Py_XDECREF(default_stream);
  Py_XDECREF(new_stream);
  Py_XDECREF(queued);
  Py_XDECREF(data);
  Py_XDECREF(input);
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_io_synchronizeQueuedOutput ****************/
/**************************************************************************/
/* Get sys.stdout and sys.stderr to behave                                */
/**************************************************************************/
/* Control output/errput on various ranks on a communicator. Synchronous  */
/*                                                                        */
/* synchronizeQueuedOutput()                                              */
/* synchronizeQueuedOutput(stdout=string | None, stderr = string or None) */
/*                                                                        */
/* Use this to change output methodology. All variants will flush any     */
/* previously queued output on stdout or stderr through root's outputs.   */
/* This sync is done by messaging the contents of the data queue (a file  */
/* on disk) to the master (rank 0) process which will output to its       */
/* sys.stdout or sys.stderr.   The initial mechanism queues no data       */
/*                                                                        */
/* There a number of variants that allow the user to change where output  */
/* will go for output operations that follow.                             */
/*                                                                        */
/* To queue stdout on slave processes to foo.out.1, foo.out.2, ...        */
/* >>> synchronizeQueuedOutput('foo')                                     */
/* If you never call sQO again, then all output is diverted to the files  */
/* If you DO call sQO again..                                             */
/* >>> synchronizeQueuedOutput('foo')                                     */
/* or                                                                     */
/* >>> synchronizeQueuedOutput()                                          */
/* then the contents of foo.out.1, foo.out.2, etc... are messaged to      */
/* rank0 which will output them, and the files are opened as empty files  */
/*                                                                        */
/* If you only want to do that to stderr,                                 */
/* >>> synchronizeQueuedOutput(stderr='foo')                              */
/*                                                                        */
/* If you want to throw away output on the slaves...                      */
/* >>> synchronizeQueuedOutput('/dev/null')                               */
/*                                                                        */
/* If you want to turn on unrestricted output again                       */
/* >>> synchronizeQueuedOutput(stdout=None,stderr=None)                   */
/*                                                                        */
/* If you simply want to sync any output that may be queued, then call    */
/* with no arguments.                                                     */
/*                                                                        */
/* Even though calls are synchronous, each rank can have its own output   */
/* model (e.g. rank 1 can divert to /dev/null, rank 7 queues to a file,   */
/* rank 93 just prints without queuing                                    */
/*                                                                        */
/* Examples:                                                              */
/* >>> synchronizeQueuedOutput() #Just synchronize however it was done    */
/*                               # last time.                             */
/* >>> synchronizeQueuedOutput('/dev/null','/dev/null') # Slave proc pipe */
/*                               # subsequent output to /dev/null         */
/* synchronizeQueuedOutput('foo',None) # Slave processors route           */
/*                               # subsequent output on stdout to file    */
/*                               # foo.out.<procid> and subsequent output */
/*                               # on sterr to original stderr            */
/* synchronizeQueuedOutput('std','std') # Slave processors route output   */
/*                               # to temporary holding files of the form */
/*                               # std.out.<procid> and std.err.<procid>. */
/*                               # These files are deleted and reopened   */
/*                               # on synchronization.                    */
/**************************************************************************/
PyObject* pyMPI_io_synchronizeQueuedOutput(PyObject* pySelf, PyObject* args, PyObject* kw) {

  /* TODO: build synchronize() method */
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  PyObject* sysout = 0;
  PyObject* syserr = 0;
  static char* kwlist[] = {"stdout","stderr",0};

  COVERAGE();
#if 0
  DEPRECATED("Use file synchronize() method");
#endif


  /* ----------------------------------------------- */
  /* MPI better be in ready state                    */
  /* ----------------------------------------------- */
  RAISEIFNOMPI();
  Assert(self);

  /* ----------------------------------------------- */
  /* If we have arguments, we will reopen or reset   */
  /* the appropriate stream                          */
  /* ----------------------------------------------- */
  PYCHECK( PyArg_ParseTupleAndKeywords(args,kw,"|OO:synchronizedQueuedOutput",kwlist,&sysout,&syserr) );

  /* ----------------------------------------------- */
  /* With no arguments, synchronize both and we're   */
  /* done. With one or both, sync only those streams */
  /* ----------------------------------------------- */
  if ( sysout == 0 && syserr == 0 ) {
    PYCHECK( synchronize_system_stream(self,"stdout","__stdout__",0) );
    PYCHECK( synchronize_system_stream(self,"stderr","__stderr__",0) );
  }

  if ( sysout != 0 ) {
    PYCHECK( synchronize_system_stream(self,"stdout","__stdout__",sysout) );
  }

  if ( syserr != 0 ) {
    PYCHECK( synchronize_system_stream(self,"stderr","__stderr__",syserr) );
  }


  Py_INCREF(Py_None);
  return Py_None;

 pythonError:
  return 0;
}

/**************************************************************************/
/* LOCAL  **************    io_synchronizedWrite   ************************/
/**************************************************************************/
/* The actual guts of sychronized write.  This handles whether to append  */
/* a newline to each line of the input or not.                            */
/**************************************************************************/
static PyObject* io_synchronizedWrite(PyObject* pySelf, PyObject* args, PyObject* kw,kind_of_write newline_flag) {
  PyMPI_Comm* self = (PyMPI_Comm*)pySelf;
  PyObject* line = 0;
  PyObject* newLine = 0;
  PyObject* space = 0;
  int i;
  int rank = 0;
  PyObject* arg = 0;
  PyObject* str = 0;
  PyObject* sysout = 0;
  PyObject* resultWrite = 0;
  PyObject* resultFlush = 0;
  PyObject* messages = 0;
  PyObject* message = 0;

  /* ----------------------------------------------- */
  /* Prepare a string with space separated strings   */
  /* of all of the arguments (may be empty)          */
  /* ----------------------------------------------- */
  PYCHECK( line = PyString_FromString("") );
  PYCHECK( space = PyString_FromString(" ") );
   
  Assert( PyTuple_Check(args) );
  for(i=0;i<PyTuple_GET_SIZE(args);++i) {
    arg = PyTuple_GET_ITEM(args,i);

    /* ----------------------------------------------- */
    /* Add a space                                     */
    /* ----------------------------------------------- */
    if ( i > 0 ) {
      PYCHECK( newLine = PySequence_Concat(line,space) );
      Py_DECREF(line);
      line = newLine;
      newLine = 0;
    }

    /* ----------------------------------------------- */
    /* Add a string                                    */
    /* ----------------------------------------------- */
    PYCHECK( str = PyObject_Str(arg) );
    PYCHECK( newLine = PySequence_Concat(line,str) );

    Py_DECREF(str);
    Py_DECREF(line);
    line = newLine;
    newLine = 0;
  }

  /* ----------------------------------------------- */
  /* The master collects and writes while the slaves */
  /* send pieces                                     */
  /* ----------------------------------------------- */
  Assert( self );
  PYCHECK( messages /*owned*/ = pyMPI_collective(self,0,line,0) );
   
  /* ----------------------------------------------- */
  /* Write a string for each processor (including    */
  /* the master).  Flush when done.                   */
  /* ----------------------------------------------- */
  if ( self->rank == 0 ) {
    static char *kwlist[] = {"file", 0};
    /* ----------------------------------------------- */
    /* TODO: address this LEAK by adding to fini list  */
    /* We need to check for a dictionary with exactly  */
    /* file=XXXXX                                      */
    /* ----------------------------------------------- */
    if ( empty_tuple == 0 ) empty_tuple = PyTuple_New(0);

    PYCHECK( sysout /*borrowed*/ = PySys_GetObject("stdout") );
    if ( kw ) PYCHECK( PyArg_ParseTupleAndKeywords(empty_tuple, kw, "|O", kwlist, &sysout) );

    /* ----------------------------------------------- */
    /* Write lines out in rank order                   */
    /* ----------------------------------------------- */
    Assert( PyTuple_Check(messages) );
    for(rank=0; rank<self->size; ++rank) {
      message /*borrowed*/ = PyTuple_GET_ITEM(messages,rank);
      PYCHECK( resultWrite = PyObject_CallMethod(sysout,"write","O",message) );
      Py_DECREF(resultWrite); resultWrite = 0;
      if ( newline_flag == ADD_NEWLINE ) {
        PYCHECK( resultWrite = PyObject_CallMethod(sysout,"write","s","\n") );
        Py_DECREF(resultWrite); resultWrite = 0;
      }
      message = 0;
    }

    PYCHECK( resultFlush = PyObject_CallMethod(sysout,"flush","") );
    Py_DECREF(resultFlush); resultFlush = 0;
  }

  /* ----------------------------------------------- */
  /* Throw a barrier to ensure all release together  */
  /* ----------------------------------------------- */
  MPICHECK(self->communicator,
           MPI_Barrier(self->communicator));

  Py_DECREF(line);
  Py_DECREF(space);
  Py_XDECREF(messages);

  Py_XINCREF(Py_None);
  return Py_None;

 pythonError:
  Py_XDECREF(line);
  Py_XDECREF(newLine);
  Py_XDECREF(space);
  Py_XDECREF(resultWrite);
  Py_XDECREF(resultFlush);
  Py_XDECREF(messages);
  return 0;
}

/**************************************************************************/
/* GMETHOD ************* pyMPI_io_synchronizedWriteln *********************/
/**************************************************************************/
/* Convenience function to write in rank order                            */
/**************************************************************************/
/* See synchronizedWrite().  This is identical, but adds a newline        */
/**************************************************************************/
PyObject* pyMPI_io_synchronizedWriteln(PyObject* pySelf, PyObject* args, PyObject* kw) {
  return io_synchronizedWrite(pySelf,args,kw,ADD_NEWLINE);
}

/**************************************************************************/
/* GMETHOD ************** pyMPI_io_synchronizedWrite **********************/
/**************************************************************************/
/* Convenience function to write in rank order                            */
/**************************************************************************/
/* Write strings to a file (stdout) in rank order                         */
/*                                                                        */
/* synchronizedWrite(...,file=sys.stdout) --> None                        */
/*                                                                        */
/* For arbitrary arguments, prepare a space separated representation of   */
/* the inputs and message to the master.  Messages are guaranteed to be   */
/* printed in rank order.  All processors must call.  The file argument   */
/* may be any file or file-like object such as an instance with a         */
/* .write(s) member or pipe. Only the master rank actually uses the file  */
/* argument.  The file argument is ignored elsewhere.                     */
/*                                                                        */
/* Beware of the race condition inherent if you try to invoke something   */
/* like mpi.sychronizedWrite(file=open('foo','w')) -- all processes try   */
/* to create 'foo', but only one wins.  The winner may NOT be the root    */
/* rank, so you unexpectedly get an empty file.                           */
/*                                                                        */
/* >>> synchronizedWrite('Rank',mpi.rank,'is alive\n')                    */
/* Rank 0 is alive                                                        */
/* Rank 1 is alive                                                        */
/*                                                                        */
/* >>> synchronizedWrite('Rank',mpi.rank,'is alive\n',file=sys.stderr)    */
/*                                                                        */
/* >>> if WORLD.rank == 0:                                                */
/* ...    debug = open('debug','w')                                       */
/* ... else:                                                              */
/* ...    debug = None                                                    */
/* >>> synchronizedWrite('Rank',mpi.rank,'is alive\n',file=debug)         */
/*                                                                        */
/**************************************************************************/
PyObject* pyMPI_io_synchronizedWrite(PyObject* pySelf, PyObject* args, PyObject* kw) {
  return io_synchronizedWrite(pySelf,args,kw,NO_ADD_NEWLINE);
}


END_CPLUSPLUS
