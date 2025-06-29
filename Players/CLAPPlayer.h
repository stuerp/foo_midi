
/** $VER: CLAPPlayer.h (2025.06.29) P. Stuer - Wrapper for CLAP plugins **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "CLAPEvents.h"

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements the CLAP player.
/// </summary>
class CLAPPlayer : public player_t
{
public:
    CLAPPlayer() noexcept;

    virtual ~CLAPPlayer();

    virtual uint32_t GetSampleBlockSize() const noexcept override { return 2 * 1024; } // 2 channels

private:
    #pragma region player_t

    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual bool Reset() override;

    virtual void SendEvent(uint32_t data) override { SendEvent(data, 0); }
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber) override { SendSysEx(data, size, portNumber, 0); }

    virtual void SendEvent(uint32_t, uint32_t time) override;
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber, uint32_t time) override;

    #pragma endregion

private:
    CLAP::InputEvents _InEvents;
    CLAP::OutputEvents _OutEvents;

    std::vector<float> LChannel;
    std::vector<float> RChannel;
};

#pragma warning(default: 4820) // x bytes padding added after data member
