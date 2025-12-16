#include "MP2KScanner.hpp"

#include "Constants.hpp"
#include "Debug.hpp"
#include "Profile.hpp"
#include "Rom.hpp"

MP2KScanner::MP2KScanner(const Rom &rom) : rom(rom)
{
}

std::vector<MP2KScanner::Result> MP2KScanner::Scan(std::shared_ptr<Profile> profile)
{
    /* 'profile' is optional. If it is non-null, we scan only for the entry requested in the profile.
     * In that case the result is only written to the profile. The return value is an empty list.
     * If it is null, we try to find all occurances and return them in a list without writing
     * to a (non-provided) profile. */
    std::vector<Result> results;

    InitPointerCache();

    size_t findPos = SEARCH_START;

    while (true) {
        /* 1. Determine songtable position */
        size_t songTablePos;
        uint16_t songCount;

        if (profile && !profile->songTableInfoConfig.IsAuto())
            songTablePos = profile->songTableInfoConfig.pos;

        const bool songtableValid = FindSongTable(findPos, songTablePos, songCount);
        if (!songtableValid)
            break;

        if (profile && !profile->songTableInfoConfig.IsAuto())
            songCount = profile->songTableInfoConfig.count;

        if (profile && profile->playerTableConfig.size() != 0) {
            /* Load player table from configuration. In this case, the
             * sound mode must be provided via config as well, since the scanner cannot
             * find it otherwise. */
            profile->playerTableScanned = profile->playerTableConfig;

            if (profile->mp2kSoundModeConfig.IsAuto())
                break;

            profile->mp2kSoundModeScanned = profile->mp2kSoundModeScanned;
            break;
        }

        /* 2. Determine player table position */
        PlayerTableInfo playerTableInfo;
        size_t playerTablePos;
        const bool playerTableValid = FindPlayerTable(songTablePos, playerTablePos, playerTableInfo);
        if (!playerTableValid)
            continue;

        /* 3. Determine sound mode. */
        uint32_t soundMode;
        size_t soundModePos;
        const bool soundModeValid = FindSoundMode(playerTablePos, soundModePos, soundMode);
        if (!soundModeValid)
            continue;

        /* 4. Save to result list. */
        Result result{
            .mp2kSoundMode{
                .vol = static_cast<uint8_t>((soundMode >> 12) & 0xF),
                .rev = static_cast<uint8_t>((soundMode >> 0) & 0xFF),
                .freq = static_cast<uint8_t>((soundMode >> 16) & 0xF),
                .maxChannels = static_cast<uint8_t>((soundMode >> 8) & 0xF),
                .dacConfig = static_cast<uint8_t>((soundMode >> 20) & 0xF),
            },
            .playerTableInfo = playerTableInfo,
            .songTableInfo{
                .pos = songTablePos,
                .count = songCount,
                .tableIdx = static_cast<uint8_t>(results.size()),
            },
        };

        if (profile) {
            // If we are auto scanning for the song table, make sure we have selected the right one.
            // If we are manual scanning for the song table, always return immediately.
            if ((profile->songTableInfoConfig.IsAuto() && profile->songTableInfoConfig.tableIdx == results.size())
                    || !profile->songTableInfoConfig.IsAuto()) {
                profile->songTableInfoScanned = result.songTableInfo;
                profile->playerTableScanned = result.playerTableInfo;
                profile->mp2kSoundModeScanned = result.mp2kSoundMode;
                break;
            }
        }

        results.emplace_back(result);
    }

    return results;
}

