
/** $VER: Support.h (2025.09.17) P. Stuer **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <Windows.h>

#include <math.h>

namespace msc
{

/// <summary>
/// Returns the input value clamped between min and max.
/// </summary>
template <class T>
inline static T Clamp(T value, T minValue, T maxValue)
{
    return std::min(std::max(value, minValue), maxValue);
}

/// <summary>
/// Returns true of the input value is in the interval between min and max.
/// </summary>
template <class T>
inline static T InRange(T value, T minValue, T maxValue)
{
    return (minValue <= value) && (value <= maxValue);
}

/// <summary>
/// Wraps around a value.
/// </summary>
template<class T>
inline static T Wrap(T value, T max)
{
    return (max + (value % max)) % max;
}

/// <summary>
/// Maps a value from one range (srcMin, srcMax) to another (dstMin, dstMax).
/// </summary>
template<class T, class U>
inline static U Map(T value, T srcMin, T srcMax, U dstMin, U dstMax)
{
    return dstMin + (U) (((double) (value - srcMin) * (double) (dstMax - dstMin)) / (double) (srcMax - srcMin));
}

/// <summary>
/// Returns true if the specified flags are set.
/// </summary>
template <class T>
inline static bool IsSet(T a, T b)
{
    return (a & b) == b;
}

/// <summary>
/// Gets the handle of the module that contains the executing code.
/// </summary>
inline HMODULE GetCurrentModule() noexcept
{
    HMODULE hModule = NULL;

    ::GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCWSTR) GetCurrentModule, &hModule);

    return hModule;
}
/// <summary>
/// Converts points to DIPs (Device Independent Pixels).
/// </summary>
inline static FLOAT ToDIPs(FLOAT points)
{
    return (points / 72.0f) * (FLOAT) USER_DEFAULT_SCREEN_DPI; // FIXME: Should 96.0 change on high DPI screens?
}

/// <summary>
/// Creates a unique file name with an optional file extension. The caller takes responsibility for deleting the file after use if necessary.
/// </summary>
class unique_path_t
{
public:
    unique_path_t() noexcept : unique_path_t(fs::path())
    {
    }

    unique_path_t(const fs::path & extension) noexcept
    {
        char TempPath[MAX_PATH] = {};

        if (::GetTempPathA(_countof(TempPath), TempPath) == 0)
            return;

        GUID guid;

        if (!SUCCEEDED(::CoCreateGuid(&guid)))
            return;

        char FileName[MAX_PATH];

        if (-1 == ::sprintf_s(FileName, sizeof(FileName), "%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]))
            return;

        _Path = fs::path(TempPath) / FileName;

        _Path.replace_extension(extension);
    }

    fs::path Path() const noexcept { return _Path; }

    bool IsEmpty() const noexcept { return _Path.empty(); }

private:
    fs::path _Path;
};

std::string GetErrorMessage(const std::string & errorMessage, DWORD errorCode, ...);
bool IsOneOf(const std::wstring & item, const std::vector<std::wstring> & list) noexcept;
bool IsOneOf(const char * item, const std::vector<const char *> & list) noexcept;

}
