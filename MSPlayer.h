
/** $VER: MSPlayer.h (2022.12.30) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <interface.h>

class nomidisynth;

/// <summary>
/// Implements the Nuclear Option player.
/// </summary>
#pragma warning(disable: 4820) // x bytes padding added after data member
class MSPlayer : public MIDIPlayer
{
public:
    MSPlayer();
    virtual ~MSPlayer();

    void SetSynth(unsigned int synth);
    void SetBank(unsigned int bank);
    void SetExtp(unsigned int extp);

    typedef void (*enum_callback)(unsigned int synth, unsigned int bank, const char * name);

    static void enum_synthesizers(enum_callback callback);

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
