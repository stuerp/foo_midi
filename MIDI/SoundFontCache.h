
/** $VER: SoundFontCache.h (2025.07.20) - Soundfont cache for the BASSMIDI player **/

#pragma once

#include <bassmidi.h>

#include <string>

#include "Soundfont.h"

extern void CacheInit();
extern void CacheDispose();

extern HSOUNDFONT CacheAddSoundfont(const fs::path & filePath);
extern void CacheRemoveSoundfont(HSOUNDFONT hSoundFont);

extern void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded);
