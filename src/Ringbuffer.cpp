#include <algorithm>

#include "Ringbuffer.h"

/*
 * public Ringbuffer
 */

Ringbuffer::Ringbuffer(size_t elementCount)
    : bufData(elementCount), freeCount(elementCount)
{
}

void Ringbuffer::Put(sample *inData, size_t nElements)
{
    std::unique_lock<std::mutex> lock(countLock);
    while (freeCount < nElements){
        sig.wait(lock);
    }
    while (nElements > 0) {
        size_t count = putChunk(inData, nElements);
        inData += count;
        nElements -= count;
    }
}

void Ringbuffer::Take(sample *outData, size_t nElements)
{
    if (dataCount < nElements) {
        // underrun
        std::fill(outData, outData + nElements, sample{0.0f, 0.0f});
    } else {
        // output
        std::unique_lock<std::mutex> lock(countLock);
        while (nElements > 0) {
            size_t count = takeChunk(outData, nElements);
            outData += count;
            nElements -= count;
        }
        sig.notify_one();
    }
}

void Ringbuffer::Clear()
{
    std::unique_lock<std::mutex> lock(countLock);
    std::fill(bufData.begin(), bufData.end(), sample{0.0f, 0.0f});
    freePos = 0;
    dataPos = 0;
    freeCount = bufData.size();
    dataCount = 0;
}

/*
 * private Ringbuffer
 */

size_t Ringbuffer::putChunk(sample *inData, size_t nElements)
{
    bool wrap = nElements >= bufData.size() - freePos;
    size_t count;
    size_t newfree;
    if (wrap) {
        count = size_t(bufData.size() - freePos);
        newfree = 0;
    } else {
        count = nElements;
        newfree = freePos + count;
    }
    std::copy(inData, inData + count, &bufData[freePos]);
    freePos = newfree;
    freeCount -= count;
    dataCount += count;
    return count;
}

size_t Ringbuffer::takeChunk(sample *outData, size_t nElements)
{
    bool wrap = nElements >= bufData.size() - dataPos;
    size_t count;
    size_t newdata;
    if (wrap) {
        count = size_t(bufData.size() - dataPos);
        newdata = 0;
    } else {
        count = nElements;
        newdata = dataPos + count;
    }
    std::copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
    dataPos = newdata;
    freeCount += count;
    dataCount -= count;
    return count;
}
