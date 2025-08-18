
/** $VER: Win32Exception.cpp (2025.03.20) P. Stuer **/

#include "pch.h"

#include "Win32Exception.h"

#include "Encoding.h"

#include <string.h>
#include <stdarg.h>

namespace riff
{

const char * strrstr(const char * __restrict s1, const char *__restrict s2);

/// <summary>
/// Gets the error message of the specified error code.
/// </summary>
std::string GetErrorMessage(const std::string & errorMessage, DWORD errorCode, ...)
{
    va_list args;

    va_start(args, errorCode);

    std::string Text;

    Text.resize(256);

    DWORD Result = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, errorCode, 0, Text.data(), (DWORD) Text.size(), &args);

    va_end(args);

    if (Result != 0)
    {
        // Remove a trailing "\r\n".
        char * p = (char *) strrstr(Text.c_str(), "\r\n");

        if (p != nullptr)
            *p = '\0';

        // Remove a trailing period ('.').
        p = (char *) ::strrchr(Text.c_str(), '.');

        if (p != nullptr)
            *p = '\0';

        return FormatText("%s: %s (0x%08X)", errorMessage.c_str(), Text.c_str(), errorCode);
    }
    else
    {
        Result = ::FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, ::GetLastError(), 0, Text.data(), (DWORD) Text.size(), nullptr);

        return FormatText("%s: Unknown error (0x%08X): %s", errorMessage.c_str(), errorCode, Text.c_str());
    }
}

/// <summary>
/// Returns a pointer to the last occurance of a string.
/// </summary>
const char * strrstr(const char * __restrict s1, const char *__restrict s2)
{
    const size_t l1 = ::strlen(s1);
    const size_t l2 = ::strlen(s2);

    if (l2 > l1)
        return nullptr;

    for (const char * s = s1 + (l1 - l2); s >= s1; --s)
        if (::strncmp(s, s2, l2) == 0)
            return s;

    return nullptr;
}

};
