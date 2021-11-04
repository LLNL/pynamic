/* pyMPI_Config.h.  Generated from pyMPI_Config.h.in by configure.  */
/* pyMPI_Config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `efence' library (-lefence). */
/* #undef HAVE_LIBEFENCE */

/* Define to 1 if you have the `m' library (-lm). */
/* #undef HAVE_LIBM */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mpc_flush' function. */
/* #undef HAVE_MPC_FLUSH */

/* Define to 1 if you have the `mpc_isatty' function. */
/* #undef HAVE_MPC_ISATTY */

/* Are MPI_Comm_get_name and MPI_Comm_set_name defined by the MPI
   implementation? */
/* #undef HAVE_MPI_COMM_NAME_OPERATIONS */

/* Is MPI_File_close defined by the MPI implementation? */
/* #undef HAVE_MPI_FILE_OPERATIONS */

/* Is MPI_Finalized defined by the MPI implementation? */
/* #undef HAVE_MPI_FINALIZED */

/* Is MPI_Initialized defined by the MPI implementation? */
/* #undef HAVE_MPI_INITIALIZED */

/* Define to 1 if you have the <pm_util.h> header file. */
/* #undef HAVE_PM_UTIL_H */

/* Define to 1 if you have the `PyOS_StdioReadline' function. */
/* #undef HAVE_PYOS_STDIOREADLINE */

/* Define to 1 if you have the `Py_ReadOnlyBytecodeFlag' function. */
/* #undef HAVE_PY_READONLYBYTECODEFLAG */

/* Does Python use int or Py_ssize_t */
#define HAVE_PY_SSIZE_T /**/

/* Define to 1 if you have the `setlinebuf' function. */
/* #undef HAVE_SETLINEBUF */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Name of package */
#define PACKAGE "pyMPI"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT ""

/* Define to the full name of this package. */
#define PACKAGE_NAME ""

/* Define to the full name and version of this package. */
#define PACKAGE_STRING ""

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME ""

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION ""

/* Do not allow MPI_Cancel() on isend requests */
#define PYMPI_BADCANCEL 0

/* Name of MPI enabled C compiler */
#define PYMPI_COMPILER "/opt/toss/openmpi/4.1/gnu/bin/mpicc"

/* Modified compiler flags */
#define PYMPI_COMPILER_FLAGS "-g -O2 -fPIC -I/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/include/pyMPI2.6 -I/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/include/python3.8"

/* Replacement function for get_makefile_filename */
#define PYMPI_GET_MAKEFILE_FILENAME_DEF "def __parallel_get_makefile_filename(): return '/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/lib/pyMPI2.6/augmentedMakefile'\n"

/* Does this Python include Numeric? */
/* #undef PYMPI_HAS_NUMERIC */

/* Does this Python include Numpy? */
/* #undef PYMPI_HAS_NUMPY */

/* Can we get local NPROCESSORS? */
#define PYMPI_HAVE_SYSCONF_NPROCESSORS /**/

/* Installation prefix */
#define PYMPI_INCLUDEDIR "/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/include/pyMPI2.6"

/* Use simplified isatty() */
#define PYMPI_ISATTY 1

/* Library location */
#define PYMPI_LIBDIR "/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/lib/pyMPI2.6"

/* Name of the MPI enabled C compiler for linking */
#define PYMPI_LINKER "/opt/toss/openmpi/4.1/gnu/bin/mpicc"

/* Extra link flags needed to build */
#define PYMPI_LINKER_FLAGS " -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/lib/pyMPI2.6 -lpyMPI -Wl,-rpath=/usr/WS2/tossbldr/pynamic/pynamic/pynamic-pyMPI-2.6a1 -lutility0 -lutility1 -lutility2 -lmodulefinal -lmodule0 -lmodule1 -lmodule2 -lmodule3 -lmodule4 -lmodule5 -lmodulebegin   -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/lib/python3.8/config-3.8-x86_64-linux-gnu -lpython3.8 -L/usr/lib64 -L/usr/lib64 -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/gdbm-1.21-5yl4zza5mivngaibolp3tbbp4tnzlmvu/lib -L/usr/lib64 -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/libffi-3.3-srolgbhpnjygafxyl62kxaqvoigfzv6a/lib64 -L/usr/lib64 -L/usr/lib64 -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/readline-8.1-ghwkfka5sg4h3lujzoha5qp3nyg3xtaf/lib  -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/util-linux-uuid-2.36.2-lndaojng7yeuorl72ardtumgjugz6r36/lib -L/usr/lib64 -L/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/zlib-1.2.11-l44ubhzacggmhtnh5x24txt57gbadcdx/lib -Xlinker -export-dynamic   -lcrypt -lpthread -ldl  -lutil -lm  "

/* Are we running in an OSX environment? */
/* #undef PYMPI_MACOSX */

/* Major release */
#define PYMPI_MAJOR "2"

/* Are we running in MS Windows environment? */
/* #undef PYMPI_MICROSOFT_WINDOWS */

/* Minor release */
#define PYMPI_MINOR "6"

/* Installation prefix */
#define PYMPI_PREFIX "/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2"

/* Issue a newline after prompt */
#define PYMPI_PROMPT_NL 1

/* Directory where python library files live */
#define PYMPI_PYTHON_CONFIGDIR "/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/lib/python3.8/config-3.8-x86_64-linux-gnu"

/* the "-l" name of python library */
#define PYMPI_PYTHON_LIBRARY "python3.8"

/* Sub versioning a=alpha, b=beta, .x=release */
#define PYMPI_RELEASE "a1"

/* Installation directory */
#define PYMPI_SITEDIR "/usr/WS1/legendre/spack/opt/spack/linux-rhel8-zen/gcc-8.4.1/python-3.8.12-pyqiogxquuwf6stoknpvvpwu5n2rlcd2/lib/pyMPI2.6/site-packages"

/* PACKAGE Short version */
#define PYMPI_VERSION_NAME "pyMPI2.6"

/* Short version */
#define PYMPI_VERSION_SHORT "2.6"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "2.6a1"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */
