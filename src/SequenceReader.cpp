#include <cmath>
#include <cassert>

#include "SequenceReader.h"
#include "Xcept.h"
#include "Util.h"
#include "Debug.h"
#include "Rom.h"
#include "PlayerContext.h"
#include "ConfigManager.h"

#define NOTE_TIE -1
#define NOTE_ALL 0xFE
#define LOOP_ENDLESS -1

/*
 * SequenceReader data
 */

const std::vector<uint32_t> SequenceReader::freqLut = {
    5734, 7884, 10512, 13379,
    15768, 18157, 21024, 26758,
    31536, 36314, 40137, 42048
};

const std::map<uint8_t, int8_t> SequenceReader::delayLut = {
    {0x80,0 },
    {0x81,1 }, {0x82,2 }, {0x83,3 }, {0x84,4 }, {0x85,5 }, {0x86,6 }, {0x87,7 }, {0x88,8 },
    {0x89,9 }, {0x8A,10}, {0x8B,11}, {0x8C,12}, {0x8D,13}, {0x8E,14}, {0x8F,15}, {0x90,16},
    {0x91,17}, {0x92,18}, {0x93,19}, {0x94,20}, {0x95,21}, {0x96,22}, {0x97,23}, {0x98,24},
    {0x99,28}, {0x9A,30}, {0x9B,32}, {0x9C,36}, {0x9D,40}, {0x9E,42}, {0x9F,44}, {0xA0,48},
    {0xA1,52}, {0xA2,54}, {0xA3,56}, {0xA4,60}, {0xA5,64}, {0xA6,66}, {0xA7,68}, {0xA8,72},
    {0xA9,76}, {0xAA,78}, {0xAB,80}, {0xAC,84}, {0xAD,88}, {0xAE,90}, {0xAF,92}, {0xB0,96}
};

const std::map<uint8_t, int8_t> SequenceReader::noteLut = {
    {0xCF,0 },
    {0xD0,1 }, {0xD1,2 }, {0xD2,3 }, {0xD3,4 }, {0xD4,5 }, {0xD5,6 }, {0xD6,7 }, {0xD7,8 },
    {0xD8,9 }, {0xD9,10}, {0xDA,11}, {0xDB,12}, {0xDC,13}, {0xDD,14}, {0xDE,15}, {0xDF,16},
    {0xE0,17}, {0xE1,18}, {0xE2,19}, {0xE3,20}, {0xE4,21}, {0xE5,22}, {0xE6,23}, {0xE7,24},
    {0xE8,28}, {0xE9,30}, {0xEA,32}, {0xEB,36}, {0xEC,40}, {0xED,42}, {0xEE,44}, {0xEF,48},
    {0xF0,52}, {0xF1,54}, {0xF2,56}, {0xF3,60}, {0xF4,64}, {0xF5,66}, {0xF6,68}, {0xF7,72},
    {0xF8,76}, {0xF9,78}, {0xFA,80}, {0xFB,84}, {0xFC,88}, {0xFD,90}, {0xFE,92}, {0xFF,96}
};

/*
 * public SequenceReader
 */

SequenceReader::SequenceReader(PlayerContext& ctx, int8_t maxLoops) 
    : ctx(ctx), maxLoops(maxLoops)
{
}

void SequenceReader::Process(bool liveMode)
{
    ctx.seq.bpmStack += uint32_t(float(ctx.seq.bpm) * speedFactor);
    while (ctx.seq.bpmStack >= BPM_PER_FRAME * INTERFRAMES) {
        processSequenceTick(liveMode);
        ctx.seq.bpmStack -= BPM_PER_FRAME * INTERFRAMES;
    }
}

bool SequenceReader::EndReached() const
{
    return endReached;
}

void SequenceReader::Restart()
{
    liveNotes.clear();
    liveNoteID = 0;
    numLoops = 0;
    endReached = false;
}

void SequenceReader::SetSpeedFactor(float speedFactor)
{
    this->speedFactor = speedFactor;
}

