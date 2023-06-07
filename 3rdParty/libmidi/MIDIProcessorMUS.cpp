#include "MIDIProcessor.h"

const uint8_t MIDIProcessor::mus_default_tempo[5] = { 0xFF, 0x51, 0x09, 0xA3, 0x1A };

const uint8_t MIDIProcessor::mus_controllers[15] = { 0,0,1,7,10,11,91,93,64,67,120,123,126,127,121 };

bool MIDIProcessor::is_mus(std::vector<uint8_t> const & p_file)
{
    if (p_file.size() < 0x20) return false;
    if (p_file[0] != 'M' || p_file[1] != 'U' || p_file[2] != 'S' || p_file[3] != 0x1A) return false;
    uint16_t length = p_file[4] | (p_file[5] << 8);
    uint16_t offset = p_file[6] | (p_file[7] << 8);
    uint16_t instrument_count = p_file[12] | (p_file[13] << 8);
    if (offset >= 16 + instrument_count * 2 && offset < 16 + instrument_count * 4 && (size_t) (offset + length) <= p_file.size()) return true;
    return false;
}

bool MIDIProcessor::process_mus(std::vector<uint8_t> const & p_file, MIDIContainer & p_out)
{
    uint16_t length = p_file[4] | (p_file[5] << 8);
    uint16_t offset = p_file[6] | (p_file[7] << 8);

    p_out.Initialize(0, 0x59);

    {
        MIDITrack track;
        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, mus_default_tempo, _countof(mus_default_tempo)));
        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));
        p_out.AddTrack(track);
    }

    MIDITrack track;

    unsigned current_timestamp = 0;

    uint8_t velocity_levels[16] = { 0 };

    if ((size_t) offset >= p_file.size() || (size_t) (offset + length) > p_file.size())
        return false;

    std::vector<uint8_t>::const_iterator it = p_file.begin() + offset, end = p_file.begin() + offset + length;

    uint8_t buffer[4];

    while (it != end)
    {
        buffer[0] = *it++;
        if (buffer[0] == 0x60) break;

        MIDIEvent::EventType type;

        unsigned bytes_to_write;

        unsigned channel = buffer[0] & 0x0F;
        if (channel == 0x0F) channel = 9;
        else if (channel >= 9) ++channel;

        switch (buffer[0] & 0x70)
        {
            case 0x00:
                type = MIDIEvent::NoteOn;
                if (it == end) return false;
                buffer[1] = *it++;
                buffer[2] = 0;
                bytes_to_write = 2;
                break;

            case 0x10:
                type = MIDIEvent::NoteOn;
                if (it == end) return false;
                buffer[1] = *it++;
                if (buffer[1] & 0x80)
                {
                    if (it == end) return false;
                    buffer[2] = *it++;
                    velocity_levels[channel] = buffer[2];
                    buffer[1] &= 0x7F;
                }
                else
                {
                    buffer[2] = velocity_levels[channel];
                }
                bytes_to_write = 2;
                break;

            case 0x20:
                type = MIDIEvent::PitchWheel;
                if (it == end) return false;
                buffer[1] = *it++;
                buffer[2] = buffer[1] >> 1;
                buffer[1] <<= 7;
                bytes_to_write = 2;
                break;

            case 0x30:
                type = MIDIEvent::ControlChange;
                if (it == end) return false;
                buffer[1] = *it++;
                if (buffer[1] >= 10 && buffer[1] <= 14)
                {
                    buffer[1] = mus_controllers[buffer[1]];
                    buffer[2] = 1;
                    bytes_to_write = 2;
                }
                else return false; /*throw exception_io_data( "Unhandled MUS system event" );*/
                break;

            case 0x40:
                if (it == end) return false;
                buffer[1] = *it++;
                if (buffer[1])
                {
                    if (buffer[1] < 10)
                    {
                        type = MIDIEvent::ControlChange;
                        buffer[1] = mus_controllers[buffer[1]];
                        if (it == end) return false;
                        buffer[2] = *it++;
                        bytes_to_write = 2;
                    }
                    else return false; /*throw exception_io_data( "Invalid MUS controller change event" );*/
                }
                else
                {
                    type = MIDIEvent::ProgramChange;
                    if (it == end) return false;
                    buffer[1] = *it++;
                    bytes_to_write = 1;
                }
                break;

            default:
                return false; /*throw exception_io_data( "Invalid MUS status code" );*/
        }

        track.AddEvent(MIDIEvent(current_timestamp, type, channel, buffer + 1, bytes_to_write));

        if (buffer[0] & 0x80)
        {
            int delta = DecodeVariableLengthQuantity(it, end);
            if (delta < 0) return false; /*throw exception_io_data( "Invalid MUS delta" );*/
            current_timestamp += delta;
        }
    }

    track.AddEvent(MIDIEvent(current_timestamp, MIDIEvent::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

    p_out.AddTrack(track);

    return true;
}
