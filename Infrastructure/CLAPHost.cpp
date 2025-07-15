
/** $VER: CLAPHost.cpp (2025.07.13) P. Stuer **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "Support.h"
#include "Log.h"

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
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " CLAP Host received request from plug-in to restart.");
    };

    // Handles a request to activate and start processing the plug-in.
    request_process  =  [](const clap_host * self)
    {
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " CLAP Host received request to activate plug-in.");
    };

    // Handles a request to schedule a call to plugin->on_main_thread(plugin) on the main thread.
    request_callback = [](const clap_host * self)
    {
        auto * This = (Host *) self;

        This->_PlugIn->on_main_thread(This->_PlugIn);
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

    #define CLAP_SDK_VERSION TOSTRING(CLAP_VERSION_MAJOR) "." TOSTRING(CLAP_VERSION_MINOR) "." TOSTRING(CLAP_VERSION_REVISION)

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is built with CLAP " CLAP_SDK_VERSION ".");

    GetPlugIns_(directoryPath);

    std::sort(_PlugIns.begin(), _PlugIns.end(), [](PlugIn a, PlugIn b) { return a.Name < b.Name; });

    return _PlugIns;
}

/// <summary>
/// Loads a plug-in file and creates the plug-ing with the specified index.
/// </summary>
bool Host::Load(const fs::path & filePath, uint32_t index) noexcept
{
    if ((_FilePath == filePath) && (_Index == index) && IsPlugInLoaded())
        return true; // Already loaded

    UnLoad();

    _hPlugIn = ::LoadLibraryW(::UTF8ToWide((const char *) filePath.u8string().c_str()).c_str());

    if (_hPlugIn == NULL)
        return false;

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(_hPlugIn, "clap_entry");

    if ((Entry != nullptr) && (Entry->init != nullptr) && Entry->init((const char *) filePath.u8string().c_str()))
    {
        const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

        if ((Factory != nullptr) && (Factory->get_plugin_descriptor != nullptr))
        {
            _PlugInDescriptor = Factory->get_plugin_descriptor(Factory, index);

            if (_PlugInDescriptor != nullptr)
            {
                _PlugIn = Factory->create_plugin(Factory, this, _PlugInDescriptor->id);

                if (IsPlugInLoaded())
                    return true;
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
        // FIXME: Ugly hack for Odin2 plug-in
        if (::strcmp(_PlugInDescriptor->id, "com.thewavewarden.odin2") != 0)
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

    if (!_PlugIn->init(_PlugIn))
        return false;

    GetGUI(); // Must be called after init().

    if (!_PlugIn->activate(_PlugIn, sampleRate, minFrames, maxFrames))
        return false;

    GetVoiceInfo(); // Must be called after activate().

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
    if (!::IsWindow(hWnd))
        return;

    GetGUI();

    if (!HasGUI())
        return;

// FIXME: GUI support not implemented yet
/*
    if (!_Window.IsWindow())
    {
        _Window.Run(this);
    }
    else
        _Window.BringWindowToTop();
*/
}

/// <summary>
/// Hides the GUI of the hosted plug-in.
/// </summary>
void Host::HideGUI() noexcept
{
    if (!HasGUI() || !_Window.IsWindow())
        return;

// FIXME: GUI support not implemented yet
/*
    _PlugInGUI->set_parent(_PlugIn, nullptr);

    _Window.DestroyWindow();
*/
}

/// <summary>
/// Returns true if the plug-in GUI is shown.
/// </summary>
bool Host::IsGUIVisible() const noexcept
{
// FIXME: GUI support not implemented yet
    return false; // _Window.IsWindow() && _Window.IsWindowVisible();
}

#pragma region Private

/// <summary>
/// Gets the CLAP plug-ins.
/// </summary>
void Host::GetPlugIns_(const fs::path & directoryPath) noexcept
{
    try
    {
        for (const auto & Entry : fs::directory_iterator(directoryPath))
        {
            if (Entry.is_directory())
            {
                GetPlugIns_(Entry.path());
            }
            else
            if (IsOneOf(Entry.path().extension().u8string(), { u8".clap", u8".dll" }))
            {
                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is examining \"%s\"...", (const char *) Entry.path().u8string().c_str());

                GetPlugInEntries(Entry.path(), [this, Entry](const std::string & plugInName, uint32_t index, bool hasGUI)
                {
                    PlugIn PlugIn =
                    {
                        .Name     = plugInName,
                        .Index    = index,
                        .FilePath = Entry.path(),
                        .HasGUI   = hasGUI
                    };

                    _PlugIns.push_back(PlugIn);
                });
            }
        }
    }
    catch (std::exception e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME, " fails to get CLAP plug-ins: %s", e.what());
    }
}

