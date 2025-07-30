
/** $VER: Window.cpp (2025.06.30) P. Stuer **/

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

    _Parameters._Host->GetPreferredGUISize(_MinMaxBounds);

    AdjustSize(_MinMaxBounds);

    if (::IsRectEmpty(&_Parameters._Bounds))
        _Parameters._Bounds = _MinMaxBounds;

    MoveWindow(&_Parameters._Bounds);

    SetWindowTextW((std::wstring(L"CLAP Plug-in ") + ::UTF8ToWide(_Parameters.Name)).c_str());

    _Parameters._Host->ShowGUI(m_hWnd);

    return TRUE;
}

/// <summary>
/// Handles the WM_CLOSE message.
/// </summary>
void Window::OnClose() noexcept
{
    GetWindowRect(&_Parameters._Bounds);

    _Parameters._Host->HideGUI();

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
/*
// FIXME: GUI support not implemented yet
DWORD WINAPI Window::DialogThreadProc(Host * host)
{
    CLAP::Window dlg;

    CLAP::Window::Parameters dp =
    {
        ._Bounds = { },
        ._FilePath = "", //_FilePath,
        ._Index = 0, //_Index,
        ._GUIBounds = { },
    };

    host->GetGUISize(dp._GUIBounds);

    HWND hWnd = dlg.Create(GetDesktopWindow(), (LPARAM) &dp);

    if (hWnd != NULL)
    {
        clap_window_t Window = { .api = "win32", .win32 = hWnd };

        if (host->SetWindow(&Window))
        {
            ShowWindow(SW_SHOW);

            // Message pump
            MSG msg;

            while (GetMessage(&msg, NULL, 0, 0))
            {
//              if (!IsDialogMessage(hWnd, &msg))
                {
                    ::TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }

    return 0;
}
*/
}
