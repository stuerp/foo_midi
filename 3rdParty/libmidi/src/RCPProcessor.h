
/** $VER: RCPProcessor.h (2026.11.23) **/

#pragma once

#include "pch.h"

#include "MIDIProcessor.h"
#include "Exception.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace rcp
{
using namespace midi;

class processor_t : public midi::processor_t
{
public:
    static bool IsRCP(bytes_t const & data, const std::wstring & fileExtension) noexcept;
    static bool Process(bytes_t const & data, const std::wstring & filePath, container_t & container);

private:
};

}
