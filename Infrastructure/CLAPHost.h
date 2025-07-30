
/** $VER: CLAPHost.h (2025.07.30) P. Stuer - Implements a CLAP host (Work in Progress) **/

#pragma once

#include <clap/clap.h>

#include "CLAPWindow.h"

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
    bool HasEditor;
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

    std::vector<PlugIn> GetPlugIns(const fs::path & directoryPath) noexcept;

    bool Load(const fs::path & filePath, uint32_t index);
    void Unload();

    bool ActivatePlugIn(double sampleRate);
    void DeactivatePlugIn() const;

    bool StartProcessing() noexcept;
    void StopProcessing() noexcept;

    bool IsProcessing() const noexcept { return _IsProcessing; }
    bool IsPlugInLoaded() const noexcept { return (_PlugIn != nullptr); }

    bool Process(const clap_process_t & processor) noexcept;

    void OpenEditor(HWND hWnd, bool isFloating) noexcept;

    std::string GetPlugInName() const noexcept { return (_PlugInDescriptor != nullptr) ? _PlugInDescriptor->name : ""; }

    void GetPreferredGUISize(RECT & wr) const noexcept;
    void ShowGUI(HWND hWnd) noexcept;
    void HideGUI() noexcept;

    bool PlugInPrefers64bitAudio() const noexcept { return ((_OutPortInfo.flags & CLAP_AUDIO_PORT_PREFERS_64BITS) == CLAP_AUDIO_PORT_PREFERS_64BITS); }

    const char * GetPlugInId() const noexcept { return _PlugInDescriptor ? _PlugInDescriptor->id : ""; }

    const std::vector<uint8_t> GetPlugInState() const noexcept;
    void SetPlugInState(const std::vector<uint8_t> & state) noexcept;

private:
    void GetPlugIns_(const fs::path & directoryPath) noexcept;
    void GetPlugInEntries(const fs::path & filePath, const std::function<void (const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept;

    static bool VerifyNotePorts(const clap_plugin_t * plugIn) noexcept;
    bool VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept;
    static bool HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept;

    void GetVoiceInfo() noexcept;

    void InitializeGUI(bool isFloating) noexcept;

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

    std::vector<PlugIn> _PlugIns;

    fs::path _FilePath;
    uint32_t _Index;

    HMODULE _hPlugIn;
    const clap_plugin_descriptor_t * _PlugInDescriptor;
    const clap_plugin_t * _PlugIn;
    const clap_plugin_gui_t * _PlugInGUI;

    clap_audio_port_info _OutPortInfo;

    CLAP::Window _Window;

    bool _IsProcessing;
};

 const uint32_t BlockSize = 2 * 256;

}

extern CLAP::Host _CLAPHost;
