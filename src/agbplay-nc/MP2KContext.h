#pragma once

#include <vector>

#include "SequenceReader.h"
#include "SoundMixer.h"
#include "Rom.h"
#include "MP2KPlayer.h"

/* Instead of defining lots of global objects, we define
 * a context with all the things we need. So anything which
 * needs anything out of this context only needs a reference
 * to a MP2KContext */

struct MP2KContext {
    MP2KContext(const Rom &rom, const MP2KSoundMode &soundMode, const AgbplayMixingOptions &mixingOptions);
    MP2KContext(const MP2KContext&) = delete;
    MP2KContext& operator=(const MP2KContext&) = delete;

    void Process(std::vector<std::vector<sample>>& trackAudio);
    void InitSong(size_t songPos);
    bool HasEnded() const;
    size_t GetCurInterFrame() const;

    const Rom& rom;
    SequenceReader reader;
    SoundMixer mixer;
    MP2KPlayer player; // TODO extend for multiple music players
    MP2KSoundMode soundMode;
    AgbplayMixingOptions mixingOptions;

    // sound channels
    std::list<SoundChannel> sndChannels;
    std::list<SquareChannel> sq1Channels;
    std::list<SquareChannel> sq2Channels;
    std::list<WaveChannel> waveChannels;
    std::list<NoiseChannel> noiseChannels;

    size_t curInterFrame = 0;
};
