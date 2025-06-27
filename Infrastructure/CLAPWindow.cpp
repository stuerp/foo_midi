
/** $VER: CLAPWindow.cpp (2025.06.27) P. Stuer **/

#include "pch.h"

#include "CLAPWindow.h"

//CAppModule _Module;

/// <summary>
/// Initializes the dialog.
/// </summary>
BOOL CLAPWindow::OnInitDialog(CWindow w, LPARAM lParam)
{
    if (lParam != NULL)
    {
        _Parameters = *(DialogParameters *) lParam;

        if (IsRectEmpty(&_Parameters._Bounds))
        {
            _Parameters._Bounds =
            {
                .right  = 320,
                .bottom = 200
            };

            //::MapDialogRect(m_hWnd, &_Parameters._Bounds);
            MapDialogRect(&_Parameters._Bounds);
        }

        MoveWindow(&_Parameters._Bounds);
    }

    return TRUE;
}
