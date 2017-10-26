// a very thin wrapper around the BipBuffer code

#include "bipbuffer.hh"

#include "BipBuffer.h"

#include <exception>
#include <stdexcept>
#include <cassert>

bipbuffer::bipbuffer(const size_t size) {
  bb = BipBuffer_New(size);
  if(!bb)
    throw std::bad_alloc();
}

bipbuffer::~bipbuffer() {
  assert(bb);
  BipBuffer_Free(bb);
  bb = 0;
}

bipbuffer::bipbuffer(const bipbuffer& other) {
  if(bb)
    BipBuffer_Free(bb);
  bb = other.bb;
}

void bipbuffer::clear() {
  BipBuffer_Clear(bb);
}

bool bipbuffer::grow(const size_t size) {
  return (bool)BipBuffer_Grow(bb, size);
}

size_t bipbuffer::usedSize() const {
  return BipBuffer_UsedSize(bb);
}

size_t bipbuffer::bufferSize() const {
  return BipBuffer_BufferSize(bb);
}

void* bipbuffer::writeTryReserve(const size_t size, size_t &reserved) {
  return (void*)BipBuffer_WriteTryReserve(bb, size, &reserved);
}

void* bipbuffer::writeReserve(const size_t size) {
  return (void*)BipBuffer_WriteReserve(bb, size);
}

void bipbuffer::writeCommit(const size_t size) {
  BipBuffer_WriteCommit(bb, size);
}

int bipbuffer::write(void const * const data, const size_t size) {
  BipBuffer_Write(bb, (BYTE*)data, size);
}

void* bipbuffer::readTryReserve(const size_t size, size_t &reserved) {
  return (void*)BipBuffer_ReadTryReserve(bb, size, &reserved);
}

void* bipbuffer::readReserve(const size_t size) {
  return (void*)BipBuffer_ReadReserve(bb, size);
}

void bipbuffer::readCommit(const size_t size) {
  BipBuffer_ReadCommit(bb, size);
}

int bipbuffer::read(void * const data, const size_t size) {
  BipBuffer_Read(bb, (BYTE*)data, size);
}
