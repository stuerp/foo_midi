#include "MIDIProcessor.h"

const uint8_t MIDIProcessor::hmp_default_tempo[5] = { 0xFF, 0x51, 0x18, 0x80, 0x00 };

bool MIDIProcessor::is_hmp(std::vector<uint8_t> const & data)
{
    if (data.size() < 8)
        return false;

    if (data[0] != 'H' || data[1] != 'M' || data[2] != 'I' || data[3] != 'M' || data[4] != 'I' || data[5] != 'D' || data[6] != 'I' || (data[7] != 'P' && data[7] != 'R'))
        return false;

    return true;
}

unsigned MIDIProcessor::DecodeVariableLengthQuantityHMP(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end)
{
    uint32_t Quantity = 0;

    uint32_t Shift = 0;
    uint8_t Byte;

    do
    {
        if (it == end)
            return 0;

        Byte = *it++;
        Quantity = Quantity + ((Byte & 0x7F) << Shift);
        Shift += 7;
    }
    while (!(Byte & 0x80));

    return Quantity;
}

bool MIDIProcessor::process_hmp(std::vector<uint8_t> const & p_file, MIDIContainer & p_out)
{
    bool is_funky = p_file[7] == 'R';

    uint8_t track_count_8;
    uint16_t dtx = 0xC0;

    uint32_t offset = is_funky ? 0x1A : 0x30;

    if (offset >= p_file.size())
        return false;

    std::vector<uint8_t>::const_iterator it = p_file.begin() + offset;
    std::vector<uint8_t>::const_iterator end = p_file.end();

    track_count_8 = *it;

    if (is_funky)
    {
        if (p_file.size() <= 0x4D)
            return false;
        dtx = (p_file[0x4C] << 16) | p_file[0x4D];
        if (!dtx) // dtx == 0, will cause division by zero on tempo calculations
            return false;
    }

    p_out.Initialize(1, dtx);

    {
        MIDITrack track;
        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, hmp_default_tempo, _countof(hmp_default_tempo)));
        track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, EndOfTrack, _countof(EndOfTrack)));
        p_out.AddTrack(track);
    }

    uint8_t buffer[4];

    if (it == end) return false;
    buffer[0] = *it++;

    while (it != end)
    {
        if (buffer[0] != 0xFF)
        {
            buffer[0] = *it++;
            continue;
        }
        if (it == end) break;
        buffer[1] = *it++;
        if (buffer[1] != 0x2F)
        {
            buffer[0] = buffer[1];
            continue;
        }
        break;
    }

    offset = is_funky ? 3 : 5;
    if ((unsigned long) (end - it) < offset) return false;
    it += offset;

    unsigned track_count = track_count_8;

    for (unsigned i = 1; i < track_count; ++i)
    {
        uint16_t track_size_16;
        uint32_t track_size_32;

        if (is_funky)
        {
            if (end - it < 4) break;
            track_size_16 = it[0] | (it[1] << 8);
            it += 2;
            track_size_32 = track_size_16 - 4;
            if ((unsigned long) (end - it) < track_size_32 + 2) break;
            it += 2;
        }
        else
        {
            if (end - it < 8) break;
            track_size_32 = it[0] | (it[1] << 8) | (it[2] << 16) | (it[3] << 24);
            it += 4;
            track_size_32 -= 12;
            if ((unsigned long) (end - it) < track_size_32 + 8) break;
            it += 4;
        }

        MIDITrack track;

        unsigned current_timestamp = 0;

        std::vector<uint8_t> _buffer;
        _buffer.resize(3);

        std::vector<uint8_t>::const_iterator track_end = it + track_size_32;

        while (it != track_end)
        {
            unsigned delta = DecodeVariableLengthQuantityHMP(it, track_end);
            current_timestamp += delta;
            if (it == track_end) return false;
            _buffer[0] = *it++;
            if (_buffer[0] == 0xFF)
            {
                if (it == track_end) return false;
                _buffer[1] = *it++;
                int meta_count = DecodeVariableLengthQuantity(it, track_end);
                if (meta_count < 0) return false; /*throw exception_io_data( "Invalid HMP meta message" );*/
                if (track_end - it < meta_count) return false;
                _buffer.resize(meta_count + 2);
                std::copy(it, it + meta_count, _buffer.begin() + 2);
                it += meta_count;
                track.AddEvent(MIDIEvent(current_timestamp, MIDIEvent::Extended, 0, &_buffer[0], meta_count + 2));
                if (_buffer[1] == 0x2F) break;
            }
            else if (_buffer[0] >= 0x80 && _buffer[0] <= 0xEF)
            {
                unsigned bytes_read = 2;
                switch (_buffer[0] & 0xF0)
                {
                    case 0xC0:
                    case 0xD0:
                        bytes_read = 1;
                }
                if ((unsigned long) (track_end - it) < bytes_read) return false;
                std::copy(it, it + bytes_read, _buffer.begin() + 1);
                it += bytes_read;
                track.AddEvent(MIDIEvent(current_timestamp, (MIDIEvent::EventType) ((_buffer[0] >> 4) - 8), _buffer[0] & 0x0F, &_buffer[1], bytes_read));
            }
            else return false; /*throw exception_io_data( "Unexpected status code in HMP track" );*/
        }

        offset = is_funky ? 0 : 4;
        if (end - it < (signed long) offset) return false;
        it = track_end + offset;

        p_out.AddTrack(track);
    }

    return true;
}
