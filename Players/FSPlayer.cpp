
/** $VER: FSPlayer.cpp (2025.03.01) P. Stuer **/

#include "framework.h"

#include "FSPlayer.h"

#include "Support.h"

FSPlayer::FSPlayer() : player_t()
{
    ::memset(_Synths, 0, sizeof(_Synths));
    ::memset(_Settings, 0, sizeof(_Settings));

    _DoDynamicLoading = true;
    _DoReverbAndChorusProcessing = true;
    _VoiceCount = 256;

    _InterpolationMode = FLUID_INTERP_DEFAULT;
}

FSPlayer::~FSPlayer()
{
    Shutdown();
}

void FSPlayer::Initialize(const WCHAR * basePath)
{
    _FluidSynth.Initialize(basePath);
}

void FSPlayer::SetSoundFonts(const std::vector<soundfont_t> & soundFonts)
{
    if (_SoundFonts == soundFonts)
        return;

    _SoundFonts = soundFonts;

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

    for (auto & Setting : _Settings)
    {
        _FluidSynth.SetIntegerSetting(Setting, "synth.reverb.active", enabled ? 1 : 0);
        _FluidSynth.SetIntegerSetting(Setting, "synth.chorus.active", enabled ? 1 : 0);
    }

    _DoReverbAndChorusProcessing = enabled;
}

void FSPlayer::SetVoiceCount(uint32_t voiceCount)
{
    if (_VoiceCount == voiceCount)
        return;

    for (auto & Setting : _Settings)
        _FluidSynth.SetIntegerSetting(Setting, "synth.polyphony", (int) voiceCount);

    _VoiceCount = voiceCount;
}

void FSPlayer::SetInterpolationMode(uint32_t method)
{
    if (_InterpolationMode == method)
        return;

    for (const auto & Synth : _Synths)
    {
        if (Synth != nullptr)
            _FluidSynth.SetInterpolationMethod(Synth, -1, (int) method);
    }

    _InterpolationMode = method;
}

uint32_t FSPlayer::GetActiveVoiceCount() const noexcept
{
    uint32_t VoiceCount = 0;

    for (const auto & Synth : _Synths)
    {
        if (Synth != nullptr)
            VoiceCount += _FluidSynth.GetActiveVoiceCount(Synth);
    }

    return VoiceCount;
}

struct context_t
{
    FluidSynth * _FluidSynth;
    fluid_settings_t * _Settings;
};

#pragma region player_t

void Callback(void * data, const char * name, int type)
{
    switch (type)
    {
/*
        case FLUID_NUM_TYPE:
        {
            double value;
            fluid_settings_getnum(d->settings, name, &value);
            fluid_ostream_printf(d->out, "%.3f\n", value);
            break;
        }

        case FLUID_INT_TYPE:
        {
            int value, hints;
            fluid_settings_getint(d->settings, name, &value);

            if(fluid_settings_get_hints(d->settings, name, &hints) == FLUID_OK)
            {
                if(!(hints & FLUID_HINT_TOGGLED))
                {
                    fluid_ostream_printf(d->out, "%d\n", value);
                }
                else
                {
                    fluid_ostream_printf(d->out, "%s\n", value ? "True" : "False");
                }
            }

            break;
        }
*/
        case FLUID_STR_TYPE:
        {
            auto Context = (context_t *) data;

            char * Value = nullptr;

            Context->_FluidSynth->GetStringSetting(Context->_Settings, name, &Value);

            console::printf("%s: %s", name, Value);

            Context->_FluidSynth->Free(Value);
            break;
        }
    }
};

