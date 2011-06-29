#**************************************************************************#
#* FILE   **************         reduces.py        ************************#
#**************************************************************************#
#* Author: Patrick Miller February 14 2002                                *#
#**************************************************************************#
#* Test interface and expected behavior of mpi.reduce and allreduce       *#
#**************************************************************************#

import mpi
mpi.synchronizeQueuedOutput("/dev/null","/dev/null")
import unittest
import sys

##################################################################
#                          CLASS REDUCE                          #
##################################################################
class reduce(unittest.TestCase):
    reducer = mpi.reduce

    ##################################################################
    #                       MEMBER COMPAREROOT                       #
    # Data comparison on root process                                #
    ##################################################################
    def compareRoot(self,label,v,expected):
        self.failUnless(type(v) == type(expected),'%s] Bad result type %s on %d, expected %s'%(
            label,type(v),mpi.rank,type(expected)))
        self.failUnless(v==expected,'%s] Bad value %r on %d (expected %r)'%(
            label,v,mpi.rank,expected))
        return


    ##################################################################
    #                      MEMBER COMPARESLAVE                       #
    # Data comparison for slaves (None normally)                     #
    ##################################################################
    def compareSlave(self,label,v,expected):
        self.failUnless(v==None,'%s] Invalid rebroadcast on %d'%(
            label,mpi.rank))
        return
    

    ##################################################################
    #                         MEMBER COMPARE                         #
    # Compare root or slave as indicated by process rank             #
    ##################################################################
    def compare(self,label,v,expected,root=0):
        if mpi.rank == root:
            self.compareRoot(label,v,expected)
        else:
            self.compareSlave(label,v,expected)
        return
    

    ##################################################################
    #                         MEMBER TRIALS                          #
    # Test a bunch of label, value, operation, against expected      #
    # result.                                                        #
    ##################################################################
    def trials(self,*tests):
        for label,v,op,expected in tests:
            result = self.reducer(v,op)
        self.compare(label,result, expected)
        return


    ##################################################################
    #                     MEMBER TESTNATIVEFLOAT                     #
    # Test native reduce against loop reduced value                  #
    ##################################################################
    def testNativeFloat(self):
        SUM = 0.0
        PROD = 1.0
        MIN = 9999999.0
        MAX = -9999999.0
        MINLOC = (MIN,-1)
        MAXLOC = (MAX,99999)
        for rank in range(mpi.procs):
            x = float(rank+1)
            SUM += x
            PROD *= x
            MAX = max(MAX,x)
            MIN = min(MIN,x)
            if x < MINLOC[0]:
                MINLOC = (x,rank)
            if x > MAXLOC[0]:
                MAXLOC = (x,rank)

        x = float(mpi.rank+1)
        self.trials(
            ["float max", x, mpi.MAX,  MAX],
            ["float min", x, mpi.MIN,  MIN],
            ["float sum", x, mpi.SUM,  SUM],
            ["float prod",x, mpi.PROD, PROD],
            # LAND,
            # BAND,
            # LOR,
            # BOR,
            # LXOR,
            # BXOR,
            ["float minloc",x, mpi.MINLOC, MINLOC],
            ["float maxloc",x, mpi.MAXLOC, MAXLOC],

            )
        return


    ##################################################################
    #                     MEMBER TESTPYTHONFLOAT                     #
    # Test that the native reduce and the ones invoking the hand-    #
    # rolled python implementations work the same                    #
    ##################################################################
    def testPythonFloat(self):
        MIN = 9999999.0
        MAX = -9999999.0
        SUM = 0.0
        PROD = 1.0
        MINLOC = (MIN,-1)
        MAXLOC = (MAX,99999)
        x = float(mpi.rank+1)
        for label,operation,ground in [
            ('float ground min',mpi.MIN, MIN),
            ('float ground sum',mpi.MAX, MAX),
            ('float ground sum',mpi.SUM, SUM),
            ('float ground sum',mpi.PROD, PROD),
            # LAND,
            # BAND,
            # LOR,
            # BOR,
            # LXOR,
            # BXOR,
            ('float ground minloc',mpi.MINLOC, MINLOC),
            ('float ground maxloc',mpi.MAXLOC, MAXLOC),
            ]:
            self.failUnless( self.reducer(x,operation) == self.reducer(x,operation,ground=ground),
                '%s] failure on rank %d'%(label,mpi.rank))
        return


    ##################################################################
    #                      MEMBER TESTNATIVEINT                      #
    # Test native reduce against loop reduced value                  #
    ##################################################################
    def testNativeInt(self):
        MIN = 99999
        MAX = -99999
        SUM = 0
        PROD = 1
        LAND = 1
        BAND = -1
        LOR = 0
        BOR = 0
        LXOR = 0
        BXOR = 0
        MINLOC = (MIN,-1)
        MAXLOC = (MAX,99999)
        for rank in range(mpi.procs):
            x = rank+1
            MAX = max(MAX,x)
            MIN = min(MIN,x)
            SUM += x
            PROD *= x
            if LAND and x:
                LAND = 1
            else:
                LAND = 0
            BAND &= x
            if LOR or x:
                LOR = 1
            else:
                LOR = 0
            BOR |= x
            if x:
                LXOR = not LXOR
            else:
                LXOR = LXOR
            BXOR ^= x
            if x < MINLOC[0]:
                MINLOC = (x,rank)
            if x > MAXLOC[0]:
                MAXLOC = (x,rank)
            

        x = mpi.rank+1
        self.trials(
           ["int max", x, mpi.MAX,  MAX],
           ["int min", x, mpi.MIN,  MIN],
           ["int sum", x, mpi.SUM,  SUM],
           ["int prod",x, mpi.PROD, PROD],
           ["int land",x, mpi.LAND, LAND],
           ["int band",x, mpi.BAND, BAND],
           ["int lor",x,  mpi.LOR,  LOR],
           ["int bor",x,  mpi.BOR,  BOR],
           ["int lxor",x, mpi.LXOR, LXOR],
           ["int minloc",x, mpi.MINLOC, MINLOC],
           ["int maxloc",x, mpi.MAXLOC, MAXLOC],
           )
        return


    ##################################################################
    #                      MEMBER TESTPYTHONINT                      #
    # Test that the native reduce and the ones invoking the hand-    #
    # rolled python implementations work the same                    #
    ##################################################################
    def testPythonInt(self):
        MIN = 99999
        MAX = -99999
        SUM = 0
        PROD = 1
        LAND = 1
        BAND = -1
        LOR = 0
        BOR = 0
        LXOR = 0
        BXOR = 0
        MINLOC = (MIN,-1)
        MAXLOC = (MAX,99999)
        x = mpi.rank+1
        for label,operation,ground in [
            ('int ground min',mpi.MIN, MIN),
            ('int ground sum',mpi.MAX, MAX),
            ('int ground sum',mpi.SUM, SUM),
            ('int ground sum',mpi.PROD, PROD),
            ('int ground land',mpi.LAND, LAND),
            ('int ground band',mpi.BAND, BAND),
            ('int ground lor',mpi.LOR, LOR),
            ('int ground bor',mpi.BOR, BOR),
            ('int ground lxor',mpi.LXOR, LXOR),
            ('int ground bxor',mpi.BXOR, BXOR),
            ('int ground minloc',mpi.MINLOC, MINLOC),
            ('int ground maxloc',mpi.MAXLOC, MAXLOC),
            ]:
            self.failUnless(
                self.reducer(x,operation) == self.reducer(x,operation,ground=ground),
                '%s] failure on rank %d'%(label,mpi.rank))
        return
    

    ##################################################################
    #                       MEMBER TESTSTRING                        #
    # mpi.SUM turns into Python string catenation!                   #
    ##################################################################
    def testString(self):
        expected = ''
        for i in range(mpi.procs):
            expected += 'rank %d'%i
        s = 'rank %d'%mpi.rank
        v = self.reducer(s,mpi.SUM)
        self.compare('string catenation',v,expected)
        return


    ##################################################################
    #                    MEMBER TESTUSERFUNCTION                     #
    # operator can now be a user defined function of two variables   #
    ##################################################################
    def testUserFunction(self):
        def f(a,b): return a+b+2
        expected = mpi.procs*(mpi.procs-1)/2 + 2*(mpi.procs-1)
        v = self.reducer(mpi.rank,f)
        self.compare('simple user function',v,expected)
        return


    ##################################################################
    #                         MEMBER MEMORY                          #
    # Try to approximate the amount of memory being used             #
    ##################################################################
    def memory(self):
        try:
            import os
            import string
            memstring = os.popen('ps -p %s -o vsz=""'%os.getpid()).read().strip()
            m = string.atof(memstring[:-1])
            if memstring[-1] == 'M':
                m *= 1e6
            elif memstring[-1] == 'K':
                m *= 1e3
        except:
            m = 0.0
        return m


    ##################################################################
    #                     MEMBER TESTMEMORYLEAK                      #
    # What prompted this whole business is reduce's memory leak.     #
    # This test fails under old pyMPI and succeeds now               #
    ##################################################################
    #
    # This test is giving me some problems so I'm going to comment it out for
    # now
    #
    #def testMemoryLeak(self):
    #    # -----------------------------------------------
    #    # Do a single one just to get any buffers or
    #    # whatever inplace
    #    # -----------------------------------------------
    #    dt = 0.01
    #    ignore = self.reducer(dt,mpi.MIN)

    #    # -----------------------------------------------
    #    # Get a guess at the memory being used
    #    # and simulate a long run
    #    # -----------------------------------------------
    #    start = self.memory()
    #    for i in xrange(25000):
    #        ignore = self.reducer(dt,mpi.MIN)
    #    end = self.memory()

    #    # -----------------------------------------------
    #    # If we have grown much, something is wrong
    #    # -----------------------------------------------
    #    self.failUnless(
    #        (end-start)/(start+1e-6) < 0.10,
    #        'Memory grew less that 2 percent (grew from %d to %d)'%(start,end))
    #    return



##################################################################
#                        CLASS ALLREDUCE                         #
# Allreduce should test everything that reduce does (except that #
# slaves should have the value instead of None).                 #
##################################################################
class allreduce(reduce):
    reducer = mpi.allreduce
    compareSlave = reduce.compareRoot


if __name__ == '__main__':
    try:
        unittest.main()
    except:
        pass
    mpi.barrier() # Wait for everyone!
