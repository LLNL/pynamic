from distutils.core import setup,Extension

setup(name='advanced',
      ext_modules=[Extension('advanced',['advanced.c']),
                  ],
      )

      
