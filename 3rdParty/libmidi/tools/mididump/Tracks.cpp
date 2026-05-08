
/** $VER: Tracks.cpp (2025.09.10) P. Stuer **/

#include "pch.h"

#include "MIDIProcessor.h"

#include "Messages.h"
#include "SysEx.h"
#include "Tables.h"

using namespace midi;

/// <summary>
/// Processes a metadata message.
/// </summary>
static void ProcessMetaData(const midi::event_t & me) noexcept
{
    ::printf("Meta Data                    ");

    switch (me.Data[1])
    {
        case midi::MetaDataType::SequenceNumber:
        {
            ::printf(" Sequence Number");
            break;
        }

        case midi::MetaDataType::Text:
        {
            ::printf(" Text \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::Copyright:
        {
            ::printf(" Copyright \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::TrackName:
        {
            ::printf(" Track Name \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::InstrumentName:
        {
            ::printf(" Instrument Name \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::Lyrics:
        {
            ::printf(" Lyrics \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::Marker:
        {
            ::printf(" Marker \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::CueMarker:
        {
            ::printf(" Cue Marker \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::DeviceName:
        {
            ::printf(" Device Name \"%s\"", (me.Data.size() > 2) ? msc::TextToUTF8((const char *) me.Data.data() + 2, me.Data.size() - 2).c_str() : "");
            break;
        }

        case midi::MetaDataType::ChannelPrefix:
        {
            ::printf(" Channel Prefix");
            break;
        }

        case midi::MetaDataType::MIDIPort:
        {
            ::printf(" Set MIDI Port %d", me.Data[2]);
            break;
        }

        case midi::MetaDataType::EndOfTrack:
        {
            ::printf(" End of Track");
            break;
        }

        case midi::MetaDataType::SetTempo:
        {
            const uint32_t Tempo = ((uint32_t) me.Data[2] << 16) | ((uint32_t) me.Data[3] << 8) | (uint32_t) me.Data[4];

            ::printf(" Set Tempo (%d μs/quarter note, %d bpm)", Tempo, (int) ((60 * 1000 * 1000) / Tempo));
            break;
        }

        case midi::MetaDataType::SMPTEOffset:
        {
            ::printf(" Set SMPTE Offset");
            break;
        }

        case midi::MetaDataType::TimeSignature:
        {
            ::printf(" Time Signature %d/%d, %d ticks per beat, %d 32nd notes per MIDI quarter note", me.Data[2], 1 << me.Data[3], me.Data[4], me.Data[5]);
            break;
        }

        case midi::MetaDataType::KeySignature:
        {
            static const char * MajorScales[] =
            {
                "Cb", "Db", "Eb", "Fb", "Gb", "Ab", "Bb",   // 7 flats
                "Gb", "Ab", "Bb", "Cb", "Db", "Eb", "F",    // 6 flats
                "Db", "Eb", "F",  "Gb", "Ab", "Bb", "C",    // 5 flats
                "Ab", "Bb", "C",  "Db", "Eb", "F",  "G",    // 4 flats
                "Eb", "F",  "G",  "Ab", "Bb", "C",  "D",    // 3 flats
                "Bb", "C",  "D",  "Eb", "F",  "G",  "A",    // 2 flats
                "F",  "G",  "A",  "Bb", "C",  "D",  "E",    // 1 flat

                "C",  "D",  "E",  "F",  "G",  "A",  "B",    // No flats or sharps

                "G",  "A",  "B",  "C",  "D",  "E",  "F#",   // 1 sharp
                "D",  "E",  "F#", "G",  "A",  "B",  "C#",   // 2 sharps
                "A",  "B",  "C#", "D",  "E",  "F#", "G#",   // 3 sharps
                "E",  "F#", "G#", "A",  "B",  "C#", "D#",   // 4 sharps
                "B",  "C#", "D#", "E",  "F#", "G#", "A#",   // 5 sharps
                "F#", "G#", "A#", "B",  "C#", "D#", "E#",   // 6 sharps
                "C#", "D#", "E#", "F#", "G#", "A#", "B#",   // 7 sharps
            };

            static const char * MinorScales[] =
            {
                "Ab", "Bb", "Cb", "Db", "Eb", "Fb", "Gb",   // 7 flats
                "Eb", "F",  "Gb", "Ab", "Bb", "Cb", "Db",   // 6 flats
                "Bb", "C",  "Db", "Eb", "F",  "Gb", "Ab",   // 5 flats
                "F",  "G",  "Ab", "Bb", "C",  "Db", "Eb",   // 4 flats
                "C",  "D",  "Eb", "F",  "G",  "Ab", "Bb",   // 3 flats
                "G",  "A",  "Bb", "C",  "D",  "Eb", "F",    // 2 flats
                "D",  "E",  "F",  "G",  "A",  "Bb", "C",    // 1 flat

                "A",  "B",  "C",  "D",  "E",  "F",  "G",    // No flats or sharps

                "E",  "F#", "G",  "A",  "B",  "C",  "D",    // 1 sharp
                "B",  "C#", "D",  "E",  "F#", "G",  "A",    // 2 sharps
                "F#", "G#", "A",  "B",  "C#", "D",  "E",    // 3 sharps
                "C#", "D#", "E",  "F#", "G#", "A",  "B",    // 4 sharps
                "G#", "A#", "B",  "C#", "D#", "E",  "F#",   // 5 sharps
                "D#", "E#", "F#", "G#", "A#", "B",  "C#",   // 6 sharps
                "A#", "B#", "C#", "D#", "E#", "F#", "G#",   // 7 sharps
            };

            ::printf(" Key Signature %s %s", (me.Data[3] == 0) ? MajorScales[49 + (((char) me.Data[2]) * 7)] : MinorScales[49 + (((char) me.Data[2]) * 7)], (me.Data[3] == 0) ? "major" : "minor");
            break;
        }

        case midi::MetaDataType::SequencerSpecific:
        {
            ::printf(" Sequencer Specific (%d bytes)", (int) me.Data.size() - 2);
            break;
        }

        default:
            break;
    }
}

/// <summary>
/// Processes a SysEx message.
/// </summary>
static void ProcessSysEx(const midi::event_t & me) noexcept
{
    sysex_t SysEx(me.Data);

    SysEx.Identify();

    ::printf(" \"%s\", \"%s\"", SysEx.Manufacturer.c_str(), SysEx.Description.c_str());
}

/// <summary>
/// Processes MIDI events.
/// </summary>
static uint32_t ProcessEvent(const midi::event_t & event, uint32_t eventTimeInMS, uint32_t time, size_t index) noexcept
{
    // Output the header.
    {
        char Display1[16];
        char Display2[16];
        char Display3[16];

        if (event.Time!= time)
        {
            ::_snprintf_s(Display1, _countof(Display1), "%8u ticks",  event.Time);
            ::_snprintf_s(Display2, _countof(Display2), "%8.2fs", (double) eventTimeInMS / 1000.);

            const uint32_t t = eventTimeInMS / 1000;

            ::_snprintf_s(Display3, _countof(Display3), "%02d:%02d:%02d", t / 3600, (t % 3600) / 60, t % 60);
        }
        else
            Display1[0] = Display2[0] = Display3[0] = '\0';

        if (event.Type != midi::event_t::event_type_t::Extended)
            ::printf("%8d %-14s %-10s  %-8s  (%2d) ", (int) index, Display1, Display2, Display3, event.ChannelNumber + 1);
        else
            ::printf("%8d %-14s %-10s  %-8s       ", (int) index, Display1, Display2, Display3);
    }

    if (event.Type != midi::event_t::event_type_t::Extended)
        ::printf(" %02X", (event.Type + 8) << 4);

    const int ByteCount = 16;

    int i = 1;

    for (const auto & d : event.Data)
    {
        if (i++ < ByteCount)
            ::printf(" %02X", d);
        else
        {
            ::printf(" ..");
            break;
        }
    }

    if (event.Type == midi::event_t::event_type_t::Extended)
        ::printf("   ");

    ::printf("%*.s", std::max(0, (ByteCount - (int) event.Data.size()) * 3), "");

    ::putchar(' ');

    switch (event.Type)
    {
        case midi::event_t::event_type_t::NoteOff:
        {
            ::printf("Note Off                      Note %3d, Velocity %3d", event.Data[0], event.Data[1]);
            break;
        }

        case midi::event_t::event_type_t::NoteOn:
        {
            ::printf("Note On                       Note %3d, Velocity %3d", event.Data[0], event.Data[1]);
            break;
        }

        case midi::event_t::event_type_t::KeyPressure:
        {
            ::printf("Key Pressure                  A0");
            break;
        }

        case midi::event_t::event_type_t::ControlChange:
        {
            ::printf("Control Change %3d            %s", event.Data[0], DescribeControlChange(event.Data[0], event.Data[1]).c_str()); break;
        }

        case midi::event_t::event_type_t::ProgramChange:
        {
            ::printf("Program Change %3d            \"%s\"", event.Data[0], Instruments[event.Data[0]]); break;
            break;
        }

        case midi::event_t::event_type_t::ChannelPressure:
        {
            ::printf("Channel Pressure              D0");
            break;
        }

        case midi::event_t::event_type_t::PitchBendChange:
        {
            ::printf("Pitch Bend Change             %d", 8192 - ((int) (event.Data[1] & 0x7F) << 7) | (event.Data[0] & 0x7F));
            break;
        }

        case midi::event_t::event_type_t::Extended:
        {
            switch (event.Data[0])
            {
                case midi::SysEx:                ::printf("SysEx                        "); ProcessSysEx(event); break;

                case midi::MIDITimeCodeQtrFrame: ::printf("MIDI Time Code Qtr Frame     "); break;
                case midi::SongPositionPointer:  ::printf("Song PositionPointer         "); break;
                case midi::SongSelect:           ::printf("Song Select                  "); break;

                case midi::TuneRequest:          ::printf("Tune Request                 "); break;
                case midi::SysExEnd:             ::printf("SysEx End                    "); break;
                case midi::TimingClock:          ::printf("Timing Clock                 "); break;

                case midi::Start:                ::printf("Start                        "); break;
                case midi::Continue:             ::printf("Continue                     "); break;
                case midi::Stop:                 ::printf("Stop                         "); break;

                case midi::ActiveSensing:        ::printf("Active Sensing               "); break;
                case midi::MetaData:             ProcessMetaData(event); break;

                default:                        ::printf("Unknown event type %02X      ", event.Data[0]); break;
            }

            break;
        }
    }

    ::putchar('\n');

    return event.Time;
}

/// <summary>
/// Processes all tracks.
/// </summary>
void ProcessTracks(const midi::container_t & container)
{
    const uint32_t SubsongIndex = 0;

    uint32_t TrackIndex = 0;

    for (const auto & Track : container)
    {
        uint32_t ChannelCount = container.GetChannelCount(SubsongIndex);
        uint32_t Duration = container.GetDuration(SubsongIndex, false);
        uint32_t DurationInMS = container.GetDuration(SubsongIndex, true);

        ::printf("\nTrack %2d: %d channels, %8d ticks, %8.2fs\n", TrackIndex + 1, ChannelCount, Duration, (float) DurationInMS / 1000.0f);
        ::puts("Index    | Ticks        | Time    | Time     | Ch | Data");

        uint32_t Time = std::numeric_limits<uint32_t>::max();
        size_t i = 0;

        for (const auto & Event : Track)
            Time = ProcessEvent(Event, container.TimestampToMS(Event.Time, SubsongIndex), Time, i++);

        ++TrackIndex;
    }
}
