
/** $VER: Stream.cpp (2026.05.10) P. Stuer **/

#include "pch.h"

#include "MIDI.h"
#include "MIDIContainer.h"

#include "Messages.h"
#include "SysEx.h"

using namespace midi;

static uint8_t DrumKit = 0;
static bool IsGS = false;

static uint8_t DataEntry_MSB = 255;
static uint8_t DataEntry_LSB = 255;

static uint8_t RPN_MSB = 255;   // RPN Most Significant Byte
static uint8_t RPN_LSB = 255;   // RPN Least Significant Byte

static uint8_t NRPN_MSB = 255;  // RPN Most Significant Byte
static uint8_t NRPN_LSB = 255;  // RPN Least Significant Byte

static uint32_t ProcessMessage(uint32_t currentTime, uint32_t messageNumber, const midi::message_t & message, uint32_t messageTimeInMS, const midi::sysex_table_t & sysExMap) noexcept;

/// <summary>
/// Processes the stream.
/// </summary>
void ProcessStream(const midi::container_t & container, const std::vector<midi::message_t> & stream, const midi::sysex_table_t & sysExMap, const std::vector<uint8_t> & portNumbers, bool skipNormalEvents)
{
/*
    midi::sysex_t Test({ midi::StatusCode::SysEx, 0x41, 0x10, 0x00, 0x48, 0x12, 0x10, 0x00, 0x20, 0x3F, 0x03, 0xFF, midi::StatusCode::SysExEnd });
    Test.Identify();
*/
    const uint32_t SubsongIndex = 0;

    ::printf("%u events, %u unique SysEx messages, %u ports\n", (uint32_t) stream.size(), (uint32_t) sysExMap.Size(), (uint32_t) portNumbers.size());

    uint32_t CurrentTime = std::numeric_limits<uint32_t>::max(); // In ms
    uint32_t MessageNumber = 0;

    for (const auto & m : stream)
    {
        // Skip all normal MIDI events.
        if (skipNormalEvents && !m.IsSysEx())
            continue;

        CurrentTime = ProcessMessage(CurrentTime, MessageNumber++, m, container.TimestampToMS(m.Time, SubsongIndex), sysExMap);
    }
}

