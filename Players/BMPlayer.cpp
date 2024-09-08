
/** $VER: BMPlayer.cpp (2024.09.08) **/

#include "framework.h"

#include "BMPlayer.h"

#include "BASSInitializer.h"
#include "SoundFontCache.h"

#include <sflist.h>

#include <string>

#include <map>
#include <time.h>

static BASSInitializer _BASSInitializer;

BMPlayer::BMPlayer() : player_t()
{
    ::memset(_Stream, 0, sizeof(_Stream));

    _InterpolationMode = 0;
    _DoReverbAndChorusProcessing = true;
    _IgnoreCC32 = false;
    _VoiceCount = 256;
    _SFList[0] = nullptr;
    _SFList[1] = nullptr;

    if (!_BASSInitializer.Initialize())
        throw std::runtime_error("Unable to initialize BASS MIDI");
}

BMPlayer::~BMPlayer()
{
    Shutdown();
}

void BMPlayer::SetInterpolationMode(uint32_t interpolationMode)
{
    if (_InterpolationMode == interpolationMode)
        return;

    _InterpolationMode = interpolationMode;

    Shutdown();
}

void BMPlayer::SetVoiceCount(uint32_t voiceCount)
{
    if (_VoiceCount == voiceCount)
        return;

    _VoiceCount = std::clamp(voiceCount, 1U, 100000U);

    Shutdown();
}

void BMPlayer::EnableEffects(bool enabled)
{
    if (_DoReverbAndChorusProcessing == enabled)
        return;

    _DoReverbAndChorusProcessing = enabled;

    Shutdown();
}

/// <summary>
/// Sets the list of soundfonts that will be used.
/// </summary>
void BMPlayer::SetSoundFonts(const std::vector<soundfont_t> & soundFonts)
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

    for (const auto & Stream : _Stream)
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
/// Gets statistics about the use of the soundfonts.
/// </summary>
bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded)
{
    if (!_BASSInitializer.IsInitialized())
        return false;

    ::CacheGetStatistics(sampleDataSize, sampleDataLoaded);

    return true;
}

#pragma region Private

bool BMPlayer::Startup()
{
    if (IsStarted())
        return true;

    ::memset(_BankLSBOverride, 0, sizeof(_BankLSBOverride));

    std::vector<BASS_MIDI_FONTEX> SoundFontConfigurations;

    if (!_SoundFonts.empty())
    {
        for (const auto & sf : _SoundFonts)
        {
            if (!LoadSoundFontConfiguration(sf, SoundFontConfigurations))
            {
                _ErrorMessage = "Unable to load configuration for soundfont \"" + sf.FilePath() + "\"";

                return false;
            }
        }
    }

    const DWORD MIDIChannelCount = 16;
    const DWORD Flags = (DWORD)(BASS_SAMPLE_FLOAT | BASS_STREAM_DECODE | (_DoReverbAndChorusProcessing ? 0 : BASS_MIDI_NOFX));

    for (auto & Stream : _Stream)
    {
        Stream = ::BASS_MIDI_StreamCreate(MIDIChannelCount, Flags, (DWORD) _SampleRate);

        if (Stream == 0)
        {
            _ErrorMessage = "Unable to create BASS MIDI streams";

            return false;
        }

        ::BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_MIDI_SRC, (float) _InterpolationMode);
        ::BASS_ChannelSetAttribute(Stream, BASS_ATTRIB_MIDI_VOICES, (float) _VoiceCount);

        ::BASS_MIDI_StreamSetFonts(Stream, SoundFontConfigurations.data(), (DWORD) SoundFontConfigurations.size() | BASS_MIDI_FONT_EX);
    }

//  SetBankOverride();

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);

    return true;
}

void BMPlayer::Shutdown()
{
    for (auto & Stream : _Stream)
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

    _IsInitialized = false;
}

/// <summary>
/// Sends a message to the library.
/// </summary>
void BMPlayer::SendEvent(uint32_t message)
{
    const uint8_t Event[3]
    {
        static_cast<uint8_t>(message),
        static_cast<uint8_t>(message >>  8),
        static_cast<uint8_t>(message >> 16)
    };

    const uint8_t Status = message & 0xF0;

    if (_IgnoreCC32 && (Status == StatusCodes::ControlChange) && (Event[1] == (ControlChangeNumbers::BankSelect | ControlChangeNumbers::LSB)))
        return;

    uint8_t PortNumber = (message >> 24) & 0x7F;

    if (PortNumber > (_countof(_Stream) - 1))
        PortNumber = 0;

    const DWORD EventSize = (DWORD)((Status >= 0xF8 && Status <= 0xFF) ? 1 : ((Status == StatusCodes::ProgramChange || Status == StatusCodes::ChannelPressure) ? 2 : 3));

    ::BASS_MIDI_StreamEvents(_Stream[PortNumber], BASS_MIDI_EVENTS_RAW, Event, EventSize);
}

