
/** $VER: Win32.cpp (2026.07.22) P. Stuer - Win32 routines **/

#include "pch.h"

#include "Win32.h"

#pragma comment(lib, "comdlg32")

namespace msc
{

/// <summary>
/// Converts a GUID to a wide string.
/// </summary>
std::wstring GUIDToWide(const GUID & id) noexcept
{
    OLECHAR Text[39] = { }; // {00000000-0000-0000-0000-000000000000}\0

    (void) ::StringFromGUID2(id, Text, (int) _countof(Text));

    return std::wstring(Text);
}

/// <summary>
/// Converts a wide string to a GUID.
/// </summary>
GUID WideToGUID(const std::wstring & text) noexcept
{
    GUID Id;

    HRESULT hResult = ::IIDFromString(text.c_str(), &Id);

    if (!SUCCEEDED(hResult))
        return GUID();

    return Id;
}

/// <summary>
/// Returns the fully-expanded file path of the specified file.
/// </summary>
std::wstring GetFullPath(const std::wstring & fileName) noexcept
{
    std::vector<wchar_t> Text(32768);

    const DWORD Length = ::SearchPathW(nullptr, fileName.c_str(), nullptr, (DWORD) Text.size(), Text.data(), nullptr);

    if ((Length == 0) || (Length >= Text.size()))
        return L"";

    return std::wstring(Text.data(), Length);
}

/// <summary>
/// Displays a dialog to select a file for opening.
/// </summary>
bool OpenFileDialog(HWND hParent, const std::vector<std::pair<std::wstring_view, std::wstring_view>> & filters, uint32_t selectedFilter, const std::wstring_view & defaultExtension, const std::wstring_view & title, const std::wstring_view & directoryPath, std::wstring & filePath) noexcept
{
    std::wstring FilterText;

    // Construct the filter text.
    {
        for (auto Filter : filters)
        {
            FilterText.append(Filter.first);
            FilterText.push_back('\0');
            FilterText.append(Filter.second);
            FilterText.push_back('\0');
        }

        FilterText.push_back('\0');
        FilterText.push_back('\0');
    }

    OPENFILENAMEW ofn =
    {
        .lStructSize     = sizeof(ofn),
        .hwndOwner       = hParent,
        .lpstrFilter     = FilterText.c_str(),
        .nFilterIndex    = (DWORD) selectedFilter,
        .lpstrFile       = (LPWSTR) filePath.data(),
        .nMaxFile        = (DWORD) filePath.capacity(),
        .lpstrInitialDir = directoryPath.data(),
        .lpstrTitle      = title.data(),
        .Flags           = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_LONGNAMES | OFN_EXPLORER | OFN_ENABLESIZING,
        .lpstrDefExt     = defaultExtension.data(),
    };

    if (!::GetOpenFileNameW(&ofn))
        return false;

    filePath = ofn.lpstrFile;

    return true;
}

}
