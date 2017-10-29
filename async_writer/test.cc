
#include "async_writer.hh"

#include <cstdio>
#include <cstring>
#include <unistd.h>

int main(void)
{
  {
    async_writer writer(stdout);
    writer.write(strdup("foo\n"), 4);
    fprintf(stderr, "available: %zu\n", writer.select(100));
    sleep(1);
    fprintf(stderr, "available: %zu\n", writer.select(100));
    writer.seek(1);
    writer.write(strdup("aa"), 2);
    fprintf(stderr, "available: %zu\n", writer.select(100));
    writer.finalize();
    fprintf(stderr, "available: %zu\n", writer.select(100));
  }
  return 0;
}

