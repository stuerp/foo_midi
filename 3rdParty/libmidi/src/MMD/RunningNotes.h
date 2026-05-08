
/** $VER: RunningNotes.h (2026.05.07) P. Stuer - Based on Valley Bell's mmd2mid (https://github.com/ValleyBell/MidiConverters). **/

#pragma once

#include <CppCoreCheck\Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 5045 ALL_CPPCORECHECK_WARNINGS)

#include <cstdint>

namespace mmd
{

class stream_t;

struct running_note_t
{
    uint8_t Channel;
    uint8_t Note;
    uint8_t Velocity;
    uint32_t Duration;
};

class running_notes_t
{
public:
    running_notes_t() : _Count() { }

    void Add(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t duration) noexcept;

    bool EmitNote(stream_t & stream, uint8_t note, uint8_t duration) noexcept;

    void Flush(stream_t & stream, uint32_t & deltaTime) noexcept;
    size_t Update(stream_t & stream, uint32_t & deltaTime) noexcept;

public:
    static const size_t MaxItems = 32;

    running_note_t _Items[MaxItems];
    size_t _Count;
};

}
