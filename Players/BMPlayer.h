
/** $VER: BMPlayer.h (2024.08.25) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "SoundFont.h"

#include <bassmidi.h>

extern bool GetSoundFontStatistics(uint64_t & sampleDataSize, uint64_t & sampleDataLoaded);

struct sflist_presets;

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

    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    virtual bool GetErrorMessage(std::string & errorMessage) override;

    #pragma endregion

    bool LoadSoundFontConfiguration(const soundfont_t & soundFont, std::vector<BASS_MIDI_FONTEX> & soundFontConfigurations) noexcept;
    void SetBankOverride() noexcept;

    void CompoundPresets(std::vector<BASS_MIDI_FONTEX> & in, std::vector<long> & channels, std::vector<BASS_MIDI_FONTEX> & out) noexcept;

    bool IsStarted() const noexcept
    {
        for (const auto & Stream : _Stream)
            if (Stream == 0)
                return false;

        return true;
    }

private:
    static const uint32_t MaxSamples = 512;
    static const uint32_t ChannelCount = 2;

    float _Buffer[MaxSamples * ChannelCount];

    std::string _ErrorMessage;

    std::vector<HSOUNDFONT> _SoundFontHandles;
    sflist_presets * _Presets[2];

    HSTREAM _Stream[16];

    std::string _SoundFontDirectoryPath;
    std::vector<soundfont_t> _SoundFonts;

    float _Volume;
    uint32_t _InterpolationMode;
    uint32_t _VoiceCount;

    uint8_t _BankLSBOverride[_countof(_Stream) * 16]; // No. of streams times 16 channels

    bool _DoReverbAndChorusProcessing;
    bool _IgnoreCC32; // Ignore Control Change 32 (Bank Select) messages in the MIDI stream.
};
#pragma warning(default: 4820) // x bytes padding added after data member
