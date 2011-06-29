#**************************************************************************#
#* FILE   **************  expose_configuration.py  ************************#
#**************************************************************************#
#* Author: Patrick Miller January 16 2003                                 *#
#**************************************************************************#
"""
Walk through the #defines in the autoconf/automake file and expose
each item as an integer, float, or string
"""

import sys
import re

#sys.stdout = sys.stderr

config_file = sys.argv[1]

def cstring(s):
    return '"'+repr(s)[1:-1].replace('"','\\\"').replace("'",'\\\'')+'"'

# -----------------------------------------------
# Pull #define values out of config.h file
# -----------------------------------------------
pattern = re.compile(r'#define\s+(\S+)\s+(.*)')
last_comment = ''
definitions = {}
for line in open(config_file).xreadlines():
    if line.startswith('/*'):
        last_comment = line[3:-4]
    elif line.startswith('#define'):
        # Parse out the name and value
        match = pattern.match(line)
        name = match.group(1)
        if match.group(2):
            value = eval(match.group(2))
        else:
            value = None
        definitions[name] = (value,last_comment)
        last_comment = ''

# -----------------------------------------------
# Build the configuration file
# -----------------------------------------------
print r'''
/**************************************************************************/
/* FILE ****************  pyMPI_configuration.c ***************************/
/**************************************************************************/
/* Author: Machine generated                                              */
/**************************************************************************/

#include "mpi.h"
#undef _POSIX_C_SOURCE
#include "Python.h"
#include "pyMPI.h"
#include "pyMPI_Macros.h"

START_CPLUSPLUS

/**************************************************************************/
/* GLOBAL **************  pyMPI_configuration_fini ************************/
/**************************************************************************/
/* No global state to clean up here.  Just used to fulfill the contract   */
/* with the PLUGIN macro.                                                 */
/**************************************************************************/
void pyMPI_configuration_fini(void) {
}

/**************************************************************************/
/* LOCAL  **************     increment_refcount    ************************/
/**************************************************************************/
/* This simply is used to adapt to the model of the PARAMETER macro which */
/* expects its "converter" argument to return an owned reference to the   */
/* first argument.  When that argument is a borrowed reference (as here   */
/* where the Py_None singleton is passed in as the parameter), we need to */
/* INCREF it.                                                             */
/**************************************************************************/
static PyObject* increment_refcount(PyObject* x) {
  Py_XINCREF(x);
  return x;
}

/**************************************************************************/
/* GLOBAL **************  pyMPI_configuration_init ************************/
/**************************************************************************/
/* This builds a submodule (mpi.configuration) which is populated with    */
/* the values set in pyMPI_Config.h.  This includes useful information    */
/* like the flags and compilers used to build pyMPI.  This is more direct */
/* and more complete than what we can patch into distutils.               */
/**************************************************************************/
void pyMPI_configuration_init(PyObject** docStringP) {
   PyObject* configuration = 0;
   PyObject* configuration_dictionary = 0;
   PyObject* doc_string = 0;
   static PyMethodDef configuration_methods[] = {{0,0}};

   PYCHECK( configuration /*owned*/ = Py_InitModule("mpi.configuration",
                                                     configuration_methods) );
   PYCHECK( configuration_dictionary /*borrowed*/ = PyModule_GetDict(configuration) );
   PYCHECK( doc_string /*owned*/ = PyString_FromString("pyMPI configuration information\n\nConfiguration information\n\n") );
'''
keys = definitions.keys()
keys.sort()
for key in keys:
    value,comment = definitions[key]
    macro = key
    if isinstance(value,int):
        converter = 'PyInt_FromLong'
    elif isinstance(value,str):
        converter = 'PyString_FromString'
    elif isinstance(value,float):
        converter = 'PyInt_FromLong'
    elif value is None:
        converter = 'increment_refcount'
        macro = 'Py_None'
    else:
        raise key,value
    print '   PARAMETER(%s,"%s",%s,%s,configuration_dictionary,&doc_string);'%(
        macro,key,cstring(comment),converter)

print
print '   PYCHECK( PyDict_SetItemString(configuration_dictionary,"__doc__",doc_string) );'
print '   PYCHECK( PyDict_SetItemString(pyMPI_dictionary,"configuration",configuration) );'
print '   return;'
print ''
print ' pythonError:'
print '   Py_XDECREF(configuration);'
print '   Py_XDECREF(doc_string);'
print '   return;'
print '}'

print
print 'END_CPLUSPLUS'


