#pragma once

#include "Types.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <span>
#include <vector>

/* This Ringbuffer dynamically adjusts its size to only buffer as much data
 * as both reader and writer need for minimum amount of buffering.
 * SetNumBuffers (default=1) can be used to create an additional safety margin
 * if low latency is not stable. */

class LowLatencyRingbuffer
{
public:
    LowLatencyRingbuffer();
    LowLatencyRingbuffer(const LowLatencyRingbuffer &) = delete;
    LowLatencyRingbuffer &operator=(const LowLatencyRingbuffer &) = delete;

    void Reset();
    void SetNumBuffers(size_t numBuffers);

    void Put(std::span<sample> inBuffer);
    void Take(std::span<sample> outBuffer);

private:
    size_t PutSome(std::span<sample> inBuffer);
    size_t TakeSome(std::span<sample> outBuffer);
    void IncreaseBufferSize(size_t requiredBufferSize);

    std::mutex mtx;
    std::condition_variable cv;
    std::vector<sample> buffer;
    size_t freePos = 0;
    size_t freeCount = 0;
    size_t dataPos = 0;
    size_t dataCount = 0;
    std::atomic<size_t> lastTake = 0;    // this variable may be read without holding a lock
    size_t lastPut = 0;
    size_t numBuffers = 1;
};
