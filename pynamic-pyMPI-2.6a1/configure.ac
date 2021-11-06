dnl ==-*-Autoconf-*-===========================================================
dnl File:    configure.in
dnl Authors: Patrick Miller, Martin Casado
dnl
dnl Copyright (C) 2000 University of California Regents               
dnl ===========================================================================

dnl Process this file with autoconf to produce a configure script.
AC_INIT(initmpi.h)
AC_CANONICAL_HOST

dnl -----------------------------------------------
dnl Versioning
dnl -----------------------------------------------
AC_SUBST(PYMPI_MAJOR)
PYMPI_MAJOR=2
AC_DEFINE_UNQUOTED(PYMPI_MAJOR,"$PYMPI_MAJOR",[Major release])

AC_SUBST(PYMPI_MINOR)
PYMPI_MINOR=6
AC_DEFINE_UNQUOTED(PYMPI_MINOR,"$PYMPI_MINOR",[Minor release])

AC_SUBST(PYMPI_RELEASE)
PYMPI_RELEASE=a1
AC_DEFINE_UNQUOTED(PYMPI_RELEASE,"$PYMPI_RELEASE",[Sub versioning a=alpha, b=beta, .x=release])

AC_SUBST(PYMPI_VERSION_SHORT)
PYMPI_VERSION_SHORT="$PYMPI_MAJOR.$PYMPI_MINOR"
AC_DEFINE_UNQUOTED(PYMPI_VERSION_SHORT,"$PYMPI_VERSION_SHORT",[Short version])

dnl -----------------------------------------------
dnl Utility macros
dnl -----------------------------------------------
# AC_CHEC_AVAILABLE(VAR, [BASE_NAME])
# -----------------------------------
AC_DEFUN([AC_CHECK_AVAILABLE],[
  AC_CHECK_PROG([$1],[$2],[$2])
])

# AC_PATH_LOOKUP(VAR, [BASE_NAME-OR-PATH])
# ----------------------------------------
AC_DEFUN([AC_PATH_FULL],[
  AC_SUBST([$1])
  ac_path_full_value=["$2"]

  if test -x ["$ac_path_full_value"]; then
    [$1]=[$2]
    AC_MSG_CHECKING([executable $ac_path_full_value])
    AC_MSG_RESULT(yes)
  else
    AC_PATH_PROG([$1],[$ac_path_full_value])
  fi

  dnl Make sure we got something
  if test -z "$$1"; then
    AC_MSG_WARN([$ac_path_full_value does not exist])
  fi
])

# AC_PYTHON_MAKEVAR(VAR,msg=VAR)
# ----------------------
AC_DEFUN([AC_PYTHON_MAKEVAR],[
 AC_SUBST(py$1)
 ac_python_makevar_value="$2"
 if test -z "$ac_python_makevar_value"; then
   ac_python_makevar_value="Python $1"
 fi
 AC_MSG_CHECKING($ac_python_makevar_value)
 [ py$1=`$PYTHON -c "from distutils.sysconfig import parse_makefile, get_makefile_filename; print parse_makefile(get_makefile_filename())['$1']" | $SED 's|-Wl,-bE:Modules/python.exp||g'` ]
 AC_MSG_RESULT($py$1)
])


dnl -----------------------------------------------
dnl Automake connection
dnl -----------------------------------------------
AM_INIT_AUTOMAKE(pyMPI,"$PYMPI_MAJOR"."$PYMPI_MINOR""$PYMPI_RELEASE")
AM_CONFIG_HEADER(pyMPI_Config.h)

AC_SUBST(PYMPI_VERSION_NAME)
PYMPI_VERSION_NAME="$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR"
AC_DEFINE_UNQUOTED(PYMPI_VERSION_NAME,"$PYMPI_VERSION_NAME",[PACKAGE Short version])


AC_PROG_RANLIB

dnl -----------------------------------------------
dnl HOST specific stuff
dnl -----------------------------------------------
AC_MSG_CHECKING(host overrides)
case "$host" in
  *cygwin*)
    AC_DEFINE(PYMPI_MICROSOFT_WINDOWS,,[Are we running in MS Windows environment?])
    LIBEXT=".dll.a"
    MACOSX=0
    AC_MSG_RESULT(Cygnus on windows uses dll)
    ;;
  *-apple-*)
    LIBEXT=".a"
    MACOSX=1
    AC_DEFINE(PYMPI_MACOSX,,[Are we running in an OSX environment?])
    if test -e /Library/Frameworks/Python.framework -o -e /System/Library/Frameworks/Python.framework ; then
      LDFLAGS="$LDFLAGS -framework Python"
      FRAMEWORK=1
      AC_MSG_RESULT(using -framework Python for MacOSX)
    else
      AC_MSG_RESULT(no)
    fi
    ;;
  *)
    LIBEXT=".a"
    MACOSX=0
    AC_MSG_RESULT(no)
    ;;
esac

dnl ---------------------------------------------------
dnl Hack for bad send cancel
dnl ---------------------------------------------------
AC_MSG_CHECKING(fatal error on cancel of isend (--with-bad-cancel))
AC_ARG_WITH(bad-cancel,[--with-bad-cancel=yes    Assume that MPI_Cancel aborts on isend requests],[
    BADCANCEL="$withval"
   ],[
    BADCANCEL="no"
   ]
)
AC_MSG_RESULT($BADCANCEL)
if test "$BADCANCEL" = "yes"; then
  PYMPI_BADCANCEL="1"
else
  PYMPI_BADCANCEL="0"
fi
AC_SUBST(PYMPI_BADCANCEL)
AC_DEFINE_UNQUOTED(PYMPI_BADCANCEL,$PYMPI_BADCANCEL,[Do not allow MPI_Cancel() on isend requests])


