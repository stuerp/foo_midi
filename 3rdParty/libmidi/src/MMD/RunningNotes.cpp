
/** $VER: RunningNotes.cpp (2026.05.08) P. Stuer - Based on Valley Bell's mmd2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include <CppCoreCheck\Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include "RunningNotes.h"
#include "Stream.h"

#include <MIDI.h>

namespace mmd
{

/// <summary>
/// Adds a note event to the "running notes" list, so that Note Off events can be inserted automatically by Update() while processing delays.
/// "velocity" specifies the velocity for the Note Off event. A value of 0x80 results in Note On with velocity 0 (which is also interpreted as a Note Off).
/// "duration" specifies the number of ticks after which the note is turned off.
/// </summary>
void running_notes_t::Add(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t duration) noexcept
{
    if (_Count >= MaxItems)
        return;

    running_note_t & rn = _Items[_Count++];

    rn.Channel  = channel;
    rn.Note     = note;
    rn.Velocity = velocity;
    rn.Duration = duration;
}

/// <summary>
/// Returns true if a Note On message should be emitted.
/// </summary>
bool running_notes_t::EmitNote(stream_t & stream, uint8_t note, uint8_t duration) noexcept
{
    Update(stream);

    for (size_t i = 0; i < _Count; ++i)
    {
        if (_Items[i].Note == note)
        {
            // The note is already playing. Set its new duration.
            _Items[i].Duration = stream.DeltaTime + duration;

            return false; // Don't emit a new note.
        }
    }

    return true;
}

/// <summary>
/// Writes Note Off events for all running notes.
/// </summary>
void running_notes_t::Flush(stream_t & stream) noexcept
{
    // Set deltaTime to the longest note duration.
    for (size_t i = 0; i < _Count; ++i)
    {
        if (_Items[i].Duration > stream.DeltaTime)
            stream.DeltaTime = _Items[i].Duration;
    }

    Update(stream);
}

/// <summary>
/// Checks if any note expires within the N ticks specified by the "DeltaTime" parameter of the stream and
/// insert Note Off events when they do. In that case, the value of "deltaTime" will be reduced.
/// Call this function from the delta time handler and before extending notes.
/// Returns the number of expired notes.
/// </summary>
size_t running_notes_t::Update(stream_t & stream) noexcept
{
    size_t ExpiredNotes = 0;

    while (_Count > 0)
    {
        // 1. Check if we're going beyond a note's timeout.
        uint32_t NewDeltaTime = stream.DeltaTime + 1;

        for (size_t i = 0; i < _Count; ++i)
        {
            running_note_t * tn = &_Items[i];

            if (tn->Duration < NewDeltaTime)
                NewDeltaTime = tn->Duration;
        }

        if (NewDeltaTime > stream.DeltaTime)
            break; // No expired notes found.

        // 2. Reduce the remaining length of each note.
        for (size_t i = 0; i < _Count; ++i)
            _Items[i].Duration -= NewDeltaTime;

        stream.DeltaTime -= NewDeltaTime;

        // 3. Add a Note Off event for expired notes.
        for (size_t i = 0; i < _Count; ++i)
        {
            running_note_t & rn = _Items[i];

            if (rn.Duration != 0)
                continue;

            {
                stream.WriteVariableLengthQuantity(NewDeltaTime);
                NewDeltaTime = 0; // Any other expired note will get a delta time of 0.

                stream.Grow(3u);

                if (rn.Velocity != 0x80)
                {
                    stream.Data[stream.Offset++] = (uint8_t) (midi::StatusCode::NoteOff | rn.Channel);
                    stream.Data[stream.Offset++] = rn.Note;
                    stream.Data[stream.Offset++] = rn.Velocity;
                }
                else
                {
                    stream.Data[stream.Offset++] = (uint8_t) (midi::StatusCode::NoteOn | rn.Channel);
                    stream.Data[stream.Offset++] = rn.Note;
                    stream.Data[stream.Offset++] = 0u;
                }
            }

            ExpiredNotes++;

            // Remove the expired note from the array.
            if (--_Count == 0)
                break;

            ::memmove(&rn, &_Items[i + 1], (_Count - i) * sizeof(_Items[0]));
            i--;
        }
    }

    return ExpiredNotes;
}

}