bool MP2KScanner::FindSongTable(size_t &findStartPos, size_t &songTablePos, uint16_t &songCount) const
{
    for (size_t i = findStartPos; i < rom.Size() - 3; i += 4) {
        size_t candidatePos = i;
        bool candidateValid = true;

        /* check if MIN_SONG_NUM entries look like a valid song table */
        for (size_t j = 0; j < MIN_SONG_NUM; j++) {
            if (!IsValidSongTableEntry(i + j * 8, rom.IsGsf())) {
                i += j * 8;
                candidateValid = false;
                break;
            }
        }

        if (!candidateValid)
            continue;

        /* scan for a reference to the song table (for verification) */
        if (!IsPosReferenced(candidatePos))
            continue;

        /* count number of songs */
        uint16_t candidateSongCount = 0;
        uint16_t candidatePopulatedSongCount = 0;

        // COUNT_AUTO == 0xFFFF, so use one less
        for (uint16_t songIdx = 0; songIdx < 0xFFFE; songIdx++) {
            const size_t pos = candidatePos + 8 * songIdx;
            if (pos >= rom.Size() - 3)
                break;

            if (rom.IsGsf()) {
                if (!IsValidSongTableEntry(pos, true))
                    break;
                if (IsValidSongTableEntry(pos, false)) {
                    candidateSongCount = songIdx + 1;
                    candidatePopulatedSongCount++;
                }
            } else {
                if (!IsValidSongTableEntry(pos, false))
                    break;
                candidateSongCount++;
                candidatePopulatedSongCount = candidateSongCount;
            }
        }

        /* Do not allow song tables which are entirely (or almost blank).
         * This way GSF sets are not accidentally detected with number of songs=0
         * (e.g. when the falsely detected songtable only consists of zero bytes. */
        if (candidatePopulatedSongCount < MIN_SONG_NUM)
            continue;

        /* return results */
        songCount = candidateSongCount;
        songTablePos = candidatePos;
        findStartPos = candidatePos + songCount * 8;
        return true;
    }

    return false;
}

bool MP2KScanner::FindPlayerTable(size_t songTablePos, size_t &playerTablePos, PlayerTableInfo &playerTableInfo) const
{
    /* If we know the pointer to the song table, we can find the pointer to the player table by searching
     * through the ROM. Both player table and songtable are usually referenced in:
     * m4aSongNum{Start,StartOrChange,StartOrContinue,Stop,SongNumContinue}.
     * mplay_table and song_table will appear right after another in the literal pools of those functions.
     * Because we already know the song table pos, we can just check all the places where it is referenced,
     * and then check the 4 bytes before that reference. Those have to be a valid player table. */

    const size_t songTableRef = songTablePos + AGB_MAP_ROM;

    size_t musicPlayerCount = 0;
    size_t playerTableStartPos = 0;
    size_t matchCount = 0;

    for (size_t i = SEARCH_START; i < rom.Size() - 3; i += 4) {
        /* 1. Search the entire ROM for a reference to our song table. */
        if (rom.ReadU32(i) != songTableRef)
            continue;

        if (i < 4)
            continue;

        /* 2. Check if the pointer before the reference also looks like a pointer. */
        if (!rom.ValidPointer(rom.ReadU32(i - 4)))
            continue;

        /* 3. Check how many music players we have. */
        const size_t playerTablePosCandidate = rom.ReadAgbPtrToPos(i - 4);
        const size_t MAX_MUSIC_PLAYERS = 32;

        size_t musicPlayerCountCandidate;
        for (musicPlayerCountCandidate = 0; musicPlayerCountCandidate < MAX_MUSIC_PLAYERS; musicPlayerCountCandidate++) {
            if (!IsValidPlayerTableEntry(playerTablePosCandidate + musicPlayerCountCandidate * 12))
                break;
        }

        if (musicPlayerCountCandidate == 0)
            continue;

        /* 4. If we found a player table previously, check if it is the same one. */
        if (playerTableStartPos != 0 && playerTableStartPos != playerTablePosCandidate) {
            Debug::print("Found multiple player table candidates (candidateA={:#09x} candidateB={:#09x}. Bad ROM-hack?",
                    playerTableStartPos, playerTablePosCandidate);
            break;
        }

        playerTableStartPos = playerTablePosCandidate;
        musicPlayerCount = musicPlayerCountCandidate;
        matchCount += 1;

        /* 4. If we have found 5 functions, that's enough, since that's the usual number of m4a functions
         * found in games. */
        if (matchCount == 5)
            break;
    }

    /* 5. If we haven't found anything, bail out. */
    if (matchCount == 0)
        return false;

    /* 6. We can still load games with fewer references to the player table. But let the user know, there
     *    may be a problem with the ROM. */
    if (matchCount < 5)
        Debug::print("Found unexpected number of player table matches ({}). Bad ROM-hack?", matchCount);

    /* 7. Return results */
    playerTableInfo.clear();
    for (size_t i = 0; i < musicPlayerCount; i++) {
        const size_t playerPos = playerTableStartPos + i * 12;
        playerTableInfo.emplace_back(PlayerInfo{rom.ReadU8(playerPos + 8), rom.ReadU8(playerPos + 10)});
    }
    playerTablePos = playerTableStartPos;
    return true;
}

