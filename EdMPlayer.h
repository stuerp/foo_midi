
/** $VER: EdMPlayer.h (2023.08.19) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "MIDIPlayer.h"

#include <CMIDIModule.hpp>
#include <COpllDevice.hpp>
#include <CSMF.hpp>
#include <CSccDevice.hpp>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
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
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    void ResetDrumChannels();
    void SetDrumChannel(int channel, int enable);

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
