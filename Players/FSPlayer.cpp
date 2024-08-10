
/** $VER: FSPlayer.cpp (2023.08.20) P. Stuer **/

#include "framework.h"

#include "FSPlayer.h"

FSPlayer::FSPlayer() : player_t()
{
    ::memset(_Settings, 0, sizeof(_Settings));
    ::memset(_Synth, 0, sizeof(_Synth));

    _DoDynamicLoading = true;
    _DoReverbAndChorusProcessing = true;
    _VoiceCount = 256;

    _InterpolationMode = FLUID_INTERP_DEFAULT;
}

FSPlayer::~FSPlayer()
{
    Shutdown();
}

bool FSPlayer::Initialize(const WCHAR * basePath)
{
    return _FluidSynth.Initialize(basePath);
}

void FSPlayer::SetSoundFontDirectory(const char * directoryPath)
{
    if (_SoundFontDirectoryPath == directoryPath)
        return;

    _SoundFontDirectoryPath = directoryPath;

    Shutdown();
}

void FSPlayer::SetSoundFontFile(const char * filePath)
{
    if (_SoundFontFilePath == filePath)
        return;

    _SoundFontFilePath = filePath;

    Shutdown();
}

void FSPlayer::EnableDynamicLoading(bool enabled)
{
    if (_DoDynamicLoading == enabled)
        return;

    _DoDynamicLoading = enabled;

    Shutdown();
}

void FSPlayer::EnableEffects(bool enabled)
{
    if (_DoReverbAndChorusProcessing == enabled)
        return;

    for (size_t i = 0; i < _countof(_Settings); ++i)
    {
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.reverb.active", enabled ? 1 : 0);
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.chorus.active", enabled ? 1 : 0);
    }

    _DoReverbAndChorusProcessing = enabled;
}

void FSPlayer::SetVoiceCount(uint32_t voiceCount)
{
    if (_VoiceCount == voiceCount)
        return;

    for (size_t i = 0; i < _countof(_Settings); ++i)
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.polyphony", (int) voiceCount);

    _VoiceCount = voiceCount;
}

void FSPlayer::SetInterpolationMode(uint32_t method)
{
    if (_InterpolationMode == method)
        return;

    for (size_t i = 0; i < _countof(_Synth); ++i)
        if (_Synth[i])
            _FluidSynth.SetInterpolationMethod(_Synth[i], -1, (int) method);

    _InterpolationMode = method;
}

uint32_t FSPlayer::GetActiveVoiceCount() const noexcept
{
    uint32_t VoiceCount = 0;

    for (size_t i = 0; i < _countof(_Synth); ++i)
        if (_Synth[i])
            VoiceCount += _FluidSynth.GetActiveVoiceCount(_Synth[i]);

    return VoiceCount;
}

