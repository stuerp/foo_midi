
/** $VER: FSPlayer.h (2025.07.23) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "Soundfont.h"
#include "Exception.h"

#include "FS.h"

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a MIDI player using FluidSynth.
/// </summary>
class FSPlayer : public player_t
{
public:
    FSPlayer() noexcept;
    virtual ~FSPlayer();

    void Initialize(const WCHAR * basePath);

    void SetSoundfonts(const std::vector<soundfont_t> & _soundFonts);

    void EnableDynamicLoading(bool enabled = true);
    void EnableEffects(bool enabled = true);
    void SetVoiceCount(uint32_t voices);

    void SetInterpolationMode(uint32_t mode);

    uint32_t GetActiveVoiceCount() const noexcept;

    DWORD GetVersion()
    {
        if (!_API.IsInitialized() || (_API.GetVersion == nullptr))
            throw component::runtime_error("FluidSynth not yet initialized");

        int Major, Minor, Micro;

        _API.GetVersion(&Major, &Minor, &Micro);

        return ((DWORD) Major << 24) | (Minor << 16) | (Micro << 8);
    }

private:
    #pragma region player_t

    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * sampleData, uint32_t samplesCount) override;
    virtual bool Reset() override;

    virtual uint8_t GetPortCount() const noexcept override { return (uint8_t) _Synths.size(); };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    #pragma endregion

    bool InitializeSettings() noexcept;

    fluid_sfloader_t * GetSoundFontLoader(fluid_settings_t * settings) const noexcept;
/*
    bool IsStarted() const noexcept
    {
        if (_Synths.empty())
            return false;

        for (const auto & Synth : _Synths)
            if (Synth == nullptr)
                return false;

        return true;
    }
*/
    void LoadSoundfont(fluid_synth_t * synth, const soundfont_t & sf);

    DWORD MakeDWORD(int a, int b, int c, int d) const noexcept
    {
        return ((DWORD) a << 24) | (b << 16) | (c << 8) | d;
    }

private:
    fluid_settings_t * _Settings; // All synths share the same config.

    static const size_t MaxPorts = 1;

    std::vector<fluid_synth_t *> _Synths; // Each synth corresponds to a port.
    
    std::vector<soundfont_t> _SoundFonts;

    bool _HasBankSelects;
    bool _DoReverbAndChorusProcessing;
    bool _DoDynamicLoading;
    uint32_t _VoiceCount;

    uint32_t _InterpolationMethod;

    FluidSynth::API _API;
};

#pragma warning(default: 4820) // x bytes padding added after data member
