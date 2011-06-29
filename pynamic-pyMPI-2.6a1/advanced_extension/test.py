import sys
import string
try:
    import mpi
    time = mpi.wtime
except:
    mpi = None
    from time import time

from advanced import diffuse

if len(sys.argv) > 1:
    n = string.atoi(sys.argv[1])
else:
    n = 100000

t0 = time()
A,escape = diffuse(35,n,70)
t1 = time()
B = [x / float(n) for x in A]
H = int(max(B)*100+0.5)


if mpi is None or mpi.rank == 0:
    print A
    print escape,"walkers escaped the problem"
    for i in range(H,0,-1):
        for el in B:
            if el*100 > i:
                sys.stdout.write('X')
            else:
                sys.stdout.write(' ')
        sys.stdout.write('\n')
    for el in B:
        if el:
            sys.stdout.write('X')
        else:
            sys.stdout.write('.')
    sys.stdout.write('\n')
    print 'Time:',t1-t0
        
