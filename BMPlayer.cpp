
/** BASS Player **/

#include "BMPlayer.h"

#include <SFList.h>

#include <string>

#include <map>
#include <time.h>

#undef USE_STD_THREAD
#ifdef USE_STD_THREAD
#include <thread>
#endif

#pragma region("Cache")

struct Cached_SoundFont
{
    HSOUNDFONT hSoundFont;
    unsigned long _ReferenceCount;

    PresetArray * _Presets;
    time_t _TimeReleased;

    Cached_SoundFont() : hSoundFont(0), _Presets(nullptr)
    {
    }
};

static critical_section _CacheLock;

static std::map<std::string, Cached_SoundFont> _CacheList;

static bool _IsCacheRunning = false;

static void CacheRun();

#ifdef USE_STD_THREAD
static std::thread * Cache_Thread = NULL;
#else
class Cache_Thread : public pfc::thread
{
public:
    virtual void threadProc()
    {
        CacheRun();
    }

    virtual ~Cache_Thread()
    {
        _IsCacheRunning = false;
        waitTillDone();
    }
};

static Cache_Thread * _CacheThread = nullptr;
#endif

static void CacheInit()
{
#ifdef USE_STD_THREAD
    Cache_Thread = new std::thread(cache_run);
#else
    _CacheThread = new Cache_Thread;
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

    for (auto it = _CacheList.begin(); it != _CacheList.end(); ++it)
    {
        if (it->second.hSoundFont)
            BASS_MIDI_FontFree(it->second.hSoundFont);

        if (it->second._Presets)
            DeletePresetArray(it->second._Presets);
    }
}

static void CacheRun()
{
    _IsCacheRunning = true;

    while (_IsCacheRunning)
    {
        time_t now;

        time(&now);

        {
            insync(_CacheLock);

            for (auto it = _CacheList.begin(); it != _CacheList.end();)
            {
                if (it->second._ReferenceCount == 0)
                {
                    if (::difftime(now, it->second._TimeReleased) >= 10.0)
                    {
                        if (it->second.hSoundFont)
                            BASS_MIDI_FontFree(it->second.hSoundFont);

                        if (it->second._Presets)
                            DeletePresetArray(it->second._Presets);

                        it = _CacheList.erase(it);
                        continue;
                    }
                }

                ++it;
            }
        }

        ::Sleep(250);
    }
}

#pragma endregion

#pragma region("BASS Initializer")

#pragma warning(disable: 4820) // x bytes padding added after data member
static class Bass_Initializer
{
public:
    Bass_Initializer() : _IsInitialized(false)
    {
    }

    ~Bass_Initializer()
    {
        if (_IsInitialized)
            CacheDispose();
    }

    bool check_initialized()
    {
        insync(_InitializerLock);

        return _IsInitialized;
    }

    void set_base_path()
    {
        base_path = core_api::get_my_full_path();

        size_t slash = base_path.find_last_of('\\');

        base_path.erase(base_path.begin() + (const __int64)(slash + 1), base_path.end());
    }

    void load_plugin(const char * name)
    {
        std::string full_path = base_path;

        full_path += name;

        BASS_PluginLoad((const char *) pfc::stringcvt::string_os_from_utf8(full_path.c_str()).get_ptr(), BASS_UNICODE);
    }

    bool initialize()
    {
        insync(_InitializerLock);

        if (!_IsInitialized)
        {
        #ifdef SF2PACK
            set_base_path();

            load_plugin("bassflac.dll");
            load_plugin("basswv.dll");
            load_plugin("bassopus.dll");
            load_plugin("bass_mpc.dll");
        #endif
            BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 0);
            BASS_SetConfig(BASS_CONFIG_UPDATETHREADS, 0);

            _IsInitialized = !!BASS_Init(0, 44100, 0, NULL, NULL);

            if (!_IsInitialized)
                _IsInitialized = _Already = (BASS_ErrorGetCode() == BASS_ERROR_ALREADY);

            if (_IsInitialized)
            {
                BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, (const void *) NULL);
                BASS_SetConfig(BASS_CONFIG_MIDI_VOICES, 256);

                CacheInit();
            }
        }

        return _IsInitialized;
    }

