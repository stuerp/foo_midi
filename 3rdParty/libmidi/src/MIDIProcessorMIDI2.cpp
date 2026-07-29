
/** $VER: MIDIProcessorMIDI2.cpp (2026.05.20) MIDI 2.0 Format (https://midi.org/specs) **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "MIDIContainer.h"

#include "MIDI2.h"

#include "Exception.h"

namespace midi
{
using namespace midi2;

bool processor_t::IsMIDI2(std::vector<uint8_t> const & data) noexcept
{
    // UMP packets are built from 32-bit words so the file size should be a multiple of 4.
    if ((data.size() < 8) || ((data.size() % 4) != 0))
        return false;

    if (::memcmp(data.data(), "SMF2CLIP", 8) != 0)
        return false;

    return true;
}

bool processor_t::ProcessMIDI2(std::vector<uint8_t> const & data, container_t & container)
{
    if ((data.size() < 8) || ((data.size() % 4) != 0))
        throw midi::exception("Insufficient MIDI 2.0 data");

    if (::memcmp(data.data(), "SMF2CLIP", 8) != 0)
        throw midi::exception("MIDI 2.0 signature not found");

    container.FileFormat = FileFormat::UMP;

    const auto Packets = ump_t::FromBytes(data.data() + 8, data.size() - 8);

    container.Initialize(0, 480); // Default 480 Ticks Per Quarter Note

    uint32_t CurrentTick = 0;
    uint8_t CurrentGroup = 0;

    track_t Track;

    std::vector<uint8_t> SysEx;
    uint32_t SysExTick = 0;

    SysEx.reserve(16);

    for (const auto & Packet : Packets)
    {
        switch (Packet.GetMessageType())
        {
            case MessageType::Utility:
            {
                const auto Message = utility_message_t(Packet);

                const uint16_t Status = Message.Status();

                if (Status == 0x03)
                {
                    const uint32_t TimeDivision = Message.Payload();

                    container.SetTimeDivision(TimeDivision);
                }
                else
                if (Status == 0x04)
                {
                    CurrentTick += Message.Payload();
                }
                break;
            }

            case MessageType::System:
                break;

            case MessageType::MIDI1ChannelVoice:
            {
                const auto Message = midi1_channel_voice_message_t(Packet);

                // Convert the MIDI 2.0 group to a MIDI 1.0 Port message.
                if (Message.Group() != CurrentGroup)
                {
                    CurrentGroup = Message.Group();

                    const uint8_t Data[3] = { StatusCode::MetaData, MetaDataType::MIDIPort, CurrentGroup };

                    Track.AddEvent(event_t(CurrentTick, event_t::Extended, 0, Data, _countof(Data)));
                }

                const uint16_t MessageCatagory = Message.MessageCategory();
                const uint16_t ChannelNumber = Message.Channel();

                switch (MessageCatagory)
                {
                    case 0x80: // Appendix D 2.1 Note Off
                    case 0x90: // Appendix D 2.1 Note On
                    case 0xA0: // Appendix D 2.1 Polphonic Key Pressure
                    case 0xB0: // Appendix D 2.1 Control Change
                    case 0xE0: // Appendix D 2.5 Pitch Bend
                    {
                        Track.AddEvent(event_t(CurrentTick, (event_t::event_type_t) ((MessageCatagory >> 4) - 8), ChannelNumber, &Packet.Data[2], 2));
                        break;
                    }

                    case 0xC0: // Appendix D 2.4 Program Change
                    case 0xD0: // Appendix D 2.2 Channel Pressure
                    {
                        Track.AddEvent(event_t(CurrentTick, (event_t::event_type_t) ((MessageCatagory >> 4) - 8), ChannelNumber, &Packet.Data[2], 1));
                        break;
                    }

                    case 0xF0: // Appendix D 2.6 Extended
                        throw midi::exception("MIDI 2.0 status not implemented yet");
                }
                break;
            }

            case MessageType::Data:
            {
                const auto Message = sysex7_message_t(Packet);

                // Convert the MIDI 2.0 group to a MIDI 1.0 Port message.
                if (Message.Group() != CurrentGroup)
                {
                    CurrentGroup = Message.Group();

                    const uint8_t Data[3] = { StatusCode::MetaData, MetaDataType::MIDIPort, CurrentGroup };

                    Track.AddEvent(event_t(CurrentTick, event_t::Extended, 0, Data, _countof(Data)));
                }

                const uint16_t Status = Message.Status();

                if ((Status == 0) || (Status == 1))
                {
                    SysEx.push_back(StatusCode::SysEx);
                }

                const uint8_t Size = Message.Size();

                if (Size != 0)
                {
                    auto d = (uint8_t * const) &Packet.Data[2];

                    for (uint8_t i = 0; i < Size; ++i)
                        SysEx.push_back(*d++);
                }

                if ((Status == 0) || (Status == 3))
                {
                    SysEx.push_back(StatusCode::SysExEnd);

                    Track.AddEvent(event_t(SysExTick, event_t::Extended, 0, SysEx.data(), SysEx.size()));
                }
                break;
            }

            case MessageType::MIDI2ChannelVoice:
            {
                const auto Message = midi2_channel_voice_message_t(Packet);

                // Convert the MIDI 2.0 group to a MIDI 1.0 Port message.
                if (Message.Group() != CurrentGroup)
                {
                    CurrentGroup = Message.Group();

                    const uint8_t Data[3] = { StatusCode::MetaData, MetaDataType::MIDIPort, CurrentGroup };

                    Track.AddEvent(event_t(CurrentTick, event_t::Extended, 0, Data, _countof(Data)));
                }

                const uint16_t MessageCategory = Message.MessageCategory();
                const uint16_t ChannelNumber = Message.Channel();

                uint8_t Data[2] = { };

                switch (MessageCategory)
                {
                    case 0x80: // Appendix D 2.1 Note Off
                    {
                        Data[0] = (uint8_t) ( Packet.Data[2]       & 0x7F); // Note
                        Data[1] = (uint8_t) ((Packet.Data[4] >> 1) & 0x7F); // Velocity

                        Track.AddEvent(event_t(CurrentTick, event_t::NoteOff, ChannelNumber, Data, 2));
                        break;
                    }

                    case 0x90: // Appendix D 2.1 Note On
                    {
                        Data[0] = (uint8_t) ( Packet.Data[2]       & 0x7F); // Note
                        Data[1] = (uint8_t) ((Packet.Data[4] >> 1) & 0x7F); // Velocity

                        if (Data[1] == 0x00)
                            Data[1] = 0x01;

                        Track.AddEvent(event_t(CurrentTick, event_t::NoteOn, ChannelNumber, Data, 2));
                        break;
                    }

                    case 0xA0: // Appendix D 2.1 Polyphonic Key Pressure
                    {
                        Data[0] = (uint8_t) ( Packet.Data[2]       & 0x7F);
                        Data[1] = (uint8_t) ((Packet.Data[4] >> 1) & 0x7F); // Convert 32-bit to 7-bit value.

                        Track.AddEvent(event_t(CurrentTick, event_t::KeyPressure, ChannelNumber, Data, 2));
                        break;
                    }

                    case 0xB0: // Appendix D 2.1 Control Change
                    {
                        Data[0] = (uint8_t) ( Packet.Data[2]       & 0x7F);
                        Data[1] = (uint8_t) ((Packet.Data[4] >> 1) & 0x7F); // Convert 32-bit to 7-bit value.

                        Track.AddEvent(event_t(CurrentTick, event_t::ControlChange, ChannelNumber, Data, 2));
                        break;
                    }

                    case 0xC0: // Appendix D 2.4 Program Change
                    {
                        // Bank Select
                        if ((Packet.Data[3] & 0x01) == 0x01)
                        {
                            Data[0] = Controller::BankSelect;
                            Data[1] = (uint8_t) (Packet.Data[6] & 0x7F);

                            Track.AddEvent(event_t(CurrentTick, event_t::ControlChange, ChannelNumber, Data, 2));

                            Data[0] = Controller::BankSelectLSB;
                            Data[1] = (uint8_t) (Packet.Data[7] & 0x7F);

                            Track.AddEvent(event_t(CurrentTick, event_t::ControlChange, ChannelNumber, Data, 2));
                        }

                        // Program Change
                        Data[0] = (uint8_t) (Packet.Data[4] & 0x7F);

                        Track.AddEvent(event_t(CurrentTick, event_t::ProgramChange, ChannelNumber, Data, 1));
                        break;
                    }

                    case 0xD0: // Appendix D 2.2 Channel Pressure
                    {
                        Data[0] = (uint8_t) ((Packet.Data[4] >> 1) & 0x7F); // Convert 32-bit to 7-bit value.

                        Track.AddEvent(event_t(CurrentTick, event_t::ChannelPressure, ChannelNumber, Data, 1));
                        break;
                    }

                    case 0xE0: // Appendix D 2.5 Pitch Bend
                    {
                        Data[0] = (uint8_t) (((Packet.Data[4] << 6) & 0x40) | ((Packet.Data[5] >> 2) & 0x3F)); // Convert 32-bit to 14-bit value.
                        Data[1] = (uint8_t) (((Packet.Data[4] >> 1) & 0x7F));

                        Track.AddEvent(event_t(CurrentTick, event_t::PitchBendChange, ChannelNumber, Data, 2));
                        break;
                    }

                    case 0xF0: // Appendix D 2.6 Extended
                        throw midi::exception("MIDI 2.0 message type not implemented yet");
                }
                break;
            }

            case MessageType::ExtendedData:
            {
                const auto RawMessage = midi2::message_t(Packet);

                const uint16_t Status = RawMessage.Status();

                if (Status < 4)
                {
                    // System Exclusive (8-bit) Message
                    break; // Cannot be translated to MIDI 1.0.
                }
                else
                    throw midi::exception("MIDI 2.0 message type not implemented yet");

                break;
            }

            case MessageType::FlexData:
            {
                const auto Message = flex_data_message_t(Packet);

                const uint16_t Status = Message.Status();
                const uint8_t StatusBank = Message.StatusBank();

                std::string  Description = "<Unknown>";

                switch ((StatusBank << 8) | Status)
                {
                    case 0x0000: // Set Tempo
                    {
                        const uint32_t Tempo = flex_data_message_t::ToMIDI1Tempo(Message.Tempo());

                        const uint8_t Data[5] = { StatusCode::MetaData, MetaDataType::SetTempo, (uint8_t) (Tempo >> 16),(uint8_t) (Tempo >> 8), (uint8_t) Tempo };

                        Track.AddEvent(event_t(0, event_t::Extended, 0, Data, _countof(Data)));
                        break;
                    }

                    default:
                    {
                        MetaDataType Type = MetaDataType::Text;

                        switch ((StatusBank << 8) | Status)
                        {
                            case 0x0102: Type = MetaDataType::TrackName; break;
                            case 0x0104: Type = MetaDataType::Copyright; break;

                            case 0x0201: Type = MetaDataType::Lyrics; break;
                        }

                        uint8_t Data[15] = { StatusCode::MetaData, Type };

                        auto p = (const char *) &Packet.Data[4];
                        size_t n = 2;

                        for (size_t i = 4, j = 2; (i < 16) && (*p != '\0'); ++i, ++n)
                            Data[j++] = (uint8_t) *p++;

                        Track.AddEvent(event_t(CurrentTick, event_t::Extended, 0, Data, n));
                        break;
                    }
                }
                break;
            }

            case MessageType::Stream:
            {
                auto Message = stream_message_t(Packet);

                const uint16_t Status = Message.Status();

                switch (Status)
                {
                    case 0x20: // 7.1.10 Start of Clip
                        break; // Ignore

                    case 0x21: // 7.1.11 End of Clip
                    {
                        Track.AddEvent(event_t(CurrentTick, event_t::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));
                        break;
                    }
                }
                break;
            }
        }
    }

    container.AddTrack(Track);

    return true;
}

}
