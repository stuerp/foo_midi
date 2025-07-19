
/** $VER: SoundFontCache.h (2024.08.25) - Sound font cache for the BASSMIDI player **/

#pragma once

#include <bassmidi.h>
#include <sflist.h>

#include <string>

struct sflist_t;

extern void CacheInit();
extern void CacheDispose();

extern HSOUNDFONT CacheAddSoundfont(const std::string & filePath);
extern void CacheRemoveSoundfont(HSOUNDFONT hSoundFont);

extern sflist_t * CacheAddSoundfontList(const std::string & filePath);
extern void CacheRemoveSoundfontList(sflist_t * soundFontList);

extern void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded);