dnl ---------------------------------------------------
dnl Hack for missing prompt
dnl ---------------------------------------------------
AC_MSG_CHECKING(Assume stdin is interactive (--with-isatty))
AC_ARG_WITH(isatty,[--with-isatty=yes    Assume stdin/out/err are interactive],[
    ISATTY="$withval"
  ],[
    # LLNL specific overrides
    case "$SYS_TYPE" in
    chaos_2_ia32*)
      ISATTY="yes"
      ;;
    *)
      ;;
    esac
  ]
)
AC_MSG_RESULT($ISATTY)
if test "$ISATTY" = "yes"; then
  PYMPI_ISATTY="1"
else
  PYMPI_ISATTY="0"
fi
AC_SUBST(PYMPI_ISATTY)
AC_DEFINE_UNQUOTED(PYMPI_ISATTY,$PYMPI_ISATTY,[Use simplified isatty()])

dnl ---------------------------------------------------
dnl Hack for missing prompt
dnl ---------------------------------------------------
AC_MSG_CHECKING(Append a newline to prompt (--with-prompt-nl))
AC_ARG_WITH(prompt-nl,[--with-prompt-nl=yes    Append a newline to prompt because of flushing issues],[
    PROMPT_NL="$withval"
  ],[
    # LLNL specific overrides
    case "$SYS_TYPE" in
    chaos_2_ia32*)
      PROMPT_NL="yes"
      ;;
    *)
      ;;
    esac
  ]
)
AC_MSG_RESULT($PROMPT_NL)
if test "$PROMPT_NL" = "yes"; then
  PYMPI_PROMPT_NL="1"
else
  PYMPI_PROMPT_NL="0"
fi
AC_SUBST(PYMPI_PROMPT_NL)
AC_DEFINE_UNQUOTED(PYMPI_PROMPT_NL,$PYMPI_PROMPT_NL,[Issue a newline after prompt])


dnl ---------------------------------------------------
dnl If we set CC in environment, use it, otherwise
dnl look for mpicc, otherwise use the normal AC_PROG_CC
dnl ---------------------------------------------------
if test -z "$CC"; then 
  AC_CHECK_AVAILABLE(CC,mpcc)
fi
if test -z "$CC"; then 
  AC_CHECK_AVAILABLE(CC,mpxlc)
fi
if test -z "$CC"; then
  AC_CHECK_AVAILABLE(CC,mpiicc)
fi
if test -z "$CC"; then
  AC_CHECK_AVAILABLE(CC,mpicc)
fi

dnl -----------------------------------------------
dnl We pretend to cross compile to prevent autoconf
dnl from actually trying C compiler (which may need
dnl POE or mpirun or something)
dnl -----------------------------------------------
save_cross_compiling="$cross_compiling"
cross_compiling="yes"
AC_PROG_CC
cross_compiling="$save_cross_compiling"

dnl -----------------------------------------------
dnl Need const in a couple of places (e.g. talk to
dnl qsort())
dnl -----------------------------------------------
AC_C_CONST

dnl -----------------------------------------------
dnl We can set CC to a C++ compiler and pyMPI
dnl will still build (but we need to let AC know)
dnl -----------------------------------------------
AC_MSG_CHECKING(for $CC is really C++)
AC_EGREP_CPP(yes,
[#ifdef __cplusplus
  yes
#endif
],[
  cc_is_cxx="yes"
],[
  cc_is_cxx=""
]
)
if test -z "$cc_is_cxx"; then
  AC_MSG_RESULT(no)
  AC_LANG(C)
else
  AC_MSG_RESULT(yes)
  AC_LANG(C++)
  CXX="$CC"
fi

dnl -----------------------------------------------
dnl More tools
dnl -----------------------------------------------
AC_PATH_PROGS(SED,sed)
AC_PATH_PROGS(GREP,grep egrep)

dnl -----------------------------------------------
dnl Look for C++ by substituting CC or ++ for cc
dnl -----------------------------------------------
if test -z "$CXX"; then
  AC_CHECK_AVAILABLE(CXX,`echo $CC | $SED -e s/cc\$/CC/g`)
fi
if test -z "$CXX"; then
  AC_CHECK_AVAILABLE(CXX,`echo $CC | $SED -e s/cc\$/++/g`)
fi
if test -z "$CXX"; then
  AC_CHECK_AVAILABLE(CXX,`echo $CC | $SED -e s/c\$/C/g`)
fi
cross_compiling="yes"
AC_PROG_CXX
cross_compiling="$save_cross_compiling"

dnl -----------------------------------------------
dnl Convert to full paths since we don't know the
dnl path that will be invoked when pyMPI uses
dnl this C or C++ compiler
dnl -----------------------------------------------
AC_PATH_FULL(CC,$CC)
AC_PATH_FULL(CXX,$CXX)

dnl -----------------------------------------------
dnl Perform all tests with C compiler choice (which
dnl may actually be C++)
dnl -----------------------------------------------
AC_LANG(C)


dnl -----------------------------------------------
dnl mpicc has a bug for the IBM SP2, we use this
dnl to get around the problem
dnl -----------------------------------------------
AC_MSG_CHECKING(if $CC -E -w is a valid CPP)
  cat > conftest.$ac_ext <<EOF
#include "confdefs.h"
#include <assert.h>
Syntax Error
EOF
$CC -E -w conftest.$ac_ext > /dev/null 2> conftest.out
ac_err=`grep -v '^ *+' conftest.out | grep -v "^conftest.${ac_ext}\$"`
if test -z "$ac_err"; then
  AC_MSG_RESULT(yes)
  CPP="$CC -E -w"
else
  AC_MSG_RESULT(no)
