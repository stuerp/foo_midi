
/** BASS Player **/

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
struct Cached_SoundFont
{
    HSOUNDFONT hSoundFont;
    size_t _ReferenceCount;

    sflist_presets * _Presets;
    time_t _TimeReleased;

    Cached_SoundFont() : hSoundFont(0), _Presets(nullptr)
    {
    }
};
#pragma warning(default: 4820)

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
            ::BASS_MIDI_FontFree(it->second.hSoundFont);

        if (it->second._Presets)
            sflist_free(it->second._Presets);
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
                            ::BASS_MIDI_FontFree(it->second.hSoundFont);

                        if (it->second._Presets)
                            sflist_free(it->second._Presets);

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
        insync(_InitializerLock);

        return _IsInitialized;
    }

    bool initialize()
    {
        insync(_InitializerLock);

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

            _IsInitialized = !!BASS_Init(0, 44100, 0, NULL, NULL);

            if (!_IsInitialized)
                _IsInitialized = _IsAlreadyInitialized = (::BASS_ErrorGetCode() == BASS_ERROR_ALREADY);

            if (_IsInitialized)
            {
                ::BASS_SetConfigPtr(BASS_CONFIG_MIDI_DEFFONT, (const void *) NULL);
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
    critical_section _InitializerLock;

    std::string _BasePath;

    bool _IsInitialized;
    bool _IsAlreadyInitialized;
} _BASSInitializer;
#pragma warning(default: 4820) // x bytes padding added after data member

#pragma endregion

#pragma region("SoundFont")

static HSOUNDFONT cache_open_font(const char * filePath)
{
    HSOUNDFONT hSoundFont = 0;

    insync(_CacheLock);

    Cached_SoundFont & entry = _CacheList[filePath];

    if (!entry.hSoundFont)
    {
        if ((!stricmp_utf8_partial(filePath, "file://") || !strstr(filePath, "://")))
        {
            size_t path_offset = (size_t)(!stricmp_utf8_partial(filePath, "file://") ? 7 : 0);

            hSoundFont = ::BASS_MIDI_FontInit(pfc::stringcvt::string_wide_from_utf8(filePath + path_offset), 0);
        }
        else
            return 0;

        if (hSoundFont)
        {
            entry.hSoundFont = hSoundFont;
            entry._ReferenceCount = 1;
        }
        else
            _CacheList.erase(filePath);
    }
    else
    {
        hSoundFont = entry.hSoundFont;
        ++entry._ReferenceCount;
    }

    return hSoundFont;
}

static sflist_presets * sflist_open_file(const char * filePath)
{
    size_t path_offset = (size_t)(!stricmp_utf8_partial(filePath, "file://") ? 7 : 0);

    pfc::string8 base_path = filePath + path_offset;

    base_path.truncate(base_path.scan_filename());

    pfc::string8 our_path = "";

    if (!path_offset)
        our_path = "file://";

    our_path += filePath;

    char * sflist_file = nullptr;

    try
    {
        abort_callback_dummy m_abort;
        file::ptr f;

        filesystem::g_open_read(f, our_path, m_abort);

        size_t length = f->get_size_ex(m_abort);

        sflist_file = (char *) ::malloc(length + 1);

        if (!sflist_file)
            return 0;

        f->read_object(sflist_file, length, m_abort);

        sflist_file[length] = '\0';

        char error[sflist_max_error];

        sflist_presets * presetlist = sflist_load(sflist_file, strlen(sflist_file), base_path, error);

        ::free(sflist_file);

        return presetlist;
    }
    catch (...)
    {
        if (sflist_file)
            ::free(sflist_file);

        return 0;
    }
}

static sflist_presets * cache_open_list(const char * path)
{
    sflist_presets * presetlist = nullptr;

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

static void cache_close_font(HSOUNDFONT hSoundFont)
{
    insync(_CacheLock);

    for (auto it = _CacheList.begin(); it != _CacheList.end(); ++it)
    {
        if (it->second.hSoundFont == hSoundFont)
        {
            if (--it->second._ReferenceCount == 0)
                time(&it->second._TimeReleased);
            break;
        }
    }
}

static void cache_close_list(sflist_presets * presetlist)
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

                if (::BASS_MIDI_FontGetInfo(it->second.hSoundFont, &info))
                {
                    total_sample_size += info.samsize;
                    samples_loaded_size += info.samload;
                }
            }
        }
        else
        if (it->second._Presets)
        {
            sflist_presets * presetlist = it->second._Presets;

            for (unsigned int i = 0, j = presetlist->count; i < j; ++i)
            {
                HSOUNDFONT hfont = presetlist->presets[i].font;

                has_font& h = uniqueList[hfont];

                if (!h.has)
                {
                    h.has = 1;

                    if (::BASS_MIDI_FontGetInfo(hfont, &info))
                    {
                        total_sample_size += info.samsize;
                        samples_loaded_size += info.samload;
                    }
                }
            }
        }
    }
}

