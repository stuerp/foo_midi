/** $VER: CLAPWindow.h (2025.08.01) P. Stuer **/

#pragma once

#include "pch.h"

#include <atlcrack.h>
#include <sdk/coreDarkMode.h>
#include <sdk/ui_element.h>

#include "Resource.h"

namespace CLAP
{
class PlugIn;

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements the parent window for an embedded CLAP editor.
/// </summary>
class Window : public CDialogImpl<Window>, private ui_config_callback_impl
{
public:
    explicit Window(PlugIn & plugIn) noexcept : _PlugIn(plugIn) { }

    enum { IDD = IDD_CLAP_WINDOW };

    bool Open(HWND owner, bool live, bool activate);
    void Show(bool activate);
    void Close();
    bool Resize(uint32_t width, uint32_t height);
    bool SetVisible(bool visible);

private:
    BOOL OnInitDialog(CWindow, LPARAM);
    void OnDestroy();
    void ui_colors_changed() override;
    void SetEditorIcons(HICON largeIcon, HICON smallIcon);
    void FitEditor(uint32_t width, uint32_t height);
    void OnCancel(UINT, int, CWindow) { Close(); }

    BEGIN_MSG_MAP_EX(Window)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CLOSE(Close)
        MSG_WM_DESTROY(OnDestroy)
        COMMAND_ID_HANDLER_EX(IDCANCEL, OnCancel)
    END_MSG_MAP()

    PlugIn & _PlugIn;
    fb2k::CCoreDarkModeHooks _DarkModeHooks;
    service_ptr _LiveEditScope;
    HICON _LargeIcon = nullptr;
    HICON _SmallIcon = nullptr;
    double _EditorSizeScale = 1.0;
    bool _Live = false;
    bool _Opened = false;
    bool _UseFallbackIcons = false;
};

}
