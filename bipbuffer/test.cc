#include "bipbuffer.hh"
#include <cstdio>
#include <cstring>
#include <cassert>

int main(void)
{
  bipbuffer bb(10000);

  char s1[] = "Hello"; 
  size_t got;
  void *b = bb.writeTryReserve(sizeof(s1), got);
  assert(got == sizeof(s1));
  memcpy(b, s1, got);
  printf("Used1: %d\n", (int)bb.usedSize());
  bb.writeCommit(got);
  printf("Used2: %d\n", (int)bb.usedSize());
  void *c = bb.readTryReserve(1000000, got);
  printf("CanRead1: %d\n", (int)got);
  printf("Used3: %d\n", (int)bb.usedSize());
  bb.readCommit(got);
  printf("Used4: %d\n", (int)bb.usedSize());

  return 0;
}
