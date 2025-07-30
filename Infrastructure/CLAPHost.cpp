
/** $VER: CLAPHost.cpp (2025.07.30) P. Stuer **/

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
    clap_version  = CLAP_VERSION;
    host_data     = nullptr;

    name          = STR_COMPONENT_BASENAME;
    vendor        = STR_COMPONENT_COMPANY_NAME;
    url           = STR_COMPONENT_URL;
    version       = STR_COMPONENT_VERSION;

    get_extension = [](const clap_host * self, const char * extensionId) -> const void *
    {
        if (::strcmp(extensionId, CLAP_EXT_AUDIO_PORTS) == 0)
            return &GetAudioPortsExtension;

        if (::strcmp(extensionId, CLAP_EXT_AUDIO_PORTS_CONFIG) == 0)
            return &GetAudioPortsConfigExtension;

        if ((::strcmp(extensionId, CLAP_EXT_CONTEXT_MENU) == 0) || (::strcmp(extensionId, CLAP_EXT_CONTEXT_MENU_COMPAT) == 0))
            return &GetContextMenuExtension;

        if (::strcmp(extensionId, CLAP_EXT_LOG) == 0)
            return &GetLogExtension;

        if (::strcmp(extensionId, CLAP_EXT_STATE) == 0)
            return &GetStateExtension;

        if (::strcmp(extensionId, CLAP_EXT_NOTE_PORTS) == 0)
            return &GetNotePortsExtension;

        if (::strcmp(extensionId, CLAP_EXT_THREAD_CHECK) == 0)
            return &GetThreadCheckExtension;

        if (::strcmp(extensionId, CLAP_EXT_TIMER_SUPPORT) == 0)
            return &GetTimerSupportExtension;

        Log.AtDebug().Write(STR_COMPONENT_BASENAME " CLAP plug-in requests unsupported extension \"%s\".", extensionId);

        return nullptr;
    };

    // Handles a request to deactivate and reactivate the plug-in.
    request_restart = [](const clap_host * self)
    {
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " CLAP Host received request from plug-in to restart.");
    };

    // Handles a request to activate and start processing the plug-in.
    request_process = [](const clap_host * self)
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
/// Loads a plug-in file and creates the plug-in with the specified index.
/// </summary>
bool Host::Load(const fs::path & filePath, uint32_t index)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("Host::Load() must be called from the main thread");

    if (_FilePath.empty() || (index == (uint32_t) -1))
        return false;

    if ((_FilePath == filePath) && (_Index == index) && (_PlugIn != nullptr))
        return true; // Already loaded

    _FilePath = filePath;
    _Index    = index;

    bool Result = false;

    Unload();

    _hPlugIn = ::LoadLibraryW(::UTF8ToWide((const char *) _FilePath.u8string().c_str()).c_str());

    if (_hPlugIn == NULL)
        return false;

    auto Entry = (const clap_plugin_entry_t *) ::GetProcAddress(_hPlugIn, "clap_entry");

    if (Entry == nullptr)
        return false;

    if (Entry->init == nullptr)
        return false;

    if (!Entry->init((const char *) _FilePath.u8string().c_str()))
        return false;

    const auto * Factory = (const clap_plugin_factory_t *) Entry->get_factory(CLAP_PLUGIN_FACTORY_ID);

    if (Factory == nullptr)
        return false;

    if (Factory->get_plugin_descriptor == nullptr)
        return false;

    _PlugInDescriptor = Factory->get_plugin_descriptor(Factory, _Index);

    if (_PlugInDescriptor == nullptr)
        return false;

    _PlugIn = Factory->create_plugin(Factory, this, _PlugInDescriptor->id);

    if (_PlugIn == nullptr)
        return false;

    Result = _PlugIn->init(_PlugIn);

    return true;
}

/// <summary>
/// Unloads the currently hosted plug-in.
/// </summary>
void Host::Unload()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("Host::Unload() must be called from the main thread");

    if (_PlugInGUI)
    {
        HideGUI();

        _PlugInGUI->destroy(_PlugIn);
        _PlugInGUI = nullptr;
    }

    if (_PlugIn)
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
/// Activates the loaded plug-in.
/// </summary>
bool Host::ActivatePlugIn(double sampleRate)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("Host::ActivatePlugIn() must be called from the main thread");

    if (_PlugIn == nullptr)
        return false;

    if (!_PlugIn->activate(_PlugIn, sampleRate, 1, CLAP::BlockSize))
        return false;

    GetVoiceInfo(); // Must be called after activate().

    return true;
}

/// <summary>
/// Deactivates the loaded plug-in.
/// </summary>
void Host::DeactivatePlugIn() const
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("Host::DectivatePlugIn() must be called from the main thread");

    if (_PlugIn == nullptr)
        return;

    _PlugIn->deactivate(_PlugIn);
}

/// <summary>
/// Notifies the plug-in we're about to start processing.
/// </summary>
bool Host::StartProcessing() noexcept
{
    if (_PlugIn == nullptr)
        return false;

    _IsProcessing = true;

    return _PlugIn->start_processing(_PlugIn);
}

