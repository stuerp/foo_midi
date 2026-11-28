
/** $VER: CLAPHost.cpp (2025.10.07) P. Stuer - Implements a CLAP host **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "Support.h"
#include "Log.h"

#include "CLAPHost.h"
namespace CLAP
{
Host::Host() noexcept
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

        if (::strcmp(extensionId, CLAP_EXT_GUI) == 0)
            return &GetGUIExtension;

        if (::strcmp(extensionId, CLAP_EXT_LATENCY) == 0)
            return &GetLatencyExtension;

        if (::strcmp(extensionId, CLAP_EXT_LOG) == 0)
            return &GetLogExtension;

        if (::strcmp(extensionId, CLAP_EXT_NOTE_NAME) == 0)
            return &GetNoteNameExtension;

        if (::strcmp(extensionId, CLAP_EXT_NOTE_PORTS) == 0)
            return &GetNotePortsExtension;

        if (::strcmp(extensionId, CLAP_EXT_PARAMS) == 0)
            return &GetParamsExtension;

        if (::strcmp(extensionId, CLAP_EXT_POSIX_FD_SUPPORT) == 0)
            return &GetPOSIXFDSupportExtension;

        if (::strcmp(extensionId, CLAP_EXT_STATE) == 0)
            return &GetStateExtension;

        if (::strcmp(extensionId, CLAP_EXT_THREAD_CHECK) == 0)
            return &GetThreadCheckExtension;

        if (::strcmp(extensionId, CLAP_EXT_THREAD_POOL) == 0)
            return &GetThreadPoolExtension;

        if (::strcmp(extensionId, CLAP_EXT_TIMER_SUPPORT) == 0)
            return &GetTimerSupportExtension;

        if (::strcmp(extensionId, CLAP_EXT_VOICE_INFO) == 0)
            return &GetVoiceInfoExtension;

        Log.AtDebug().Write(STR_COMPONENT_BASENAME " CLAP plug-in requests unsupported extension \"%s\".", extensionId);

        return nullptr;
    };

    // Handles a request to deactivate and reactivate the plug-in.
    request_restart = [](const clap_host * self)
    {
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " CLAP Host received request to deactivate and restart the plug-in.");
    };

    // Handles a request to activate and start processing the plug-in.
    request_process = [](const clap_host * self)
    {
        Log.AtTrace().Write(STR_COMPONENT_BASENAME " CLAP Host received request to activate and start processing the plug-in.");
    };

    // Handles a request to schedule a call to plugin->on_main_thread(plugin) on the main thread.
    request_callback = [](const clap_host * self)
    {
        if (auto * PlugIn = static_cast<CLAP::PlugIn *>(self->host_data))
            PlugIn->RequestCallback();
    };

    _CurrentDSO = nullptr;
}

/// <summary>
/// Gets the CLAP plug-ins.
/// </summary>
std::vector<PlugInEntry> Host::GetPlugInEntries(const fs::path & directoryPath) noexcept
{
    _PlugInEntries.clear();

    if (directoryPath.empty())
        return _PlugInEntries;

    #define CLAP_SDK_VERSION TOSTRING(CLAP_VERSION_MAJOR) "." TOSTRING(CLAP_VERSION_MINOR) "." TOSTRING(CLAP_VERSION_REVISION)

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " is built with CLAP " CLAP_SDK_VERSION ".");

    GetPlugInDescriptors_(directoryPath);

    std::sort(_PlugInEntries.begin(), _PlugInEntries.end(), [](PlugInEntry a, PlugInEntry b) { return a.Name < b.Name; });

    return _PlugInEntries;
}

/// <summary>
/// Releases the module after its last instance has been destroyed.
/// </summary>
DSO::~DSO()
{
    if (_Entry)
        _Entry->deinit();

    if (_hModule)
        ::FreeLibrary(_hModule);
}

bool Host::Load(const fs::path & filePath, uint32_t index)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("Host::Load() must be called from the main thread");

    if (filePath.empty() || index == static_cast<uint32_t>(-1))
        return false;

    auto Module = std::find_if(_PlugInModules.begin(), _PlugInModules.end(), [&filePath, index](const DSO & item)
    {
        return item._FilePath == filePath && item._Index == index;
    });

    if (Module == _PlugInModules.end())
    {
        // Keep the DLL and entry initialized for as long as a decoder or editor uses it.
        std::list<DSO> Candidate(1);
        auto & DSO = Candidate.front();
        DSO._FilePath = filePath;
        DSO._Index = index;
        DSO._hModule = ::LoadLibraryW(filePath.c_str());
        if (!DSO._hModule)
            return false;

        const auto * Entry = reinterpret_cast<const clap_plugin_entry_t *>(::GetProcAddress(DSO._hModule, "clap_entry"));
        if (!Entry || !Entry->init || !Entry->deinit || !Entry->get_factory ||
            !::clap_version_is_compatible(Entry->clap_version) || !Entry->init(reinterpret_cast<const char *>(filePath.u8string().c_str())))
            return false;

        DSO._Entry = Entry;
        DSO._Factory = static_cast<const clap_plugin_factory_t *>(Entry->get_factory(CLAP_PLUGIN_FACTORY_ID));
        if (!DSO._Factory || !DSO._Factory->get_plugin_descriptor || !DSO._Factory->create_plugin)
            return false;

        DSO._Descriptor = DSO._Factory->get_plugin_descriptor(DSO._Factory, index);
        if (!DSO._Descriptor)
            return false;

        Module = Candidate.begin();
        _PlugInModules.splice(_PlugInModules.end(), Candidate);
    }

    if (_CurrentDSO != &*Module)
    {
        Unload();
        _CurrentDSO = &*Module;
    }

    return true;
}

void Host::Unload()
{
    DSO * Previous = std::exchange(_CurrentDSO, nullptr);
    if (Previous && !Previous->_InstanceCount)
        _PlugInModules.remove_if([Previous](const DSO & item) { return &item == Previous; });
}

std::shared_ptr<PlugIn> Host::CreatePlugIn()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("Host::CreatePlugIn() must be called from the main thread");

    if (!_CurrentDSO)
        return nullptr;

    auto Instance = std::make_shared<PlugIn>(*this, *_CurrentDSO);
    ++_CurrentDSO->_InstanceCount;
    return Instance;
}

void Host::DestroyPlugIn(DSO & module)
{
    if (--module._InstanceCount == 0 && &module != _CurrentDSO)
        _PlugInModules.remove_if([&module](const DSO & item) { return &item == &module; });
}

#pragma region Private

/// <summary>
/// Gets the CLAP plug-ins.
/// </summary>
void Host::GetPlugInDescriptors_(const fs::path & directoryPath) noexcept
{
    try
    {
        for (const auto & Entry : fs::directory_iterator(directoryPath))
        {
            if (Entry.is_directory())
            {
                GetPlugInDescriptors_(Entry.path());
            }
            else
            if (msc::IsOneOf(Entry.path().extension().string().c_str(), { ".clap", ".dll" }))
            {
                Log.AtInfo().Write(STR_COMPONENT_BASENAME " is examining \"%s\"...", (const char *) Entry.path().u8string().c_str());

                #pragma warning(disable: 4820) // 'x' bytes padding
                GetPlugInEntries(Entry.path(), [this, Entry](const std::string & id, const std::string & plugInName, uint32_t index, bool hasGUI)
                {
                    PlugInEntry PlugIn =
                    {
                        .Id        = id,
                        .Name      = plugInName,
                        .Index     = index,
                        .FilePath  = Entry.path(),
                        .HasEditor = hasGUI
                    };

                    _PlugInEntries.push_back(PlugIn);
                });
                #pragma warning(default: 4820)
            }
        }
    }
    catch (const std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME, " failed to get CLAP plug-ins: %s", e.what());
    }
}

/// <summary>
/// Gets the CLAP plug-ins in plug-in file.
/// </summary>
void Host::GetPlugInEntries(const fs::path & filePath, const std::function<void(const std::string & id, const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept
{
    HMODULE hModule = ::LoadLibraryW(msc::UTF8ToWide((const char *) filePath.u8string().c_str()).c_str());

    if (hModule == NULL)
        return;

    auto PlugInEntry = (const clap_plugin_entry_t *) ::GetProcAddress(hModule, "clap_entry");

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
                            auto * PlugIn = Factory->create_plugin(Factory, this, Descriptor->id);

                            if (PlugIn != nullptr)
                            {
                                if (PlugIn->init(PlugIn))
                                {
                                    if (VerifyNotePorts(PlugIn) && VerifyAudioPorts(PlugIn))
                                        callback(std::string(Descriptor->id), std::string(Descriptor->name), i, HasGUI(PlugIn, false));
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

    ::FreeLibrary(hModule);
}

/// <summary>
/// Verifies if the note ports of the plug-in meet our conditions.
/// </summary>
bool Host::VerifyNotePorts(const clap_plugin_t * plugIn) noexcept
{
    if (plugIn->get_extension == nullptr)
        return true;

    const auto NotePorts = (const clap_plugin_note_ports_t *) plugIn->get_extension(plugIn, CLAP_EXT_NOTE_PORTS);

    if (NotePorts == nullptr)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins without the Note Ports extension.");

        return false;
    }

    const bool InputPort = true;
    const auto InputPortCount = NotePorts->count(plugIn, InputPort);

    if (InputPortCount != 1)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 1 MIDI input port.");

        return false;
    }
/*
    const bool OutputPort = false;
    const auto OutputPortCount = NotePorts->count(plugIn, OutputPort);

    if (OutputPortCount != 0)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with MIDI output ports.");

        return false;
    }
*/
    clap_note_port_info PortInfo = {};

    const uint32_t PortIndex = 0;

    if (!NotePorts->get(plugIn, PortIndex, InputPort, &PortInfo))
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to get input port information.");

        return false;
    }

    if ((PortInfo.supported_dialects & CLAP_NOTE_DIALECT_MIDI) == 0)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with note ports without MIDI dialect support.");

        return false;
    }

    return true;
}

