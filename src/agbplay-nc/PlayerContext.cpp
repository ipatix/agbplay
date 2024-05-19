#include "PlayerContext.h"
#include "ConfigManager.h"

PlayerContext::PlayerContext(const Rom &rom, const MP2KSoundMode &soundMode, const AgbplayMixingOptions &mixingOptions)
    : rom(rom), reader(*this), mixer(*this, STREAM_SAMPLERATE, 1.0f), seq(mixingOptions.trackLimit), soundMode(soundMode), mixingOptions(mixingOptions)
{
}

void PlayerContext::Process(std::vector<std::vector<sample>>& trackAudio)
{
    reader.Process();
    mixer.Process(trackAudio);
    curInterFrame++;
}

void PlayerContext::InitSong(size_t songHeaderPos)
{
    GameConfig& cfg = ConfigManager::Instance().GetCfg();

    sndChannels.clear();
    sq1Channels.clear();
    sq2Channels.clear();
    waveChannels.clear();
    noiseChannels.clear();

    curInterFrame = 0;
    seq.Init(songHeaderPos);
    bnk.Init(seq.GetSoundBankPos());
    reader.Restart();
    mixer.ResetFade();

    uint32_t fixedModeRate = reader.freqLut.at(soundMode.freq - 1);
    uint8_t reverb = 0;
    if (seq.GetReverb() & 0x80)
        reverb = seq.GetReverb() & 0x7F;
    else if (soundMode.rev & 0x80)
        reverb = soundMode.rev & 0x7F;
    float pcmMasterVolume = static_cast<float>(soundMode.vol + 1) / 16.0f;
    auto reverbType = cfg.GetRevType();
    uint8_t numTracks = static_cast<uint8_t>(seq.tracks.size());

    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, reverbType, numTracks);
}

bool PlayerContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}

size_t PlayerContext::GetCurInterFrame() const
{
    return curInterFrame;
}
