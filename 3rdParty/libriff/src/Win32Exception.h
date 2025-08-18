
/** $VER: Win32Exception.h (2025.03.20) P. Stuer **/

#pragma once

#include <Windows.h>
#include <strsafe.h>

#include <stdexcept>
#include <string>

namespace riff
{

std::string GetErrorMessage(const std::string & errorMessage, DWORD errorCode, ...);

class win32_exception : public std::runtime_error
{
public:
    win32_exception(const std::string & errorMessage) noexcept : std::runtime_error(GetErrorMessage(errorMessage, GetLastError())) {  }

    win32_exception(const std::string & errorMessage, DWORD errorCode) noexcept : std::runtime_error(GetErrorMessage(errorMessage, errorCode)) {  }
};

}
