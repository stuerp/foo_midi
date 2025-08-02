
/** $VER: CLAPWindow.h (2025.08.01) P. Stuer **/

#pragma once

#include <CppCoreCheck/Warnings.h>
#pragma warning(disable: 5045 ALL_CPPCORECHECK_WARNINGS)

#include <atlbase.h>
#include <atlapp.h>
#include <atlcrack.h>
#include <atlctrls.h>
#include <atlmisc.h>

#include <clap/clap.h>

#include "Resource.h"

#include "CLAPPlugIn.h"

#pragma hdrstop

namespace CLAP
{
class Host;

#pragma warning(disable: 4820) // x bytes padding added after data member

/// <summary>
/// Implements a parent window for non-floating CLAP GUI's.
/// </summary>
class Window : public CDialogImpl<Window>
{
public:
    Window() noexcept : m_bMsgHandled(false), _Context(), _MinMaxBounds() { }

    Window(const Window &) = delete;
    Window & operator=(const Window &) = delete;
    Window(Window &&) = delete;
    Window & operator=(Window &&) = delete;

    virtual ~Window() { }

    enum { IDD = IDD_CLAP_WINDOW };

    struct Context
    {
        const CLAP::Host * _Host;

        CRect _Bounds;
    };

    void AdjustSize(RECT & wr) const noexcept;

private:
    #pragma region CDialogImpl

    BOOL OnInitDialog(CWindow w, LPARAM lParam) noexcept;
    void OnClose() noexcept;

    void OnGetMinMaxInfo(LPMINMAXINFO mmi) const noexcept;

    /// <summary>
    /// Returns a brush that the system uses to draw the dialog background.
    /// </summary>
    HBRUSH OnCtlColorDlg(HDC, HWND) const noexcept
    {
        return (HBRUSH) NULL_BRUSH; // ::GetStockObject(DKGRAY_BRUSH);
    }

    BEGIN_MSG_MAP_EX(Window)
        MSG_WM_INITDIALOG(OnInitDialog)

        MSG_WM_CTLCOLORDLG(OnCtlColorDlg)
        MSG_WM_GETMINMAXINFO(OnGetMinMaxInfo)

        MSG_WM_CLOSE(OnClose)
    END_MSG_MAP()

    #pragma endregion

private:
    Context _Context;
    CRect _MinMaxBounds;
};

}
