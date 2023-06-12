
/** $VER: EdMPlayer.h (2023.01.02) **/

#pragma once

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
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, unsigned long) override;

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, size_t port) override;

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
