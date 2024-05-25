#pragma once

#include <memory>

#include "Types.h"
#include "Resampler.h"

struct MP2KChn {
    MP2KChn(const Note &note, const ADSR &env);
    MP2KChn(const MP2KChn &) = delete;
    MP2KChn& operator=(const MP2KChn&) = delete;

    std::unique_ptr<Resampler> rs;

    Note note;
    ADSR env;
    EnvState envState = EnvState::INIT;

    uint32_t pos = 0;
    float interPos = 0.0f;
    float freq = 0.0f;

    bool stop = false;
};
