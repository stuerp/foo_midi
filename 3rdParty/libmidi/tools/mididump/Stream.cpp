
/** $VER: Stream.cpp (2026.04.10) P. Stuer **/

#include "pch.h"

#include "MIDI.h"
#include "MIDIContainer.h"

#include "Messages.h"
#include "SysEx.h"

using namespace midi;

static uint8_t CCMSB = 255; // Control Change Most Significant Byte
static uint8_t CCLSB = 255; // Control Change Least Significant Byte

static uint8_t DELSB = 0;

static uint32_t ProcessEvent(const midi::message_t & message, uint32_t messageTimeInMS, uint32_t timestamp, size_t index, const midi::sysex_table_t & sysExMap);

/// <summary>
/// Processes the stream.
/// </summary>
void ProcessStream(const midi::container_t & container, const std::vector<midi::message_t> & stream, const midi::sysex_table_t & sysExMap, const std::vector<uint8_t> & portNumbers, bool skipNormalEvents)
{
    const uint32_t SubsongIndex = 0;

    ::printf("%u messages, %u unique SysEx messages, %u ports\n", (uint32_t) stream.size(), (uint32_t) sysExMap.Size(), (uint32_t) portNumbers.size());

    uint32_t Time = std::numeric_limits<uint32_t>::max();
    size_t i = 0;

    for (const auto & m : stream)
    {
        // Skip all normal MIDI events.
        if (skipNormalEvents && !m.IsSysEx())
            continue;

        Time = ProcessEvent(m, container.TimestampToMS(m.Time, SubsongIndex), Time, i++, sysExMap);
    }
}

