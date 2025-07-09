
/** $VER: MusicKeyboardUIElement.h (2023.12.24) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 5262 ALL_CPPCORECHECK_WARNINGS)

#include "framework.h"

#include <queue>

#include <sdk/playback_control.h>
#include <sdk/play_callback.h>

#include <helpers/foobar2000-lite+atl.h>
#include <helpers/BumpableElem.h>
#include <libPPUI/win32_op.h>

#include "IMIDIReceiver.h"
#include "Keyboard.h"

#include "Resource.h"

namespace foo_vis_midi
{
/// <summary>
/// Implements the UIElement interface.
/// </summary>
#pragma warning(disable: 4820)
class MusicKeyboardWindow : public ui_element_instance, public CWindowImpl<MusicKeyboardWindow>, private play_callback_impl_base
{
public:
    explicit MusicKeyboardWindow() : _InterfaceVersion(~0U) { }

    MusicKeyboardWindow(const MusicKeyboardWindow &) = delete;
    MusicKeyboardWindow & operator=(const MusicKeyboardWindow &) = delete;
    MusicKeyboardWindow(MusicKeyboardWindow &&) = delete;
    MusicKeyboardWindow & operator=(MusicKeyboardWindow &&) = delete;

    virtual ~MusicKeyboardWindow();

    MusicKeyboardWindow(ui_element_config::ptr config, ui_element_instance_callback_ptr callback);

    #pragma region CWindowImpl

    DECLARE_WND_CLASS_EX(TEXT("{2941030c-fd32-4268-917d-587d4f06c43c}"), CS_VREDRAW | CS_HREDRAW, (-1));

    BEGIN_MSG_MAP_EX(MusicKeyboardWindow)
        MSG_WM_CREATE(OnCreate)
        MSG_WM_DESTROY(OnDestroy)
        MSG_WM_SIZE(OnResize);
        MSG_WM_PAINT(OnPaint)
    END_MSG_MAP()

    #pragma endregion

    #pragma region("ui_element_instance")

    /// <summary>
    /// 
    /// </summary>
    void initialize_window(HWND parent)
    {
        WIN32_OP(Create(parent) != NULL);
    }

    /// <summary>
    /// 
    /// </summary>
    HWND get_wnd()
    {
        return *this;
    }

    /// <summary>
    /// 
    /// </summary>
    void set_configuration(ui_element_config::ptr config)
    {
        _Config = config;
    }

    /// <summary>
    /// 
    /// </summary>
    ui_element_config::ptr get_configuration()
    {
        return _Config;
    }

    /// <summary>
    /// 
    /// </summary>
    static GUID g_get_guid()
    {
        static const GUID _GUID = {0xdc7a4e88,0x9c0e,0x4f82,{0xbd,0x99,0x21,0xeb,0x3b,0x41,0x37,0x68}};

        return _GUID;
    }

    /// <summary>
    /// 
    /// </summary>
    static GUID g_get_subclass()
    {
        return ui_element_subclass_utility;
    }

    /// <summary>
    /// 
    /// </summary>
    static void g_get_name(pfc::string_base& out)
    {
        out = STR_COMPONENT_NAME " Musical Keyboard";
    }

    /// <summary>
    /// 
    /// </summary>
    static const char * g_get_description()
    {
        return "Visualizes MIDI messages.";
    }

    /// <summary>
    /// 
    /// </summary>
    static ui_element_config::ptr g_get_default_configuration()
    {
        return ui_element_config::g_create_empty(g_get_guid());
    }

    /// <summary>
    /// 
    /// </summary>
    void notify(const GUID& notificationGUID, t_size, const void *, t_size)
    {
        if (notificationGUID == ui_element_notify_colors_changed || notificationGUID == ui_element_notify_font_changed)
            Invalidate();
    }

    #pragma endregion

    #pragma region IMIDIReceiver

    void Initialize(uint32_t interfaceVersion) noexcept { _InterfaceVersion = interfaceVersion; }
    void ProcessMessage(uint32_t message, uint32_t timestamp) noexcept;
    void SetPosition(uint32_t position)  noexcept { _Position = position; Invalidate(); }

    #pragma endregion

protected:
    // This must be declared as protected for ui_element_impl_withpopup<> to work.
    const ui_element_instance_callback_ptr m_callback; // Don't rename this.

private:
    #pragma region("play_callback")

    void on_playback_starting(play_control::t_track_command, bool) { }
    void on_playback_new_track(metadb_handle_ptr);
    void on_playback_stop(play_control::t_stop_reason);
    void on_playback_seek(double) { }
    void on_playback_pause(bool);
    void on_playback_edited(metadb_handle_ptr) { }
    void on_playback_dynamic_info(const file_info&) { }
    void on_playback_dynamic_info_track(const file_info&) { }
    void on_playback_time(double) { }
    void on_volume_change(float) { }

    #pragma endregion

private:
    #pragma region CWindowImpl

    LRESULT OnCreate(LPCREATESTRUCT);
    void OnDestroy();
    void OnResize(UINT type, CSize size);
    void OnPaint(CDCHandle);

    static VOID CALLBACK TimerCallback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer) noexcept;

    void UpdateRefreshRateLimit() const noexcept;

    #pragma endregion

private:
    #pragma region Rendering

    HRESULT Render();

    void SetScale(ID2D1RenderTarget * renderTarget);

    #pragma endregion

    #pragma region Direct2D

    HRESULT CreateDeviceIndependentResources();
    HRESULT CreateDeviceDependentResources();
    HRESULT CreateGridPatternBrush(ID2D1RenderTarget *, ID2D1BitmapBrush **);
    void Dispose();

    #pragma endregion

private:
    struct MIDIEvent
    {
        uint32_t Message;
        uint32_t Timestamp;

        MIDIEvent() : Message(0), Timestamp(~0U) { }
        MIDIEvent(uint32_t message, uint32_t timestamp) : Message(message), Timestamp(timestamp) { }

        bool IsValid() const noexcept { return Timestamp != ~0U; }
    };

    uint32_t _InterfaceVersion;
    CRITICAL_SECTION _Lock;
    PTP_TIMER _ThreadPoolTimer;
    uint32_t _Position;

    std::queue<MIDIEvent> _Events;

    ui_element_config::ptr _Config;

    #pragma region Direct2D

    // Device-independent resources
    CComPtr<ID2D1Factory> _D2DFactory;

    // Device-dependent resources
    CComPtr<ID2D1HwndRenderTarget> _RenderTarget;
    CComPtr<ID2D1BitmapBrush> _GridPatternBrush;

    #pragma endregion

    std::unique_ptr<Keyboard> _Keyboard;
};
#pragma warning(default: 4820)

/// <summary>
/// ui_element_impl_withpopup autogenerates standalone version of our component and proper menu commands.
/// Use ui_element_impl instead if you don't want that.
/// </summary>
class MusicKeyboardUIElement : public ui_element_impl_withpopup<MusicKeyboardWindow>
{
public:
    explicit MusicKeyboardUIElement() { };

    MusicKeyboardUIElement(const MusicKeyboardUIElement &) = delete;
    MusicKeyboardUIElement & operator=(const MusicKeyboardUIElement &) = delete;
    MusicKeyboardUIElement(MusicKeyboardUIElement &&) = delete;
    MusicKeyboardUIElement & operator=(MusicKeyboardUIElement &&) = delete;

    virtual ~MusicKeyboardUIElement() { };
};

extern MusicKeyboardWindow * _This;

/// <summary>
/// Handles the playback events we're subscribed to.
/// </summary>
class PlaybackEventHandler : public play_callback_static
{
public:
    PlaybackEventHandler() = delete;

    PlaybackEventHandler(const PlaybackEventHandler &) = delete;
    PlaybackEventHandler & operator=(const PlaybackEventHandler &) = delete;
    PlaybackEventHandler(PlaybackEventHandler &&) = delete;
    PlaybackEventHandler & operator=(PlaybackEventHandler &&) = delete;

    virtual ~PlaybackEventHandler() = delete;

    /// <summary>
    /// Controls which methods your callback wants called; returned value should not change in run time, you should expect it to be queried only once (on startup). See play_callback::flag_* constants.
    /// </summary>
    virtual unsigned get_flags()
    {
        return flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_pause;
    }

    /// <summary>
    /// Playback process is being initialized. on_playback_new_track() should be called soon after this when first file is successfully opened for decoding.
    /// </summary>
    virtual void on_playback_starting(play_control::t_track_command, bool) { }

    /// <summary>
    /// Playback advanced to new track.
    /// </summary>
    virtual void on_playback_new_track(metadb_handle_ptr track);

    /// <summary>
    /// Playback stopped.
    /// </summary>
    virtual void on_playback_stop(play_control::t_stop_reason reason);

    /// <summary>
    /// The user has seeked to a specific time.
    /// </summary>
    virtual void on_playback_seek(double) { }

    /// <summary>
    /// Playback paused/resumed.
    /// </summary>
    virtual void on_playback_pause(bool);

    /// <summary>
    /// Current track gets edited.
    /// </summary>
    virtual void on_playback_edited(metadb_handle_ptr) { }

    /// <summary>
    /// Dynamic info f.e. VBR bitrate changed.
    /// </summary>
    virtual void on_playback_dynamic_info(const file_info &) { }

    /// <summary>
    /// Per-track dynamic info (stream track titles etc) changed. Happens less often than on_playback_dynamic_info().
    /// </summary>
    virtual void on_playback_dynamic_info_track(const file_info &) { }

    /// <summary>
    /// Called every second, for time display
    /// </summary>
    virtual void on_playback_time(double) { }

    /// <summary>
    /// User changed volume settings. Possibly called when not playing.
    /// @param p_new_val new volume level in dB; 0 for full volume.
    /// </summary>
    virtual void on_volume_change(float) { }
};
}
