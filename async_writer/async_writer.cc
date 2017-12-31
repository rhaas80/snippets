#include "async_writer.hh"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <unistd.h>

async_writer::async_writer(FILE* out_fh, const size_t max_bytes_queued_) :
   bytes_queued(0), fh(out_fh), max_bytes_queued(max_bytes_queued_),
   thread_active(false)
{
  int bytes_mutex_init = pthread_mutex_init(&bytes_lock, NULL);
  int bytes_cond_init = pthread_cond_init(&bytes_wait, NULL);
  if(bytes_mutex_init != 0 || bytes_cond_init != 0) {
    fprintf(stderr, "Failed to initialize mutexes for bytes variable: %d %d\n",
            bytes_mutex_init, bytes_cond_init);
    exit(1);
  }
  int queue_mutex_init = pthread_mutex_init(&queue_lock, NULL);
  int queue_cond_init = pthread_cond_init(&queue_wait, NULL);
  if(queue_mutex_init != 0 || queue_cond_init != 0) {
    fprintf(stderr, "Failed to initialize mutexes for queue variable: %d %d\n",
            queue_mutex_init, queue_cond_init);
    exit(1);
  }

  int thread_create = pthread_create(&writer_thread, NULL, writer_func,
                                       static_cast<void*>(this));
  if(thread_create != 0) {
    fprintf(stderr, "Failed to create writer thread: %d\n", thread_create);
    exit(1);
  }
  thread_active = true;
};

async_writer::~async_writer()
{
  if(thread_active)
    finalize();
  assert(!thread_active);
}

void async_writer::write(const void* buf, size_t count, void (*free_func)(void*))
{
  assert(thread_active);

  cmd_block_t write_cmd;
  write_cmd.write_block.cmd = CMD_WRITE;
  write_cmd.write_block.count = count;
  write_cmd.write_block.buf = buf;
  write_cmd.write_block.free_func = free_func;

  pthread_mutex_lock(&bytes_lock);
  while(bytes_queued >= max_bytes_queued)
    pthread_cond_wait(&bytes_wait, &bytes_lock);
  pthread_mutex_lock(&queue_lock);

  bytes_queued += count;
  cmd_queue.push(write_cmd);

  pthread_cond_signal(&queue_wait);
  pthread_mutex_unlock(&queue_lock);
  pthread_mutex_unlock(&bytes_lock);
}

void async_writer::seek(long offset)
{
  assert(thread_active);

  cmd_block_t seek_cmd;
  seek_cmd.seek_block.cmd = CMD_SEEK;
  seek_cmd.seek_block.offset = offset;

  pthread_mutex_lock(&queue_lock);

  cmd_queue.push(seek_cmd);

  pthread_cond_signal(&queue_wait);
  pthread_mutex_unlock(&queue_lock);
}

void async_writer::finalize()
{
  if(!thread_active)
    return;

  // make writer thread exit
  cmd_block_t exit_cmd;
  exit_cmd.exit_block.cmd = CMD_EXIT;

  pthread_mutex_lock(&queue_lock);

  cmd_queue.push(exit_cmd);

  pthread_cond_signal(&queue_wait);
  pthread_mutex_unlock(&queue_lock);

  // this waits for writer thread to finish
  int join_ierr = pthread_join(writer_thread, NULL);
  if(join_ierr != 0) {
    fprintf(stderr, "Failed to join with writer thread: %d\n", join_ierr);
    exit(1);
  }

  thread_active = false;
}

size_t async_writer::select(const size_t bytes_requested)
{
  assert(bytes_requested <= max_bytes_queued);

  pthread_mutex_lock(&bytes_lock);
  while(max_bytes_queued - bytes_queued < bytes_requested)
    pthread_cond_wait(&bytes_wait, &bytes_lock);
  const size_t bytes_available = max_bytes_queued - bytes_queued;
  pthread_mutex_unlock(&bytes_lock);

  return bytes_available;
}


void* async_writer::writer_func(void* calldata)
{
  async_writer* obj = static_cast<async_writer*>(calldata);
  obj->writer();
  return NULL;
}
  
void async_writer::writer()
{
  bool done = false;
  while(!done) {
    pthread_mutex_lock(&queue_lock);
    while(cmd_queue.empty())
      pthread_cond_wait(&queue_wait, &queue_lock);
    cmd_block_t cmd_block = cmd_queue.front();
    cmd_queue.pop();
    pthread_mutex_unlock(&queue_lock);
    switch(cmd_block.cmd_hdr.cmd) {
      case CMD_WRITE: {
        size_t written =
          fwrite(cmd_block.write_block.buf, 1, cmd_block.write_block.count, fh);
        if(written != cmd_block.write_block.count) {
          fprintf(stderr, "Could not write %zd bytes: %s\n",
                  cmd_block.write_block.count, strerror(errno));
          exit(1);
        }
        if(cmd_block.write_block.free_func)
          cmd_block.write_block.free_func(const_cast<void*>(cmd_block.write_block.buf));

        pthread_mutex_lock(&bytes_lock);
        bytes_queued -= written;
        pthread_cond_signal(&bytes_wait);
        pthread_mutex_unlock(&bytes_lock);
        } break;
      case CMD_SEEK: {
        int seeked = fseek(fh, cmd_block.seek_block.offset, SEEK_SET);
        if(seeked != 0) {
          fprintf(stderr, "Could not seek to position %ld: %s\n", cmd_block.seek_block.offset, strerror(errno));
          exit(1);
        }
        } break;
      case CMD_EXIT: {
        // not really required but keeps writes here rather than in caller
        int flushed = fflush(fh);
        if(flushed != 0) {
          fprintf(stderr, "Could not flush: %s\n", strerror(errno));
          exit(1);
        }
        done = true;
        } break;
      default:
        fprintf(stderr, "Invalid command %d\n", cmd_block.cmd_hdr.cmd);
        exit(1);
        break;
    }
  }
}
