import mpi,sys
from array import *
from struct import *

#Function to return BMP header
def makeBMPHeader( width, height ):

    #Set up the bytes in the BMP header
    headerBytes = [ 66, 77, 28, 88, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0]
    headerBytes += [40] + 3*[0] + [100] + 3*[0] + [ 75,0,0,0,1,0,24] + [0]*9 + [18,11,0,0,18,11]
    headerBytes += [0]*10

    data = b''
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
myYPixelRange[0] = int(mpi.rank*bmpSize[1]/mpi.procs)
myYPixelRange[1] = int((mpi.rank+1)*bmpSize[1]/mpi.procs)

if mpi.rank == 0:
    print("Starting computation (groan)\n")

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
masterArray = mpi.reduce( myArray, mpi.SUM, 0 )

#Tell user that we're done
message = "process " + str(mpi.rank) + " done with computation!!"
print(message)

#Process zero does the file writing
if mpi.rank == 0:
    #Write a BMP header
    myBMPHeader = makeBMPHeader( bmpSize[0], bmpSize[1] )
    print("Header length is ", len(myBMPHeader))
    print("BMP size is ", bmpSize)
    print("Data length is ", len(masterArray))

    #Open the output file and write to the BMP
    outFile = open( 'output.bmp', 'wb' )
    outFile.write( myBMPHeader )
    outFile.write( masterArray )
    outFile.close()


