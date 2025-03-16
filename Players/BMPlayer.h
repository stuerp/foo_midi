
/** $VER: BMPlayer.h (2025.03.16) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "SoundFont.h"

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

    void SetSoundFonts(const std::vector<soundfont_t> & _soundFonts);

    void SetInterpolationMode(uint32_t interpolationMode);
    void EnableEffects(bool enable = true);
    void SetVoiceCount(uint32_t voices);

    uint32_t GetActiveVoiceCount() const noexcept;

private:
    #pragma region player_t

    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * sampleData, uint32_t samplesCount) override;
    virtual bool Reset() override;

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;

    #pragma endregion

    bool LoadSoundFontConfiguration(const soundfont_t & soundFont, std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations) noexcept;

    bool IsStarted() const noexcept
    {
        for (const auto & Stream : _Streams)
            if (Stream == 0)
                return false;

        return true;
    }

    static bool IsOneOf(const std::wstring & ext, const std::vector<std::wstring> & extensions);

private:
    static const uint32_t MaxSamples = 512;
    static const uint32_t ChannelCount = 2;

    float _Buffer[MaxSamples * ChannelCount];

    std::string _ErrorMessage;

    std::vector<HSOUNDFONT> _SoundFontHandles;
    sflist_t * _SFList[2];

    static const size_t MaxPorts = 16;

    HSTREAM _Streams[MaxPorts]; // Each stream corresponds to a port.

    std::string _SoundFontDirectoryPath;
    std::vector<soundfont_t> _SoundFonts;

    float _Volume;
    uint32_t _InterpolationMode;
    uint32_t _VoiceCount;

    bool _DoReverbAndChorusProcessing;
    bool _IgnoreCC32; // Ignore Control Change 32 (Bank Select) messages in the MIDI stream.
};

#pragma warning(default: 4820) // x bytes padding added after data member
