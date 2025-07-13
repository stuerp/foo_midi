
/** $VER: BMPlayer.cpp (2025.07.12) **/

#include "pch.h"

#include "BMPlayer.h"

#include "BASSInitializer.h"
#include "SoundFontCache.h"
#include "Exception.h"
#include "Log.h"

#include <sflist.h>

#include <map>

#include <time.h>

static BASSInitializer _BASSInitializer;

BMPlayer::BMPlayer() : player_t()
{
    ::memset(_Streams, 0, sizeof(_Streams));

    _InterpolationMode = 0;
    _DoReverbAndChorusProcessing = true;
    _IgnoreCC32 = false;
    _VoiceCount = 256;
    _SFList[0] = nullptr;
    _SFList[1] = nullptr;

    ::memset(_NRPNLSB, 0xFF, sizeof(_NRPNLSB));
    ::memset(_NRPNMSB, 0xFF, sizeof(_NRPNMSB));

    _SrcFrames = nullptr;

    if (!_BASSInitializer.Initialize())
        throw component::runtime_error("Unable to initialize BASSMIDI");
}

BMPlayer::~BMPlayer()
{
    Shutdown();
}

/// <summary>
/// Sets the synthesis interpolation mode.
/// </summary>
void BMPlayer::SetInterpolationMode(uint32_t interpolationMode)
{
    if (_InterpolationMode == interpolationMode)
        return;

    _InterpolationMode = interpolationMode;

    Shutdown();
}

/// <summary>
/// Sets the number of voices to use.
/// </summary>
void BMPlayer::SetVoiceCount(uint32_t voiceCount)
{
    if (_VoiceCount == voiceCount)
        return;

    _VoiceCount = std::clamp(voiceCount, 1U, 100000U);

    Shutdown();
}

/// <summary>
/// Enables or disables reverb and chorus processing.
/// </summary>
void BMPlayer::EnableEffects(bool enabled)
{
    if (_DoReverbAndChorusProcessing == enabled)
        return;

    _DoReverbAndChorusProcessing = enabled;

    Shutdown();
}

/// <summary>
/// Sets the sound fonts to use for synthesis.
/// </summary>
void BMPlayer::SetSoundfonts(const std::vector<soundfont_t> & soundFonts)
{
    if (_SoundFonts == soundFonts)
        return;

    _SoundFonts = soundFonts;

    Shutdown();
}

/// <summary>
/// Gets the numbers of voices that are currently active.
/// </summary>
uint32_t BMPlayer::GetActiveVoiceCount() const noexcept
{
    uint32_t VoiceCount = 0;

    float Voices;

    for (const auto & Stream : _Streams)
    {
        if (Stream)
        {
            Voices = 0.f;

            if (::BASS_ChannelGetAttribute(Stream, BASS_ATTRIB_MIDI_VOICES_ACTIVE, &Voices))
                VoiceCount += (size_t) (int) (Voices);
        }
    }

    return VoiceCount;
}

/// <summary>
/// Gets statistics about the use of the soundfonts. BASS MIDI specific.
/// </summary>
bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded)
{
    if (!_BASSInitializer.IsInitialized())
        return false;

    ::CacheGetStatistics(sampleDataSize, sampleDataLoaded);

    return true;
}

#pragma region Private

#pragma region player_t

