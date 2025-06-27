
/** $VER: CLAPPlayer.cpp (2025.06.27) P. Stuer - Wrapper for CLAP plugins **/

#include "pch.h"

#include "CLAPPlayer.h"
#include "CLAPHost.h"
#include "Resource.h"
#include "Encoding.h"
#include "Exception.h"

#include <filesystem>

#pragma region player_t

CLAPPlayer::CLAPPlayer() : player_t(), _hPlugin(), _PlugInIndex(~0u), _PlugIn()
{
}

CLAPPlayer::~CLAPPlayer()
{
    Shutdown();
}

bool CLAPPlayer::Startup()
{
    if (_PlugIn != nullptr)
        return true;

    LChannel.resize(GetSampleBlockSize());
    RChannel.resize(GetSampleBlockSize());

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(_hPlugin, "clap_entry");

    if ((Entry != nullptr) && (Entry->init != nullptr) && Entry->init(_FilePath.c_str()))
    {
        const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if (Factory != nullptr)
        {
            const auto * Descriptor = Factory->get_plugin_descriptor(Factory, _PlugInIndex);

            if (Descriptor != nullptr)
            {
                _PlugIn = Factory->create_plugin(Factory, &CLAP::Host::Parameters, Descriptor->id);

                if (_PlugIn != nullptr)
                {
                    if (_PlugIn->init(_PlugIn))
                    {
                        const double SampleRate  = _SampleRate;
                        const uint32_t MinFrames = 1;
                        const uint32_t MaxFrames = GetSampleBlockSize();

                        return _PlugIn->activate(_PlugIn, SampleRate, MinFrames, MaxFrames);
                    }
                    else
                        console::error(STR_COMPONENT_BASENAME " failed to initialized CLAP plug-in.");
                }
                else
                    console::error(STR_COMPONENT_BASENAME " failed to create CLAP plug-in.");
            }
        }
    }

    return false;
}

void CLAPPlayer::Shutdown()
{
    if (_PlugIn == nullptr)
        return;

    _PlugIn->deactivate(_PlugIn);
                    
    _PlugIn->destroy(_PlugIn);

    _PlugIn = nullptr;
}

void CLAPPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
//  console::print(STR_COMPONENT_BASENAME ": CLAPPlayer.Render(", sampleCount, "), ", (int64_t) _EventList.Events.size(), " events");

    float * OutChannels[2] = { LChannel.data(), RChannel.data() };

    const clap_audio_buffer_t AudioOut =
    {
        .data32        = OutChannels,
        .channel_count = _countof(OutChannels)
    };

    clap_audio_buffer_t AudioOutputs[1] = { AudioOut };

    const int64_t SteadyTimeNotAvailable = -1;

    clap_process_t Process =
    {
        .steady_time         = SteadyTimeNotAvailable,
        .frames_count        = sampleCount,
        .transport           = nullptr,
        .audio_inputs        = nullptr,
        .audio_outputs       = AudioOutputs,
        .audio_inputs_count  = 0,
        .audio_outputs_count = _countof(AudioOutputs),
        .in_events           = &_EventList,
        .out_events          = nullptr
    };

    ::memset(sampleData, 0, ((size_t) sampleCount * _countof(OutChannels)) * sizeof(audio_sample));

    if (_PlugIn->process(_PlugIn, &Process) == CLAP_PROCESS_ERROR)
        return; // throw exception_io_data("CLAP plug-in event processing failed");

    for (size_t j = 0; j < sampleCount; ++j)
    {
        sampleData[j * 2 + 0] = (audio_sample) LChannel[j];
        sampleData[j * 2 + 1] = (audio_sample) RChannel[j];
    }

    _EventList.Clear();
}

void CLAPPlayer::SendEvent(uint32_t data, uint32_t time)
{
    auto Status = (uint8_t) (data);
    auto Data1  = (uint8_t) (data >>  8);
    auto Data2  = (uint8_t) (data >> 16);
//  auto Port   = (uint8_t) (data >> 24);

    _EventList.Add(Status, Data1, Data2, time);
}

void CLAPPlayer::SendSysEx(const uint8_t * data, size_t size, uint32_t portNumber, uint32_t time)
{
    _EventList.Add(data, size, time);
};

bool CLAPPlayer::LoadPlugIn(const char * pathName, uint32_t index)
{
    _hPlugin = ::LoadLibraryA(pathName);

    _FilePath = pathName;
    _PlugInIndex = index;

    return (_hPlugin != NULL);
}
