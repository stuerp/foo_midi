
/** $VER: Enum.h (2025.09.17) P. Stuer **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <Windows.h>

#include <type_traits>

template<typename Enum> concept Flags = std::is_enum_v<Enum>;

template<Flags Enum> constexpr Enum operator |(Enum lhs, Enum rhs)
{
    using underlying_t = std::underlying_type_t<Enum>;

    return (Enum) ((underlying_t) lhs | (underlying_t) rhs);
}

template<Flags Enum> constexpr Enum & operator |=(Enum & lhs, Enum rhs)
{
    lhs = lhs | rhs;

    return lhs;
}

template<Flags Enum> constexpr Enum operator &(Enum lhs, Enum rhs)
{
    using underlying_t = std::underlying_type_t<Enum>;

    return (Enum) ((underlying_t) lhs & (underlying_t) rhs);
}

template<Flags Enum> constexpr Enum & operator &=(Enum & lhs, Enum rhs)
{
    lhs = lhs & rhs;

    return lhs;
}

template<Flags Enum> constexpr Enum operator ~(Enum value)
{
    using underlying_t = std::underlying_type_t<Enum>;

    return (Enum) ~(underlying_t) value;
}

/// <summary>
/// Sets the  specified flags.
/// </summary>
template<Flags Enum> constexpr Enum Set(Enum & value, Enum flags)
{
    value = value | flags;

    return value;
}

/// <summary>
/// Unsets the  specified flags.
/// </summary>
template<Flags Enum> constexpr Enum UnSet(Enum & value, Enum flags)
{
    value = value & ~flags;

    return value;
}

/// <summary>
/// Returns true when all the specified flags are set.
/// </summary>
template<Flags Enum> bool IsSet(Enum value, Enum flags)
{
    using underlying_t = std::underlying_type_t<Enum>;

    return ((underlying_t) value & (underlying_t) flags) == (underlying_t) flags;
}