/// <summary>
/// Gets the CLAP plug-ins in plug-in file.
/// </summary>
void Host::GetPlugInEntries(const fs::path & filePath, const std::function<void(const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept
{
    HMODULE hPlugIn = ::LoadLibraryW(::UTF8ToWide((const char *) filePath.u8string().c_str()).c_str());

    if (hPlugIn == NULL)
        return;

    auto PlugInEntry = (const clap_plugin_entry_t *) ::GetProcAddress(hPlugIn, "clap_entry");

    try
    {
        if ((PlugInEntry != nullptr) && (PlugInEntry->init != nullptr) && PlugInEntry->init((const char *) filePath.u8string().c_str()))
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
                        #define SafeString(x) ((x) != nullptr ? (x) : "")

                        Log.AtInfo().Write
                        (
                            "Id: \"%s\", Name: \"%s\", Version: \"%s\", Description: \"%s\", Vendor: \"%s\", URL: \"%s\", Manual URL: \"%s\", Support URL: \"%s\", CLAP Version: %d.%d.%d",
                            SafeString(Descriptor->id), SafeString(Descriptor->name), SafeString(Descriptor->version), SafeString(Descriptor->description),
                            SafeString(Descriptor->vendor), SafeString(Descriptor->url), SafeString(Descriptor->manual_url), SafeString(Descriptor->support_url),
                            Descriptor->clap_version.major, Descriptor->clap_version.minor, Descriptor->clap_version.revision
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
                            Log.AtInfo().Write(STR_COMPONENT_BASENAME,
                                " CLAP host CLAP version %d.%d.%d is not compatible with plug-in CLAP version %d.%d.%d.",
                                CLAP_VERSION.major, CLAP_VERSION.minor, CLAP_VERSION.revision,
                                Descriptor->clap_version.major, Descriptor->clap_version.minor, Descriptor->clap_version.revision);
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
    // Odin2 has no get_extension method.
    if (plugIn->get_extension == nullptr)
        return true;

    const auto NotePorts = (const clap_plugin_note_ports_t *)(plugIn->get_extension(plugIn, CLAP_EXT_NOTE_PORTS));

    if (NotePorts == nullptr)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without Note Ports extension.");

        return false;
    }

    constexpr auto InputPort = true;

    const auto InputPortCount = NotePorts->count(plugIn, InputPort);

    if (InputPortCount != 1)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 1 MIDI input port.");

        return false;
    }
/*
    constexpr auto OutputPort = false;

    const auto OutputPortCount = NotePorts->count(plugIn, OutputPort);

    if (OutputPortCount != 0)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with MIDI output ports.");

        return false;
    }
*/
    clap_note_port_info PortInfo = {};

    const auto PortIndex = 0;

    NotePorts->get(plugIn, PortIndex, InputPort, &PortInfo);

    if ((PortInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI) == 0)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without MIDI dialect support.");

        return false;
    }

    return true;
}

/// <summary>
/// Verifies the audio ports.
/// </summary>
bool Host::VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept
{
    // Odin2 has no get_extension method.
    if (plugIn->get_extension == nullptr)
        return true;

    const auto AudioPorts = static_cast<const clap_plugin_audio_ports_t *>(plugIn->get_extension(plugIn, CLAP_EXT_AUDIO_PORTS));

    if (AudioPorts == nullptr)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without Audio Ports extension.");

        return false;
    }
/*
    constexpr auto InputPort = true;

    const auto InputPortCount = AudioPorts->count(plugIn, InputPort);

    if (InputPortCount != 0)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with audio input ports.");

        return false;
    }
*/
    constexpr auto OutputPort = false;
/*
    const auto OutputPortCount = AudioPorts->count(plugIn, OutputPort);

    if (OutputPortCount != 1)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 1 audio output port.");

        return false;
    }
*/
    const auto PortIndex = 0;

    AudioPorts->get(plugIn, PortIndex, OutputPort, &_PortInfo);

    if (!(_PortInfo.channel_count == 2 && ::strcmp(_PortInfo.port_type, CLAP_PORT_STEREO) == 0))
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 2 output channels in stereo.");

        return false;
    }

    return true;
}

