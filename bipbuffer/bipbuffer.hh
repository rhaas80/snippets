// a very thin wrapper around the BitBuffer code

#include <stddef.h>

struct _wBipBuffer;

// all of these are exactly equivalent to their BipBuffer_XXX equivs
class bipbuffer
{
  public:
    bipbuffer(const size_t size);
    ~bipbuffer();
    bipbuffer(const bipbuffer& other);

    void clear();
    bool grow(const size_t size);
    size_t usedSize() const;
    size_t bufferSize() const;

    void* writeTryReserve(const size_t size, size_t &reserved);
    void* writeReserve(const size_t size);
    void writeCommit(const size_t size);
    int write(void const * const data, const size_t size);

    void* readTryReserve(const size_t size, size_t &reserved);
    void* readReserve(const size_t size);
    void readCommit(const size_t size);
    int read(void * const data, const size_t size);

  private:
    _wBipBuffer *bb;
};
