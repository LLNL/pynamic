# File:   PyMPITest.py 
# Date:   Thu Jun 07 17:09:12 PDT 2001 
# Author: Martin Casado
# Last Modified: 2001-Jun-08 10:44:37
#
# Tests are setup as follows.  Only information from
# the process with rank 0 are displayed by the testing 
# system. Tests should no print out any information during
# testing.  The process with rank 0 should assert whether
# the test failed or succeded
#
# TODO: Replace standard unittest.TextTestRunner() with something
# more suitable for parallel tests

import sys

try:
    import mpi
except ImportError:
    print "Failed to import mpi"
    sys.exit(1)

try:
    import unittest
except ImportError:    
    print "You need to have PyUnit installed to run pyMPI tests"
    sys.exit(1)

#====================================================================
# Base class for pyMPI tests
#====================================================================

class PyMPIBaseTestCase(unittest.TestCase):
    def __message(self):
        e,msg,traceback = sys.exc_info()
        if e is self.failureException:
            exception_name = 'Failure'
        else:
            exception_name = e.__name__

        traceback = traceback.tb_next
        filename = traceback.tb_frame.f_code.co_filename
        function = traceback.tb_frame.f_code.co_name
        lineno = traceback.tb_lineno
        
        return '\n  File "%s", line %d, in %s [rank %d]\n    %s: %s'%(
            filename,
            lineno,
            function,
            mpi.rank,
            exception_name,
            msg)
    
    def runTest(self):
        mpi.barrier()

        name = self.__class__.__name__
        if name[:5] == 'PyMPI': name = name[5:]
        if name[-8:] == 'TestCase': name = name[:-8]
        mpi.trace(name+'<')

        try:
            try:
                self.parallelRunTest()
                any_errors = mpi.gather([])
            except:
                any_errors = mpi.gather([self.__message()])

            if mpi.rank == 0 and any_errors:
                import string
                raise self.failureException,string.join(any_errors,'')
        finally:
            mpi.barrier()
            mpi.traceln('>')

        return

#====================================================================
# Extremely simple test case with a one way
# send between two processes
#====================================================================
class PyMPISimpleSendTestCase(PyMPIBaseTestCase):
    def ping(self,msg):
        if mpi.procs < 2: return
        try:
            if mpi.rank == 0:
                received,status = mpi.recv(1)
                self.failUnless(received==msg,'Bad message received')
            elif mpi.rank == 1:
                mpi.send(msg,0)
        finally:
            mpi.barrier()

    def parallelRunTest(self):
        MSGSIZE = 10
        longTestString = ""
        for i in range(MSGSIZE):
           longTestString = [i, str(i), longTestString]
        self.ping('hello world')
        self.ping(longTestString)
        return
    
#====================================================================
# test out sendrecv(), can be run with arbitrary number of processes 
# Originally written by Pat Miller
# Modified by Martin Casado
#====================================================================
class PyMPISimpleSendRecvTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        if mpi.procs%2 == 1:
            self.fail("Simple sendrecv must be run with an even number of processes")

        if mpi.procs > 1:
            # Swap ranks even/odd, evens send first
            if mpi.rank % 2 == 0:
                mpi.send(mpi.rank,mpi.rank+1)
                nextRank,stat = mpi.recv(mpi.rank+1)
                if nextRank != mpi.rank+1:
                    self.fail("Received incorrect rank")
            else:
                prevRank,stat = mpi.recv(mpi.rank-1)
                mpi.send(mpi.rank,mpi.rank-1)
                if prevRank != mpi.rank-1:
                    self.fail("Received incorrect rank.  Expected %r, got %r"%(
                        mpi.rank-1,prevRank
                        ))

        # Try an around the horn sendrecv check
        me = mpi.rank
        rightside = (me+1)%mpi.procs
        leftside = (me-1+mpi.procs)%mpi.procs
        msg,stat2 = mpi.sendrecv(me,rightside,leftside)

        if msg != leftside:
            failStr = "Failed simple sendrecv "
            failStr += "Process " + str(mpi.rank) + " received "
            failStr += str(msg) + " instead of " + str(leftside)
            self.fail(failStr)

        #Do another check with a longer message
        longMsg = ()
        for i in range(256):
          longMsg += (i, str(i) )

        msg, stat2 = mpi.sendrecv( longMsg, leftside, rightside )
        if msg != longMsg:
          failStr = "Failed simple sendrecv for long messages"
          self.fail( failStr )

        return
          

