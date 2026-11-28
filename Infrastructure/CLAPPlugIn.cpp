
/** $VER: CLAPPlugIn.cpp (2025.08.02) P. Stuer - Implements a CLAP plug-in wrapper **/

#include "pch.h"

#include "Configuration.h"
#include "Resource.h"
#include "Support.h"
#include "Log.h"

#include "CLAPHost.h"
#include "CLAPPlugIn.h"
#include "CLAPWindow.h"

#include <sdk/threadsLite.h>
#include <span>

namespace CLAP
{

PlugIn::PlugIn(Host & host, DSO & module)
    : _Host(host), _Module(module), _Callbacks(host)
{
    _Callbacks.host_data = this;
}

PlugIn::~PlugIn() = default;

bool PlugIn::Initialize()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::Initialize() must be called from the main thread");

    _PlugIn = _Module._Factory->create_plugin(_Module._Factory, &_Callbacks, _Module._Descriptor->id);
    if (!_PlugIn || !_PlugIn->init(_PlugIn))
        return false;

    LoadState(CfgCLAPConfig[_Module._Descriptor->id]);
    return true;
}

void PlugIn::Terminate()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::Terminate() must be called from the main thread");

    CloseEditor();
    Deactivate();

    if (_PlugIn)
        _PlugIn->destroy(std::exchange(_PlugIn, nullptr));

    _Host.DestroyPlugIn(_Module);
}

const char * PlugIn::GetName() const noexcept
{
    return _Module._Descriptor->name;
}

const fs::path & PlugIn::GetFilePath() const noexcept
{
    return _Module._FilePath;
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

    _IsActive = true;

    if (GetVoiceInfo())
    {
        Log.AtInfo().Write(STR_COMPONENT_BASENAME " CLAP player has max. %d voices.", _VoiceInfo.voice_capacity);

        if ((_VoiceInfo.flags & CLAP_VOICE_INFO_SUPPORTS_OVERLAPPING_NOTES) == 0)
            Log.AtWarn().Write(STR_COMPONENT_BASENAME " CLAP player does not support overlapping notes.");
    }

    return GetAudioPortsInfo();
}

/// <summary>
/// Deactivates the plug-in.
/// </summary>
void PlugIn::Deactivate()
{
    if (!core_api::is_main_thread())
        throw std::runtime_error("PlugIn::Dectivate() must be called from the main thread");

    // [main-thread & active]
    if (std::exchange(_IsActive, false))
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

bool PlugIn::OpenEditor(HWND owner, bool live, bool activate)
{
    if (!_PlugIn || !_PlugIn->get_extension || !::IsWindow(owner))
        return false;

    if (_GUI)
    {
        _Editor->Show(activate);
        return true;
    }

    const auto * GUI = static_cast<const clap_plugin_gui_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_GUI));
    if (!GUI || !GUI->is_api_supported(_PlugIn, CLAP_WINDOW_API_WIN32, false) ||
        !GUI->create(_PlugIn, CLAP_WINDOW_API_WIN32, false))
        return false;

    _GUI = GUI;
    _GuiWasDestroyed = false;

    if (!_Editor)
        _Editor = std::make_unique<Window>(*this);

    const bool Opened = _Editor->Open(owner, live, activate);
    if (!Opened)
        DestroyGUI();
    return Opened;
}

void PlugIn::CloseEditor()
{
    if (_Editor)
        _Editor->Close();
}

void PlugIn::FinishEditor(bool saveState)
{
    DestroyGUI();

    if (saveState)
        CfgCLAPConfig[_Module._Descriptor->id] = SaveState();
}

void PlugIn::DestroyGUI()
{
    if (const auto * GUI = std::exchange(_GUI, nullptr))
    {
        if (!_GuiWasDestroyed)
            GUI->hide(_PlugIn);

        GUI->destroy(_PlugIn);
    }
}

bool PlugIn::AttachGUI(HWND parent)
{
    clap_window_t Window = { .api = CLAP_WINDOW_API_WIN32, .win32 = parent };
    return _GUI->set_parent(_PlugIn, &Window);
}

bool PlugIn::ShowGUI()
{
    return _GUI->show(_PlugIn);
}

bool PlugIn::ResizeEditor(uint32_t width, uint32_t height)
{
    return _Editor && _Editor->Resize(width, height);
}

bool PlugIn::SetEditorVisible(bool visible)
{
    return _Editor && _Editor->SetVisible(visible);
}

void PlugIn::EditorClosed(bool wasDestroyed)
{
    fb2k::inMainThread([Instance = weak_from_this(), wasDestroyed]
    {
        if (auto PlugIn = Instance.lock(); PlugIn && PlugIn->_GUI)
        {
            PlugIn->_GuiWasDestroyed = wasDestroyed;
            PlugIn->CloseEditor();
        }
    });
}

void PlugIn::RequestCallback()
{
    fb2k::inMainThread([Instance = weak_from_this()]
    {
        if (auto PlugIn = Instance.lock(); PlugIn && PlugIn->_PlugIn && PlugIn->_PlugIn->on_main_thread)
            PlugIn->_PlugIn->on_main_thread(PlugIn->_PlugIn);
    });
}

void PlugIn::SetGUIScale(double scale) const
{
    if (_GUI->set_scale)
        _GUI->set_scale(_PlugIn, scale);
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
std::vector<uint8_t> PlugIn::SaveState() const
{
    if (!_PlugIn || !_PlugIn->get_extension)
        return { };

    const auto * Context = static_cast<const clap_plugin_state_context_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_STATE_CONTEXT));
    const auto * Extension = static_cast<const clap_plugin_state_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_STATE));
    std::vector<uint8_t> State;
    const clap_ostream_t Stream
    {
        .ctx = &State,
        .write = [](const clap_ostream_t * stream, const void * data, uint64_t size) -> int64_t
        {
            auto & State = *static_cast<std::vector<uint8_t> *>(stream->ctx);
            const auto * Bytes = static_cast<const uint8_t *>(data);
            State.insert(State.end(), Bytes, Bytes + size);
            return static_cast<int64_t>(size);
        }
    };

    if (Context)
        Context->save(_PlugIn, &Stream, CLAP_STATE_CONTEXT_FOR_DUPLICATE);
    else if (Extension)
        Extension->save(_PlugIn, &Stream);

    return State;
}

void PlugIn::LoadState(const std::vector<uint8_t> & state)
{
    if (state.empty() || !_PlugIn || !_PlugIn->get_extension)
        return;

    const auto * Context = static_cast<const clap_plugin_state_context_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_STATE_CONTEXT));
    const auto * Extension = static_cast<const clap_plugin_state_t *>(_PlugIn->get_extension(_PlugIn, CLAP_EXT_STATE));
    std::span<const uint8_t> Bytes(state);
    const clap_istream_t Stream
    {
        .ctx = &Bytes,
        .read = [](const clap_istream_t * stream, void * data, uint64_t size) -> int64_t
        {
            auto & Bytes = *static_cast<std::span<const uint8_t> *>(stream->ctx);
            const auto Count = static_cast<size_t>(std::min<uint64_t>(size, Bytes.size()));
            if (Count)
                ::memcpy(data, Bytes.data(), Count);
            Bytes = Bytes.subspan(Count);
            return static_cast<int64_t>(Count);
        }
    };

    if (Context)
        Context->load(_PlugIn, &Stream, CLAP_STATE_CONTEXT_FOR_DUPLICATE);
    else if (Extension)
        Extension->load(_PlugIn, &Stream);
}

}
