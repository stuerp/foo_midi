
/** $VER: NukedSC-55Player.cpp (2025.06.22) P. Stuer - Wrapper for Nuked SC-55 **/

#include "pch.h"

#include "NukedSC-55Player.h"

#include <filesystem>

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
    return true;
}

void NukedSC55Player::Shutdown()
{
}

void NukedSC55Player::Render(audio_sample * sampleData, uint32_t sampleCount)
{
/*
    int32_t Data[256 * 2];

    while (sampleCount != 0)
    {
        uint32_t ToDo = sampleCount;

        if (ToDo > 256)
            ToDo = 256;

        ::memset(Data, 0, ToDo * 2 * sizeof(*Data));

        for (size_t i = 0; i < _countof(_Synthesizers); ++i)
            _Synthesizers[i]->synthesize_mixing(Data, ToDo, (float) _SampleRate);

        audio_math::convert_from_int32((const t_int32 *) Data, ToDo * 2, sampleData, 65536.0);

        sampleData += ToDo * 2;
        sampleCount -= ToDo;
    }
*/
}

void NukedSC55Player::SendEvent(uint32_t data)
{
}

void NukedSC55Player::SetBasePath(const std::wstring & basePathName) noexcept
{
    _BasePathName = basePathName;
}
