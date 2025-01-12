
/** $VER: NukeSC-55Player.h (2024.09.29) **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include "Player.h"

#pragma warning(disable: 4266) // A derived class did not override all overloads of a virtual function.
#pragma warning(disable: 4820) // x bytes padding added after data member

#define SDL_MAIN_HANDLED

#include <SDL.h>

/// <summary>
/// Implements the Nuke SC-55 player.
/// </summary>
class NukeSC55Player : public player_t
{
public:
    NukeSC55Player();
    virtual ~NukeSC55Player();

    void SetBasePath(const std::wstring & basePath);

protected:
    virtual bool Startup() override;
    virtual void Shutdown() override;
    virtual void Render(audio_sample *, uint32_t) override;
    virtual void SendEvent(uint32_t) override;
    virtual void SendSysEx(const uint8_t *, size_t, uint32_t) override;

private:
    void Initialize();
    void Terminate();

    size_t ReadROM(const std::wstring & fileName, uint8_t * data, size_t size) const noexcept;

private:
    std::wstring _BasePathName;
    SDL_Thread * _Thread;
};

#pragma warning(default: 4820) // x bytes padding added after data member
