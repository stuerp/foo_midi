
/** $VER: String.h (2025.09.01) P. Stuer - String support routines **/

#pragma once

#include <string>
#include <ranges>
#include <algorithm>
#include <cctype>

namespace msc
{
/// <summary>
/// Returns true if the strings are equal using a case-insenstive comparison.
/// </summary>
inline bool IsEqual(const std::string & a, const std::string & b) noexcept
{
    return std::ranges::equal
    (
        a, b, {},
        [](char ch)
        {
            return std::tolower(static_cast<unsigned char>(ch));
        },
        [](char ch)
        {
            return std::tolower(static_cast<unsigned char>(ch));
        }
    );
}

/// <summary>
/// Returns true if the first string is lexicographical less than the second.
/// </summary>
inline bool IsLess(const std::string & a, const std::string & b) noexcept
{
    auto ToLower = [](char c)
    {
        return std::tolower(static_cast<unsigned char>(c));
    };

    return std::ranges::lexicographical_compare(a | std::views::transform(ToLower), b | std::views::transform(ToLower));
}

}