/*
 * private SequenceReader
 */

void SequenceReader::processSequenceTick(bool liveMode)
{
    Rom& rom = Rom::Instance();
    // process all tracks
    bool isSongRunning = false;
    int itrk = -1;
    for (Track& trk : ctx.seq.tracks) {
        itrk += 1;
        const uint8_t trackIdx = static_cast<uint8_t>(itrk);
        if (!trk.isRunning)
            continue;

        isSongRunning = true;
        tickTrackNotes(uint8_t(itrk), trk.activeNotes);

        if (liveMode) {
            ctx.ProcessMidi();
        } else {
            // count down last delay and process
            while (trk.isRunning && trk.delay == 0) {
                uint8_t cmd = rom.ReadU8(trk.pos);

                // check if a previous command should be repeated
                if (cmd < 0x80) {
                    cmd = trk.lastCmd;
                    if (cmd < 0x80) {
                        // song data error, command not initialized
                        cmdPlayFine(trackIdx);
                        break;
                    }
                } else {
                    trk.pos++;
                    if (cmd >= 0xBD) {
                        // repeatable command
                        trk.lastCmd = cmd;
                    }
                }

                if (cmd >= 0xCF) {
                    // note command
                    cmdPlayNote(cmd, trackIdx);
                } else if (cmd >= 0xB1) {
                    // state altering command
                    cmdPlayCommand(cmd, trackIdx);
                } else {
                    trk.delay = delayLut.at(cmd);
                }
            }    // end of processing loop
        }

        if (!liveMode && trk.isRunning)
            trk.delay--;

        if (trk.lfos != 0 && trk.mod != 0) {
            if (trk.lfodlCount == 0) {
                trk.lfoPhase += trk.lfos;
                int lfoPoint;
                if (static_cast<int8_t>(trk.lfoPhase - 64) >= 0)
                    lfoPoint = 128 - trk.lfoPhase;
                else
                    lfoPoint = static_cast<int8_t>(trk.lfoPhase);
                lfoPoint *= trk.mod;
                lfoPoint >>= 6;

                if (trk.lfoValue != lfoPoint) {
                    trk.lfoValue = static_cast<int8_t>(lfoPoint);
                    if (trk.modt == MODT::PITCH)
                        trk.updatePitch = true;
                    else
                        trk.updateVolume = true;
                }
            } else {
                trk.lfodlCount--;
            }
        }

        // TODO actually only update one or there other and not always both
        if (trk.updateVolume || trk.updatePitch || trk.mod > 0) {
            setTrackPV(trackIdx, 
                    trk.GetVol(),
                    trk.GetPan(),
                    trk.pitch = trk.GetPitch());
            trk.updateVolume = false;
            trk.updatePitch = false;
        } else {
            trk.pitch = trk.GetPitch();
        }
    }

    if (!isSongRunning && !endReached) {
        ctx.mixer.StartFadeOut(SONG_FINISH_TIME);
        endReached = true;
    }

    ctx.seq.tickCount++;
}

int SequenceReader::tickTrackNotes(uint8_t track_idx, std::bitset<NUM_NOTES>& activeNotes)
{
    std::bitset<128> backBuffer;
    int active = 0;

    auto tickFunc = [&](auto& channels) {
        for (auto& chn : channels) {
            if (chn.GetTrackIdx() == track_idx) {
                if (chn.TickNote()) {
                    active++;
                    backBuffer[chn.GetNote().midiKeyTrackData % 128] = true;
                }
            }
        }
    };

    tickFunc(ctx.sndChannels);
    tickFunc(ctx.sq1Channels);
    tickFunc(ctx.sq2Channels);
    tickFunc(ctx.waveChannels);
    tickFunc(ctx.noiseChannels);

    activeNotes = backBuffer;
    return active;
}

