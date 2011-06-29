#**************************************************************************#
#* FILE   **************        gathers.py         ************************#
#**************************************************************************#
#* Author: Chris Murray  Martin Casado Tue Feb 19 18:15:03 PST 2002        # 
#**************************************************************************#
#* Test interface and expected behavior of mpi.gather/mpi.allgather        # 
#**************************************************************************#

import mpi
mpi.synchronizeQueuedOutput("/dev/null","/dev/null")
import unittest

#====================================================================
# test out gather()/allgather
#====================================================================
class gathers(unittest.TestCase):
    def testGather(self):

        #decide on targets
        tgts = [0,0,0]
        #tgts = [int(mpi.procs / 3), 0, int(mpi.procs / 2) ]
        
        #values to be gathered
        list1 = [ mpi.rank + 1 ]
        list2 = [0, mpi.rank, mpi.rank*mpi.rank, mpi.rank + 20]
        string1 = "S" + str(mpi.rank % 4) + "FOO"
        tuple1 = ("t")
        tuple2 = ( "fOo", [1,mpi.rank, 3], (0,1) )

        list3 = [] #Test gather where each process passes in a different count
        for x in range(mpi.rank+2):
          list3 += [x]
        longList = range(256)

        #do gathers
        results = [0,0,0, 0,0,0,     0,0]
        results[0] = mpi.gather( list1,   1,  tgts[0])
        results[1] = mpi.gather( list2,   3,  tgts[1])
        results[2] = mpi.gather( string1, 3,  tgts[2])
        results[3] = mpi.gather( tuple1,  1,  tgts[0])
        results[4] = mpi.gather( tuple2,  1,  tgts[1])
        results[5] = mpi.gather( tuple2,  3,  tgts[2])
        results[6] = mpi.gather( list3, mpi.rank+2, tgts[0] )
        results[7] = mpi.gather( longList, 256, tgts[0] )

        #correct answers
        correctAnswers = [0,0,0,      0,0,0,      0,0]
        for x in range(8):
            correctAnswers[x] = []

        for x in range(mpi.procs):
            correctAnswers[0] += [x + 1]
            correctAnswers[1] += [0, x, x*x]
            correctAnswers[2] += ["S", str(x%4), "F"]
            correctAnswers[3] += ["t"]
            correctAnswers[4] += [ "fOo" ]
            correctAnswers[5] += [ "fOo", [1, x, 3], (0,1) ]

            for i in range(x+2):
              correctAnswers[6] += [i]

            correctAnswers[7] = range(256)*mpi.procs
       
        for x in range(8):
            if mpi.rank == tgts[x%3] and results[x] != correctAnswers[x]:
                self.fail(" gather failed on test "+str(x))

            elif mpi.rank != tgts[x%3] and results[x] != None:
                errstr = "gather failed on off-target";
                errstr += "process on test " + str(x)
                self.fail( errstr)

    def testAllGather(self):
            
        #values to be allGathered
        list1 = [ mpi.rank + 1 ]
        list2 = [0, mpi.rank, mpi.rank*mpi.rank, mpi.rank + 20]
        string1 = "S" + str(mpi.rank % 4) + "FOO"
        tuple1 = ["t"]
        tuple2 = [ "fOo", [1,mpi.rank, 3], (0,1) ]
        item1 = str(mpi.rank%10) + "c"
        if mpi.rank%3 == 1:
            item1 = [ str(mpi.rank%10), "c" ]
        if mpi.rank%3 == 2:
            item1 = ( str(mpi.rank%10), "c" )

        #do gathers
        results = [0,0,0,      0,0,0,       0]
        results[0] = mpi.allgather( list1,   1)
        results[1] = mpi.allgather( list2,   3)
        results[2] = mpi.allgather( string1, 3)
        results[3] = mpi.allgather( tuple1,  1)
        results[4] = mpi.allgather( tuple2,  1)
        results[5] = mpi.allgather( tuple2,  3)
        results[6] = mpi.allgather( item1, 2 )

        #correct answers
        correctAnswers = [0,0,0,    0,0,0,    0]
        for x in range(7):
            correctAnswers[x] = []

        for x in range(mpi.procs):
            correctAnswers[0] += [x + 1]
            correctAnswers[1] += [0, x, x*x]
            correctAnswers[2] += ["S", str(x%4), "F"]
            correctAnswers[3] += ["t"]
            correctAnswers[4] += [ "fOo" ]
            correctAnswers[5] += [ "fOo", [1, x, 3], (0,1) ]
        
        for x in range(6):
            if results[x] != correctAnswers[x]:
                self.fail("AllGather failed on test "+str(x))

#====================================================================
# main
#====================================================================

if __name__ == '__main__':
    try:
        unittest.main()
    except:
        pass
    mpi.barrier() # Wait for everyone!
