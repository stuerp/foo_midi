
/** $VER: Exception.h (2025.07.01) P. Stuer **/

#pragma once

#include "Resource.h"

namespace component
{

class runtime_error : public std::runtime_error
{
public:
    explicit runtime_error(const std::string & message) : std::runtime_error(message) { }

    explicit runtime_error(const char * message) : std::runtime_error(message) { }

    virtual ~runtime_error() noexcept { }
};

std::string GetErrorMessage(const std::string & errorMessage, DWORD errorCode, ...);

}
