
/** $VER: FSPlayer.h (2025.03.15) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "SoundFont.h"

#include "FluidSynth.h"

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a MIDI player using FluidSynth.
/// </summary>
class FSPlayer : public player_t
{
public:
    FSPlayer();
    virtual ~FSPlayer();

    void Initialize(const WCHAR * basePath);

    void SetSoundFonts(const std::vector<soundfont_t> & _soundFonts);

    void EnableDynamicLoading(bool enabled = true);
    void EnableEffects(bool enabled = true);
    void SetVoiceCount(uint32_t voices);

    void SetInterpolationMode(uint32_t mode);

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

    fluid_sfloader_t * GetSoundFontLoader(fluid_settings_t * settings) const;

    bool IsStarted() const noexcept
    {
        for (const auto & Synth : _Synths)
            if (Synth == nullptr)
                return false;

        return true;
    }

private:
    std::string _ErrorMessage;

    static const size_t MaxPorts = 16;

    fluid_synth_t * _Synths[MaxPorts]; // Each synth corresponds to a port.
    fluid_settings_t * _Settings[_countof(_Synths)];

    std::vector<soundfont_t> _SoundFonts;

    bool _DoDynamicLoading;
    bool _DoReverbAndChorusProcessing;
    uint32_t _VoiceCount;

    uint32_t _InterpolationMethod;

    FluidSynth _FluidSynth;
};
#pragma warning(default: 4820) // x bytes padding added after data member