fi
rm -rf conftest*
AC_PROG_CPP


dnl -----------------------------------------------
dnl ln -s 
dnl -----------------------------------------------
AC_SUBST(LN_S)
if test -z "$LN_S"; then
	LN_S='ln -s'
else
	LN_S"$LN_S"
fi

dnl -----------------------------------------------
dnl Look for python (override with --with-python or
dnl PYTHON in environment).
dnl
dnl We need some care NOT to grab /usr/bin/python
dnl if /usr/bin/python is python1.5 (E.g. the
dnl REDHAT problem)
dnl -----------------------------------------------
if test -z "$PYTHON"; then
  AC_MSG_CHECKING(for --with-python)
  AC_ARG_WITH(python,[--with-python=python2.7.2    Use non-standard python],[
    PYTHON="$withval"
    AC_MSG_RESULT(yes)
  ],[
    AC_MSG_RESULT(no)
    for dir in `echo $PATH | tr : ' '`; do
	dnl Look for the largest versioned, non-Python1.5

       for x in `echo $dir/python* | tr ' ' '\012' | sort -n -r` ; do
	   # Don't check the python-config's
	   case "$x" in
	     *-config*) continue;;
	     *) ;;
           esac
	   if test -x "$x"; then
	     x_ok=`[$x -c 'print "OK"']`
	     if test "$x_ok" = "OK"; then
  		x_version=`[$x -c 'import sys; print sys.version[:3]']`
	
	        if test "$x_version" = "1.5"; then
                  AC_MSG_WARN($x is version 1.5, skipping)
                else
	          PYTHON="$x"
                  break
                fi
             fi
           fi
       done
       if test -n "$PYTHON"; then
          break
       fi
    done
  ])
fi

if test -z "$PYTHON"; then
  AC_MSG_ERROR("Could not find the path to a valid Python.  Use --with-python")
fi

AC_PATH_FULL(PYTHON,$PYTHON)
AC_MSG_CHECKING(for Python)
AC_MSG_RESULT($PYTHON)

dnl -----------------------------------------------
dnl Make check needs mpirun, failure is OK
dnl -----------------------------------------------
AC_PATH_PROG(MPIRUNEXE,MPIRun.exe)
AC_PATH_PROG(MPIRUN,mpirun)
AC_PATH_PROG(MVRUN,mvrun)
AC_PATH_PROG(POE,poe)
AC_PATH_PROG(NOPOE,nopoe)

dnl -----------------------------------------------
dnl Good python
dnl -----------------------------------------------
AC_MSG_CHECKING(Python version 2.2 or higher)
if $PYTHON -c 'import sys; sys.exit( sys.hexversion <= ( 2 << 24 | 2 << 16 ) )'; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_ERROR("Must use Python 2.2 or higher")
fi

dnl -----------------------------------------------
dnl Disutils?
dnl -----------------------------------------------
AC_MSG_CHECKING(distutils?)
PYDISTUTILS=`$PYTHON -c "import distutils; print 'yes'" 2>/dev/null`
AC_MSG_RESULT($PYDISTUTILS)
if test -z "$PYDISTUTILS"; then
  AC_MSG_ERROR("pyMPI requires distutils")
fi

dnl -----------------------------------------------
dnl Disutils works
dnl -----------------------------------------------
AC_MSG_CHECKING(distutils works)
if [$PYTHON -c "from distutils.sysconfig import parse_makefile, get_makefile_filename; print parse_makefile(get_makefile_filename())['VERSION']" > /dev/null 2>&1]; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  [$PYTHON -c "from distutils.sysconfig import parse_makefile, get_makefile_filename; print parse_makefile(get_makefile_filename())['VERSION']"]
  AC_MSG_ERROR(Your python distribution is incomplete.  Perhaps there is no XXX/lib/python2.x/config)
fi

dnl -----------------------------------------------
dnl Numeric
dnl -----------------------------------------------
AC_MSG_CHECKING(Numeric?)
PYNUMERIC=`[$PYTHON -c "import Numeric; print 'yes'" 2>/dev/null]`
AC_MSG_RESULT($PYNUMERIC)
if test -n "$PYNUMERIC"; then
  AC_DEFINE(PYMPI_HAS_NUMERIC,,[Does this Python include Numeric?])
fi

dnl -----------------------------------------------
dnl Numpy
dnl -----------------------------------------------
AC_MSG_CHECKING(Numpy?)
PYNUMPY=`[$PYTHON -c "import numpy; print 'yes'" 2>/dev/null]`
AC_MSG_RESULT($PYNUMPY)
if test -n "$PYNUMPY"; then
  AC_DEFINE(PYMPI_HAS_NUMPY,,[Does this Python include Numpy?])
fi

dnl -----------------------------------------------
dnl Version string...
dnl -----------------------------------------------
AC_PYTHON_MAKEVAR(VERSION,Python version string)
if test -z "$pyVERSION"; then
  AC_MSG_ERROR(Python must be 1.5.2 or higher)
fi

dnl -----------------------------------------------
dnl Intuit where the prefix is
dnl -----------------------------------------------
if test "$prefix" = "NONE"; then
  AC_MSG_CHECKING(install prefix for $PYTHON)
  PYPREFIX=`$PYTHON -c "from distutils.sysconfig import PREFIX; print PREFIX" 2>/dev/null`

  if test "$PYPREFIX" = ""; then
    PYPREFIX="$ac_default_prefix"
  fi
  AC_MSG_RESULT($PYPREFIX)
  prefix="$PYPREFIX"
fi

dnl -----------------------------------------------
dnl Make sure prefix exists...
dnl -----------------------------------------------
AC_MSG_CHECKING(Prefix exists...)
if test -d "$prefix"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_WARN(Prefix $prefix does not exist, will be created in Make process) 
fi

