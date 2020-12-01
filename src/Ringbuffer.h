#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <cstddef>

class Ringbuffer
{
public:
    Ringbuffer(size_t elementCount);
    Ringbuffer(const Ringbuffer&) = delete;
    Ringbuffer& operator=(const Ringbuffer&) = delete;

    void Put(float *inData, size_t nElements);
    void Take(float *outData, size_t nElements);
    void Clear();
private:
    size_t put(float *inData, size_t nElements);
    size_t take(float *outData, size_t nElements);

    std::vector<float> bufData;
    std::mutex countLock;
    std::condition_variable sig;
    size_t freePos;
    size_t dataPos;
    size_t freeCount;
    size_t dataCount;
};
