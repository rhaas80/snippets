#include "BipBuffer.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main(void)
{
  wBipBuffer *bb = BipBuffer_New(10000);
  assert(bb);

  char s1[] = "Hello"; 
  size_t got;
  void *b = BipBuffer_WriteTryReserve(bb, sizeof(s1), &got);
  assert(got == sizeof(s1));
  memcpy(b, s1, got);
  printf("Used1: %d\n", (int)BipBuffer_UsedSize(bb));
  BipBuffer_WriteCommit(bb, got);
  printf("Used2: %d\n", (int)BipBuffer_UsedSize(bb));
  void *c = BipBuffer_ReadTryReserve(bb, 1000000, &got);
  printf("CanRead1: %d\n", (int)got);
  printf("Used3: %d\n", (int)BipBuffer_UsedSize(bb));
  BipBuffer_ReadCommit(bb, got);
  printf("Used4: %d\n", (int)BipBuffer_UsedSize(bb));
  BipBuffer_Free(bb);

  return 0;
}
