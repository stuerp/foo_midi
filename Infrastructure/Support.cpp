 
/** $VER: Support.cpp (2025.07.20) - Support functions **/

#include "pch.h"

/// <summary>
/// Returns true if the string matches one of the list.
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
/// Returns true if the string matches one of the list.
/// </summary>
bool IsOneOf(const char * item, const std::vector<const char *> & list) noexcept
{
    for (const auto & Item : list)
    {
        if (::_stricmp(item, Item) == 0)
            return true;
    }

    return false;
}