bool BMPlayer::Startup()
{
    if (_IsStarted)
        return true;

    _SrcFrames = new float[MaxFrames * MaxChannels];

    if (_SrcFrames == nullptr)
        return false;

    std::vector<BASS_MIDI_FONTEX> SoundFontConfigurations;

    for (const auto & sf : _SoundFonts)
    {
        if (!LoadSoundFontConfiguration(sf, SoundFontConfigurations))
        {
            _ErrorMessage = "Unable to load configuration for soundfont \"" + sf.FilePath() + "\"";

            return false;
        }
    }

    const DWORD ChannelsPerStream = 32;
    const DWORD Flags = (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_DoReverbAndChorusProcessing ? 0 : BASS_MIDI_NOFX));

    for (auto & Stream : _Streams)
    {
        Stream = ::BASS_MIDI_StreamCreate(ChannelsPerStream, Flags, (DWORD) _SampleRate);

        if (Stream == 0)
        {
            _ErrorMessage = "Unable to create BASS MIDI streams";

            return false;
        }

        ::BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_MIDI_SRC, (float) _InterpolationMode);
        ::BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_MIDI_VOICES, (float) _VoiceCount);

        ::BASS_MIDI_StreamSetFonts(Stream, SoundFontConfigurations.data(), (DWORD) SoundFontConfigurations.size() | BASS_MIDI_FONT_EX);
    }

    _ErrorMessage = "";

    _IsStarted = true;

    {
        const DWORD BASSVersion = GetVersion();

        Log.AtInfo().Format(STR_COMPONENT_BASENAME " is using BASS %d.%d.%d.%d.", (BASSVersion >> 24) & 0xFF, (BASSVersion >> 16) & 0xFF, (BASSVersion >> 8) & 0xFF, BASSVersion & 0xFF);

        const DWORD BASSMIDIVersion = GetMIDIVersion();

        Log.AtInfo().Format(STR_COMPONENT_BASENAME " is using BASS MIDI %d.%d.%d.%d.", (BASSMIDIVersion >> 24) & 0xFF, (BASSMIDIVersion >> 16) & 0xFF, (BASSMIDIVersion >> 8) & 0xFF, BASSMIDIVersion & 0xFF);
    }

    Configure(_MIDIFlavor, _FilterEffects);
    Reset();

    return true;
}

void BMPlayer::Shutdown()
{
    for (auto & Stream : _Streams)
    {
        if (Stream)
        {
            ::BASS_StreamFree(Stream);
            Stream = 0;
        }
    }

    for (auto SoundFont : _SoundFontHandles)
        ::CacheRemoveSoundFont(SoundFont);

    _SoundFontHandles.resize(0);

    if (_SFList[0])
    {
        ::CacheRemoveSoundFontList(_SFList[0]);
        _SFList[0] = nullptr;
    }

    if (_SFList[1])
    {
        ::CacheRemoveSoundFontList(_SFList[1]);
        _SFList[1] = nullptr;
    }

    if (_SrcFrames != nullptr)
    {
        delete[] _SrcFrames;
        _SrcFrames = nullptr;
    }

    _IsStarted = false;
}

void BMPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    while (dstCount != 0)
    {
        const uint32_t srcCount = std::min(dstCount, MaxFrames);
        const uint32_t NumSamples = srcCount * MaxChannels;

        ::memset(dstFrames, 0, NumSamples * sizeof(dstFrames[0]));

        for (auto & Stream : _Streams)
        {
            ::BASS_ChannelGetData(Stream, _SrcFrames, BASS_DATA_FLOAT | (DWORD) (NumSamples * sizeof(_SrcFrames[0])));

            // Convert the format of the rendered output.
            for (size_t j = 0; j < NumSamples; ++j)
                dstFrames[j] += _SrcFrames[j];
        }

        dstFrames += NumSamples;
        dstCount -= srcCount;
    }
}

/// <summary>
/// Resets the player.
/// </summary>
bool BMPlayer::Reset()
{
    for (uint8_t PortNumber = 0; PortNumber < MaxPorts; ++PortNumber)
        ResetPort(PortNumber, 0);

    ::memset(_NRPNLSB, 0xFF, sizeof(_NRPNLSB));
    ::memset(_NRPNMSB, 0xFF, sizeof(_NRPNMSB));

    return true;
}