private:
    critical_section _InitializerLock;

    std::string base_path;

    bool _IsInitialized;
    bool _Already;
} _BASSInitializer;
#pragma warning(default: 4820) // x bytes padding added after data member

#pragma endregion

#pragma region("SoundFont")

static HSOUNDFONT cache_open_font(const char * filePath)
{
    HSOUNDFONT font = NULL;

    insync(_CacheLock);

    Cached_SoundFont & entry = _CacheList[filePath];

    if (!entry.hSoundFont)
    {
        if ((!stricmp_utf8_partial(filePath, "file://") || !strstr(filePath, "://")))
        {
            size_t path_offset = (size_t)(!stricmp_utf8_partial(filePath, "file://") ? 7 : 0);

            font = BASS_MIDI_FontInit(pfc::stringcvt::string_wide_from_utf8(filePath + path_offset), 0);
        }
        else
        {
            return 0;
        }

        if (font)
        {
            entry.hSoundFont = font;
            entry._ReferenceCount = 1;
        }
        else
        {
            _CacheList.erase(filePath);
        }
    }
    else
    {
        font = entry.hSoundFont;
        ++entry._ReferenceCount;
    }

    return font;
}

static PresetArray * sflist_open_file(const char * url)
{
    size_t PathOffset = (size_t)(!stricmp_utf8_partial(url, "file://") ? 7 : 0);

    pfc::string8 DirectoryPath = url + PathOffset;

    DirectoryPath.truncate(DirectoryPath.scan_filename());

    pfc::string8 FilePath = "";

    if (PathOffset == 0)
        FilePath = "file://";

    FilePath += url;

    char * Data = nullptr;

    try
    {
        abort_callback_dummy abortHandler;
        file::ptr File;

        filesystem::g_open_read(File, FilePath, abortHandler);

        size_t Size = File->get_size_ex(abortHandler);

        Data = (char *) ::malloc(Size + 1);

        if (Data)
        {
            File->read_object(Data, Size, abortHandler);

            Data[Size] = '\0';

            char error[sflist_max_error];

            PresetArray * presetlist = LoadPresetArray(Data, Size, DirectoryPath, error);

            ::free(Data);

            return presetlist;
        }
        else
            return 0;
    }
    catch (...)
    {
        if (Data)
            ::free(Data);

        return 0;
    }
}

static PresetArray * cache_open_list(const char * path)
{
    PresetArray * presetlist = nullptr;

    insync(_CacheLock);

    Cached_SoundFont & entry = _CacheList[path];

    if (!entry._Presets)
    {
        if ((!stricmp_utf8_partial(path, "file://") || !strstr(path, "://")))
        {
            presetlist = sflist_open_file(path);
        }
        else
        {
            return 0;
        }

        if (presetlist)
        {
            entry._Presets = presetlist;
            entry._ReferenceCount = 1;
        }
        else
        {
            _CacheList.erase(path);
        }
    }
    else
    {
        presetlist = entry._Presets;
        ++entry._ReferenceCount;
    }

    return presetlist;
}

static void cache_close_font(HSOUNDFONT handle)
{
    insync(_CacheLock);

    for (auto it = _CacheList.begin(); it != _CacheList.end(); ++it)
    {
        if (it->second.hSoundFont == handle)
        {
            if (--it->second._ReferenceCount == 0)
                time(&it->second._TimeReleased);
            break;
        }
    }
}

static void cache_close_list(PresetArray * presetlist)
{
    insync(_CacheLock);

    for (auto it = _CacheList.begin(); it != _CacheList.end(); ++it)
    {
        if (it->second._Presets == presetlist)
        {
            if (--it->second._ReferenceCount == 0)
                time(&it->second._TimeReleased);
            break;
        }
    }
}

struct has_font
{
    int has;

    has_font() : has(0)
    {
    }
};

