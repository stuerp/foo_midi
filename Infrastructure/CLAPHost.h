
/** $VER: CLAPHost.h (2025.10.07) P. Stuer - Implements a CLAP host **/

#pragma once

#include "pch.h"

#include <clap/clap.h>

#include "CLAPPlugIn.h"
#include "CLAPWindow.h"

namespace CLAP
{
class Window;

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Represents a CLAP plug-in entry.
/// </summary>
struct PlugInEntry
{
    std::string Id;
    std::string Name;
    uint32_t Index;
    fs::path FilePath;
    bool HasEditor;
};

/// <summary>
/// Represents a CLAP DSO (Dynamically Shared Object)
/// </summary>
class DSO
{
public:
    DSO() : _Index(), _hModule(), _Entry(), _Factory(), _Descriptor(), _InstanceCount() { }

    fs::path _FilePath;
    uint32_t _Index;

    HMODULE _hModule;
    const clap_plugin_entry_t * _Entry;
    const clap_plugin_factory_t * _Factory;
    const clap_plugin_descriptor_t * _Descriptor;

    size_t _InstanceCount; // Number of plug-in instance created from this DSO
};

/// <summary>
/// Implements a host for CLAP plug-ins.
/// </summary>
class Host : public clap_host_t
{
public:
    Host() noexcept;

    Host(const Host &) = delete;
    Host(const Host &&) = delete;
    Host & operator=(const Host &) { return *this; }
    Host & operator=(Host &&) = delete;

    virtual ~Host() { };

    std::vector<PlugInEntry> GetPlugInEntries(const fs::path & directoryPath) noexcept;

    bool Load(const fs::path & filePath, uint32_t index);
    void Unload();

    std::shared_ptr<PlugIn> CreatePlugIn();
    void DestroyPlugIn();

    void OpenEditor(std::shared_ptr<PlugIn> plugIn, HWND hWnd, bool isFloating);

    bool IsPlugInLoaded() const noexcept { return (_CurrentDSO != nullptr) && (_CurrentDSO->_hModule != NULL); }

    std::string GetPlugInId() const noexcept { return ((_CurrentDSO != nullptr) && (_CurrentDSO->_Descriptor != nullptr)) ? _CurrentDSO->_Descriptor->id : ""; }
    std::string GetPlugInName() const noexcept { return ((_CurrentDSO != nullptr) && (_CurrentDSO->_Descriptor != nullptr)) ? _CurrentDSO->_Descriptor->name : ""; }

private:
    void GetPlugInDescriptors_(const fs::path & directoryPath) noexcept;
    void GetPlugInEntries(const fs::path & filePath, const std::function<void (const std::string & id, const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept;

    static bool VerifyNotePorts(const clap_plugin_t * plugIn) noexcept;
    static bool VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept;
    static bool HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept;

private:
    static const clap_host_audio_ports_t GetAudioPortsExtension;
    static const clap_host_audio_ports_config_t GetAudioPortsConfigExtension;
    static const clap_host_context_menu_t GetContextMenuExtension;
    static const clap_host_gui_t GetGUIExtension;
    static const clap_host_latency_t GetLatencyExtension;
    static const clap_host_log_t GetLogExtension;
    static const clap_host_note_name_t GetNoteNameExtension;
    static const clap_host_note_ports_t GetNotePortsExtension;
    static const clap_host_params_t GetParamsExtension;
    static const clap_host_posix_fd_support_t GetPOSIXFDSupportExtension;
    static const clap_host_state_t GetStateExtension;
    static const clap_host_timer_support_t GetTimerSupportExtension;
    static const clap_host_thread_check_t GetThreadCheckExtension;
    static const clap_host_thread_pool_t GetThreadPoolExtension;
    static const clap_host_voice_info_t GetVoiceInfoExtension;

    std::vector<PlugInEntry> _PlugInEntries;

    std::vector<DSO> _PlugInModules;
    DSO * _CurrentDSO;
};

}

extern CLAP::Host _CLAPHost;
