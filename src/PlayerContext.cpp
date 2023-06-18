#include "PlayerContext.h"
#include "ConfigManager.h"
#include "Debug.h"

PlayerContext::PlayerContext(
        int8_t maxLoops, uint8_t maxTracks, EnginePars pars, RtMidiIn *midiin)
    : reader(*this, maxLoops), mixer(*this, STREAM_SAMPLERATE, 1.0f),
      seq(maxTracks), pars(pars), midiin(midiin)
{
}

void PlayerContext::Process(std::vector<std::vector<sample>>& trackAudio)
{
    reader.Process(midiin != nullptr);
    mixer.Process(trackAudio);
}

void PlayerContext::ProcessMidi()
{
    if (midiin == nullptr) {
        return;
    }
    std::vector<unsigned char> message;
    int nBytes;

    while (true) {
        double stamp = midiin->getMessage(&message);
        nBytes = message.size();
        if (nBytes == 0) {
            break;
        }
        uint8_t status = message[0];
        uint8_t uarg = message[1];
        uint8_t uarg2 = message[2];
        int8_t sarg = static_cast<int8_t>(uarg);
        int8_t sarg2 = static_cast<int8_t>(uarg2);

        if (status < 0x80) {
            // invalid status
            continue;
        }
        // tempo
        if (status == 0xFF && message[1] == 0x51) {
            uint32_t mspt = (message[3] << 16) | (message[4] << 8) | message[5];
            uint8_t bpm = static_cast<uint8_t>(60000000.0f / mspt / 2.0f);
            reader.PlayLiveCommand(0xBB, bpm, 0, 0);
            continue;
        }
        if (status >= 0xF0) {
            // todo: parse meta / sysex
            continue;
        }
        uint8_t channel = status & 0x0F;
        switch (status & 0xF0) {
        case 0x80:    // note off
            reader.PlayLiveNoteOff(message[1], channel);
            continue;
        case 0x90:    // note on
            if (message[2] == 0) {
                reader.PlayLiveNoteOff(message[1], channel);
            } else {
                // Debug::print(
                //         "note on %02X %02X %02X", message[0], message[1],
                //         message[2]);

                reader.PlayLiveNoteOn(message[1], message[2], channel);
            }
            continue;
        case 0xA0:    // polyphonic aftertouch
            continue;
        case 0xB0:    // control change
            switch (message[1]) {
            case 7:    // volume
                reader.PlayLiveCommand(0xBE, uarg2, 0, channel);
                continue;
            case 10:    // pan
                reader.PlayLiveCommand(0xBF, 0, sarg2, channel);
                continue;
            case 20:    // bend range
                reader.PlayLiveCommand(0xC1, uarg2, 0, channel);
                continue;
            case 21:    // lfo speed
                reader.PlayLiveCommand(0xC2, uarg2, 0, channel);
                continue;
            case 26:    // lfo delay
                reader.PlayLiveCommand(0xC3, uarg2, 0, channel);
                continue;
            case 1:    // mod depth
                reader.PlayLiveCommand(0xC4, uarg2, 0, channel);
                continue;
            case 22:    // mod type
                reader.PlayLiveCommand(0xC5, uarg2, 0, channel);
                continue;
            case 24:    // tune
                reader.PlayLiveCommand(0xC8, 0, sarg2, channel);
                continue;
            case 33:    // priority
                reader.PlayLiveCommand(0xBA, uarg2, 0, channel);
                continue;
            case 123:    // all notes off
                KillAllChannels();
                continue;
            }
            continue;
        case 0xC0:    // program change
            // Debug::print("IN %d: program change %d", channel, message[1]);
            reader.PlayLiveCommand(0xBD, uarg, 0, channel);
            continue;
        case 0xD0:    // channel aftertouch
            continue;
        case 0xE0:    // pitch bend
            reader.PlayLiveCommand(0xC0, 0, sarg2, channel);
            continue;
        }
    }
}

void PlayerContext::InitSong(size_t songHeaderPos, bool liveMode)
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

    if (liveMode) {
        // need to ensure 16 tracks
        while (seq.tracks.size() < 16) {
            seq.tracks.push_back(Track(0));
        }
    }

    uint8_t numTracks = static_cast<uint8_t>(seq.tracks.size());

    mixer.Init(fixedModeRate, reverb, pcmMasterVolume, reverbType, numTracks);
}

bool PlayerContext::HasEnded() const
{
    return reader.EndReached() && mixer.IsFadeDone();
}

void PlayerContext::KillAllChannels()
{
    sndChannels.clear();
    sq1Channels.clear();
    sq2Channels.clear();
    waveChannels.clear();
    noiseChannels.clear();
}