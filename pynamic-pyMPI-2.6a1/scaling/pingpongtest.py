import sys
from time import sleep,asctime
import mpi
from mpi import time
import copy

import pingpong

def PRINTF(fd,fmt,*args):
     if args:
          s = fmt%args
     else:
          s = fmt
     fd.write(s)
     #sys.stdout.write(s)
     return


"""
set logscale x 2
set logscale y

plot "pingpong.plt" using 1:2 title "pyMPI" with linespoints, "pingpong.plt" using 1:4 title "C" with linespoints
"""

# Randomly split into senders and receivers
if mpi.rank == 0:
     import random
     ranks = list(range(mpi.size))
     random.shuffle(ranks)
     half = mpi.size/2
     senders = ranks[:half]
     receivers = ranks[half:half*2]
     dead = ranks[half*2:]
     sendmap = dict(zip(senders,receivers))
     recvmap = dict(zip(receivers,senders))

     mpi.bcast(sendmap)
     mpi.bcast(recvmap)
     mpi.bcast(dead)
else:
     sendmap = mpi.bcast()
     recvmap = mpi.bcast()
     dead = mpi.bcast()


runs = 10
k = 25

# Log file
fd = open('pingpong_%d.plt'%mpi.size,'w')
fd.write('# %s\n'%asctime())
fd.write('# %d processors\n'%mpi.size)

# Run various size messages
for power in range(k):
     mpi.barrier()
     n = 2 ** power
     msg = 'a'*n

     python = []
     c = []
     # I am a sender
     if mpi.rank in sendmap.keys():
          dst = sendmap[mpi.rank]
          for run in range(runs):
               mpi.barrier()
               t0 = time()
               x = mpi.send(msg,dst)
               y,status = mpi.recv(dst)
               python.append( time()-t0 )
               del t0
               del x

               mpi.barrier()
               t0 = time()
               x = pingpong.send(n,dst)
               y = pingpong.recv(n,dst)
               c.append( time()-t0 )

               del t0
               del x
               del y
               

     # I am a receiver
     elif mpi.rank in recvmap.keys():
          src = recvmap[mpi.rank]
          for run in range(runs):
               mpi.barrier()
               msg,status = mpi.recv()
               x = mpi.send(msg,src)

               mpi.barrier()
               y = pingpong.recv(n,src)
               z = pingpong.send(n,src)

               del msg
               del status
               del x
               del y
               del z

     # I am the odd one out
     else:
          for run in range(runs):
               mpi.barrier()
               mpi.barrier()

     # Get from all processes
     all_python = mpi.reduce(python,mpi.SUM)
     all_c = mpi.reduce(c,mpi.SUM)

     if all_python and all_c and mpi.rank == 0:
          if half:
               divisor = half
          else:
               divisor = 1

          if runs > 1:
               all_python = all_python[1:]
               all_c = all_c[1:]
               
          best_python = min(all_python)
          worst_python = max(all_python)
          avg_python = reduce(lambda x,y: x+y, all_python)/runs

          best_c = min(all_c)
          worst_c = max(all_c)
          avg_c = reduce(lambda x,y: x+y, all_c)/runs

          PRINTF(fd,'%8d ',n)
          for t in [best_python,best_c,worst_python,worst_c,avg_python,avg_c]:
               try:
                    bps = n/t
               except:
                    bps = 0
               PRINTF(fd,' %8.6f %8.0f',t,bps)
          PRINTF(fd,'\n')
