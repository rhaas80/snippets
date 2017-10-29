#ifndef ASYNC_WRITER_HH_
#define ASYNC_WRITER_HH_

#include <cstddef>
#include <cstdio>
#include <pthread.h>
#include <queue>

// a class to provide asynchronous IO to a file
// it is the caller's responsibility to fopen() the file, fclose() it, and to
// malloc() data for write()
class async_writer
{
  public:
    async_writer(FILE* out_fh, size_t max_bytes_queued_ = 1000000000);
    ~async_writer();
  private:
    // no copying or assignment
    async_writer(const async_writer& other);
    async_writer& operator=(const async_writer& other);

  public:
    // write buf to file, free() memory once done ie. buf must be obtained from
    // malloc()
    void write(const void* buf, size_t count);
    // seek to location offset in the file
    void seek(long offset);
    // flush command queue and wait for writer to finish
    void finalize();

  private:
    // data types to keep track of what the writer end needs to do
    enum cmd_name {CMD_INVALID = 0, CMD_WRITE, CMD_SEEK, CMD_EXIT, CMD_END};
    union cmd_block_t {
      struct cmd_hdr_t {
        cmd_name cmd;
      } cmd_hdr;
      struct write_block_t {
        cmd_name cmd;
        const void *buf;
        size_t count;
      } write_block;
      struct seek_block_t {
        cmd_name cmd;
        long offset;
      } seek_block;
      struct exit_block_t {
        cmd_name cmd;
      } exit_block;
    };
    typedef std::queue<cmd_block_t> cmd_queue_t;
    cmd_queue_t cmd_queue;
    size_t bytes_queued;

    // the output file
    FILE* fh; 
    // write() blocks until there are no more than max_bytes_queued bytes queued up
    const size_t max_bytes_queued;

    // threading support
    pthread_t writer_thread;
    // if holding both mutexes then bytes_lock must be aquired first!
    // protects access to bytes_queued variable and signals when it is reduced
    pthread_mutex_t bytes_lock;
    pthread_cond_t bytes_wait;
    // protects access to queue and signals when a new work item is added
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_wait;
    
    // these two functions implement the writer thread. The static one is
    // passed to pthread_create and wraps the member function
    static void* writer_func(void* calldata);
    void writer();
};

#endif // ASYNC_WRITER_HH_
