
/** $VER: NukePlayer.h (2023.08.19) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

#include <functional>

class nomidisynth;

/// <summary>
/// Implements the Nuke player.
/// </summary>
#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member
class NukePlayer : public player_t
{
public:
    NukePlayer();
    virtual ~NukePlayer();

    void SetSynth(uint32_t);
    void SetBankNumber(uint32_t);
    void SetExtp(uint32_t);

    static void GetPreset(const pfc::string name, unsigned int & synth, unsigned int & bank);
    static void GetPreset(size_t index, unsigned int & synth, unsigned int & bank);
    static pfc::string GetPresetName(unsigned int synth, unsigned int bank);
    static size_t GetPresetIndex(unsigned int synth, unsigned int bank);

    static void InitializePresets(std::function<void (const pfc::string name, unsigned int synth, unsigned int bank)> functor) noexcept;
    static void EnumeratePresets(std::function<void (const pfc::string name, unsigned int synth, unsigned int bank)> functor) noexcept;

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual void SendEvent(uint32_t data) override;
    virtual void SendSysEx(const uint8_t * /*event*/, size_t /*size*/, uint32_t /*portNumber*/) override { }

private:
    nomidisynth * _Synth;

    unsigned int _SynthId;
    unsigned int _BankId;
    unsigned int _Extp;
};
#pragma warning(default: 4820) // x bytes padding added after data member

#pragma region Nuke Presets
struct NukePreset
{
    pfc::string Name;
    unsigned int SynthId;
    unsigned int BankId;
};
#pragma endregion
