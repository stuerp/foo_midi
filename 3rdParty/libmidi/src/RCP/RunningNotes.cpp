
/** $VER: RunningNotes.cpp (2025.03.21) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include <CppCoreCheck\Warnings.h>

#pragma warning(disable: 4100 4625 4626 4710 4711 4738 4820 5045 ALL_CPPCORECHECK_WARNINGS)

#include "RunningNotes.h"

#include <MIDI.h>

namespace rcp
{

/// <summary>
/// Adds a note event to the "running notes" list, so that Note Off events can be inserted automatically by Check() while processing delays.
/// "length" specifies the number of ticks after which the note is turned off.
/// "velocity" specifies the velocity for the Note Off event. A value of 0x80 results in Note On with velocity 0.
/// Returns a pointer to the inserted struct or NULL if (NoteCnt >= NoteMax).
/// </summary>
void running_notes_t::Add(uint8_t channel, uint8_t note, uint8_t velocity, uint32_t duration)
{
    if (_Count >= MaxItems)
        return;

    running_note_t & rn = _Notes[_Count++];

    rn.Channel  = channel;
    rn.Code     = note;
    rn.Velocity = velocity;
    rn.Duration = duration;
}

// Checks, if any note expires within the N ticks specified by the "duration" parameter and
// insert respective Note Off events. In that case "duration" will be reduced.
// Call this function from the delay handler and before extending notes.
// Returns the number of expired notes.
size_t running_notes_t::Update(midi_stream_t & stream, uint32_t & duration)
{
    size_t ExpiredNotes = 0;

    while (_Count > 0)
    {
        uint32_t NewDuration = duration + 1;

        // 1. Check if we're going beyond a note's timeout.
        for (size_t i = 0; i < _Count; ++i)
        {
            running_note_t & n = _Notes[i];

            if (n.Duration < NewDuration)
                NewDuration = n.Duration;
        }

        if (NewDuration > duration)
            break; // The note is still playing. Continue processing the event.

        // 2. Advance all notes by X ticks.
        for (size_t i = 0; i < _Count; ++i)
            _Notes[i].Duration -= NewDuration;

        duration -= NewDuration;

        // 3. Send NoteOff for expired notes.
        for (size_t i = 0; i < _Count; ++i)
        {
            running_note_t & rn = _Notes[i];

            if (rn.Duration > 0)
                continue;

            {
                stream.WriteVariableLengthQuantity(NewDuration);

                NewDuration = 0;

                stream.Ensure(3);

                if (rn.Velocity < 0x80)
                {
                    stream.Add((uint8_t) (midi::NoteOff | rn.Channel));
                    stream.Add(rn.Code);
                    stream.Add(rn.Velocity);
                }
                else
                {
                    stream.Add((uint8_t) (midi::NoteOn | rn.Channel));
                    stream.Add(rn.Code);
                    stream.Add(0);
                }
            }

            ExpiredNotes++;

            if (--_Count == 0)
                break;

            ::memmove(&rn, &_Notes[(size_t) i + 1], ((size_t) _Count - i) * sizeof(running_note_t));
            i--;
        }
    }

    return ExpiredNotes;
}

// Writes Note Off events for all running notes.
// "cutNotes" = false -> all notes are played fully (even if "delay" is smaller than the longest note)
// "cutNotes" = true -> notes playing after "delay" ticks are cut there
uint32_t running_notes_t::Flush(midi_stream_t & stream, bool shorten)
{
    uint32_t Duration = stream.GetDuration();

    for (uint16_t i = 0; i < _Count; ++i)
    {
        if (_Notes[i].Duration > Duration)
        {
            if (shorten)
                _Notes[i].Duration = Duration; // Cut all notes at timestamp.
            else
                Duration = _Notes[i].Duration; // Remember the highest timestamp.
        }
    }

    Update(stream, Duration);

    return Duration;
}

}
