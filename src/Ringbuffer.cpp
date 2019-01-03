#include <algorithm>

#include "Ringbuffer.h"

using namespace std;
using namespace agbplay;

/*
 * public Ringbuffer
 */

Ringbuffer::Ringbuffer(size_t elementCount)
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

void Ringbuffer::Put(float *inData, size_t nElements)
{
    unique_lock<mutex> lock(countLock);
    while (freeCount < nElements){
        sig.wait(lock);
    }
    while (nElements > 0) {
        size_t count = put(inData, nElements);
        inData += count;
        nElements -= count;
    }
}

void Ringbuffer::Take(float *outData, size_t nElements)
{
    if (dataCount < nElements) {
        // underrun
        fill(outData, outData + nElements, 0.0f);
    } else {
        // output
        unique_lock<mutex> lock(countLock);
        while (nElements > 0) {
            size_t count = take(outData, nElements);
            outData += count;
            nElements -= count;
        }
        sig.notify_one();
    }
}

void Ringbuffer::Clear()
{
    unique_lock<mutex> lock(countLock);
    fill(bufData.begin(), bufData.end(), 0.0f);
}

/*
 * private Ringbuffer
 */

size_t Ringbuffer::put(float *inData, size_t nElements)
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
    copy(inData, inData + count, &bufData[freePos]);
    freePos = newfree;
    freeCount -= count;
    dataCount += count;
    return count;
}

size_t Ringbuffer::take(float *outData, size_t nElements)
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
    copy(&bufData[dataPos], &bufData[dataPos] + count, outData);
    dataPos = newdata;
    freeCount += count;
    dataCount -= count;
    return count;
}
