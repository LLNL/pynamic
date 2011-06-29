#**************************************************************************#
#* FILE   **************      findExternals.py     ************************#
#**************************************************************************#
#* Author: Patrick Miller May 16 2002                                     *#
#**************************************************************************#
#*  *#
#**************************************************************************#

import os
import re
import string
import sys
import xml.sax.saxutils

typedef  = re.compile(r'typedef[^;]*')
variable = re.compile(r'/\*\s+extern\s+\*/\s*(\w+\s*\**)([^=;]*)')
function = re.compile(r'extern\s+DL_IMPORT[^;]*')
uppercase = re.compile(r'[ABCDEFGHIJKLMNOPQRSTUVWXYZ]')
lowercase = re.compile(r'[abcdefghijklmnopqrstuvwxyz]')

##################################################################
#                       FUNCTION CHECKDOC                        #
##################################################################
def checkdoc(doc,name,file,body,start):
    line = body.count('\n',0,start)+1
    idx = string.rfind(body,'/* GLOBAL',0,start)

    # -----------------------------------------------
    # Better have a doc string....
    # -----------------------------------------------
    if idx < 0:
        sys.stderr.write('%s:%d: Warning: No doc string for %r\n'%(file,line,name))
        return

    # -----------------------------------------------
    # See if the name matches
    # -----------------------------------------------
    doc_string = body[idx:start]
    doc_lines = doc_string.split('\n')
    header = ' '.join(doc_lines[0].replace('*','').replace('/','').split())
    if header != 'GLOBAL '+name:
        sys.stderr.write('%s:%d: Warning: %r documented by %s\n'%(file,line,name,header))
        return

    # -----------------------------------------------
    # Pull out the doc lines
    # -----------------------------------------------
    dox = ''
    for L in doc_lines[1:]:
        L = L.strip()
        if not L:
            continue
        if L[:2] != '/*':
            sys.stderr.write('%s:%d: Warning: comment must start /*\n'%(file,line))
        if L[-2:] != '*/':
            sys.stderr.write('%s:%d: Warning: comment must end */\n'%(file,line))
        if L.endswith(' **/'):
            sys.stderr.write('%s:%d: Warning: Stupid " **/"\n'%(file,line))

        comment = L[2:-2]
        if filter(lambda x: x != '*',comment):
            dox += comment+'\n'

    if dox.strip() == '':
        sys.stderr.write('%s:%d: Warning: Empty doc string -- %r\n'%(file,line,name))
    doc[name] = dox
    return


##################################################################
#                       FUNCTION CHECKNAME                       #
##################################################################
def checkName(name,file,body,pos):
    "checkName(name,file,body,pos)\n\nIssue a warning on stderr if name is not pyMPI legal"
    # Convert to identifier (filter out *())
    identifier = filter(lambda x: x in string.letters or x in string.digits or x not in '*()', name)

    # Straight "initXXXX" names are always allowed
    if identifier[:4] == 'init':
        return

    # In file pyMPI_zoop.c, we favor names like pyMPI_zoop_XXXX
    # In file pyMPI_foop_goop.c, we favor names like
    # pyMPI_goop_XXXXX
    directory,filename = os.path.split(file)
    fileroot = os.path.splitext(filename)[0]
    root = string.split(fileroot,'_')[-1]
    startswith = 'pyMPI_'+root+'_'

    # But, names that match the fileroot are always allowed
    if identifier == fileroot or identifier == startswith[:-1]:
        return


    length = len(startswith)
    if identifier != fileroot and identifier[:length] != startswith:
        line = body[:pos].count('\n')
        sys.stderr.write('%s:%d: Warning: Expected %r to start with %s\n'%(
            file,line,name,startswith))
        return

    # We are stomping out camel case
    if uppercase.match(identifier) is not None and lowercase.match(identifier) is not None:
        sys.stderr.write('%s:%d: Warning: %r is in camelCase %s\n'%(
            file,line,name))
    
    return


##################################################################
#                     FUNCTION FINDEXTERNALS                     #
##################################################################
def findExternals(files,doc):
    "findExernals(files)\n\nSearch for global variables and functions"
    types = {}
    externals = {}
    for file in files:
        types[file] = []
        externals[file] = []

        body = open(file).read()

        # -----------------------------------------------
        # Look for typedef ... lines
        # -----------------------------------------------
        pos = 0
        while 1:
            match = typedef.search(body,pos)
            if not match: break
            pos = match.end()+1

            T = match.group(0)+';'
            n = T.count('{')
            while n or body[pos] != ';':
                if body[pos] == '}': n -= 1
                T += body[pos]
                pos += 1
            types[file].append(T+';')

        # -----------------------------------------------
        # Look for /* extern */ variable_decl lines
        # -----------------------------------------------
        pos = 0
        while 1:
            match = variable.search(body,pos)
            if not match: break
            pos = match.end()+1

            type = match.group(1).strip().replace('\n',' ')
            name = match.group(2).strip().replace('\n',' ')

            externals[file].append('extern DL_IMPORT(%s) %s;'%(type,name))

            if name[:6] != 'pyMPI_':
                sys.stderr.write('%s:%d: Warning: variable %r should start with pyMPI_\n'%(
            file,line,name))

        
        # -----------------------------------------------
        # Look for global functions 
        # -----------------------------------------------
        pos = 0
        while 1:
            match = function.search(body,pos)
            if not match: break
            pos = match.end()+1
            
            extern = match.group(0).replace('\n',' ')
            name = extern.split('(')[1].split(')')[1].strip()

            checkdoc(doc,name,file,body,match.start())

            externals[file].append( extern + ';' )
            checkName(name,file,body,pos)


    result = []
    for file in files:
        if types[file]:
            result.append('/* %s */'%file)
            result += types[file]
            result.append('')

    for file in files:
        if externals[file]:
            result.append('/* %s */'%file)
            result += externals[file]
            result.append('')

    return result

if __name__ == '__main__':
    import sys
    dox = {}
    externals = findExternals(sys.argv[1:],doc=dox)
    keys = dox.keys()
    keys.sort()
    print '#if 0'
    print '<documentation>'
    for key in keys:
        print '<doc name=%s>'%xml.sax.saxutils.quoteattr(key)
        print xml.sax.saxutils.escape(dox[key],{"'":'&apos;','"':'&quot;'})
        print '</doc>'
    print '</documentation>'
    print '#endif\n'
    print string.join(externals,'\n')
