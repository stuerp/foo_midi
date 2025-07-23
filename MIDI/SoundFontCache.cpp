
/** $VER: SoundfontCache.cpp (2025.07.23) - Soundfont cache for wavetable players **/

#include "pch.h"

#include "SoundfontCache.h"
#include "Encoding.h"
#include "CriticalSection.h"

#include <map>
#include <thread>

#include <time.h>

static volatile bool _IsCacheRunning = false;

static void CacheRun();

static std::thread * _CacheThread = nullptr;

#pragma warning(disable: 4820) // x bytes padding added after data member

struct cache_item_t
{
    cache_item_t() : _hSoundfont(), _ReferenceCount(), _TimeReleased()
    {
    }

    HSOUNDFONT _hSoundfont;
    size_t _ReferenceCount;
    time_t _TimeReleased;
};

struct marker_t
{
    bool IsUsed;

    marker_t() : IsUsed(false)
    {
    }
};

static std::map<std::string, cache_item_t> _CacheItems;
static critical_section_t _CacheCriticalSection;

static const BASS_FILEPROCS _BASSMIDICallbacks
{
    // Close
    [](void * user)
    {
        try
        {
            auto File = (file::ptr *) user;

            delete File;
        }
        catch (...)
        {
        }
    },

    // Size,
    [](void * user) -> QWORD
    {
        try
        {
            auto File = (file::ptr *) user;

            t_filesize FileSize = (*File)->get_size_ex(fb2k::noAbort);

            return (QWORD) FileSize;
        }
        catch (...)
        {
            return 0;
        }
    },

    // Read
    [](void * data, DWORD size, void * user) -> DWORD
    {
        try
        {
            auto File = (file::ptr *) user;

            size = (DWORD) (*File)->read(data, (t_size) size, fb2k::noAbort);

            return size;
        }
        catch (...)
        {
            return 0;
        }
    },

    // Seek
    [](QWORD offset, void * user) ->BOOL
    {
        try
        {
            auto File = (file::ptr *) user;

            (*File)->seek_ex((t_sfilesize) offset, file::t_seek_mode::seek_from_beginning, fb2k::noAbort);

            return TRUE;
        }
        catch (...)
        {
            return FALSE;
        }
    }
};

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

    for (auto it : _CacheItems)
    {
        if (it.second._hSoundfont != NULL)
            ::BASS_MIDI_FontFree(it.second._hSoundfont);
    }
}

/// <summary>
/// Adds a Soundfont to the cache.
/// </summary>
HSOUNDFONT CacheAddSoundfont(const fs::path & filePath)
{
    HSOUNDFONT hSoundfont = 0;

    critical_section_lock_t Lock(_CacheCriticalSection);

    std::string Key = filePath.string();

    // Convert to lowercase. This does not preserve Unicode correctness but that's OK. I just need a unique key.
    std::transform(Key.begin(), Key.end(), Key.begin(), [](unsigned char ch) { return (char) std::tolower(ch); });

    cache_item_t & Item = _CacheItems[Key];

    if (Item._hSoundfont == 0)
    {
        const DWORD Flags = BASS_MIDI_FONT_XGDRUMS;

        auto  * File = new file::ptr;

        filesystem::g_open(*File, (const char *) filePath.string().c_str(), filesystem::open_mode_read, fb2k::noAbort);

        hSoundfont = ::BASS_MIDI_FontInitUser(&_BASSMIDICallbacks, File, Flags);
//      hSoundfont = ::BASS_MIDI_FontInit(::UTF8ToWide((const char *) filePath.string().c_str()).c_str(), Flags);

        if (hSoundfont != 0)
        {
            Item._hSoundfont = hSoundfont;
            Item._ReferenceCount = 1;
        }
        else
            _CacheItems.erase(Key);
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

    for (auto it : _CacheItems)
    {
        if (it.second._hSoundfont == hSoundfont)
        {
            if (--it.second._ReferenceCount == 0)
                ::time(&it.second._TimeReleased);
            break;
        }
    }
}

/// <summary>
/// Gets statistics about the cache.
/// </summary>
void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded)
{
    std::map<HSOUNDFONT, marker_t> SoundfontMarkers;

    critical_section_lock_t Lock(_CacheCriticalSection);

    totalSampleDataSize    = 0;
    totalSamplesDataLoaded = 0;

    for (auto it : _CacheItems)
    {
        if (it.second._hSoundfont == NULL)
            continue;

        marker_t & sfm = SoundfontMarkers[it.second._hSoundfont];

        if (sfm.IsUsed)
            continue;

        sfm.IsUsed = true;

        BASS_MIDI_FONTINFO FontInfo;

        if (::BASS_MIDI_FontGetInfo(it.second._hSoundfont, &FontInfo))
        {
            totalSampleDataSize    += FontInfo.samsize;
            totalSamplesDataLoaded += FontInfo.samload;
        }
    }
}

static void CacheRun()
{
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

                        it = _CacheItems.erase(it);
                        continue;
                    }
                }

                ++it;
            }
        }

        ::Sleep(2500);
    }
}
