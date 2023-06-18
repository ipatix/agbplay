#pragma once

#include <vector>

#include "SequenceReader.h"
#include "SoundMixer.h"
#include <rtmidi/RtMidi.h>

/* Instead of defining lots of global objects, we define
 * a context with all the things we need. So anything which
 * needs anything out of this context only needs a reference
 * to a PlayerContext */

struct PlayerContext {
    PlayerContext(const PlayerContext&) = delete;
    PlayerContext& operator=(const PlayerContext&) = delete;
    PlayerContext(
            int8_t maxLoops, uint8_t maxTracks, EnginePars pars,
            RtMidiIn *midiin);

    void Process(std::vector<std::vector<sample>>& trackAudio);
    void ProcessMidi();
    void InitSong(size_t songPos, bool liveMode = false);
    bool HasEnded() const;
    void KillAllChannels();

    SequenceReader reader;
    SoundMixer mixer;
    Sequence seq;
    SoundBank bnk;
    EnginePars pars;
    RtMidiIn *midiin;

    // sound channels
    std::list<SoundChannel> sndChannels;
    std::list<SquareChannel> sq1Channels;
    std::list<SquareChannel> sq2Channels;
    std::list<WaveChannel> waveChannels;
    std::list<NoiseChannel> noiseChannels;
};
