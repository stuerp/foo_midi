
/** $VER: NukedOPL3Player.h (2025.07.09) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

#include <interface.h>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements the player.
/// </summary>
class NukedOPL3Player : public player_t
{
public:
    NukedOPL3Player() noexcept;
    virtual ~NukedOPL3Player();

    void SetSynth(uint32_t);
    void SetBankNumber(uint32_t);
    void SetSoftPanning(uint32_t);

    static void GetPreset(const std::string & name, uint32_t & synth, uint32_t & bank) noexcept;
    static void GetPreset(size_t index, uint32_t & synth, uint32_t & bank) noexcept;
    static std::string GetPresetName(uint32_t synth, uint32_t bank) noexcept;
    static size_t GetPresetIndex(uint32_t synth, uint32_t bank) noexcept;

    static void InitializePresets(std::function<void (const std::string & name, uint32_t synth, uint32_t bank)> functor) noexcept;
    static void EnumeratePresets(std::function<void (const std::string & name, uint32_t synth, uint32_t bank)> functor) noexcept;

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;

    virtual uint8_t GetPortCount() const noexcept override { return 1; };

    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * /*event*/, size_t /*size*/, uint32_t /*portNumber*/) override { }

private:
    nomidisynth * _Synth;

    uint32_t _SynthId;
    uint32_t _BankId;
    uint32_t _Extp;
};

struct NukedPreset
{
    std::string Name;
    uint32_t SynthId;
    uint32_t BankId;
};

extern std::vector<NukedPreset> _NukedPresets;
