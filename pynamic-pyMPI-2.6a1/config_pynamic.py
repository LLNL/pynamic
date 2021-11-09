#! /usr/bin/env python

# Please see COPYRIGHT information at the end of this file
# File: config_pynamic.py
# Authors: Dong H. Ahn and Greg Lee
#
# An addon to pyMPI, which allows dynamic library linking system
# stress test.
#
# command: ./config_pynamic.py generates shared library
#          codes, builds shared libraries using those codes, and then
#          configures/builds pyMPI with those libraries.
#

from so_generator import print_error, parse_and_run, run_command, print_usage
import sys
import os

#
# run the so_generator
#
if len(sys.argv) < 3:
    print_usage('config_pynamic.py')
    sys.exit(1)
configure_args, python_command, bigexe, use_mpi4py, processes = parse_and_run('config_pynamic.py')

if not use_mpi4py:
    #
    # configure pyMPI or mpi4py with the pynamic-generated libraries
    #
    command = './configure --with-prompt-nl --with-isatty --with-python=%s --with-libs="' % (sys.executable)
    command += '-Wl,-rpath=%s ' %(os.getcwd())
    for p, d, f in os.walk('./'):
        for file in f:
            if file.find('.so') != -1 and file.find('lib') != -1:
                command += '-l' + file[3:file.find('.')] + ' '
    command += '" '
    for arg in configure_args:
        command += arg + ' '
    
    ret = run_command(command, False)
    if ret != 0:
        print('configure returned an error! Please specify/fix configure args with the -c option')
        sys.exit(ret)
    
    #
    # run the pyMPI Makefile
    #
    command = 'make clean'
    run_command(command)
    
    command = 'make dist-local'
    run_command(command)
    
    command = 'make -j ' + str(processes)
    run_command(command)
else:
    command = 'make -f Makefile.mpi4py clean'
    run_command(command)
    
    target = 'pynamic-mpi4py'
    if bigexe:
        target += ' pynamic-bigexe-mpi4py'
    command = 'make -j ' + str(processes) + ' -f Makefile.mpi4py ' + target
    run_command(command)
    
if bigexe == False:
    command = 'rm -f pynamic-bigexe-pyMPI pynamic-bigexe-sdb-pyMPI pynamic-bigexe-mpi4py'
    run_command(command, False)
    
#
# build the addall utility program
#
if os.path.exists('./addall.c') != True:
    print_error('required file addall.c not found!')
    sys.exit(0)

command = "gcc -g addall.c -o addall"
run_command(command)

#
# check DBG, text, symbol table, and string table size.
#
if os.path.exists('./get-symtab-sizes') != True:
    print_error('required file get-symtab-sizes not found!')
    sys.exit(0)

for exe in ['pynamic-pyMPI', 'pynamic-sdb-pyMPI', 'pynamic-bigexe-pyMPI', 'pynamic-bigexe-sdb-pyMPI', 'pynamic-mpi4py', 'pynamic-bigexe-mpi4py']:
    info_file = 'sharedlib_section_info_%s' %(exe)
    os.system('rm -f %s' %(info_file))
    if os.path.exists(exe):
        command = "./get-symtab-sizes %s > %s" %(exe, info_file)
        ret = run_command(command)
        if ret != 0:
            print_error('Failed to get executable statistics for %s!' %(exe))
        else:
            command = "tail -10 %s" %(info_file)
            run_command(command)

#
#COPYRIGHT
#
#Copyright (c) 2007, The Regents of the University of California.
#Produced at the Lawrence Livermore National Laboratory
#Written by Gregory Lee, Dong Ahn, John Gyllenhaal, Bronis de Supinski.
#UCRL-CODE-228991.
#All rights reserved.
#
#This file is part of Pynamic.   For details contact Greg Lee (lee218@llnl.gov).  Please also read the "ADDITIONAL BSD NOTICE" in pynamic.LICENSE.
#
#Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
#* Redistributions of source code must retain the above copyright notice, this list of conditions and the disclaimer below.
#* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the disclaimer (as noted below) in the documentation and/or other materials provided with the distribution.
#* Neither the name of the UC/LLNL nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OF THE UNIVERSITY OF CALIFORNIA, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