bool GetSoundFontStatistics(uint64_t & sizeTotal, uint64_t & sizeLoaded)
{
    if (!_BASSInitializer.IsInitialized())
        return false;

    cache_get_stats(sizeTotal, sizeLoaded);

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
    _Presets[0] = 0;
    _Presets[1] = 0;

    if (!_BASSInitializer.initialize())
        throw std::runtime_error("Unable to initialize BASS");
}

BMPlayer::~BMPlayer()
{
    shutdown();
}

void BMPlayer::setSoundFont(const char * directoryPath)
{
    _SoundFontDirectoryPath = directoryPath;
    shutdown();
}

void BMPlayer::setFileSoundFont(const char * filePath)
{
    _SoundFontFilePath = filePath;
    shutdown();
}

void BMPlayer::setInterpolation(int interpolationLevel)
{
    _InterpolationLevel = interpolationLevel;

    shutdown();
}

void BMPlayer::setEffects(bool enabled)
{
    _AreEffectsEnabled = enabled;

    shutdown();
}

void BMPlayer::setVoices(int voiceCount)
{
    if (voiceCount < 1)
        voiceCount = 1;
    else
    if (voiceCount > 100000)
        voiceCount = 100000;

    _VoiceCount = voiceCount;

    if (_stream[0])
        ::BASS_ChannelSetAttribute(_stream[0], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);

    if (_stream[1])
        ::BASS_ChannelSetAttribute(_stream[1], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);

    if (_stream[2])
        ::BASS_ChannelSetAttribute(_stream[2], BASS_ATTRIB_MIDI_VOICES, (float) voiceCount);
}

