/**************************************************************************/
/* FILE   **************     pyMPI_reductions.c    ************************/
/**************************************************************************/
/* Author: Patrick Miller May 17 2002                                     */
/**************************************************************************/
/* Information for handling built in MPI_op types and operations.         */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

#include <stdlib.h>

START_CPLUSPLUS

/**************************************************************************/
/* LOCAL  **************   reduction_information   ************************/
/**************************************************************************/
/* An array of descriptions for MPI reduction operations.  It includes    */
/* the MPI_Op value, the Python function that implements it, and the      */
/* name of the operation.                                                 */
/**************************************************************************/
static struct reduction_information { MPI_Op op; PyObject* function; char* name; } *reduction_map = 0;
static unsigned int reduction_map_size = 0;

/**************************************************************************/
/* LOCAL  **************          compare          ************************/
/**************************************************************************/
/* Reduction information comparision function for qsort()                 */
/**************************************************************************/
static int compare(const void* voidA, const void* voidB) {
  struct reduction_information* A = (struct reduction_information*)voidA;
  struct reduction_information* B = (struct reduction_information*)voidB;
  Assert(A);
  Assert(B);
  /* Note: We want to do ((long)A->op) - ((long)B->op), but we have overflow issues */
  if ( A->op <  B->op ) return -1;
  if ( A->op == B->op ) return  0;
  return 1;
}

/**************************************************************************/
/* GLOBAL **************  pyMPI_reductions_python  ************************/
/**************************************************************************/
/* Returns a borrowed reference to the function that implements pyMPI     */
/* reduction operation.                                                   */
/**************************************************************************/
PyObject* pyMPI_reductions_python(MPI_Op op) {
  int lo = 0;
  int hi = reduction_map_size-1;
  int mid;
  
  Assert(reduction_map);
  while (lo <= hi) {
    mid = (lo+hi)/2;
    if ( reduction_map[mid].op == op ) {
      Assert(reduction_map[mid].function);
      return reduction_map[mid].function;
    } else if ( reduction_map[mid].op > op ) {
      hi = mid - 1;
    } else {
      lo = mid + 1;
    }
  }
  return 0;
}

/**************************************************************************/
/* LOCAL  **************     registerReduction     ************************/
/**************************************************************************/
/* This routine builds a local mapping to the MPI reduction parameters    */
/* This was done in part because MPI implementations are free to use ANY  */
/* representation for their reductions (e.g. pointers) and we have a      */
/* requirement to represent them as integers to Python and to test for    */
/* validity when using in reductions.                                     */
/**************************************************************************/
static void registerReduction(MPI_Op op,
                              char *name, char *doc, char *function_body,
                              PyObject ** docStringP)
{
  PyObject *scratch = 0;
  PyObject *status = 0;
  PyObject *function = 0;
  static PyObject *global_dict = 0;

  COVERAGE();

  /* ----------------------------------------------- */
  /* Insert value into dictionary and doc string     */
  /* ----------------------------------------------- */
  PARAMETER((long)op,name,doc,PyInt_FromLong,pyMPI_dictionary,docStringP);

  /* ----------------------------------------------- */
  /* Build function object to do serialized reduce   */
  /* ----------------------------------------------- */
  PYCHECK(scratch /*owned*/ = PyDict_New());
  if (!global_dict) {
    PyObject *builtin = 0;
    PYCHECK(builtin /*borrowed*/ = PyImport_ImportModule("__builtin__"));
    global_dict /*borrowed */  = PyModule_GetDict(builtin);
  }
  PYCHECK(status /*owned*/ =
          PyRun_String(function_body, Py_file_input, global_dict,
                       scratch));
  Py_XDECREF(status);
  status = 0;

  /* ----------------------------------------------- */
  /* Make room for the entry in the reduction map    */
  /* ----------------------------------------------- */
  if ( !reduction_map ) {
    reduction_map = (struct reduction_information*)malloc(sizeof(struct reduction_information));
  }
  Assert(reduction_map);

  reduction_map = (struct reduction_information*)realloc(reduction_map,(1+reduction_map_size)*sizeof(struct reduction_information));
  Assert(reduction_map);

  /* ----------------------------------------------- */
  /* Find the Python function and insert it into map */
  /* ----------------------------------------------- */
  PYCHECK(function /*owned*/ = PyDict_GetItemString(scratch, name));
  Assert(function);
  reduction_map[reduction_map_size].op       = op;
  reduction_map[reduction_map_size].function = function;
  reduction_map[reduction_map_size].name     = name;
  reduction_map_size++;

  /* ----------------------------------------------- */
  /* We maintain the list in sorted order so we can  */
  /* do a binary search of the table.                */
  /* ----------------------------------------------- */
  qsort(reduction_map,reduction_map_size,sizeof(struct reduction_information),compare);
  return;

pythonError:
  Py_XDECREF(scratch);
  return;
}

