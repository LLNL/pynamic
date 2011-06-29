#**************************************************************************#
#* FILE   ************** grind_docs_and_prototypes.py *********************#
#**************************************************************************#
#* Author: Patrick Miller January 21 2003                                 *#
#**************************************************************************#
"""
"""


import sys
import os
import re
import compiler
import parser


##################################################################
#                        FUNCTION CSTRING                        #
##################################################################
def cstring(s):
    return '"'+repr(s)[1:-1].replace('"','\\\"').replace("'",'\\\'')+'"'

##################################################################
#                         CLASS GRINDER                          #
##################################################################
class Grinder:
    def check(self,source,file):
        raise NotImplementedError

    def grind(self):
        return

    def warning(self,file,line,message):
        sys.stderr.write('%s:%d: Warning: %s\n'%(file,line,message))
        return



##################################################################
#                       CLASS CHECKSTRING                        #
##################################################################
class CheckString(Grinder):
    pattern = None

    def check(self,source,file):
        start = 0
        while 1:
            match = self.pattern.search(source,start)
            if match is None: break

            line = source.count('\n',0,match.start())+1
         
            message = self.message(match)
            if message:
                self.warning(file,line,message)

            start = match.end()
        return

    
##################################################################
#                        CLASS CHECKTABS                         #
##################################################################
class CheckTabs(CheckString):
    pattern = re.compile('\t')

    def message(self,match):
        return 'Using a tab character'

##################################################################
#                        CLASS CHECKTODO                         #
##################################################################
class CheckTODO(CheckString):
    pattern = re.compile('TODO:.*')

    def message(self,match):
        todo = match.group()
        if todo.endswith('*/'):
            todo = todo[:-2]

        return todo.strip()


##################################################################
#                      CLASS CHECKSPACESTAR                      #
##################################################################
class CheckSpaceStar(CheckString):
    pattern = re.compile(r'\*\*\*\*\*\*\*\*\*\*\*\* \*\*/')

    def message(self,match):
        return 'Broken star line'

##################################################################
#                     CLASS CHECKOPENCOMMENT                     #
##################################################################
class CheckOpenComment(CheckString):
    pattern = re.compile(r'/\*\s*\-+.*')

    def message(self,match):
        if match.group().endswith('*/'): return None
        return '/* comment without close on same line'


##################################################################
#                          CLASS DOCBOX                          #
##################################################################
class DocBox(Grinder):
    def __init__(self):
        self.dox = {}
        self.externals = {}
        return

    def check(self,source,file):
        start = 0
        while 1:
            pattern = re.compile(r'/\*+\*/\s/\*\s+(\w+)\s+\*+\s+(\S+)\s+\*+/\s/[\*\s]+\*/\s')
            match = pattern.search(source,start)
            if match is None: break

            # -----------------------------------------------
            # Here, we can break out the kind and "value"
            # -----------------------------------------------
            line = source.count('\n',0,match.start())+1
            kind = match.group(1)
            name = match.group(2)

            # -----------------------------------------------
            # Pick up the doc lines (/* ... */)
            # -----------------------------------------------
            slash_star = re.compile('/\*.+[\n]')
            start = match.end()
            arguments = []
            arg = ''
            while 1:
                match = slash_star.match(source,start)
                if match is None: break
                comment = match.group().strip()

                # -----------------------------------------------
                # Look for unterminated comments
                # -----------------------------------------------
                if not comment.endswith('*/'):
                    self.warning(file,source.count('\n',0,match.start())+1,'Unterminated comment')
                # Strip off the /* and */
                arg_line = match.group()[3:-3]
                # If the line is only ******, then we start a new arg
                # otherwise we append to the current arg
                if arg_line == len(arg_line)*'*':
                    if arg:
                        arguments.append(arg)
                        arg = ''
                else:
                    arg += arg_line.rstrip()+'\n'
                start = match.end()

            # -----------------------------------------------
            # The next line can affect processing
            # -----------------------------------------------
            one_line = re.compile('.*')
            match = one_line.match(source,start)
            assert match,"There must be a follow on line"
            start = match.end()
            follow_line = match.group()

            # -----------------------------------------------
            # Process the doc box
            # -----------------------------------------------
            try:
                method = getattr(self,kind)
            except:
                try:
                    method = self.OTHER
                except:
                    method = None
            if method is None:
                raise AttributeError,kind
                
            try:
                method(kind,name,file,line,follow_line,*arguments)
            except:
                self.warning(file,line,'FORMAT ERROR')
                raise

        return


