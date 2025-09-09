
/** $VER: Win32Exception.h (2025.09.01) P. Stuer **/

#pragma once

#define WIN32_LEAN_AND_MEAN

#include <sdkddkver.h>
#include <Windows.h>
#include <strsafe.h>

#include <stdexcept>
#include <string>

#include "Support.h"

namespace msc
{

class win32_exception : public std::runtime_error
{
public:
    win32_exception(const std::string & errorMessage) noexcept : std::runtime_error(GetErrorMessage(errorMessage, GetLastError())) {  }

    win32_exception(const std::string & errorMessage, DWORD errorCode) noexcept : std::runtime_error(GetErrorMessage(errorMessage, errorCode)) {  }
};

}
