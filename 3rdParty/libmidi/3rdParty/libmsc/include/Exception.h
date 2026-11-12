
/** $VER: Exception.h (2025.09.06) P. Stuer **/

#pragma once

#include <Windows.h>
#include <strsafe.h>

#include <stdexcept>
#include <string>

namespace msc
{

class exception : public std::runtime_error
{
public:
    exception(const std::string & message) : std::runtime_error(message) { }
   
    exception(const char * message) : std::runtime_error(message) { }

    virtual ~exception() noexcept { }
};

}
