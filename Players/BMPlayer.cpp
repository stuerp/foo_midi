
/** $VER: BMPlayer.cpp (2024.05.05) **/

#include "framework.h"

#include "BMPlayer.h"

#include <sflist.h>

#include <string>

#include <map>
#include <time.h>

#undef USE_STD_THREAD
#ifdef USE_STD_THREAD
#include <thread>
#endif

#pragma region("Cache")

#pragma warning(disable: 4820)
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
#pragma warning(default: 4820)

static critical_section _CacheLock;

static std::map<std::string, CacheItem> _CacheItems;

static bool _IsCacheRunning = false;

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

static void CacheInit()
{
#ifdef USE_STD_THREAD
    Cache_Thread = new std::thread(cache_run);
#else
    _CacheThread = new CacheThread;
    _CacheThread->start();
#endif
}

static void CacheDispose()
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

/// <summary>
/// Adds a SoundFont to the cache.
/// </summary>
static HSOUNDFONT CacheAddSoundFont(const char * filePath)
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
static void CacheRemoveSoundFont(HSOUNDFONT hSoundFont)
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

/// <summary>
/// Adds a SoundFont List to the cache.
/// </summary>
static sflist_presets * CacheAddSoundFontList(const char * filePath)
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
static void CacheRemoveSoundFontList(sflist_presets * soundFontList)
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
static void CacheGetStatistics(uint64_t & totalSampleDataSize, uint64_t & totalSamplesDataLoaded)
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

#pragma endregion

#pragma region("BASS Initializer")

#pragma warning(disable: 4820) // x bytes padding added after data member
static class BASSInitializer
{
public:
    BASSInitializer() : _IsInitialized(false)
    {
    }

    ~BASSInitializer()
    {
        if (_IsInitialized)
            CacheDispose();
    }

    bool IsInitialized()
    {
        insync(_Lock);

        return _IsInitialized;
    }

    bool Initialize()
    {
        insync(_Lock);

        if (!_IsInitialized)
        {
        #ifdef SF2PACK
            SetBasePath();

            LoadPlugIn("bassflac.dll");
            LoadPlugIn("basswv.dll");
            LoadPlugIn("bassopus.dll");
            LoadPlugIn("bass_mpc.dll");
        #endif
            ::BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
            ::BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

            _IsInitialized = !!BASS_Init(0, 44100, 0, 0, NULL);

            if (!_IsInitialized)
                _IsInitialized = (::BASS_ErrorGetCode() == BASS_ERROR_ALREADY);

            if (_IsInitialized)
            {
                ::BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, (const void *) 0);
                ::BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, 256);

                CacheInit();
            }
        }

        return _IsInitialized;
    }

    void SetBasePath()
    {
        _BasePath = core_api::get_my_full_path();

        size_t slash = _BasePath.find_last_of('\\');

        _BasePath.erase(_BasePath.begin() + (const __int64)(slash + 1), _BasePath.end());
    }

    void LoadPlugIn(const char * fileName)
    {
        std::string PathName = _BasePath;

        PathName += fileName;

        ::BASS_PluginLoad((const char *) pfc::stringcvt::string_os_from_utf8(PathName.c_str()).get_ptr(), BASS_UNICODE);
    }

private:
    critical_section _Lock;

    std::string _BasePath;

    bool _IsInitialized;
} _BASSInitializer;
#pragma warning(default: 4820) // x bytes padding added after data member

#pragma endregion

#pragma region("BASS MIDI")
BMPlayer::BMPlayer() : MIDIPlayer()
{
    ::memset(_Stream, 0, sizeof(_Stream));

    _InterpolationMode = 0;
    _DoReverbAndChorusProcessing = true;
    _VoiceCount = 256;
    _Presets[0] = 0;
    _Presets[1] = 0;

    if (!_BASSInitializer.Initialize())
        throw std::runtime_error("Unable to initialize BASS MIDI");
}

BMPlayer::~BMPlayer()
{
    Shutdown();
}

void BMPlayer::SetSoundFontDirectory(const char * directoryPath)
{
    if (_SoundFontDirectoryPath == directoryPath)
        return;

    _SoundFontDirectoryPath = directoryPath;

    Shutdown();
}

void BMPlayer::SetSoundFontFile(const char * filePath)
{
    if (_SoundFontFilePath == filePath)
        return;

    _SoundFontFilePath = filePath;

    Shutdown();
}

void BMPlayer::SetInterpolationMode(uint32_t interpolationMode)
{
    if (_InterpolationMode == interpolationMode)
        return;

    _InterpolationMode = interpolationMode;

    Shutdown();
}

void BMPlayer::EnableEffects(bool enabled)
{
    if (_DoReverbAndChorusProcessing == enabled)
        return;

    _DoReverbAndChorusProcessing = enabled;

    Shutdown();
}

