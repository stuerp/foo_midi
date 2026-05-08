
/** $VER: Exception.h (2025.03.20) P. Stuer **/

#pragma once

#include <Windows.h>
#include <strsafe.h>

#include <stdexcept>
#include <string>

namespace midi
{

class exception : public std::runtime_error
{
public:
    exception(const std::string & message) : std::runtime_error(message) { }
   
    exception(const char * message) : std::runtime_error(message) { }

    virtual ~exception() noexcept { }
};

}