##################################################################
#                      CLASS FINDEXTERNALS                       #
##################################################################
class FindExternals(DocBox):
    def grind(self):
        global width
        files = self.externals.keys()
        files.sort()
        for file in files:
            print '/*%s*/'%('*'*width)
            print '/*%s*/'%file.center(width)
            print '/*%s*/'%('*'*width)
            for external in self.externals[file]:
                print external
            print

    def MACRO(self,kind,name,file,line,follow_line,*args):
        return

    def TYPE(self,kind,name,file,line,follow_line,*args):
        return

    def FILE(self,kind,name,file,line,follow_line,author,information=''):
        # -----------------------------------------------
        # The name in the FILE box must match the file
        # name (checks for file renaming).
        # -----------------------------------------------
        if file != name:
            self.warning(file,line+1,"Doc filename does't match, %r"%name)

        # -----------------------------------------------
        # Make sure there is an author line
        # -----------------------------------------------
        if not author.startswith('Author'):
            self.warning(file,line+3,'Expected an Author line')

        # -----------------------------------------------
        # Initialize set of file document info
        # -----------------------------------------------
        if file in self.dox:
            self.warning(file,line,'repeats FILE box')
        else:
            self.dox[file] = [information]

        # -----------------------------------------------
        # Initialize set of file external linkage info
        # -----------------------------------------------
        if file in self.externals:
            self.warning(file,line,'repeats FILE box')
        else:
            self.externals[file] = []
        return


    def METHOD(self,kind,name,file,line,follow,doc=None,pydoc=None):
        self.LOCAL(kind,name,file,line,follow,doc)
        self.method(kind,name,file,line,follow,doc,pydoc)
        return

    def GMETHOD(self,kind,name,file,line,follow,doc=None,pydoc=None):
        self.GLOBAL(kind,name,file,line,follow,doc)
        self.method(kind,name,file,line,follow,doc,pydoc)
        return

    def method(self,kind,name,file,line,follow,doc=None,pydoc=None):
        if pydoc is None:
            self.warning(file,line,'Python documentation missing')
            pydoc = 'Undocumented functionality'
        uname = name.upper()
        if not uname.startswith('PYMPI_'):
            uname = 'PYMPI_'+uname
        self.externals[file].append('#define %s_DOC %s'%(uname,cstring(pydoc)))
        return

    def LOCAL(self,kind,name,file,line,follow,doc=None):
        if doc is None: doc = ''
        
        # -----------------------------------------------
        # We make sure we have a FILE box under our belt
        # -----------------------------------------------
        if file not in self.dox:
            self.warning(file,1,"%s doesn't have a FILE box"%file)
            self.FILE(kind,name,file,line,follow,'undocumented')

        if doc.strip() == '':
            self.warning(file,line+3,'Empty doc for %s'%name)

        self.dox[file].append('<local>%s</local>'%(doc))
        if follow.find(name) < 0:
            self.warning(file,line+3+doc.count('\n'),"%s documents %r"%(name,follow))
        return

    def GLOBAL(self,kind,name,file,line,follow,doc=None):
        self.LOCAL(kind,name,file,line,follow,doc)

        # kill any trailing spaces
        follow = follow.strip()

        if '=' in follow:
            self.externals[file].append('extern %s;'%follow[:follow.index('=')-1])

        elif ';' in follow:
            self.externals[file].append('extern %s;'%follow[:follow.index(';')-1])

        elif follow.endswith('{'):
            decl = follow[:-1]

            pattern = re.compile(r'(\w+)(\s*)\(')
            match = pattern.search(decl)
            if match.group(2):
                self.warning(file,line+4+doc.count('\n'),'Space before ( in file decl')

            rtype = decl[:match.start()]
            name = match.group(1)
            signature = decl[match.end()-1:]
            proto = 'extern DL_IMPORT(%s) %s%s;'%(rtype.strip(),name,signature)
            if not name.startswith('pyMPI'):  # WORKAROUND FOR KULL/ROSE
                proto = '/* '+proto+' */'

            self.externals[file].append(proto)
        else:
            raise ValueError,follow
        return


