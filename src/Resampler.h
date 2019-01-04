#pragma once

#include <vector>

/* 
 * res_data_fetch_cb fetches samplesRequired samples to fetchBuffer
 * so that the buffer can provide exactly samplesRequired samples
 *
 * returns false in case of 'end of stream'
 */
typedef bool (*res_data_fetch_cb)(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);

class Resampler {
public:
    // return value false by Process signals the "end of stream"
    virtual bool Process(float *outData, size_t numBlocks, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) = 0;
    virtual ~Resampler();
protected:
    Resampler();
    std::vector<float> fetchBuffer;
    float phase;
};

class NearestResampler : public Resampler {
public:
    NearestResampler();
    ~NearestResampler() override;
    bool Process(float *outData, size_t numBlocks, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
};

class LinearResampler : public Resampler {
public:
    LinearResampler();
    ~LinearResampler() override;
    bool Process(float *outData, size_t numBlocks, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
};
