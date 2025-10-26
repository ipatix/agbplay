#include "LowLatencyRingbuffer.hpp"

#include <algorithm>
#include <cassert>

#include "Debug.hpp"

LowLatencyRingbuffer::LowLatencyRingbuffer()
{
    Reset();
}

void LowLatencyRingbuffer::Reset()
{
    std::unique_lock l(mtx);
    dataCount = 0;
    dataPos = 0;
    freeCount = buffer.size();
    freePos = 0;
}

void LowLatencyRingbuffer::SetNumBuffers(size_t numBuffers)
{
    std::unique_lock l(mtx);

    /* Cannot buffer nothing. There must be always one read chunk in the ringbuffer. */
    if (numBuffers == 0)
        numBuffers = 1;
    this->numBuffers = numBuffers;
}

void LowLatencyRingbuffer::Put(std::span<sample> inBuffer)
{
    lastPut = inBuffer.size();
    std::unique_lock l(mtx);

    /* Increase buffer size if last put an last take can't fit in the buffer.
     * This avoids a stuck ringbuffer when the input/output chunks become to large. */
    size_t bufferedNumBuffers = numBuffers;
    size_t bufferedLastTake = lastTake;
    size_t requiredBufferSize = bufferedNumBuffers * bufferedLastTake + lastPut;

    if (buffer.size() < requiredBufferSize)
        IncreaseBufferSize(requiredBufferSize);

    /* Wait as long there is still data in the ringbuffer and the receiver is able to read
     * at least an entire chunk (or multiples). */
    while (dataCount > bufferedNumBuffers * bufferedLastTake) {
        cv.wait(l);
        bufferedNumBuffers = numBuffers;
        bufferedLastTake = lastTake;
        requiredBufferSize = bufferedNumBuffers * bufferedLastTake + lastPut;
        if (buffer.size() < requiredBufferSize)
            IncreaseBufferSize(requiredBufferSize);
    }

    while (inBuffer.size() > 0) {
        const size_t elementsPut = PutSome(inBuffer);
        inBuffer = inBuffer.subspan(elementsPut);
    }
}

void LowLatencyRingbuffer::Take(std::span<sample> outBuffer)
{
    lastTake = outBuffer.size();

    /* I know this isn't entirely realtime safe, but it's better than nothing... */
    std::unique_lock l(mtx, std::try_to_lock);
    if (!l.owns_lock() || outBuffer.size() > dataCount) {
        std::fill(outBuffer.begin(), outBuffer.end(), sample{0.0f, 0.0f});
        return;
    }

    while (outBuffer.size() > 0) {
        const size_t elementsTaken = TakeSome(outBuffer);
        outBuffer = outBuffer.subspan(elementsTaken);
    }

    cv.notify_one();
}

size_t LowLatencyRingbuffer::PutSome(std::span<sample> inBuffer)
{
    assert(inBuffer.size() <= freeCount);
    const bool wrap = inBuffer.size() >= (buffer.size() - freePos);

    size_t putCount;
    size_t newFreePos;
    if (wrap) {
        putCount = buffer.size() - freePos;
        newFreePos = 0;
    } else {
        putCount = inBuffer.size();
        newFreePos = freePos + inBuffer.size();
    }

    std::copy_n(inBuffer.begin(), putCount, buffer.begin() + freePos);

    freePos = newFreePos;
    assert(freeCount >= putCount);
    freeCount -= putCount;
    dataCount += putCount;
    return putCount;
}

size_t LowLatencyRingbuffer::TakeSome(std::span<sample> outBuffer)
{
    assert(outBuffer.size() <= dataCount);
    const bool wrap = outBuffer.size() >= (buffer.size() - dataPos);

    size_t takeCount;
    size_t newDataPos;
    if (wrap) {
        takeCount = buffer.size() - dataPos;
        newDataPos = 0;
    } else {
        takeCount = outBuffer.size();
        newDataPos = dataPos + outBuffer.size();
    }

    std::copy_n(buffer.begin() + dataPos, takeCount, outBuffer.begin());

    dataPos = newDataPos;
    freeCount += takeCount;
    assert(dataCount >= takeCount);
    dataCount -= takeCount;
    return takeCount;
}

void LowLatencyRingbuffer::IncreaseBufferSize(size_t requiredBufferSize)
{
    /* mtx LOCK MUST BE HELD WHEN CALLING THIS FUNCTION. */
    if (buffer.size() >= requiredBufferSize)
        return;

    /* First we have to move the data of the current buffer to the beginning to avoid
     * silence inserted at the wraparound address. This has some oveheader, but it
     * shouldn't occur often, and if it does occur, it never occurs again
     * (unless the chunk sizes increase once again). */
    std::vector<sample> backupBuffer(dataCount);
    const size_t beforeWraparoundSize = std::min<size_t>(dataCount, buffer.size() - dataPos);
    std::span<sample> beforeWraparound(buffer.begin() + dataPos, beforeWraparoundSize);
    const size_t afterWraparoundSize = dataCount - beforeWraparoundSize;
    std::span<sample> afterWraparound(buffer.begin(), afterWraparoundSize);
    std::copy_n(buffer.begin() + dataPos, beforeWraparoundSize, backupBuffer.begin());
    std::copy_n(buffer.begin(), afterWraparoundSize, backupBuffer.begin() + beforeWraparoundSize);
    assert(beforeWraparoundSize + afterWraparoundSize == dataCount);
    assert(dataCount <= requiredBufferSize);

    /* resize buffer and restore data */
    buffer.resize(requiredBufferSize);
    std::copy_n(backupBuffer.begin(), dataCount, buffer.begin());
    std::fill(buffer.begin() + dataCount, buffer.end(), sample{0.0f, 0.0f});
    // dataCount remains unchanged
    dataPos = 0;
    freeCount = buffer.size() - dataCount;
    freePos = dataCount;
}