bool MP2KScanner::FindSoundMode(size_t playerTablePos, size_t &soundModePos, uint32_t &soundMode) const
{
    if (FindSoundModeNormal(playerTablePos, soundModePos, soundMode))
        return true;
    return FindSoundModeMetroid(playerTablePos, soundModePos, soundMode);
}

bool MP2KScanner::FindSoundModeNormal(size_t playerTablePos, size_t &soundModePos, uint32_t &soundMode) const
{
    /* We have to find the literal pool from m4aSoundInit(). */
    size_t playerTableReferencePos;
    size_t findStartPos = SEARCH_START;
    while (IsPosReferenced(playerTablePos, findStartPos, playerTableReferencePos)) {
        /* If we have found a reference, check if the following pattern exists:
         * [0x00] mix code (ROM-addr)
         * [0x04] mix code (RAM-addr)
         * [0x08] mix code size (CpuSet Arg)
         * [0x0C] SoundInfo ptr (RAM-addr)
         * [0x10] CgbChan ptr (RAM-addr)
         * [0x14] sound mode <---- data of interest
         * [0x18] player table len (0xNN, 0x00, 0x00, 0x00)
         * [0x1C] player table pos (ROM-addr) <---- we pivot from this supplied address
         * [0x20] memacc area TODO confirm (RAM-addr)
         */

        if (playerTableReferencePos < 0x1C)
            continue;

        const size_t signaturePos = playerTableReferencePos - 0x1C;

        if (signaturePos + 0x24 > rom.Size())
            continue;

        // fmt::print("Found reference to playerTable=0x{:x} at 0x{:x}\n", playerTablePos, playerTableReferencePos);

        // fmt::print("sound mode signature:\n");
        // fmt::print(" - mix code ROM addr: 0x{:08x}\n", rom.ReadU32(signaturePos + 0));
        // fmt::print(" - mix code RAM addr: 0x{:08x}\n", rom.ReadU32(signaturePos + 4));
        // fmt::print(" - mix code size: 0x{:08x}\n", rom.ReadU32(signaturePos + 8));
        // fmt::print(" - SoundInfo ptr: 0x{:08x}\n", rom.ReadU32(signaturePos + 12));
        // fmt::print(" - CgbChan ptr: 0x{:08x}\n", rom.ReadU32(signaturePos + 16));
        // fmt::print(" - sound mode: 0x{:08x}\n", rom.ReadU32(signaturePos + 20));
        // fmt::print(" - player table len: 0x{:08x}\n", rom.ReadU32(signaturePos + 24));
        // fmt::print(" - player table pos: 0x{:08x}\n", rom.ReadU32(signaturePos + 28));
        // fmt::print(" - memacc area: 0x{:08x}\n", rom.ReadU32(signaturePos + 32));

        /* check mix code (ROM-addr) */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0x0)))
            continue;

        // fmt::print("mix code ROM valid\n");

        /* check mix code (RAM-addr) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x4)))
            continue;

        // fmt::print("mix code RAM valid\n");

        /* check mix code size (CpuSet Arg) */
        const uint32_t cpusetArg = rom.ReadU32(signaturePos + 0x8);
        if ((cpusetArg & (1 << 26)) == 0)    // Is 32 bit copy?
            continue;
        if ((cpusetArg & 0x1FFFFF) >= 0x800)    // Is data smaller than 0x800 words? (usually just SEARCH_START)
            continue;

        // fmt::print("mix code size valid\n");

        /* check SoundInfo pointer (RAM addr) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0xC)))
            continue;

        // fmt::print("SoundInfo valid\n");

        /* check CgbChan pointer (RAM addr) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x10)))
            continue;

        // fmt::print("CgbChan valid\n");

        /* check sound mode */
        const size_t soundModePosCandidate = signaturePos + 0x14;
        const uint32_t soundModeCandidate = rom.ReadU32(soundModePosCandidate);
        if ((soundModeCandidate & 0xFF) != 0)    // reserved byte must be 0
            continue;
        if (uint32_t maxchn = (soundModeCandidate >> 8) & 0xF; maxchn < 1 || maxchn > 12)
            continue;
        if (uint32_t freq = (soundModeCandidate >> 16) & 0xF; freq == 0 || freq > 12)
            continue;
        if (uint32_t dac = (soundModeCandidate >> 20) & 0xF; dac < 8 || dac > 11)
            continue;

        // fmt::print("sound mode valid\n");

        /* check player table len */
        const uint32_t playerTableLen = rom.ReadU32(signaturePos + 0x18);
        if (playerTableLen > 32)
            continue;

        // fmt::print("player table len valid\n");

        /* check player table pos (probably redundant as it's an argument) */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0x1C)))
            continue;

        // fmt::print("player table pos valid\n");

        /* check memacc address (TODO is this really the memacc address?) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x20)))
            continue;

        // fmt::print("memacc valid\n");

        soundModePos = soundModePosCandidate;
        soundMode = soundModeCandidate;
        return true;
    }

    return false;
}

bool MP2KScanner::FindSoundModeMetroid(size_t playerTablePos, size_t &soundModePos, uint32_t &soundMode) const
{
    size_t playerTableReferencePos;
    size_t findStartPos = SEARCH_START;
    while (IsPosReferenced(playerTablePos, findStartPos, playerTableReferencePos)) {
        /* If we have found a reference, check if the following pattern exists:
         * [0x00] sound initialized? (RAM-addr)
         * [0x04] REG_IE
         * [0x08] REG_SOUNDCNT_X
         * [0x0C] REG_SOUNDCNT_H
         * [0x10] SOUNDCNT_H init value
         * [0x14] REG_SOUNDBIAS (upper byte)
         * [0x18] REG_SOUND1CNT_H (upper byte)
         * [0x1C] REG_SOUNDCNT_L
         * [0x20] mix code-ptr (RAM-addr)
         * [0x24] mix code (RAM-addr)
         * [0x28] mix code (ROM-addr)
         * [0x2C] DMA ctrl data
         * [0x30] reverb code-ptr (RAM-addr)
         * [0x34] reverb code (RAM-addr)
         * [0x38] reverb code (ROM-addr)
         * [0x3C] DMA ctrl data
         * [0x40] downsampler code-ptr (RAM-addr)
         * [0x44] downsampler code (RAM-addr)
         * [0x48] downsampler code (ROM-addr)
         * [0x4C] DMA ctrl data
         * [0x50] other DMA ctrl data???
         * [0x54] ???
         * [0x58] player table len (0xNN, 0x00, 0x00, 0x00)
         * [0x5C] sound mode <---- data of interest
         * [0x60] ??? (RAM-addr)
         * [0x64] other DMA ctrl data???
         * [0x68] REG_DMA3SAD
         * [0x6C] player table pos (ROM-addr) <---- we pivot from this supplied address
         * [0x70] other DMA ctrl data???
         * [0x74] ??? ... This stuff seems to vary between Metroid Zero Mission and Wario Ware Twisted
         */

        if (playerTableReferencePos < 0x6C)
            continue;

        const size_t signaturePos = playerTableReferencePos - 0x6C;

        if (signaturePos + 0x78 > rom.Size())
            continue;

        /* Check actual signature */

        /* [0x00] */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x0)))
            continue;

        /* [0x04] */
        if (rom.ReadU32(signaturePos + 0x4) != 0x04000200)
            continue;

        /* [0x08] */
        if (rom.ReadU32(signaturePos + 0x8) != 0x04000084)
            continue;

        /* [0x0C] */
        if (rom.ReadU32(signaturePos + 0xC) != 0x04000082)
            continue;

        /* [0x14] */
        if (rom.ReadU32(signaturePos + 0x14) != 0x04000089)
            continue;

        /* [0x18] */
        if (rom.ReadU32(signaturePos + 0x18) != 0x04000063)
            continue;

        /* [0x1C] */
        if (rom.ReadU32(signaturePos + 0x1C) != 0x04000080)
            continue;

        /* [0x20] */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x20)))
            continue;

        /* [0x24] */
        if (!IsValidIwramPointer(rom.ReadU32(signaturePos + 0x24)))
            continue;

        /* [0x28] */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0x28)))
            continue;

        /* [0x30] */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x30)))
            continue;

        /* [0x34] */
        if (!IsValidIwramPointer(rom.ReadU32(signaturePos + 0x34)))
            continue;

        /* [0x38] */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0x38)))
            continue;

        /* [0x40] */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x40)))
            continue;

        /* [0x44] */
        if (!IsValidIwramPointer(rom.ReadU32(signaturePos + 0x44)))
            continue;

        /* [0x48] */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0x48)))
            continue;

        /* [0x58] */
        const uint32_t playerTableLen = rom.ReadU32(signaturePos + 0x58);
        if (playerTableLen > 32)
            continue;

        /* [0x5C] */
        const size_t soundModePosCandidate = signaturePos + 0x5C;
        const uint32_t soundModeCandidate = rom.ReadU32(soundModePosCandidate);
        /* Compared to "Normal" sound modes vs Metroid sound modes is, that the
         * reserved byte appears to be actually used. It does something with stereo/mono
         * switching, but we probably don't care about that in agbplay. */
        if (uint32_t maxchn = (soundModeCandidate >> 8) & 0xF; maxchn < 1 || maxchn > 12)
            continue;
        if (uint32_t freq = (soundModeCandidate >> 16) & 0xF; freq == 0 || freq > 12)
            continue;
        if (uint32_t dac = (soundModeCandidate >> 20) & 0xF; dac < 8 || dac > 11)
            continue;

        /* [0x60] */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 0x60)))
            continue;

        /* [0x68] */
        if (rom.ReadU32(signaturePos + 0x68) != 0x040000D4)
            continue;

        /* [0x6C] */
        /* check player table pos (probably redundant as it's an argument) */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0x6C)))
            continue;

        soundModePos = soundModePosCandidate;
        soundMode = soundModeCandidate;
        return true;
    }

    return false;
}

