
/** $VER: SoundFontCache.h (2023.06.12) **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <bassmidi.h>

struct sflist_presets;

extern void CacheInit();
extern void CacheDispose();

extern HSOUNDFONT CacheAddSoundFont(const char * filePath);
extern void CacheRemoveSoundFont(HSOUNDFONT hSoundFont);

extern sflist_presets * CacheAddSoundFontList(const char * filePath);
extern void CacheRemoveSoundFontList(sflist_presets * soundFontList);

extern void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded);
