/** $VER: CLAPWindow.cpp (2025.08.02) P. Stuer **/

#include "pch.h"

#include <shellapi.h>
#include <cmath>
#include <sdk/dsp_manager.h>

#include "CLAPWindow.h"
#include "CLAPPlugIn.h"
#include "Encoding.h"
#include "Log.h"

namespace CLAP
{
bool Window::Open(HWND owner, bool live, bool activate)
{
    _Live = live;
    _Opened = false;
    _EditorSizeScale = 1.0;

    if (!_Live)
        return DoModal(owner) != -1 && _Opened;

    if (!Create(owner) || !_Opened)
    {
        Close();
        return false;
    }

    Show(activate);
    return true;
}

void Window::Show(bool activate)
{
    if (!m_hWnd)
        return;

    if (activate)
    {
        ShowWindow(SW_RESTORE);
        ::SetForegroundWindow(m_hWnd);
    }
    else
    {
        ShowWindow(SW_SHOWNOACTIVATE);
    }

    // Limit decoder lookahead while the live UI is visible so its meters follow the audible track.
    if (_Live && _LiveEditScope.is_empty())
    {
        if (auto Manager = dsp_config_manager_v3::tryGet(); Manager.is_valid())
            _LiveEditScope = Manager->acquire_live_edit_scope();
    }
}

void Window::Close()
{
    if (!m_hWnd)
        return;

    if (_Live)
        DestroyWindow();
    else
        EndDialog(0);
}

BOOL Window::OnInitDialog(CWindow, LPARAM)
{
    try
    {
        SetWindowTextW(msc::UTF8ToWide(_PlugIn.GetName()).c_str());
        _DarkModeHooks.AddDialog(m_hWnd);

        HICON LargeIcon = nullptr;
        HICON SmallIcon = nullptr;
        ::ExtractIconExW(_PlugIn.GetFilePath().c_str(), 0, &LargeIcon, &SmallIcon, 1);
        _UseFallbackIcons = !LargeIcon && !SmallIcon;
        if (_UseFallbackIcons)
            ui_colors_changed();
        else
            SetEditorIcons(LargeIcon, SmallIcon);

        RECT Bounds = {};
        _PlugIn.GetPreferredGUISize(Bounds);
        const auto Width = static_cast<uint32_t>(Bounds.right);
        const auto Height = static_cast<uint32_t>(Bounds.bottom);
        Resize(Width, Height);

        if (_PlugIn.AttachGUI(m_hWnd))
        {
            FitEditor(Width, Height);
            CenterWindow(GetParent());
            _Opened = _PlugIn.ShowGUI();
        }
    }
    catch (const std::exception & Error)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to open the CLAP editor: %s", Error.what());
    }

    if (!_Opened && !_Live)
        EndDialog(0);

    return _Live ? FALSE : TRUE;
}

void Window::FitEditor(uint32_t width, uint32_t height)
{
    const double Scale = static_cast<double>(::GetDpiForWindow(m_hWnd)) / USER_DEFAULT_SCREEN_DPI;
    const HWND Child = ::GetWindow(m_hWnd, GW_CHILD);
    RECT Native = {};
    if (Scale != 1.0 && Child && ::GetWindowRect(Child, &Native) &&
        std::abs((Native.right - Native.left) - std::lround(width * Scale)) <= 1 &&
        std::abs((Native.bottom - Native.top) - std::lround(height * Scale)) <= 1)
    {
        // Some editors apply Win32 DPI scaling themselves and report their size in logical pixels.
        _EditorSizeScale = Scale;
        Resize(width, height);
        return;
    }

    _PlugIn.SetGUIScale(Scale);
    RECT Bounds = {};
    _PlugIn.GetPreferredGUISize(Bounds);
    Resize(static_cast<uint32_t>(Bounds.right), static_cast<uint32_t>(Bounds.bottom));
}

bool Window::Resize(uint32_t width, uint32_t height)
{
    if (!m_hWnd || width == 0 || height == 0 || width > 32767 || height > 32767)
        return false;

    RECT Bounds = { 0, 0, std::lround(width * _EditorSizeScale), std::lround(height * _EditorSizeScale) };
    if (!::AdjustWindowRectExForDpi(&Bounds, GetStyle(), FALSE, GetExStyle(), ::GetDpiForWindow(m_hWnd)))
        return false;

    return SetWindowPos(nullptr, 0, 0, Bounds.right - Bounds.left, Bounds.bottom - Bounds.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE) != FALSE;
}

bool Window::SetVisible(bool visible)
{
    if (!m_hWnd)
        return false;

    if (visible)
        Show(false);
    else
    {
        ShowWindow(SW_HIDE);
        _LiveEditScope.release();
    }
    return true;
}

void Window::OnDestroy()
{
    _LiveEditScope.release();

    try
    {
        _PlugIn.FinishEditor(_Opened);
    }
    catch (const std::exception & Error)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to save the CLAP editor state: %s", Error.what());
    }

    _UseFallbackIcons = false;
    SetEditorIcons(nullptr, nullptr);
}

void Window::ui_colors_changed()
{
    if (!m_hWnd || !_UseFallbackIcons)
        return;

    const auto Resource = MAKEINTRESOURCEW(ui_config_manager::g_is_dark_mode() ? IDI_CLAP_DARK : IDI_CLAP_NORMAL);
    const UINT Dpi = ::GetDpiForWindow(m_hWnd);
    const HINSTANCE Instance = core_api::get_my_instance();

    const auto LargeIcon = static_cast<HICON>(::LoadImageW(Instance, Resource, IMAGE_ICON,
        ::GetSystemMetricsForDpi(SM_CXICON, Dpi), ::GetSystemMetricsForDpi(SM_CYICON, Dpi), LR_DEFAULTCOLOR));
    const auto SmallIcon = static_cast<HICON>(::LoadImageW(Instance, Resource, IMAGE_ICON,
        ::GetSystemMetricsForDpi(SM_CXSMICON, Dpi), ::GetSystemMetricsForDpi(SM_CYSMICON, Dpi), LR_DEFAULTCOLOR));

    if (LargeIcon || SmallIcon)
        SetEditorIcons(LargeIcon, SmallIcon);
}

void Window::SetEditorIcons(HICON largeIcon, HICON smallIcon)
{
    SetIcon(largeIcon ? largeIcon : smallIcon, TRUE);
    SetIcon(smallIcon ? smallIcon : largeIcon, FALSE);

    if (const auto Previous = std::exchange(_LargeIcon, largeIcon))
        ::DestroyIcon(Previous);
    if (const auto Previous = std::exchange(_SmallIcon, smallIcon))
        ::DestroyIcon(Previous);
}

}