#====================================================================
# Test out barriers
# This test should be tested in an environment that can detect
# deadlocks, otherwise it is pretty worthless
#====================================================================
class PyMPIBarrierTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        mpi.barrier()
        mpi.barrier()
        mpi.barrier()
        mpi.barrier()
        return

#====================================================================
# Broadcast test
# adapted from Pat Miller's bcast-test.py
#====================================================================
class PyMPIBroadcastTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        if mpi.rank == 0:
            st = "Hello World!!" 
            recvSt = mpi.bcast(st)
            if recvSt != "Hello World!!":
                self.fail("bcast test failed on short string")

            longStr = ""
            for i in range(2048):
              longStr += "Foo"
              longStr += str(i)

            recvSt = mpi.bcast(longStr)
            if recvSt != longStr:
              self.fail( "bcast test failed on long string")
        else:
            recvSt = mpi.bcast()
            if recvSt != "Hello World!!":
                self.fail("Received incorrect string in broadcast")

            longStr = ""
            for i in range(2048):
              longStr += "Foo"
              longStr += str(i)

            recvSt = mpi.bcast( )
            if recvSt != longStr:
                self.fail( "bcast test failed on long string")

        return

#====================================================================
# Broadcast test 2
# Test pyMPI's broadcast for several types
#====================================================================
class PyMPIBroadcast2TestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        myList = [ 0,0,0,0,0,0 ]

        myList[0] = 42
        myList[1] = "Hello"
        myList[2] = ["Another list", 2]
        myList[3] = ("A tuple", 2)
        myList[4] = 4+5j
        myList[5] = 3.14159

        for x in range(6):
            mpi.barrier()
            z = mpi.bcast( myList[x],0 )
            if z != myList[x]:
                self.fail( "Broadcast test 2 failed on test " + str(x))

        return
            
#====================================================================
# Cartesian 
# Test pyMPI's cartesian communicator support
#====================================================================
class PyMPICartesianTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        cart = mpi.cart_create((1,int(mpi.procs)),(1,1),0)
        if mpi.rank == 0:
            try:
                cart.ndims  
                cart.dims  
                cart.periods  
                cart.coords()  
                cart.shift(1,1)
            except:
                self.fail("could not perform operations on cartesian communicator")

        return

#====================================================================
# ClacPi 
# adapted from Pat Miller's calcPi.py 
#====================================================================
class PyMPICalcPiTestCase(PyMPIBaseTestCase):
    def f(self,a):
       "The function to integrate"
       return 4.0/(1.0 + a*a)

    def integrate(self, rectangles, function):
        # equivalent to mpi.WORLD.bcast(n,0) or rather a
        # C call to MPI_Bcast(MPI_COMM_WORLD,n,0,&status)
        n = mpi.bcast(rectangles)

        h = 1.0/n
        sum = 0.0
        for i in range(mpi.rank+1,n+1,mpi.procs):
            x = h * (i-0.5)
            sum = sum + function(x)

        myAnswer = h * sum
        answer = mpi.allreduce(myAnswer,mpi.SUM)
        return answer

    def parallelRunTest(self):
        import math

        if mpi.rank == 0:
            n = 2000
        else:
            n = 0

        pi = self.integrate(n,self.f)
        if mpi.rank == 0:
            diff = abs(math.pi-pi)
            if diff > 1e-6: 
                self.fail("calculated pi off by more than 1e-16")
        return

