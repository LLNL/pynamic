import sys, os
import time
end_time = time.time()
start_time = 0
mpi_avail = True
try:
	import mpi
except:
	mpi_avail = False
if mpi_avail == False:
	print 'Sequoia Benchmark Version 0.9.0\n'
	if len(sys.argv) > 1:
		start_time = float(sys.argv[1])
		print 'startup time = ' + str(end_time - start_time) + ' secs'
	print 'pynamic driver beginning... now importing modules'
else:
	if mpi.rank == 0:
		print 'Sequoia Benchmark Version 0.9.0\n'
		if len(sys.argv) > 1:
			start_time = float(sys.argv[1])
			print 'startup time = ' + str(end_time - start_time) + ' secs'
		print 'pynamic driver beginning... now importing modules'
import_start = time.time()
import libmodule0
import libmodule1
import libmodule2
import libmodule3
import libmodule4
import_end = time.time()
import_time = import_end - import_start
if mpi_avail == False:
	print 'pynamic driver finished importing all modules... visiting all module functions'
else:
	if mpi.rank == 0:
		print 'pynamic driver finished importing all modules... visiting all module functions'
call_start = time.time()
libmodule0.libmodule0_entry()
libmodule1.libmodule1_entry()
libmodule2.libmodule2_entry()
libmodule3.libmodule3_entry()
libmodule4.libmodule4_entry()
call_end = time.time()
call_time = call_end - call_start
if mpi_avail == False:
	print '\nPynamic: module import time = ' + str(import_time) + ' secs'
	print 'Pynamic: module visit time = ' + str(call_time) + ' secs'
	print 'Pynamic: module test passed!\n'
	sys.exit(0)

import_time = mpi.reduce(import_time, mpi.SUM)
call_time = mpi.reduce(call_time, mpi.SUM)
if mpi.rank == 0:
	import_time = import_time / mpi.size
	call_time = call_time / mpi.size
	print '\nPynamic: module import time = ' + str(import_time) + ' secs'
	print 'Pynamic: module visit time = ' + str(call_time) + ' secs'
	print 'Pynamic: module test passed!\n'
	print 'Pynamic: testing mpi capability...\n'
mpi_start = time.time()
import mpi,sys
from array import *
from struct import *

#Function to return BMP header
def makeBMPHeader( width, height ):

    #Set up the bytes in the BMP header
    headerBytes = [ 66, 77, 28, 88, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0]
    headerBytes += [40] + 3*[0] + [100] + 3*[0] + [ 75,0,0,0,1,0,24] + [0]*9 + [18,11,0,0,18,11]
    headerBytes += [0]*10

    #Pack this data as a string
    data =""
    for x in range(54):
        data += pack( 'B', headerBytes[x] )
        
    #Create a string to overwrite the width and height in the BMP header
    replaceString = pack( '<I', width )
    replaceString += pack( '<I', height)
    
    #Return a 54-byte string that will be the new BMP header
    newString = data[0:18] + replaceString + data[26:54]
    return newString

#Define our fractal parameters
c = 0.4 + 0.3j
maxIterationsPerPoint = 64
distanceWhenUnbounded = 3.0

#define our function to iterate
def f( x ):
    return x*x + c

#Define the bounds of the xy plane we will work in
globalBounds = (-0.6, -0.6, 0.4, 0.4 ) #x1, y1, x2, y2

#define the size of the BMP to output
#For now this must be divisible by the # of processes
bmpSize = (400,400)

#Define the range of y-pixels in the BMP this process works on
myYPixelRange = [ 0,0]
myYPixelRange[0] = mpi.rank*bmpSize[1]/mpi.procs
myYPixelRange[1] = (mpi.rank+1)*bmpSize[1]/mpi.procs

if mpi.rank == 0:
    print "Starting computation (groan)\n"

#Now we can start to iterate over pixels!!
myString = ""
myArray = array('B')
for y in range( myYPixelRange[0], myYPixelRange[1]):
    for x in range( 0, bmpSize[0] ):
        
        #Calculate the (x,y) in the plane from the (x,y) in the BMP
        thisX = globalBounds[0] + (float(x)/bmpSize[0])*(globalBounds[2]-globalBounds[0])
        thisY = (float(y)/bmpSize[1])*(globalBounds[3] - globalBounds[1])
        thisY += globalBounds[1]
        
        #Create a complex # representation of this point
        thisPoint = complex(thisX, thisY)

        #Iterate the function f until it grows unbounded
        nxt = f( thisPoint )
        numIters = 0
        while 1:
            dif = nxt-thisPoint
            if abs(nxt - thisPoint) > distanceWhenUnbounded:
                break;
            if numIters >= maxIterationsPerPoint:
                break;
            nxt = f(nxt)
            numIters = numIters+1

        #Convert the number of iterations to a color value
        colorFac = 255.0*float(numIters)/float(maxIterationsPerPoint)
        myRGB = ( colorFac*0.8 + 32, 24+0.1*colorFac, 0.5*colorFac )

        #append this color value to a running list
        myArray.append( int(myRGB[2]) ) #blue first
        myArray.append( int(myRGB[1]) )    #The green
        myArray.append( int(myRGB[0]) )  #Red is last

#Now I reduce the lists to process 0!!
masterString = mpi.reduce( myArray.tostring(), mpi.SUM, 0 )

#Tell user that we're done
message = "process " + str(mpi.rank) + " done with computation!!"
print message

#Process zero does the file writing
if mpi.rank == 0:
    masterArray = array('B')
    masterArray.fromstring(masterString)

    #Write a BMP header
    myBMPHeader = makeBMPHeader( bmpSize[0], bmpSize[1] )
    print "Header length is ", len(myBMPHeader)
    print "BMP size is ", bmpSize
    print "Data length is ", len(masterString)

    #Open the output file and write to the BMP
    outFile = open( 'output.bmp', 'w' )
    outFile.write( myBMPHeader )
    outFile.write( masterString )
    outFile.close()


mpi.barrier()
mpi_end = time.time()
if mpi.rank == 0:
	print '\nPynamic: fractal mpi time = ' + str(mpi_end - mpi_start) + ' secs'
	print 'Pynamic: mpi test passed!\n'