/// <summary>
/// Sends a SysEx message to the library.
/// </summary>
void BMPlayer::SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber)
{
    if (portNumber > (_countof(_Stream) - 1))
        portNumber = 0;

    if (portNumber == 0)
    {
        for (auto & Stream : _Stream)
            ::BASS_MIDI_StreamEvents(Stream, BASS_MIDI_EVENTS_RAW, event, (DWORD) size);
    }
    else
        ::BASS_MIDI_StreamEvents(_Stream[portNumber], BASS_MIDI_EVENTS_RAW, event, (DWORD) size);
}

void BMPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    while (sampleCount != 0)
    {
        const size_t ToDo = std::min(sampleCount, MaxSamples);
        const size_t SampleCount = ToDo * ChannelCount;

        ::memset(sampleData, 0, SampleCount * sizeof(*sampleData));

        for (auto & Stream : _Stream)
        {
            ::BASS_ChannelGetData(Stream, _Buffer, BASS_DATA_FLOAT | (DWORD) (SampleCount * sizeof(*_Buffer)));

            // Convert the format of the rendered output.
            for (size_t j = 0; j < SampleCount; ++j)
                sampleData[j] += _Buffer[j];
        }

        sampleData  += SampleCount;
        sampleCount -= (uint32_t) ToDo;
    }
}

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
        HSOUNDFONT hSoundFont = ::CacheAddSoundFont(soundFont.FilePath().c_str());

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
                        for (DWORD i = 0; i < SoundFontInfo.presets; ++i)
                        {
                            const int    PresetNumber = LOWORD(Presets[i]);
                            const int    SrcBank      = HIWORD(Presets[i]);
                            const char * PresetName   = ::BASS_MIDI_FontGetPreset(hSoundFont, PresetNumber, SrcBank);

                            console::print("Bank ", SrcBank, " Preset ", PresetNumber, ": \"", PresetName, "\"");

                            int DstBank = SrcBank;

                            if (soundFont.BankOffset() != -1)
                            {
                                if (DstBank != 128)
                                    DstBank = soundFont.BankOffset();
                            }
                            else
                            {
                                if (DstBank != 128)
                                    DstBank = 1;
                            }

                            BASS_MIDI_FONTEX fex =
                            {
                                hSoundFont, PresetNumber, SrcBank, PresetNumber, (DstBank >> 7) & 0x7F, DstBank & 0x7F
                            };

                            soundFontConfigurations.push_back(fex);
                        }
                    }

                    ::free(Presets);
                }
            }
        }
        else
        {
            BASS_MIDI_FONTEX fex = { hSoundFont, -1, -1, -1, 0, 0 };

            soundFontConfigurations.push_back(fex);
        }

        return true;
    }

    if ((::stricmp_utf8(FileExtension.c_str(), "sflist") == 0) || (::stricmp_utf8(FileExtension.c_str(), "json") == 0))
    {
        sflist_t ** SFList = &_SFList[0];

        if (*SFList)
            SFList = &_SFList[1];

        *SFList = ::CacheAddSoundFontList(soundFont.FilePath().c_str());

        if (!*SFList)
            return false;

        BASS_MIDI_FONTEX * fex = (*SFList)->FontEx;

        for (size_t i = 0, j = (*SFList)->Count; i < j; ++i)
            soundFontConfigurations.push_back(fex[i]);

        return true;
    }

    return false;
}

/// <summary>
/// Sets the bank override, if any, for all channels of all streams.
/// </summary>
void BMPlayer::SetBankOverride() noexcept
{
    _IgnoreCC32 = false;

    // Send a Control Change 32 message (Bank Select LSB) to each channel of each stream.
    for (size_t i = 0; i < _countof(_BankLSBOverride); ++i)
    {
        if (_BankLSBOverride[i] != 0)
            _IgnoreCC32 = true;

        ::BASS_MIDI_StreamEvent(_Stream[i / 16], i % 16, MIDI_EVENT_BANK_LSB, _BankLSBOverride[i]);
    }
}

/// <summary>
/// Unused
/// </summary>
void BMPlayer::CompoundPresets(std::vector<BASS_MIDI_FONTEX> & src, std::vector<long> & channels, std::vector<BASS_MIDI_FONTEX> & dst) noexcept
{
    if (src.size() == 0)
    {
        BASS_MIDI_FONTEX fex = { 0, -1, -1, -1, 0, 0 };

        src.push_back(fex);
    }

    if (channels.size() > 0)
    {
        for (auto & sf : src)
        {
            for (auto & Channel : channels)
            {
                _BankLSBOverride[Channel - 1] = (uint8_t) Channel;

                sf.dbanklsb = (int) Channel;

                dst.push_back(sf);
            }
        }
    }
    else
    {
        for (auto & sf : src)
            dst.push_back(sf);
    }
}

/// <summary>
/// Gets the current error message.
/// </summary>
bool BMPlayer::GetErrorMessage(std::string & errorMessage)
{
    if (_ErrorMessage.length() == 0)
        return false;

    errorMessage = _ErrorMessage;

    return true;
}

#pragma endregion