#====================================================================
#====================================================================
class PyMPINullCommTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        uselessCommNumber = int(mpi.COMM_NULL)
        useless = mpi.communicator(uselessCommNumber)
        try:
            useless.barrier()
        except RuntimeError,msg:
            pass  # success
        else:    
            self.fail("Error not handled correctly")
        return

#====================================================================
#====================================================================
class PyMPIBadCommTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        if mpi.name != "LAM":
            try:
                badComm = mpi.communicator(1234)
            except RuntimeError,msg:
                pass
            else:
                self.fail("error not handled correctly")
        else:
            self.fail("LAM bombs on this BadComm check")

        return

#====================================================================
# PairedSendRecv
#====================================================================
class PyMPIPairedSendRecvTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        msg = "I am from proc %d"%mpi.rank
        if mpi.procs % 2 != 0:
            self.fail("Test needs even number of processes")
        if mpi.rank % 2 == 0:
           sendResult = mpi.send(msg,mpi.rank+1)
           recv,stat = mpi.recv()
        else:
           recv,stat = mpi.recv()
           sendResult = mpi.send(msg,mpi.rank-1)
        return

#====================================================================
# Pickled
# Adapted from pickled.py by Patrick Miller
#====================================================================
class something:
    def __init__(self,x):
        self.x = x

class PyMPIPickledTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        # -----------------------------------------------
        # See if we can broadcast a python object
        # -----------------------------------------------
        original = [1,2,3]
        if mpi.rank == 0:
            result = mpi.bcast(original)
        else:
            result = mpi.bcast(None)
        if original != result:
            self.fail('Bcast of pickled list fails')

        # -----------------------------------------------
        # See if we can broadcast an advanced python object
        # -----------------------------------------------
        if mpi.rank == 0:
            val = something(33)
        else:
            val = None
        received = mpi.bcast(val)
        if received.x != 33:
            self.fail('Bcast fails with advanced object')

        return

#====================================================================
# Reduce
# Adapted from reduce.py by Pat Miller
#====================================================================
class PyMPIReduceTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        v = 1+(mpi.rank)%2

        results = []
        for kind in ['MAX','MIN','SUM','PROD','LAND',
                     'LOR','LXOR',
                     'MINLOC','MAXLOC' ]:
            function = getattr(mpi,kind)
            try:
                r0 = mpi.allreduce(v,function)
            except RuntimeError,s:
                self.fail("All reduce") 

            try:
                r1 = mpi.reduce(v,function)
            except RuntimeError,s:
                self.fail("All reduce") 
            results.append((kind,function,r0,r1))

        return

#====================================================================
# SampleMIMD
# Adapted from sampleMIMD.py by Pat Miller
#====================================================================
class PyMPISampleMIMDTestCase(PyMPIBaseTestCase):
    # -----------------------------------------------
    # Split the machine and have half work on 4/(1+x^2)
    # and the other half work on sin(x)
    # -----------------------------------------------
    def f(self,a):
        return 4.0/(1.0+a*a)

    def integrate(self,comm, rectangles, function):
        n = comm.bcast(rectangles)

        h = 1.0/n
        sum = 0.0
        for i in range(comm.rank+1,n+1,comm.procs):
            x = h * (i-0.5)
            sum = sum + function(x)

        myAnswer = h * sum
        answer = comm.allreduce(myAnswer,mpi.SUM)
        return answer

    def parallelRunTest(self):
        import math
        if mpi.procs < 2:
            #Doing this serially as I only have one processor
            integralPi = self.integrate(mpi,10000,self.f)
            integralSin = self.integrate(mpi,10000,math.sin)
        else:
            # -----------------------------------------------
            # Split into two independent communicators
            # -----------------------------------------------
            split = mpi.procs/2
            first = mpi.comm_create(range(0,split))
            second = mpi.comm_create(range(split,mpi.procs))
            mpi.barrier()
            if first:            
                integralPi = self.integrate(first,10000,self.f)
            elif second:
                integralSin = self.integrate(second,10000,math.sin)
        return