/// <summary>
/// Processes MIDI stream events.
/// </summary>
static uint32_t ProcessEvent(const midi::message_t & message, uint32_t messageTimeInMS, uint32_t time, size_t index, const midi::sysex_table_t & sysExMap)
{
    // Output the header.
    {
        char Display1[16];
        char Display2[16];
        char Display3[16];

        if (message.Time != time)
        {
            ::_snprintf_s(Display1, _countof(Display1), "%8u ticks",  message.Time);

            ::_snprintf_s(Display2, _countof(Display2), "%8.2f s", (double) messageTimeInMS / 1000.);

            const uint32_t t = messageTimeInMS / 1000;

            ::_snprintf_s(Display3, _countof(Display3), "%02d:%02d:%02d", t / 3600, (t % 3600) / 60, t % 60);
        }
        else
            Display1[0] = Display2[0] = Display3[0] = '\0';

        ::printf("%8d %-14s %-10s %-8s ", (int) index, Display1, Display2, Display3);
    }

    // MIDI Event
    if (!message.IsSysEx())
    {
        uint8_t Event[3]
        {
            (uint8_t) (message.Data),
            (uint8_t) (message.Data >> 8),
            (uint8_t) (message.Data >> 16)
        };

        const uint8_t StatusCode = (const uint8_t) (Event[0] & 0xF0u);

        const uint32_t EventSize = (uint32_t) ((StatusCode >= midi::TimingClock   && StatusCode <= midi::MetaData) ? 1 :
                                              ((StatusCode == midi::ProgramChange || StatusCode == midi::ChannelPressure) ? 2 : 3));

        uint8_t PortNumber = (message.Data >> 24) & 0x7F;

        ::printf("%02X", Event[0]);

        if (EventSize > 1)
        {
            ::printf(" %02X", (int) Event[1]);

            if (EventSize > 2)
                ::printf(" %02X", (int) Event[2]);
            else
                ::printf("   ");
        }
        else
            ::printf("      ");

        int Channel = (Event[0] & 0x0F) + 1;

        ::printf(" Port %d, Channel %2d, ", PortNumber, Channel);

        switch (StatusCode)
        {
            case midi::NoteOff:
                ::printf("Note Off                        %3d, Velocity %3d\n", Event[1], Event[2]);
                break;

            case midi::NoteOn:
                ::printf("Note On                         %3d, Velocity %3d\n", Event[1], Event[2]);
                break;

            case midi::KeyPressure:
                ::printf("Key Pressure (Aftertouch)       %3d\n", Event[1]);
                break;

            case midi::ControlChange:
            {
                ::printf("Control Change                  %3d %3d \"%s\"\n", Event[1], Event[2], DescribeControlChange(Event[1], Event[2]).c_str());

                if (Event[1] == 98)     // Non-Registered Parameter LSB
                    CCLSB = Event[2];
                else
                if (Event[1] == 99)     // Non-Registered Parameter MSB
                    CCMSB = Event[2];
                else
                if (Event[1] == 100)    // Registered Parameter LSB
                    CCLSB = Event[2];
                else
                if (Event[1] == 101)    // Registered Parameter MSB
                    CCMSB = Event[2];
                else
                if (Event[1] == 38)     // LSB for CC 0 - 31.
                    DELSB = Event[2];

                if ((CCLSB != 255) && (CCMSB != 255))
                {
                    int RPN = (CCMSB << 7) | CCLSB; // Registered Parameter Number

                    if ((Event[1] == 6) || (Event[1] == 96) || (Event[1] == 97))
                    {
                        int Value = (Event[2] << 7) | DELSB;

                        if (Event[1] == 6)      // Data Entry
                            ::printf("Set RPN 0x%04X to %3d\n", RPN, Value);
                        else
                        if (Event[1] == 96)
                            ::printf("Increment RPN 0x%04X by %3d\n", RPN, Value);
                        else
                        if (Event[1] == 97)
                            ::printf("Decrement RPN 0x%04X by %3d\n", RPN, Value);

                        CCMSB = CCLSB = 255;
                        DELSB = 0;
                    }
                }
                break;
            }

            case midi::ProgramChange:
                ::printf("Program Change                  %3d     \"%s\"\n", Event[1], Instruments[Event[1]]);;
                break;

            case midi::ChannelPressure:
                ::printf("Channel Pressure (Aftertouch)   %3d\n", Event[1]);
                break;

            case midi::PitchBendChange:
                ::printf("Pitch Bend Change             %5d\n", 8192 - ((int) (Event[2] & 0x7F) << 7) | (Event[1] & 0x7F));
                break;

            case midi::SysEx:
                ::puts("SysEx");
                break;

            case midi::MIDITimeCodeQtrFrame:
                ::puts("MIDI Time Code Qtr Frame");
                break;

            case midi::SongPositionPointer:
                ::puts("Song Position Pointer");
                break;

            case midi::SongSelect:
                ::puts("Song Select");
                break;

            case midi::TuneRequest:
                ::puts("Tune Request");
                break;

            case midi::SysExEnd:
                ::puts("SysEx End");
                break;

            case midi::TimingClock:
                ::puts("Timing Clock");
                break;

            case midi::Start:
                ::puts("Start");
                break;

            case midi::Continue:
                ::puts("Continue");
                break;

            case midi::Stop:
                ::puts("Stop");
                break;

            case midi::ActiveSensing:
                ::puts("Active Sensing");
                break;

            case midi::MetaData:
                ::puts("Meta Data");
                break;

            default:
                ::printf("<Unknown Status Code: 0x%02X>\n", StatusCode);
        }
    }
    // SysEx Index
    else
    {
        const uint32_t Index = message.Data & 0xFFFFFFu;

        const uint8_t * MessageData;
        size_t MessageSize;
        uint8_t Port;

        sysExMap.GetItem(Index, MessageData, MessageSize, Port);

        // Show the message in the output.
        {
            ::printf("SysEx");

            for (size_t j = 0; j < MessageSize; ++j)
                ::printf(" %02X", MessageData[j]);
        }

        ::printf(" Port %d", Port);

        // Identify the SysEx message.
        if (MessageSize > 2)
        {
            sysex_t SysEx(MessageData, MessageSize);

            SysEx.Identify();

            ::printf(", \"%s\", \"%s\", \"%s\", \"%s\"", SysEx.Manufacturer.c_str(), SysEx.Model.c_str(), SysEx.Command.c_str(), SysEx.Description.c_str());
        }

        ::putchar('\n');
    }

    return message.Time;
}
