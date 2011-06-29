#**************************************************************************#
#* FILE   **************       scatters.py         ************************#
#**************************************************************************#
#* Author: Chris Murray  Martin Casado Tue Feb 19 18:15:03 PST 2002        # 
#**************************************************************************#
#* Test interface and expected behavior of mpi.gather/mpi.allgather        # 
#**************************************************************************#

import mpi
mpi.synchronizeQueuedOutput("/dev/null","/dev/null")
import unittest

#====================================================================
# Scatter
#====================================================================
class scatters(unittest.TestCase):
    def testScatter(self):

        #decide on roots
        roots = [0,0,0]
        roots = [int(mpi.procs / 3), 0, int(mpi.procs / 2) ]
        nprocs = mpi.procs
        
        #values to be scattered
        list1 = []
        list2 = []
        longList = []
        tuple1 = ()
        string1 = ""
        for x in range(nprocs):
            list1 += [nprocs - x]
            list2 += [ "fOo", x, ( 8,str(x*x) )]
            tuple1 += (2*x, 2*x + 1)
            string1 += "fOo for" + str(x%10)
            for i in range(1024):
                longList += [ 500-4*i - 10*x, "K" ]
        
        #do scatters
        results = [0,0,0,   0,0,0,      0,0,0]
        results[0] = mpi.scatter( list1,  roots[0])
        results[1] = mpi.scatter( list2,  roots[0] )
        results[2] = mpi.scatter( tuple1, roots[0] )
        results[3] = mpi.scatter( string1,roots[0])
        results[4] = mpi.scatter( longList, roots[0] )

        #correct answers
        correctAnswers = [0,0,0,    0,0,0,    0,0,0]
        for x in range(5):
            correctAnswers[x] = []

        correctAnswers[0] = [nprocs - mpi.rank]
        correctAnswers[1] = list2[ mpi.rank*3: mpi.rank*3 + 3]
        correctAnswers[2] = (2*mpi.rank, 2*mpi.rank+1)
        correctAnswers[3] = "fOo for"+str(mpi.rank%10)
        correctAnswers[4] = longList[2048*mpi.rank: 2048*(mpi.rank+1)]
        
        for x in range(5):
            if results[x] != correctAnswers[x]:
                failString = "scatter failed on test "+str(x)
                failString += " and process " + str(mpi.rank) + "\n"
                failString += "Scatter result was " + str(results[x])
                failString += " and it should be "+ str(correctAnswers[x])
                self.fail( failString);

#====================================================================
# main
#====================================================================

if __name__ == '__main__':
    try:
        unittest.main()
    except:
        pass
    mpi.barrier() # Wait for everyone!
