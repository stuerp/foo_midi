#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <interface.h>

class nomidisynth;

#pragma warning(disable: 4820) // x bytes padding added after data member
class MSPlayer : public MIDIPlayer
{
public:
    MSPlayer();
    virtual ~MSPlayer();

    void set_synth(unsigned int synth);
    void set_bank(unsigned int bank);
    void set_extp(unsigned int extp);

    typedef void (*enum_callback)(unsigned int synth, unsigned int bank, const char * name);

    static void enum_synthesizers(enum_callback callback);

protected:
    virtual void send_event(uint32_t b);
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port);
    virtual void render(audio_sample * out, unsigned long count);

    virtual void shutdown();
    virtual bool startup();

private:
    nomidisynth * synth;

    unsigned int synth_id;
    unsigned int bank_id;
    unsigned int extp;

    char _Padding[4];
};
#pragma warning(default: 4820) // x bytes padding added after data member
