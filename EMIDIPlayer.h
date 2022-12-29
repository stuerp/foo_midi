#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <CMIDIModule.hpp>
#include <COpllDevice.hpp>
#include <CSMF.hpp>
#include <CSccDevice.hpp>

#pragma warning(disable: 4820) // x bytes padding added after data member
class EMIDIPlayer : public MIDIPlayer
{
public:
    EMIDIPlayer();
    virtual ~EMIDIPlayer();

private:
    virtual void send_event(uint32_t);
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port);
    virtual void render(audio_sample *, unsigned long);

    virtual void shutdown();
    virtual bool startup();

    void reset_drum_channels();
    void set_drum_channel(int channel, int enable);

private:
    dsa::CMIDIModule mModule[8];

    enum
    {
        mode_gm = 0,
        mode_gm2,
        mode_gs,
        mode_xg
    } synth_mode;

    BYTE gs_part_to_ch[16];
    BYTE drum_channels[16];

    bool bInitialized;
    char _Padding[3];
};
#pragma warning(default: 4820) // x bytes padding added after data member
