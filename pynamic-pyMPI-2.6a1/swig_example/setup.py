from distutils.core import setup,Extension
try:
    from distutils.sysconfig import mode
    mode('parallel')
    del mode
except:
    pass

setup(name='swigged',
      ext_modules=[Extension('example',['example.i','examplecode.c']),
                  ],
      )

      
