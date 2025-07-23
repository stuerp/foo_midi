
/** $VER: FSPlayer.cpp (2025.07.23) P. Stuer **/

#include "pch.h"

#include "FSPlayer.h"

#include "Encoding.h"
#include "Support.h"
#include "Log.h"

static void LogMessage(int level, const char * message, void * data);

FSPlayer::FSPlayer() noexcept : player_t(), _Settings()
{
    _HasBankSelects = false;
    _DoDynamicLoading = false;
    _DoReverbAndChorusProcessing = true;
    _VoiceCount = 256;

    _InterpolationMethod = FLUID_INTERP_DEFAULT;
}

FSPlayer::~FSPlayer()
{
    Shutdown();
}

/// <summary>
/// Initialize the engine. (FluidSynth specific)
/// </summary>
void FSPlayer::Initialize(const WCHAR * basePath)
{
    _API.Initialize(basePath);
}

/// <summary>
/// Sets the synthesis interpolation mode.
/// </summary>
void FSPlayer::SetInterpolationMode(uint32_t method)
{
    if (_InterpolationMethod == method)
        return;

    _InterpolationMethod = method;
}

/// <summary>
/// Sets the number of voices to use.
/// </summary>
void FSPlayer::SetVoiceCount(uint32_t voiceCount)
{
    if (_VoiceCount == voiceCount)
        return;

    _VoiceCount = voiceCount;
}

/// <summary>
/// Enables or disables reverb and chorus processing.
/// </summary>
void FSPlayer::EnableEffects(bool enabled)
{
    if (_DoReverbAndChorusProcessing == enabled)
        return;

    _DoReverbAndChorusProcessing = enabled;
}

/// <summary>
/// Enables or disables dynamic loading of the sound fonts.
/// </summary>
void FSPlayer::EnableDynamicLoading(bool enabled)
{
    if (_DoDynamicLoading == enabled)
        return;

    _DoDynamicLoading = enabled;

    Shutdown();
}

/// <summary>
/// Sets the sound fonts to use for synthesis.
/// </summary>
void FSPlayer::SetSoundfonts(const std::vector<soundfont_t> & soundFonts)
{
    if (_SoundFonts == soundFonts)
        return;

    _SoundFonts = soundFonts;

    Shutdown();
}

/// <summary>
/// Gets the numbers of voices that are currently active.
/// </summary>
uint32_t FSPlayer::GetActiveVoiceCount() const noexcept
{
    uint32_t VoiceCount = 0;

    for (const auto & Synth : _Synths)
    {
        if (Synth != nullptr)
            VoiceCount += _API.GetActiveVoiceCount(Synth);
    }

    return VoiceCount;
}

#pragma region player_t

struct context_t
{
    FluidSynth::API * _FluidSynth;
    fluid_settings_t * _Settings;
};

bool FSPlayer::Startup()
{
    if (_IsStarted)
        return true;
/*
    if (IsStarted())
        return true;
*/
    // Determine if the MIDI stream contains CC#0 Bank Select messages that any other bank than 0 and 127 (Drums)
    for (const auto & m : _Messages)
    {
        int Status = (int) (m.Data & 0xF0u);
        int Param1 = (int) (m.Data >>  8);
        int Param2 = (int) (m.Data >> 16);

        if ((Status == midi::ControlChange) && (Param1 == midi::BankSelect) && (Param2 != 0) && (Param2 != 127))
        {
            _HasBankSelects = true;
            break;
        }
    }

    if (!_API.IsInitialized())
        return false;

    // We don't need any drivers. foobar2000 is going play our samples.
    {
        const char * Drivers[] = { nullptr };

        int Result = _API.RegisterDriver(Drivers);

        assert(Result == FLUID_OK);
    }

    if (!InitializeSettings())
        return false;

    _Synths.resize(_PortNumbers.size());

    // Create the synthesizers.
    for (auto & Synth : _Synths)
    {
        // Create the synthesizer.
        {
            fluid_sfloader_t * Loader = GetSoundFontLoader(_Settings);

            if (Loader == nullptr)
            {
                _ErrorMessage = "Failed to create SoundFont loader";

                return false;
            }

            Synth = _API.CreateSynthesizer(_Settings);

            if (Synth == nullptr)
            {
                _ErrorMessage = "Failed to create synthesizer";

                return false;
            }

            _API.AddSoundFontLoader    (Synth, Loader);
            _API.SetInterpolationMethod(Synth, -1, (int) _InterpolationMethod);
            _API.SetChorusType         (Synth, fluid_chorus_mod::FLUID_CHORUS_MOD_SINE);

            try
            {
                for (const auto & sf : _SoundFonts)
                    LoadSoundfont(Synth, sf);
            }
            catch (std::exception e)
            {
                _ErrorMessage = e.what();

                Shutdown();

                return false;
            }
        }
    }

    _API.SetLogFunction(FLUID_PANIC, LogMessage, nullptr);
    _API.SetLogFunction(FLUID_ERR,   LogMessage, nullptr);
    _API.SetLogFunction(FLUID_WARN,  LogMessage, nullptr);
    _API.SetLogFunction(FLUID_DBG,   LogMessage, nullptr);

    _ErrorMessage = "";

    _IsStarted = true;

    {
        const DWORD Version = GetVersion();

        Log.AtInfo().Write(STR_COMPONENT_BASENAME " is using FluidSynth %d.%d.%d.", (Version >> 24) & 0xFF, (Version >> 16) & 0xFF, (Version >> 8) & 0xFF);
    }

    Configure(_MIDIFlavor, _FilterEffects);
    Reset();

    return true;
}