bool MP2KScanner::IsPosReferenced(size_t pos) const
{
    return wordPointerCache.contains(static_cast<uint32_t>(pos + AGB_MAP_ROM));
}

bool MP2KScanner::IsPosReferenced(size_t pos, size_t &findStartPos, size_t &referencePos) const
{
    bool foundReference = false;
    for (size_t j = findStartPos; j < rom.Size() - 3; j += 4) {
        const uint32_t referenceCandidate = rom.ReadU32(j);
        if (!rom.ValidPointer(referenceCandidate))
            continue;
        if (referenceCandidate - AGB_MAP_ROM != pos)
            continue;

        foundReference = true;
        referencePos = j;
        findStartPos = j + 4;
        break;
    }
    return foundReference;
}

bool MP2KScanner::IsPosReferenced(const std::vector<size_t> &poss, size_t &index) const
{
    for (size_t j = SEARCH_START; j < rom.Size() - 3; j += 4) {
        const uint32_t referenceCandidate = rom.ReadU32(j);
        if (!rom.ValidPointer(referenceCandidate))
            continue;

        for (size_t i = 0; i < poss.size(); i++) {
            if (referenceCandidate - AGB_MAP_ROM == poss[i]) {
                index = i;
                return true;
            }
        }
    }

    return false;
}

