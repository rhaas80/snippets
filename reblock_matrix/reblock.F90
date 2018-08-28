program complete

  implicit none
  include "mpif.h"

  ! how many MPI ranks to use with this example
  integer, parameter :: nranks = 4
  integer, dimension(nranks) :: all_matdim = (/2,3,1,4/)

  ! layout of parallel striped matrix
  integer, dimension(:), allocatable :: matdim_node
  integer :: matdim
  integer, dimension(:), allocatable :: matstart

  ! layout of the parallel blocked matrix
  integer, dimension(:,:), allocatable :: blockdim
  integer, dimension(:,:), allocatable :: blockstart
  integer :: nblocks_i, nblocks_j

  ! local matrices
  real*8, dimension(:,:), allocatable :: A, B

  ! MPI stuff
  integer rank, sz

  ! send and receive buffers
  integer :: bufp, bufsiz
  real*8, dimension(:), allocatable :: buf
  integer sender, receiver
  integer imin, imax, jmin, jmax
  integer loc_i, loc_j

  ! loop counters, misc stuff
  integer :: i,j, r, ierr

  call MPI_INIT(ierr)

  call MPI_Comm_size(MPI_COMM_WORLD, sz, ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, rank, ierr)

  allocate(matdim_node(0:sz-1), matstart(0:sz-1))
  matdim_node = all_matdim(1:sz)
  matdim = sum(matdim_node)
  matstart = 0
  do i = 1,SIZE(matstart)-1
    matstart(i) = matstart(i-1) + matdim_node(i-1)
  end do

  ! describe the blocked matrix
  nblocks_i = int(sqrt(real(sz)))
  nblocks_j = sz / nblocks_i
  if (nblocks_i * nblocks_j .ne. sz) then
    write (*,'(a,i4,i4,i4)')  "Warnning: will leave some emtpy ranks since %d x %d != %d", nblocks_i, nblocks_j, sz
    STOP
  end if
  allocate(blockstart(0:sz-1,2))
  blockstart = 0
  allocate(blockdim(0:sz-1,2))
  blockdim = 0
  r = 0
  do i = 0,nblocks_i-1
    do j = 0,nblocks_j-1
      blockstart(r,1) = i * int(matdim/nblocks_i)
      blockstart(r,2) = j * int(matdim/nblocks_j)
      blockdim(r,1) = min(matdim, (i+1)*int(matdim/nblocks_i)) - blockstart(r,1)
      blockdim(r,2) = min(matdim, (j+1)*int(matdim/nblocks_j)) - blockstart(r,2)
      r = r + 1
    end do
  end do

  if (rank .eq. 0) then
    print *, ("Blocks are:")
    do r = 0,sz-1
      write (*, '("at (",i4,i4,") sz (",i4,i4,")")') blockstart(r,1), blockstart(r,2), blockdim(r,1), blockdim(r,2)
    end do
  end if

  ! set up a trial matrix
  allocate(A(matdim_node(rank),matdim))
  A = 0
  do i = 1, SIZE(A, 1)
    do j = 1,SIZE(A,2)
      A(i,j) = 10*(i+matstart(rank))+j
    end do
  end do

  ! show matrix
  if(rank .eq. 0) then
    print *, "A"
  end if
  call MPI_Barrier(MPI_COMM_WORLD, ierr)
  do i = 0,sz-1
    if(i .eq. rank) then
      do j = 1, SIZE(A, 1)
        print *,  A(j,:)
      end do
    end if
    call MPI_Barrier(MPI_COMM_WORLD, ierr)
  end do

  ! my block
  allocate(B(blockdim(rank,1),blockdim(rank,2)))
  B=0

  ! get some large enough buffer which can be at most the largest number of rows
  ! of a rank times the largest number of columns of a rank
  ! this can be at most 50% of the size of a local matrix
  bufsiz = int(maxval(matdim_node) * maxval(blockdim(:,2)))
  allocate(buf(bufsiz))
  do sender = 0,sz-1
    do receiver = 0,sz-1
      ! sender is stripped, receiver is blocked
      imin = max(1+matstart(sender), 1+blockstart(receiver,1))
      imax = min(matstart(sender)+matdim_node(sender), &
                 & blockstart(receiver,1)+blockdim(receiver,1))
      jmin = max(1, 1+blockstart(receiver,2))
      jmax = min(matdim, &
                 & blockstart(receiver,2)+blockdim(receiver,2))
      if(imin .gt. imax .or. jmin .gt. jmax) then
        cycle ! no overlap
      end if
  
      ! pack array data into serialized container
      if (rank .eq. sender .or. rank .eq. receiver) then
        bufp = 0
        ! TODO: this could be done using a slice
        ! TODO: could be done using a MPI datatype or alltoallv to avoid the buf
        ! see
        ! https://events.prace-ri.eu/event/176/session/1/contribution/13/material/slides/0.pdf
        ! for how to do this
        do i = imin, imax
          do j = jmin, jmax
            loc_i = i - matstart(sender)
            loc_j = j
            bufp = bufp + 1
            if (rank .eq. sender) then
              buf(bufp) = A(loc_i,loc_j)
            end if
          end do
        end do
  
      ! ship container
      if (sender .ne. receiver) then
          if (rank .eq. sender) then
            if (bufp .eq. 0) then
              print *, "Bufp is zero in sender or reciever"
              STOP
            end if
            call MPI_Send(buf, bufp, MPI_DOUBLE, receiver, 0, &
                          & MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
          else if (rank .eq. receiver) then
            if (bufp .eq. 0) then
              print *, "Bufp is zero in sender or reciever"
              STOP
            end if
            call MPI_Recv(buf, bufp, MPI_DOUBLE, sender, 0, &
                          & MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
          end if
        end if
      end if
  
      ! unpack serialized data into local array while transposing
      if (rank .eq. receiver) then
        bufp = 0
        do i  = imin, imax
          do j = jmin, jmax
            loc_i = i - blockstart(receiver,1)
            loc_j = j - blockstart(receiver,2)
            bufp = bufp + 1
            B(loc_i,loc_j) = buf(bufp)
          end do
        end do
      end if
    end do
  end do

  ! show results
  if (rank .eq. 0) then
    write (6,*) "B"
  end if
  call MPI_Barrier(MPI_COMM_WORLD, ierr)
  do i = 1, matdim 
    do j = 1, matdim
      if ((i .ge. 1+blockstart(rank,1) .and. i .le. blockstart(rank,1)+blockdim(rank,1)) .and. &
        & (j .ge. 1+blockstart(rank,2) .and. j .le. blockstart(rank,2)+blockdim(rank,2))) then
        write (6, '(f8.0," ")', advance="no") B(i - blockstart(rank,1), j - blockstart(rank,2))
        flush (UNIT=6)
      end if
      call MPI_Barrier(MPI_COMM_WORLD, ierr) ! this is horrible, matdim**2 barriers!
    end do
    if (rank .eq. 0) then
      write (6, *) " "
      flush (UNIT=6)
    end if
    call MPI_Barrier(MPI_COMM_WORLD, ierr)
    call sleep (1) ! sigh, this helps not to mangle output.
  end do

  call MPI_Finalize(ierr)

end program
