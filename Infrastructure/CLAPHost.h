
/** $VER: CLAPHost.h (2025.06.28) P. Stuer **/

#pragma once

#include <sdk/foobar2000-lite.h>

#include <filesystem>

#include <clap/clap.h>

namespace fs = std::filesystem;

namespace CLAP
{

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Represents a CLAP plug-in.
/// </summary>
struct PlugIn
{
    std::string Name;
    uint32_t Index;
    fs::path FilePath;
    bool HasGUI;
};

/// <summary>
/// Implements a host for CLAP plug-ins.
/// </summary>
class Host : public clap_host_t
{
public:
    Host() noexcept
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

    std::vector<PlugIn> GetPlugIns(const fs::path & directoryPath) noexcept;

private:
    void GetPlugIns_(const fs::path & directoryPath) noexcept;
    void GetPlugIns(const fs::path & filePath, const std::function<void (const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept;

    static bool VerifyNotePorts(const clap_plugin_t * plugIn) noexcept;
    static bool VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept;
    static bool HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept;

private:
    static const clap_host_log LogHandler;
    static const clap_host_note_ports_t NotePortsHandler;
    static const clap_host_state StateHandler;

    std::vector<PlugIn> _PlugIns;
};

extern Host _Host;

}
