#**************************************************************************#
#* FILE   **************          popen.py         ************************#
#**************************************************************************#
#* Author: Patrick Miller October 30 2002                                 *#
#**************************************************************************#
"""
Check if issues with pipe open/read/close are under control.
"""
#**************************************************************************#

import unittest
import os
import mpi

class PipeTest(unittest.TestCase):
    def testPipe(self):
        try:
            pipe = os.popen('ls')
            s = pipe.read()
            status = pipe.close()
        finally:
            mpi.barrier()
        return

if __name__ == '__main__':
    try:
        try:
            unittest.main()
        except:
            pass
    finally:
        mpi.barrier() # Wait for everyone!
