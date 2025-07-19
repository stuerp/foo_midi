 
/** $VER: Support.cpp (2025.07.13) - Support functions **/

#include "pch.h"

/// <summary>
/// Returns true if the string matches on of the list.
/// </summary>
bool IsOneOf(const std::wstring & item, const std::vector<std::wstring> & list) noexcept
{
    for (const auto & Item : list)
    {
        if (::_wcsicmp(item.c_str(), Item.c_str()) == 0)
            return true;
    }

    return false;
}

/// <summary>
/// Returns true if the string matches on of the list.
/// </summary>
bool IsOneOf(const std::u8string & item, const std::vector<std::u8string> & list) noexcept
{
    for (const auto & Item : list)
    {
        if (::_stricmp((const char *) item.c_str(), (const char *) Item.c_str()) == 0)
            return true;
    }

    return false;
}
