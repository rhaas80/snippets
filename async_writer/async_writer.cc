#include "async_writer.hh"

#include <cassert>
#include <cstdlib>

#include <sys/types.h>
#include <unistd.h>

async_writer::async_writer(FILE* out_fh, const size_t max_bytes_queued_) :
   bytes_queued(0), fh(out_fh), max_bytes_queued(max_bytes_queued_)
{
  int bytes_mutex_init = pthread_mutex_init(&bytes_lock, NULL);
  int bytes_cond_init = pthread_cond_init(&bytes_wait, NULL);
  assert(bytes_mutex_init == 0 && bytes_cond_init == 0);
  int queue_mutex_init = pthread_mutex_init(&queue_lock, NULL);
  int queue_cond_init = pthread_cond_init(&queue_wait, NULL);
  assert(queue_mutex_init == 0 && queue_cond_init == 0);

  int thread_create = pthread_create(&writer_thread, NULL, writer_func,
                                       static_cast<void*>(this));
  assert(thread_create == 0);
};

async_writer::~async_writer()
{
  // make writer thread exit
  cmd_block_t exit_cmd;
  exit_cmd.exit_block.cmd = CMD_EXIT;

  pthread_mutex_lock(&queue_lock);

  cmd_queue.push(exit_cmd);

  pthread_cond_signal(&queue_wait);
  pthread_mutex_unlock(&queue_lock);
  
  // this waits for writer thread to finish
  int join = pthread_join(writer_thread, NULL);
  assert(join == 0);
}

void async_writer::write(const void* buf, size_t count)
{
  cmd_block_t write_cmd;
  write_cmd.write_block.cmd = CMD_WRITE;
  write_cmd.write_block.count = count;
  write_cmd.write_block.buf = buf;

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
  cmd_block_t seek_cmd;
  seek_cmd.seek_block.cmd = CMD_SEEK;
  seek_cmd.seek_block.offset = offset;

  pthread_mutex_lock(&queue_lock);

  cmd_queue.push(seek_cmd);

  pthread_cond_signal(&queue_wait);
  pthread_mutex_unlock(&queue_lock);
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
        assert(written == cmd_block.write_block.count);
        free(const_cast<void*>(cmd_block.write_block.buf));

        pthread_mutex_lock(&bytes_lock);
        bytes_queued -= written;
        pthread_cond_signal(&bytes_wait);
        pthread_mutex_unlock(&bytes_lock);
        } break;
      case CMD_SEEK: {
        int seeked = fseek(fh, cmd_block.seek_block.offset, SEEK_SET);
        assert(seeked == 0);
        } break;
      case CMD_EXIT: {
        // not really required but keeps writes here rather than in caller
        int flushed = fflush(fh);
        assert(flushed == 0);
        done = true;
        } break;
      default:
        assert(0);
        break;
    }
  }
}
