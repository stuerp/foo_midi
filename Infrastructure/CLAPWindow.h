
/** $VER: CLAPWindow.h (2025.06.27) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include "Resource.h"

#pragma hdrstop

#pragma warning(disable: 4820) // x bytes padding added after data member

struct DialogParameters
{
    HWND _hWnd;
    RECT _Bounds;
};

/// <summary>
/// Implements a parent window for non-floating CLAP GUI's.
/// </summary>
class CLAPWindow : public CDialogImpl<CLAPWindow>
{
public:
    CLAPWindow() noexcept : m_bMsgHandled(false), _Parameters() { }

    CLAPWindow(const CLAPWindow &) = delete;
    CLAPWindow & operator=(const CLAPWindow &) = delete;
    CLAPWindow(CLAPWindow &&) = delete;
    CLAPWindow & operator=(CLAPWindow &&) = delete;

    virtual ~CLAPWindow() { }

    enum { IDD = IDD_CLAP_WINDOW };

private:
    #pragma region CDialogImpl
    BOOL OnInitDialog(CWindow w, LPARAM lParam);

    /// <summary>
    /// Handles the WM_CLOSE message.
    /// </summary>
    void OnClose()
    {
        GetWindowRect(&_Parameters._Bounds);

        DestroyWindow();
    }

#ifdef _DEBUG
    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background. For layout debugging purposes.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND) const noexcept
    {
        return (HBRUSH) ::GetStockObject(DKGRAY_BRUSH);
    }
#endif

    BEGIN_MSG_MAP_EX(CLAPWindow)
        MSG_WM_INITDIALOG(OnInitDialog)
        MSG_WM_CLOSE(OnClose)
    END_MSG_MAP()

private:
    DialogParameters _Parameters;
};