void SequenceReader::setTrackPV(uint8_t track_idx, uint8_t vol, int8_t pan, int16_t pitch)
{
    auto setFunc = [&](auto& channels) {
        for (auto& chn : channels) {
            if (chn.GetTrackIdx() == track_idx) {
                chn.SetVol(vol, pan);
                chn.SetPitch(pitch);
            }
        }
    };

    setFunc(ctx.sndChannels);
    setFunc(ctx.sq1Channels);
    setFunc(ctx.sq2Channels);
    setFunc(ctx.waveChannels);
    setFunc(ctx.noiseChannels);
}

void SequenceReader::PlayLiveNoteOn(uint8_t key, uint8_t vel, uint8_t trackIdx)
{
    enqueueNote(key, vel, 0, trackIdx, true);
}

void SequenceReader::PlayLiveNoteOff(uint8_t key, uint8_t trackIdx)
{
    uint32_t noteOffId = liveNotes[std::make_pair(trackIdx, key)];
    for (auto &chn : ctx.sndChannels) {
        if (chn.GetTrackIdx() == trackIdx &&
            chn.GetNote().noteId == noteOffId) {
            chn.Release();
            liveNotes.erase(std::make_pair(trackIdx, key));
            return;
        }
    }
    for (auto &chn : ctx.sq1Channels) {
        if (chn.GetTrackIdx() == trackIdx &&
            chn.GetNote().noteId == noteOffId) {
            chn.LiveRelease();
            liveNotes.erase(std::make_pair(trackIdx, key));
            return;
        }
    }
    for (auto &chn : ctx.sq2Channels) {
        if (chn.GetTrackIdx() == trackIdx &&
            chn.GetNote().noteId == noteOffId) {
            chn.LiveRelease();
            liveNotes.erase(std::make_pair(trackIdx, key));
            return;
        }
    }
    for (auto &chn : ctx.waveChannels) {
        if (chn.GetTrackIdx() == trackIdx &&
            chn.GetNote().noteId == noteOffId) {
            chn.LiveRelease();
            liveNotes.erase(std::make_pair(trackIdx, key));
            return;
        }
    }
    for (auto &chn : ctx.noiseChannels) {
        if (chn.GetTrackIdx() == trackIdx &&
            chn.GetNote().noteId == noteOffId) {
            chn.LiveRelease();
            liveNotes.erase(std::make_pair(trackIdx, key));
            return;
        }
    }
}

void SequenceReader::PlayLiveCommand(
        uint8_t cmd, uint8_t uarg, int8_t sarg, uint8_t trackIdx)
{
    Track &trk = ctx.seq.tracks[trackIdx];

    switch (cmd) {
    case 0xBA:
        // PRIO
        trk.priority = uarg;
        break;
    case 0xBB:
        // TEMPO
        ctx.seq.bpm = static_cast<uint16_t>(uarg * 2);
        break;
    case 0xBC:
        // KEYSH
        trk.keyShift = sarg;
        break;
    case 0xBD:
        // VOICE
        trk.prog = uarg;
        break;
    case 0xBE:
        // VOL
        trk.vol = uarg;
        trk.updateVolume = true;
        break;
    case 0xBF:
        // PAN
        trk.pan = static_cast<int8_t>(sarg - 0x40);
        trk.updateVolume = true;
        break;
    case 0xC0:
        // BEND
        trk.bend = static_cast<int8_t>(sarg - 0x40);
        trk.updatePitch = true;
        break;
    case 0xC1:
        // BENDR
        trk.bendr = uarg;
        trk.updatePitch = true;
        break;
    case 0xC2:
        // LFOS
        // TODO rewrite to match libagbsnd
        trk.lfos = uarg;
        if (trk.lfos == 0)
            trk.ResetLfoValue();
        break;
    case 0xC3:
        // LFODL
        // TODO rewrite to match libagbsnd
        trk.lfodlCount = trk.lfodl = uarg;
        break;
    case 0xC4:
        // MOD
        // TODO rewrite to match libagbsnd
        trk.mod = uarg;
        trk.updateVolume = true;
        trk.updatePitch = true;
        if (trk.mod == 0)
            trk.ResetLfoValue();
        break;
    case 0xC5:
        // MODT
        {
            uint8_t modt = uarg;
            if (static_cast<MODT>(modt) == trk.modt)
                return;
            trk.modt = static_cast<MODT>(modt);
            trk.updateVolume = true;
            trk.updatePitch = true;
        }
        break;
    case 0xC8:
        // TUNE
        trk.tune = static_cast<int8_t>(sarg - 0x40);
        trk.updatePitch = true;
        break;
    }

}

