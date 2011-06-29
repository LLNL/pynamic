#**************************************************************************#
#* FILE   **************      editMakefile.py      ************************#
#**************************************************************************#
#* Author: Patrick Miller November 11 2002                                *#
#**************************************************************************#
"""
Read a file in Makefile format and change a variable definition
usage: python editMakefile.py VARNAME new def < Makefile > Makefile.new
"""
#**************************************************************************#

import sys
import string
import re

def adjust_Makefile_vars(Makefile,varname,defn):
    # Remove continuation lines (change \<nl> into a space)
    lines = string.join(string.split(Makefile,'\\\n'),' ')
    pattern = re.compile(r'^%s\s*(:?=).*'%varname,re.MULTILINE)
    m = pattern.search(lines)
    if m is None:
        raise ValueError,varname
    assign = m.group(1)
    return lines[:m.start()]+ "%s%s%s"%(varname,assign,defn)+lines[m.end():]

if __name__ == '__main__':
    body = sys.stdin.read()
    if len(sys.argv) < 2:
        assert len(sys.argv) > 0
        sys.stderr.write('usage: %s VARNAME new def < Makefile\n'%sys.argv[0])
        sys.exit(1)
    try:
        sys.stdout.write(adjust_Makefile_vars(body, sys.argv[1], string.join(sys.argv[2:])))
    except ValueError:
        sys.stderr.write('Variable %s not in file\n'%sys.argv[1])
        sys.exit(1)
        
    
