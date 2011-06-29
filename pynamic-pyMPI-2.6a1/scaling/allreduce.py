import mpi
import math
from time import time
import pingpong
from pingpong import timefunction
import os
import copy

def safediv(x,y):
  try:
    return x/y
  except ZeroDivisionError:
    return 0*x

rank = mpi.rank
size = mpi.size
count = 100

x = math.cos(rank)

def zeros(dims,value=0.0):
     z = 0
     dd = list(dims[:])
     dd.reverse()
     for d in dd:
        zz = [] 
        for i in range(d):
           zz.append(copy.deepcopy(z))
        z = zz
     return z


right = min([math.cos(i) for i in range(size)])
times = zeros((size,3,3))

for comm_size in range(1,size+1):
    comm = mpi.WORLD.comm_create(mpi.WORLD[:comm_size])
    if comm is None: continue
    
    # -----------------------------------------------
    # Try the default send method
    # -----------------------------------------------
    comm.barrier()
    t1,min = timefunction(count,comm.allreduce,x,mpi.MIN)
    local_error = abs(min-right)

    low1,high1,avg1,error = (
        comm.allreduce(t1,mpi.MIN),
        comm.allreduce(t1,mpi.MAX),
        comm.allreduce(t1,mpi.SUM)/size,
        comm.allreduce(local_error,mpi.MAX,type=float),
        )
    if rank == 0:
        times[comm_size-1][0] = (low1,high1,avg1)


    # -----------------------------------------------
    # Try the fast send method
    # -----------------------------------------------
    comm.barrier()
    t1,min = timefunction(count,comm.allreduce,x,mpi.MIN,type=float)

    local_error = abs(min-right)

    low2,high2,avg2,error = (
        comm.allreduce(t1,mpi.MIN),
        comm.allreduce(t1,mpi.MAX),
        comm.allreduce(t1,mpi.SUM)/size,
        comm.allreduce(local_error,mpi.MAX,type=float),
        )
    if rank == 0:
        times[comm_size-1][1] = (low2,high2,avg2)

    # -----------------------------------------------
    # Try the straight C method
    # -----------------------------------------------
    comm.barrier()
    t1,min = timefunction(count,pingpong.allreduce_doublemin,x,comm)
    local_error = abs(min-right)

    low3,high3,avg3,error = (
        comm.allreduce(t1,mpi.MIN),
        comm.allreduce(t1,mpi.MAX),
        comm.allreduce(t1,mpi.SUM)/size,
        comm.allreduce(local_error,mpi.MAX,type=float),
        )
    if rank == 0:
        times[comm_size-1][2] = (low3,high3,avg3)

        

if rank == 0:
    fp = open('allreduce.plt','w')
    fp.write('# low-pickle low-fast low-c high-pickle high-fast high-c avg-pickle avg-fast avg-c\n')
    for i in range(size):
        fp.write(str(i)+' ')
        for j in range(3):
            for k in range(3):
                fp.write(str(times[i][k][j])+' ')
        fp.write('\n')
    fp.close()

