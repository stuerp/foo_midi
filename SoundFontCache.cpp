
/** $VER: SoundFontCache.cpp (2023.06.12) **/

#include <CppCoreCheck/Warnings.h>

#pragma warning(disable: 4625 4626 5045 ALL_CPPCORECHECK_WARNINGS)

#include "SoundFontCache.h"

#include <sflist.h>

#include <string>
#include <map>
#include <time.h>

#undef USE_STD_THREAD
#ifdef USE_STD_THREAD
#include <thread>
#endif

static bool _IsCacheRunning = false;

static sflist_presets * LoadSoundFontList(const char * filePath);
static void CacheRun();

#ifdef USE_STD_THREAD
static std::thread * Cache_Thread = NULL;
#else
class CacheThread : public pfc::thread
{
public:
    virtual void threadProc()
    {
        CacheRun();
    }

    virtual ~CacheThread()
    {
        _IsCacheRunning = false;
        waitTillDone();
    }
};

static CacheThread * _CacheThread = nullptr;
#endif

#pragma warning(disable: 4820) // x bytes padding added after data member
struct CacheItem
{
    HSOUNDFONT _hSoundFont;
    sflist_presets * _SoundFontList;
    size_t _ReferenceCount;
    time_t _TimeReleased;

    CacheItem() : _hSoundFont(0), _SoundFontList(nullptr), _ReferenceCount(0), _TimeReleased(0)
    {
    }
};
#pragma warning(default: 4820) // x bytes padding added after data member

static std::map<std::string, CacheItem> _CacheItems;
static critical_section _CacheLock;

/// <summary>
/// Initializes the cache.
/// </summary>
void CacheInit()
{
#ifdef USE_STD_THREAD
    Cache_Thread = new std::thread(cache_run);
#else
    _CacheThread = new CacheThread;
    _CacheThread->start();
#endif
}

/// <summary>
/// Disposes the cache.
/// </summary>
void CacheDispose()
{
#ifdef USE_STD_THREAD
    Cache_Running = false;
    Cache_Thread->join();
    delete Cache_Thread;
    Cache_Thread = NULL;
#else
    delete _CacheThread;
    _CacheThread = NULL;
#endif

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
HSOUNDFONT CacheAddSoundFont(const char * filePath)
{
    HSOUNDFONT hSoundFont = 0;

    insync(_CacheLock);

    CacheItem & Item = _CacheItems[filePath];

    if (Item._hSoundFont == 0)
    {
        const char * FilePath = (::stricmp_utf8_partial(filePath, "file://") == 0) ? filePath + 7 : filePath;

        if (::strstr(FilePath, "://") != nullptr)
            return 0;

        hSoundFont = ::BASS_MIDI_FontInit(pfc::stringcvt::string_wide_from_utf8(FilePath), 0);

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
    insync(_CacheLock);

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
sflist_presets * CacheAddSoundFontList(const char * filePath)
{
    sflist_presets * SoundFontList = nullptr;

    insync(_CacheLock);

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
void CacheRemoveSoundFontList(sflist_presets * soundFontList)
{
    insync(_CacheLock);

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

    insync(_CacheLock);

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
            sflist_presets * SoundFontList = it->second._SoundFontList;

            for (unsigned int i = 0, j = SoundFontList->count; i < j; ++i)
            {
                HSOUNDFONT hSoundFont = SoundFontList->presets[i].font;

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
static sflist_presets * LoadSoundFontList(const char * filePath)
{
    size_t Offset = (size_t)(::stricmp_utf8_partial(filePath, "file://") == 0 ? 7 : 0);

    pfc::string8 DirectoryPath = filePath + Offset;

    DirectoryPath.truncate(DirectoryPath.scan_filename());

    pfc::string8 FilePath = "";

    if (Offset == 0)
        FilePath = "file://";

    FilePath += filePath;

    char * Data = nullptr;

    try
    {
        abort_callback_dummy AbortHandler;

        file::ptr f;

        filesystem::g_open_read(f, FilePath, AbortHandler);

        size_t Size = (size_t)f->get_size_ex(AbortHandler);

        Data = (char *) ::malloc(Size + 1);

        if (Data == nullptr)
            return 0;

        f->read_object(Data, Size, AbortHandler);

        {
            Data[Size] = '\0';

            char ErrorMessage[sflist_max_error];

            sflist_presets * SoundFontList = ::sflist_load(Data, ::strlen(Data), DirectoryPath, ErrorMessage);

            ::free(Data);

            return SoundFontList;
        }
    }
    catch (...)
    {
        if (Data)
            ::free(Data);

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
            insync(_CacheLock);

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