/// <summary>
/// Processes MIDI stream events.
/// </summary>
static uint32_t ProcessMessage(uint32_t currentTime, uint32_t messageNumber, const midi::message_t & message, uint32_t messageTimeInMS, const midi::sysex_table_t & sysExMap) noexcept
{
    // Output the message header.
    {
        char Display1[16];
        char Display2[16];

        if (message.Time != currentTime)
        {
            ::_snprintf_s(Display1, _countof(Display1), "%8.3fs",  (double) message.Time / 1000.f);

            const uint32_t t = message.Time / 1000;

            ::_snprintf_s(Display2, _countof(Display2), "%02d:%02d:%02d.%03d", t / 3600, (t % 3600) / 60, t % 60, message.Time % 1000);
        }
        else
            Display1[0] = Display2[0] = '\0';

        ::printf("%8u %-11s %-12s ", messageNumber, Display1, Display2);
    }

    // MIDI Event
    if (!message.IsSysEx())
    {
        const auto StatusCode    = (uint8_t)   message.Data;
        const auto Data1         = (uint8_t)  (message.Data >>  8);
        const auto Data2         = (uint8_t)  (message.Data >> 16);
        const auto PortNumber    = (uint8_t) ((message.Data >> 24) & 0x7F);

        const auto MessageType   = (uint8_t)  (StatusCode & 0xF0);
        const auto ChannelNumber = (uint8_t) ((StatusCode & 0x0F) + 1);

        const auto MessageSize   = (uint32_t) (msc::InRange(MessageType, (uint8_t) midi::TimingClock, (uint8_t) midi::MetaData) ? 1 :
                                              ((MessageType == midi::ProgramChange || MessageType == midi::ChannelPressure) ? 2 : 3));

        ::printf("%02X", StatusCode);

        if (MessageSize > 1)
        {
            ::printf(" %02X", (int) Data1);

            if (MessageSize > 2)
                ::printf(" %02X", (int) Data2);
            else
                ::printf("   ");
        }
        else
            ::printf("      ");

        ::printf(" Port %d, Channel %2d, ", PortNumber, ChannelNumber);

        switch (MessageType)
        {
            case midi::NoteOff:
                ::printf("Note Off                        %3d, Velocity %3d\n", Data1, Data2);
                break;

            case midi::NoteOn:
                ::printf("Note On                         %3d, Velocity %3d%s\n", Data1, Data2, (ChannelNumber == 10) ? msc::FormatText(", \"%s\"", DescribeDrum(DrumKit, Data1, IsGS).c_str()).c_str() : "");
                break;

            case midi::KeyPressure:
                ::printf("Key Pressure (Aftertouch)       %3d\n", Data1);
                break;

            case midi::ControlChange:
            {
                ::printf("Control Change              %3d %3d, %s\n", Data1, Data2, DescribeControlChange(Data1, Data2).c_str());

                switch (Data1)
                {
                    case midi::Controller::DataEntry:
                        DataEntry_MSB = Data2;
                        break;

                    case midi::Controller::DataEntryLSB:    // LSB for CC 0 - 31.
                        DataEntry_LSB = Data2;
                        break;

                    case midi::Controller::NRPNLSB:         // Non-Registered Parameter LSB (NRPN)
                        NRPN_LSB = Data2;
                        break;

                    case midi::Controller::NRPNMSB:         // Non-Registered Parameter MSB (NRPN)
                        NRPN_MSB = Data2;
                        break;

                    case midi::Controller::RPNLSB:          // Registered Parameter LSB (RPN)
                        RPN_LSB = Data2;
                        break;

                    case midi::Controller::RPNMSB:          // Registered Parameter MSB (RPN)
                        RPN_MSB = Data2;
                        break;
                }
                break;
            }

            case midi::ProgramChange:
            {
                std::string ProgramName;

                if (ChannelNumber != 10)
                    ProgramName = Instruments[Data1];
                else
                {
                    ProgramName = DescribeDrumSet(Data1, IsGS);
                    DrumKit = Data1;
                }

                ::printf("Program Change                  %3d, \"%s\"\n", Data1, ProgramName.c_str());
                break;
            }

            case midi::ChannelPressure:
                ::printf("Channel Pressure (Aftertouch)   %3d\n", Data1);
                break;

            case midi::PitchBendChange:
                ::printf("Pitch Bend Change             %5d\n", midi::BytesToPitchBend(Data1, Data2));
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
                ::printf("<Unknown message type: 0x%02X>\n", MessageType);
        }
    }
    // SysEx Index
    else
    {
        const uint32_t Index = message.Data & 0xFFFFFFu;

        const uint8_t * MessageData;

        size_t MessageSize;
        uint8_t PortNumber;

        sysExMap.GetItem(Index, MessageData, MessageSize, PortNumber);

        // Show the message in the output.
        {
            ::printf("         Port %d, SysEx", PortNumber);

            for (size_t j = 0; j < MessageSize; ++j)
                ::printf(" %02X", MessageData[j]);
        }

        // Identify the SysEx message.
        if (MessageSize > 2)
        {
            sysex_t SysEx(MessageData, MessageSize);

            SysEx.Identify();

            ::printf(", \"%s\", \"%s\", \"%s\", \"%s\"", SysEx.Manufacturer.c_str(), SysEx.Model.c_str(), SysEx.Command.c_str(), SysEx.Description.c_str());
        }

        if (!IsGS && sysex_t::IsGSSystemOn(MessageData, MessageSize))
            IsGS = true;

        ::putchar('\n');
    }

    return message.Time;
}
