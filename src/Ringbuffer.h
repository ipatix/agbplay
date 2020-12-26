#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstddef>

#include "Types.h"

class Ringbuffer
{
public:
    Ringbuffer(size_t elementCount);
    Ringbuffer(const Ringbuffer&) = delete;
    Ringbuffer& operator=(const Ringbuffer&) = delete;

    void Put(sample *inData, size_t nElements);
    void Take(sample *outData, size_t nElements);
    void Clear();
private:
    size_t putChunk(sample *inData, size_t nElements);
    size_t takeChunk(sample *outData, size_t nElements);

    std::vector<sample> bufData;
    std::mutex countLock;
    std::condition_variable sig;
    size_t freePos = 0;
    size_t dataPos = 0;
    size_t freeCount;
    size_t dataCount = 0;
};
