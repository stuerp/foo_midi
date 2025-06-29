
/** $VER: CLAPPlayer.h (2025.06.29) P. Stuer - Wrapper for CLAP plugins **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"
#include "CLAPEventList.h"

#include <filesystem>

namespace fs = std::filesystem;

#include <clap/clap.h>

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements the CLAP player.
/// </summary>
class CLAPPlayer : public player_t
{
public:
    CLAPPlayer(const fs::path & pathName, uint32_t index) noexcept;

    virtual ~CLAPPlayer();

    virtual uint32_t GetSampleBlockSize() const noexcept override { return 2 * 1024; } // 2 channels

private:
    virtual bool Startup();
    virtual void Shutdown();
    virtual void Render(audio_sample *, uint32_t);
    virtual bool Reset();

    virtual void SendEvent(uint32_t, uint32_t time);
    virtual void SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber, uint32_t time);

private:
    HMODULE _hPlugIn;
    fs::path _FilePath;
    uint32_t _PlugInIndex;

    const clap_plugin_t * _PlugIn;
    CLAP::InputEvents _InEvents;
    CLAP::OutputEvents _OutEvents;

    std::vector<float> LChannel;
    std::vector<float> RChannel;
};

#pragma warning(default: 4820) // x bytes padding added after data member
