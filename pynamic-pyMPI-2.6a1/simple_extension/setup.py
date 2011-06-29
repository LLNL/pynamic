from distutils.core import setup,Extension
try:
    from distutils.sysconfig import mode
    mode('parallel')
    del mode
except:
    pass

setup(name='simple',
      ext_modules=[Extension('simple',['simple.c']),
                  ],
      )

      