void SequenceReader::enqueueNote(
        uint8_t key, uint8_t vel, uint8_t len, uint8_t trackIdx, bool liveMode)
{
    Track &trk = ctx.seq.tracks[trackIdx];

    trk.lastNoteLen = len;
    trk.lastNoteKey = key;
    trk.lastNoteVel = vel;

    // don't play invalid instruments
    if (trk.prog > 127)
        return;

    trk.lfodlCount = trk.lfodl;
    if (trk.lfodl != 0)
        trk.ResetLfoValue();

    // initialize note data
    Note note;
    note.length = trk.lastNoteLen;
    note.midiKeyTrackData = trk.lastNoteKey;
    note.midiKeyPitch = ctx.bnk.GetMidiKey(trk.prog, trk.lastNoteKey);
    note.velocity = trk.lastNoteVel;
    note.priority = trk.priority;
    note.rhythmPan = ctx.bnk.GetPan(trk.prog, trk.lastNoteKey);
    note.pseudoEchoVol = trk.pseudoEchoVol;
    note.pseudoEchoLen = trk.pseudoEchoLen;
    note.trackIdx = trackIdx;
    note.noteId = 0;

    // prepare cgb polyphony suppression
    const ConfigManager& cm = ConfigManager::Instance();
    CGBPolyphony cgbPolyphony = cm.GetCgbPolyphony();

    auto cgbPolyphonySuppressFunc = [&](auto& channels) {
        // return 'true' if a note is allowed to play, 'false' if others with higher priority are playing
        if (cgbPolyphony == CGBPolyphony::MONO_STRICT) {
            // only one tone should play in mono strict mode
            assert(channels.size() <= 1);
            if (channels.size() > 0) {
                const Note& playing_note = channels.front().GetNote();

                if (!channels.front().IsReleasing()) {
                    if (playing_note.priority > note.priority)
                        return false;
                    if (playing_note.priority == note.priority) {
                        if (channels.front().GetTrackIdx() < trackIdx)
                            return false;
                    }
                }
            }
            channels.clear();
        } else if (cgbPolyphony == CGBPolyphony::MONO_SMOOTH) {
            for (auto& chn : channels) {
                if (chn.GetState() < EnvState::PSEUDO_ECHO && !chn.IsFastReleasing()) {
                    const Note& playing_note = chn.GetNote();
                    if (playing_note.priority > note.priority)
                        return false;
                    if (playing_note.priority == note.priority) {
                        if (chn.GetTrackIdx() < trackIdx)
                            return false;
                    }
                }
                chn.Release(true);
            }
        }
        return true;
    };

    // enqueue actual note
    switch (ctx.bnk.GetInstrType(trk.prog, trk.lastNoteKey)) {
    case InstrType::PCM:
        if (liveMode) {
            note.noteId = liveNoteID++;
            liveNotes[std::make_pair(trackIdx, key)] = note.noteId;
        }
        ctx.sndChannels.emplace_back(
                ctx.bnk.GetSampInfo(trk.prog, trk.lastNoteKey),
                ctx.bnk.GetADSR(trk.prog, trk.lastNoteKey), note, false);
        break;
    case InstrType::PCM_FIXED:
        if (liveMode) {
            note.noteId = liveNoteID++;
            liveNotes[std::make_pair(trackIdx, key)] = note.noteId;
        }
        ctx.sndChannels.emplace_back(
                ctx.bnk.GetSampInfo(trk.prog, trk.lastNoteKey),
                ctx.bnk.GetADSR(trk.prog, trk.lastNoteKey), note, true);
        break;
    case InstrType::SQ1:
        if (!cgbPolyphonySuppressFunc(ctx.sq1Channels))
            return;
        if (liveMode) {
            note.noteId = liveNoteID++;
            liveNotes[std::make_pair(trackIdx, key)] = note.noteId;
        }
        ctx.sq1Channels.emplace_back(
                ctx.bnk.GetCGBDef(trk.prog, trk.lastNoteKey).wd,
                ctx.bnk.GetADSR(trk.prog, trk.lastNoteKey), note,
                ctx.bnk.GetSweep(trk.prog, trk.lastNoteKey));
        break;
    case InstrType::SQ2:
        if (!cgbPolyphonySuppressFunc(ctx.sq2Channels))
            return;
        if (liveMode) {
            note.noteId = liveNoteID++;
            liveNotes[std::make_pair(trackIdx, key)] = note.noteId;
        }
        ctx.sq2Channels.emplace_back(
                ctx.bnk.GetCGBDef(trk.prog, trk.lastNoteKey).wd,
                ctx.bnk.GetADSR(trk.prog, trk.lastNoteKey), note, 0);
        break;
    case InstrType::WAVE:
        if (!cgbPolyphonySuppressFunc(ctx.waveChannels))
            return;
        if (liveMode) {
            note.noteId = liveNoteID++;
            liveNotes[std::make_pair(trackIdx, key)] = note.noteId;
        }
        ctx.waveChannels.emplace_back(
                ctx.bnk.GetCGBDef(trk.prog, trk.lastNoteKey).wavePtr,
                ctx.bnk.GetADSR(trk.prog, trk.lastNoteKey), note,
                cm.GetCfg().GetAccurateCh3Volume());
        break;
    case InstrType::NOISE:
        if (!cgbPolyphonySuppressFunc(ctx.noiseChannels))
            return;
        if (liveMode) {
            note.noteId = liveNoteID++;
            liveNotes[std::make_pair(trackIdx, key)] = note.noteId;
        }
        ctx.noiseChannels.emplace_back(
                ctx.bnk.GetCGBDef(trk.prog, trk.lastNoteKey).np,
                ctx.bnk.GetADSR(trk.prog, trk.lastNoteKey), note);
        break;
    case InstrType::INVALID: return;
    }

    // new notes need correct pitch and volume applied
    trk.updateVolume = true;
    trk.updatePitch = true;
}

