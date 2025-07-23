
/** $VER: BMPlayer.h (2025.07.23) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "Soundfont.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded);

struct sflist_t;

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a MIDI player using BASS MIDI.
/// </summary>
class BMPlayer : public player_t
{
public:
    BMPlayer();
    virtual ~BMPlayer();

    void SetSoundfonts(const std::vector<soundfont_t> & _soundFonts);

    void SetInterpolationMode(uint32_t interpolationMode);
    void EnableEffects(bool enable = true);
    void SetVoiceCount(uint32_t voices);

    uint32_t GetActiveVoiceCount() const noexcept;

    DWORD GetVersion() const noexcept
    {
        return ::BASS_GetVersion();
    }

    DWORD GetMIDIVersion() const noexcept
    {
        return ::BASS_MIDI_GetVersion();
    }

private:
    #pragma region player_t

    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * sampleData, uint32_t samplesCount) override;
    virtual bool Reset() override;

    virtual uint8_t GetPortCount() const noexcept override { return (uint8_t) _Streams.size(); };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    #pragma endregion

    void LoadSoundfontConfiguration(const soundfont_t & soundFont, std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations);
/*
    bool IsStarted() const noexcept
    {
        for (const auto & Stream : _Streams)
            if (Stream == 0)
                return false;

        return true;
    }
*/
private:
    static const uint32_t MaxFrames = 512;
    static const uint32_t MaxChannels = 2;

    float * _SrcFrames;

    std::vector<HSOUNDFONT> _SoundfontHandles;

    static const size_t MaxPorts = 1;

    std::vector<HSTREAM> _Streams; // Each stream corresponds to a port.

    std::vector<soundfont_t> _Soundfonts;

    float _Volume;
    uint32_t _InterpolationMode;
    uint32_t _VoiceCount;

    bool _HasBankSelects;
    bool _DoReverbAndChorusProcessing;
    bool _IgnoreCC32; // Ignore Control Change 32 (Bank Select) messages in the MIDI stream.

    uint8_t _NRPNLSB[16]; // The last NRPN LSB seen for a channel.
    uint8_t _NRPNMSB[16]; // The last NRPN MSB seen for a channel.
};

#pragma warning(default: 4820) // x bytes padding added after data member
