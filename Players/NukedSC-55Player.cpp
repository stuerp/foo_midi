
/** $VER: NukedSC-55Player.cpp (2025.07.09) P. Stuer - Wrapper for Nuked SC-55 **/

#include "pch.h"

#include "NukedSC-55Player.h"

#include "Encoding.h"

#pragma region player_t

NukedSC55Player::NukedSC55Player() : player_t()
{
}

NukedSC55Player::~NukedSC55Player()
{
    Shutdown();
}

bool NukedSC55Player::Startup()
{
    if (_IsStarted)
        return true;

    _IsStarted = true;

    return true;
}

void NukedSC55Player::Shutdown()
{
}

void NukedSC55Player::Render(audio_sample * dstFrames, uint32_t dstCount)
{
}

void NukedSC55Player::SendEvent(uint32_t data)
{
}
