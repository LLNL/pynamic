#! /usr/bin/env python

# Please see COPYRIGHT information at the end of this file.
# File: so_generator.py
# Author: Greg Lee
#
# Generates Python callable modules and C utility libraries
# and compiles them into shared object files.

import sys, os, string
import random
import multiprocessing as mp
from subprocess import *
from sysconfig import get_paths

var_types = ['int', 'long', 'float', 'double', 'char *']

def run_command(command, exit_on_error=True):
    print(command)
    ret = os.system(command)
    if ret != 0:
        print_error('%s failed!' %(command), exit_on_error)
    return ret

#write a function declaration to a file
def write_function_declaration(f, function_name, function):
    function_type = function[0]
    f.write (function_type + ' ' + function_name + '(')
    num_args = function[1]
    for arg_num in range(num_args):
        arg_name = 'arg' + str(arg_num)
        type = function[arg_num + 2]
        f.write(type + ' ' + arg_name)
        if (arg_num != num_args - 1):
            f.write(', ')
    f.write(')')

#write a function call to a file
def write_function_call(f, function_name, function):
    function_type = function[0]
    f.write('\t' + function_type + ' ' + function_name + 'val' + ' = ')
    f.write(function_name + '(')
    num_args = function[1]
    for arg_num in range(num_args):
        type = function[arg_num + 2]
        if type == 'int' or type == 'long':
            f.write(str(random.randint(0,16000)))
        elif type == 'float' or type == 'double':
            f.write(str(random.random()))
        else:
            f.write('"hello"')
        if arg_num != num_args - 1:
            f.write(', ')
    f.write(');\n')

