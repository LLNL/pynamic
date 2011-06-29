## -*-Makefile-*- Process this file with automake to produce Makefile.in
#**************************************************************************#
#* FILE   **************         Makefile          ************************#
#**************************************************************************#
#* Author: Patrick Miller July 31 2002					  *#
#**************************************************************************#
#*                                                                        *#
#**************************************************************************#

AUTOMAKE_OPTIONS=no-dependencies

PYTHON=@PYTHON@
datadir=$(libdir)

version=@PYMPI_MAJOR@.@PYMPI_MINOR@
libdir=${prefix}/lib/@PACKAGE@$(version)
includedir=${prefix}/include/@PACKAGE@$(version)
sitedir=${libdir}/site-packages

install-data-local: softload_setup.py
	$(mkinstalldirs) $(DESTDIR)$(sitedir)
	-$(PYTHON) softload_setup.py install --install-lib=$(DESTDIR)$(sitedir)

#-----------------------------------------------
# To avoid link order issues, we use a special
# linker helper program that sticks -L and -l
# flags to the end of a link
#-----------------------------------------------
pyMPI_linker_SOURCES = pyMPI_linker.c
pyMPI_linker.$(OBJEXT): pyMPI_linker.c
	@pyCC@ -c $(srcdir)/pyMPI_linker.c
pyMPI_linker$(EXEEXT): $(pyMPI_linker_OBJECTS)
	@pyCC@ -o pyMPI_linker$(EXEEXT) $(pyMPI_linker_OBJECTS)

pyMPI_SOURCES = \
    pyMPI.c

$(pyMPI_OBJECTS): pyMPI_Externals.h

pyMPI_LDADD = -L. libpyMPI.a @AIX_PYMPI_EXP_FLAG@ @LDFLAGS@

libpyMPI_a_SOURCES = \
    pyMPI_banner.c \
    pyMPI_cart.c \
    pyMPI_configuration.c \
    pyMPI_comm.c \
    pyMPI_comm_asynchronous.c \
    pyMPI_comm_collective.c \
    pyMPI_comm_io.c \
    pyMPI_comm_map.c \
    pyMPI_comm_message.c \
    pyMPI_comm_misc.c \
    pyMPI_distutils.c \
    pyMPI_file.c \
    pyMPI_init.c \
    pyMPI_initialize.c \
    pyMPI_isatty.c \
    pyMPI_main.c \
    pyMPI_pickle.c \
    pyMPI_pipes.c \
    pyMPI_rco.c \
    pyMPI_readline.c \
    pyMPI_recv.c \
    pyMPI_reductions.c \
    pyMPI_request.c \
    pyMPI_signals.c \
    pyMPI_site.c \
    pyMPI_send.c \
    pyMPI_shared_file.c \
    pyMPI_status.c \
    pyMPI_sysmods.c \
    pyMPI_user_banner.c \
    pyMPI_user_directory.c \
    pyMPI_user_fini.c \
    pyMPI_user_init.c \
    pyMPI_user_startup.c \
    pyMPI_util.c 

$(libpyMPI_a_OBJECTS): pyMPI_Externals.h

#-----------------------------------------------
# Skip updating the externals.h include file
#-----------------------------------------------
.PHONY: fast
fast:
	touch pyMPI_Externals.h
	sleep 1
	touch $(libpyMPI_a_OBJECTS)
	$(RM) -f $(KILL)
	$(MAKE)

#-----------------------------------------------
# This exposes the values from pyMPI_Config.h
# to python
#-----------------------------------------------
pyMPI_configuration.c: pyMPI_Config.h utils/expose_configuration.py
	$(RM) -f pyMPI_configuration.c
	$(PYTHON) $(srcdir)/utils/expose_configuration.py pyMPI_Config.h > pyMPI_configuration.c
	chmod -w pyMPI_configuration.c


#$(pyMPI_OBJECTS): $(header_files)

#-----------------------------------------------
# Installation control...
#-----------------------------------------------
bin_PROGRAMS  = pyMPI
noinst_PROGRAMS = pyMPI_linker
bin_SCRIPTS = @PACKAGE@$(version)$(EXEEXT) pyMPI$(version)_linker$(EXEEXT)

lib_LIBRARIES = libpyMPI.a

data_DATA =  augmentedMakefile pyMPI.exp

AM_CFLAGS=@CFLAGS@

#-----------------------------------------------
# Distribution control
#-----------------------------------------------
noinst_HEADERS = \
    mpicomm.h

