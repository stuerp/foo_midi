
/** $VER: SoundFontCache.cpp (2025.06.21) - Sound font cache for the BASSMIDI player **/

#include "pch.h"

#include "SoundFontCache.h"
#include "Encoding.h"
#include "CriticalSection.h"

#include <string>
#include <map>
#include <filesystem>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

#include <time.h>

static bool _IsCacheRunning = false;

static sflist_t * LoadSoundFontList(const std::string & filePath);
static void CacheRun();

static std::thread * _CacheThread = nullptr;

#pragma warning(disable: 4820) // x bytes padding added after data member
struct CacheItem
{
    CacheItem() : _hSoundFont(), _SoundFontList(), _ReferenceCount(), _TimeReleased()
    {
    }

    HSOUNDFONT _hSoundFont;
    sflist_t * _SoundFontList;
    size_t _ReferenceCount;
    time_t _TimeReleased;
};
#pragma warning(default: 4820) // x bytes padding added after data member


static std::map<std::string, CacheItem> _CacheItems;
static critical_section_t _CacheCriticalSection;

/// <summary>
/// Initializes the cache.
/// </summary>
void CacheInit()
{
    _CacheThread = new std::thread(CacheRun);
}

/// <summary>
/// Disposes the cache.
/// </summary>
void CacheDispose()
{
    _IsCacheRunning = false;

    _CacheThread->join();

    delete _CacheThread;
    _CacheThread = nullptr;

    critical_section_lock_t Lock(_CacheCriticalSection);

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        if (it->second._hSoundFont)
            ::BASS_MIDI_FontFree(it->second._hSoundFont);

        if (it->second._SoundFontList)
            ::sflist_free(it->second._SoundFontList);
    }
}

/// <summary>
/// Adds a SoundFont to the cache.
/// </summary>
HSOUNDFONT CacheAddSoundFont(const std::string & filePath)
{
    HSOUNDFONT hSoundFont = 0;

    critical_section_lock_t Lock(_CacheCriticalSection);

    CacheItem & Item = _CacheItems[filePath];

    if (Item._hSoundFont == 0)
    {
        hSoundFont = ::BASS_MIDI_FontInit(::UTF8ToWide(filePath).c_str(), 0);

        if (hSoundFont)
        {
            Item._hSoundFont = hSoundFont;
            Item._ReferenceCount = 1;
        }
        else
            _CacheItems.erase(filePath);
    }
    else
    {
        hSoundFont = Item._hSoundFont;
        ++Item._ReferenceCount;
    }

    return hSoundFont;
}

/// <summary>
/// Removes a SoundFont from the cache.
/// </summary>
void CacheRemoveSoundFont(HSOUNDFONT hSoundFont)
{
    critical_section_lock_t Lock(_CacheCriticalSection);

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        if (it->second._hSoundFont == hSoundFont)
        {
            if (--it->second._ReferenceCount == 0)
                ::time(&it->second._TimeReleased);
            break;
        }
    }
}

/// <summary>
/// Adds a SoundFont List to the cache.
/// </summary>
sflist_t * CacheAddSoundFontList(const std::string & filePath)
{
    sflist_t * SoundFontList = nullptr;

    critical_section_lock_t Lock(_CacheCriticalSection);

    CacheItem & Item = _CacheItems[filePath];

    if (Item._SoundFontList == nullptr)
    {
        SoundFontList = LoadSoundFontList(filePath);

        if (SoundFontList)
        {
            Item._SoundFontList = SoundFontList;
            Item._ReferenceCount = 1;
        }
        else
        {
            _CacheItems.erase(filePath);
        }
    }
    else
    {
        SoundFontList = Item._SoundFontList;
        ++Item._ReferenceCount;
    }

    return SoundFontList;
}

/// <summary>
/// Removes a SoundFont List from the cache.
/// </summary>
void CacheRemoveSoundFontList(sflist_t * soundFontList)
{
    critical_section_lock_t Lock(_CacheCriticalSection);

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        if (it->second._SoundFontList == soundFontList)
        {
            if (--it->second._ReferenceCount == 0)
                ::time(&it->second._TimeReleased);
            break;
        }
    }
}

struct SoundFontMarker
{
    bool IsUsed;

    SoundFontMarker() : IsUsed(false)
    {
    }
};

/// <summary>
/// Gets statistics about the cache.
/// </summary>
void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded)
{
    std::map<HSOUNDFONT, SoundFontMarker> SoundFontMarkers;

    critical_section_lock_t Lock(_CacheCriticalSection);

    totalSampleDataSize = 0;
    totalSamplesDataLoaded = 0;

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        BASS_MIDI_FONTINFO FontInfo;

        if (it->second._hSoundFont)
        {
            SoundFontMarker & sfm = SoundFontMarkers[it->second._hSoundFont];

            if (!sfm.IsUsed)
            {
                sfm.IsUsed = true;

                if (::BASS_MIDI_FontGetInfo(it->second._hSoundFont, &FontInfo))
                {
                    totalSampleDataSize += FontInfo.samsize;
                    totalSamplesDataLoaded += FontInfo.samload;
                }
            }
        }
        else
        if (it->second._SoundFontList)
        {
            sflist_t * SoundFontList = it->second._SoundFontList;

            for (unsigned int i = 0, j = SoundFontList->Count; i < j; ++i)
            {
                HSOUNDFONT hSoundFont = SoundFontList->FontEx[i].font;

                if (hSoundFont)
                {
                    SoundFontMarker & sfm = SoundFontMarkers[hSoundFont];

                    if (!sfm.IsUsed)
                    {
                        sfm.IsUsed = true;

                        if (::BASS_MIDI_FontGetInfo(hSoundFont, &FontInfo))
                        {
                            totalSampleDataSize += FontInfo.samsize;
                            totalSamplesDataLoaded += FontInfo.samload;
                        }
                    }
                }
            }
        }
    }
}

/// <summary>
/// Loads a SoundFont List (*.sflist)
/// </summary>
static sflist_t * LoadSoundFontList(const std::string & filePath)
{
    try
    {
        std::uintmax_t Size = fs::file_size(filePath);

        std::vector<char> Data(Size + 1);

        FILE * fp = nullptr;

        ::_wfopen_s(&fp, ::UTF8ToWide(filePath).c_str(), L"r");

        if (fp == nullptr)
            return nullptr;

        ::fread(Data.data(), Data.size(), 1, fp);

        ::fclose(fp);

        {
            Data[Size] = '\0';

            char ErrorMessage[sflist_max_error];
            fs::path FilePath = filePath;

            sflist_t * SoundFontList = ::sflist_load(Data.data(), ::strlen(Data.data()), ::WideToUTF8(FilePath.parent_path()).c_str(), ErrorMessage);

            return SoundFontList;
        }
    }
    catch (...)
    {
        return nullptr;
    }
}

static void CacheRun()
{
    _IsCacheRunning = true;

    while (_IsCacheRunning)
    {
        time_t Now;

        ::time(&Now);

        {
            critical_section_lock_t Lock(_CacheCriticalSection);

            for (auto it = _CacheItems.begin(); it != _CacheItems.end();)
            {
                if (it->second._ReferenceCount == 0)
                {
                    if (::difftime(Now, it->second._TimeReleased) >= 10.0)
                    {
                        if (it->second._hSoundFont)
                            ::BASS_MIDI_FontFree(it->second._hSoundFont);

                        if (it->second._SoundFontList)
                            ::sflist_free(it->second._SoundFontList);

                        it = _CacheItems.erase(it);
                        continue;
                    }
                }

                ++it;
            }
        }

        ::Sleep(250);
    }
}