bool MP2KScanner::IsValidSongTableEntry(size_t pos, bool relaxed) const
{
    if (!rom.ValidRange(pos, 8))
        return false;

    /* 0. (optional) during GSF scanning, a relaxed scan is used. This is because
     * GSFs have unused entries from the GSF set zeroed, but we have to assume
     * they are valid for location of the song table start. */
    if (relaxed && rom.ReadU32(pos) == 0 && rom.ReadU32(pos + 4) == 0)
        return true;

    /* 1. check if pointer to song is valid */
    if (!rom.ValidPointer(rom.ReadU32(pos + 0)))
        return false;

    /* 2. check if music player numbers are correct.
     * Special case: For GSF sets p2 is always zero instead of equal to p1 */
    const uint8_t p1 = rom.ReadU8(pos + 4);
    const uint8_t z1 = rom.ReadU8(pos + 5);
    const uint8_t p2 = rom.ReadU8(pos + 6);
    const uint8_t z2 = rom.ReadU8(pos + 7);

    if (z1 != 0 || z2 != 0 || (rom.IsGsf() ? (p2 != 0) : (p1 != p2)))
        return false;

    /* 3. check if song is valid */
    const size_t songPos = rom.ReadAgbPtrToPos(pos + 0);

    const uint8_t nTracks = rom.ReadU8(songPos + 0);
    const uint8_t nBlocks = rom.ReadU8(songPos + 1);    // this field is not used, should be 0
    const uint8_t prio = rom.ReadU8(songPos + 2);
    const uint8_t rev = rom.ReadU8(songPos + 3);

    /* 3.1. some 'empty' songs have all fields set to zero, allow those. */
    if ((nTracks | nBlocks | prio | rev) == 0)
        return true;

    /* 3.2. check if voice group pointer is a valid pointer */
    if (!rom.ValidPointer(rom.ReadU32(songPos + 4)))
        return false;

    /* 3.3. verify track pointers */
    for (size_t i = 0; i < nTracks; i++) {
        if (!rom.ValidPointer(rom.ReadU32(songPos + 8 + (i * 4))))
            return false;
    }

    return true;
}

