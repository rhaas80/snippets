program complete

  implicit none
  include "mpif.h"

  ! how many MPI ranks to use with this example
  integer, parameter :: nranks = 4
  integer, dimension(nranks) :: all_matdim = (/2,3,1,4/)

  ! layout of parallel matrix
  integer, dimension(:), allocatable :: matdim_node
  integer :: matdim
  integer, dimension(:), allocatable :: matstart

  ! local matrices
  real*8, dimension(:,:), allocatable :: A, B

  ! MPI stuff
  integer rank, sz

  ! send and receive buffers
  integer :: bufp, bufsiz
  real*8, dimension(:), allocatable :: buf
  integer sender, receiver

  ! loop counters, misc stuff
  integer :: i,j, r, c, ierr

  call MPI_INIT(ierr)

  call MPI_Comm_size(MPI_COMM_WORLD, sz, ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)
  rank = rank + 1 ! count from 1 for array indexing

  allocate(matdim_node(sz), matstart(sz))
  matdim_node = all_matdim(1:sz)
  matdim = sum(matdim_node)
  matstart = 1
  do i = 2,SIZE(matstart)
    matstart(i) = matstart(i-1) + matdim_node(i-1)
  end do

  ! set up an upper triagonal matrix
  allocate(A(matdim_node(rank),matdim))
  A = 0
  do i = 1, SIZE(A, 1)
    do j = 1,SIZE(A,2)
      if(j < i + matstart(rank)-1) then
        cycle
      end if
      A(i,j) = 10*(i+matstart(rank)-1)+j
    end do
  end do

  ! show matrix
  if(rank == 1) then
    print *, "A"
  end if
  call MPI_Barrier(MPI_COMM_WORLD, ierr)
  do i = 1,sz
    if(i == rank) then
      do j = 1, SIZE(A, 1)
        print *,  A(j,:)
      end do
    end if
    call MPI_Barrier(MPI_COMM_WORLD, ierr)
  end do

  ! make a copy so that I can compare
  allocate(B(SIZE(A,1), SIZE(A,2)))
  B = A

  ! get some large enough buffer which can be at most the square of the largest
  ! set of rows per rank
  bufsiz = int(maxval(matdim_node)**2)
  allocate(buf(bufsiz))
  do sender = 1,sz
    do receiver = sender, sz
      ! pack array data into serialized container
      if(rank == sender) then
        bufp = 1
        do r = 1,matdim_node(sender)
          do c = matstart(receiver), matstart(receiver)-1+matdim_node(receiver)
            if(matstart(sender)-1 + r >= c) then
              cycle
            end if
            buf(bufp) = A(r,c)
            bufp = bufp + 1
          end do
        end do
      end if
  
      ! ship container
      if(sender .ne. receiver) then
        if(rank == sender) then
          call MPI_Send(buf, SIZE(buf), MPI_DOUBLE, receiver-1, 0, &
                        & MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
        elseif(rank == receiver) then
          call MPI_Recv(buf, SIZE(buf), MPI_DOUBLE, sender-1, 0, &
                        & MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
        end if
      end if
  
      ! unpack serialized data into local array while transposing
      if(rank == receiver) then
        bufp = 1
        do c = matstart(sender), matstart(sender)-1+matdim_node(sender)
          do r = 1,matdim_node(receiver)
            if(matstart(receiver)-1 + r <= c) then 
              cycle
            end if
            B(r,c) = buf(bufp)
            bufp = bufp + 1
          end do
        end do
      end if
    end do
  end do


  ! show matrix
  if(rank == 1) then
    print *, "B"
  end if
  call MPI_Barrier(MPI_COMM_WORLD, ierr)
  do i = 1,sz
    if(i == rank) then
      do j = 1, SIZE(B, 1)
        print *,  B(j,:)
      end do
    end if
    call MPI_Barrier(MPI_COMM_WORLD, ierr)
  end do

  call MPI_Finalize(ierr)

end program
