
# Set POE to poe or nopoe on IBM or % make POE=nopoe
POE=
pyMPI=../pyMPI
CC=`$(POE) $(pyMPI) -c 'import mpi; print mpi.configuration.PYMPI_COMPILER'`
CFLAGS=`$(POE) $(pyMPI) -c 'import mpi; print mpi.configuration.PYMPI_COMPILER_FLAGS'`
LFLAGS=`$(POE) $(pyMPI) -c 'import mpi; print mpi.configuration.PYMPI_LINKER_FLAGS'`

conan: conan.o
	cmd="$(CC) -o conan conan.o $(LFLAGS)"; echo; echo $$cmd; echo; $$cmd

conan.o: conan.c
	cmd="$(CC) -c conan.c $(CFLAGS)"; echo; echo $$cmd; echo; $$cmd

clean:
	rm -f conan.o conan


