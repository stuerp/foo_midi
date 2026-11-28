
/** $VER: CLAPPlugIn.h (2025.08.02) P. Stuer - Implements a CLAP plug-in wrapper **/

#pragma once

#include <clap/clap.h>
#include <memory>

namespace CLAP
{
class Host;
class DSO;
class Window;

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Implements a CLAP plug-in wrapper
/// </summary>
class PlugIn : public std::enable_shared_from_this<PlugIn>
{
public:
    PlugIn(Host & host, DSO & module);
    ~PlugIn();

    PlugIn(const PlugIn &) = delete;
    PlugIn(const PlugIn &&) = delete;
    PlugIn & operator=(const PlugIn &) { return *this; }
    PlugIn & operator=(PlugIn &&) = delete;

    bool Initialize();
    void Terminate();

    bool Activate(double sampleRate);
    void Deactivate();

    bool StartProcessing();
    void StopProcessing();

    bool Prefers64bitAudio() const noexcept { return ((_OutPortInfo.flags & CLAP_AUDIO_PORT_PREFERS_64BITS) == CLAP_AUDIO_PORT_PREFERS_64BITS); }
    bool IsProcessing() const noexcept { return _IsProcessing; }

    bool Process(const clap_process_t & processor) noexcept;

    bool OpenEditor(HWND owner, bool live, bool activate);
    void CloseEditor();
    void FinishEditor(bool saveState);
    bool ResizeEditor(uint32_t width, uint32_t height);
    bool SetEditorVisible(bool visible);
    void EditorClosed(bool wasDestroyed);
    void RequestCallback();

    const char * GetName() const noexcept;
    const fs::path & GetFilePath() const noexcept;

    void SetGUIScale(double scale) const;
    void GetPreferredGUISize(RECT & wr) const;
    bool AttachGUI(HWND parent);
    bool ShowGUI();

private:
    std::vector<uint8_t> SaveState() const;
    void LoadState(const std::vector<uint8_t> & state);
    void DestroyGUI();

    bool GetAudioPortsInfo() noexcept;
    bool GetVoiceInfo() noexcept;

private:
    Host & _Host;
    DSO & _Module;
    clap_host_t _Callbacks;
    const clap_plugin_t * _PlugIn = nullptr;
    const clap_plugin_gui_t * _GUI = nullptr;
    std::unique_ptr<Window> _Editor;

    clap_audio_port_info _OutPortInfo { };
    clap_voice_info _VoiceInfo { };

    bool _IsActive = false;
    bool _IsProcessing = false;
    bool _GuiWasDestroyed = false;
};

 const uint32_t BlockSize = 2 * 256;

}