bool FSPlayer::Startup()
{
    if (!_FluidSynth.IsInitialized())
        return false;

    if (IsStarted())
        return true;

    // Create the settings.
    size_t i = 0;

    for (auto & Setting : _Settings)
    {
        Setting = _FluidSynth.CreateSettings();

        // https://www.fluidsynth.org/api/fluidsettings.xml#synth.audio-channels
        _FluidSynth.SetNumericSetting(Setting, "synth.sample-rate", (double) _SampleRate);
        _FluidSynth.SetIntegerSetting(Setting, "synth.midi-channels", 16);

        // Gain applied to the final output of the synthesizer.
        _FluidSynth.SetNumericSetting(Setting, "synth.gain", 0.2f);

        // Device identifier used for SYSEX commands, such as MIDI Tuning Standard commands.
        // Fluidsynth will only process those SYSEX commands destined for this ID (except when this setting is set to 127, which causes fluidsynth to process all SYSEX commands, regardless of the device ID).
        _FluidSynth.SetIntegerSetting(Setting, "synth.device-id", (int) (0x10 + i));

        // Load and unload samples from memory whenever presets are being selected or unselected for a MIDI channel.
        _FluidSynth.SetIntegerSetting(Setting, "synth.dynamic-sample-loading", _DoDynamicLoading ? 1 : 0);

        // Reverb effects module
        _FluidSynth.SetIntegerSetting(Setting, "synth.reverb.active", _DoReverbAndChorusProcessing ? 1 : 0);

        // Chorus effects module
        _FluidSynth.SetIntegerSetting(Setting, "synth.chorus.active", _DoReverbAndChorusProcessing ? 1 : 0);
//      _FluidSynth.SetNumericSetting(Setting, "synth.chorus.depth", 8.0f); // 0.0 - 256.0
//      _FluidSynth.SetNumericSetting(Setting, "synth.chorus.level", 2.0f); // 0.0 - 10.0
//      _FluidSynth.SetIntegerSetting(Setting, "synth.chorus.nr", 3); // 0 - 99

        // Defines how many voices can be played in parallel.
        _FluidSynth.SetIntegerSetting(Setting, "synth.polyphony", (int ) _VoiceCount);

//      char * Value = nullptr; _FluidSynth.GetDefaultStringSetting(Setting, "synth.midi-bank-select", &Value);

        /*
         This setting defines how the synthesizer interprets Bank Select messages.

            gs: (default) CC0 becomes the bank number, CC32 is ignored.
            gm: ignores CC0 and CC32 messages.
            mma: bank is calculated as CC0*128+CC32.
            xg: If CC0 is equal to 120, 126, or 127 then channel is set to drum and the bank number is set to 128 (CC32 is ignored). Otherwise the channel is set to melodic and CC32 is the bank number.
        */
        _FluidSynth.SetStringSetting(Setting, "synth.midi-bank-select", "mma");

        ++i;
    }

    // Create the synthesizers.
    i = 0;

    for (const auto & Setting : _Settings)
    {
        fluid_sfloader_t * Loader = GetSoundFontLoader(Setting);

        if (Loader == nullptr)
        {
            _ErrorMessage = "Out of memory";

            return false;
        }

        _Synths[i] = _FluidSynth.CreateSynthesizer(Setting);

        if (_Synths[i] == nullptr)
        {
            _ErrorMessage = "Out of memory";

            return false;
        }

        _FluidSynth.AddSoundFontLoader(_Synths[i], Loader);
        _FluidSynth.SetInterpolationMethod(_Synths[i], -1, (int) _InterpolationMode);

        ++i;
    }

    if (!_SoundFonts.empty())
    {
        for (const auto & sf : _SoundFonts)
        {
            for (const auto & Synth : _Synths)
            {
                int SoundFontId = _FluidSynth.LoadSoundFont(Synth, sf.FilePath().c_str(), TRUE);

                if (SoundFontId == FLUID_FAILED)
                {
                    Shutdown();

                    _ErrorMessage = "Failed to load SoundFont \"" + sf.FilePath() + "\"";

                    return false;
                }

                _FluidSynth.SetSoundFontBankOffset(Synth, SoundFontId, sf.BankOffset());
            }
        }
    }
/*
    if (_Synths[0])
    {
        auto Settings = _FluidSynth.GetSettings(_Synths[0]);

        context_t Context = { &_FluidSynth, Settings };

        _FluidSynth.ForEachSetting(Settings, &Context, Callback);
    }
*/
    _ErrorMessage = "";

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void FSPlayer::Shutdown()
{
    for (auto & Synth : _Synths)
    {
        if (Synth != nullptr)
        {
            _FluidSynth.DeleteSynthesizer(Synth);
            Synth = nullptr;
        }
    }

    for (auto & Setting : _Settings)
    {
        if (Setting)
        {
            _FluidSynth.DeleteSettings(Setting);
            Setting = nullptr;
        }
    }

    _IsInitialized = false;
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

        for (const auto & Synth : _Synths)
        {
            ::memset(Data, 0, sizeof(Data));

            _FluidSynth.WriteFloat(Synth, (int) ToDo, Data, 0, 2, Data, 1, 2);

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

    uint8_t PortNumber = 0;

    for (const auto & Synth : _Synths)
    {
        if (Synth != nullptr)
        {
            _FluidSynth.ResetSynthesizer(Synth);

            SendSysExReset(PortNumber, 0);

            ++ResetCount;
        }

        ++PortNumber;
    }

    return (ResetCount == _countof(_Synths));
}

/// <summary>
/// Sends a message to the library.
/// </summary>
void FSPlayer::SendEvent(uint32_t message)
{
    int PortNumber = (int) ((message >> 24) & 0x7F);
    int Param2     = (int) ((message >> 16) & 0xFF);
    int Param1     = (int) ((message >>  8) & 0xFF);
    int Code       = (int)  (message        & 0xF0);
    int Channel    = (int)  (message        & 0x0F);

    if (PortNumber >= (int) _countof(_Synths))
        PortNumber = 0;

    switch (Code)
    {
        case StatusCodes::NoteOff:
            _FluidSynth.NoteOff(_Synths[PortNumber], Channel, Param1);
            break;

        case StatusCodes::NoteOn:
            _FluidSynth.NoteOn(_Synths[PortNumber], Channel, Param1, Param2);
            break;

        case StatusCodes::KeyPressure:
            break;

        case StatusCodes::ControlChange:
            _FluidSynth.ControlChange(_Synths[PortNumber], Channel, Param1, Param2);
            break;

        case StatusCodes::ProgramChange:
            _FluidSynth.ProgramChange(_Synths[PortNumber], Channel, Param1);
            break;

        case StatusCodes::ChannelPressure:
            _FluidSynth.ChannelPressure(_Synths[PortNumber], Channel, Param1);
            break;

        case StatusCodes::PitchBendChange:
            _FluidSynth.PitchBend(_Synths[PortNumber], Channel, (Param2 << 7) | Param1);
            break;
    }
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void FSPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    if ((data != nullptr) && (size > 2) && (data[0] == StatusCodes::SysEx) && (data[size - 1] == StatusCodes::SysExEnd))
    {
/** Filter GM2 On events
        const uint8_t GM2[] = { 0xF0, 0x7E, 0x7F, 0x09, 0x03, 0xF7 };

        if (::memcmp(data, GM2, _countof(GM2)) == 0)
            return;
**/
        ++data;
        size -= 2;

        if (portNumber >= _countof(_Synths))
            portNumber = 0;

        if (portNumber == 0)
        {
            for (const auto & Synth : _Synths)
                _FluidSynth.SysEx(Synth, (const char *) data, (int) size, NULL, NULL, NULL, 0);
        }
        else
            _FluidSynth.SysEx(_Synths[portNumber], (const char *) data, (int) size, NULL, NULL, NULL, 0);
    }
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

#pragma endregion

static void * HandleOpen(const char * filePath) noexcept
{
    try
    {
        pfc::string FilePath = "";

        if (::strstr(filePath, "://") == 0)
            FilePath = "file://";

        FilePath += filePath;

        file::ptr * File = new file::ptr;

        filesystem::g_open(*File, FilePath, filesystem::open_mode_read, fb2k::noAbort);

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
        file::ptr * File = (file::ptr *) handle;

        (*File)->read_object(data, (t_size) size, fb2k::noAbort);

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
        file::ptr * File = (file::ptr *) handle;

        (*File)->seek_ex(offset, (file::t_seek_mode) origin, fb2k::noAbort);

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
        file::ptr * File = (file::ptr *) handle;

        return (fluid_long_long_t) (*File)->get_position(fb2k::noAbort);
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