#====================================================================
# SRNonblock
#====================================================================

class PyMPISRNonblockTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        hello = "Hello World!!"

        to = mpi.rank + 1
        if mpi.rank == mpi.procs -1: to = 0
        frm = mpi.rank - 1
        if mpi.rank == 0: frm = mpi.procs -1

        mpi.isend(hello,to)
        handle = mpi.irecv(frm)
        handle.wait()
        if handle.message != hello:
            self.fail("Received unexpected reply from:%d "%(frm))
        mpi.isend(hello,to)
        handle = mpi.irecv(frm)
        if handle.message != hello:
            self.fail("Received unexpected reply from:%d "%(frm))
        mpi.isend(hello,to)
        handle = mpi.irecv(frm)
        while handle.test[0] == 0:
            pass
        if handle.message != hello:
            self.fail("Received unexpected reply from:%d "%(frm))

        #Try and isend/irecv a long message to fullly test the new msg model
        longMsg = []
        for i in range(64):
            longMsg = ["foo", i, longMsg]

        mpi.isend( longMsg, to )
        handle = mpi.irecv(frm)
        handle.wait()
        if handle.message != longMsg:
            self.fail( "irecv failed on long message.")
        longMsg = longMsg.reverse()
        mpi.isend( longMsg, to )
        handle = mpi.irecv(frm)
        while handle.test[0] == 0:
            pass
        if handle.message != longMsg:
            self.fail( "irecv using wait failed on long message" )

        return

#===================================================================
# Self-Send Nonblock
#====================================================================

class PyMPISelfSendNonblockTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):

        #Every process sends six messages to itself
        myMsgs = ["I", "talk", "to", "myself", "next message is BIG", ""]

        #The last message is BIG to test the new message model
        for i in range(512):
            myMsgs[5] += str(i)

        #Do all the asynchronous sends: each process sends to ITSELF
        for x in range(6):
            mpi.isend( myMsgs[x], mpi.rank, x )  

        #Get receive handles for all the receives  
        recvHandles = [0,0,0,    0,0,0]
        for x in range(6):
            recvHandles[x] = mpi.irecv( mpi.rank, x )

        #Wait for all receives to complete
        mpi.waitall(recvHandles)

        #Check for correct answers
        for x in range(6):
            if recvHandles[x].message != myMsgs[x]:
                failStr = "Self-Selding non-blocking communication test fail"
                failStr += "\nFailure on process " + str(mpi.rank) + ", test "
                failStr += str(x)
                self.fail( failStr )

        return

#===================================================================
# ComplexNonblock
#====================================================================

class PyMPIComplexNonblockTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        if mpi.procs < 2:
            self.fail("This test needs at least 2 processes to run")

        mySmallData = "Hello from " + str(mpi.rank)
        myBigData = [0,"Foo", "goo"]
        for x in range(90):
          myBigData = [x+1,x*x, 12.4, ("c", "a"), myBigData]

        to = (mpi.rank + 1)%mpi.procs
        frm = (mpi.rank-1+mpi.procs)%mpi.procs

        #First we send asynchronously and receive synchronously
        sendHandle1 = mpi.isend( myBigData,   to, 0)
        sendHandle2 = mpi.isend( mySmallData, to, 1)
        msgReceived1, status = mpi.recv(frm,0)
        msgReceived2, status = mpi.recv(frm,1)

        #Check for failures
        if msgReceived1 != myBigData:
            self.fail("Complex NonBlock failed on first test with big data")
        if msgReceived2 != "Hello from " + str(frm):
            self.fail("Complex NonBlock failed on first test with small data")

        #Next we will do a blocking send and a non-blocking receive
        if mpi.rank==0:

          #Change the data we're sending just for the heck of it
          myBigData[0] = ("changed")
          myBigData[1] = "Also changed"
          mySmallData = ("Hi", mpi.rank)

          #Perform 2 blocking sends to send the data
          mpi.send( myBigData, 1, 1 )
          mpi.send( mySmallData, 1, 2 )

        elif mpi.rank==1:

          #Get recv handles for the two messages
          recvHandle1 = mpi.irecv( 0,1)
          recvHandle2 = mpi.irecv( 0,2)
          finished = [0,0]

          #Loop until both messages come in
          while finished[0] == 0 and finished[1] == 0:
            if finished[0] == 0:
              finished[0] = recvHandle1.test()
            if finished[1] == 0:
              finished[1] = recvHandle2.test()

          #We got the messages, now check them
          if recvHandle1.message != myBigData:
            self.fail( "Complex non-block failed on 2nd test with big data")
          if recvHandle2.message != ("Hi", 0):
            self.fail( "Complex non-block failed on 2nd test with small data")

        return

