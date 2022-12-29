#pragma once
#define __ADLPlayer_h__

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#pragma warning(disable: 4820) // x bytes padding added after data member
class ADLPlayer : public MIDIPlayer
{
public:
    // zero variables
    ADLPlayer();

    // close, unload
    virtual ~ADLPlayer();

    enum
    {
        ADLMIDI_EMU_NUKED = 0,
        ADLMIDI_EMU_NUKED_174,
        ADLMIDI_EMU_DOSBOX
    };

    // configuration
    void setCore(unsigned);
    void setBank(unsigned);
    void setChipCount(unsigned);
    void set4OpCount(unsigned);
    void setFullPanning(bool);

protected:
    virtual void send_event(uint32_t b);
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port);
    virtual void render(audio_sample * out, unsigned long count);

    virtual void shutdown();
    virtual bool startup();

private:
    static void render_internal(void * context, int count, short * out);

    void reset_drum_channels();

private:
    struct ADL_MIDIPlayer * midiplay[3];

    unsigned uEmuCore;
    unsigned uBankNumber;
    unsigned uChipCount;
    unsigned u4OpCount;

    bool bFullPanning;
    char _Padding[7];
};
#pragma warning(default: 4820) // x bytes padding added after data member
