#!/usr/bin/python

import mpi4py.MPI as MPI
import numpy as np
import sys

comm = MPI.COMM_WORLD
size = comm.Get_size()
rank = comm.Get_rank()

matdim_node = [2,3,1,4]

matdim_node = matdim_node[:size]
matdim = sum(matdim_node)
matstart = np.zeros(len(matdim_node), dtype=int)
for i in range(1,len(matstart)):
  matstart[i] = matstart[i-1] + matdim_node[i-1]

# describe the blocked matrix
nblocks_i = int(np.sqrt(size))
nblocks_j = size / nblocks_i
if (nblocks_i * nblocks_j != size):
  print("Warnning: will leave some emtpy ranks since %d x %d != %d" % (nblocks_i, nblocks_j, size))
blockstart = np.zeros((size,2), dtype=int)
blockdim = np.zeros((size,2), dtype=int)
r = 0
for i in range(nblocks_i):
  for j in range(nblocks_j):
    blockstart[r,0] = i * int(matdim/nblocks_i)
    blockstart[r,1] = j * int(matdim/nblocks_j)
    blockdim[r,0] = min(matdim, (i+1)*int(matdim/nblocks_i)) - blockstart[r,0]
    blockdim[r,1] = min(matdim, (j+1)*int(matdim/nblocks_j)) - blockstart[r,1]
    r += 1

# quick sanity check
r = 0
for i in range(nblocks_i):
  for j in range(nblocks_j):
    if ((i == nblocks_i-1) and not (blockstart[r,0] + blockdim[r,0] == matdim)):
      print("Incorrect matrix decomp in x: ", blockstart, blockdim)
      raise("Error")
    if ((j == nblocks_j-1) and not (blockstart[r,1] + blockdim[r,1] == matdim)):
      print("Incorrect matrix decomp in y: ", blockstart, blockdim)
      raise("Error")
    r += 1

if (rank == 0):
  print("Blocks are:")
  for r in range(size):
    print("at (%d,%d) sz (%d,%d)" % (blockstart[r,0], blockstart[r,1], blockdim[r,0], blockdim[r,1]))

# set up a trial matrix
A = np.zeros(shape=(matdim_node[rank], matdim))
for i in range(0,A.shape[0]):
  for j in range(0,A.shape[1]):
    A[i,j] = 10*(i+matstart[rank]+1)+j+1

# show matrix
if rank == 0:
  print "A"
comm.Barrier()
for i in range(size):
  if(i == rank):
    print(A)
  comm.Barrier()

# my block
B = np.zeros(shape=blockdim[rank,:])

# get some large enough buffer which can be at most the largest number of rows
# of a rank times the largest number of columns of a rank
# this can be at most 50% of the size of a local matrix
bufsiz = int(max(matdim_node) * max(blockdim[:,1]))
buf = np.zeros(bufsiz)
for sender in range(0,size):
  for receiver in range(0, size):
    # sender is stripped, receiver is blocked
    imin = max(matstart[sender], blockstart[receiver,0])
    imax = min(matstart[sender]+matdim_node[sender],
               blockstart[receiver,0]+blockdim[receiver,0])
    jmin = max(0, blockstart[receiver,1])
    jmax = min(0+matdim,
               blockstart[receiver,1]+blockdim[receiver,1])
    if(imin >= imax or jmin >= jmax): continue # no overlap

    # pack array data into serialized container
    if (rank == sender or rank == receiver):
      bufp = 0
      # TODO: this could be done using a slice
      # TODO: could be done using a MPI datatype or alltoallv to avoid the buf
      # see
      # https://events.prace-ri.eu/event/176/session/1/contribution/13/material/slides/0.pdf
      # for how to do this
      for i in range(imin, imax):
        for j in range(jmin, jmax):
          loc_i = i - matstart[sender]
          loc_j = j
          if (rank == sender):
            buf[bufp] = A[loc_i,loc_j]
          bufp += 1

    # ship container
    if sender != receiver:
        if rank == sender:
          if (bufp == 0):
            print("Bufp is zero in sender or reciever")
            raise("Error")
          comm.send(buf[0:bufp], dest=receiver)
        elif rank == receiver:
          if (bufp == 0):
            print("Bufp is zero in sender or reciever")
            raise("Error")
          buf[0:bufp] = comm.recv(source=sender)

    # unpack serialized data into local array while transposing
    if rank == receiver:
      bufp = 0
      for i in range(imin, imax):
        for j in range(jmin, jmax):
          loc_i = i - blockstart[receiver,0]
          loc_j = j - blockstart[receiver,1]
          B[loc_i,loc_j] = buf[bufp]
          bufp += 1

# show results
if rank == 0:
  print("B")
comm.Barrier()
for i in range(matdim):
  for j in range(matdim):
    if ((i >= blockstart[rank,0] and i < blockstart[rank,0]+blockdim[rank,0]) and 
        (j >= blockstart[rank,1] and j < blockstart[rank,1]+blockdim[rank,1])):
      sys.stdout.write(str(B[i - blockstart[rank,0], j - blockstart[rank,1]]) + "\t")
      sys.stdout.flush()
    comm.Barrier() # this is horrible, matdim**2 barriers!
  if (rank == 0):  
    print("")
  comm.Barrier()