#====================================================================
# WaitAll
#====================================================================
class PyMPIWaitAllTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        if mpi.procs < 2:
            self.fail('This test needs at least 2 processes')

        if mpi.rank == 0:
            req1 = mpi.isend("hello",1,0)
            req2 = mpi.isend("world",1,1)

            req3 = mpi.isend(",",1,2)
            req4 = mpi.isend("this",1,3)
            req5 = mpi.isend("is",1,4)
            req6 = mpi.isend("your",1,5)
            req7 = mpi.isend("new",1,6)
            req8 = mpi.isend("master",1,7)

            try:
                mpi.waitall((req1))
            except:
                self.fail("waitall()")

        elif mpi.rank == 1:
            req1 = mpi.irecv(0,0)
            req2 = mpi.irecv(0,1)
            req3 = mpi.irecv(0,2)
            req4 = mpi.irecv(0,3)
            req5 = mpi.irecv(0,4)
            req6 = mpi.irecv(0,5)
            req7 = mpi.irecv(0,6)
            req8 = mpi.irecv(0,7)
            try:
                mpi.waitall((req1,req2,req3,req4,req5,req6,req7,req8))
            except:
                self.fail("waitall()")

        return

#====================================================================
# WaitAny
#====================================================================
class PyMPIWaitAnyTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        if mpi.procs < 2:
            self.fail('This test needs at least 2 processes')

        if mpi.rank == 0:
            req  = mpi.isend("hi there, bubba,",1,0)
            print 'send'
            req1 = mpi.isend("this is",1,1)
            req2 = mpi.isend("opportunity",1,2)
            req3 = mpi.isend("knocking....",1,3)
            try:
                mpi.waitany(req,req1,req2)
            except:
                self.fail("mpi.waitany() failed")
        elif mpi.rank == 1:
            req = []
            print 'recv0'
            req.append( mpi.irecv(0,0))
            print 'recv1'
            try:
                req.append( mpi.irecv(0,1))
            except:
                print 'bad?'
                print sys.exc_info()[1]
                raise
            print 'recv2'
            req.append( mpi.irecv(0,2))
            print 'recv3'
            req.append( mpi.irecv(0,3))
            print 'recv4'

            i = 0
            print 'while'
            while i < 4:
                print 'i is',i
                result = -1
                try:
                    print 'waitany?'
                    result = mpi.waitany(req)
                    print 'got',result
                except:
                    self.fail("mpi.waitany() failed")
                req.remove(req[result])
                i = i + 1

        return