static void cache_get_stats(uint64_t & total_sample_size, uint64_t & samples_loaded_size)
{
    std::map<HSOUNDFONT, has_font> uniqueList;

    insync(_CacheLock);

    total_sample_size = 0;
    samples_loaded_size = 0;

    for (auto it = _CacheList.begin(); it != _CacheList.end(); ++it)
    {
        BASS_MIDI_FONTINFO info;

        if (it->second.hSoundFont)
        {
            has_font & h = uniqueList[it->second.hSoundFont];

            if (!h.has)
            {
                h.has = 1;

                if (BASS_MIDI_FontGetInfo(it->second.hSoundFont, &info))
                {
                    total_sample_size += info.samsize;
                    samples_loaded_size += info.samload;
                }
            }
        }
        else
        if (it->second._Presets)
        {
            PresetArray * presetlist = it->second._Presets;

            for (unsigned int i = 0, j = presetlist->Count; i < j; ++i)
            {
                HSOUNDFONT hfont = presetlist->Items[i].font;

                has_font& h = uniqueList[hfont];

                if (!h.has)
                {
                    h.has = 1;

                    if (BASS_MIDI_FontGetInfo(hfont, &info))
                    {
                        total_sample_size += info.samsize;
                        samples_loaded_size += info.samload;
                    }
                }
            }
        }
    }
}

bool GetSoundFontStatistics(uint64_t & total_sample_size, uint64_t & samples_loaded_size)
{
    if (!_BASSInitializer.check_initialized())
        return false;

    cache_get_stats(total_sample_size, samples_loaded_size);

    return true;
}

#pragma endregion

#pragma region("BASS Player")
BMPlayer::BMPlayer() : MIDIPlayer()
{
    ::memset(_stream, 0, sizeof(_stream));

    _InterpolationLevel = 0;
    _AreEffectsEnabled = true;
    _VoiceCount = 256;
    _presetList[0] = 0;
    _presetList[1] = 0;

    if (!_BASSInitializer.initialize())
        throw std::runtime_error("Unable to initialize BASS");
}

BMPlayer::~BMPlayer()
{
    shutdown();
}

void BMPlayer::SetSoundFontDirectory(const char * directoryPath)
{
    _SoundFontDirectoryPath = directoryPath;
    shutdown();
}

void BMPlayer::SetSoundFontFile(const char * filePath)
{
    _SoundFontFilePath = filePath;
    shutdown();
}

void BMPlayer::SetInterpolation(int interpolationLevel)
{
    _InterpolationLevel = interpolationLevel;

    shutdown();
}

void BMPlayer::SetEffects(bool enableEffects)
{
    _AreEffectsEnabled = enableEffects;

    shutdown();
}

void BMPlayer::SetVoices(int voiceCount)
{
    if (voiceCount < 1)
        voiceCount = 1;
    else
    if (voiceCount > 100000)
        voiceCount = 100000;

    _VoiceCount = voiceCount;

    if (_stream[0])
        BASS_ChannelSetAttribute(_stream[0], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);

    if (_stream[1])
        BASS_ChannelSetAttribute(_stream[1], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);

    if (_stream[2])
        BASS_ChannelSetAttribute(_stream[2], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);
}