/// <summary>
/// Verifies the audio ports of the plug-in meet our conditions.
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
    const bool InputPort = true;
    const auto InputPortCount = AudioPorts->count(plugIn, InputPort);

    if (InputPortCount != 0)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with audio input ports.");

        return false;
    }
*/
    const bool OutputPort = false;
/*
    const auto OutputPortCount = AudioPorts->count(plugIn, OutputPort);

    if (OutputPortCount != 1)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 1 audio output port.");

        return false;
    }
*/
    const uint32_t PortIndex = 0;

    clap_audio_port_info OutPortInfo = { };

    if (!AudioPorts->get(plugIn, PortIndex, OutputPort, &OutPortInfo))
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to get output port information.");

        return false;
    }

    if (!(OutPortInfo.channel_count == 2 && ::strcmp(OutPortInfo.port_type, CLAP_PORT_STEREO) == 0))
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " does not support CLAP plug-ins with more than 2 output channels in stereo.");

        return false;
    }

    return true;
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

    if (!GUI->is_api_supported(plugIn, CLAP_WINDOW_API_WIN32, isFloatingGUI))
        return false;

    return true;
}

/// <summary>
/// Implements the host's CLAP_EXT_AUDIO_PORTS extension.
/// </summary>
const clap_host_audio_ports_t Host::GetAudioPortsExtension =
{
   // Checks if the host allows a plugin to change a given aspect of the audio ports definition.
   .is_rescan_flag_supported = [](const clap_host_t * self, uint32_t flag) -> bool
    {
        return false; // TODO
    },

   // Rescan the full list of audio ports according to the flags. It is illegal to ask the host to rescan with a flag that is not supported. Certain flags require the plugin to be de-activated.
   .rescan = [](const clap_host_t * self, uint32_t flags)
    {
        // TODO
    },
};