/// <summary>
/// Sends a message to the library.
/// </summary>
void BMPlayer::SendEvent(uint32_t data)
{
    const uint8_t Event[3]
    {
        (uint8_t) (data),        // Status
        (uint8_t) (data >>  8),  // Param 1
        (uint8_t) (data >> 16)   // Param 2
    };

    const uint8_t Status = Event[0] & 0xF0u;

    if (_IgnoreCC32 && (Status == midi::ControlChange) && (Event[1] == midi::BankSelectLSB))
        return;

    // Ignore the Data Entry message for a NRPN Vibrato Depth. BASSMIDI overreacts to this SC-88Pro specific parameter.
    if ((_MIDIFlavor == MIDIFlavors::SC88Pro) && (Status == midi::ControlChange))
    {
        size_t Channel = Event[0] & 0x0Fu;

        if ((Event[1] == midi::ControlChangeNumbers::NRPNMSB) && (Event[2] == 0x01)) // Set NRPN MSB Vibrato Depth
        {
            _NRPNMSB[Channel] = Event[2];
        }
        else
        if ((Event[1] == midi::ControlChangeNumbers::NRPNLSB) && (Event[2] == 0x09)) // Set NRPN LSB Vibrato Depth
        {
            _NRPNLSB[Channel] = Event[2];
        }
        else
        if ((Event[1] == midi::ControlChangeNumbers::DataEntry) && (_NRPNMSB[Channel] == 0x01) && (_NRPNLSB[Channel] == 0x09))
        {
            _NRPNMSB[Channel] = _NRPNLSB[Channel] = 0xFF;

            return; // Ignore the Data Entry message.
        }
    }

    uint8_t PortNumber = (data >> 24) & 0x7Fu;

    if (PortNumber > (_countof(_Streams) - 1))
        PortNumber = 0;

    const DWORD EventSize = (DWORD)((Status >= midi::TimingClock && Status <= midi::MetaData) ? 1 : ((Status == midi::ProgramChange || Status == midi::ChannelPressure) ? 2 : 3));

    ::BASS_MIDI_StreamEvents(_Streams[PortNumber], BASS_MIDI_EVENTS_RAW, Event, EventSize);
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void BMPlayer::SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber)
{
    if (portNumber > (_countof(_Streams) - 1))
        portNumber = 0;

    if (portNumber == 0)
    {
        for (auto & Stream : _Streams)
            ::BASS_MIDI_StreamEvents(Stream, BASS_MIDI_EVENTS_RAW, event, (DWORD) size);
    }
    else
        ::BASS_MIDI_StreamEvents(_Streams[portNumber], BASS_MIDI_EVENTS_RAW, event, (DWORD) size);
}

#pragma endregion

/// <summary>
/// Loads the configuration from the specified soundfont.
/// </summary>
bool BMPlayer::LoadSoundFontConfiguration(const soundfont_t & soundFont, std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations) noexcept
{
    std::filesystem::path FilePath(soundFont.FilePath());

    if (IsOneOf(FilePath.extension(), { L".sf2", L".sf3", L".sf2pack", L".sfogg" }))
    {
        HSOUNDFONT hSoundFont = ::CacheAddSoundFont(soundFont.FilePath());

        if (hSoundFont == 0)
        {
            Shutdown();

            _ErrorMessage = "Unable to load sound font \"" + soundFont.FilePath() + "\"";

            return false;
        }

        _SoundFontHandles.push_back(hSoundFont);

        ::BASS_MIDI_FontSetVolume(hSoundFont, soundFont.Volume());

        BASS_MIDI_FONTEX fex = { hSoundFont, -1, -1, -1, soundFont.BankOffset(), 0 }; // Load the whole sound font.

        soundFontConfigurations.push_back(fex);

        return true;
    }

    if (IsOneOf(FilePath.extension(), { L".sflist", L".json" }))
    {
        sflist_t ** SFList = &_SFList[0];

        if (*SFList)
            SFList = &_SFList[1];

        *SFList = ::CacheAddSoundFontList(soundFont.FilePath());

        if (!*SFList)
            return false;

        BASS_MIDI_FONTEX * fex = (*SFList)->FontEx;

        for (size_t i = 0, j = (*SFList)->Count; i < j; ++i)
            soundFontConfigurations.push_back(fex[i]);

        return true;
    }

    return false;
}

