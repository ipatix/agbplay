#pragma once

#include <vector>
#include <span>

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
    virtual bool Process(std::span<float> buffer, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) = 0;
    virtual void Reset() = 0;
    virtual ~Resampler();
    static bool ResamplerChainSampleFetchCB(std::vector<float>& fetchBuffer, size_t samplesRequired, void *cbdata);
    struct ResamplerChainData {
        // pointer to access our object
        Resampler *_this;
        // parameters for process method
        float phaseInc;
        res_data_fetch_cb cbPtr;
        void *cbdata;
    };
protected:
    std::vector<float> fetchBuffer;
    float phase;
};

class NearestResampler : public Resampler {
public:
    NearestResampler();
    ~NearestResampler() override;
    bool Process(std::span<float> buffer, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
    void Reset() override;
};

class LinearResampler : public Resampler {
public:
    LinearResampler();
    ~LinearResampler() override;
    bool Process(std::span<float> buffer, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
    void Reset() override;
};

class SincResampler : public Resampler {
public:
    SincResampler();
    ~SincResampler() override;
    bool Process(std::span<float> buffer, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
    void Reset() override;
private:
    static float fast_sinf(float t);
    static float fast_cosf(float t);
    static float fast_sincf(float t);
    static float window_func(float t);
};

class BlepResampler : public Resampler {
public:
    BlepResampler();
    ~BlepResampler() override;
    bool Process(std::span<float> buffer, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
    void Reset() override;
private:
    static float fast_Si(float t);
};

class BlampResampler : public Resampler {
public:
    BlampResampler();
    ~BlampResampler() override;
    bool Process(std::span<float> buffer, float phaseInc, res_data_fetch_cb cbPtr, void *cbdata) override;
    void Reset() override;
private:
    static float fast_Ti(float t);
};