# create a .c file for use within Python
def generate_c_file(file_prefix_in, my_id, num_functions, call_depth, extern, utility_enabled, fun_print, name_length):
    global extern_list
    global utility_list
    file_prefix = file_prefix_in + str(my_id)
    filename = file_prefix + '.c'
    f = open(filename, 'w')

    if file_prefix_in == 'libmodule':
        header = '#include <Python.h>\n'
        if utility_enabled:
            header += '#include "pynamic.h"\n'
    else:
        header = '#include "' + file_prefix + '.h"\n\n'
    f.write(header)

    if file_prefix_in == 'libmodule':
        #declare external function to call if not first module
        if my_id != 0 and extern:
            f.write('extern ')
            function_name = file_prefix_in + str(my_id - 1) + '_extern'
            function = extern_list[my_id - 1]
            write_function_declaration(f, function_name, function)
            f.write(';\n')

        #define my external function
        if extern:
            function_name = file_prefix + '_extern'
            function = extern_list[my_id]
            write_function_declaration(f, function_name, function)
            f.write('\n{\n')
            f.write('\t' + function[0] + ' ret_val;')
            if fun_print:
                f.write('\tprintf("I am ' + function_name + ' called from another module\\n");\n')
            f.write('\treturn ret_val;\n')
            f.write('}\n\n')

    #prepare function definitions
    functions = create_function_list(num_functions)
    if file_prefix_in == 'libutility':
        utility_list.append(functions)

    #function declarations
    if file_prefix_in == 'libmodule':
        for i in range(num_functions):
            function_name = file_prefix + '_fun' + str(i)
            for j in range(name_length):
                function_name += str(j%10)
            function = functions[i]
            write_function_declaration(f, function_name, function)
            f.write(';\n')
        f.write('\n')
    else:
        utility_header_name = file_prefix + '.h'
        utility_header_file = open(utility_header_name, 'w')
        utility_header_file.write('#include <stdio.h>\n#include <stdlib.h>\n\n')
        for i in range(num_functions):
            function_name = file_prefix + '_fun' + str(i)
            for j in range(name_length):
                function_name += str(j%10)
            function = functions[i]
            write_function_declaration(utility_header_file, function_name, function)
            utility_header_file.write(';\n')
        utility_header_file.write('\n')
        utility_header_file.close()

    for i in range(num_functions):
        #function declaration
        function_name = file_prefix + '_fun' + str(i)
        for j in range(name_length):
            function_name += str(j%10)
        function = functions[i]
        write_function_declaration(f, function_name, function)
        function_type = function[0]
        f.write('\n{\n')

        #variable declarations
        f.write('\t' + function_type + ' ret_val;\n')
        f.write('\tint a, b, c, loop;\n')
        f.write('\tfloat d = 0.0, e = 1.0, f = 2.0;\n')
        f.write('\tdouble g = 0.0, h = 3.0, i = 4.0;\n')
        f.write('\tchar j[10], k[100], l[10][10];\n\n')

        #function body
        if fun_print:
            f.write('\tprintf("In module ' + file_prefix + ' function ' + function_name + '\\n");\n')
        f.write('\tfor (loop = 0; loop < 10; loop++)\n\t{\n')
        f.write('\t\ta = loop;\n')

        if file_prefix_in == 'libmodule' and utility_enabled:
            utility_num = random.randint(0, len(utility_list) - 1)
            utility_file = utility_list[utility_num]
            utility_fun_num = random.randint(0, len(utility_file) - 1)
            callee = utility_file[utility_fun_num]
            callee_name = 'libutility' + str(utility_num) + '_fun' + str(utility_fun_num)
            for j in range(name_length):
                callee_name += str(j%10)
            f.write('\t')
            write_function_call(f, callee_name, callee)

        f.write('\t}\n')

        #call the next function if not the last function, and not
        #the last function in the max call depth
        if i != num_functions - 1 and i % call_depth != call_depth - 1:
            callee = functions[i + 1]
            callee_name = file_prefix + '_fun' + str(i + 1)
            for j in range(name_length):
                callee_name += str(j%10)
            write_function_call(f, callee_name, callee)

        f.write('\treturn ret_val;\n}\n\n')

    if file_prefix_in == 'libmodule':
        #Python callable entry function
        function_name = file_prefix + '_entry'
        f.write('static PyObject *py_' + function_name + '(')
        f.write('PyObject *self, PyObject *args)\n{\n')
        f.write('\tint ret_val = 0;\n')
        for i in range(num_functions):
            if i % call_depth == 0:
                callee = functions[i]
                callee_name = file_prefix + '_fun' + str(i)
                for j in range(name_length):
                    callee_name += str(j%10)
                write_function_call(f, callee_name, callee)

        #call the previous module's extern function
        if my_id != 0 and extern:
            callee = extern_list[my_id - 1]
            callee_name = file_prefix_in + str(my_id - 1) + '_extern'
            write_function_call(f, callee_name, callee)

        f.write('\treturn Py_BuildValue("i", ret_val);\n}\n\n')

        #Python module initialization code
        f.write('static PyMethodDef ' + file_prefix + 'Methods[] = {\n')
        f.write('\t{"' + function_name + '", py_' + function_name + ', METH_VARARGS, "a function."},\n')
        f.write('\t{NULL, NULL, 0, NULL}\n')
        f.write('};\n\n')
        if sys.version_info.major == 2:
            f.write('void init' + file_prefix + '()\n')
            f.write('{\n')
            f.write('\tPy_InitModule("' + file_prefix + '", ' + file_prefix + 'Methods);\n')
            f.write('}\n\n')
        else:
            f.write('PyMODINIT_FUNC PyInit_' + file_prefix + '()\n')
            f.write('{\n')
            f.write('\tstatic struct PyModuleDef mod = {\n')
            f.write('\t\tPyModuleDef_HEAD_INIT,\n')
            f.write('"' + file_prefix + '",\n')
            f.write('"",\n')
            f.write('-1,\n')
            f.write(file_prefix + 'Methods\n')
            f.write('};\n')
            f.write('return PyModule_Create(&mod);\n')
            f.write('}\n\n')
    f.close()

# compile a .c file into a Python-usable .so file
def compile_file(file_prefix, num_module_files, num_utility_files, include_dir, CC):
    filename = file_prefix + '.c'
    cwd = os.getcwd()
    outfile = file_prefix + '.so'
    if (CC.find('xl')) != -1:
        command = '%s -g -qmkshrobj' %(CC)
    else:
        command = '%s -g -fPIC -shared' %(CC)
    if file_prefix.find('module') != -1:
        if file_prefix.find('begin') != -1:
            for i in range(num_module_files):
                command += ' -lmodule' + str(i)
        command += ' -I%s' %(include_dir)
        command += ' -Wl,-rpath=' + cwd + ' -L' + cwd
        for i in range(num_utility_files):
            command += ' -lutility' + str(i)

    command += ' -o ' + outfile + ' ' + filename
    run_command(command)

    # create .o file
    outfile = file_prefix + '.o'
    command = '%s -g -fPIC -c' %(CC)
    command += ' -o ' + outfile + ' ' + filename
    command += ' -I%s' %(include_dir)
    run_command(command)

