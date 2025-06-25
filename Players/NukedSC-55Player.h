
/** $VER: NukedSC-55Player.h (2025.06.22) P. Stuer - Wrapper for Nuked SC-55 **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements the Nuked SC-55 player.
/// </summary>
class NukedSC55Player : public player_t
{
public:
    NukedSC55Player();

    virtual ~NukedSC55Player();

    void SetBasePath(const std::wstring & basePath) noexcept;

private:
    virtual bool Startup();
    virtual void Shutdown();
    virtual void Render(audio_sample *, uint32_t);
    virtual bool Reset() { return false; }

    virtual void SendEvent(uint32_t);
    virtual void SendEvent(uint32_t, uint32_t time) { };

private:
    std::wstring _BasePathName;
};

#pragma warning(default: 4820) // x bytes padding added after data member