/// <summary>
/// Returns true if the hosted plug-in prefers 64-bit output.
/// </summary>
bool Host::PlugInPrefers64bitAudio() const noexcept
{
    // Odin2 has no get_extension() method.
    if ((_PlugIn == nullptr) || (_PlugIn->get_extension == nullptr))
        return true;

    const auto AudioPorts = static_cast<const clap_plugin_audio_ports_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_AUDIO_PORTS));

    if (AudioPorts == nullptr)
        return false;

    constexpr auto OutputPort = false;

    const auto PortIndex = 0;
    clap_audio_port_info PortInfo = { };

    AudioPorts->get(_PlugIn, PortIndex, OutputPort, &PortInfo);

    return ((PortInfo.flags & CLAP_AUDIO_PORT_PREFERS_64BITS) == CLAP_AUDIO_PORT_PREFERS_64BITS);
}

/// <summary>
/// Returns true if the specified plug-in has a GUI.
/// </summary>
bool Host::HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept
{
    // Odin2 has no get_extension method.
    if (plugIn->get_extension == nullptr)
        return false;

    const auto * GUI = (const clap_plugin_gui_t *) plugIn->get_extension(plugIn, CLAP_EXT_GUI);

    const bool Result = (GUI != nullptr) && GUI->is_api_supported(plugIn, "win32", isFloatingGUI);

    return Result;
}

/// <summary>
/// Implements the host's CLAP_EXT_LOG extension.
/// </summary>
const clap_host_log Host::LogHandler
{
    // Logs a message from the plug-in.
    .log =  [](const clap_host_t * self, clap_log_severity severity, const char * message)
    {
        switch (severity)
        {
            case CLAP_LOG_DEBUG:    { Log.AtDebug().Write(STR_COMPONENT_BASENAME " CLAP plug-in Debug: %s",   message); break; }
            case CLAP_LOG_INFO:     { Log.AtInfo() .Write(STR_COMPONENT_BASENAME " CLAP plug-in Info: %s",    message); break; }
            case CLAP_LOG_WARNING:  { Log.AtWarn() .Write(STR_COMPONENT_BASENAME " CLAP plug-in Warning: %s", message); break; }
            case CLAP_LOG_ERROR:    { Log.AtError().Write(STR_COMPONENT_BASENAME " CLAP plug-in Error: %s",   message); break; }
            case CLAP_LOG_FATAL:    { Log.AtFatal().Write(STR_COMPONENT_BASENAME " CLAP plug-in Fatal: %s",   message); break; }

            case CLAP_LOG_PLUGIN_MISBEHAVING:
            {
                Log.AtWarn() .Write(STR_COMPONENT_BASENAME " noticed CLAP plug-in is misbehaving: ", message); break;
            }
        }
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_NOTE_PORTS extension.
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
/// Implements the host's CLAP_EXT_STATE extension. The plug-in reports its state has changed and should be saved again. If a parameter value changes, then it is implicit that the state is dirty.
/// </summary>
const clap_host_state Host::StateHandler
{
    .mark_dirty = [](const clap_host_t * self)
    {
    }
};

/// <summary>
/// Gets the plug-in voice info extension.
/// </summary>
void Host::GetVoiceInfo() noexcept
{
    if ((_PlugIn == nullptr) || (_PlugIn->get_extension == nullptr)) // Odin2 has no get_extension method.
        return;

    auto _PlugInVoiceInfo = (const clap_plugin_voice_info *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_VOICE_INFO);

    if (_PlugInVoiceInfo == nullptr)
        return;

    clap_voice_info VoiceInfo = { };

    if (!_PlugInVoiceInfo->get(_PlugIn, &VoiceInfo))
        return;

    // TODO: Process voice info information.
}

/// <summary>
/// Gets the plug-in GUI extension.
/// </summary>
void Host::GetGUI() noexcept
{
    // Odin2 has no get_extension method.
    if (_PlugIn->get_extension == nullptr)
        return;

    _PlugInGUI = (const clap_plugin_gui_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_GUI);

    if (_PlugInGUI == nullptr)
        return;

    if (!_PlugInGUI->is_api_supported(_PlugIn, "win32", false))
        return;

    if (!_PlugInGUI->create(_PlugIn, "win32", false))
        return;
}

/// <summary>
/// Gets the preferred size of the GUI window.
/// </summary>
void Host::GetGUISize(RECT & wr) const noexcept
{
    if (!HasGUI())
        return;

    uint32_t Width = 0, Height = 0;

    if (!_PlugInGUI->get_size(_PlugIn, &Width, &Height))
    {
        Width  = 320;
        Height = 200;
    }

    wr.right  = (long) Width;
    wr.bottom = (long) Height;
}

#pragma endregion

Host _Host;
}
