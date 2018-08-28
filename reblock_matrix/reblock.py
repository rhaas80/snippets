#!/usr/bin/python

import mpi4py.MPI as MPI
import numpy as np

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()

matdim_node = [2,3,1,4]

matdim_node = matdim_node[:size]
matdim = sum(matdim_node)
matstart = np.zeros(len(matdim_node), dtype=int)
for i in range(1,len(matstart)):
  matstart[i] = matstart[i-1] + matdim_node[i-1]

# set up an upper triagonal matrix
A = np.zeros(shape=(matdim_node[rank], matdim))
for i in range(0,A.shape[0]):
  for j in range(0,A.shape[1]):
    if(j < i + matstart[rank]): continue
    A[i,j] = 10*(i+matstart[rank]+1)+j+1

# show matrix
if rank == 0:
    print "A"
comm.Barrier()
for i in range(size):
  if(i == rank):
    print(A)
  comm.Barrier()

# make a copy so that I can compare
B = A.copy()

# get some large enough buffer which can be at most the square of the largest
# set of rows per rank
bufsiz = int(max(matdim_node)**2)
buf = np.zeros(bufsiz)
for sender in range(0,size):
  for receiver in range(sender, size):
    # pack array data into serialized container
    if rank == sender:
      bufp = 0
      for r in range(matdim_node[sender]):
        for c in range(matstart[receiver],
                       matstart[receiver]+matdim_node[receiver]):
          if(matstart[sender] + r >= c): continue
          buf[bufp] = A[r,c]
          bufp += 1

    # ship container
    if sender != receiver:
        if rank == sender:
          comm.send(buf, dest=receiver)
        elif rank == receiver:
          buf = comm.recv(source=sender)

    # unpack serialized data into local array while transposing
    if rank == receiver:
      bufp = 0
      for c in range(matstart[sender], matstart[sender]+matdim_node[sender]):
        for r in range(matdim_node[receiver]):
          if(matstart[receiver] + r <= c): continue
          B[r,c] = buf[bufp]
          bufp += 1

# show results
if rank == 0:
    print "B"
comm.Barrier()
for i in range(size):
  if(i == rank):
    print(B)
  comm.Barrier()

