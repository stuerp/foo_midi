
/** $VER: SoundfontCache.cpp (2025.06.21) - Soundfont cache for the BASSMIDI player **/

#include "pch.h"

#include "SoundfontCache.h"
#include "Encoding.h"
#include "CriticalSection.h"

#include <map>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;

#include <time.h>

static bool _IsCacheRunning = false;

static sflist_t * LoadSoundfontList(const std::string & filePath);
static void CacheRun();

static std::thread * _CacheThread = nullptr;

#pragma warning(disable: 4820) // x bytes padding added after data member
struct CacheItem
{
    CacheItem() : _hSoundfont(), _SoundfontList(), _ReferenceCount(), _TimeReleased()
    {
    }

    HSOUNDFONT _hSoundfont;
    sflist_t * _SoundfontList;
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
        if (it->second._hSoundfont)
            ::BASS_MIDI_FontFree(it->second._hSoundfont);

        if (it->second._SoundfontList)
            ::sflist_free(it->second._SoundfontList);
    }
}

/// <summary>
/// Adds a Soundfont to the cache.
/// </summary>
HSOUNDFONT CacheAddSoundfont(const std::string & filePath)
{
    HSOUNDFONT hSoundfont = 0;

    critical_section_lock_t Lock(_CacheCriticalSection);

    CacheItem & Item = _CacheItems[filePath];

    if (Item._hSoundfont == 0)
    {
        hSoundfont = ::BASS_MIDI_FontInit(::UTF8ToWide(filePath).c_str(), 0);

        if (hSoundfont)
        {
            Item._hSoundfont = hSoundfont;
            Item._ReferenceCount = 1;
        }
        else
            _CacheItems.erase(filePath);
    }
    else
    {
        hSoundfont = Item._hSoundfont;
        ++Item._ReferenceCount;
    }

    return hSoundfont;
}

/// <summary>
/// Removes a Soundfont from the cache.
/// </summary>
void CacheRemoveSoundfont(HSOUNDFONT hSoundfont)
{
    critical_section_lock_t Lock(_CacheCriticalSection);

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        if (it->second._hSoundfont == hSoundfont)
        {
            if (--it->second._ReferenceCount == 0)
                ::time(&it->second._TimeReleased);
            break;
        }
    }
}

/// <summary>
/// Adds a Soundfont List to the cache.
/// </summary>
sflist_t * CacheAddSoundfontList(const std::string & filePath)
{
    sflist_t * SoundFontlist = nullptr;

    critical_section_lock_t Lock(_CacheCriticalSection);

    CacheItem & Item = _CacheItems[filePath];

    if (Item._SoundfontList == nullptr)
    {
        SoundFontlist = LoadSoundfontList(filePath);

        if (SoundFontlist)
        {
            Item._SoundfontList = SoundFontlist;
            Item._ReferenceCount = 1;
        }
        else
        {
            _CacheItems.erase(filePath);
        }
    }
    else
    {
        SoundFontlist = Item._SoundfontList;
        ++Item._ReferenceCount;
    }

    return SoundFontlist;
}

/// <summary>
/// Removes a Soundfont list from the cache.
/// </summary>
void CacheRemoveSoundfontList(sflist_t * soundfontList)
{
    critical_section_lock_t Lock(_CacheCriticalSection);

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        if (it->second._SoundfontList == soundfontList)
        {
            if (--it->second._ReferenceCount == 0)
                ::time(&it->second._TimeReleased);
            break;
        }
    }
}

struct soundfont_marker_t
{
    bool IsUsed;

    soundfont_marker_t() : IsUsed(false)
    {
    }
};

/// <summary>
/// Gets statistics about the cache.
/// </summary>
void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded)
{
    std::map<HSOUNDFONT, soundfont_marker_t> SoundfontMarkers;

    critical_section_lock_t Lock(_CacheCriticalSection);

    totalSampleDataSize = 0;
    totalSamplesDataLoaded = 0;

    for (auto it = _CacheItems.begin(); it != _CacheItems.end(); ++it)
    {
        BASS_MIDI_FONTINFO FontInfo;

        if (it->second._hSoundfont)
        {
            soundfont_marker_t & sfm = SoundfontMarkers[it->second._hSoundfont];

            if (!sfm.IsUsed)
            {
                sfm.IsUsed = true;

                if (::BASS_MIDI_FontGetInfo(it->second._hSoundfont, &FontInfo))
                {
                    totalSampleDataSize += FontInfo.samsize;
                    totalSamplesDataLoaded += FontInfo.samload;
                }
            }
        }
        else
        if (it->second._SoundfontList)
        {
            sflist_t * SoundFontList = it->second._SoundfontList;

            for (unsigned int i = 0, j = SoundFontList->Count; i < j; ++i)
            {
                HSOUNDFONT hSoundfont = SoundFontList->FontEx[i].font;

                if (hSoundfont != NULL)
                {
                    soundfont_marker_t & sfm = SoundfontMarkers[hSoundfont];

                    if (!sfm.IsUsed)
                    {
                        sfm.IsUsed = true;

                        if (::BASS_MIDI_FontGetInfo(hSoundfont, &FontInfo))
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
/// Loads a Soundfont list (*.sflist)
/// </summary>
static sflist_t * LoadSoundfontList(const std::string & filePath)
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

            sflist_t * SoundfontList = ::sflist_load(Data.data(), ::strlen(Data.data()), ::WideToUTF8(FilePath.parent_path()).c_str(), ErrorMessage);

            return SoundfontList;
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
                        if (it->second._hSoundfont)
                            ::BASS_MIDI_FontFree(it->second._hSoundfont);

                        if (it->second._SoundfontList)
                            ::sflist_free(it->second._SoundfontList);

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
