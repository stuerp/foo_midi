
/** $VER: Window.cpp (2025.06.29) P. Stuer **/

#include "pch.h"

#include "CLAPWindow.h"
#include "CLAPHost.h"
#include "Encoding.h"

namespace CLAP
{

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL Window::OnInitDialog(CWindow w, LPARAM lParam) noexcept
{
    if (lParam == NULL)
        return FALSE;

    _Parameters = *(Parameters *) lParam;

    _MinMaxBounds = _Parameters._GUIBounds;

    AdjustSize(_MinMaxBounds);

    if (::IsRectEmpty(&_Parameters._Bounds))
        _Parameters._Bounds = _MinMaxBounds;

    MoveWindow(&_Parameters._Bounds);

    std::string Name = CLAP::Host::GetInstance().GetPlugInName();

    SetWindowTextW((std::wstring(L"CLAP Plug-in ") + ::UTF8ToWide(Name)).c_str());

    return TRUE;
}

/// <summary>
/// Handles the WM_CLOSE message.
/// </summary>
void Window::OnClose() noexcept
{
    GetWindowRect(&_Parameters._Bounds);

    DestroyWindow();
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