dnl -----------------------------------------------
dnl The prefix is needed inside pyMPI
dnl -----------------------------------------------
AC_DEFINE_UNQUOTED(PYMPI_PREFIX,"$prefix",[Installation prefix])
AC_DEFINE_UNQUOTED(PYMPI_LIBDIR,"$prefix/lib/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR",[Library location])
AC_DEFINE_UNQUOTED(PYMPI_INCLUDEDIR,"$prefix/include/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR",[Installation prefix])
AC_DEFINE_UNQUOTED(PYMPI_SITEDIR,"$prefix/lib/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR/site-packages",[Installation directory])

dnl -----------------------------------------------
dnl Checks for python header files.
dnl -----------------------------------------------
AC_MSG_CHECKING(for python includes locations)
AC_SUBST(PYINCLUDEDIR)
PYINCLUDEDIR=`$PYTHON -c "from distutils.sysconfig import get_python_inc; print get_python_inc()" 2>/dev/null`
AC_MSG_RESULT($PYINCLUDEDIR)

AC_MSG_CHECKING(that include directory exists)
if test -d "$PYINCLUDEDIR" -a -r "$PYINCLUDEDIR" -a -x "$PYINCLUDEDIR"; then
  AC_MSG_RESULT(yes)
  CPPFLAGS="$CPPFLAGS -I$PYINCLUDEDIR"
else
  AC_MSG_RESULT(no)
  AC_MSG_WARN(No installed headers for version $pyVERSION)
fi

AC_MSG_CHECKING(python_inc)
PYINCLUDEDIRPLATFORM=`$PYTHON -c "from distutils.sysconfig import get_python_inc; print get_python_inc(1)" 2>/dev/null`
AC_MSG_RESULT($PYINCLUDEDIRPLATFORM)

AC_MSG_CHECKING(that include directory exists)
if test -d "$PYINCLUDEDIRPLATFORM" -a -r "$PYINCLUDEDIRPLATFORM" -a -x "$PYINCLUDEDIRPLATFORM"; then
  AC_MSG_RESULT(yes)
  CPPFLAGS="$CPPFLAGS -I$PYINCLUDEDIRPLATFORM"
else
  AC_MSG_RESULT(no)
  AC_MSG_WARN(No installed platform specific headers for version $pyVERSION)
fi

dnl -----------------------------------------------
dnl check numpy headers, into site-package/numpy/core
dnl should take care of exec_prefix...
dnl -----------------------------------------------
if test -n "$PYNUMERIC"; then
  AC_MSG_CHECKING(for Numeric header files via exec_prefix)
  PYLIBDIRPLATFORM=`$PYTHON -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)" 2>/dev/null`
  if test -r $PYLIBDIRPLATFORM/Numeric/core/include/numeric/arrayobject.h; then
    AC_MSG_RESULT($PYLIBDIRPLATFORM)
    CPPFLAGS="$CPPFLAGS -I$PYLIBDIRPLATFORM/numeric/core/include -I$PYINCLUDEDIR"
  else
    AC_MSG_RESULT(no)
  fi

  AC_MSG_CHECKING(Numeric outside site-packages?)
  PYNUMERICIDIR=`[$PYTHON -c "import Numeric,os; print os.path.split(numeric.__file__)[0]" 2>/dev/null]`
  if test -r $PYNUMERICIDIR/core/include/Numeric/arrayobject.h; then
    AC_MSG_RESULT($PYNUMERICIDIR)
    CPPFLAGS="$CPPFLAGS -I$PYNUMERICIDIR/core/include"
  else
    AC_MSG_RESULT(no)
  fi
fi
if test -n "$PYNUMPY"; then
  AC_MSG_CHECKING(for numpy header files via exec_prefix)
  PYLIBDIRPLATFORM=`$PYTHON -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)" 2>/dev/null`
  if test -r $PYLIBDIRPLATFORM/numpy/core/include/numpy/arrayobject.h; then
    AC_MSG_RESULT($PYLIBDIRPLATFORM)
    CPPFLAGS="$CPPFLAGS -I$PYLIBDIRPLATFORM/numpy/core/include -I$PYINCLUDEDIR"
  else
    AC_MSG_RESULT(no)
  fi

  AC_MSG_CHECKING(numpy outside site-packages?)
  PYNUMPYIDIR=`[$PYTHON -c "import numpy,os; print os.path.split(numpy.__file__)[0]" 2>/dev/null]`
  if test -r $PYNUMPYIDIR/core/include/numpy/arrayobject.h; then
    AC_MSG_RESULT($PYNUMPYIDIR)
    CPPFLAGS="$CPPFLAGS -I$PYNUMPYIDIR/core/include"
  else
    AC_MSG_RESULT(no)
  fi
fi


dnl -----------------------------------------------
dnl Checks for python lib files...
dnl -----------------------------------------------
AC_MSG_CHECKING(for python library location)
AC_SUBST(PYLIBDIR)
PYLIBDIR=`$PYTHON -c "from distutils.sysconfig import get_python_lib; print get_python_lib()" 2>/dev/null`
AC_MSG_RESULT($PYLIBDIR)

AC_MSG_CHECKING(that lib directory is accessable)
if test -d "$PYLIBDIR" -a -r "$PYLIBDIR" -a -x "$PYLIBDIR"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
  AC_MSG_WARN(No installed lib for version $pyVERSION)
fi

