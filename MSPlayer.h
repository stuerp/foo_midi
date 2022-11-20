#ifndef __MSPlayer_h__
#define __MSPlayer_h__

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include "interface.h"

class nomidisynth;

class MSPlayer : public MIDIPlayer
{
public:
    // zero variables
    MSPlayer();

    // close, unload
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
    unsigned int synth_id;
    unsigned int bank_id;
    unsigned int extp;
    nomidisynth * synth;
};

#endif
