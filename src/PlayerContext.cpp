#include "PlayerContext.h"
#include "ConfigManager.h"

PlayerContext::PlayerContext(int8_t maxLoops, uint8_t maxTracks, EnginePars pars)
    : reader(*this, maxLoops), mixer(*this, STREAM_SAMPLERATE, 1.0f), seq(maxTracks), pars(pars)
{
}

void PlayerContext::Process(std::vector<std::vector<sample>>& trackAudio)
{
    reader.Process();
    mixer.Process(trackAudio);
}

void PlayerContext::InitSong(size_t songHeaderPos)
{
    GameConfig& cfg = ConfigManager::Instance().GetCfg();

    sndChannels.clear();
    sq1Channels.clear();
    sq2Channels.clear();
    waveChannels.clear();
    noiseChannels.clear();

    seq.Init(songHeaderPos);
    bnk.Init(seq.GetSoundBankPos());
    reader.Restart();
    mixer.ResetFade();

    uint32_t fixedModeRate = reader.freqLut.at(cfg.GetEngineFreq() - 1);
    uint8_t reverb = 0;
    if (seq.GetReverb() & 0x80)
        reverb = seq.GetReverb() & 0x7F;
    else if (cfg.GetEngineRev() & 0x80)
        reverb = cfg.GetEngineRev() & 0x7F;
    float pcmMasterVolume = static_cast<float>(cfg.GetPCMVol() + 1) / 16.0f;
    auto reverbType = cfg.GetRevType();
    uint8_t numTracks = static_cast<uint8_t>(seq.tracks.size());

    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, reverbType, numTracks);
}

bool PlayerContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}
