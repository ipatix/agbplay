#include "MP2KScanner.h"

#include "Rom.h"
#include "Constants.h"

MP2KScanner::MP2KScanner(const Rom &rom) : rom(rom)
{
}

std::vector<MP2KScanner::Result> MP2KScanner::Scan()
{
    std::vector<Result> results;

    InitPointerCache();

    size_t findStartPos = SEARCH_START;

    while (true) {
        size_t songTablePos;
        uint16_t songCount;
        const bool songtableValid = FindSongTable(findStartPos, songTablePos, songCount);
        if (!songtableValid)
            break;

        PlayerTableInfo playerTableInfo;
        size_t playerTablePos;
        const bool maxTracksValid = FindPlayerTable(songTablePos, playerTablePos, playerTableInfo);
        if (!maxTracksValid)
            break;

        uint32_t soundMode;
        size_t soundModePos;
        const bool soundModeValid = FindSoundMode(playerTablePos, soundModePos, soundMode);
        if (!soundModeValid)
            break;

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
        findStartPos += candidatePos + songCount * 8;
        return true;
    }

    return false;
}

bool MP2KScanner::FindPlayerTable(size_t songTablePos, size_t &playerTablePos, PlayerTableInfo &playerTableInfo) const
{
    /* The player table is usually located right before the song table.
     * For cases where it is not (e.g. romhacks), the user will have to specify
     * it manually. */

    /* 1. Search in reverse and skip possibly up to 3 linker alignment words. */
    const size_t maxAlignmentWords = 3;
    size_t playerTableEndPos = songTablePos;

    for (size_t i = 0; i < maxAlignmentWords; i++) {
        if (playerTableEndPos < 4)
            return false;
        if (rom.ReadU32(playerTableEndPos - 4) != 0)
            break;
        playerTableEndPos -= 4;
    }

    /* 2. Search in reverse how many player entries there are */
    const size_t maxMusicPlayers = 32; // maximum of mks4agb
    size_t playerTableStartPos = playerTableEndPos;
    size_t musicPlayerCount = 0;
    std::vector<size_t> playerTableStartPosCandidates;
    std::vector<size_t> musicPlayerCountCandidates;

    for (size_t i = 0; i < maxMusicPlayers; i++) {
        if (playerTableStartPos < 12)
            break;

        /* Some games use zero padded player table entires. Such an entry may be the first entry,
         * so we need to check all possible start addresses of valid candidates. */

        if (!IsValidPlayerTableEntry(playerTableStartPos - 12))
            break;

        playerTableStartPos -= 12;
        musicPlayerCount += 1;
        playerTableStartPosCandidates.insert(playerTableStartPosCandidates.begin(), playerTableStartPos);
        musicPlayerCountCandidates.insert(musicPlayerCountCandidates.begin(), musicPlayerCount);
    }

    if (musicPlayerCount == 0)
        return false;

    /* 3. check for reference to player table */
    size_t candidateIndex;
    if (!IsPosReferenced(playerTableStartPosCandidates, candidateIndex))
        return false;
    playerTableStartPos = playerTableStartPosCandidates.at(candidateIndex);
    musicPlayerCount = musicPlayerCountCandidates.at(candidateIndex);

    /* 4. return results */
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
    /* We have to find the literal pool from m4aSoundInit(). */
    size_t playerTableReferencePos;
    size_t findStartPos = SEARCH_START;
    while (IsPosReferenced(playerTablePos, findStartPos, playerTableReferencePos)) {
        /* If we have found a reference, check if the following pattern exists:
         * - mix code (ROM-addr)
         * - mix code (RAM-addr)
         * - mix code size (CpuSet Arg)
         * - SoundInfo ptr (RAM-addr)
         * - CgbChan ptr (RAM-addr)
         * - sound mode <---- data of interest
         * - player table len (0xNN, 0x00, 0x00, 0x00)
         * - player table pos (ROM-addr) <---- we pivot from this supplied address
         * - memacc area TODO confirm (RAM-addr) */

        if (playerTableReferencePos < 28)
            continue;

        const size_t signaturePos = playerTableReferencePos - 28;

        //fmt::print("Found reference to playerTable=0x{:x} at 0x{:x}\n", playerTablePos, playerTableReferencePos);

        //fmt::print("sound mode signature:\n");
        //fmt::print(" - mix code ROM addr: 0x{:08x}\n", rom.ReadU32(signaturePos + 0));
        //fmt::print(" - mix code RAM addr: 0x{:08x}\n", rom.ReadU32(signaturePos + 4));
        //fmt::print(" - mix code size: 0x{:08x}\n", rom.ReadU32(signaturePos + 8));
        //fmt::print(" - SoundInfo ptr: 0x{:08x}\n", rom.ReadU32(signaturePos + 12));
        //fmt::print(" - CgbChan ptr: 0x{:08x}\n", rom.ReadU32(signaturePos + 16));
        //fmt::print(" - sound mode: 0x{:08x}\n", rom.ReadU32(signaturePos + 20));
        //fmt::print(" - player table len: 0x{:08x}\n", rom.ReadU32(signaturePos + 24));
        //fmt::print(" - player table pos: 0x{:08x}\n", rom.ReadU32(signaturePos + 28));
        //fmt::print(" - memacc area: 0x{:08x}\n", rom.ReadU32(signaturePos + 32));

        /* check mix code (ROM-addr) */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 0)))
            continue;

        //fmt::print("mix code ROM valid\n");

        /* check mix code (RAM-addr) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 4)))
            continue;

        //fmt::print("mix code RAM valid\n");

        /* check mix code size (CpuSet Arg) */
        const uint32_t cpusetArg = rom.ReadU32(signaturePos + 8);
        if ((cpusetArg & (1 << 26)) == 0) // Is 32 bit copy?
            continue;
        if ((cpusetArg & 0x1FFFFF) >= 0x800) // Is data smaller than 0x800 words? (usually just SEARCH_START)
            continue;

        //fmt::print("mix code size valid\n");

        /* check SoundInfo pointer (RAM addr) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 12)))
            continue;

        //fmt::print("SoundInfo valid\n");

        /* check CgbChan pointer (RAM addr) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 16)))
            continue;

        //fmt::print("CgbChan valid\n");

        /* check sound mode */
        const size_t soundModePosCandidate = signaturePos + 20;
        const uint32_t soundModeCandidate = rom.ReadU32(soundModePosCandidate);
        if ((soundModeCandidate & 0xFF) != 0) // reserved byte must be 0
            continue;
        if (uint32_t maxchn = (soundModeCandidate >> 8) & 0xF; maxchn < 1 || maxchn > 12)
            continue;
        if (uint32_t freq = (soundModeCandidate >> 16) & 0xF; freq == 0 || freq > 12)
            continue;
        if (uint32_t dac = (soundModeCandidate >> 20) & 0xF; dac < 8 || dac > 11)
            continue;

        //fmt::print("sound mode valid\n");

        /* check player table len */
        const uint32_t playerTableLen = rom.ReadU32(signaturePos + 24);
        if (playerTableLen > 32)
            continue;

        //fmt::print("player table len valid\n");

        /* check player table pos (probably redundant as it's an argument) */
        if (!rom.ValidPointer(rom.ReadU32(signaturePos + 28)))
            continue;

        //fmt::print("player table pos valid\n");

        /* check memacc address (TODO is this really the memacc address?) */
        if (!IsValidRamPointer(rom.ReadU32(signaturePos + 32)))
            continue;

        //fmt::print("memacc valid\n");

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
        const size_t referenceCandidate = rom.ReadU32(j);
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
    for (size_t j = SEARCH_START; j < rom.Size() - 3; j+= 4) {
        const size_t referenceCandidate = rom.ReadU32(j);
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
    const uint8_t nBlocks = rom.ReadU8(songPos + 1);  // this field is not used, should be 0
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