#create a python driver file
def create_driver(num_files, filename, mpi_wrapper_text):
    f = open(filename, "w")
    text = """import sys, os
import time
end_time = time.time()
start_time = 0
mpi_avail = True
try:
"""
    text += mpi_wrapper_text
    text += """    mpi = mpi_wrapper()
except:
    class dummy_mpi:
        def __init__(self):
            self.rank = 0
            self.procs = 1
        def barrier(self):
            pass
    mpi = dummy_mpi()
    mpi_avail = False

mpi.barrier()
myRank = mpi.rank
nProcs = mpi.procs
if myRank == 0:
    print('Pynamic: Version 1.3.3')
    print('Pynamic: run on %s with %s MPI tasks\\n' %(time.strftime("%x %X"), nProcs))
    if len(sys.argv) > 1:
        start_time = float(sys.argv[1])
        print('Pynamic: startup time = ' + str(end_time - start_time) + ' secs')
    print('Pynamic: driver beginning... now importing modules')

    import_start = time.time()
"""
    f.write(text)

    f.write('import libmodulebegin\n')
    for i in range(num_files):
        f.write('import libmodule' + str(i) + '\n')
    f.write('import libmodulefinal\n')

    text = """mpi.barrier()
if myRank == 0:
    import_end = time.time()
    import_time = import_end - import_start
    print('Pynamic: driver finished importing all modules... visiting all module functions')

    call_start = time.time()
"""
    f.write(text)

    f.write('libmodulebegin.begin_break_here()\n')
    for i in range(num_files):
        f.write('libmodule' + str(i) + '.libmodule' + str(i) + '_entry()\n')
    f.write('libmodulefinal.break_here()\n')

    text = """mpi.barrier()
if myRank == 0:
    call_end = time.time()
    call_time = call_end - call_start
    print('Pynamic: module import time = ' + str(import_time) + ' secs')
    print('Pynamic: module visit time = ' + str(call_time) + ' secs')
    print('Pynamic: module test passed!\\n')
if mpi_avail == False:
    sys.exit(0)

if myRank == 0:
    print('Pynamic: testing mpi capability...\\n')
    mpi_start = time.time()
"""
    f.write(text)

    #test mpi capabilities
    fractal_file = open('./examples/fractal.py', 'r')
    lines = fractal_file.readlines()
    for line in lines[1:]:
        if line.find('message') == -1: #suppress each task printing
            f.write(line)

    text = """mpi.barrier()
if myRank == 0:
    mpi_end = time.time()
    print('\\nPynamic: fractal mpi time = ' + str(mpi_end - mpi_start) + ' secs')
    print('Pynamic: mpi test passed!\\n')
"""
    f.write(text)

    f.close()

#create a function list    (type + args quantity and types)
def create_function_list(num_functions):
    functions = []
    for i in range(num_functions):
        function = []
        function_type = var_types[random.randint(0, len(var_types) - 1)]
        function.append(function_type)
        num_args = random.randint(0,5)
        function.append(num_args)
        for arg_num in range(num_args):
            type = var_types[random.randint(0, len(var_types) - 1)]
            function.append(type)
        functions.append(function)
    return functions