/// <summary>
/// Notifies the plug-in we're about to stop processing.
/// </summary>
void Host::StopProcessing() noexcept
{
    if (_PlugIn == nullptr)
        return;

    _PlugIn->stop_processing(_PlugIn);

    _IsProcessing = false;
}

/// <summary>
/// Processes events.
/// </summary>
bool Host::Process(const clap_process_t & processor) noexcept
{
    return (_PlugIn->process(_PlugIn, &processor) == CLAP_PROCESS_ERROR);
}

/// <summary>
/// Opens the editor of the hosted plug-in.
/// </summary>
void Host::OpenEditor(HWND hWnd, bool isFloating) noexcept
{
    if (!::IsWindow(hWnd))
        return;

    InitializeGUI(isFloating);

    if (_PlugInGUI == nullptr)
        return;

    Window::Parameters p =
    {
    };

    p._Host = this;

    _Window.DoModal(hWnd, (LPARAM) &p);
}

/// <summary>
/// Shows the GUI to the host window.
/// </summary>
void Host::ShowGUI(HWND hWnd) noexcept
{
    if (_PlugInGUI == nullptr)
        return;

    clap_window_t w = { .api = "win32", .win32 = hWnd };

    if (!_PlugInGUI->set_parent(_PlugIn, &w))
        return;

    _PlugInGUI->show(_PlugIn);
}

/// <summary>
/// Hides the GUI from the host window.
/// </summary>
void Host::HideGUI() noexcept
{
    if ((_PlugInGUI == nullptr) || !_Window.IsWindow())
        return;

    _PlugInGUI->hide(_PlugIn);
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
            if (IsOneOf(Entry.path().extension().string().c_str(), { ".clap", ".dll" }))
            {
                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is examining \"%s\"...", (const char *) Entry.path().u8string().c_str());

                GetPlugInEntries(Entry.path(), [this, Entry](const std::string & plugInName, uint32_t index, bool hasGUI)
                {
                    PlugIn PlugIn =
                    {
                        .Name      = plugInName,
                        .Index     = index,
                        .FilePath  = Entry.path(),
                        .HasEditor = hasGUI
                    };

                    _PlugIns.push_back(PlugIn);
                });
            }
        }
    }
    catch (const std::exception & e)
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
                                        callback(SafeString(Descriptor->name), i, HasGUI(PlugIn, false));
                                }
                                else
                                    Log.AtError().Write(STR_COMPONENT_BASENAME " failed to initialize plug-in.");

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
    if (plugIn->get_extension == nullptr)
        return true;

    const auto NotePorts = (const clap_plugin_note_ports_t *)(plugIn->get_extension(plugIn, CLAP_EXT_NOTE_PORTS));

    if (NotePorts == nullptr)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without the Note Ports extension.");

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
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with note ports without MIDI dialect support.");

        return false;
    }

    return true;
}

/// <summary>
/// Verifies the audio ports.
/// </summary>
bool Host::VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept
{
    if (plugIn->get_extension == nullptr)
        return true;

    const auto AudioPorts = (const clap_plugin_audio_ports_t *) plugIn->get_extension(plugIn, CLAP_EXT_AUDIO_PORTS);

    if (AudioPorts == nullptr)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without the Audio Ports extension.");

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

    AudioPorts->get(plugIn, PortIndex, OutputPort, &_OutPortInfo);

    if (!(_OutPortInfo.channel_count == 2 && ::strcmp(_OutPortInfo.port_type, CLAP_PORT_STEREO) == 0))
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 2 output channels in stereo.");

        return false;
    }

    return true;
}

/// <summary>
/// Gets the plug-in voice info extension.
/// </summary>
void Host::GetVoiceInfo() noexcept
{
    if ((_PlugIn == nullptr) || (_PlugIn->get_extension == nullptr))
        return;

    const auto _PlugInVoiceInfo = (const clap_plugin_voice_info *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_VOICE_INFO);

    if (_PlugInVoiceInfo == nullptr)
        return;

    clap_voice_info VoiceInfo = { };

    if (!_PlugInVoiceInfo->get(_PlugIn, &VoiceInfo))
        return;

    // TODO: Process voice info information.
}

/// <summary>
/// Returns true if the specified plug-in has a GUI.
/// </summary>
bool Host::HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept
{
    // Must be called after clap_plugin_t::init().
    if (plugIn->get_extension == nullptr)
        return false;

    const auto * GUI = (const clap_plugin_gui_t *) plugIn->get_extension(plugIn, CLAP_EXT_GUI);

    if (GUI == nullptr)
        return false;

    if (!GUI->is_api_supported(plugIn, "win32", isFloatingGUI))
        return false;

    return true;
}

/// <summary>
/// Gets the plug-in GUI extension.
/// </summary>
void Host::InitializeGUI(bool isFloating) noexcept
{
    _PlugIn->init(_PlugIn);

    if (_PlugIn->get_extension == nullptr)
        return;

    _PlugInGUI = (const clap_plugin_gui_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_GUI);

    if (_PlugInGUI == nullptr)
        return;

    if (!_PlugInGUI->is_api_supported(_PlugIn, "win32", isFloating))
        return;

    if (!_PlugInGUI->create(_PlugIn, "win32", isFloating))
        return;
}

