
/** $VER: SoundfontCache.cpp (2025.08.27) - Soundfont cache for wavetable players **/

#include "pch.h"

#include "SoundfontCache.h"
#include "Encoding.h"
#include "Exception.h"
#include "CriticalSection.h"
#include "Log.h"
#include "Resource.h"

#include <map>
#include <thread>

#include <time.h>

static volatile bool _IsCacheRunning = false;

static void CacheRun();

static std::thread * _CacheThread = nullptr;

#pragma warning(disable: 4820) // x bytes padding added after data member

struct cache_item_t
{
    cache_item_t() : IsTemporary(false), _hSoundfont(), _ReferenceCount(), _TimeReleased()
    {
    }

    fs::path FilePath;
    bool IsTemporary;
    HSOUNDFONT _hSoundfont;
    size_t _ReferenceCount;
    time_t _TimeReleased;
};

struct marker_t
{
    marker_t() : IsUsed(false)
    {
    }

    bool IsUsed;
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
/// Returns a native OS path when the soundfont is a regular disk file.
/// Archive / VFS items (foobar paths with '|', remote, unrecognized) return empty so FontInitUser is used.
/// </summary>
static std::wstring TryGetNativeSoundfontPath(const fs::path & filePath)
{
    const std::string Path = (const char *) filePath.u8string().c_str();

    if (Path.find('|') != std::string::npos)
        return { };

    if (filesystem::g_is_remote_or_unrecognized(Path.c_str()))
        return { };

    const pfc::string Native = filesystem::g_get_native_path(Path.c_str());

    if (Native.isEmpty())
        return { };

    if (::strchr(Native.c_str(), '|') != nullptr)
        return { };

    const std::wstring Wide = msc::UTF8ToWide(Native.c_str());

    const DWORD Attributes = ::GetFileAttributesW(Wide.c_str());

    if ((Attributes == INVALID_FILE_ATTRIBUTES) || (Attributes & FILE_ATTRIBUTE_DIRECTORY))
        return { };

    return Wide;
}

/// <summary>
/// Adds a Soundfont to the cache.
/// </summary>
HSOUNDFONT CacheAddSoundfont(const fs::path & filePath, bool isTemporary)
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
        const std::wstring NativePath = TryGetNativeSoundfontPath(filePath);

        if (!NativePath.empty())
        {
            Log.AtInfo().Write(STR_COMPONENT_BASENAME " is loading soundfont \"%s\" with BASS_MIDI_FontInit.", filePath.string().c_str());

            hSoundfont = ::BASS_MIDI_FontInit(NativePath.c_str(), Flags);
        }
        else
        {
            Log.AtInfo().Write(STR_COMPONENT_BASENAME " is loading soundfont \"%s\" with BASS_MIDI_FontInitUser.", filePath.string().c_str());

            auto * File = new file::ptr;

            filesystem::g_open(*File, filePath.string().c_str(), filesystem::open_mode_read, fb2k::noAbort);

            hSoundfont = ::BASS_MIDI_FontInitUser(&_BASSMIDICallbacks, File, Flags);
        }

        if (hSoundfont != 0)
        {
            Item.FilePath = filePath;
            Item.IsTemporary = isTemporary;
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

    for (auto & it : _CacheItems)
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

        BASS_MIDI_FONTINFO FontInfo = { };

        if (::BASS_MIDI_FontGetInfo(it.second._hSoundfont, &FontInfo))
        {
            totalSampleDataSize    += FontInfo.samsize;
            totalSamplesDataLoaded += FontInfo.samload;
        }
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
                const auto & Item = it->second;

                if (Item._ReferenceCount == 0)
                {
                    if (::difftime(Now, Item._TimeReleased) >= 10.0)
                    {
                        if (Item._hSoundfont)
                            ::BASS_MIDI_FontFree(Item._hSoundfont);

                        if (Item.IsTemporary)
                        {
                            if (!::DeleteFileA((const char *) Item.FilePath.string().c_str()))
                                Log.AtError().Write(msc::GetErrorMessage(msc::FormatText("Failed to delete temporary file \"%s\"", Item.FilePath.string().c_str()).c_str(), ::GetLastError()).c_str());
                        }

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
