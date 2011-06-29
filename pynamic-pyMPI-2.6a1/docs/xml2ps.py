import sys
import os
from xml.sax import make_parser
from xml.sax.handler import ContentHandler

class DocHandler(ContentHandler):
    def __init__(self):
        self.includes = []
        self.stack = ['']
        self.attrs = []
        return

    def include(self,name):
        if name not in self.includes:
            self.includes.append(name)
        return

    def lout(self):
        s = ''
        for include in self.includes:
            s += '@SysInclude{%s}\n'%include
        s += self.stack[-1]
        return s

    def __default(self,name,attrs,body):
        return body
    
    def report(self,name,attrs,body):
        self.include('report')
        s = '@Report\n'
        s += '  @Title{%s}\n'%attrs.get('title','')
        s += '  @Author{%s}\n'%attrs.get('author','')
        s += '  @Institution{%s}\n'%attrs.get('institution','LLNL')
        s += '//\n'
        return s+str(body)

    def section(self,name,attrs,body):
        title = attrs.get('title','')
        return '@Section\n  @Title{%s}\n@Begin\n%s\n@End @Section\n'%(title,body)

    def bold(self,name,attrs,body):
        return '@B{ %s }'%body

    def ital(self,name,attrs,body):
        return '@I{ %s }'%body

    def p(self,name,attrs,body):
        return '@PP\n'
    
    def c(self,name,attrs,body):
        self.include('cprint')
        style = attrs.get('style','varying')
        return '@IndentedDisplay @CP style{%s}{%s}'%(style,body.replace('"/"','/'))

    def python(self,name,attrs,body):
        return self.c(name,attrs,body)

    def verbatim(self,name,attrs,body):
        return '@Verbatim @Begin\n%s@End @Verbatim\n'%body
    
    def startElement(self,name,attrs):
        self.stack.append('')
        self.attrs.append(attrs)
        return

    def characters(self,ch):
        self.stack[-1] += str(ch).replace('/','"/"')
        # / | & { } # @ ^ ~ \ "
        return

    def endElement(self,name):
        body = self.stack[-1]
        del self.stack[-1]
        attrs = self.attrs[-1]
        del self.attrs[-1]
        try:
            method = getattr(self,name)
        except:
            method = self.__default
        s = method(name,attrs,body)
        self.stack[-1] += s
        return

if __name__ == '__main__':
    source = sys.argv[1]
    base,ext = os.path.splitext(source)
    lout = base + '.lout'
    ps = base + '.ps'
    parser = make_parser()
    handler = DocHandler()
    parser.setContentHandler(handler)

    file = open(source)
    xml = file.readline()
    assert xml.startswith('<?xml')
    parser.parse(file)

    open(lout,'w').write( handler.lout() )
    pipe = os.popen('lout %s'%lout)
    pipe.read()
    pipe.close()
    
    # Get cross references
    pipe = os.popen('lout %s'%lout) 
    open(ps,'w').write( pipe.read() )
    


