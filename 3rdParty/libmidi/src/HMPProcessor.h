
/** $VER: HMPProcessor.h (2026.11.21) **/

#pragma once

#include "pch.h"

#include "MIDIProcessor.h"
#include "Exception.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace midi
{

namespace hmp
{

class processor_t : public midi::processor_t
{
public:
    static bool IsHMP(bytes_t const & data) noexcept;
    static bool Process(bytes_t const & data, container_t & container);

private:
    static void DecodeTrack(iterator_t begin, iterator_t end, track_t & track);
    static void AddExtendedEvent(track_t & track, uint32_t time, uint8_t status, uint8_t type, iterator_t payload, uint32_t payloadSize);
    static void AddTempoTrack(container_t & container, uint32_t Tempo);

    /// <summary>
    /// Decodes an HMP variable-length quantity.
    /// </summary>
    static uint32_t DecodeVariableLengthQuantity(iterator_t & begin, iterator_t end) noexcept
    {
        uint32_t Value = 0;

        if (!DecodeVariableLengthQuantity(begin, end, Value))
            return 0;

        return Value;
    }

    /// <summary>
    /// Decodes an HMP variable-length quantity.
    /// </summary>
    static bool DecodeVariableLengthQuantity(iterator_t & it, iterator_t end, uint32_t & value) noexcept
    {
        value = 0;

        uint32_t Shift = 0;

        // A 32-bit value needs at most five 7-bit groups. HMP stores the least-significant group first and sets bit 7 on the final byte.
        for (uint32_t Count = 0; Count < 5; ++Count)
        {
            if (it == end)
                return false;

            const uint8_t Byte = *it++;
            const uint32_t Payload = Byte & 0x7Fu;

            if (Shift >= 32 || Payload > (std::numeric_limits<uint32_t>::max() >> Shift))
                return false;

            value |= Payload << Shift;

            if ((Byte & 0x80u) != 0)
                return true;

            Shift += 7;
        }

        return false;
    }
};

}

}
