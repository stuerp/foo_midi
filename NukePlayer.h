
/** $VER: NukePlayer.h (2023.01.04) **/

#pragma once

#include "MIDIPlayer.h"

#include <functional>

class nomidisynth;

/// <summary>
/// Implements the Nuke player.
/// </summary>
#pragma warning(disable: 4820) // x bytes padding added after data member
class NukePlayer : public MIDIPlayer
{
public:
    NukePlayer();
    virtual ~NukePlayer();

    void SetSynth(unsigned int synth);
    void SetBank(unsigned int bank);
    void SetExtp(unsigned int extp);

    static void GetPreset(const pfc::string8 name, unsigned int & synth, unsigned int & bank);
    static void GetPreset(size_t index, unsigned int & synth, unsigned int & bank);
    static pfc::string8 GetPresetName(unsigned int synth, unsigned int bank);
    static size_t GetPresetIndex(unsigned int synth, unsigned int bank);

    static void InitializePresets(std::function<void (const pfc::string8 name, unsigned int synth, unsigned int bank)> functor) noexcept;
    static void EnumeratePresets(std::function<void (const pfc::string8 name, unsigned int synth, unsigned int bank)> functor) noexcept;

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample * out, unsigned long count) override;
    virtual void SendEvent(uint32_t b) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, size_t port) override;

private:
    nomidisynth * _Synth;

    unsigned int _SynthId;
    unsigned int _BankId;
    unsigned int _Extp;
};
#pragma warning(default: 4820) // x bytes padding added after data member

#pragma region("Nuke Presets")
struct NukePreset
{
    pfc::string8 Name;
    unsigned int SynthId;
    unsigned int BankId;
};
#pragma endregion
