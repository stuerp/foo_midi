
/** $VER: CLAPHost.cpp (2025.06.29) P. Stuer **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "CLAPHost.h"
#include "CLAPPlayer.h"

namespace CLAP
{

Host::Host() noexcept :_hPlugIn(), _PlugInDescriptor(), _PlugIn(), _PlugInGUI()
{
    clap_version     = CLAP_VERSION;
    host_data        = nullptr;

    name             = STR_COMPONENT_BASENAME;
    vendor           = STR_COMPONENT_COMPANY_NAME;
    url              = STR_COMPONENT_URL;
    version          = STR_COMPONENT_VERSION;

    get_extension    = [](const clap_host * self, const char * extensionId) -> const void *
    {
        if (::strcmp(extensionId, CLAP_EXT_LOG) == 0)
            return &LogHandler;

        if (::strcmp(extensionId, CLAP_EXT_STATE) == 0)
            return &StateHandler;

        if (::strcmp(extensionId, CLAP_EXT_NOTE_PORTS) == 0)
            return &NotePortsHandler;

/* Queried by Dexed 
    clap.thread-check
    clap.thread-pool
    clap.audio-ports
    clap.audio-ports-config
    clap.timer-support
    clap.posix-fd-support
    clap.latency
    clap.gui
    clap.params
    clap.note-name
    clap.voice-info

    clap.resource-directory.draft/0
    clap.track-info.draft/1
    clap.context-menu.draft/0
    clap.preset-load.draft/2
    clap.remote-controls.draft/2
*/
        return nullptr;
    };

    // Handles a request to deactivate and reactivate the plug-in.
    request_restart  = [](const clap_host * self)
    {
    };

    // Handles a request to activate and start processing the plug-in.
    request_process  =  [](const clap_host * self)
    {
    };

    // Handles a request to schedule a call to plugin->on_main_thread(plugin) on the main thread.
    request_callback = [](const clap_host * self)
    {
    };
}

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
/// Loads a plug-in file and creates the plug-ing with the specified index.
/// </summary>
bool Host::Load(const fs::path & filePath, uint32_t index) noexcept
{
    UnLoad();

    _hPlugIn = ::LoadLibraryA(filePath.string().c_str());

    if (_hPlugIn == NULL)
        return false;

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(_hPlugIn, "clap_entry");

    if ((Entry != nullptr) && (Entry->init != nullptr) && Entry->init(filePath.string().c_str()))
    {
        const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if (Factory != nullptr)
        {
            _PlugInDescriptor = Factory->get_plugin_descriptor(Factory, index);

            if (_PlugInDescriptor != nullptr)
            {
                _PlugIn = Factory->create_plugin(Factory, this, _PlugInDescriptor->id);

                if (IsPlugInLoaded())
                {
                    return GetGUI();
                }
            }
        }
    }

    return false;
}

/// <summary>
/// Unloads the currently hosted plug-in.
/// </summary>
void Host::UnLoad() noexcept
{
    if (HasGUI())
    {
        HideGUI();

        _PlugInGUI->destroy(_PlugIn);
        _PlugInGUI = nullptr;
    }

    if (IsPlugInLoaded())
    {
        _PlugIn->destroy(_PlugIn);
        _PlugIn = nullptr;
    }

    _PlugInDescriptor = nullptr;

    if (_hPlugIn != NULL)
    {
        ::FreeLibrary(_hPlugIn);
        _hPlugIn = NULL;
    }
}

/// <summary>
/// Returns true if the host has loaded a plug-in.
/// </summary>
bool Host::IsPlugInLoaded() const noexcept
{
    return (_PlugIn != nullptr);
}

/// <summary>
/// Activates the loaded plug-in.
/// </summary>
bool Host::ActivatePlugIn(double  sampleRate, uint32_t minFrames, uint32_t maxFrames) noexcept
{
    if (!IsPlugInLoaded())
        return false;

    #define CLAP_SDK_VERSION TOSTRING(CLAP_VERSION_MAJOR) "." TOSTRING(CLAP_VERSION_MINOR) "." TOSTRING(CLAP_VERSION_REVISION)

    console::print(STR_COMPONENT_BASENAME " is using CLAP ", CLAP_SDK_VERSION ".");
    console::print("CLAP plug-in ", _PlugInDescriptor->name, _PlugInDescriptor->version, " is using CLAP ", _PlugInDescriptor->clap_version.major, ".", _PlugInDescriptor->clap_version.minor, ".", _PlugInDescriptor->clap_version.revision, ".");

    if (!_PlugIn->init(_PlugIn))
        return false;

    if (!_PlugIn->activate(_PlugIn, sampleRate, minFrames, maxFrames))
        return false;

    return _PlugIn->start_processing(_PlugIn);
}

/// <summary>
/// Deactivates the loaded plug-in.
/// </summary>
void Host::DeactivatePlugIn() const noexcept
{
    if (!IsPlugInLoaded())
        return;

    _PlugIn->stop_processing(_PlugIn);

    _PlugIn->deactivate(_PlugIn);

}

/// <summary>
/// Processes events.
/// </summary>
bool Host::Process(const clap_process_t & processor) noexcept
{
    return (_PlugIn->process(_PlugIn, &processor) == CLAP_PROCESS_ERROR);
}

/// <summary>
/// Returns true if the host has loaded a plug-in that has a GUI.
/// </summary>
bool Host::HasGUI() const noexcept
{
    return (_PlugInGUI != nullptr);
}

/// <summary>
/// Shows the GUI of the hosted plug-in.
/// </summary>
void Host::ShowGUI(HWND hWnd) noexcept
{
    if (!HasGUI() || (hWnd == NULL))
        return;

    if (!_Window.IsWindow())
    {
        CLAP::Window::Parameters dp =
        {
            ._Bounds = { },
            ._FilePath = _FilePath,
            ._Index = _Index,
            ._GUIBounds = { },
        };

        GetGUISize(_PlugInGUI, dp._GUIBounds);

        if (_Window.Create(hWnd, (LPARAM) &dp) != NULL)
        {
            clap_window_t Window = { .api = "win32", .win32 = _Window };

            if (_PlugInGUI->set_parent(_PlugIn, &Window))
            {
                _Window.ShowWindow(SW_SHOW);
            }
        }
    }
    else
        _Window.BringWindowToTop();
}

/// <summary>
/// Hides the GUI of the hosted plug-in.
/// </summary>
void Host::HideGUI() noexcept
{
    if (!_Window.IsWindow())
        return;

    _Window.DestroyWindow();
}

/// <summary>
/// Returns true if the plug-in GUI is shown.
/// </summary>
bool Host::IsGUIVisible() const noexcept
{
    return _Window.IsWindow() && _Window.IsWindowVisible();
}

#pragma region Private

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

/// <summary>
/// Gets the GUI extension.
/// </summary>
bool Host::GetGUI() noexcept
{
    _PlugInGUI = (const clap_plugin_gui_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_GUI);

    if (_PlugInGUI == nullptr)
        return false;

    if (!_PlugInGUI->is_api_supported(_PlugIn, "win32", false))
        return false;

    if (!_PlugInGUI->create(_PlugIn, "win32", false))
        return false;

    return true;
}

/// <summary>
/// Gets the preferred size of the GUI window.
/// </summary>
void Host::GetGUISize(const clap_plugin_gui_t * gui, RECT & wr) const noexcept
{
    uint32_t Width = 0, Height = 0;

    if (!gui->get_size(_PlugIn, &Width, &Height))
    {
        Width  = 320;
        Height = 200;
    }

    wr.right  = (long) Width;
    wr.bottom = (long) Height;
}

#pragma endregion
}