void FSPlayer::Shutdown()
{
    for (auto & Synth : _Synths)
    {
        if (Synth != nullptr)
        {
            _API.DeleteSynthesizer(Synth);
            Synth = nullptr;
        }
    }

    _Synths.resize(0);

    if (_Settings != nullptr)
    {
        _API.DeleteSettings(_Settings);
        _Settings = nullptr;
    }

    _IsStarted = false;
}

void FSPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    static const uint32_t MaxFrames = 512;
    static const uint32_t MaxChannels = 2;

    ::memset(dstFrames, 0, ((size_t) dstCount * MaxChannels) * sizeof(audio_sample));

    uint32_t dstDone = 0;

    while (dstDone < dstCount)
    {
        float srcFrames[MaxFrames * MaxChannels];

        uint32_t srcCount = dstCount - dstDone;

        if (srcCount > MaxFrames)
            srcCount = MaxFrames;

        for (const auto & Synth : _Synths)
        {
            ::memset(srcFrames, 0, sizeof(srcFrames));

            _API.WriteFloat(Synth, (int) srcCount, srcFrames, 0, MaxChannels, srcFrames, 1, MaxChannels);

            // Convert the format of the rendered output.
            for (uint32_t i = 0, j = 0; i < srcCount; ++i)
            {
                dstFrames[j] += srcFrames[j];
                ++j;
                dstFrames[j] += srcFrames[j];
                ++j;
            }
        }

        dstFrames += srcCount * MaxChannels;
        dstDone += srcCount;
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
            _API.ResetSynthesizer(Synth);

            ResetPort(PortNumber, 0);

            ++ResetCount;
        }

        ++PortNumber;
    }

    return (ResetCount == _Synths.size());
}

/// <summary>
/// Sends a message to the library.
/// </summary>
void FSPlayer::SendEvent(uint32_t data)
{
    auto PortNumber = (size_t) ((data >> 24) & 0x7F);
    auto Param2     = (int)    ((data >> 16) & 0xFF);
    auto Param1     = (int)    ((data >>  8) & 0xFF);
    auto Code       = (int)     (data        & 0xF0);
    auto Channel    = (int)     (data        & 0x0F);

    if (PortNumber >= _Synths.size())
        PortNumber = 0;

    switch (Code)
    {
        case midi::NoteOff:
            _API.NoteOff(_Synths[PortNumber], Channel, Param1);
            break;

        case midi::NoteOn:
            _API.NoteOn(_Synths[PortNumber], Channel, Param1, Param2);
            break;

        case midi::KeyPressure:
            _API.KeyPressure(_Synths[PortNumber], Channel, Param1, Param2);
            break;

        case midi::ControlChange:
            _API.ControlChange(_Synths[PortNumber], Channel, Param1, Param2);
            break;

        case midi::ProgramChange:
            _API.ProgramChange(_Synths[PortNumber], Channel, Param1);
            break;

        case midi::ChannelPressure:
            _API.ChannelPressure(_Synths[PortNumber], Channel, Param1);
            break;

        case midi::PitchBendChange:
            _API.PitchBend(_Synths[PortNumber], Channel, (Param2 << 7) | Param1);
            break;
    }
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void FSPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber)
{
    if ((data != nullptr) && (size > 2) && (data[0] == midi::SysEx) && (data[size - 1] == midi::SysExEnd))
    {
        ++data;
        size -= 2;

        if (portNumber >= _Synths.size())
            portNumber = 0;

        if (portNumber == 0)
        {
            for (const auto & Synth : _Synths)
                _API.SysEx(Synth, (const char *) data, (int) size, NULL, NULL, NULL, 0);
        }
        else
            _API.SysEx(_Synths[portNumber], (const char *) data, (int) size, NULL, NULL, NULL, 0);
    }
}