bool MP2KScanner::IsValidPlayerTableEntry(size_t pos) const
{
    /* A player table entry usually looks like this:
     * - RAM pointer
     * - RAM pointer
     * - max track count
     * - unknown flag (0 or 1)
     *
     * Optionally, all fields may be zero.
     */
    const uint32_t playerPtr = rom.ReadU32(pos + 0);
    const uint32_t trackPtr = rom.ReadU32(pos + 4);
    const uint16_t trackLimit = rom.ReadU16(pos + 8);
    const uint16_t unknown = rom.ReadU16(pos + 10);

    if (playerPtr == 0 && trackPtr == 0 && trackLimit == 0 && unknown == 0)
        return true;

    if (!IsValidRamPointer(playerPtr))
        return false;
    if (!IsValidRamPointer(trackPtr))
        return false;
    if (trackLimit > MAX_TRACKS)
        return false;
    if (unknown > 1)
        return false;

    return true;
}

void MP2KScanner::InitPointerCache()
{
    if (wordPointerCache.size() > 0)
        return;

    wordPointerCache.clear();

    /* find list of all pointers in ROM */
    for (size_t i = SEARCH_START; i < rom.Size() - 3; i += 4) {
        const uint32_t ptr = rom.ReadU32(i);
        if (rom.ValidPointer(ptr) && (ptr % 4) == 0)
            wordPointerCache.insert(ptr);
    }
}

bool MP2KScanner::IsValidIwramPointer(uint32_t word)
{
    if (word >= 0x03000000 && word <= 0x03007FFF)
        return true;
    return false;
}

bool MP2KScanner::IsValidEwramPointer(uint32_t word)
{
    if (word >= 0x02000000 && word <= 0x0203FFFF)
        return true;
    return false;
}

bool MP2KScanner::IsValidRamPointer(uint32_t word)
{
    if (IsValidIwramPointer(word))
        return true;
    if (IsValidEwramPointer(word))
        return true;
    return false;
}