void SequenceReader::cmdPlayNote(uint8_t cmd, uint8_t trackIdx)
{
    Rom &rom = Rom::Instance();
    Track &trk = ctx.seq.tracks[trackIdx];

    uint8_t len = noteLut.at(cmd);
    uint8_t key = trk.lastNoteKey;
    uint8_t vel = trk.lastNoteVel;

    if (rom.ReadU8(trk.pos) < 0x80) {
        key = rom.ReadU8(trk.pos++);

        if (rom.ReadU8(trk.pos) < 0x80) {
            vel = rom.ReadU8(trk.pos++);

            if (rom.ReadU8(trk.pos) < 0x80) {
                len += rom.ReadU8(trk.pos++);
            }
        }
    }

    enqueueNote(key, vel, len, trackIdx, false);
}

void SequenceReader::cmdPlayCommand(uint8_t cmd, uint8_t trackIdx)
{
    Rom& rom = Rom::Instance();
    Track& trk = ctx.seq.tracks[trackIdx];

    switch (cmd) {
    case 0xB1:
        // FINE
        cmdPlayFine(trackIdx);
        break;
    case 0xB2:
        // GOTO
        if (trackIdx == 0) {
            // handle agbplay's internal loop counter
            if (maxLoops != LOOP_ENDLESS && numLoops++ >= maxLoops && !endReached) {
                endReached = true;
                ctx.mixer.StartFadeOut(SONG_FADE_OUT_TIME);
            }
        }
        trk.pos = rom.ReadAgbPtrToPos(trk.pos);
        break;
    case 0xB3:
        // PATT
        if (trk.patternLevel >= TRACK_CALL_STACK_SIZE) {
            cmdPlayFine(trackIdx);
            break;
        }
        trk.returnPos[trk.patternLevel++] = trk.pos + 4;
        trk.pos = rom.ReadAgbPtrToPos(trk.pos);
        break;
    case 0xB4:
        // PEND
        if (trk.patternLevel == 0)
            break;

        trk.pos = trk.returnPos[--trk.patternLevel];
        break;
    case 0xB5:
        // REPT
        {
            uint8_t count = rom.ReadU8(trk.pos++);
            if (count == 0) {
                cmdPlayFine(trackIdx);
                break;
            }
            if (++trk.reptCount < count) {
                trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            } else {
                trk.reptCount = 0;
                trk.pos += 4;
            }
        }
        break;
    case 0xB9:
        // MEMACC
        cmdPlayMemacc(trackIdx);
        break;
    case 0xBA:
        // PRIO
        trk.priority = rom.ReadU8(trk.pos++);
        break;
    case 0xBB:
        // TEMPO
        ctx.seq.bpm = static_cast<uint16_t>(rom.ReadU8(trk.pos++) * 2);
        break;
    case 0xBC:
        // KEYSH
        trk.keyShift = rom.ReadS8(trk.pos++);
        break;
    case 0xBD:
        // VOICE
        trk.prog = rom.ReadU8(trk.pos++);
        break;
    case 0xBE:
        // VOL
        trk.vol = rom.ReadU8(trk.pos++);
        trk.updateVolume = true;
        break;
    case 0xBF:
        // PAN
        trk.pan = static_cast<int8_t>(rom.ReadS8(trk.pos++) - 0x40);
        trk.updateVolume = true;
        break;
    case 0xC0:
        // BEND
        trk.bend = static_cast<int8_t>(rom.ReadS8(trk.pos++) - 0x40);
        trk.updatePitch = true;
        break;
    case 0xC1:
        // BENDR
        trk.bendr = rom.ReadU8(trk.pos++);
        trk.updatePitch = true;
        break;
    case 0xC2:
        // LFOS
        // TODO rewrite to match libagbsnd
        trk.lfos = rom.ReadU8(trk.pos++);
        if (trk.lfos == 0)
            trk.ResetLfoValue();
        break;
    case 0xC3:
        // LFODL
        // TODO rewrite to match libagbsnd
        trk.lfodlCount = trk.lfodl = rom.ReadU8(trk.pos++);
        break;
    case 0xC4:
        // MOD
        // TODO rewrite to match libagbsnd
        trk.mod = rom.ReadU8(trk.pos++);
        trk.updateVolume = true;
        trk.updatePitch = true;
        if (trk.mod == 0)
            trk.ResetLfoValue();
        break;
    case 0xC5:
        // MODT
        {
            uint8_t modt = rom.ReadU8(trk.pos++);
            if (static_cast<MODT>(modt) == trk.modt)
                return;
            trk.modt = static_cast<MODT>(modt);
            trk.updateVolume = true;
            trk.updatePitch = true;
        }
        break;
    case 0xC8:
        // TUNE
        trk.tune = static_cast<int8_t>(rom.ReadS8(trk.pos++) - 0x40);
        trk.updatePitch = true;
        break;
    case 0xCD:
        // xCMD
        cmdPlayXCmd(trackIdx);
        break;
    case 0xCE:
        // EOT
        {
            uint8_t key = rom.ReadU8(trk.pos);
            if (key >= 0x80) {
                key = trk.lastNoteKey;
            } else {
                trk.pos++;
                trk.lastNoteKey = key;
            }
            auto stopFunc = [&](auto& channels) {
                for (auto& chn : channels) {
                    if (chn.GetTrackIdx() == trackIdx && chn.GetNote().midiKeyTrackData == key) {
                        chn.Release();
                        break;
                    }
                }
            };
            stopFunc(ctx.sndChannels);
            stopFunc(ctx.sq1Channels);
            stopFunc(ctx.sq2Channels);
            stopFunc(ctx.waveChannels);
            stopFunc(ctx.noiseChannels);
        }
        break;
    default:
        cmdPlayFine(trackIdx);
        break;
    }
}

