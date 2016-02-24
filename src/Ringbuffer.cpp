#include <algorithm>

#include "Ringbuffer.h"

using namespace std;
using namespace agbplay;

/*
 * public Ringbuffer
 */

Ringbuffer::Ringbuffer(uint32_t elementCount)
    : bufData(elementCount)
{
    freePos = 0;
    dataPos = 0;
    freeCount = elementCount;
    dataCount = 0;
}

Ringbuffer::~Ringbuffer()
{
}

void Ringbuffer::Put(float *inData, uint32_t nElements)
{
    boost::mutex::scoped_lock lock(countLock);
    while (freeCount < nElements){
        sig.wait(lock);
    }
    while (nElements > 0) {
        uint32_t count = put(inData, nElements);
        inData += count;
        nElements -= count;
    }
}

void Ringbuffer::Take(float *outData, uint32_t nElements)
{
    if (dataCount < nElements) {
        // underrun
        fill(outData, outData + nElements, 0.0f);
    } else {
        // output
        boost::mutex::scoped_lock lock(countLock);
        while (nElements > 0) {
            uint32_t count = take(outData, nElements);
            outData += count;
            nElements -= count;
        }
        sig.notify_one();
    }
}

void Ringbuffer::Flush()
{
    boost::mutex::scoped_lock lock(countLock);
    fill(bufData.begin(), bufData.end(), 0.0f);
}

/*
 * private Ringbuffer
 */

uint32_t Ringbuffer::put(float *inData, uint32_t nElements)
{
    bool wrap = nElements > bufData.size() - freePos;
    uint32_t count;
    uint32_t newfree;
    if (wrap) {
        count = uint32_t(bufData.size() - freePos);
        newfree = 0;
    } else {
        count = nElements;
        newfree = freePos + count;
    }
    copy(inData, inData + count, &bufData[freePos]);
    freePos = newfree;
    freeCount -= count;
    dataCount += count;
    return count;
}

uint32_t Ringbuffer::take(float *outData, uint32_t nElements)
{
    bool wrap = nElements > bufData.size() - dataPos;
    uint32_t count;
    uint32_t newdata;
    if (wrap) {
        count = uint32_t(bufData.size() - dataPos);
        newdata = 0;
    } else {
        count = nElements;
        newdata = dataPos + count;
    }
    copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
    dataPos = newdata;
    freeCount += count;
    dataCount -= count;
    return count;
}
