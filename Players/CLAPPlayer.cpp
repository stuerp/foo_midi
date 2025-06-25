
/** $VER: CLAPPlayer.cpp (2025.06.25) P. Stuer - Wrapper for CLAP plugins **/

#include "pch.h"

#include "CLAPPlayer.h"
#include "Resource.h"
#include "Encoding.h"

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

    console::print(STR_COMPONENT_BASENAME ": CLAPPlayer.Startup()");

    LChannel.resize(GetSampleBlockSize());
    RChannel.resize(GetSampleBlockSize());

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(_hPlugin, "clap_entry");

    if ((Entry != nullptr) && (Entry->init != nullptr) && Entry->init(_PathName.c_str()))
    {
        const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if (Factory != nullptr)
        {
            const auto * Descriptor = Factory->get_plugin_descriptor(Factory, _PlugInIndex);

            if (Descriptor != nullptr)
            {
                const clap_host_t Host = { CLAP_VERSION, nullptr, STR_COMPONENT_BASENAME, "", "", "1.0", nullptr, nullptr, nullptr };

                _PlugIn = Factory->create_plugin(Factory, &Host, Descriptor->id);

                if ((_PlugIn != nullptr) && _PlugIn->init(_PlugIn))
                {
                    if (!VerifyNotePorts())
                        return false;

                    if (!VerifyAudioPorts())
                        return false;

                    const double SampleRate = 33103.0; // 44100.0;
                    const uint32_t MinFrames = 1;
                    const uint32_t MaxFrames = GetSampleBlockSize();

                    return _PlugIn->activate(_PlugIn, SampleRate, MinFrames, MaxFrames);
                }
            }
        }
    }

    return false;
}

void CLAPPlayer::Shutdown()
{
    if (_PlugIn == nullptr)
        return;

    console::print(STR_COMPONENT_BASENAME ": CLAPPlayer.Shutdown()");

    _PlugIn->deactivate(_PlugIn);
                    
    _PlugIn->destroy(_PlugIn);

    _PlugIn = nullptr;
}

void CLAPPlayer::Render(audio_sample * sampleData, uint32_t sampleCount)
{
    console::print(STR_COMPONENT_BASENAME ": CLAPPlayer.Render(", sampleCount, "), ", (int64_t) _EventList.Events.size(), " events");

    const int64_t SteadyTimeNotAvailable = -1;

   // Render the samples.
    {
        float * OutChannels[2] = { LChannel.data(), RChannel.data() };

        clap_audio_buffer_t AudioOut;

        AudioOut.data32        = OutChannels;
        AudioOut.channel_count = _countof(OutChannels);
        AudioOut.latency       = 0;

        clap_audio_buffer_t AudioOutputs[1] = { AudioOut };

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

        _PlugIn->process(_PlugIn, &Process);

        ::memset(sampleData, 0, ((size_t) sampleCount * _countof(OutChannels)) * sizeof(audio_sample));

        for (size_t j = 0; j < sampleCount; ++j)
        {
            sampleData[j * 2 + 0] = (audio_sample) LChannel[j];
            sampleData[j * 2 + 1] = (audio_sample) RChannel[j];
        }
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

    _PathName = pathName;
    _PlugInIndex = index;

    return (_hPlugin != NULL);
}

/// <summary>
/// Verifies the note ports.
/// </summary>
bool CLAPPlayer::VerifyNotePorts() const noexcept
{
    const auto NotePorts = (const clap_plugin_note_ports_t *)(_PlugIn->get_extension(_PlugIn, CLAP_EXT_NOTE_PORTS));

    if (NotePorts == nullptr)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Note Ports extension not implemented.");

        return false;
    }

    constexpr auto InputPort  = true;

    const auto InputPortCount = NotePorts->count(_PlugIn, InputPort);

    if (InputPortCount != 1)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Should have only 1 MIDI input port.");

        return false;
    }

    constexpr auto OutputPort = false;

    const auto OutputPortCount = NotePorts->count(_PlugIn, OutputPort);

    if (OutputPortCount != 0)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Should have no MIDI output ports.");

        return false;
    }

    clap_note_port_info PortInfo = {};

    const auto PortIndex = 0;

    NotePorts->get(_PlugIn, PortIndex, InputPort, &PortInfo);

    if ((PortInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI) == 0)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Should have MIDI dialect support.");

        return false;
    }

    return true;
}

/// <summary>
/// Verifies the audio ports.
/// </summary>
bool CLAPPlayer::VerifyAudioPorts() const noexcept
{
    const auto AudioPorts = static_cast<const clap_plugin_audio_ports_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_AUDIO_PORTS));

    if (AudioPorts == nullptr)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Audio Ports extension not implemented.");

        return false;
    }

    constexpr auto InputPort = true;

    const auto InputPortCount = AudioPorts->count(_PlugIn, InputPort);

    if (InputPortCount != 0)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Should have no input audio ports.");

        return false;
    }

    constexpr auto OutputPort = false;

    const auto OutputPortCount = AudioPorts->count(_PlugIn, OutputPort);

    if (OutputPortCount != 1)
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Should have only 1 output audio port.");

        return false;
    }

    clap_audio_port_info PortInfo = {};

    const auto PortIndex = 0;

    AudioPorts->get(_PlugIn, PortIndex, OutputPort, &PortInfo);

    if (!(PortInfo.channel_count == 2 && ::strcmp(PortInfo.port_type, CLAP_PORT_STEREO) == 0))
    {
        console::warning(STR_COMPONENT_BASENAME ": Plug-in not supported. Should have only 2 stereo output channels.");

        return false;
    }

    return true;
}

void CLAPPlayer::SetBasePath(const std::wstring & basePathName) noexcept
{
    _BasePathName = basePathName;
}
