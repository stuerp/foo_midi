
/** $VER: CLAPPlayer.cpp (2025.07.09) P. Stuer - Wrapper for CLAP plugins **/

#include "pch.h"

#include "CLAPPlayer.h"
#include "CLAPHost.h"

#include "Resource.h"
#include "Encoding.h"
#include "Exception.h"

#pragma region player_t

CLAPPlayer::CLAPPlayer(CLAP::Host * host) noexcept : player_t(), _OutChannels { }, _AudioOut { }, _AudioOuts { }
{
    _Host = host;
}

CLAPPlayer::~CLAPPlayer()
{
    Shutdown();
}

bool CLAPPlayer::Startup()
{
    if (_IsStarted)
        return true;

    if (!_Host->IsPlugInLoaded())
        return false;

    if (!_Host->PlugInPrefers64bitAudio())
    {
        _OutChannels[0] = new float[GetBlockSize()];
        _OutChannels[1] = new float[GetBlockSize()];

        _AudioOut.data32        = (float **) _OutChannels;
        _AudioOut.channel_count = _countof(_OutChannels);
    }
    else
    {
        _OutChannels[0] = new double[GetBlockSize()];
        _OutChannels[1] = new double[GetBlockSize()];

        _AudioOut.data64        = (double **) _OutChannels;
        _AudioOut.channel_count = _countof(_OutChannels);
    }

    _AudioOuts[0] = _AudioOut;

    if (!_Host->ActivatePlugIn((double) _SampleRate, 1, GetBlockSize()))
        return false;

    _IsStarted = true;

    Configure(_MIDIFlavor, _FilterEffects);
    Reset();

    return true;
}

void CLAPPlayer::Shutdown()
{
    if (!_IsStarted)
        return;

    if (!_Host->IsPlugInLoaded())
        return;

    _Host->DeactivatePlugIn();

    if (_OutChannels[1])
    {
        delete[] _OutChannels[1];
        _OutChannels[1] = nullptr;
    }

    if (_OutChannels[0])
    {
        delete[] _OutChannels[0];
        _OutChannels[0] = nullptr;
    }

    _IsStarted = false;
}

void CLAPPlayer::Render(audio_sample * dstFrames, uint32_t dstCount)
{
    ::memset(dstFrames, 0, ((size_t) dstCount * _countof(_OutChannels)) * sizeof(audio_sample));

    try
    {
        const int64_t SteadyTimeNotAvailable = -1;

        clap_process_t Processor =
        {
            .steady_time         = SteadyTimeNotAvailable,
            .frames_count        = dstCount,
            .transport           = nullptr,
            .audio_inputs        = nullptr,
            .audio_outputs       = _AudioOuts,
            .audio_inputs_count  = 0,
            .audio_outputs_count = _countof(_AudioOuts),
            .in_events           = &_InEvents,
            .out_events          = &_OutEvents,
        };

        if (_Host->Process(Processor))
            return; // throw exception_io_data("CLAP plug-in event processing failed");

        if (_AudioOut.data32 != nullptr)
        {
            const float * _LChannel = (float *) _OutChannels[0];
            const float * _RChannel = (float *) _OutChannels[1];

            for (size_t j = 0; j < dstCount; ++j)
            {
                dstFrames[j * 2 + 0] = (audio_sample) _LChannel[j];
                dstFrames[j * 2 + 1] = (audio_sample) _RChannel[j];
            }
        }
        else
        {
            const double * _LChannel = (double *) _OutChannels[0];
            const double * _RChannel = (double *) _OutChannels[1];

            for (size_t j = 0; j < dstCount; ++j)
            {
                dstFrames[j * 2 + 0] = (audio_sample) _LChannel[j];
                dstFrames[j * 2 + 1] = (audio_sample) _RChannel[j];
            }
        }
    }
    catch (...) { }

    _InEvents.Events.clear();
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
    auto Status     = (uint8_t) (data);
    auto Data1      = (uint8_t) (data >>  8);
    auto Data2      = (uint8_t) (data >> 16);
    auto PortNumber = (uint8_t) (data >> 24);

#ifdef _DEBUG
//  console::print(::FormatText("%8u: %02X %02X %02X", time, Status, Data1, Data2).c_str());
#endif

    _InEvents.Add(Status, Data1, Data2, PortNumber, time);
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

//  console::print(::FormatText("%8u: %s", time, Buffer.c_str()).c_str());
#endif

    _InEvents.Add(data, (uint32_t) size, (uint16_t) portNumber, time);
};
