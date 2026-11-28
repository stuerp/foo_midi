
/** $VER: CLAPPlayer.cpp (2025.10.06) P. Stuer - Wrapper for CLAP plugins **/

#include "pch.h"

#include "CLAPPlayer.h"
#include "CLAPPlugIn.h"

#include "Resource.h"
#include "Encoding.h"
#include "Exception.h"
#include "Log.h"

#include <sdk/playback_control.h>
#include <sdk/threadsLite.h>
#include <mutex>

#pragma region player_t

namespace
{
    struct PlaybackInstance
    {
        playable_location_impl Location;
        fs::path FilePath;
        uint32_t Index;
        std::shared_ptr<CLAP::PlugIn> PlugIn;
        bool EditorRequested = false;
    };

    std::mutex PlaybackInstancesMutex;
    std::vector<PlaybackInstance> PlaybackInstances;
    std::weak_ptr<CLAP::PlugIn> PlaybackEditor;
}

CLAPPlayer::CLAPPlayer(fs::path filePath, uint32_t index, const playable_location * playbackLocation)
    : _FilePath(std::move(filePath)), _Index(index), _OutChannels { }, _AudioOut { }, _AudioOuts { }
{
    if (playbackLocation)
        _PlaybackLocation.emplace(*playbackLocation);
}

bool CLAPPlayer::OpenPlaybackEditor(const fs::path & filePath, uint32_t index, bool activate)
{
    metadb_handle_ptr Track;
    if (!playback_control::get()->get_now_playing(Track))
        return false;

    std::shared_ptr<CLAP::PlugIn> PlugIn;
    {
        const std::lock_guard Lock(PlaybackInstancesMutex);
        for (auto It = PlaybackInstances.rbegin(); It != PlaybackInstances.rend(); ++It)
        {
            if (It->Location != Track->get_location())
                continue;

            std::error_code Error;
            if (!filePath.empty() && (It->Index != index ||
                (It->FilePath != filePath && !fs::equivalent(It->FilePath, filePath, Error))))
                continue;

            if (!activate && It->EditorRequested)
                return true;

            It->EditorRequested = true;
            PlugIn = It->PlugIn;
            break;
        }
    }

    if (!PlugIn)
        return false;

    if (PlugIn->OpenEditor(core_api::get_main_window(), true, activate))
        PlaybackEditor = PlugIn;
    return true;
}

void CLAPPlayer::ClosePlaybackEditor()
{
    if (auto Editor = PlaybackEditor.lock())
        Editor->CloseEditor();

    PlaybackEditor.reset();
}

CLAPPlayer::~CLAPPlayer()
{
    Shutdown();
}

bool CLAPPlayer::Startup()
{
    if (_IsStarted)
        return true;

    _ErrorMessage.clear();
    bool Ready = false;
    fb2k::inMainThreadSynchronous2([this, &Ready]
    {
        try
        {
            if (_CLAPHost.Load(_FilePath, _Index))
            {
                _PlugIn = _CLAPHost.CreatePlugIn();
                Ready = _PlugIn && _PlugIn->Initialize() && _PlugIn->Activate(_SampleRate);
            }
        }
        catch (const std::exception & Error)
        {
            Log.AtError().Write(STR_COMPONENT_BASENAME " failed to start the CLAP plug-in: %s", Error.what());
        }
    });

    if (!Ready)
    {
        Shutdown();
        _ErrorMessage = "Failed to initialize or activate the CLAP plug-in";
        return false;
    }

    if (!_PlugIn->Prefers64bitAudio())
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

    if (!_PlugIn->StartProcessing())
    {
        Shutdown();
        _ErrorMessage = "Failed to start CLAP audio processing";
        return false;
    }

    _IsStarted = true;

    Configure(_MIDIFlavor, _FilterEffects);
    Reset();

    if (_PlaybackLocation)
    {
        const std::lock_guard Lock(PlaybackInstancesMutex);
        PlaybackInstances.push_back({ *_PlaybackLocation, _FilePath, _Index, _PlugIn });
    }

    return true;
}

void CLAPPlayer::Shutdown()
{
    _IsStarted = false;

    if (!_PlugIn)
        return;

    if (_PlaybackLocation)
    {
        const std::lock_guard Lock(PlaybackInstancesMutex);
        std::erase_if(PlaybackInstances, [this](const PlaybackInstance & item) { return item.PlugIn == _PlugIn; });
    }

    _PlugIn->StopProcessing();

    for (void * & Channel : _OutChannels)
    {
        if (_PlugIn->Prefers64bitAudio())
            delete[] static_cast<double *>(Channel);
        else
            delete[] static_cast<float *>(Channel);
        Channel = nullptr;
    }

    _AudioOut = {};

    // The audio thread has stopped using this instance. Close its editor and unload it on the UI thread.
    fb2k::inMainThread([PlugIn = std::move(_PlugIn)] { PlugIn->Terminate(); });
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

        if (!_PlugIn->Process(Processor))
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

    // Clunky in C++ but otherwise the Address Saninitzer goes nuts.
    for (auto Event : _InEvents.Events)
        std::free((void *) Event);

    _InEvents.Events.clear();
}

/// <summary>
/// Resets the player.
/// </summary>
bool CLAPPlayer::Reset()
{
    assert(_countof(_AudioOuts) == 1);

    ResetPort(0, 0);

    return true;
}

void CLAPPlayer::SendEvent(uint32_t data, uint32_t time)
{
    auto Status     = (uint8_t) (data);
    auto Data1      = (uint8_t) (data >>  8);
    auto Data2      = (uint8_t) (data >> 16);
    auto PortNumber = (uint8_t) (data >> 24);

    Log.AtTrace().Write(STR_COMPONENT_BASENAME " is sending MIDI message %8u: %02X %02X %02X.", time, Status, Data1, Data2);

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

    Log.AtTrace().Write(STR_COMPONENT_BASENAME " is sending MIDI SysEx message %8u: %s.", time, Buffer.c_str());
#endif

    _InEvents.Add(data, (uint32_t) size, (uint16_t) portNumber, time);
};
