
/** $VER: EdMPlayer.h (2023.01.02) **/

#pragma once

#pragma warning(disable: 5045)

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <CMIDIModule.hpp>
#include <COpllDevice.hpp>
#include <CSMF.hpp>
#include <CSccDevice.hpp>

#pragma warning(disable: 4820) // x bytes padding added after data member
class EdMPlayer : public MIDIPlayer
{
public:
    EdMPlayer();
    virtual ~EdMPlayer();

private:
    virtual bool startup() override;
    virtual void shutdown() override;
    virtual void render(audio_sample *, unsigned long) override;

    virtual void send_event(uint32_t) override;
    virtual void send_sysex(const uint8_t * event, size_t size, size_t port) override;

    void reset_drum_channels();
    void set_drum_channel(int channel, int enable);

private:
    dsa::CMIDIModule _Module[8];

    enum
    {
        ModeGM = 0,
        ModeGM2,
        ModeGS,
        ModeXG
    } _SynthMode;

    uint8_t _GSPartToChannel[16];
    uint8_t _DrumChannels[16];

    bool _Initialized;
};
#pragma warning(default: 4820) // x bytes padding added after data member
