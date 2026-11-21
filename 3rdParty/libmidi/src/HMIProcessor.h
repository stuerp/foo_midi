
/** $VER: HMIProcessor.h (2026.11.21) **/

#pragma once

#include "pch.h"

#include "MIDIProcessor.h"
#include "Exception.h"

#include <cstdint>
#include <vector>

namespace midi
{

namespace hmi
{

class processor_t : public midi::processor_t
{
public:
    static bool IsHMI(bytes_t const & data) noexcept;
    static bool Process(bytes_t const & data, container_t & container);

private:
    static void AddLoopMarker(container_t & container, uint32_t time, bool begin);
    static void DecodeHMIControlEvent(iterator_t & it, iterator_t tail, uint32_t runningTime, container_t & container);
};

}

}