#the main driver
def run_so_generator(num_files, avg_num_functions, call_depth, extern, seed, seedval, num_utility_files, avg_num_u_functions, fun_print, name_length, include_dir, CC, processes):

    for p,d,f in os.walk('./'):
        if p == './':
            for file in f:
                if (file.find('libmodule') != -1 or file.find('libutility') != -1 or file.find('pynamic.h') != -1) and file.find('libmodulefinal.c') == -1 and file.find('libmodulebegin.c') == -1:
                    os.remove(file)

    if extern:
        global extern_list
        extern_list = create_function_list(num_files)

    if seed == True:
        random.seed(seedval)

    utility_enabled = False

    command = 'rm -f libpynamic.a'
    run_command(command)

    pynamic_header_name = 'pynamic.h'
    pynamic_header_file = open(pynamic_header_name, 'w')
    pynamic_header_file.write('#include <math.h>\n')
    pool = mp.Pool(processes=processes)
    if num_utility_files > 0:
        global utility_list
        utility_list = []
        utility_enabled = True

        file_prefix = 'libutility'
        for i in range(num_utility_files):
            num_functions = random.randint(avg_num_u_functions/2, avg_num_u_functions*3/2)
            pynamic_header_file.write('#include "' + file_prefix + str(i) + '.h"\n')
            generate_c_file(file_prefix, i, num_functions, call_depth, extern, utility_enabled, fun_print, name_length)
        results = [pool.apply_async(compile_file, args=(file_prefix+str(i), i, num_utility_files, include_dir, CC)) for i in range(num_utility_files)]
        [p.get() for p in results]

    compile_file("libmodulefinal", 0, 0, include_dir, CC)
    command = 'ar cru libpynamic.a libmodulefinal.o'
    run_command(command)

    if num_utility_files > 0:
        command = 'ar cru libpynamic.a '
        for i in range(num_utility_files):
            command = '%s %s' %(command, file_prefix+str(i)+'.o')
        run_command(command)
    pynamic_header_file.write('void initlibmodulebegin();\n')
    for i in range(num_files - num_utility_files):
        pynamic_header_file.write('void initlibmodule%d();\n' %(i))
    pynamic_header_file.write('void initlibmodulefinal();\n')
    pynamic_header_file.close()

    file_prefix = 'libmodule'
    for i in range(num_files - num_utility_files):
        num_functions = random.randint(avg_num_functions/2, avg_num_functions*3/2)
        generate_c_file(file_prefix, i, num_functions, call_depth, extern, utility_enabled, fun_print, name_length)
    results = [pool.apply_async(compile_file, args=(file_prefix+str(i), i, num_utility_files, include_dir, CC)) for i in range(num_files - num_utility_files)]
    [p.get() for p in results]
    if num_files - num_utility_files > 0:
        command = 'ar cru libpynamic.a '
        for i in range(num_files - num_utility_files):
            command = '%s %s' %(command, file_prefix+str(i)+'.o')
        run_command(command)

    command = 'ranlib libpynamic.a'
    run_command(command)

    compile_file("libmodulebegin", num_files - num_utility_files, 0, include_dir, CC)
    command = 'ar cru libpynamic.a libmodulebegin.o'
    run_command(command)

    f = open("pyMPI_initialize.c", "r")
    lines = f.readlines()
    f.close()
    f = open("pynamic_sdb_pyMPI_initialize.c", "w")
    for line in lines:
        f.write(line)
        if line.find('pyMPI_Macros.h') != -1:
            f.write('#include "pynamic.h"\n')
        if line.find('PyImport_AppendInittab') != -1:
            f.write('  PyImport_AppendInittab("libmodulebegin", initlibmodulebegin);\n')
            for i in range(num_files - num_utility_files):
                f.write('  PyImport_AppendInittab("libmodule%d", initlibmodule%d);\n' %(i, i))
            f.write('  PyImport_AppendInittab("libmodulefinal", initlibmodulefinal);\n')
    f.close()

    print('Generating driver...')

    mpi_wrapper_text = """    import mpi as actual_mpi
    class mpi_wrapper:
        def __init__(self):
            self.rank = actual_mpi.rank
            self.procs = actual_mpi.procs
            self.SUM = actual_mpi.SUM
        def reduce(self, buffer, operation, destination):
            return actual_mpi.reduce(buffer, operation, destination)
        def barrier(self):
            actual_mpi.barrier
"""
    create_driver(num_files - num_utility_files, "pynamic_driver.py", mpi_wrapper_text)
    mpi_wrapper_text = """    from mpi4py import MPI as actual_mpi
    class mpi_wrapper:
        def __init__(self):
            self.rank = actual_mpi.COMM_WORLD.Get_rank()
            self.procs = actual_mpi.COMM_WORLD.Get_size()
            self.SUM = actual_mpi.SUM
        def reduce(self, buffer, operation, destination):
            return actual_mpi.COMM_WORLD.reduce(buffer, op=operation, root=destination)
        def barrier(self):
            return actual_mpi.COMM_WORLD.Barrier()
"""
    create_driver(num_files - num_utility_files, "pynamic_driver_mpi4py.py", mpi_wrapper_text)
    print('Done!\n')

def print_usage(executable):
    config_options = ''
    if executable == 'config_pynamic.py':
        config_options = '[-c <configure_options>]'
    print('\nUSAGE:\n\t%s <num_files> <avg_num_functions> [options] %s\n' %(executable, config_options))
    print('\t<num_files> = total number of shared objects to produce')
    print('\t<avg_num_functions> = average number of functions per shared object')
    print('\nOPTIONS:\n')
    if executable == 'config_pynamic.py':
        print('-c <configure_options>')
        print('\tpass the whitespace separated list of <configure_options> to configure')
        print('\twhen building pyMPI.  All args after -c are sent to configure and not ')
        print('\tinterpreted by Pynamic\n')
    print('-b\n\tgenerate the pynamic-bigexe-pyMPI and pynamic-bigexe-sdb-pyMPI executables\n')
    print('-d <call_depth>\n\tmaximum Pynamic call stack depth, default = 10\n')
    print('-e\n\tenables external functions to call across modules\n')
    print('-i <python_include_dir>\n\tadd <python_include_dir> when compiling modules\n')
    print('-j <num_processes>\n\tbuild in parallel with a max of <num_processes> processes\n')
    print('-n <length>\n\tadd <length> characters to the function names\n')
    print('-p\n\tadd a print statement to every generated function\n')
    print('-s <random_seed>\n\tseed to the random number generator\n')
    print('-u <num_utility_mods> <avg_num_u_functions>')
    print('\tcreate <num_utility_mods> math library-like utility modules')
    print('\twith an average of <avg_num_u_functions> functions')
    print('\tNOTE: Number of python modules = <num_files> - <avg_num_u_functions>\n')
    print('--with-cc=<command>')
    print('\tuse the C compiler located at <command> to build Pynamic modules.\n')
    print('--with-python=<command>')
    print('--with-mpi4py')
    print('\tBuild with mpi4py. Default on Python3+')
    print('--with-pympi')
    print('\tBuild with pympi. Default on Python2')
    print('\tuse the python located at <command> to build Pynamic modules.  Will')
    print('\talso be passed to the pyMPI configure script.\n')
    sys.exit(-1)

