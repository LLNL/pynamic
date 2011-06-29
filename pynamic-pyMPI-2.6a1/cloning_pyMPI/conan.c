#include "mpi.h"
#include "Python.h"

static PyMethodDef methods[] = { {0,0} };

static void inithyborian(void) {
  Py_InitModule("hyborian",methods);
}

static void initvalusia(void) {
  Py_InitModule("valusia",methods);
}

static void initthuria(void) {
  Py_InitModule("thuria",methods);
}

static void initconan(void) {
  PyObject* conan = 0;

  /* Init the conan module with an @tribute */
  conan = Py_InitModule("conan",methods);
  PyObject_SetAttrString(conan,"robert",PyString_FromString("howard"));

  /* You can other kinds of init here */
  inithyborian();
  initvalusia();
  initthuria();
}

#ifdef _cplusplus
extern "C" {
#endif

  /* The Prototype is in pyMPI.h or just insert directly */
  extern int pyMPI_Main(int Python_owns_MPI,int* argc_ptr,char*** argv_ptr);

  /**************************************************************************/
  /* GLOBAL **************     pyMPI_user_startup    ************************/
  /**************************************************************************/
  /* Specify any modules you want to be able to load _on demand_            */
  /**************************************************************************/
  void pyMPI_user_startup(void) {
    PyImport_AppendInittab("conan",initconan);
  }

  /**************************************************************************/
  /* GLOBAL **************        pyMPI_banner       ************************/
  /**************************************************************************/
  /* Set part of startup banner (best to put in parenthesis)                */
  /* Use strcpy or strcat (but watch the size)                              */
  /**************************************************************************/
  void pyMPI_banner(char* banner, int n ) {
    // You can set the startup banner here
    strcpy(banner,"(CONAN v0.1 beta)");
  }

#ifdef _cplusplus
}
#endif

/**************************************************************************/
/* GLOBAL **************            main           ************************/
/**************************************************************************/
/* % make                                                                 */
/* % ./conan                                                              */
/* Python 2.2.2 (CONAN v0.1 beta) on linux2                               */
/* Type "help", "copyright", "credits" or "license" for more information. */
/* >>> import conan                                                       */
/* >>> conan.robert                                                       */
/* 'howard'                                                               */
/* >>> import valusia                                                     */
/* >>> import thuria                                                      */
/* >>>                                                                    */
/**************************************************************************/
int main(int argc, char **argv) {
  int status;

  // Put any special init like PETSC here
  if ( MPI_Init(&argc,&argv) != MPI_SUCCESS ) {
    exit(1);
  }

  /* 0 indicates you want to shut down MPI */
  status = pyMPI_Main(0,&argc,&argv);

  MPI_Finalize();

  return status;
}