unsigned int BMPlayer::getVoicesActive()
{
    unsigned int VoiceCount = 0;

    float Voices;

    if (_stream[0])
    {
        Voices = 0.;

        if (::BASS_ChannelGetAttribute(_stream[0], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (unsigned int) (int) (Voices);
    }

    if (_stream[1])
    {
        Voices = 0.;

        if (::BASS_ChannelGetAttribute(_stream[1], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (unsigned int) (int) (Voices);
    }

    if (_stream[2])
    {
        Voices = 0.;

        if (::BASS_ChannelGetAttribute(_stream[2], BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
            VoiceCount += (unsigned int) (int) (Voices);
    }

    return VoiceCount;
}

#pragma region("Private")
bool BMPlayer::startup()
{
    if (_stream[0] && _stream[1] && _stream[2])
        return true;

    _stream[0] = ::BASS_MIDI_StreamCreate(16, (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_AreEffectsEnabled ? 0 : BASS_MIDI_NOFX)), (unsigned int) _SampleRate);
    _stream[1] = ::BASS_MIDI_StreamCreate(16, (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_AreEffectsEnabled ? 0 : BASS_MIDI_NOFX)), (unsigned int) _SampleRate);
    _stream[2] = ::BASS_MIDI_StreamCreate(16, (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_AreEffectsEnabled ? 0 : BASS_MIDI_NOFX)), (unsigned int) _SampleRate);

    if (!_stream[0] || !_stream[1] || !_stream[2])
        return false;

    ::BASS_ChannelSetAttribute(_stream[0], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationLevel);
    ::BASS_ChannelSetAttribute(_stream[1], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationLevel);
    ::BASS_ChannelSetAttribute(_stream[2], BASS_ATTRIB_MIDI_SRC, (float) _InterpolationLevel);

    setVoices(_VoiceCount);

    ::memset(bank_lsb_override, 0, sizeof(bank_lsb_override));

    std::vector<BASS_MIDI_FONTEX> presetList;

    if (_SoundFontFilePath.length())
    {
        if (!load_font_item(presetList, _SoundFontFilePath))
            return false;
    }

    if (_SoundFontDirectoryPath.length())
    {
        if (!load_font_item(presetList, _SoundFontDirectoryPath))
            return false;
    }

    ::BASS_MIDI_StreamSetFonts(_stream[0], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX);
    ::BASS_MIDI_StreamSetFonts(_stream[1], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX);
    ::BASS_MIDI_StreamSetFonts(_stream[2], &presetList[0], (unsigned int) presetList.size() | BASS_MIDI_FONT_EX);

    reset_parameters();

    _IsInitialized = true;

    SetFilter(_FilterType, _FilterEffects);

    return true;
}

void BMPlayer::shutdown()
{
    if (_stream[2])
    {
        ::BASS_StreamFree(_stream[2]);
        _stream[2] = 0;
    }

    if (_stream[1])
    {
        ::BASS_StreamFree(_stream[1]);
        _stream[1] = 0;
    }

    if (_stream[0])
    {
        ::BASS_StreamFree(_stream[0]);
        _stream[0] = 0;
    }

    for (size_t i = 0; i < _SoundFonts.size(); ++i)
        cache_close_font(_SoundFonts[i]);

    _SoundFonts.resize(0);

    if (_Presets[0])
    {
        cache_close_list(_Presets[0]);
        _Presets[0] = 0;
    }

    if (_Presets[1])
    {
        cache_close_list(_Presets[1]);
        _Presets[1] = 0;
    }

    _IsInitialized = false;
}

void BMPlayer::SendEvent(uint32_t b)
{
    unsigned char event[3]
    {
        static_cast<uint8_t>(b),
        static_cast<uint8_t>(b >> 8),
        static_cast<uint8_t>(b >> 16)
    };

    size_t port = (b >> 24) & 0x7F;

//  const unsigned channel = b & 0x0F;
    const unsigned command = b & 0xF0;
    const unsigned event_length = (unsigned int)((command >= 0xF8 && command <= 0xFF) ? 1 : ((command == 0xC0 || command == 0xD0) ? 2 : 3));

    if (port > 2)
        port = 0;

    if (bank_lsb_overridden && command == 0xB0 && event[1] == 0x20)
        return;

    ::BASS_MIDI_StreamEvents(_stream[port], BASS_MIDI_EVENTS_RAW, event, event_length);
}

void BMPlayer::SendSysEx(const uint8_t * event, size_t size, size_t port)
{
    if (port > 2)
        port = 0;

    ::BASS_MIDI_StreamEvents(_stream[port], BASS_MIDI_EVENTS_RAW, event, static_cast<unsigned int>(size));

    if (port == 0)
    {
        ::BASS_MIDI_StreamEvents(_stream[1], BASS_MIDI_EVENTS_RAW, event, static_cast<unsigned int>(size));
        ::BASS_MIDI_StreamEvents(_stream[2], BASS_MIDI_EVENTS_RAW, event, static_cast<unsigned int>(size));
    }
}

void BMPlayer::render(audio_sample * samples, unsigned long samplesSize)
{
    float Buffer[512 * 2];

    while (samplesSize > 0)
    {
        unsigned long ToDo = samplesSize;

        if (ToDo > 512)
            ToDo = 512;

        ::memset(samples, 0, (ToDo * 2) * sizeof(audio_sample));

        for (size_t i = 0; i < 3; ++i)
        {
            ::BASS_ChannelGetData(_stream[i], Buffer, BASS_DATA_FLOAT | static_cast<unsigned int>((ToDo * 2) * sizeof(float)));

            for (size_t j = 0; j < (ToDo * 2); ++j)
                samples[j] += (audio_sample) Buffer[j];
        }

        samples     += (ToDo * 2);
        samplesSize -= ToDo;
    }
}

void BMPlayer::compound_presets(std::vector<BASS_MIDI_FONTEX> & out, std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels)
{
    if (in.size() == 0)
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

bool BMPlayer::load_font_item(std::vector<BASS_MIDI_FONTEX> & presetList, std::string in_path)
{
    std::string ext;

    size_t dot = in_path.find_last_of('.');

    if (dot != std::string::npos)
        ext.assign(in_path.begin() + (const __int64)(dot + 1), in_path.end());

    if (!stricmp_utf8(ext.c_str(), "sf2") || !stricmp_utf8(ext.c_str(), "sf3")
    #ifdef SF2PACK
        || !stricmp_utf8(ext.c_str(), "sf2pack") || !stricmp_utf8(ext.c_str(), "sfogg")
    #endif
        )
    {
        HSOUNDFONT font = cache_open_font(in_path.c_str());

        if (!font)
        {
            shutdown();

            _ErrorMessage = "Unable to load SoundFont: ";
            _ErrorMessage += in_path.c_str();

            return false;
        }

        _SoundFonts.push_back(font);

        BASS_MIDI_FONTEX fex = { font, -1, -1, -1, 0, 0 };

        presetList.push_back(fex);

        return true;
    }
    else
    if (!stricmp_utf8(ext.c_str(), "sflist") || !stricmp_utf8(ext.c_str(), "json"))
    {
        sflist_presets ** __presetList = &_Presets[0];

        if (*__presetList)
            __presetList = &_Presets[1];

        *__presetList = cache_open_list(in_path.c_str());

        if (!*__presetList)
            return false;

        BASS_MIDI_FONTEX * fontex = (*__presetList)->presets;

        for (size_t i = 0, j = (*__presetList)->count; i < j; ++i)
            presetList.push_back(fontex[i]);

        return true;
    }

    return false;
}

void BMPlayer::reset_parameters()
{
    bank_lsb_overridden = false;

    for (size_t i = 0; i < 48; ++i)
    {
        if (bank_lsb_override[i])
            bank_lsb_overridden = true;

        ::BASS_MIDI_StreamEvent(_stream[i / 16], i % 16, MIDI_EVENT_BANK_LSB, bank_lsb_override[i]);
    }
}

bool BMPlayer::getErrorMessage(std::string & errorMessage)
{
    if (_ErrorMessage.length() == 0)
        return false;

    errorMessage = _ErrorMessage;

    return true;
}
#pragma endregion

#pragma endregion

