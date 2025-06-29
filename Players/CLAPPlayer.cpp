
/** $VER: CLAPPlayer.cpp (2025.06.27) P. Stuer - Wrapper for CLAP plugins **/

#include "pch.h"

#include "CLAPPlayer.h"
#include "CLAPHost.h"
#include "Resource.h"
#include "Encoding.h"
#include "Exception.h"

#pragma region player_t

CLAPPlayer::CLAPPlayer(const fs::path & filePath, uint32_t index) noexcept : player_t(), _hPlugIn(), _PlugInIndex(~0u), _PlugIn()
{
    _FilePath = filePath;
    _PlugInIndex = index;
}

CLAPPlayer::~CLAPPlayer()
{
    Shutdown();
}

bool CLAPPlayer::Startup()
{
    if (_PlugIn != nullptr)
        return true;

    _hPlugIn = ::LoadLibraryA(_FilePath.string().c_str());

    if (_hPlugIn == NULL)
        throw midi::exception_t(pfc::string("Unable to load CLAP plug-in from \"") + _FilePath.string().c_str() + "\"");

    LChannel.resize(GetSampleBlockSize());
    RChannel.resize(GetSampleBlockSize());

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(_hPlugIn, "clap_entry");

    if ((Entry != nullptr) && (Entry->init != nullptr) && Entry->init(_FilePath.string().c_str()))
    {
        const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if (Factory != nullptr)
        {
            const auto * Descriptor = Factory->get_plugin_descriptor(Factory, _PlugInIndex);

            if (Descriptor != nullptr)
            {
                _PlugIn = Factory->create_plugin(Factory, &CLAP::Host::GetInstance(), Descriptor->id);

                if (_PlugIn != nullptr)
                {
                    if (_PlugIn->init(_PlugIn))
                    {
                        const double SampleRate  = _SampleRate;
                        const uint32_t MinFrames = 1;
                        const uint32_t MaxFrames = GetSampleBlockSize();

                        if (_PlugIn->activate(_PlugIn, SampleRate, MinFrames, MaxFrames))
                        {
                            if (_PlugIn->start_processing(_PlugIn))
                            {
                                _IsInitialized = true;

                                Configure(_MIDIFlavor, _FilterEffects);
                                Reset();

                                return true;
                            }
                        }
                    }
                    else
                        console::error(STR_COMPONENT_BASENAME " failed to initialize CLAP plug-in.");
                }
                else
                    console::error(STR_COMPONENT_BASENAME " failed to create CLAP plug-in.");
            }
            else
                console::error(STR_COMPONENT_BASENAME " failed to get CLAP plug-in descriptor.");
        }
        else
            console::error(STR_COMPONENT_BASENAME " failed to initialize CLAP factory.");
    }
    else
        console::error(STR_COMPONENT_BASENAME " failed to get CLAP plug-in entry point.");

    return false;
}

void CLAPPlayer::Shutdown()
{
    if (_PlugIn != nullptr)
    {
        _PlugIn->stop_processing(_PlugIn);

        _PlugIn->deactivate(_PlugIn);
                    
        _PlugIn->destroy(_PlugIn);

        _PlugIn = nullptr;
    }

    if (_hPlugIn != NULL)
    {
        ::FreeLibrary(_hPlugIn);
        _hPlugIn = NULL;
    }
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

    if (_PlugIn->process(_PlugIn, &Processor) == CLAP_PROCESS_ERROR)
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
