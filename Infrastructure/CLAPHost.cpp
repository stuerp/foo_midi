
/** $VER: CLAPHost.cpp (2025.06.28) P. Stuer **/

#include "pch.h"

#include <sdk/foobar2000-lite.h>

#include "Configuration.h"
#include "Resource.h"
#include "CLAPHost.h"
#include "CLAPPlayer.h"

namespace CLAP
{

/// <summary>
/// Gets the CLAP plug-ins.
/// </summary>
std::vector<PlugIn> Host::GetPlugIns(const fs::path & directoryPath) noexcept
{
    _PlugIns.clear();

    if (directoryPath.empty())
        return _PlugIns;

    GetPlugIns_(directoryPath);

    std::sort(_PlugIns.begin(), _PlugIns.end(), [](PlugIn a, PlugIn b) { return a.Name < b.Name; });

    return _PlugIns;
}

/// <summary>
/// Gets the CLAP plug-ins.
/// </summary>
void Host::GetPlugIns_(const fs::path & directoryPath) noexcept
{
    for (const auto & Entry : fs::directory_iterator(directoryPath))
    {
        if (Entry.is_directory())
        {
            GetPlugIns_(Entry.path());
        }
        else
        if ((Entry.path().extension() == ".clap") || (Entry.path().extension() == ".dll"))
        {
            console::print(STR_COMPONENT_BASENAME " is examining \"", Entry.path().string().c_str(), "\"...");

            GetPlugIns(Entry.path(), [this, Entry](const std::string & plugInName, uint32_t index, bool hasGUI)
            {
                PlugIn PlugIn =
                {
                    .Name     = "CLAP " + plugInName,
                    .Index    = index,
                    .FilePath = Entry.path(),
                    .HasGUI   = hasGUI
                };

                _PlugIns.push_back(PlugIn);
            });
        }
    }
}


/// <summary>
/// Gets the CLAP plug-ins in plug-in file.
/// </summary>
void Host::GetPlugIns(const fs::path & filePath, const std::function<void(const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept
{
    HMODULE hPlugIn = ::LoadLibraryA(filePath.string().c_str());

    if (hPlugIn == NULL)
        return;

    auto PlugInEntry = (const clap_plugin_entry_t *) ::GetProcAddress(hPlugIn, "clap_entry");

    try
    {
        if ((PlugInEntry != nullptr) && (PlugInEntry->init != nullptr) && PlugInEntry->init(filePath.string().c_str()))
        {
            const auto * Factory = (const clap_plugin_factory_t *) PlugInEntry->get_factory(CLAP_PLUGIN_FACTORY_ID);

            if (Factory != nullptr)
            {
                uint32_t PlugInCount = Factory->get_plugin_count(Factory);

                for (uint32_t i = 0; i < PlugInCount; ++i)
                {
                    const auto * Descriptor = Factory->get_plugin_descriptor(Factory, i);

                    if (Descriptor != nullptr)
                    {
                        console::print
                        (
                            "Id: \"", Descriptor->id, "\", ",
                            "Name: \"", Descriptor->name, "\", ",
                            "Version: \"", Descriptor->version, "\", ",
                            "Description: \"", Descriptor->description, "\"",
                            "Vendor: \"", Descriptor->vendor, "\", ",
                            "URL: \"", Descriptor->url, "\"",
                            "Manual URL: \"", Descriptor->manual_url, "\", ",
                            "Support URL: \"", Descriptor->support_url, "\", "
                        );

                        if (::clap_version_is_compatible(Descriptor->clap_version))
                        {
                            const auto * PlugIn = Factory->create_plugin(Factory, this, Descriptor->id);

                            if (PlugIn != nullptr)
                            {
                                if (PlugIn->init(PlugIn))
                                {
                                    if (VerifyNotePorts(PlugIn) && VerifyAudioPorts(PlugIn))
                                        callback(Descriptor->name, i, HasGUI(PlugIn, false));
                                }

                                PlugIn->destroy(PlugIn);
                            }
                        }
                        else
                            console::print(STR_COMPONENT_BASENAME,
                                " CLAP host version ",
                                CLAP_VERSION.major, ".", CLAP_VERSION.minor, ".", CLAP_VERSION.revision,
                                " is not compatible with plug-in version ",
                                Descriptor->clap_version.major, ".", Descriptor->clap_version.minor, ".", Descriptor->clap_version.revision);
                    }
                }
            }

            PlugInEntry->deinit();
        }
    }
    catch (...) { };

    ::FreeLibrary(hPlugIn);
}

/// <summary>
/// Verifies the note ports.
/// </summary>
bool Host::VerifyNotePorts(const clap_plugin_t * plugIn) noexcept
{
    const auto NotePorts = (const clap_plugin_note_ports_t *)(plugIn->get_extension(plugIn, CLAP_EXT_NOTE_PORTS));

    if (NotePorts == nullptr)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without Note Ports extension.");

        return false;
    }

    constexpr auto InputPort = true;

    const auto InputPortCount = NotePorts->count(plugIn, InputPort);

    if (InputPortCount != 1)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 1 MIDI input port.");

        return false;
    }
/*
    constexpr auto OutputPort = false;

    const auto OutputPortCount = NotePorts->count(plugIn, OutputPort);

    if (OutputPortCount != 0)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with MIDI output ports.");

        return false;
    }
*/
    clap_note_port_info PortInfo = {};

    const auto PortIndex = 0;

    NotePorts->get(plugIn, PortIndex, InputPort, &PortInfo);

    if ((PortInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI) == 0)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without MIDI dialect support.");

        return false;
    }

