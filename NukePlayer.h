
/** $VER: NukePlayer.h (2023.01.02) Nuke **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

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

    typedef void (*SynthesizerEnumerator)(unsigned int synth, unsigned int bank, const char * name);

    static void EnumerateSynthesizers(SynthesizerEnumerator callback);

protected:
    virtual bool startup() override;
    virtual void shutdown() override;
    virtual void render(audio_sample * out, unsigned long count) override;
    virtual void send_event(uint32_t b) override;
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port) override;

private:
    nomidisynth * _Synth;

    unsigned int _SynthId;
    unsigned int _BankId;
    unsigned int _Extp;
};
#pragma warning(default: 4820) // x bytes padding added after data member
