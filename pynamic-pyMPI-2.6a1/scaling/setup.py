from distutils.core import setup, Extension

setup(ext_modules=[
    Extension('pingpong',['pingpong.c']),
    ],
      )

