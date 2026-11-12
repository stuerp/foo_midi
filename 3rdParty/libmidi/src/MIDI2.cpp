
/** $VER: MIDI2.cpp (2026.05.19) **/

#include "pch.h"

#include <MIDI2.h>

namespace midi2
{

/// <summary>
/// Initializes a new instance.
/// </summary>
ump_t::ump_t(const uint8_t * data, size_t size) noexcept
{
    Size = std::min(size, _countof(Data));

    ::memcpy(Data, data, Size);
}

/// <summary>
/// Gets the packet size (in bytes) for the specified message type.
/// </summary>
constexpr size_t ump_t::GetPacketSize(MessageType mt) noexcept
{
    const uint8_t Sizes[16] = { 4, 4, 4, 8, 8, 16, 0, 0, 0, 0, 0, 0, 0, 16, 0, 16 };

    return (size_t) Sizes[(size_t) mt];
}

/// <summary>
/// Gets the packets from the byte stream.
/// </summary>
std::vector<ump_t> ump_t::FromBytes(const uint8_t * data, size_t size) noexcept
{
    std::vector<ump_t> Packets;

    Packets.reserve(size / 8);

    while (size != 0)
    {
        const auto Type = (MessageType) (data[0] >> 4);

        size_t Size = ump_t::GetPacketSize(Type);

        Packets.push_back(ump_t(data, Size));

        data += Size;
        size -= Size;
    }

    return Packets;
}

}