def print_error(error_msg, exit_on_error=True):
    print('\n************************* ERROR *************************')
    print(error_msg)
    print('*********************************************************\n')
    if exit_on_error == True:
        sys.exit(-1)

def parse_and_run(executable):
    #parse and extract command line args
    exit = 0
    try:
        num_files = int(sys.argv[1])
        avg_num_functions = int(sys.argv[2])
        num_utility_files = 0
        avg_num_u_functions = 0
        call_depth = 10
        bigexe = False
        extern = False
        seed = False
        seedval = -1
        next = 0
        fun_print = False
        use_mpi4py = False
        name_length = 0
        include_dir = ''
        configure_args = []
        python_command = sys.executable
        processes = 1
        if sys.version_info.major > 2:
            use_mpi4py = True
            
        try:
            CC = os.environ['CC']
        except:
            CC = 'gcc'

        for i in range(3, len(sys.argv)):
            if next == 0:
                if sys.argv[i] == '-b':
                    bigexe = True
                    try:
                        os.environ['CFLAGS'] += ' -DBUILD_PYNAMIC_BIGEXE'
                    except:
                        os.environ['CFLAGS'] = '-DBUILD_PYNAMIC_BIGEXE'
                elif sys.argv[i] == '-e':
                    extern = True
                elif sys.argv[i] == '-d':
                    call_depth = int(sys.argv[i + 1])
                    next = 1
                elif sys.argv[i] == '-s':
                    seed = True
                    seedval = int(sys.argv[i + 1])
                    next = 1
                elif sys.argv[i] == '-u':
                    num_utility_files = int(sys.argv[i + 1])
                    avg_num_u_functions = int(sys.argv[i + 2])
                    next = 2
                elif sys.argv[i] == '-p':
                    fun_print = True
                elif sys.argv[i] == '-n':
                    name_length = int(sys.argv[i + 1])
                    next = 1
                elif sys.argv[i] == '-i':
                    include_dir = sys.argv[i + 1]
                    next = 1
                elif sys.argv[i] == '-j':
                    processes = int(sys.argv[i + 1])
                    next = 1
                elif sys.argv[i] == '-c':
                    configure_args += sys.argv[i+1:]
                    next = 99999
                elif sys.argv[i].find('--with-cc=') != -1:
                    CC = sys.argv[i][10:]
                elif sys.argv[i].find('--with-python=') != -1:
                    configure_args.append(sys.argv[i])
                    python_command = sys.argv[i][14:]
                elif sys.argv[i].find('--with-mpi4py') != -1:
                    use_mpi4py = True
                elif sys.argv[i].find('--with-pympi') != -1:
                    use_mpi4py = False
                else:
                    print_error('Unknown option %s' %(sys.argv[i]))
                    print_usage(executable)
            else:
                next = next - 1

        if include_dir == '':
            # try to automatically find include directory for default python
            include_dir = get_paths()['include']
    except Exception as e:
        import traceback
        print(repr(e))
        traceback.print_exc()
        if exit == 1:
            # handle caught exit
            sys.exit(-1)
        print('\n#############################')
        print('# invalid arguments passed! #')
        print('#############################')
        print_usage(executable)
        
    run_so_generator(num_files, avg_num_functions, call_depth, extern, seed, seedval, num_utility_files, avg_num_u_functions, fun_print, name_length, include_dir, CC, processes)

    if use_mpi4py:
        os.environ["NUM_UTILITIES"] = str(num_utility_files)
        os.environ["NUM_MODULES"] = str(num_files - num_utility_files)
        os.environ["PYTHON_EXE"] = python_command

    return configure_args, python_command, bigexe, use_mpi4py, processes

#MAIN FUNCTION
if __name__ == '__main__':
    if len(sys.argv) < 3:
        print_usage('python so_generator.py')
    parse_and_run('python so_generator.py')

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