#====================================================================
# ReduceSum
#====================================================================
class PyMPIReduceSumTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        target = int(mpi.procs / 2)
        #target = 0
        myStr = "la" + str(mpi.rank)
        myNum = mpi.rank
        myList = [ "la", myNum ]
        myTuple = ("FoO", "GOO")
        myLongStr = ""
        for i in range(512):
          myLongStr += str(i%10)

        results = [0,0,0,0,0]
        results[0] = mpi.reduce(myStr,mpi.SUM, target)
        results[1] = mpi.reduce(myNum,mpi.SUM, target)
        results[2] = mpi.reduce(myList,mpi.SUM, target)
        results[3]= mpi.reduce(myTuple,mpi.SUM, target)
        results[4]= mpi.reduce( myLongStr, mpi.SUM, target)

        #Set up correct answer for string reduce
        correctAnswers = [0,0,0,0,""]

        #Set up correct answer for list reduce
        correctAnswers[0] = ""
        correctAnswers[1] = (mpi.procs*(mpi.procs-1))/2
        correctAnswers[2] = []
        correctAnswers[3] = ("FoO", "GOO")*mpi.procs
        for x in range(mpi.procs):
            correctAnswers[0] += "la" + str(x)
            correctAnswers[2] += ["la", x]

        for x in range(mpi.procs):
            correctAnswers[4] += myLongStr

        for x in range(5):
            if mpi.rank == target:
                if correctAnswers[x] != results[x]:
                    self.fail( "Reduce SUM failed on test %s (\n%r is not \n%r)"%(x,results[x],correctAnswers[x]) )
            else:
                if results[x] != None:
                    failStr = "Reduce SUM failed on off-target proc on test "
                    failStr += str(i)
                    self.fail(failStr)
        return

#====================================================================
# reduceProduct
#====================================================================
class PyMPIReduceProductTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        #target = int(mpi.procs / 3)
        target = 0
        myNum1 = 2
        myNum2 = (mpi.rank % 2)+ 1
        myNum3 = mpi.rank * 2
        if mpi.rank == 0:
            obj1 = [1, "foo"]
            obj2 = "String"
        if mpi.rank != 0:
            obj1 = (mpi.rank % 2) + 1;
            obj2 = 1;

        answer1 = mpi.reduce( myNum1, mpi.PROD, target )
        answer2 = mpi.reduce( myNum2, mpi.PROD, target )
        answer3 = mpi.reduce( myNum3, mpi.PROD, target )
        answer4 = mpi.reduce( obj1,   mpi.PROD, target )
        answer5 = mpi.reduce( obj2,   mpi.PROD, target )

        #Generate correct answers
        correctAnswer1 = 1
        correctAnswer2 = 1
        correctAnswer4 = [1,"foo"]
        for x in range(mpi.procs):
            correctAnswer1 *= 2
            correctAnswer2 *= (x % 2) + 1
            if x > 0:
                correctAnswer4 *= (x% 2) + 1

        if mpi.rank == target:
            if answer1 != correctAnswer1:
                self.fail("reduce PROD failed on #'s 0")
            if answer2 != correctAnswer2:
                failString = "reduce PROD failed on #'s test 1\n"
                failString += "Reduce gave result " + str(answer2)
                failString += " and correct answer is " + str(correctAnswer2)
                self.fail(failString)
            if answer3 != 0:
                self.fail("reduce PROD failed on #'s 2")
            if answer4 != correctAnswer4:
                self.fail("reduce PROD failed on list and #'s")
            if answer5 != "String":
                self.fail("reduce PROD failed on string and #'s")

        return

