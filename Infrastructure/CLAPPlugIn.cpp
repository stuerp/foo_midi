
/** $VER: CLAPPlugIn.cpp (2025.08.02) P. Stuer - Implements a CLAP plug-in wrapper **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "Support.h"
#include "Log.h"

#include "CLAPHost.h"
#include "CLAPPlugIn.h"
#include "CLAPPlayer.h"

namespace CLAP
{

PlugIn::PlugIn(const std::string & id, Host * host, const clap_plugin_t * plugIn) noexcept : _GUI(), _IsProcessing(false)
{
    _Id     = id;
    _Host   = host;
    _PlugIn = plugIn;
}

/// <summary>
/// Initializes the plug-in.
/// </summary>
bool PlugIn::Initialize()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::init() must be called from the main thread");

    // [main-thread]
    auto Result = _PlugIn->init(_PlugIn);

    LoadState(CfgCLAPConfig[_Id]);

    return Result;
}

/// <summary>
/// Terminates the plug-in.
/// </summary>
void PlugIn::Terminate()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::destroy() must be called from the main thread");

    if (_GUI)
    {
        HideGUI();

        // [main-thread]
        _GUI->destroy(_PlugIn);
        _GUI = nullptr;
    }

    if (_PlugIn)
    {
        // [main-thread & !active]
        _PlugIn->destroy(_PlugIn);
        _PlugIn = nullptr;
    }

    _Host->DestroyPlugIn();
}

/// <summary>
/// Activates the plug-in.
/// </summary>
bool PlugIn::Activate(double sampleRate)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::Activate() must be called from the main thread");

    // [main-thread & !active]
    if (!_PlugIn->activate(_PlugIn, sampleRate, 1, CLAP::BlockSize))
        return false;

    if (!GetAudioPortsInfo())
        return false;

    if (!GetVoiceInfo()) // Must be called after activate().
        return false;

    Log.AtInfo().Write(STR_COMPONENT_BASENAME " CLAP player has max. %d voices.", _VoiceInfo.voice_capacity);

    if ((_VoiceInfo.flags & CLAP_VOICE_INFO_SUPPORTS_OVERLAPPING_NOTES) == 0)
        Log.AtWarn().Write(STR_COMPONENT_BASENAME " CLAP player does not support overlapping notes.");

    return true;
}

/// <summary>
/// Deactivates the plug-in.
/// </summary>
void PlugIn::Deactivate() const
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::Dectivate() must be called from the main thread");

    // [main-thread & active]
    _PlugIn->deactivate(_PlugIn);
}

/// <summary>
/// Notifies the plug-in we're about to start processing.
/// </summary>
bool PlugIn::StartProcessing()
{
    if (_IsProcessing)
        throw std::runtime_error("PlugIn::StartProcessing() called when already processing");

    // [audio-thread & active & !processing]
    if (!_PlugIn->start_processing(_PlugIn))
        return false;

    _IsProcessing = true;

    return true;
}

/// <summary>
/// Notifies the plug-in we're about to stop processing.
/// </summary>
void PlugIn::StopProcessing()
{
    // [audio-thread & active & processing]
    if (_IsProcessing)
        _PlugIn->stop_processing(_PlugIn);

    _IsProcessing = false;
}

/// <summary>
/// Processes events.
/// </summary>
bool PlugIn::Process(const clap_process_t & processor) noexcept
{
    // [audio-thread & active & processing]
    return (_PlugIn->process(_PlugIn, &processor) != CLAP_PROCESS_ERROR);
}

/// <summary>
/// Returns true if the specified plug-in has a GUI.
/// </summary>
bool PlugIn::HasGUI(bool isFloatingGUI)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::HasGUI() must be called from the main thread");

    if (_PlugIn->get_extension == nullptr)
        return false;

    // Must be called after clap_plugin_t::init().
    const auto * GUI = (const clap_plugin_gui_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_GUI);

    if (GUI == nullptr)
        return false;

    // [main-thread]
    if (!GUI->is_api_supported(_PlugIn, CLAP_WINDOW_API_WIN32, isFloatingGUI))
        return false;

    return true;
}

/// <summary>
/// Creates the plug-in GUI.
/// </summary>
bool PlugIn::CreateGUI(bool isFloating)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::CreateGUI() must be called from the main thread");

    if (_PlugIn->get_extension == nullptr)
        return false;

    // Must be called after clap_plugin_t::init().
    _GUI = (const clap_plugin_gui_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_GUI);

    if (_GUI == nullptr)
        return false;

    // [main-thread]
    if (!_GUI->is_api_supported(_PlugIn, CLAP_WINDOW_API_WIN32, isFloating))
        return false;

    // [main-thread]
    if (!_GUI->create(_PlugIn, CLAP_WINDOW_API_WIN32, isFloating))
        return false;

    return true;
}

/// <summary>
/// Destroys the plug-in GUI.
/// </summary>
void PlugIn::DestroyGUI()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::CreateGUI() must be called from the main thread");

    if (_GUI == nullptr)
        return;

    // [main-thread]
    _GUI->destroy(_PlugIn);

    _GUI = nullptr;
}

/// <summary>
/// Shows the GUI to the host window.
/// </summary>
bool PlugIn::ShowGUI(HWND hWnd)
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::ShowGUI() must be called from the main thread");

    if (_GUI == nullptr)
        return false;

    clap_window_t w = { .api = CLAP_WINDOW_API_WIN32, .win32 = hWnd };

    // [main-thread & !floating]
    if (!_GUI->set_parent(_PlugIn, &w))
        return false;

    // [main-thread]
    if (!_GUI->show(_PlugIn))
        return false;

    return true;
}

/// <summary>
/// Hides the GUI from the host window.
/// </summary>
void PlugIn::HideGUI()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::HideGUI() must be called from the main thread");

    if (_GUI == nullptr)
        return;

    // [main-thread]
    _GUI->hide(_PlugIn);
}

/// <summary>
/// Gets the preferred size of the GUI.
/// </summary>
void PlugIn::GetPreferredGUISize(RECT & wr) const
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::ShowGUI() must be called from the main thread");

    if (_GUI == nullptr)
        return;

    uint32_t Width = 0, Height = 0;

    // [main-thread]
    if (!_GUI->get_size(_PlugIn, &Width, &Height))
    {
        Width  = 320;
        Height = 200;
    }

    wr.right  = (long) Width;
    wr.bottom = (long) Height;
}

/// <summary>
/// Gets information about the output audio ports.
/// </summary>
bool PlugIn::GetAudioPortsInfo() noexcept
{
    if (_PlugIn->get_extension == nullptr)
        return true;

    const auto AudioPorts = (const clap_plugin_audio_ports_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_AUDIO_PORTS);

    if (AudioPorts == nullptr)
        return false;

    const bool OutputPort = false;
    const uint32_t PortIndex = 0;

    return AudioPorts->get(_PlugIn, PortIndex, OutputPort, &_OutPortInfo);
}

/// <summary>
/// Gets the plug-in voice info extension.
/// </summary>
bool PlugIn::GetVoiceInfo() noexcept
{
    if (_PlugIn->get_extension == nullptr)
        return false;

    const auto PlugInVoiceInfo = (const clap_plugin_voice_info *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_VOICE_INFO);

    if (PlugInVoiceInfo == nullptr)
        return false;

    if (!PlugInVoiceInfo->get(_PlugIn, &_VoiceInfo))
        return false;

    return true;
}

/// <summary>
/// Gets the plugin state.
/// </summary>
const std::vector<uint8_t> PlugIn::SaveState() const noexcept
{
    if (_PlugIn == nullptr)
        return { };

    if (_PlugIn->get_extension == nullptr)
        return { };

    auto StateContext = (const clap_plugin_state_context_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_STATE_CONTEXT);

    if (StateContext == nullptr)
        return { };

    std::vector<uint8_t> State;

    clap_ostream_t Stream
    {
        .ctx = &State,

        // Returns the number of bytes written; -1 on write error
        .write = [](const clap_ostream * stream, const void * data, uint64_t size) -> int64_t
        {
            auto State = (std::vector<uint8_t> *) stream->ctx;

            State->insert(State->end(), (uint8_t *) data, (uint8_t *) data + size);

            return (int64_t) size;
        }
    };

    StateContext->save(_PlugIn, &Stream, CLAP_STATE_CONTEXT_FOR_DUPLICATE);

    return State;
}

/// <summary>
/// Sets the plugin state.
/// </summary>
void PlugIn::LoadState(const std::vector<uint8_t> & state) noexcept
{
    if (_PlugIn == nullptr)
        return;

    if (_PlugIn->get_extension == nullptr)
        return;

    auto StateContext = (const clap_plugin_state_context_t *) _PlugIn->get_extension(_PlugIn, CLAP_EXT_STATE_CONTEXT);

    if (StateContext == nullptr)
        return;

    std::vector<uint8_t> State;

    clap_istream_t Stream
    {
        .ctx = &State,

        // Returns the number of bytes read; 0 indicates end of file and -1 a read error.
        .read = [](const clap_istream * stream, void * data, uint64_t size) -> int64_t
        {
            auto State = (std::vector<uint8_t> *) stream->ctx;

            size = std::min((size_t) size, State->size());

            ::memcpy(data, State->data(), size);

            return (int64_t) size;
        }
    };

    StateContext->load(_PlugIn, &Stream, CLAP_STATE_CONTEXT_FOR_DUPLICATE);
}

}
