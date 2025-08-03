
/** $VER: EdMPlayer.h (2025.08.03) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

#include <CMIDIModule.hpp>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

class EdMPlayer : public player_t
{
public:
    EdMPlayer();
    virtual ~EdMPlayer();

private:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual bool Reset() override;

    virtual uint8_t GetPortCount() const noexcept override { return _countof(_Modules); };

    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t * event, size_t size, uint32_t portNumber) override;

    void InitializeDrumChannels() noexcept;
    void SetDrumChannel(int channel, bool enable) noexcept;

private:
    static const int MaxModules = 16;

    static const uint32_t MaxFrames = 256;
    static const uint32_t MaxChannels = 2;

    std::vector<int32_t> _SrcFrames;

    dsa::CMIDIModule _Modules[MaxModules];

    enum
    {
        ModeGM = 0,
        ModeGM2,
        ModeGS,
        ModeXG
    } _SynthMode;

    uint8_t _PartToChannel[16];
    bool _IsDrumChannel[16];         // True if the channel is a drum channel
};

#pragma warning(default: 4820) // x bytes padding added after data member
