import numpy
import mpi
import os
from distutils.core import setup, Extension
from distutils.sysconfig import mode

print
try:
    machine = os.environ['HOSTNAME'].split('.')[0]
except:
    machine = 'xxx'

try:
    if mpi.rank == 0:
        import cbenchmark
except ImportError:
    mode('parallel')
    setup(script_args=['install','--install-lib=.'],
          ext_modules=[
        Extension('cbenchmark',['cbenchmark.c'])
        ],
          )
mpi.barrier()
import cbenchmark

    
LOG_MAX_MESSAGE_SIZE = 21
LOG_MIN_MESSAGE_SIZE = 2
MAX_MESSAGE_SIZE = 1 << LOG_MAX_MESSAGE_SIZE

def TIMER_START():
    global __time
    __time = mpi.wtime()*1e6
    return

def TIMER_STOP():
    global __time
    t = mpi.wtime()*1e6
    __time = t - __time
    return

def TIMER_ELAPSED():
    global __time
    return __time

message = numpy.array(MAX_MESSAGE_SIZE*"a")

master = 0
slave = mpi.size-1

assert slave != master

def latency(cnt,bytes):
    if mpi.rank == 0:
        TIMER_START()
        for i in range(cnt):
            mpi.send(message[:bytes],slave)
        TIMER_STOP()
        msg,status = mpi.recv(slave)

        total = TIMER_ELAPSED()
        return total/cnt,"u-sec"
    
    elif mpi.rank == slave:
        for i in range(cnt):
            msg,status = mpi.recv(master)
        mpi.send(message[:4],master)
        return 0.0,"u-sec"

    else:
        return 0.0,"u-sec"


def roundtrip(cnt,bytes):
    if mpi.rank == 0:
        TIMER_START()
        for i in range(cnt):
            mpi.send(message[:bytes],slave)
            msg,status = mpi.recv(slave)
        TIMER_STOP()

        total = TIMER_ELAPSED()
        return cnt / ( total *1e-6 ),"transactions/sec"
    
    elif mpi.rank == slave:
        for i in range(cnt):
            msg,status = mpi.recv(master)
            mpi.send(message[:bytes],master)
        return 0.0,"transactions/sec"

    else:
        return 0.0,"transactions/sec"
    

def bandwidth(cnt,bytes):
    if mpi.rank == 0:
        TIMER_START()
        for i in range(cnt):
            mpi.send(message[:bytes],slave)
        msg,status = mpi.recv(slave)
        TIMER_STOP()

        total = TIMER_ELAPSED()
        return ((4+(bytes*cnt))/1024.0) / (total*1e-6),"KB/sec"
    
    elif mpi.rank == slave:
        for i in range(cnt):
            msg,status = mpi.recv(master)
        mpi.send(message[:bytes],master)
        return 0.0,"KB/sec"

    else:
        return 0.0,"KB/sec"

def bibandwidth(cnt,bytes):
    if mpi.rank == 0:
        TIMER_START()
        for i in range(cnt):
            r1 = mpi.irecv(slave)
            r0 = mpi.isend(message[:bytes],slave)
            mpi.waitall([r0,r1])
        TIMER_STOP()

        total = TIMER_ELAPSED()
        return (((2.0*bytes*cnt))/1024.0) / (total*1e-6),"KB/sec"
    
    elif mpi.rank == slave:
        for i in range(cnt):
            r1 = mpi.irecv(master)
            r0 = mpi.isend(message[:bytes],master)
            mpi.waitall([r0,r1])

        return 0.0,"KB/sec"

    else:
        return 0.0,"KB/sec"


sizes = [1 << i for i in range(LOG_MAX_MESSAGE_SIZE)]
functions = [latency]
functions = [latency, roundtrip, bandwidth, bibandwidth]
if mpi.rank == master:
    print
for function in functions:
    if mpi.rank == master:
        print '-- %s --'%function.__name__
        fp = open('%s.%s'%(function.__name__,machine),'w')
        fp.write('# -- %s on %s --\n'%(function.__name__,machine))
    for size in sizes:
        x,units = function(100,size)
        c_version = getattr(cbenchmark,function.__name__)
        y = c_version(100,size)
        if mpi.rank == master:
            print '%10d %g %s (%g)'%(size,x,units,y)
            fp.write('%s\t%s\t%s\n'%(size,x,y))
    if mpi.rank == master:
        fp.close()

        name = function.__name__
        datafile = '%s.%s'%(name,machine)
        outputfile = '%s.ps'%(datafile)
        command = """
        set terminal postscript color
        set output "%(outputfile)s"
        set logscale y
        set title "-- %(name)s --"
        set xlabel "Message Size"
        set ylabel "%(units)s"
        plot "%(datafile)s" using 1:2 title "pyMPI" with linespoints, "%(datafile)s" using 1:3 title "C MPI" with linespoints
        """%locals()
        open('%s.plt'%datafile,'w').write(command)
        try:
            os.system('gnuplot %s.plt'%datafile)
        except:
            pass
                        