#pragma endregion

/// <summary>
/// Initializes the settings.
/// </summary>
bool FSPlayer::InitializeSettings() noexcept
{
    _Settings = _API.CreateSettings();

    if (_Settings == nullptr)
        return false;

    _API.SetIntegerSetting(_Settings, "synth.midi-channels", 32);

    // The sample rate of the audio generated by the synthesizer.
    _API.SetNumericSetting(_Settings, "synth.sample-rate", (double) _SampleRate);

    // Gain applied to the final output of the synthesizer.
    _API.SetNumericSetting(_Settings, "synth.gain", 0.8);

    // Device identifier used for SYSEX commands, such as MIDI Tuning Standard commands.
    // Fluidsynth will only process those SYSEX commands destined for this ID (except when this setting is set to 127, which causes fluidsynth to process all SYSEX commands, regardless of the device ID).
//  _API.SetIntegerSetting(Setting, "synth.device-id", (int) (0x10 + i));

    // Load and unload samples from memory whenever presets are being selected or unselected for a MIDI channel. Supported when using FluidSynth 2.4.4 or later. Earlier versions generate a clicking sound when using incorrectly padded samples. (see https://github.com/FluidSynth/fluidsynth/issues/1484)
    if (GetVersion() >= MakeDWORD(2, 4, 4, 0))
        _API.SetIntegerSetting(_Settings, "synth.dynamic-sample-loading", _DoDynamicLoading ? 1 : 0); // Causes clicking in the cymbals of "ran.rmi".

    // When set to 1 (TRUE) the reverb effects module is activated. Otherwise, no reverb will be added to the output signal. Note that the amount of signal sent to the reverb module depends on the "reverb send" generator defined in the SoundFont.
    _API.SetIntegerSetting(_Settings, "synth.reverb.active", _DoReverbAndChorusProcessing ? 1 : 0);
    // Sets the amount of reverb damping. (0.0 - 1.0, Default: 0.3)
    _API.SetNumericSetting(_Settings, "synth.reverb.damp", 0.3);
    // Sets the reverb output amplitude. (0.0 -   1.0, Default: 0.7)
    _API.SetNumericSetting(_Settings, "synth.reverb.level", 0.7);
    // Sets the room size (i.e. amount of wet) reverb. (0.0 - 1.0, Default: 0.5)
    _API.SetNumericSetting(_Settings, "synth.reverb.room-size", 0.5);
    // Sets the stereo spread of the reverb signal. (0.0 - 100.0, Default: 0.8)
    _API.SetNumericSetting(_Settings, "synth.reverb.width", 0.8);

    // When set to 1 (TRUE) the chorus effects module is activated. Otherwise, no chorus will be added to the output signal. Note that the amount of signal sent to the chorus module depends on the "chorus send" generator defined in the SoundFont.
    _API.SetIntegerSetting(_Settings, "synth.chorus.active", _DoReverbAndChorusProcessing ? 1 : 0);
    // Specifies the modulation depth of the chorus. Max. value depends on synth sample-rate. 0.0 - 21.0 is safe for sample-rate values up to 96KHz.
    _API.SetNumericSetting(_Settings, "synth.chorus.depth", 4.25);
    // Specifies the output amplitude of the chorus signal. (0.0 - 10.0)
    _API.SetNumericSetting(_Settings, "synth.chorus.level", 0.6);
    // Sets the voice count of the chorus. (0 - 99, CPU time consumption proportional to this value.)
    _API.SetIntegerSetting(_Settings, "synth.chorus.nr", 3);
    // Sets the modulation speed in Hz. (0.1 - 5.0 Hz)
    _API.SetNumericSetting(_Settings, "synth.chorus.speed", 0.2);

    // Defines how many voices can be played in parallel.
    _API.SetIntegerSetting(_Settings, "synth.polyphony", (int) _VoiceCount);

    // Defines how the synthesizer interprets Bank Select messages.
    _API.SetStringSetting(_Settings, "synth.midi-bank-select", "mma");

    // Read the settings from an external text file, if present.
    FluidSynth::Host Host;

    Host.LoadConfig(_API, pfc::string(CfgFluidSynthConfigFilePath).c_str(), _Settings);

    // The external config may have overriden the requested sample rate.
    double ActualSampleRate = 0.;

    if (_API.GetNumericSetting(_Settings, "synth.sample-rate", &ActualSampleRate) == FLUID_OK)
        SetSampleRate((uint32_t) ActualSampleRate);

    return true;
}

