#include <stdlib.h>
#include <mpi.h>
#include <Python.h>

#if !defined(STR)
#define XSTR(X) #X
#define STR(X) XSTR(X)
#endif

#if !defined(ROOT_DIR)
#error ROOT_DIR not defined
#endif

int main(int argc, char *argv[])
{
   char *orig_pythonpath, *pythonpath;
   unsigned long len;
   int rank;

   orig_pythonpath = getenv("PYTHONPATH");
   if (!orig_pythonpath) {
      pythonpath = STR(ROOT_DIR);
   }
   else {
      len = strlen(orig_pythonpath) + strlen(STR(ROOT_DIR)) + 3;
      pythonpath = (char *) malloc(strlen(orig_pythonpath) + strlen(STR(ROOT_DIR)) + 3);
      snprintf(pythonpath, len, "%s:%s", orig_pythonpath, STR(ROOT_DIR));
   }
   setenv("PYTHONPATH", pythonpath, 1);
   
   MPI_Init(&argc, &argv);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);
   printf("rank - %d\n", (int) rank);
   Py_Initialize();
   PyRun_SimpleString("import pynamic_driver_mpi4py\n");
   Py_Finalize();

   MPI_Finalize();
   return 0;
}
