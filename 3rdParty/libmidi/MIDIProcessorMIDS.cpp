#include "MIDIProcessor.h"

bool MIDIProcessor::IsMIDS(std::vector<uint8_t> const & data)
{
    if (data.size() < 8)
        return false;

    if (data[0] != 'R' || data[1] != 'I' || data[2] != 'F' || data[3] != 'F')
        return false;

    uint32_t Size = data[4] | (data[5] << 8) | (data[6] << 16) | (data[7] << 24);

    if ((Size < 8) || (data.size() < Size + 8))
        return false;

    if (data[8] != 'M' || data[9] != 'I' || data[10] != 'D' || data[11] != 'S' || data[12] != 'f' || data[13] != 'm' || data[14] != 't' || data[15] != ' ')
        return false;

    return true;
}

bool MIDIProcessor::ProcessMIDS(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    if (data.size() < 20)
        return false;

    std::vector<uint8_t>::const_iterator it = data.begin() + 16;
    std::vector<uint8_t>::const_iterator end = data.end();

    uint32_t fmt_size = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);

    it += 4;

    if ((unsigned long) (end - it) < fmt_size)
        return false;

    uint32_t time_format = 1;
    /*uint32_t max_buffer = 0;*/
    uint32_t flags = 0;

    if (fmt_size >= 4)
    {
        time_format = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
        it += 4;
        fmt_size -= 4;
        if (!time_format) // dtx == 0, will cause division by zero on tempo calculations
            return false;
    }

    if (fmt_size >= 4)
    {
        /*max_buffer = it[ 0 ] | ( it[ 1 ] << 8 ) | ( it[ 2 ] << 16 ) | ( it[ 3 ] << 24 );*/
        it += 4;
        fmt_size -= 4;
    }

    if (fmt_size >= 4)
    {
        flags = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
        it += 4;
        fmt_size -= 4;
    }

    it += fmt_size;

    if (it == end)
        return false;

    if (fmt_size & 1)
        ++it;

    container.Initialize(0, time_format);

    if (end - it < 4)
        return false;

    if (it[0] != 'd' || it[1] != 'a' || it[2] != 't' || it[3] != 'a')
        return false; /*throw exception_io_data( "MIDS missing RIFF data chunk" );*/

    it += 4;

    {
        MIDITrack track;

        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));
        container.AddTrack(track);
    }

    if (end - it < 4)
        return false;

    uint32_t data_size = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
    it += 4;

    std::vector<uint8_t>::const_iterator body_end = it + data_size;

    if (body_end - it < 4)
        return false;

    uint32_t segment_count = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
    it += 4;

    bool is_eight_byte = !!(flags & 1);

    MIDITrack track;

    unsigned current_timestamp = 0;

    for (unsigned i = 0; i < segment_count; ++i)
    {
        if (end - it < 12)
            return false;

        it += 4;

        uint32_t segment_size = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);

        it += 4;

        std::vector<uint8_t>::const_iterator segment_end = it + segment_size;

        while (it != segment_end && it != body_end)
        {
            if (segment_end - it < 4)
                return false;

            uint32_t delta = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
            it += 4;
            uint32_t event;
            current_timestamp += delta;
            if (!is_eight_byte)
            {
                if (segment_end - it < 4) return false;
                it += 4;
            }
            if (segment_end - it < 4) return false;
            event = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
            it += 4;
            if (event >> 24 == 0x01)
            {
                uint8_t buffer[5] = { 0xFF, 0x51 };
                buffer[2] = (uint8_t) (event >> 16);
                buffer[3] = (uint8_t) (event >> 8);
                buffer[4] = (uint8_t) event;
                container.AddEventToTrack(0, MIDIEvent(current_timestamp, MIDIEvent::Extended, 0, buffer, sizeof(buffer)));
            }
            else if (!(event >> 24))
            {
                unsigned event_code = (event & 0xF0) >> 4;
                if (event_code >= 0x8 && event_code <= 0xE)
                {
                    unsigned bytes_to_write = 1;
                    uint8_t buffer[2];
                    buffer[0] = (uint8_t) (event >> 8);
                    if (event_code != 0xC && event_code != 0xD)
                    {
                        buffer[1] = (uint8_t) (event >> 16);
                        bytes_to_write = 2;
                    }
                    track.AddEvent(MIDIEvent(current_timestamp, (MIDIEvent::EventType) (event_code - 8), event & 0x0F, buffer, bytes_to_write));
                }
            }
        }
    }

    track.AddEvent(MIDIEvent(current_timestamp, MIDIEvent::Extended, 0, MIDIEventEndOfTrack, _countof(MIDIEventEndOfTrack)));

    container.AddTrack(track);

    return true;
}
