
/** $VER: SoundFontCache.h (2024.08.25) **/

#pragma once

#include <bassmidi.h>
#include <sflist.h>

#include <string>

struct sflist_presets;

extern void CacheInit();
extern void CacheDispose();

extern HSOUNDFONT CacheAddSoundFont(const std::string & filePath);
extern void CacheRemoveSoundFont(HSOUNDFONT hSoundFont);

extern sflist_presets * CacheAddSoundFontList(const std::string & filePath);
extern void CacheRemoveSoundFontList(sflist_presets * soundFontList);

extern void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded);