void SequenceReader::cmdPlayFine(uint8_t trackIdx)
{
    auto stopFunc = [&](auto& channels) {
        for (auto& chn : channels) {
            if (chn.GetTrackIdx() == trackIdx)
                chn.Release();
        }
    };
    stopFunc(ctx.sndChannels);
    stopFunc(ctx.sq1Channels);
    stopFunc(ctx.sq2Channels);
    stopFunc(ctx.waveChannels);
    stopFunc(ctx.noiseChannels);
    ctx.seq.tracks[trackIdx].isRunning = false;
}

void SequenceReader::cmdPlayMemacc(uint8_t trackIdx)
{
    Rom& rom = Rom::Instance();
    Track& trk = ctx.seq.tracks[trackIdx];

    uint8_t op = rom.ReadU8(trk.pos++);
    uint8_t& memory = ctx.seq.memaccArea[rom.ReadU8(trk.pos++)];
    uint8_t data = rom.ReadU8(trk.pos++);

    switch (op) {
    case 0:
        memory = data;
        return;
    case 1:
        memory += data;
        return;
    case 2:
        memory -= data;
        return;
    case 3:
        memory = ctx.seq.memaccArea[data];
        return;
    case 4:
        memory += ctx.seq.memaccArea[data];
        return;
    case 5:
        memory -= ctx.seq.memaccArea[data];
        return;
    case 6:
        if (memory == data) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 7:
        if (memory != data) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 8:
        if (memory > data) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 9:
        if (memory >= data) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 10:
        if (memory <= data) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 11:
        if (memory < data) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 12:
        if (memory == ctx.seq.memaccArea[data]) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 13:
        if (memory != ctx.seq.memaccArea[data]) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 14:
        if (memory > ctx.seq.memaccArea[data]) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 15:
        if (memory >= ctx.seq.memaccArea[data]) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 16:
        if (memory <= ctx.seq.memaccArea[data]) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    case 17:
        if (memory < ctx.seq.memaccArea[data]) {
            trk.pos = rom.ReadAgbPtrToPos(trk.pos);
            return;
        }
        break;
    default:
        return;
    }

    // only "false" jump commands should come by here
    trk.pos += 4;
}