unsigned int BMPlayer::getVoicesActive()
{
    unsigned int VoiceCount = 0;

    float Voices;

    if (_stream[0])
    {
        Voices = 0.;

        if (BASS_ChannelGetAttribute(_stream[0], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (unsigned int) (int) (Voices);
    }

    if (_stream[1])
    {
        Voices = 0.;

        if (BASS_ChannelGetAttribute(_stream[1], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (unsigned int) (int) (Voices);
    }

    if (_stream[2])
    {
        Voices = 0.;

        if (BASS_ChannelGetAttribute(_stream[2], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (unsigned int) (int) (Voices);
    }

    return VoiceCount;
}

#pragma region("Private")
bool BMPlayer::startup()
{
    if (_stream[0] && _stream[1] && _stream[2])
        return true;

    _stream[0] = BASS_MIDI_StreamCreate(16, (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_AreEffectsEnabled ? 0 : BASS_MIDI_NOFX)), (unsigned int) _SampleRate);
    _stream[1] = BASS_MIDI_StreamCreate(16, (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_AreEffectsEnabled ? 0 : BASS_MIDI_NOFX)), (unsigned int) _SampleRate);
    _stream[2] = BASS_MIDI_StreamCreate(16, (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_AreEffectsEnabled ? 0 : BASS_MIDI_NOFX)), (unsigned int) _SampleRate);

    if (!_stream[0] || !_stream[1] || !_stream[2])
        return false;

    BASS_ChannelSetAttribute(_stream[0], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationLevel);
    BASS_ChannelSetAttribute(_stream[1], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationLevel);
    BASS_ChannelSetAttribute(_stream[2], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationLevel);

    SetVoices(_VoiceCount);

    ::memset(bank_lsb_override, 0, sizeof(bank_lsb_override));

    {
        std::vector<BASS_MIDI_FONTEX> Presets;

        if (_SoundFontFilePath.length())
        {
            if (LoadSoundFont(Presets, _SoundFontFilePath) == 0)
                return false;
        }

        if (_SoundFontDirectoryPath.length())
        {
            if (LoadSoundFont(Presets, _SoundFontDirectoryPath) == 0)
                return false;
        }

        BASS_MIDI_StreamSetFonts(_stream[0], &Presets[0], (DWORD) Presets.size() | BASS_MIDI_FONT_EX);
        BASS_MIDI_StreamSetFonts(_stream[1], &Presets[0], (DWORD) Presets.size() | BASS_MIDI_FONT_EX);
        BASS_MIDI_StreamSetFonts(_stream[2], &Presets[0], (DWORD) Presets.size() | BASS_MIDI_FONT_EX);
    }

    reset_parameters();

    _IsInitialized = true;

    SetFilter(_FilterType, _UseMIDIEffects);

    return true;
}

void BMPlayer::shutdown()
{
    if (_stream[2])
        BASS_StreamFree(_stream[2]);

    if (_stream[1])
        BASS_StreamFree(_stream[1]);

    if (_stream[0])
        BASS_StreamFree(_stream[0]);

    ::memset(_stream, 0, sizeof(_stream));

    for (unsigned long i = 0; i < _SoundFonts.size(); ++i)
    {
        cache_close_font(_SoundFonts[i]);
    }

    _SoundFonts.resize(0);

    if (_presetList[0])
    {
        cache_close_list(_presetList[0]);
        _presetList[0] = 0;
    }

    if (_presetList[1])
    {
        cache_close_list(_presetList[1]);
        _presetList[1] = 0;
    }

    _IsInitialized = false;
}

void BMPlayer::send_event(uint32_t b)
{
    unsigned char event[3]
    {
        static_cast<uint8_t>(b),
        static_cast<uint8_t>(b >> 8),
        static_cast<uint8_t>(b >> 16)
    };

    unsigned port = (b >> 24) & 0x7F;
//  const unsigned channel = b & 0x0F;
    const unsigned command = b & 0xF0;
    const unsigned event_length = (unsigned int)((command >= 0xF8 && command <= 0xFF) ? 1 : ((command == 0xC0 || command == 0xD0) ? 2 : 3));

    if (port > 2)
        port = 0;

    if (bank_lsb_overridden && command == 0xB0 && event[1] == 0x20)
        return;

    BASS_MIDI_StreamEvents(_stream[port], BASS_MIDI_EVENTS_RAW, event, event_length);
}

void BMPlayer::send_sysex(const uint8_t * event, size_t size, size_t port)
{
    if (port > 2)
        port = 0;

    BASS_MIDI_StreamEvents(_stream[port], BASS_MIDI_EVENTS_RAW, event, static_cast<unsigned int>(size));

    if (port == 0)
    {
        BASS_MIDI_StreamEvents(_stream[1], BASS_MIDI_EVENTS_RAW, event, static_cast<unsigned int>(size));
        BASS_MIDI_StreamEvents(_stream[2], BASS_MIDI_EVENTS_RAW, event, static_cast<unsigned int>(size));
    }
}

void BMPlayer::render(audio_sample * out, unsigned long count)
{
    float buffer[512 * 2];

    while (count)
    {
        size_t ToDo = (size_t)count;

        if (ToDo > 512)
            ToDo = 512;

        ::memset(out, 0, (ToDo * 2) * sizeof(audio_sample));

        for (int i = 0; i < 3; ++i)
        {
            BASS_ChannelGetData(_stream[i], buffer, BASS_DATA_FLOAT | static_cast<unsigned int>((ToDo * 2) * sizeof(float)));

            for (unsigned long j = 0; j < (ToDo * 2); ++j)
            {
                out[j] += (audio_sample) buffer[j];
            }
        }

        out += (ToDo * 2);
        count -= (unsigned long)ToDo;
    }
}

void BMPlayer::compound_presets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels)
{
    if (!in.size())
    {
        BASS_MIDI_FONTEX fex = { 0, -1, -1, -1, 0, 0 };
        in.push_back(fex);
    }

    if (channels.size())
    {
        for (auto pit = in.begin(); pit != in.end(); ++pit)
        {
            for (auto it = channels.begin(); it != channels.end(); ++it)
            {
                bank_lsb_override[*it - 1] = (uint8_t)*it;

                int dbanklsb = (int) *it;
                pit->dbanklsb = dbanklsb;

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

bool BMPlayer::LoadSoundFont(std::vector<BASS_MIDI_FONTEX> & presets, std::string filePath)
{
    std::string Extension;

    size_t dot = filePath.find_last_of('.');

    if (dot != std::string::npos)
        Extension.assign(filePath.begin() + (const __int64)(dot + 1), filePath.end());

    if ((::stricmp_utf8(Extension.c_str(), "sf2") == 0) || (::stricmp_utf8(Extension.c_str(), "sf3") == 0)
    #ifdef SF2PACK
        || (::stricmp_utf8(Extension.c_str(), "sf2pack") == 0) || (::stricmp_utf8(Extension.c_str(), "sfogg") == 0)
    #endif
        )
    {
        HSOUNDFONT hSoundFont = cache_open_font(filePath.c_str());

        if (hSoundFont == 0)
        {
            shutdown();

            sLastError = "Unable to load SoundFont: ";
            sLastError += filePath.c_str();

            return false;
        }

        _SoundFonts.push_back(hSoundFont);

        BASS_MIDI_FONTEX fex = { hSoundFont, -1, -1, -1, 0, 0 };

        presets.push_back(fex);

        return true;
    }

    if ((::stricmp_utf8(Extension.c_str(), "sflist") == 0) || (::stricmp_utf8(Extension.c_str(), "json") == 0))
    {
        PresetArray ** __Presets = &_presetList[0];

        if (*__Presets)
            __Presets = &_presetList[1];

        *__Presets = cache_open_list(filePath.c_str());

        if (!*__Presets)
            return false;

        BASS_MIDI_FONTEX * fontex = (*__Presets)->Items;

        for (unsigned int i = 0, j = (*__Presets)->Count; i < j; ++i)
            presets.push_back(fontex[i]);

        return true;
    }

    return false;
}

void BMPlayer::reset_parameters()
{
    bank_lsb_overridden = false;

    for (unsigned int i = 0; i < 48; ++i)
    {
        if (bank_lsb_override[i])
            bank_lsb_overridden = true;

        BASS_MIDI_StreamEvent(_stream[i / 16], i % 16, MIDI_EVENT_BANK_LSB, bank_lsb_override[i]);
    }
}

bool BMPlayer::get_last_error(std::string & p_out)
{
    if (sLastError.length())
    {
        p_out = sLastError;
        return true;
    }

    return false;
}
#pragma endregion

#pragma endregion