EXTRA_DIST = \
	parallel_test \
	pyMPI_softload.c \
	softload_setup.py.in \
	LICENSE.TXT \
	advanced_extension/advanced.c \
		advanced_extension/setup.py \
		advanced_extension/test.py \
	benchmarks/benchmark.py \
		benchmarks/cbenchmark.c \
	cloning_pyMPI/conan.c \
		cloning_pyMPI/Makefile \
	docs/Makefile \
		docs/README \
		docs/pyMPI.pdf \
		docs/developers_guidance.xml \
		docs/developers_overview.xml \
		docs/extenders_guide.xml \
		docs/how_it_works.xml \
		docs/overview.xml \
		docs/xml2ps.py \
	examples/fractal.py \
	micro_tests/pyMPI_default_test \
	scaling/allreduce.py \
		scaling/pingpong.c \
		scaling/pingpongtest.py \
		scaling/setup.py \
	simple_extension/setup.py \
		simple_extension/simple.c \
		simple_extension/test.py \
	swig_example/example.i \
		swig_example/examplecode.c \
		swig_example/setup.py \
		swig_example/test.py \
	unittest/PyMPITest.py \
		unittest/extension.py \
		unittest/gathers.py \
		unittest/popen.py \
		unittest/reduces.py \
		unittest/run-all-tests.in \
		unittest/scatters.py \
	utils/checkdistribution.py \
		utils/crontest \
		utils/editMakefile.py \
		utils/expose_configuration.py \
		utils/findExternals.py \
		utils/grind_docs_and_prototypes.py

#-----------------------------------------------
# Developers only:
# It is easy to forget new CVS files, run
# make dist check_extra_dist to see what files
# are missing.
#-----------------------------------------------
.PHONY: check_extra_dist
check_extra_dist:
	$(PYTHON) $(srcdir)/utils/checkdistribution.py $(distdir).tar.gz $(EXTRA_DIST)

header_files = pyMPI.h pyMPI_Macros.h pyMPI_Types.h  pyMPI_Externals.h
include_HEADERS = initmpi.h $(header_files) pyMPI_Config.h

pyMPI$(version)$(EXEEXT): pyMPI$(EXEEXT)
	cp pyMPI$(EXEEXT) pyMPI$(version)$(EXEEXT)

pyMPI$(version)_linker$(EXEEXT): pyMPI_linker$(EXEEXT)
	cp pyMPI_linker$(EXEEXT) pyMPI$(version)_linker$(EXEEXT)

#-----------------------------------------------
# Be sure to use up to date pyMPI_Externals.h
# and extras (micro_test/*)
#-----------------------------------------------
dist-local: pyMPI_Externals.h

pyMPI_Externals.h: $(libpyMPI_a_SOURCES) pyMPI_Types.h utils/grind_docs_and_prototypes.py
	rm -f pyMPI_Externals.h
	$(PYTHON) $(srcdir)/utils/grind_docs_and_prototypes.py $(srcdir) $(libpyMPI_a_SOURCES) pyMPI_Types.h > pyMPI_Externals.h
	chmod -w pyMPI_Externals.h

DASHL=`echo @LIBS@ | tr ' ' '\012' | egrep '^-(l|L)'`
NOT_DASHL=`echo @LIBS@ | tr ' ' '\012' | egrep -v '^-(l|L)'`

#-----------------------------------------------
# Updated Makefile for distutils to look at
#-----------------------------------------------
augmentedMakefile: @PYLIBMAKEFILE@ Makefile
	rm -f augmentedMakefile
	$(PYTHON) $(srcdir)/utils/editMakefile.py < @PYLIBMAKEFILE@ CC @CC@ | \
		$(PYTHON) $(srcdir)/utils/editMakefile.py CXX @CXX@ | \
		$(PYTHON) $(srcdir)/utils/editMakefile.py OPT -DHAVE_MPI @CPPFLAGS@ @CFLAGS@ @pyOPT@ | \
		$(PYTHON) $(srcdir)/utils/editMakefile.py LIBS $(DASHL) @AIX_PYMPI_EXP_FLAG_LONG@ | \
		$(PYTHON) $(srcdir)/utils/editMakefile.py LINKFORSHARED  @pyLINKFORSHARED@ | \
		$(PYTHON) $(srcdir)/utils/editMakefile.py LDSHARED       ${bindir}/pyMPI$(version)_linker$(EXEEXT) @pyLDSHARED@ | \
		$(PYTHON) $(srcdir)/utils/editMakefile.py BLDSHARED      ${bindir}/pyMPI$(version)_linker$(EXEEXT) @pyBLDSHARED@ \
	>> augmentedMakefile

#--------------------------------------------------
# Just a target to echo the name of the .tgz distro
#--------------------------------------------------
.PHONY: distro
distro:
	@echo $(distdir).tar.gz

clean-local:
	-rm -rf build micro_tests/pyMPI_*[0-9]

CLEANFILES = augmentedMakefile pyMPI.exp \
	pyMPI_configuration.c \
	extensiontest.* \
	pyMPI$(version)$(EXEEXT) \
	pyMPI$(version)_linker$(EXEEXT) \
	pyMPI_linker$(EXEEXT) \
	debug \
	pyMPI_Externals.h \
	pyMPI.exp 

#-----------------------------------------------
# Make sure tests are run with the newly
# compiled pyMPI
#-----------------------------------------------
TESTS_ENVIRONMENT= PATH=.:$$PATH
TESTS      =           \
	micro_tests/pyMPI_*