#====================================================================
# ReduceLogical
#====================================================================
class PyMPIReduceLogicalTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        #decide on targets
        #targets = [ int(mpi.procs / 3), 0, mpi.procs - 1]

        targets = [ 0, 0, 0]
        #values to be reduced
        num1 = mpi.rank
        num2 = 0
        if mpi.rank == 1:
            num2 = 1
        num3 = 1
        num4 = (2 ** (mpi.rank%8))
        num5 = 8
        if mpi.rank == 1:
          num5 = 9
        list1 = [ mpi.rank + 1 ]
        list2 = [ mpi.rank]

        #do reduces
        results = [0,0,0,    0,0,0,    0,0,0,    0,0,0,    0,0,0 ]
        results[0] = mpi.reduce( num1,   mpi.LOR,  targets[0])
        results[1] = mpi.reduce( num1,   mpi.LAND, targets[1])
        results[2] = mpi.reduce( num2, mpi.LOR,  targets[2])
        results[3] = mpi.reduce( num3,  mpi.LAND,  targets[0])
        results[4] = mpi.reduce( num3,  mpi.LXOR,  targets[1])
        results[5] = mpi.reduce( list1,  mpi.LOR,  targets[2])
        results[6] = mpi.reduce( list1,  mpi.LAND,  targets[0])
        results[7] = mpi.reduce( list2,  mpi.LAND,  targets[1])
        results[8] = mpi.reduce( num1, mpi.MIN, targets[2] )
        results[9] = mpi.reduce( num2, mpi.MAX, targets[0] )
        results[10] = mpi.reduce( list1, mpi.MIN, targets[1] )
        results[11] = mpi.reduce( num1, mpi.MINLOC, targets[2])
        results[12] = mpi.reduce( list1, mpi.MAXLOC, targets[0] )
        results[13] = mpi.reduce( num4, mpi.BAND, targets[1] )
        results[14] = mpi.reduce( num5, mpi.BXOR, targets[2] )

        #correct answers
        correctAnswers = [  1,0,1,    
                            1,mpi.procs%2,1,     
                            1,1,0,
                            1,[1],(0,0),
                            ( mpi.procs - 1, [mpi.procs] ), 0, 
                            (mpi.procs%2)*8 + 1]

        for x in range(15):
            if mpi.rank == targets[x%3] and results[x] != correctAnswers[x]:
                failString = "reduce logical failed on test " + str(x)
                failString += "\nReduce gave result " + str(results[x])
                failString += " while correct answer is "
                failString += str(correctAnswers[x]) + "\n"
                self.fail(failString)

            elif mpi.rank != targets[x%3] and results[x] != None:
                errstr = "reduce LOGICAL failed on off-target";
                errstr += "process on test " + str(x)
                self.fail( errstr)
        return

#====================================================================
# AllReduce
#====================================================================
class PyMPIAllReduceTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        #decide on targets
        #values to be reduced
        num1 = mpi.rank
        num2 = 0
        if mpi.rank == 1:
            num2 = 1
        num3 = 1
        string1 = "FoO"
        list1 = [ mpi.rank + 1 ]
        list2 = [ mpi.rank]
        longList = []
        for i in range(2048):
            longList += [ 1 - mpi.rank*mpi.rank, "oOf"]

        #do all reduces
        results = [0,0,0,    0,0,0  ]
        results[0] = mpi.allreduce( num2,   mpi.LOR)
        results[1] = mpi.allreduce( list1,   mpi.LAND)
        results[2] = mpi.allreduce( list2, mpi.SUM)
        results[3] = mpi.allreduce( string1,  mpi.SUM)
        results[4] = mpi.allreduce( list1,  mpi.MAXLOC)
        results[5] = mpi.allreduce( longList,  mpi.MAXLOC)

        #correct answers
        correctAnswers = [ 0,0,[],"",0, () ]
        correctList = []
        correctAnswers[0] = 1
        correctAnswers[1] = 1
        correctAnswers[3] = "FoO"*mpi.procs
        correctAnswers[4] = (mpi.procs-1, [mpi.procs])
        for x in range(mpi.procs):
            correctAnswers[2] += [x]
        for x in range(2048):
            correctList += [1, "oOf"]
        correctAnswers[5] = (0, correctList)

        for x in range(6):
            if results[x] != correctAnswers[x]:
                failString = "All Reduce failed on test " + str(x)
                failString += "\nAllReduce gave result " + str(results[x])
                failString += " while correct answer is "
                failString += str(correctAnswers[x]) + "\n"
                self.fail(failString)
        return