dnl -----------------------------------------------
dnl Checks for python lib/config files...
dnl -----------------------------------------------
AC_MSG_CHECKING(for python lib/config location)
AC_SUBST(PYLIBCONFIGDIR)
[ PYLIBCONFIGDIR=`$PYTHON -c "from distutils.sysconfig import get_makefile_filename; import os; print os.path.split(get_makefile_filename())[0]" 2>/dev/null` ]
AC_MSG_RESULT($PYLIBCONFIGDIR)
AC_DEFINE_UNQUOTED(PYMPI_PYTHON_CONFIGDIR,"$PYLIBCONFIGDIR",[Directory where python library files live])

AC_MSG_CHECKING(that lib/config directory is accessable)
if test -d "$PYLIBCONFIGDIR" -a -r "$PYLIBCONFIGDIR" -a -x "$PYLIBCONFIGDIR"; then
  AC_MSG_RESULT(yes)
  LIBS="$LIBS -L$PYLIBCONFIGDIR"
else
  AC_MSG_RESULT(no)
  AC_MSG_WARN(No installed lib/config for version $pyVERSION)
fi

dnl -----------------------------------------------
dnl Checks for python directory for libpython$pyVERSION$LIBEXT
dnl -----------------------------------------------
pyLIBRARY="libpython$pyVERSION$LIBEXT"
AC_MSG_CHECKING($pyLIBRARY is there)
if test -r "$PYLIBCONFIGDIR/$pyLIBRARY"; then
  AC_MSG_RESULT(yes)
  LIBS="$LIBS -lpython$pyVERSION"
else
  AC_MSG_RESULT(not there)
  AC_MSG_WARN(If you get link errors add a --with-libs=-L/path/to/dir where $pyLIBRARY lives)
fi
AC_DEFINE_UNQUOTED(PYMPI_PYTHON_LIBRARY,"python$pyVERSION",[the "-l" name of python library])