void SequenceReader::cmdPlayXCmd(uint8_t trackIdx)
{
    Rom& rom = Rom::Instance();
    Track& trk = ctx.seq.tracks[trackIdx];

    uint8_t xCmdNo = rom.ReadU8(trk.pos++);

    switch (xCmdNo) {
    case 0:
        // XXX
        cmdPlayFine(trackIdx);
        break;
    case 1:
        // XWAVE (stub)
        trk.pos += 4;
        break;
    case 2:
        // XTYPE (stub)
        trk.pos++;
        break;
    case 3:
        // XXX
        cmdPlayFine(trackIdx);
        break;
    case 4:
        // XATTA (stub)
        trk.pos++;
        break;
    case 5:
        // XDECA (stub)
        trk.pos++;
        break;
    case 6:
        // XSUST (stub)
        trk.pos++;
        break;
    case 7:
        // XRELA (stub)
        trk.pos++;
        break;
    case 8:
        // XIECV
        trk.pseudoEchoVol = rom.ReadU8(trk.pos++);
        break;
    case 9:
        // XIECL
        trk.pseudoEchoLen = rom.ReadU8(trk.pos++);
        break;
    case 10:
        // XLENG (stub)
        trk.pos++;
        break;
    case 11:
        // XSWEE (stub)
        trk.pos++;
        break;
    case 12:
        // XWAIT
        trk.delay = rom.ReadU16(trk.pos);
        trk.pos += 2;
        break;
    case 13:
        // XSOFF (stub)
        trk.pos += 4;
        break;
    default:
        cmdPlayFine(trackIdx);
        break;
    }
}