#ifdef Old
/// <summary>
/// Loads the configuration from the specified soundfont.
/// </summary>
bool BMPlayer::LoadSoundFontConfiguration(const soundfont_t & soundFont, std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations) noexcept
{
    std::string FileExtension;

    size_t dot = soundFont.FilePath().find_last_of('.');

    if (dot != std::string::npos)
        FileExtension.assign(soundFont.FilePath().begin() + (const __int64)(dot + 1), soundFont.FilePath().end());

    if ((::stricmp_utf8(FileExtension.c_str(), "sf2") == 0) || (::stricmp_utf8(FileExtension.c_str(), "sf3") == 0) || (::stricmp_utf8(FileExtension.c_str(), "sf2pack") == 0) || (::stricmp_utf8(FileExtension.c_str(), "sfogg") == 0))
    {
        HSOUNDFONT hSoundFont = ::CacheAddSoundFont(soundFont.FilePath());

        if (hSoundFont == 0)
        {
            Shutdown();

            _ErrorMessage = "Unable to load SoundFont \"" + soundFont.FilePath() + "\"";

            return false;
        }

        _SoundFontHandles.push_back(hSoundFont);

        ::BASS_MIDI_FontSetVolume(hSoundFont, soundFont.Volume());

        if (soundFont.IsEmbedded())
        {
            BASS_MIDI_FONTINFO SoundFontInfo;

            if (::BASS_MIDI_FontGetInfo(hSoundFont, &SoundFontInfo))
            {
                DWORD * Presets = (DWORD *) ::malloc(SoundFontInfo.presets * sizeof(DWORD));

                if (Presets != nullptr)
                {
                    if (::BASS_MIDI_FontGetPresets(hSoundFont, Presets))
                    {
                        console::print("Mapping ", SoundFontInfo.presets, " presets from embedded sound font \"", SoundFontInfo.name, "\":");

                        for (DWORD i = 0; i < SoundFontInfo.presets; ++i)
                        {
                            const int    PresetNumber = LOWORD(Presets[i]);
                            const int    SrcBank      = HIWORD(Presets[i]);
                            const char * PresetName   = ::BASS_MIDI_FontGetPreset(hSoundFont, PresetNumber, SrcBank);

                            int DstBank = SrcBank;

/* FIXME: This is the only way to get Rock_test.rmi working.
                            if (SrcBank != 128)
                            {
                                DstBank += soundFont.BankOffset();
                            }
*/
                            BASS_MIDI_FONTEX fex =
                            {
                                hSoundFont, PresetNumber, SrcBank, PresetNumber, (DstBank >> 7) & 0x7F, DstBank & 0x7F
                            };

                            soundFontConfigurations.push_back(fex);

                            console::printf("SrcBank %3d DstBank %3d Preset %3d: \"%s\"", SrcBank, DstBank, PresetNumber, PresetName);
                        }
                    }

                    ::free(Presets);
                }
            }
        }
        else
        {
            BASS_MIDI_FONTEX fex = { hSoundFont, -1, -1, -1, 0, 0 }; // Load the whole sound font.

            soundFontConfigurations.push_back(fex);
        }

        return true;
    }

    if ((::stricmp_utf8(FileExtension.c_str(), "sflist") == 0) || (::stricmp_utf8(FileExtension.c_str(), "json") == 0))
    {
        sflist_t ** SFList = &_SFList[0];

        if (*SFList)
            SFList = &_SFList[1];

        *SFList = ::CacheAddSoundFontList(soundFont.FilePath());

        if (!*SFList)
            return false;

        BASS_MIDI_FONTEX * fex = (*SFList)->FontEx;

        for (size_t i = 0, j = (*SFList)->Count; i < j; ++i)
            soundFontConfigurations.push_back(fex[i]);

        return true;
    }

    return false;
}
#endif

/// <summary>
/// Returns true if the string matches on of the list.
/// </summary>
bool BMPlayer::IsOneOf(const std::wstring & ext, const std::vector<std::wstring> & extensions)
{
    for (const auto & Extension : extensions)
    {
        if (::_wcsicmp(ext.c_str(), Extension.c_str()) == 0)
            return true;
    }

    return false;
}

#pragma endregion
