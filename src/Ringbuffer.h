#pragma once

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <vector>
#include <cstdint>

namespace agbplay
{
    class Ringbuffer
    {
        public:
            Ringbuffer(uint32_t elementCount);
            ~Ringbuffer();

            void Put(float *inData, uint32_t nElements);
            void Take(float *outData, uint32_t nElements);
            void Clear();
        private:
            uint32_t put(float *inData, uint32_t nElements);
            uint32_t take(float *outData, uint32_t nElements);

            std::vector<float> bufData;
            boost::mutex countLock;
            boost::condition_variable sig;
            uint32_t freePos;
            uint32_t dataPos;
            uint32_t freeCount;
            uint32_t dataCount;
    };
}