void BMPlayer::SetVoiceCount(uint32_t voiceCount)
{
    if (voiceCount < 1)
        voiceCount = 1;
    else
    if (voiceCount > 100000)
        voiceCount = 100000;

    _VoiceCount = voiceCount;

    if (_Stream[0])
        ::BASS_ChannelSetAttribute(_Stream[0], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);

    if (_Stream[1])
        ::BASS_ChannelSetAttribute(_Stream[1], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);

    if (_Stream[2])
        ::BASS_ChannelSetAttribute(_Stream[2], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);
}

uint32_t BMPlayer::GetActiveVoiceCount() const noexcept
{
    uint32_t VoiceCount = 0;

    float Voices;

    if (_Stream[0])
    {
        Voices = 0.;

        if (::BASS_ChannelGetAttribute(_Stream[0], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (size_t) (int) (Voices);
    }

    if (_Stream[1])
    {
        Voices = 0.;

        if (::BASS_ChannelGetAttribute(_Stream[1], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (size_t) (int) (Voices);
    }

    if (_Stream[2])
    {
        Voices = 0.;

        if (::BASS_ChannelGetAttribute(_Stream[2], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (size_t) (int) (Voices);
    }

    return VoiceCount;
}

#pragma region("Private")
bool BMPlayer::Startup()
{
    if (_Stream[0] && _Stream[1] && _Stream[2])
        return true;

    const DWORD Flags = (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_DoReverbAndChorusProcessing ? 0 : BASS_MIDI_NOFX));

    _Stream[0] = ::BASS_MIDI_StreamCreate(16, Flags, (DWORD) _SampleRate);
    _Stream[1] = ::BASS_MIDI_StreamCreate(16, Flags, (DWORD) _SampleRate);
    _Stream[2] = ::BASS_MIDI_StreamCreate(16, Flags, (DWORD) _SampleRate);

    if (!_Stream[0] || !_Stream[1] || !_Stream[2])
        return false;

    ::BASS_ChannelSetAttribute(_Stream[0], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationMode);
    ::BASS_ChannelSetAttribute(_Stream[1], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationMode);
    ::BASS_ChannelSetAttribute(_Stream[2], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationMode);

    ::BASS_ChannelSetAttribute(_Stream[0], BASS_ATTRIB_VOLDSP, (float) .5f);
    ::BASS_ChannelSetAttribute(_Stream[1], BASS_ATTRIB_VOLDSP, (float) .5f);
    ::BASS_ChannelSetAttribute(_Stream[2], BASS_ATTRIB_VOLDSP, (float) .5f);

    SetVoiceCount(_VoiceCount);

    ::memset(_BankLSBOverride, 0, sizeof(_BankLSBOverride));

    std::vector<BASS_MIDI_FONTEX> SoundFontConfigurations;

    if (_SoundFontFilePath.length())
    {
        if (!LoadSoundFontConfiguration(SoundFontConfigurations, _SoundFontFilePath))
            return false;
    }

    if (_SoundFontDirectoryPath.length())
    {
        if (!LoadSoundFontConfiguration(SoundFontConfigurations, _SoundFontDirectoryPath))
            return false;
    }

    ::BASS_MIDI_StreamSetFonts(_Stream[0], SoundFontConfigurations.data(), (unsigned int) SoundFontConfigurations.size() | BASS_MIDI_FONT_EX);
    ::BASS_MIDI_StreamSetFonts(_Stream[1], SoundFontConfigurations.data(), (unsigned int) SoundFontConfigurations.size() | BASS_MIDI_FONT_EX);
    ::BASS_MIDI_StreamSetFonts(_Stream[2], SoundFontConfigurations.data(), (unsigned int) SoundFontConfigurations.size() | BASS_MIDI_FONT_EX);

    ResetParameters();

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void BMPlayer::Shutdown()
{
    if (_Stream[2])
    {
        ::BASS_StreamFree(_Stream[2]);
        _Stream[2] = 0;
    }

    if (_Stream[1])
    {
        ::BASS_StreamFree(_Stream[1]);
        _Stream[1] = 0;
    }

    if (_Stream[0])
    {
        ::BASS_StreamFree(_Stream[0]);
        _Stream[0] = 0;
    }

    for (auto SoundFont : _SoundFonts)
        CacheRemoveSoundFont(SoundFont);

    _SoundFonts.resize(0);

    if (_Presets[0])
    {
        CacheRemoveSoundFontList(_Presets[0]);
        _Presets[0] = 0;
    }

    if (_Presets[1])
    {
        CacheRemoveSoundFontList(_Presets[1]);
        _Presets[1] = 0;
    }

    _IsInitialized = false;
}

void BMPlayer::SendEvent(uint32_t message)
{
    uint8_t Event[3]
    {
        static_cast<uint8_t>(message),
        static_cast<uint8_t>(message >>  8),
        static_cast<uint8_t>(message >> 16)
    };

    const uint8_t Status = message & 0xF0;

    if (_IsBankLSBOverridden && (Status == 0xB0) && (Event[1] == 0x20))
        return;

    const DWORD EventSize = (DWORD)((Status >= 0xF8 && Status <= 0xFF) ? 1 : ((Status == 0xC0 || Status == 0xD0) ? 2 : 3));

    uint8_t Port = (message >> 24) & 0x7F;

    if (Port > 2)
        Port = 0;

    ::BASS_MIDI_StreamEvents(_Stream[Port], BASS_MIDI_EVENTS_RAW, Event, EventSize);
}

void BMPlayer::SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber)
{
    if (portNumber > 2)
        portNumber = 0;

    ::BASS_MIDI_StreamEvents(_Stream[portNumber], BASS_MIDI_EVENTS_RAW, event, (DWORD) size);

    if (portNumber == 0)
    {
        ::BASS_MIDI_StreamEvents(_Stream[1], BASS_MIDI_EVENTS_RAW, event, (DWORD) size);
        ::BASS_MIDI_StreamEvents(_Stream[2], BASS_MIDI_EVENTS_RAW, event, (DWORD) size);
    }
}

void BMPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    while (sampleCount != 0)
    {
        size_t ToDo = std::min(sampleCount, MaxSamples);

        ::memset(sampleData, 0, (size_t) ToDo * ChannelCount * sizeof(audio_sample));

        for (size_t i = 0; i < _countof(_Stream); ++i)
        {
            ::BASS_ChannelGetData(_Stream[i], _Buffer, BASS_DATA_FLOAT | (DWORD) (ToDo * ChannelCount * sizeof(float)));

            // Convert the format of the rendered output.
            for (size_t j = 0; j < ToDo * ChannelCount; ++j)
                sampleData[j] += _Buffer[j];
        }

        sampleData  += (ToDo * ChannelCount);
        sampleCount -= (uint32_t) ToDo;
    }
}

void BMPlayer::CompoundPresets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels)
{
    if (in.size() == 0)
    {
        BASS_MIDI_FONTEX fex = { 0, -1, -1, -1, 0, 0 };

        in.push_back(fex);
    }

    if (channels.size() > 0)
    {
        for (auto pit = in.begin(); pit != in.end(); ++pit)
        {
            for (auto cit = channels.begin(); cit != channels.end(); ++cit)
            {
                _BankLSBOverride[*cit - 1] = (uint8_t)*cit;

                int DestinationBankLSB = (int) *cit;

                pit->dbanklsb = DestinationBankLSB;

                out.push_back(*pit);
            }
        }
    }
    else
    {
        for (auto pit = in.begin(); pit != in.end(); ++pit)
        {
            out.push_back(*pit);
        }
    }
}

bool BMPlayer::LoadSoundFontConfiguration(std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations, std::string pathName)
{
    std::string FileExtension;

    size_t dot = pathName.find_last_of('.');

    if (dot != std::string::npos)
        FileExtension.assign(pathName.begin() + (const __int64)(dot + 1), pathName.end());

    if ((::stricmp_utf8(FileExtension.c_str(), "sf2") == 0) || (::stricmp_utf8(FileExtension.c_str(), "sf3") == 0)
    #ifdef SF2PACK
        || (::stricmp_utf8(FileExtension.c_str(), "sf2pack") == 0) || (::stricmp_utf8(FileExtension.c_str(), "sfogg") == 0)
    #endif
        )
    {
        HSOUNDFONT hSoundFont = CacheAddSoundFont(pathName.c_str());

        if (hSoundFont == 0)
        {
            Shutdown();

            _ErrorMessage = "Unable to load SoundFont: ";
            _ErrorMessage += pathName.c_str();

            return false;
        }

        _SoundFonts.push_back(hSoundFont);

        BASS_MIDI_FONTEX fex = { hSoundFont, -1, -1, -1, 0, 0 };

        soundFontConfigurations.push_back(fex);

        return true;
    }

    if ((::stricmp_utf8(FileExtension.c_str(), "sflist") == 0) || (::stricmp_utf8(FileExtension.c_str(), "json") == 0))
    {
        sflist_presets ** __presetList = &_Presets[0];

        if (*__presetList)
            __presetList = &_Presets[1];

        *__presetList = CacheAddSoundFontList(pathName.c_str());

        if (!*__presetList)
            return false;

        BASS_MIDI_FONTEX * fex = (*__presetList)->presets;

        for (size_t i = 0, j = (*__presetList)->count; i < j; ++i)
            soundFontConfigurations.push_back(fex[i]);

        return true;
    }

    return false;
}

void BMPlayer::ResetParameters()
{
    _IsBankLSBOverridden = false;

    for (size_t i = 0; i < _countof(_BankLSBOverride); ++i)
    {
        if (_BankLSBOverride[i] != 0)
            _IsBankLSBOverridden = true;

        ::BASS_MIDI_StreamEvent(_Stream[i / 16], i % 16, MIDI_EVENT_BANK_LSB, _BankLSBOverride[i]);
    }
}

bool BMPlayer::GetErrorMessage(std::string & errorMessage)
{
    if (_ErrorMessage.length() == 0)
        return false;

    errorMessage = _ErrorMessage;

    return true;
}
#pragma endregion

bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded)
{
    if (!_BASSInitializer.IsInitialized())
        return false;

    CacheGetStatistics(sampleDataSize, sampleDataLoaded);

    return true;
}

#pragma endregion