/**************************************************************************/
/* GLOBAL **************   pyMPI_reductions_init   ************************/
/**************************************************************************/
/* This initializes the tables of reduction operations                    */
/**************************************************************************/
void pyMPI_reductions_init(PyObject** docStringP) {
  COVERAGE();

  PYCHECK( registerReduction(MPI_MAX,
                             "MAX",
                             "Max reduction",
                             "MAX = max",docStringP) );
  PYCHECK( registerReduction(MPI_MIN,
                             "MIN",
                             "Min reduction",
                             "MIN = min",docStringP) );
  PYCHECK( registerReduction(MPI_SUM,
                             "SUM",
                             "Sum reduction",
                             "def SUM(a,b): return a+b",docStringP) );
  PYCHECK( registerReduction(MPI_PROD,
                             "PROD",
                             "Prod reduction",
                             "def PROD(a,b): return a*b",docStringP) );
  PYCHECK( registerReduction(MPI_LAND,
                             "LAND",
                             "Land reduction",
                             "def LAND(a,b):\n  if a and b:    return 1\n  else:\n    return 0\n",docStringP) );
  PYCHECK( registerReduction(MPI_BAND,
                             "BAND",
                             "Band reduction",
                             "def BAND(a,b): return a & b",docStringP) );
  PYCHECK( registerReduction(MPI_LOR,
                             "LOR",
                             "Lor reduction",
                             "def LOR(a,b):\n  if a or b:    return 1\n  else:\n    return 0\n",docStringP) );
  PYCHECK( registerReduction(MPI_BOR,
                             "BOR",
                             "Bor reduction",
                             "def BOR(a,b): return a | b",docStringP) );
  PYCHECK( registerReduction(MPI_LXOR,
                             "LXOR",
                             "Lxor reduction",
                             "def LXOR(a,b):\n  if a: return not b\n  if b: return 1\n  return 0\n",docStringP) );
  PYCHECK( registerReduction(MPI_BXOR,
                             "BXOR",
                             "Bxor reduction",
                             "def BXOR(a,b): return a ^ b",docStringP) );
  PYCHECK( registerReduction(MPI_MINLOC,
                             "MINLOC",
                             "Maxloc reduction",
                             "def MINLOC(a,b):\n  assert type(a) == type(()),'Ground must be a tuple'\n  if b[0] < a[0]: return b\n  return a\n",docStringP) );
  PYCHECK( registerReduction(MPI_MAXLOC,
                             "MAXLOC",
                             "Maxloc reduction",
                             "def MAXLOC(a,b):\n  assert type(a) == type(()),'Ground must be a tuple'\n  if b[0] > a[0]: return b\n  return a\n",docStringP) );
  
 pythonError:
  return;
}

/**************************************************************************/
/* GLOBAL **************   pyMPI_reductions_fini   ************************/
/**************************************************************************/
/* Cleanup work for the reduction plugin includes deleting the reduction  */
/* map.                                                                   */
/**************************************************************************/
void pyMPI_reductions_fini(void) {
  if ( reduction_map ) free(reduction_map);
  reduction_map_size = 0;
}

END_CPLUSPLUS

