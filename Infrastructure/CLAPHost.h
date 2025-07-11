
/** $VER: CLAPHost.h (2025.07.11) P. Stuer **/

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
    bool HasGUI;
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

    bool Load(const fs::path & filePath, uint32_t index) noexcept;
    void UnLoad() noexcept;
    bool IsPlugInLoaded() const noexcept;

    bool ActivatePlugIn(double sampleRate, uint32_t minFrames, uint32_t maxFrames) noexcept;
    void DeactivatePlugIn() const noexcept;

    bool Process(const clap_process_t & processor) noexcept;

    bool HasGUI() const noexcept;
    void ShowGUI(HWND hWnd) noexcept;
    void HideGUI() noexcept;
    bool IsGUIVisible() const noexcept;

    std::string GetPlugInName() const noexcept { return (_PlugInDescriptor != nullptr) ? _PlugInDescriptor->name : ""; }

    void GetGUISize(RECT & wr) const noexcept;
    bool SetWindow(clap_window_t * w) { return _PlugInGUI->set_parent(_PlugIn, w); }

    bool PlugInPrefers64bitAudio() const noexcept;

private:
    void GetPlugIns_(const fs::path & directoryPath) noexcept;
    void GetPlugInEntries(const fs::path & filePath, const std::function<void (const std::string & name, uint32_t index, bool hasGUI)> & callback) noexcept;

    static bool VerifyNotePorts(const clap_plugin_t * plugIn) noexcept;
    bool VerifyAudioPorts(const clap_plugin_t * plugIn) noexcept;
    static bool HasGUI(const clap_plugin_t * plugIn, bool isFloatingGUI) noexcept;

    void GetVoiceInfo() noexcept;

    void GetGUI() noexcept;

private:
    static const clap_host_log LogHandler;
    static const clap_host_note_ports_t NotePortsHandler;
    static const clap_host_state StateHandler;

    std::vector<PlugIn> _PlugIns;

    fs::path _FilePath;
    uint32_t _Index;

    HMODULE _hPlugIn;
    const clap_plugin_descriptor_t * _PlugInDescriptor;
    const clap_plugin_t * _PlugIn;
    const clap_plugin_gui_t * _PlugInGUI;

    clap_audio_port_info _PortInfo;

    CLAP::Window _Window;
};

// This shared instance must only be used by the UI element and the playback thread.
extern Host _Host;
}