#====================================================================
# Gather
#====================================================================
class PyMPIGatherTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
        #decide on targets
        targets = [0,0,0]
        #targets = [int(mpi.procs / 3), 0, int(mpi.procs / 2) ]

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

        results[0] = mpi.gather( list1,   1,  targets[0])
        results[1] = mpi.gather( list2,   3,  targets[1])
        results[2] = mpi.gather( string1, 3,  targets[2])
        results[3] = mpi.gather( tuple1,  1,  targets[0])
        results[4] = mpi.gather( tuple2,  1,  targets[1])
        results[5] = mpi.gather( tuple2,  3,  targets[2])
        results[6] = mpi.gather( list3, mpi.rank+2, targets[0] )
        results[7] = mpi.gather( longList, 256, targets[0] )

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
            if mpi.rank == targets[x%3] and results[x] != correctAnswers[x]:
                self.fail(" gather failed on test "+str(x))

            elif mpi.rank != targets[x%3] and results[x] != None:
                errstr = "gather failed on off-target";
                errstr += "process on test " + str(x)
                self.fail( errstr)

        return
                              
#====================================================================
# AllGather
#====================================================================
class PyMPIAllGatherTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
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

        return

#====================================================================
# Scatter
#====================================================================
class PyMPIScatterTestCase(PyMPIBaseTestCase):
    def parallelRunTest(self):
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

        return

#====================================================================
# Create the testing suite
#====================================================================

BasicSuite = unittest.TestSuite()

# Work for any number of processors
BasicSuite.addTest(PyMPIReduceTestCase())
BasicSuite.addTest(PyMPIBarrierTestCase())
BasicSuite.addTest(PyMPIBroadcastTestCase())
BasicSuite.addTest(PyMPIBroadcast2TestCase())
BasicSuite.addTest(PyMPICartesianTestCase())
BasicSuite.addTest(PyMPICalcPiTestCase())
BasicSuite.addTest(PyMPISampleMIMDTestCase())
BasicSuite.addTest(PyMPINullCommTestCase())
BasicSuite.addTest(PyMPIPickledTestCase())
BasicSuite.addTest(PyMPIReduceSumTestCase())

# Need to be parallel
if mpi.size > 1:
    BasicSuite.addTest(PyMPISimpleSendTestCase())
    #BasicSuite.addTest(PyMPIWaitAllTestCase())
    #BasicSuite.addTest(PyMPIComplexNonblockTestCase())
    #BasicSuite.addTest(PyMPIWaitAnyTestCase())

# Need to be parallel and even processor count
if mpi.size % 2 == 0:
    BasicSuite.addTest(PyMPISimpleSendRecvTestCase())
    #BasicSuite.addTest(PyMPIPairedSendRecvTestCase())

# Broken
#BasicSuite.addTest(PyMPIReduceProductTestCase())
#BasicSuite.addTest(PyMPIReduceLogicalTestCase())
#BasicSuite.addTest(PyMPIAllReduceTestCase())
#BasicSuite.addTest(PyMPIBadCommTestCase())
#BasicSuite.addTest(PyMPISRNonblockTestCase())
#BasicSuite.addTest(PyMPISelfSendNonblockTestCase())
#BasicSuite.addTest(PyMPIGatherTestCase())
#BasicSuite.addTest(PyMPIAllGatherTestCase())
#BasicSuite.addTest(PyMPIScatterTestCase())

#====================================================================
# dummy stream for classes to write to
#====================================================================
class _NullStream:
    def __init__(self):
        pass
    def write(self,str):
        pass

#====================================================================
# main
#====================================================================
def main():

    #BasicSuite = unittest.TestSuite()
    #BasicSuite.addTest(PyMPIBarrierTestCase())
    if mpi.rank == 0:
        print "** Testing pyMPI package with "+sys.executable+" on "+sys.platform
        print "Interpreter Version: "+sys.version
        mpi.barrier()
        unittest.TextTestRunner(stream = sys.stdout).run(BasicSuite)
    else:
        mpi.barrier()
        unittest.TextTestRunner(stream = _NullStream()).run(BasicSuite)

if __name__ == '__main__':
    main()
