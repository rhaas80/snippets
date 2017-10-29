
#include "async_writer.hh"

#include <cstdio>
#include <cstring>

int main(void)
{
  {
    async_writer writer(stdout);
    writer.write(strdup("foo\n"), 4);
    writer.seek(1);
    writer.write(strdup("aa"), 2);
  }
  return 0;
}