dnl -----------------------------------------------
dnl Checks the python directory for Makefile
dnl -----------------------------------------------
AC_MSG_CHECKING(configuration Makefile is there)
if test -r "$PYLIBCONFIGDIR/Makefile"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(not there)
  AC_MSG_ERROR(Makefile wasn't where I thought it was)
fi

dnl -----------------------------------------------
dnl Checks the python directory for config.c
dnl -----------------------------------------------
AC_SUBST(PYLIBCONFIG_C)
AC_MSG_CHECKING(module configuration table is there)
if test -r "$PYLIBCONFIGDIR/config.c"; then
  AC_MSG_RESULT(yes)
  PYLIBCONFIG_C="$PYLIBCONFIGDIR/config.c"
else
  AC_MSG_RESULT(not there)
  AC_MSG_ERROR(config.c wasn't where I thought it was)
fi

dnl -----------------------------------------------
dnl Checks the python directory for Makefile
dnl -----------------------------------------------
AC_SUBST(PYLIBMAKEFILE)
AC_MSG_CHECKING(original Python there)
if test -r "$PYLIBCONFIGDIR/Makefile"; then
  AC_MSG_RESULT(yes)
  PYLIBMAKEFILE="$PYLIBCONFIGDIR/Makefile"
else
  AC_MSG_RESULT(not there)
  AC_MSG_ERROR(Makefile wasn't where I thought it was)
fi

dnl -----------------------------------------------
dnl Here we allow extra includes (to allow for finding
dnl mpi.h and Python.h)
dnl -----------------------------------------------
AC_MSG_CHECKING(for --with-includes)
AC_ARG_WITH(includes,[--with-includes='-Ifoo ...'    additional include paths], [
  AC_MSG_RESULT($withval)
  dnl Add in -I if we forgot it (e.g. --with-includes=/usr/foop)
  flags=""
  for x in $withval; do
    case "$x" in
    /*) flags="$flags -I$x";;
    *)  flags="$flags $x";;
    esac
  done
  CPPFLAGS="$flags $CPPFLAGS"
  EXPLICIT_INCLUDES="$flags"
], [
  AC_MSG_RESULT(no)
  EXPLICIT_INCLUDES=""
])

dnl -----------------------------------------------
dnl added by ej: tries intuiting the mpi include path
dnl              from $CC.
dnl -----------------------------------------------
MPI_BASE=`$PYTHON -c "import os; print os.path.dirname('$CC')"`
AC_MSG_CHECKING(for compiler based include directory)
if test -d "$MPI_BASE" -a -r "$MPI_BASE/include/mpi.h"; then
  CPPFLAGS="-I$MPI_BASE/include $CPPFLAGS"
  AC_MSG_RESULT($MPI_BASE)
  EXPLICIT_INCLUDES="$MPI_BASE $EXPLICIT_INCLUDES"
else
  AC_MSG_RESULT(no)
fi

dnl -----------------------------------------------
dnl Look for environment variables to set MPI
dnl paths
dnl -----------------------------------------------
AC_MSG_CHECKING(MPI_COMPILE_FLAGS)
if test -z "$MPI_COMPILE_FLAGS"; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT($MPI_COMPILE_FLAGS)
  CFLAGS="$MPI_COMPILE_FLAGS $CFLAGS"
fi
AC_MSG_CHECKING(MPI_LD_FLAGS)
if test -z "$MPI_LD_FLAGS"; then
  AC_MSG_RESULT(no)
else
  AC_MSG_RESULT($MPI_LD_FLAGS)
  LIBS="$MPI_LD_FLAGS $LIBS"
fi

dnl -----------------------------------------------
dnl Look for mpi.h
dnl -----------------------------------------------
AC_CHECK_HEADER(mpi.h,[
  TEMP=NULL 
],[
  AC_MSG_ERROR(Cannot build without mpi headers.  use --with-includes=-I/...)
])
AC_CHECK_HEADER(Python.h,[
  TEMP=NULL 
],[
  AC_MSG_ERROR(Cannot build without python headers.  use --with-includes=-I/...)
])

dnl -----------------------------------------------
# Fetch the LDFLAGS, LINKFORSHARED, MODLIBS, LIBS, CCSHARED
# SYSLIBS, and LDLAST from the makefile
dnl -----------------------------------------------
AC_PYTHON_MAKEVAR(CC)
AC_PYTHON_MAKEVAR(CFLAGS)
AC_PYTHON_MAKEVAR(INCLUDEPY)
AC_PYTHON_MAKEVAR(OPT)
AC_PYTHON_MAKEVAR(LDFLAGS)
AC_PYTHON_MAKEVAR(LINKFORSHARED)
AC_PYTHON_MAKEVAR(CCSHARED)
CFLAGS="$CFLAGS $pyCCSHARED"

# Hack to make MacOSX work right
pyLINKFORSHARED=`echo "$pyLINKFORSHARED" | $SED -e 's, Python.framework/Versions/.../Python,,g'`
AC_PYTHON_MAKEVAR(LDSHARED)
AC_PYTHON_MAKEVAR(BLDSHARED)

AC_PYTHON_MAKEVAR(LOCALMODLIBS)
AC_PYTHON_MAKEVAR(BASEMODLIBS)
AC_PYTHON_MAKEVAR(LIBS)
AC_PYTHON_MAKEVAR(LDLAST)

AC_MSG_CHECKING(Python library options)
LIBS="$LIBS $pyLDFLAGS $pyLINKFORSHARED $pyLOCALMODLIBS $pyBASEMODLIBS $pyLIBS $pyLDLAST"
AC_MSG_RESULT($LIBS)

dnl -----------------------------------------------
dnl Check for dbfork
dnl -----------------------------------------------
case "$host" in
  *86-pc-linux-*)
    AC_MSG_CHECKING(for --with-dbfork)
    AC_ARG_WITH(dbfork,[--with-dbfork	Build with TV debugging library],[
      use_dbfork="$withval"
    ],[
      use_dbfork="no"
    ])
    AC_MSG_RESULT($use_dbfork)
    if test "$use_dbfork" = "yes"; then
       LIBS="-L/usr/local/tv/dflt/linux-x86/libs -ldbfork $LIBS"
    fi
    ;;
  *)
    ;;
esac

dnl -----------------------------------------------
dnl Check for debug
dnl -----------------------------------------------
AC_MSG_CHECKING(for --with-debug)
AC_ARG_WITH(debug,[--with-debug	Build with debugging symbols],[
	if test "$withval" = "yes"; then
	   withval="-g"
	fi
	CFLAGS="$withval $CFLAGS"
	AC_MSG_RESULT($withval)

	dnl -----------------------------------------------
	dnl Change -O -O1 -O2 -O3 to -g
	dnl -----------------------------------------------
	pyCFLAGS=`echo "$pyCFLAGS" | $SED s/-O3/$withval/g | $SED s/-O2/$withval/g | $SED s/-O1/$withval/g | $SED s/-O/$withval/g`
	pyOPT=`echo "$pyOPT" | $SED s/-O3/$withval/g | $SED s/-O2/$withval/g | $SED s/-O1/$withval/g | $SED s/-O/$withval/g`
	CFLAGS=`echo "$CFLAGS" | $SED s/-O3/$withval/g | $SED s/-O2/$withval/g | $SED s/-O1/$withval/g | $SED s/-O/$withval/g`
],[
	AC_MSG_RESULT(no)
])



dnl -----------------------------------------------
dnl See if we have a python.exp file
dnl -----------------------------------------------
AC_SUBST(AIX_PYMPI_EXP_FLAG)
AC_SUBST(AIX_PYMPI_EXP_FLAG_LONG)
AC_SUBST(AIX_PYTHON_EXP_FLAG)
AIX_PYMPI_EXP_FLAG=""
AIX_PYMPI_EXP_FLAG_LONG=""
AIX_PYTHON_EXP_FLAG=""

AC_MSG_CHECKING([python.exp file])
if test -r $PYLIBCONFIGDIR/python.exp; then
  AC_MSG_RESULT($PYLIBCONFIGDIR/python.exp)
  AIX_PYMPI_EXP_FLAG="-Wl,-bE:pyMPI.exp"
  AIX_PYMPI_EXP_FLAG_LONG="-Wl,-bE:$prefix/lib/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR/pyMPI.exp"
  AIX_PYTHON_EXP_FLAG="-Wl,-bE:$PYLIBCONFIGDIR/python.exp"
else
  AC_MSG_RESULT(no)
fi

dnl -----------------------------------------------
dnl See if we can use sysconfig to get local processors
dnl (use the Python CC, not the user MPI one)
dnl -----------------------------------------------
AC_MSG_CHECKING([sysconf(_SC_NPROCESSORS_CONF)])
save_CC="$CC"
CC="$pyCC"
AC_TRY_COMPILE([
#include <unistd.h>
],[
int np = sysconf(_SC_NPROCESSORS_CONF); exit(0);
],[
  sysconf="yes"
  AC_DEFINE(PYMPI_HAVE_SYSCONF_NPROCESSORS,,[Can we get local NPROCESSORS?])
],[
  sysconf="no"
])
AC_MSG_RESULT($sysconf)
CC="$save_CC"

AC_HEADER_STDC

dnl -----------------------------------------------
dnl See if we can get the real count (Use Python's
dnl CC, not the MPI enabled one)
dnl (not we use .c, not $ac_ext because $CC might
dnl be C++, but $pyCC is probably not).
dnl -----------------------------------------------
LOCAL_PROCESSORS="1"
AC_SUBST(LOCAL_PROCESSORS)
AC_MSG_CHECKING(local processor count for testing)
if test -n "$sysconf"; then
cat >conftest.c << _ACEOF
#include <unistd.h>
#if STDC_HEADERS
# include <stdlib.h>
#endif
#include <stdio.h>
int main(void) {
  int np = sysconf(_SC_NPROCESSORS_CONF);
  printf("%d\n",np);
  exit(0);
}
_ACEOF
if $pyCC conftest.c -o conftest$ac_exeext >/dev/null 2>&1; then
  LOCAL_PROCESSORS=`./conftest$ac_exeext`
fi

fi
AC_MSG_RESULT($LOCAL_PROCESSORS)

dnl -----------------------------------------------
dnl Checks for required libraries
dnl -----------------------------------------------
AC_MSG_CHECKING(for --with-libs)
AC_ARG_WITH(libs,[--with-libs='lib1 ...'    link against additional libs], [
  AC_MSG_RESULT($withval)
  LIBS="$withval $LIBS"
  EXPLICIT_LIBS="$withval"
], [
  AC_MSG_RESULT(no)
  EXPLICIT_LIBS=""
])
AC_CHECK_LIB(m,pow)
AC_CHECK_FUNCS(PyOS_StdioReadline)

dnl -----------------------------------------------
dnl Need to know about line buffering
dnl -----------------------------------------------
AC_CHECK_FUNCS(setlinebuf)

dnl -----------------------------------------------
dnl Other header tests
dnl -----------------------------------------------
AC_CHECK_HEADERS(sys/param.h)


dnl -----------------------------------------------
dnl HAVE TO HACK THE LIB STRING FOR NEXT TWO
dnl COMMANDS
dnl -----------------------------------------------
sLIBS="$LIBS"
LIBS="$LIBS $AIX_PYTHON_EXP_FLAGS -lm"
addLIBS=""

dnl -----------------------------------------------
dnl See if we can intuit what extra Python link libs are 
dnl 
dnl if link fails, will try to link with libgcc and lm (since
dnl there is a chance we are running into the
dnl __eprintf undefined symbol bug) /mc
dnl -----------------------------------------------
libdir=/usr/local/lib/python2.2/config
AC_MSG_CHECKING(Python links as is)
AC_TRY_LINK(
#ifdef __cplusplus
extern "C" {
#endif
  extern int Py_Main(int,char**);
#ifdef __cplusplus
}
#endif
, Py_Main(0,0),[
  AC_MSG_RESULT(yes)
],[
  AC_MSG_RESULT(no)

  AC_MSG_CHECKING(for -pthread)
  SAVELDFLAGS="$LDFLAGS"
  LDFLAGS="-pthread $LDFLAGS"
  AC_TRY_LINK(
#ifdef __cplusplus
    extern "C" {
#endif
    extern int Py_Main(int,char**);
#ifdef __cplusplus
    }
#endif
, Py_Main(0,0),[
  AC_MSG_RESULT(yes)
  ],[
  AC_MSG_RESULT(no)
  LDFLAGS="$SAVELDFLAGS"

  dnl libm and libgcc.a 
  AC_MSG_CHECKING(for gcc libraries)
  LIBGCC=`/usr/local/bin/gcc --print-libgcc-file-name`
  AC_MSG_RESULT(Found $LIBGCC)
  LIBS="$LIBS $LIBGCC -lm"
  AC_MSG_CHECKING(_eprintf bug workaround)
  AC_TRY_LINK(
#ifdef __cplusplus
extern "C" {
#endif
  extern int Py_Main(int,char**);
#ifdef __cplusplus
}
#endif
	, Py_Main(0,0),[
    AC_MSG_RESULT(yes)
    addLIBS="$addLIBS $LIBGCC -lm"
  ],[
    AC_MSG_RESULT(no)
    AC_MSG_WARN(Python doesn't seem to link.  Look at config.log.  You may need to add --with-libs or --with-includes or just hack the Makefile)
  ])
])
])

dnl -----------------------------------------------
dnl See if we can link a basic MPI program
dnl -----------------------------------------------
AC_MSG_CHECKING(for MPI capability)
AC_TRY_LINK([
#include "mpi.h"
#include <stdlib.h>
],[
  MPI_Init(NULL,NULL);
],[
  AC_MSG_RESULT(yes)
],[
  AC_MSG_RESULT(no)
  dnl Maybe we just need to add -lmpi (e.g. COMPAQ)
  AC_MSG_CHECKING(if adding -lmpi helps)
  LIBS="$LIBS $addLIBS -lmpi"
  AC_TRY_LINK([
#include "mpi.h"
#include <stdlib.h>
],[
  MPI_Init(NULL,NULL);
],[
    AC_MSG_RESULT(yes)
    addLIBS="$addLIBS -lmpi"
  ],[
    AC_MSG_RESULT(no)
    AC_MSG_WARN(MPI must need some more libraries...  Look at config.log.  You may need to add --with-libs info)
  ])
])

dnl -----------------------------------------------
dnl RESTORE THE LIBS SAVED ABOVE!!!
dnl -----------------------------------------------
LIBS="$sLIBS $addLIBS"

dnl -----------------------------------------------
dnl If this Python support the read-only .pyc option
dnl we use it to suppress the parallel generation
dnl of pyc files (an undetectable race condition
dnl for distributed parallel machines)
dnl -----------------------------------------------
AC_CHECK_FUNCS(Py_ReadOnlyBytecodeFlag)

dnl -----------------------------------------------
dnl check to see if mpi implementation defines
dnl MPI_Initialized
dnl -----------------------------------------------
AC_MSG_CHECKING(for Py_ssize_t)
AC_TRY_COMPILE([
#include "Python.h"
],[
Py_ssize_t idummy;
],[
AC_DEFINE(HAVE_PY_SSIZE_T,,[Does Python use int or Py_ssize_t])
AC_MSG_RESULT(yes)
],[
AC_MSG_RESULT(no)
])


dnl -----------------------------------------------
dnl check to see if mpi implementation defines
dnl MPI_Initialized
dnl -----------------------------------------------
AC_MSG_CHECKING(for MPI_Initialized())
AC_TRY_LINK([
#include "mpi.h"
],[
int flag;
MPI_Initialized(&flag);
],[
AC_DEFINE(HAVE_MPI_INITIALIZED,,[Is MPI_Initialized defined by the MPI implementation?])
AC_MSG_RESULT(yes)
],[
AC_MSG_RESULT(no)
])

dnl -----------------------------------------------
dnl check to see if mpi implementation defines
dnl MPI_Finalized
dnl -----------------------------------------------
AC_MSG_CHECKING(for MPI_Finalized())
AC_TRY_LINK([
#include "mpi.h"
],[
int flag;
MPI_Finalized(&flag);
],[
AC_DEFINE(HAVE_MPI_FINALIZED,,[Is MPI_Finalized defined by the MPI implementation?])
AC_MSG_RESULT(yes)
],[
AC_MSG_RESULT(no)
])

dnl -----------------------------------------------
dnl check to see if mpi implementation defines
dnl MPI_File_close
dnl -----------------------------------------------
AC_MSG_CHECKING(for MPI File operations (ROMIO))
AC_TRY_LINK([
#include "mpi.h"
],[
int flag;
MPI_File_close(&flag);
],[
AC_DEFINE(HAVE_MPI_FILE_OPERATIONS,,[Is MPI_File_close defined by the MPI implementation?])
AC_MSG_RESULT(yes)
],[
AC_MSG_RESULT(no)
])

dnl -----------------------------------------------
dnl AIX's dynamic load needs more help than a
dnl normal dlopen.
dnl -----------------------------------------------
AC_MSG_CHECKING(for AIX dynamic load)
AC_EGREP_CPP(yes,
[#ifdef _AIX
  yes
#endif
],[
  AC_MSG_RESULT(yes)
  AM_CONDITIONAL(AIX_DYNAMIC_LOAD,true)
],[
  AC_MSG_RESULT(no)
  AM_CONDITIONAL(AIX_DYNAMIC_LOAD,false)
])


dnl -----------------------------------------------
dnl check to see if mpi implementation defines
dnl MPI_Comm_{get,set}_name
dnl -----------------------------------------------
AC_MSG_CHECKING(for MPI Comm name operations)
AC_TRY_LINK([
#include "mpi.h"
],[
MPI_Comm com;
char name[1];
int resultlen;
MPI_Comm_get_name(com,name,resultlen);
MPI_Comm_set_name(com,name);
],[
AC_DEFINE(HAVE_MPI_COMM_NAME_OPERATIONS,,[Are MPI_Comm_get_name and MPI_Comm_set_name defined by the MPI implementation?])
AC_MSG_RESULT(yes)
],[
AC_MSG_RESULT(no)
])

dnl -----------------------------------------------
dnl Check for other AIX stuff
dnl -----------------------------------------------
AC_CHECK_HEADERS(pm_util.h)
AC_CHECK_FUNCS(mpc_flush)
AC_CHECK_FUNCS(mpc_isatty)

dnl -----------------------------------------------
dnl We will use the experimental flag until the
dnl code restructuring is done
dnl -----------------------------------------------
AC_MSG_CHECKING(for Electric Fence enabled?)
AC_ARG_ENABLE(ElectricFence,
  [  --enable-ElectricFence    Turn on Electric Fence malloc debug],
  [ ],
  [enableval="no"]
)
AC_MSG_RESULT(${enableval})
case "${enableval}" in
yes)
  LIBS="-lefence $LIBS"
  AC_CHECK_LIB(efence,EF_Abort)
  ;;
esac

dnl -----------------------------------------------
dnl Need some stuff to update disutils
dnl -----------------------------------------------
AC_DEFINE_UNQUOTED(PYMPI_GET_MAKEFILE_FILENAME_DEF,"def __parallel_get_makefile_filename(): return '$prefix/lib/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR/augmentedMakefile'\n",[Replacement function for get_makefile_filename])
AC_DEFINE_UNQUOTED(PYMPI_COMPILER,"$CC",[Name of MPI enabled C compiler])
AC_DEFINE_UNQUOTED(PYMPI_COMPILER_FLAGS,"$CFLAGS -I$prefix/include/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR -I$pyINCLUDEPY",[Modified compiler flags])
AC_DEFINE_UNQUOTED(PYMPI_LINKER,"$CC",[Name of the MPI enabled C compiler for linking])
AC_DEFINE_UNQUOTED(PYMPI_LINKER_FLAGS,"$AIX_PYMPI_EXP_FLAG_LONG -L$prefix/lib/$PACKAGE$PYMPI_MAJOR.$PYMPI_MINOR -lpyMPI $LIBS",[Extra link flags needed to build])

dnl -----------------------------------------------
dnl Output the makefile and test shell
dnl -----------------------------------------------
dnl AC_CONFIG_COMMANDS(chmod_scripts,[chmod +x unittest/run-all-tests])
AC_OUTPUT(Makefile unittest/run-all-tests softload_setup.py parallel_test)

dnl Adapted from ASCII art by ThePieMaster@thepieeater.demon.co.uk
echo '                         /|_|_|_\     )'
echo '     ~^^^~~~~~~~~^^^~~^^~~~~~^   \'
echo '    <vvvvvvvvvvvvvvvvvvvvvvvvvv>  |'
echo '     \ Putting the pie in MPI /   |'
echo '      \__\)\_________________/    |'
echo '          \ )  /        \__ _ _ _/'
echo '           )  /          \_|_|_|/'