##################################################################
#                        CLASS MICROTESTS                        #
##################################################################
class MicroTests(DocBox):
    """MicroTests

    We look through the dox boxes for things that look like Python
    code snippets (this is like the 'doctest' module), but we have
    parallization issues to deal with.
    """

    def __init__(self,*args,**kw):
        self.test_count = 0
        self.test_prelude = ''
        self.micro_tests = {}
        DocBox.__init__(self,*args,**kw)
        return

    def process_box(self,name,file,line,box):
        pattern = re.compile(r'(\n[>.][>.][>.].*)+')
        match = pattern.search(box)
        scripts = []
        while match is not None:
            source = match.group()
            script = ('# %s\n'%name+
                      '__FILE__ = %r\n'%file +
                      '__LINE__ = %r'%line +
                      '\n'.join( [text[4:] for text in source.split('\n')] )+
                      '\n'
                      )
            scripts.append(script)
            match = pattern.search(box,match.end())
        return scripts

    def find_scripts(self,name,file,line,args):
        scripts = []
        for box in args:
            scripts += self.process_box(name,file,line,box)        
        return scripts

    def indent(self,s):
        lines = filter(lambda x: x.strip(), s.split('\n'))
        if not lines: return ''
        #sys.stderr.write(str(lines))
        return '\n'.join( ['  '+line for line in lines] )+'\n'
    
    def FILE(self,kind,name,file,line,follow,*args):
        # The first script line is put at the
        # head of all scripts
        scripts = self.find_scripts(name,file,line,args)
        if scripts:
            self.test_prelude = scripts[0]
        else:
            self.test_prelude = ''
        self.test_count = 0

        self.OTHER(kind,name,file,line,follow,*args)
        return

    def OTHER(self,kind,name,file,line,follow,*args):
        scripts = self.find_scripts(name,file,line,args)
        basename = os.path.splitext(os.path.basename(file))[0]
        for script in scripts:
            # Get a unique name for this micro-test
            key = '%s_%03d'%(basename,self.test_count)
            self.test_count += 1

            # Remove the ,... from text like [1,2,3,...]
            # which is used only to clarify that many
            # items can be specified
            script = script.replace(',...','')
            script = script.replace('...,','')
            script = script.replace(', ...','')
            script = script.replace('..., ','')

            try:
                compiler.parse(script)
            except parser.ParserError:
                raise SyntaxError,'\n'+script

            harness = '#!/usr/bin/env pyMPI\n'

            prelude = (
                'try:\n'+
                '  __FILE__ = "??"\n'+
                '  __LINE__ = 0\n'+
                '  import os\n'+
                '  import sys\n'+
                '  import traceback\n'+
                '  print\n'+
                '  print %r\n'%key+
                '  print\n'+
                '  __ALL_SOURCE__ = %r\n'%(self.test_prelude+script)+
                '  __SOURCE__ = %r\n'%script
                )

            epilogue = (
                'except AssertionError,msg:\n'+
                '  print "Did not test %s because not",msg\n'%key+
                '  try: cleanup()\n'+
                '  except: pass\n'+
                '  sys.exit(77)\n'+
                'except NotImplementedError,msg:\n'+
                '  print "TODO: %s",msg\n'%key+
                '  try: cleanup()\n'+
                '  except: pass\n'+
                '  sys.exit(77)\n'+
                'except:\n'+
                '  print\n'+
                ('  print "%%s:%%d: %%s failed: %%r"%%(__FILE__,__LINE__,%r,str(sys.exc_info()[1]))\n'%key)+
                '  print __SOURCE__\n'+
                '  print traceback.format_exception_only(*sys.exc_info()[:2])[0]\n'+
                '  error_line = traceback.extract_tb(sys.exc_info()[2])[-1][1]\n'+
                '  print __ALL_SOURCE__.split("\\n")[error_line-%d]\n'%len(prelude.split('\n'))+
                '  try: cleanup()\n'+
                '  except: pass\n'+
                '  os._exit(1)\n'+
                'try: cleanup()\n'+
                'except: pass\n'
                )

            micro = (
                harness +
                prelude +
                self.indent(self.test_prelude) +
                self.indent(script) +
                epilogue
                )
            self.micro_tests[key] = micro
        return

    def grind(self):
        try:
            os.mkdir('micro_tests')
        except:
            pass
        for file in self.micro_tests:
            path = os.path.join('micro_tests',file)
            try:
                os.unlink(path)
            except:
                pass
            open(path,'w').write(self.micro_tests[file])
            os.chmod(path,0505)
        return
    

##################################################################
#                         CLASS AIX_EXP                          #
##################################################################
class AIX_exp(DocBox):
    def __init__(self,*args,**kw):
        self.globals = []
        DocBox.__init__(self,*args,**kw)
        return
                 
    def OTHER(self,kind,name,file,line,follow,*args):
        return
    
    def GLOBAL(self,kind,name,file,line,follow,doc=None):
        self.globals.append(name)
        return

    def grind(self):
        from distutils.sysconfig import expand_makefile_vars, get_config_vars

        python_exp = expand_makefile_vars(
            '$(BINLIBDEST)/config/python.exp',
            get_config_vars()
            )

        try:
            exported_from_python = open(python_exp).read()
        except:
            exported_from_python = ''

        pympi_exp = open('pyMPI.exp','w')
        pympi_exp.write(exported_from_python)
        pympi_exp.write('\n')
        exported = exported_from_python.split('\n')
        for symbol in self.globals:
            if symbol not in exported:
                pympi_exp.write(symbol)
                pympi_exp.write('\n')
                exported.append(symbol)
        return

actors = [
    CheckTabs(),
    CheckTODO(),
    CheckSpaceStar(),
    CheckOpenComment(),
    FindExternals(),
    MicroTests(),
    AIX_exp(),
    ]

if __name__ == '__main__':
    width = 60
    print '/*%s*/'%('*'*width)
    print '/*%s*/'%('pyMPI_Externals.h'.center(width))
    print '/*%s*/'%('Machine generated... Do not edit'.center(width))
    print '/*%s*/'%('*'*width)
    print
    print '#ifndef PYMPI_EXTERNALS_H'
    print '#define PYMPI_EXTERNALS_H'
    print 
    print '#ifdef __cplusplus'
    print '  extern "C" {'
    print '#endif'
    print
    print '#ifndef __THROW'
    print '#define __THROW'
    print '#endif'
    print


    directory = sys.argv[1]
    files = sys.argv[2:]
    for file in files:
        source = open(os.path.join(directory,file)).read()
        for actor in actors:
            actor.check(source,file)

    for actor in actors:
        actor.grind()

    print '#ifdef __cplusplus'
    print '  }'
    print '#endif'
    print
    print '#endif'

    