/// <summary>
/// Implements the host's CLAP_EXT_AUDIO_PORTS_CONFIG extension.
/// Plugins with very complex configuration possibilities should let the user configure the ports from the plugin GUI, and call clap_host_audio_ports.rescan(CLAP_AUDIO_PORTS_RESCAN_ALL).
/// </summary>
const clap_host_audio_ports_config_t Host::GetAudioPortsConfigExtension =
{
    // Rescan the full list of configs.
    .rescan = [](const clap_host_t * self)
    {
        // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_CONTEXT_MENU extension.
/// This extension lets the host and plugin exchange menu items and let the plugin ask the host to show its context menu.
/// </summary>
const clap_host_context_menu_t Host::GetContextMenuExtension =
{
    // Insert host's menu items into the menu builder. If target is null, assume global context. Returns true on success.
    .populate = [](const clap_host_t * self, const clap_context_menu_target_t * target, const clap_context_menu_builder_t * builder) -> bool
    {
        return false; // TODO
    },

    // Performs the given action, which was previously provided to the plugin via populate(). If target is null, assume global context. Returns true on success.
    .perform = [](const clap_host_t * self, const clap_context_menu_target_t * target, clap_id action_id) -> bool
    {
        return false; // TODO
    },

    // Returns true if the host can display a popup menu for the plugin. This may depend upon the current windowing system used to display the plugin, so the return value is invalidated after creating the plugin window.
    .can_popup = [](const clap_host_t * self) -> bool
    {
        return false; // TODO
    },

    // Shows the host popup menu for a given parameter.
    // If the plugin is using embedded GUI, then x and y are relative to the plugin's window, otherwise they're absolute coordinate, and screen index might be set accordingly.
    // If target is null, assume global context. Returns true on success.
    .popup = [](const clap_host_t * host, const clap_context_menu_target_t * target, int32_t screen_index, int32_t x, int32_t y) -> bool
    {
        return false; // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_GUI extension.
/// </summary>
const clap_host_gui_t Host::GetGUIExtension =
{
    // The host should call get_resize_hints() again.
    .resize_hints_changed = [](const clap_host_t * self)
    {
        // TODO
    },

    // Request the host to resize the client area to width, height. Return true if the new size is accepted, false otherwise. The host doesn't have to call set_size().
    //
    // Note: if not called from the main thread, then a return value simply means that the host acknowledged the request and will process it asynchronously.
    // If the request then can't be satisfied then the host will call set_size() to revert the operation.
    .request_resize = [](const clap_host_t * self, uint32_t width, uint32_t height) -> bool
    {
        auto * PlugIn = static_cast<CLAP::PlugIn *>(self->host_data);
        return PlugIn && core_api::is_main_thread() && PlugIn->ResizeEditor(width, height);
    },

    .request_show = [](const clap_host_t * self) -> bool
    {
        auto * PlugIn = static_cast<CLAP::PlugIn *>(self->host_data);
        return PlugIn && core_api::is_main_thread() && PlugIn->SetEditorVisible(true);
    },

    .request_hide = [](const clap_host_t * self) -> bool
    {
        auto * PlugIn = static_cast<CLAP::PlugIn *>(self->host_data);
        return PlugIn && core_api::is_main_thread() && PlugIn->SetEditorVisible(false);
    },

    .closed = [](const clap_host_t * self, bool wasDestroyed)
    {
        if (auto * PlugIn = static_cast<CLAP::PlugIn *>(self->host_data))
            PlugIn->EditorClosed(wasDestroyed);
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_LATENCY extension.
/// </summary>
const clap_host_latency_t Host::GetLatencyExtension =
{
    // Tell the host that the latency changed.
    // The latency is only allowed to change during plugin->activate. If the plugin is activated, call host->request_restart()
    .changed = [](const clap_host_t * self)
    {
        Log.AtInfo().Write(STR_COMPONENT_BASENAME " CLAP player has changed the latency.");
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_LOG extension.
/// </summary>
const clap_host_log_t Host::GetLogExtension
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
/// Implements the host's CLAP_EXT_NOTE_NAME extension.
/// </summary>
const clap_host_note_name_t Host::GetNoteNameExtension
{
    // Informs the host that the note names have changed.
    .changed = [](const clap_host_t * self)
    {
        Log.AtInfo().Write(STR_COMPONENT_BASENAME " CLAP player has changed the note names.");
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
/// Implements the host's CLAP_EXT_PARAMS extension.
/// </summary>
const clap_host_params_t Host::GetParamsExtension
{
    // Rescan the full list of parameters according to the flags.
    .rescan = [](const clap_host_t * self, clap_param_rescan_flags flags)
    {
        // TODO
    },

    // Clears references to a parameter.
    .clear = [](const clap_host_t * self, clap_id param_id, clap_param_clear_flags flags)
    {
        // TODO
    },

    // Request a parameter flush.
    .request_flush = [](const clap_host_t * self)
    {
        // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_POSIX_FD_SUPPORT extension.
/// </summary>
const clap_host_posix_fd_support_t Host::GetPOSIXFDSupportExtension
{
    // Returns true on success.
    .register_fd = [](const clap_host_t * self, int fd, clap_posix_fd_flags_t flags) -> bool
    {
        return false; // TODO
    },

    // Returns true on success.
    .modify_fd = [](const clap_host_t * self, int fd, clap_posix_fd_flags_t flags) -> bool
    {
        return false; // TODO
    },

    // Returns true on success.
    .unregister_fd = [](const clap_host_t * self, int fd) -> bool
    {
        return false; // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_STATE extension. The plug-in reports its state has changed and should be saved again. If a parameter value changes, then it is implicit that the state is dirty.
/// </summary>
const clap_host_state_t Host::GetStateExtension
{
    .mark_dirty = [](const clap_host_t * self)
    {
        // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_THREAD_CHECK extension. This interface is useful to do runtime checks and make sure that the functions are called on the correct threads. It is highly recommended that hosts implement this extension.
/// </summary>
const clap_host_thread_check_t Host::GetThreadCheckExtension
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

/// <summary>
/// Implements the host's CLAP_EXT_POOL_CHECK extension.
/// </summary>
const clap_host_thread_pool_t Host::GetThreadPoolExtension
{
    // Schedule num_tasks jobs in the host thread pool.
    .request_exec = [](const clap_host_t * self, uint32_t taskCount) -> bool
    {
        return false; // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_TIMER_SUPPORT extension.
/// </summary>
const clap_host_timer_support_t Host::GetTimerSupportExtension
{
    // Registers a periodic timer. The host may adjust the period if it is under a certain threshold. 30 Hz should be allowed. Returns true on success.
    .register_timer = [](const clap_host_t * self, uint32_t period_ms, clap_id * timerId) -> bool
    {
        return false; // TODO
    },

    // Returns true on success.
    .unregister_timer = [](const clap_host_t * self, clap_id timerId) -> bool
    {
        return false; // TODO
    }
};

/// <summary>
/// Implements the host's CLAP_EXT_VOICE_INFO extension.
/// </summary>
const clap_host_voice_info_t Host::GetVoiceInfoExtension
{
    // Informs the host that the voice info has changed
    .changed = [](const clap_host_t * self)
    {
        Log.AtDebug().Write(STR_COMPONENT_BASENAME " CLAP player has changed its voice configuration");
    }
};

#pragma endregion
}

CLAP::Host _CLAPHost;