bool FSPlayer::Startup()
{
    if (!_FluidSynth.IsInitialized())
        return false;

    if (_Synth[0] && _Synth[1] && _Synth[2])
        return true;

    // Create the settings.
    for (size_t i = 0; i < _countof(_Settings); ++i)
    {
        _Settings[i] = _FluidSynth.CreateSettings();

        // https://www.fluidsynth.org/api/fluidsettings.xml#synth.audio-channels
        _FluidSynth.SetNumericSetting(_Settings[i], "synth.sample-rate", (double) _SampleRate);
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.midi-channels", 16);

        // Gain applied to the final output of the synthesizer.
        _FluidSynth.SetNumericSetting(_Settings[i], "synth.gain", 0.2f);

        // Device identifier used for SYSEX commands, such as MIDI Tuning Standard commands.
        // Fluidsynth will only process those SYSEX commands destined for this ID (except when this setting is set to 127, which causes fluidsynth to process all SYSEX commands, regardless of the device ID).
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.device-id", (int) (0x10 + i));

        // Load and unload samples from memory whenever presets are being selected or unselected for a MIDI channel.
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.dynamic-sample-loading", _DoDynamicLoading ? 1 : 0);

        // Reverb effects module
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.reverb.active", _DoReverbAndChorusProcessing ? 1 : 0);

        // Chorus effects module
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.chorus.active", _DoReverbAndChorusProcessing ? 1 : 0);
//      _FluidSynth.SetNumericSetting(_Settings[i], "synth.chorus.depth", 8.0f); // 0.0 - 256.0
//      _FluidSynth.SetNumericSetting(_Settings[i], "synth.chorus.level", 2.0f); // 0.0 - 10.0
//      _FluidSynth.SetIntegerSetting(_Settings[i], "synth.chorus.nr", 3); // 0 - 99

        // Defines how many voices can be played in parallel.
        _FluidSynth.SetIntegerSetting(_Settings[i], "synth.polyphony", (int ) _VoiceCount);
    }

    // Create the synthesizers.
    for (size_t i = 0; i < _countof(_Synth); ++i)
    {
        fluid_sfloader_t * Loader = GetSoundFontLoader(_Settings[i]);

        if (Loader == nullptr)
        {
            _ErrorMessage = "Out of memory";

            return false;
        }

        _Synth[i] = _FluidSynth.CreateSynthesizer(_Settings[i]);

        if (_Synth[i] == nullptr)
        {
            _ErrorMessage = "Out of memory";

            return false;
        }

        _FluidSynth.AddSoundFontLoader(_Synth[i], Loader);
        _FluidSynth.SetInterpolationMethod(_Synth[i], -1, (int) _InterpolationMode);
    }

    if (_SoundFontFilePath.length() != 0)
    {
        char FilePath[MAX_PATH];

        pfc::stringcvt::convert_utf8_to_ascii(FilePath, _countof(FilePath), _SoundFontFilePath.c_str(), _SoundFontFilePath.size());

        for (size_t i = 0; i < _countof(_Synth); ++i)
        {
            if (_FluidSynth.LoadSoundFont(_Synth[i], FilePath, 1) == FLUID_FAILED)
            {
                Shutdown();

                _ErrorMessage = "Failed to load SoundFont bank: ";
                _ErrorMessage += _SoundFontFilePath;

                return false;
            }
        }
    }

    _ErrorMessage = "";

    _IsInitialized = true;

    return true;
}

void FSPlayer::Shutdown()
{
    for (size_t i = 0; i < _countof(_Synth); ++i)
    {
        if (_Synth[i])
        {
            _FluidSynth.DeleteSynthesizer(_Synth[i]);
            _Synth[i] = nullptr;
        }

        if (_Settings[i])
        {
            _FluidSynth.DeleteSettings(_Settings[i]);
            _Settings[i] = nullptr;
        }
    }

    _IsInitialized = false;
}

void FSPlayer::SendEvent(uint32_t message)
{
    int Port    = (int) ((message >> 24) & 0x7F);
    int Param2  = (int) ((message >> 16) & 0xFF);
    int Param1  = (int) ((message >>  8) & 0xFF);
    int Code    = (int)  (message        & 0xF0);
    int Channel = (int)  (message        & 0x0F);

    fluid_synth_t * Synth = (Port && Port < _countof(_Synth)) ? _Synth[Port] : _Synth[0];

    switch (Code)
    {
        case 0x80:
            _FluidSynth.NoteOff(Synth, Channel, Param1);
            break;
        case 0x90:
            _FluidSynth.NoteOn(Synth, Channel, Param1, Param2);
            break;
        case 0xA0:
            break;
        case 0xB0:
            _FluidSynth.ControlChange(Synth, Channel, Param1, Param2);
            break;
        case 0xC0:
            _FluidSynth.ProgramChange(Synth, Channel, Param1);
            break;
        case 0xD0:
            _FluidSynth.ChannelPressure(Synth, Channel, Param1);
            break;
        case 0xE0:
            _FluidSynth.PitchBend(Synth, Channel, (Param2 << 7) | Param1);
            break;
    }
}

void FSPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    if (data && (size > 2) && (data[0] == StatusCodes::SysEx) && (data[size - 1] == StatusCodes::SysExEnd))
    {
        ++data;
        size -= 2;

        if (portNumber >= _countof(_Synth))
            portNumber = 0;

        _FluidSynth.SysEx(_Synth[portNumber], (const char *) data, (int) size, NULL, NULL, NULL, 0);

        if (portNumber == 0)
        {
            _FluidSynth.SysEx(_Synth[1], (const char *) data, (int) size, NULL, NULL, NULL, 0);
            _FluidSynth.SysEx(_Synth[2], (const char *) data, (int) size, NULL, NULL, NULL, 0);
        }
    }
}

void FSPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    ::memset(sampleData, 0, ((size_t) sampleCount * 2) * sizeof(audio_sample));

    uint32_t Done = 0;

    while (Done < sampleCount)
    {
        float Data[512 * 2];

        uint32_t ToDo = sampleCount - Done;

        if (ToDo > 512)
            ToDo = 512;

        for (size_t i = 0; i < _countof(_Synth); ++i)
        {
            ::memset(Data, 0, sizeof(Data));

            _FluidSynth.WriteFloat(_Synth[i], (int) ToDo, Data, 0, 2, Data, 1, 2);

            // Convert the format of the rendered output.
            for (uint32_t j = 0; j < ToDo; ++j)
            {
                sampleData[j * 2 + 0] += Data[j * 2 + 0];
                sampleData[j * 2 + 1] += Data[j * 2 + 1];
            }
        }

        sampleData += ToDo * 2;
        Done += ToDo;
    }
}

bool FSPlayer::Reset()
{
    size_t ResetCount = 0;

    for (size_t i = 0; i < _countof(_Synth); ++i)
    {
        if (_Synth[i])
        {
            _FluidSynth.ResetSynthesizer(_Synth[i]);

            SendSysExReset((uint8_t) i, 0);

            ++ResetCount;
        }
    }

    return (ResetCount == _countof(_Synth));
}

bool FSPlayer::GetErrorMessage(std::string & errorMessage)
{
    if (_ErrorMessage.length())
    {
        errorMessage = _ErrorMessage;

        return true;
    }

    return false;
}

static void * HandleOpen(const char * filePath) noexcept
{
    try
    {
        abort_callback_dummy AbortHandler;

        pfc::string8 FilePath = "";

        if (::strstr(filePath, "://") == 0)
            FilePath = "file://";

        FilePath += filePath;

        file::ptr * File = new file::ptr;

        filesystem::g_open(*File, FilePath, filesystem::open_mode_read, AbortHandler);

        return (void *) File;
    }
    catch (...)
    {
        return nullptr;
    }
}

static int HandleRead(void * data, fluid_long_long_t size, void * handle) noexcept
{
    try
    {
        abort_callback_dummy AbortHandler;

        file::ptr * File = (file::ptr *) handle;

        (*File)->read_object(data, (t_size) size, AbortHandler);

        return FLUID_OK;
    }
    catch (...)
    {
        return FLUID_FAILED;
    }
}

static int HandleSeek(void * handle, fluid_long_long_t offset, int origin) noexcept
{
    try
    {
        abort_callback_dummy AbortHandler;

        file::ptr * File = (file::ptr *) handle;

        (*File)->seek_ex(offset, (file::t_seek_mode) origin, AbortHandler);

        return FLUID_OK;
    }
    catch (...)
    {
        return FLUID_FAILED;
    }
}

static fluid_long_long_t HandleTell(void * handle) noexcept
{
    try
    {
        abort_callback_dummy AbortHandler;

        file::ptr * File = (file::ptr *) handle;

        return (fluid_long_long_t) (*File)->get_position(AbortHandler);
    }
    catch (...)
    {
        return FLUID_FAILED;
    }
}

static int HandleClose(void * handle) noexcept
{
    try
    {
        file::ptr * File = (file::ptr *) handle;

        delete File;

        return FLUID_OK;
    }
    catch (...)
    {
        return FLUID_FAILED;
    }
}

fluid_sfloader_t * FSPlayer::GetSoundFontLoader(fluid_settings_t * settings) const
{
    fluid_sfloader_t * Loader = _FluidSynth.CreateSoundFontLoader(settings);

    if (Loader == nullptr)
        return nullptr;

    if (_FluidSynth.SetSoundFontLoaderCallbacks(Loader, HandleOpen, HandleRead, HandleSeek, HandleTell, HandleClose) != FLUID_OK)
        return nullptr;

    return Loader;
}