#pragma region Soundfont loader

static void * HandleOpen(const char * filePath) noexcept
{
    try
    {
        std::string URI = "file://";

        URI += filePath;

        file::ptr * File = new file::ptr;

        filesystem::g_open(*File, URI.c_str(), filesystem::open_mode_read, fb2k::noAbort);

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

fluid_sfloader_t * FSPlayer::GetSoundFontLoader(fluid_settings_t * settings) const noexcept
{
    fluid_sfloader_t * Loader = _API.CreateSoundFontLoader(settings);

    if (Loader == nullptr)
        return nullptr;

    if (_API.SetSoundFontLoaderCallbacks(Loader, HandleOpen, HandleRead, HandleSeek, HandleTell, HandleClose) != FLUID_OK)
        return nullptr;

    return Loader;
}

#pragma endregion

/// <summary>
/// Loads a soundfont into the specified synthesizer.
/// </summary>
void FSPlayer::LoadSoundfont(fluid_synth_t * synth, const soundfont_t & sf)
{
    const std::string FilePath = (const char *) sf.FilePath.string().c_str();

    if (!std::filesystem::exists(FilePath))
        throw std::exception(::FormatText("Soundfont \"%s\" not found.", FilePath.c_str()).c_str());

    if (!_API.IsSoundFont(FilePath.c_str()))
        throw std::exception(::FormatText("Soundfont \"%s\" has unknown soundfont format.", FilePath.c_str()).c_str());

    const int SoundFontId = _API.LoadSoundFont(synth, FilePath.c_str(), TRUE);

    if (SoundFontId == FLUID_FAILED)
        throw std::exception(::FormatText("Failed to load soundfont \"%s\"", FilePath.c_str()).c_str());

    int BankOffset = sf.BankOffset;

    if (sf.IsEmbedded)
    {
        if (sf.IsDLS)
        {
            if (_FileFormat == midi::XMF)
                BankOffset = -1;
        }
    }

    // Offsets the bank numbers of a loaded SoundFont by subtracting the offset from any bank number when assigning instruments.
    if (BankOffset != 0)
    {
        int Result = _API.SetSoundFontBankOffset(synth, SoundFontId, BankOffset);

        assert(Result == FLUID_OK);
    }
}

/// <summary>
/// Called by FluidSynth to log a message.
/// </summary>
static void LogMessage(int level, const char * message, void * data)
{
    switch (level)
    {
        case FLUID_PANIC: Log.AtFatal().Write(STR_COMPONENT_BASENAME " FluidSynth: FATAL %s", message); break;
        case FLUID_INFO:  Log.AtInfo() .Write(STR_COMPONENT_BASENAME " FluidSynth: %s", message); break;

#ifdef _DEBUG
        // Fluidsynth 2.4.7 issues errors while loading soundfonts but continues anyway. Disable these in release builds.
        case FLUID_ERR:   Log.AtError().Write(STR_COMPONENT_BASENAME " FluidSynth: ERROR %s", message); break;
        case FLUID_WARN:  Log.AtWarn() .Write(STR_COMPONENT_BASENAME " FluidSynth: WARNING %s", message); break;
        case FLUID_DBG:   Log.AtTrace().Write(STR_COMPONENT_BASENAME " FluidSynth: TRACE %s", message); break;
#endif
    }
}

#ifdef LATER
static void Callback(void * data, const char * name, int type)
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
#endif
