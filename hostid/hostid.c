#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */ 
#include <unistd.h>
#include <sys/types.h>

#include <mpi.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
  MPI_Init(&argc, &argv);

  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int host_ids[size];

  // get a shared memory segment and single out one process per host (the one
  // that manages to create the region)
  int host_root = 1;
  int shm_fd = shm_open("/A-UNIQUE-SHM_NAME", O_RDWR | O_CREAT | O_EXCL, 0700);
  if(shm_fd == -1) {
    if(errno == EEXIST) {
      shm_fd = shm_open("/A-UNIQUE-SHM_NAME", O_RDWR, 0700);
      assert(shm_fd != -1);
      host_root = 0;
    } else {
      assert(0);
    }
  }
  if(host_root)
    ftruncate(shm_fd, sizeof(int));
  int *shared_id = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, 
                        MAP_SHARED, shm_fd, /*offset*/0);
  if(shared_id == MAP_FAILED) {
    printf("mmap failed: %s\n", strerror(errno));
    assert(0);
  }

  // now get a nice continuous hostid
  MPI_Comm comm;
  MPI_Comm_split(MPI_COMM_WORLD, host_root, rank, &comm);
  if(host_root)
    MPI_Comm_rank(comm, shared_id);

  // need to wait to avoid a race condition
  MPI_Barrier(MPI_COMM_WORLD);

  // get my id
  MPI_Allgather(shared_id, 1, MPI_INT, host_ids, 1, MPI_INT,
                MPI_COMM_WORLD);

  if(rank == 0) {
    printf("Host to rank mapping:\n");
    for(int i = 0 ; i < size ; i++) {
      printf("rank %d is on host %d\n", i, host_ids[i]);
    }
  }
  
  // tear everything down
  MPI_Comm_free(&comm);
  munmap(shared_id, sizeof(int));
  shm_unlink("/A-UNIQUE-SHM_NAME");

  MPI_Finalize();
}
