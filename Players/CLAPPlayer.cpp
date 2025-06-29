
/** $VER: CLAPPlayer.cpp (2025.06.27) P. Stuer - Wrapper for CLAP plugins **/

#include "pch.h"

#include "CLAPPlayer.h"
#include "CLAPHost.h"

#include "Resource.h"
#include "Encoding.h"
#include "Exception.h"

#pragma region player_t

CLAPPlayer::CLAPPlayer() noexcept : player_t()
{
}

CLAPPlayer::~CLAPPlayer()
{
    Shutdown();
}

bool CLAPPlayer::Startup()
{
    if (_IsInitialized)
        return true;

    auto & Host = CLAP::Host::GetInstance();

    if (!Host.IsPlugInLoaded())
        return false;

    LChannel.resize(GetSampleBlockSize());
    RChannel.resize(GetSampleBlockSize());

    if (!Host.ActivatePlugIn((double) _SampleRate, 1, GetSampleBlockSize()))
        return false;

    _IsInitialized = true;

    Configure(_MIDIFlavor, _FilterEffects);
    Reset();

    return true;
}

void CLAPPlayer::Shutdown()
{
    if (!_IsInitialized)
        return;

    _IsInitialized = false;

    auto & Host = CLAP::Host::GetInstance();

    if (!Host.IsPlugInLoaded())
        return;

    Host.DeactivatePlugIn();
}

void CLAPPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    float * OutChannels[2] = { LChannel.data(), RChannel.data() };

    const clap_audio_buffer_t AudioOut =
    {
        .data32        = OutChannels,
        .channel_count = _countof(OutChannels)
    };

    clap_audio_buffer_t AudioOutputs[1] = { AudioOut };

    const int64_t SteadyTimeNotAvailable = -1;

    clap_process_t Processor =
    {
        .steady_time         = SteadyTimeNotAvailable,
        .frames_count        = sampleCount,
        .transport           = nullptr,
        .audio_inputs        = nullptr,
        .audio_outputs       = AudioOutputs,
        .audio_inputs_count  = 0,
        .audio_outputs_count = _countof(AudioOutputs),
        .in_events           = &_InEvents,
        .out_events          = &_OutEvents,
    };

    ::memset(sampleData, 0, ((size_t) sampleCount * _countof(OutChannels)) * sizeof(audio_sample));

//  console::print(STR_COMPONENT_BASENAME ": CLAPPlayer.Render(", sampleCount, "), ", (int64_t) _InEvents.Events.size(), " events");

    if (CLAP::Host::GetInstance().Process(Processor))
        return; // throw exception_io_data("CLAP plug-in event processing failed");

//  console::print(STR_COMPONENT_BASENAME ": CLAPPlayer.Render()");

    for (size_t j = 0; j < sampleCount; ++j)
    {
        sampleData[j * 2 + 0] = (audio_sample) LChannel[j];
        sampleData[j * 2 + 1] = (audio_sample) RChannel[j];
    }

    _InEvents.Clear();
}

/// <summary>
/// Resets the player.
/// </summary>
bool CLAPPlayer::Reset()
{
    ResetPort(0, 0);

    return true;
}

void CLAPPlayer::SendEvent(uint32_t data, uint32_t time)
{
    auto Status = (uint8_t) (data);
    auto Data1  = (uint8_t) (data >>  8);
    auto Data2  = (uint8_t) (data >> 16);
//  auto Port   = (uint8_t) (data >> 24);

    console::print(::FormatText("%8u: %02X %02X %02X", time, Status, Data1, Data2).c_str());

    _InEvents.Add(Status, Data1, Data2, time);
}

void CLAPPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber, uint32_t time)
{
#ifdef _DEBUG
    std::string Buffer; Buffer.resize((size * 3) + 1); // 3 characters + terminator

    char * p = Buffer.data();

    for (size_t i = 0; i < size; ++i)
    {
        ::sprintf_s(p, 4, "%02X ", data[i]);
        p += 3;
    }

    console::print(::FormatText("%8u: %s", time, Buffer.c_str()).c_str());
#endif

    _InEvents.Add(data, size, time);
};
