
/** $VER: Window.cpp (2025.08.02) P. Stuer **/

#include "pch.h"

#include "CLAPWindow.h"
#include "CLAPPlugIn.h"
#include "CLAPHost.h"
#include "Encoding.h"

#include "Log.h"

namespace CLAP
{

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL Window::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    if (lParam == NULL)
        return FALSE;

    try
    {
        _Context = *(Context *) lParam;

        _Context._PlugIn->GetPreferredGUISize(_MinMaxBounds);

        AdjustSize(_MinMaxBounds);

        if (::IsRectEmpty(&_Context._Bounds))
            _Context._Bounds = _MinMaxBounds;

        MoveWindow(&_Context._Bounds);

        auto Text = std::wstring(L"CLAP Plug-in ") + msc::UTF8ToWide(_Context._Host->GetPlugInName()).c_str();

        SetWindowTextW(Text.c_str());

        _Context._PlugIn->ShowGUI(m_hWnd);

        return TRUE;
    }
    catch (std::exception & e)
    {
        Log.AtError().Write(STR_COMPONENT_BASENAME " failed to intialize the host window for the CLAP editor: %s", e.what());

        return FALSE;
    }
}

/// <summary>
/// Handles the WM_CLOSE message.
/// </summary>
void Window::OnClose() noexcept
{
    GetWindowRect(&_Context._Bounds);

    _Context._PlugIn->HideGUI();

    EndDialog(0);
}

/// <summary>
/// Handles the WM_GETMINMAXINFO message.
/// </summary>
void Window::OnGetMinMaxInfo(LPMINMAXINFO mmi) const noexcept
{
    mmi->ptMinTrackSize.x = 64;
    mmi->ptMinTrackSize.y = 48;

    mmi->ptMaxTrackSize.x = _MinMaxBounds.Width();
    mmi->ptMaxTrackSize.y = _MinMaxBounds.Height();
}

/// <summary>
/// Gets the size of the host window.
/// </summary>
void Window::AdjustSize(RECT & wr) const noexcept
{
    LONG Style = GetWindowLongW(GWL_STYLE);

    ::AdjustWindowRect(&wr, (DWORD) Style, FALSE);

    wr.right  -= wr.left;
    wr.bottom -= wr.top;

    wr.left = wr.top = 0;
}

}
