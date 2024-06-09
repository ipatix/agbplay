#pragma once

#include <vector>
#include <list>

#include "LoudnessCalculator.h"
#include "SequenceReader.h"
#include "SoundMixer.h"
#include "Rom.h"
#include "MP2KPlayer.h"

/* Instead of defining lots of global objects, we define
 * a context with all the things we need. So anything which
 * needs anything out of this context only needs a reference
 * to a MP2KContext */

struct MP2KContext {
    MP2KContext(const Rom &rom, const MP2KSoundMode &mp2kSoundMode, const AgbplaySoundMode &agbplaySoundMode, const SongTableInfo &songTableInfo);
    MP2KContext(const MP2KContext&) = delete;
    MP2KContext& operator=(const MP2KContext&) = delete;

    /* original API functions */
    void m4aSoundMain();
    void m4aSongNumStart(uint16_t songPos);
    void m4aMPlayStart(uint8_t playerIdx, size_t songPos);

    /* custom helper functions */
    void SoundClear();

    bool HasEnded() const;
    size_t GetCurInterFrame() const;
    void GetVisualizerState(MP2KVisualizerState &visualizerState);

    const Rom& rom;
    SequenceReader reader;
    SoundMixer mixer;
    MP2KPlayer player; // TODO extend for multiple music players
    MP2KSoundMode mp2kSoundMode;
    AgbplaySoundMode agbplaySoundMode;
    SongTableInfo songTableInfo;
    std::vector<uint8_t> memaccArea; // TODO, this will have to be accessible from outside for emulator support
    std::vector<sample> masterAudioBuffer;
    LoudnessCalculator masterLoudnessCalculator{5.0f};

    // sound channels
    std::list<SoundChannel> sndChannels;
    std::list<SquareChannel> sq1Channels;
    std::list<SquareChannel> sq2Channels;
    std::list<WaveChannel> waveChannels;
    std::list<NoiseChannel> noiseChannels;

    size_t curInterFrame = 0;
};