/// <summary>
/// Gets the preferred size of the GUI window.
/// </summary>
void Host::GetPreferredGUISize(RECT & wr) const noexcept
{
    if (_PlugInGUI == nullptr)
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

/// <summary>
/// Implements the host's CLAP_EXT_AUDIO_PORTS extension.
/// </summary>
const clap_host_audio_ports_t Host::GetAudioPortsExtension =
{
   // Checks if the host allows a plugin to change a given aspect of the audio ports definition.
   .is_rescan_flag_supported = [](const clap_host_t * self, uint32_t flag) -> bool
    {
        // TODO
        return false;
    },

   // Rescan the full list of audio ports according to the flags. It is illegal to ask the host to rescan with a flag that is not supported. Certain flags require the plugin to be de-activated.
   .rescan = [](const clap_host_t * self, uint32_t flags)
    {
        // TODO
    },
};

/// <summary>
/// Implements the host's CLAP_AUDIO_PORTS_CONFIG extension.
/// Plugins with very complex configuration possibilities should let the user configure the ports from the plugin GUI, and call clap_host_audio_ports.rescan(CLAP_AUDIO_PORTS_RESCAN_ALL).
/// </summary>
const clap_host_audio_ports_config Host::GetAudioPortsConfigExtension =
{
    // Rescan the full list of configs.
    .rescan = [](const clap_host_t * self)
    {
        // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_CONTEXT_MENU extension.
/// This extension lets the host and plugin exchange menu items and let the plugin ask the host to show its context menu.
/// </summary>
const clap_host_context_menu Host::GetContextMenuExtension =
{
    // Insert host's menu items into the menu builder. If target is null, assume global context. Returns true on success.
    .populate = [](const clap_host_t * self, const clap_context_menu_target_t * target, const clap_context_menu_builder_t * builder) -> bool
    {
        return false;
    },

    // Performs the given action, which was previously provided to the plugin via populate(). If target is null, assume global context. Returns true on success.
    .perform = [](const clap_host_t * self, const clap_context_menu_target_t * target, clap_id action_id) -> bool
    {
        return false;
    },

    // Returns true if the host can display a popup menu for the plugin. This may depend upon the current windowing system used to display the plugin, so the return value is invalidated after creating the plugin window.
    .can_popup = [](const clap_host_t * self) -> bool
    {
        return false;
    },

    // Shows the host popup menu for a given parameter.
    // If the plugin is using embedded GUI, then x and y are relative to the plugin's window, otherwise they're absolute coordinate, and screen index might be set accordingly.
    // If target is null, assume global context. Returns true on success.
    .popup = [](const clap_host_t * host, const clap_context_menu_target_t * target, int32_t screen_index, int32_t x, int32_t y) -> bool
    {
        return false;
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_LOG extension.
/// </summary>
const clap_host_log Host::GetLogExtension
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
const clap_host_note_ports_t Host::GetNotePortsExtension =
{
    // Queries which dialects the host supports.
    .supported_dialects = [](const clap_host_t * self) -> uint32_t
    {
        return CLAP_NOTE_DIALECT_CLAP | CLAP_NOTE_DIALECT_MIDI | CLAP_NOTE_DIALECT_MIDI_MPE | CLAP_NOTE_DIALECT_MIDI2;
    },

    // Rescans the full list of note ports according to the flags.
    .rescan = [](const clap_host_t * self, uint32_t flags)
    {
        // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_STATE extension. The plug-in reports its state has changed and should be saved again. If a parameter value changes, then it is implicit that the state is dirty.
/// </summary>
const clap_host_state Host::GetStateExtension
{
    .mark_dirty = [](const clap_host_t * self)
    {
    }
};

/// <summary>
/// Implements the host's CLAP_THREAD_CHECK extension. This interface is useful to do runtime checks and make sure that the functions are called on the correct threads. It is highly recommended that hosts implement this extension.

/// </summary>
const clap_host_thread_check Host::GetThreadCheckExtension
{
    // Returns true if "this" thread is the main thread.
    .is_main_thread = [](const clap_host_t * self) -> bool
    {
        return core_api::is_main_thread();
    },

    // Returns true if "this" thread is one of the audio threads.
   .is_audio_thread = [](const clap_host_t * self) -> bool
    {
        return !core_api::is_main_thread();
    }
};

const clap_host_timer_support Host::GetTimerSupportExtension
{
    // Registers a periodic timer. The host may adjust the period if it is under a certain threshold. 30 Hz should be allowed. Returns true on success.
    .register_timer = [](const clap_host_t * self, uint32_t period_ms, clap_id * timerId) -> bool
    {
        return false;
    },

    // Returns true on success.
    .unregister_timer = [](const clap_host_t * self, clap_id timerId) -> bool
    {
        return false;
    }
};

#pragma endregion
}

CLAP::Host _CLAPHost;
