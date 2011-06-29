/**************************************************************************/
/* FILE   **************     pyMPI_distutils.c     ************************/
/**************************************************************************/
/* Author: Patrick Miller January 14 2003                                 */
/**************************************************************************/
/* Here are the fixes necessary to make distutils work to build dynamic   */
/* loading libraries and the like...                                      */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS


/**************************************************************************/
/* LOCAL  **************    force_MPI_functions    ************************/
/**************************************************************************/
/* We need to force the loading of various MPI functions.                 */
/**************************************************************************/
static void force_MPI_functions(void) {
  /* ----------------------------------------------- */
  /* Note that the rank will always be non-negative  */
  /* so the if test will never execute.  This IS THE */
  /* BEHAVIOR we want.  These calls are only here to */
  /* force the loader to include these MPI calls     */
  /* Don't put in an Assert(0) or G++ will "know" it */
  /* doesn't need to load the functions since they   */
  /* are unreachable.                                */
  /* ----------------------------------------------- */
  if ( pyMPI_rank < 0 ) {

    MPI_Abort(MPI_COMM_WORLD,0);
    MPI_Address(0,0);
    MPI_Allgather(0,0,MPI_INT,0,0,MPI_INT,MPI_COMM_WORLD);
    MPI_Allgatherv(0,0,MPI_INT,0,0,0,MPI_INT,MPI_COMM_WORLD);
    MPI_Allreduce(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Alltoall(0,0,MPI_INT,0,0,MPI_INT,MPI_COMM_WORLD);
    MPI_Alltoallv(0,0,0,MPI_INT,0,0,0,MPI_INT,MPI_COMM_WORLD);
    MPI_Attr_delete(MPI_COMM_WORLD,0);
    MPI_Attr_get(MPI_COMM_WORLD,0,0,0);
    MPI_Attr_put(MPI_COMM_WORLD,0,0);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(0,0,0,0,MPI_COMM_WORLD);
    MPI_Bsend(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Bsend_init(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Buffer_attach(0,0);
    MPI_Buffer_detach(0,0);
    MPI_Cancel(0);
    MPI_Cart_coords(MPI_COMM_WORLD,0,0,0);
    MPI_Cart_create(MPI_COMM_WORLD,0,0,0,0,0);
    MPI_Cart_get(MPI_COMM_WORLD,0,0,0,0);
    MPI_Cart_map(MPI_COMM_WORLD,0,0,0,0);
    MPI_Cart_rank(MPI_COMM_WORLD,0,0);
    MPI_Cart_shift(MPI_COMM_WORLD,0,0,0,0);
    MPI_Cart_sub(MPI_COMM_WORLD,0,0);
    MPI_Cartdim_get(MPI_COMM_WORLD,0);
    MPI_Comm_compare(MPI_COMM_WORLD,MPI_COMM_WORLD,0);
    MPI_Comm_create(MPI_COMM_WORLD,0,0);
    MPI_Comm_dup(MPI_COMM_WORLD,0);
    MPI_Comm_c2f(MPI_COMM_WORLD);
    MPI_Comm_free(0);
#ifdef HAVE_MPI_COMM_NAME_OPERATIONS
    MPI_Comm_get_name(MPI_COMM_WORLD,0,0);
    MPI_Comm_set_name(MPI_COMM_WORLD,0);
#endif
    MPI_Comm_group(MPI_COMM_WORLD,0);
    MPI_Comm_rank(MPI_COMM_WORLD,0);
    MPI_Comm_remote_group(MPI_COMM_WORLD,0);
    MPI_Comm_remote_size(MPI_COMM_WORLD,0);
    MPI_Comm_size(MPI_COMM_WORLD,0);
    MPI_Comm_split(MPI_COMM_WORLD,0,0,0);
    MPI_Comm_test_inter(MPI_COMM_WORLD,0);
    MPI_Dims_create(0,0,0);
    MPI_Errhandler_create(0,0);
    MPI_Errhandler_free(0);
    MPI_Errhandler_get(MPI_COMM_WORLD,0);
    MPI_Errhandler_set(MPI_COMM_WORLD,0);
    MPI_Error_class(0,0);
    MPI_Error_string(0,0,0);
#ifdef HAVE_MPI_FILE_OPERATIONS_NOT_DONE
    MPI_File_close();
    MPI_File_delete();
    MPI_File_f2c();
    MPI_File_get_amode();
    MPI_File_get_atomicity();
    MPI_File_get_byte_offset();
    MPI_File_get_errhandler();
    MPI_File_get_group();
    MPI_File_get_info();
    MPI_File_get_position();
    MPI_File_get_position_shared();
    MPI_File_get_size();
    MPI_File_get_type_extent();
    MPI_File_get_view();
    MPI_File_iread();
    MPI_File_iread_at();
    MPI_File_iread_shared();
    MPI_File_iwrite();
    MPI_File_iwrite_at();
    MPI_File_iwrite_shared();
    MPI_File_iwrite_shared();
    MPI_File_open();
    MPI_File_preallocate();
    MPI_File_read();
    MPI_File_read_all();
    MPI_File_read_all_begin();
    MPI_File_read_all_end();
    MPI_File_read_at();
    MPI_File_read_at_all();
    MPI_File_read_at_all_begin();
    MPI_File_read_at_all_end();
    MPI_File_read_ordered();
    MPI_File_read_ordered_begin();
    MPI_File_read_ordered_end();
    MPI_File_read_shared();
    MPI_File_seek();
    MPI_File_seek_shared();
    MPI_File_set_atomicity();
    MPI_File_set_errhandler();
    MPI_File_set_info();
    MPI_File_set_size();
    MPI_File_set_view();
    MPI_File_sync();
    MPI_File_write();
    MPI_File_write_all();
    MPI_File_write_all_begin();
    MPI_File_write_all_end();
    MPI_File_write_at();
    MPI_File_write_at_all();
    MPI_File_write_at_all_begin();
    MPI_File_write_at_all_end();
    MPI_File_write_ordered();
    MPI_File_write_ordered_begin();
    MPI_File_write_ordered_end();
    MPI_File_write_shared();
#endif
    MPI_Finalize();
    MPI_Finalized(0);
    MPI_Gather(0,0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Gatherv(0,0,0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Get_count(0,0,0);
    MPI_Get_elements(0,0,0);
    MPI_Get_processor_name(0,0);
    MPI_Get_version(0,0);
    MPI_Graph_create(MPI_COMM_WORLD,0,0,0,0,0);
    MPI_Graph_get(MPI_COMM_WORLD,0,0,0,0);
    MPI_Graph_map(MPI_COMM_WORLD,0,0,0,0);
    MPI_Graph_neighbors(MPI_COMM_WORLD,0,0,0);
    MPI_Graph_neighbors_count(MPI_COMM_WORLD,0,0);
    MPI_Graphdims_get(MPI_COMM_WORLD,0,0);
    MPI_Group_compare(0,0,0);
    MPI_Group_difference(0,0,0);
    MPI_Group_excl(0,0,0,0);
    MPI_Group_free(0);
    MPI_Group_incl(0,0,0,0);
    MPI_Group_intersection(0,0,0);
    MPI_Group_range_excl(0,0,0,0);
    MPI_Group_range_incl(0,0,0,0);
    MPI_Group_rank(0,0);
    MPI_Group_size(0,0);
    MPI_Group_translate_ranks(0,0,0,0,0);
    MPI_Group_union(0,0,0);
    MPI_Ibsend(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Info_create(0);
    MPI_Info_delete(0,0);
    MPI_Info_dup(0,0);
    MPI_Info_free(0);
    MPI_Info_get(0,0,0,0,0);
    MPI_Info_get_nkeys(0,0);
    MPI_Info_get_nthkey(0,0,0);
    MPI_Info_get_valuelen(0,0,0,0);
    MPI_Info_set(0,0,0);
    MPI_Info_set(0,0,0);
    MPI_Init(0,0);
    MPI_Init_thread(0,0,0,0);
    MPI_Initialized(0);
    MPI_Intercomm_create(MPI_COMM_WORLD,0,MPI_COMM_WORLD,0,0,0);
    MPI_Intercomm_merge(MPI_COMM_WORLD,0,0);
    MPI_Iprobe(0,0,MPI_COMM_WORLD,0,0);
    MPI_Irecv(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Irsend(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Isend(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Issend(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Keyval_create(0,0,0,0);
    MPI_Keyval_free(0);
    MPI_Op_create(0,0,0);
    MPI_Op_free(0);
    MPI_Pack(0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Pack_size(0,0,MPI_COMM_WORLD,0);
    MPI_Pcontrol(0);
    MPI_Probe(0,0,MPI_COMM_WORLD,0);
    MPI_Recv(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Recv_init(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Reduce(0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Reduce_scatter(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Request_free(0);
    MPI_Rsend(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Rsend_init(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Scan(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Scatter(0,0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Scatterv(0,0,0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Send(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Send_init(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Sendrecv(0,0,0,0,0,0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Sendrecv_replace(0,0,0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Ssend(0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Ssend_init(0,0,0,0,0,MPI_COMM_WORLD,0);
    MPI_Start(0);
    MPI_Startall(0,0);
    MPI_Test(0,0,0);
    MPI_Test_cancelled(0,0);
    MPI_Testall(0,0,0,0);
    MPI_Testany(0,0,0,0,0);
    MPI_Testsome(0,0,0,0,0);
    MPI_Topo_test(MPI_COMM_WORLD,0);
    MPI_Type_commit(0);
    MPI_Type_contiguous(0,0,0);
    MPI_Type_create_darray(0,0,0,0,0,0,0,0,0,0);
    MPI_Type_create_subarray(0,0,0,0,0,0,0);
    MPI_Type_extent(0,0);
    MPI_Type_free(0);
    MPI_Type_get_contents(0,0,0,0,0,0,0);
    MPI_Type_get_envelope(0,0,0,0,0);
    MPI_Type_hvector(0,0,0,0,0);
    MPI_Type_lb(0,0);
    MPI_Type_size(0,0);
    MPI_Type_struct(0,0,0,0,0);
    MPI_Type_ub(0,0);
    MPI_Type_vector(0,0,0,0,0);
    MPI_Unpack(0,0,0,0,0,0,MPI_COMM_WORLD);
    MPI_Wait(0,0);
    MPI_Waitall(0,0,0);
    MPI_Waitany(0,0,0,0);
    MPI_Waitsome(0,0,0,0,0);
    MPI_Type_indexed(0,0,0,MPI_INT,0);
    MPI_Wtick();
    MPI_Wtime();
  }
}

/**************************************************************************/
/* LOCAL  **************     replace_substring     ************************/
/**************************************************************************/
/* Swap out one substring for another.  Return PyObject* string if subst  */
/**************************************************************************/
static PyObject* replace_substring(char* base,char* from, char* to) {
  char* p = 0;
  char buffer[4096];

  Assert(base);
  Assert(from);
  Assert(to);
  Assert(strlen(base)+strlen(to) < sizeof(buffer));

  p = strstr(base,from);
  if ( !p ) return 0;

  strncpy(buffer,base,p-base);
  buffer[p-base] = 0;
  strcat(buffer,to);
  strcat(buffer,p+strlen(from));

  return PyString_FromString(buffer);
}

/**************************************************************************/
/* GLOBAL **************    pyMPI_distutils_init   ************************/
/**************************************************************************/
/* Add in various support to make distutils work.  We need to:            */
/* 1) Force load the MPI calls that pyMPI doesn't have so that we can     */
/*    load dynamic libraries that DO use those calls.                     */
/**************************************************************************/
void pyMPI_distutils_init(PyObject** docStringP) {
  PyObject* sysconfig = 0;
  PyObject* sysconfig_dictionary = 0;
  PyObject* sysconfig_mode = 0;
  PyObject* get_makefile_filename = 0;
  PyObject* get_platform = 0;
  PyObject* status = 0;
  PyObject* util = 0;
  PyObject* install = 0;
  PyObject* INSTALL_SCHEMES = 0;
  PyObject* util_dictionary = 0;
  PyObject* core = 0;
  PyObject *key = 0;
  PyObject *value = 0;
  PyObject *sub_key = 0;
  PyObject *sub_value = 0;
  Py_ssize_t pos = 0;
  Py_ssize_t sub_pos = 0;
  char* scheme = 0;
  PyObject* new_scheme = 0;

  COVERAGE();

  /* ----------------------------------------------- */
  /* This call does nothing, but will force loader   */
  /* to add MPI calls.                               */
  /* ----------------------------------------------- */
  force_MPI_functions();

  /* ----------------------------------------------- */
  /* Modify the Makefile returned by distutils       */
  /* ----------------------------------------------- */
  PYCHECK( sysconfig/*owned*/= PyImport_ImportModule("distutils.sysconfig") );
  PYCHECK( sysconfig_dictionary/*borrowed*/ = PyModule_GetDict(sysconfig) );

  /* Make a new reference to get_makefile_filename */
  PYCHECK( get_makefile_filename/*borrowed*/ = PyDict_GetItemString(sysconfig_dictionary,"get_makefile_filename") );
  PYCHECK( PyDict_SetItemString(sysconfig_dictionary,"__serial_get_makefile_filename",get_makefile_filename/*incs*/) );
  get_makefile_filename = 0;

  /* Build parallel get_makefile_filename */
  PYCHECK( status/*owned*/ = PyRun_String(PYMPI_GET_MAKEFILE_FILENAME_DEF, Py_file_input, sysconfig_dictionary, sysconfig_dictionary) );
  Py_DECREF(status);
  status = 0;

  /* Set default mode to parallel */
  PYCHECK( sysconfig_mode/*owned*/ = PyString_FromString("parallel") );
  PYCHECK( PyDict_SetItemString(sysconfig_dictionary,"__pympi_mode",sysconfig_mode/*incs*/) );

  /* Something to get/set the mode */
  PYCHECK( status/*owned*/ = PyRun_String("def mode(new_mode=None):\n    global __pympi_mode\n    if new_mode is not None:\n        if '__'+str(new_mode)+'_get_makefile_filename' not in globals().keys(): raise ValueError,new_mode\n        __pympi_mode = new_mode\n    return __pympi_mode\n",
                                          Py_file_input, sysconfig_dictionary, sysconfig_dictionary) );
  Py_DECREF(status);
  status = 0;

  /* Replacement get_makefile_filename that works in serial or parallel mode */
  PYCHECK( status/*owned*/ = PyRun_String("def get_makefile_filename():\n    global __pympi_mode\n    return globals()['__'+str(__pympi_mode)+'_get_makefile_filename']()\n",
                                          Py_file_input, sysconfig_dictionary, sysconfig_dictionary) );
  Py_DECREF(status);
  status = 0;

  /* ----------------------------------------------- */
  /* Modify the platform returned in distutils       */
  /* ----------------------------------------------- */
  PYCHECK( util/*owned*/ = PyImport_ImportModule("distutils.util") ) ;
  PYCHECK( util_dictionary/*borrowed*/ = PyModule_GetDict(util) );

  /* Make a new reference to get_platform */
  PYCHECK( get_platform/*borrowed*/ = PyDict_GetItemString(util_dictionary,"get_platform") );
  PYCHECK( PyDict_SetItemString(util_dictionary,"__serial_get_platform",get_platform/*incs*/) );
  get_platform = 0;

  /* Replacement get_platform that works in serial or parallel mode */
  PYCHECK( status/*owned*/ = PyRun_String("def get_platform():\n    s = __serial_get_platform()\n    import distutils.sysconfig\n    if distutils.sysconfig.mode() == 'parallel': s +='-mpi'\n    return s\n",
                                          Py_file_input, util_dictionary, util_dictionary) );
  Py_DECREF(status);
  status = 0;
  

  /* ----------------------------------------------- */
  /* Hack the new directory into distutils...        */
  /* The operative part is in the package            */
  /* distutils.command.install.INSTALL_SCHEMES       */
  /* We have to convert python$py_version_short to   */
  /* pyMPIx.x and python to pyMPI                    */
  /* ----------------------------------------------- */

  install/*owned*/ = PyImport_ImportModule("distutils.command.install");
  INSTALL_SCHEMES/*owned*/ = PyObject_GetAttrString(install,"INSTALL_SCHEMES");

  Assert(PyDict_Check(INSTALL_SCHEMES));
  for (pos=0;PyDict_Next(INSTALL_SCHEMES, &pos, &key, &value);) {
    Assert(PyDict_Check(value));

    for (sub_pos=0;PyDict_Next(value, &sub_pos, &sub_key, &sub_value);) {
      Assert(PyString_Check(sub_value));

      scheme/*borrow*/ = PyString_AS_STRING(sub_value);

      PYCHECK( new_scheme/*owned*/ = replace_substring(scheme,"python$py_version_short",PYMPI_VERSION_NAME) );
      if ( !new_scheme ) {
        PYCHECK( new_scheme/*owned*/ = replace_substring(scheme,"python",PACKAGE) );
      }
        
      if ( new_scheme ) {
        PYCHECK( PyDict_SetItem(value,sub_key,/*incs*/new_scheme) );
        Py_DECREF(new_scheme);
        new_scheme = 0;
      }
    }
  }

  /* fall through */
 pythonError:
  Py_XDECREF(sysconfig);
  Py_XDECREF(get_makefile_filename);
  Py_XDECREF(get_platform);
  Py_XDECREF(status);
  Py_XDECREF(sysconfig_mode);
  Py_XDECREF(util);
  Py_XDECREF(core);
  Py_XDECREF(install);
  Py_XDECREF(INSTALL_SCHEMES);
  Py_XDECREF(new_scheme);
  return;
}

/**************************************************************************/
/* GLOBAL **************    pyMPI_distutils_fini   ************************/
/**************************************************************************/
/* Nothing to release                                                     */
/**************************************************************************/
void pyMPI_distutils_fini(void) {
}

END_CPLUSPLUS
