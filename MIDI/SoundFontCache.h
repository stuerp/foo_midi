
/** $VER: SoundFontCache.h (2024.08.25) - Sound font cache for the BASSMIDI player **/

#pragma once

#include <bassmidi.h>
#include <sflist.h>

#include <string>

struct sflist_t;

extern void CacheInit();
extern void CacheDispose();

extern HSOUNDFONT CacheAddSoundFont(const std::string & filePath);
extern void CacheRemoveSoundFont(HSOUNDFONT hSoundFont);

extern sflist_t * CacheAddSoundFontList(const std::string & filePath);
extern void CacheRemoveSoundFontList(sflist_t * soundFontList);

extern void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded);