    return true;
}

/// <summary>
/// Verifies the audio ports.
/// </summary>
bool Host::VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept
{
    const auto AudioPorts = static_cast<const clap_plugin_audio_ports_t *>(plugIn->get_extension(plugIn, CLAP_EXT_AUDIO_PORTS));

    if (AudioPorts == nullptr)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without Audio Ports extension.");

        return false;
    }

    constexpr auto InputPort = true;

    const auto InputPortCount = AudioPorts->count(plugIn, InputPort);

    if (InputPortCount != 0)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with audio input ports.");

        return false;
    }

    constexpr auto OutputPort = false;

    const auto OutputPortCount = AudioPorts->count(plugIn, OutputPort);

    if (OutputPortCount != 1)
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 1 audio output port.");

        return false;
    }

    clap_audio_port_info PortInfo = {};

    const auto PortIndex = 0;

    AudioPorts->get(plugIn, PortIndex, OutputPort, &PortInfo);

    if (!(PortInfo.channel_count == 2 && ::strcmp(PortInfo.port_type, CLAP_PORT_STEREO) == 0))
    {
        console::error(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 2 output channels in stereo.");

        return false;
    }

    return true;
}

/// <summary>
/// Returns true if the specified plug-in has a GUI.
/// </summary>
bool Host::HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept
{
    const auto * GUI = (const clap_plugin_gui_t *) plugIn->get_extension(plugIn, CLAP_EXT_GUI);

    const bool Result = (GUI != nullptr) && GUI->is_api_supported(plugIn, "win32", isFloatingGUI);

    return Result;
}

/// <summary>
/// Implements the CLAP_EXT_LOG extension.
/// </summary>
const clap_host_log Host::LogHandler
{
    // Logs a message from the plug-in.
    .log =  [](const clap_host_t * self, clap_log_severity severity, const char * message)
    {
    #ifndef _DEBUG
        if (severity < CLAP_LOG_WARNING)
            return;
    #endif

        const char * Severities[] =
        {
            "Debug", "Info", "Warning", "Error", "Fatal", "Host Misbehaving", "Plug-in misbehaving",
        };

        const char * Severity = ((size_t) severity < _countof(Severities)) ? Severities[severity] : "Unknown";

        console::print(STR_COMPONENT_BASENAME, " received message from CLAP plug-in: ", message, " (", Severity, ")");
    }
};

/// <summary>
/// Implements the CLAP_EXT_NOTE_PORTS extension.
/// </summary>
const clap_host_note_ports_t Host::NotePortsHandler =
{
    // Queries which dialects the host supports.
    .supported_dialects = [](const clap_host_t * self) -> uint32_t
    {
        return (uint32_t) 0; // None
    },

    // Rescans the full list of note ports according to the flags.
    .rescan = [](const clap_host_t * self, uint32_t flags)
    {
    }
};

/// <summary>
/// Implements the CLAP_EXT_STATE extension. The plug-in reports its state has changed and should be saved again. If a parameter value changes, then it is implicit that the state is dirty.
/// </summary>
const clap_host_state Host::StateHandler
{
    .mark_dirty = [](const clap_host_t * self)
    {
    }
};

Host _Host;
}
