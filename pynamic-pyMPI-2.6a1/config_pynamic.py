#! /bin/env python

#
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
# To change the parameters used in producing dummy libraries--parameters
# controlling the number of shared libraries, average library and
# symbol table size, and etc--change the value of "num_files" 
# and/or "num_fuctions" in this file.
#
# Update:
#          Jan 26 2007 DHA: File created. The meat of the file 
#                           is Greg's so_generator 
#
#

from so_generator import *
from subprocess import *
import sys, os
import os.path

#
# run the so_generator
#
if len(sys.argv) < 3:
    print_usage('config_pynamic.py')
    sys.exit(1)
configure_args, python_command = parse_and_run('config_pynamic.py')

#
# configure pyMPI with the pynamic-generated libraries
#
cwd = os.getcwd()
command = './configure --with-prompt-nl --with-isatty '
command += ' --with-python=%s ' %(python_command)
command += '--with-libs=\'-Wl,-rpath=' + cwd + ' -L' + cwd + ' '
for p, d, f in os.walk('./'):
    for file in f:
        if file.find('.so') != -1 and file.find('lib') != -1:
            command += '-l' + file[3:file.find('.')] + ' '
command += '\' '
for arg in configure_args:
    command += arg + ' '
print(command)
ret = os.system(command)
if ret != 0:
    print_error('configure returned an error! Please specify/fix configure args with the -c option')

#
# run the pyMPI Makefile
#
ret = os.system("make clean")
if ret != 0:
    print_error('make clean failed!')
ret = os.system("make")
if ret != 0:
    print_error('make failed!')

#
# build the addall utility program
#
if os.path.exists('./addall.c') != True:
    print_error('required file addall.c not found!')

command = "gcc -g addall.c -o addall"
print(command)
ret = os.system(command)
if ret != 0:
    print_error('Failed to build addall utility!')

#
# check DBG, text, symbol table, and string table size.
#
if os.path.exists('./get-symtab-sizes') != True:
    print_error('required file get-symtab-sizes not found!')

os.system('rm -f sharedlib_section_info')
command = "get-symtab-sizes pynamic-pyMPI > sharedlib_section_info"
print(command)
ret = os.system(command)
if ret != 0:
    print_error('Failed to get executable statistics!')

command = "tail -10 sharedlib_section_info"
os.system(command)

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
