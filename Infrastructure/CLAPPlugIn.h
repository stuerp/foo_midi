
/** $VER: CLAPPlugIn.h (2025.08.02) P. Stuer - Implements a CLAP plug-in wrapper **/

#pragma once

#include <clap/clap.h>

namespace CLAP
{
class Host;

#pragma warning(disable: 4820) // 'x' bytes padding added after data member 'y'

/// <summary>
/// Implements a CLAP plug-in wrapper
/// </summary>
class PlugIn
{
public:
    PlugIn(Host * host, const clap_plugin_t * plugin) noexcept;

    PlugIn(const PlugIn &) = delete;
    PlugIn(const PlugIn &&) = delete;
    PlugIn & operator=(const PlugIn &) { return *this; }
    PlugIn & operator=(PlugIn &&) = delete;

    virtual ~PlugIn() { };

    bool Initialize();
    void Terminate();

    bool Activate(double sampleRate);
    void Deactivate() const;

    bool StartProcessing();
    void StopProcessing();

    bool IsProcessing() const noexcept { return _IsProcessing; }

    bool Process(const clap_process_t & processor) noexcept;

    void GetPreferredGUISize(RECT & wr) const noexcept;
    void ShowGUI(HWND hWnd);
    void HideGUI();

    bool Prefers64bitAudio() const noexcept { return ((_OutPortInfo.flags & CLAP_AUDIO_PORT_PREFERS_64BITS) == CLAP_AUDIO_PORT_PREFERS_64BITS); }

    const std::vector<uint8_t> SaveState() const noexcept;
    void LoadState(const std::vector<uint8_t> & state) noexcept;

    bool HasGUI(bool isFloatingGUI) noexcept;

private:
    bool GetAudioPortsInfo() noexcept;
    bool GetVoiceInfo() noexcept;

    void InitializeGUI(bool isFloating) noexcept;

private:
    const Host * _Host;
    const clap_plugin_t * _PlugIn;
    const clap_plugin_gui_t * _PlugInGUI;

    clap_audio_port_info _OutPortInfo { };
    clap_voice_info _VoiceInfo { };

    bool _IsProcessing;
};

 const uint32_t BlockSize = 2 * 256;

}
