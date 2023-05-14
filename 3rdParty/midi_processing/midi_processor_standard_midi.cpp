#include "midi_processor.h"

bool midi_processor::is_standard_midi(std::vector<uint8_t> const & data)
{
    if (data.size() < 18)
        return false;

    if (data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd')
        return false;

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false;

    if (data[10] == 0 && data[11] == 0)
        return false; // no tracks

    if (data[12] == 0 && data[13] == 0)
        return false; // dtx == 0, will cause division by zero on tempo calculations

    if (data[14] != 'M' || data[15] != 'T' || data[16] != 'r' || data[17] != 'k')
        return false;

    return true;
}

bool midi_processor::process_standard_midi_count(std::vector<uint8_t> const & data, size_t & trackCount)
{
    trackCount = 0;

    if (data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd')
        return false;

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false; /*throw exception_io_data("Bad MIDI header size");*/

    std::vector<uint8_t>::const_iterator it = data.begin() + 8;

    uint16_t Format = (it[0] << 8) | it[1];

    if (Format > 2)
        return false;

    trackCount = (it[2] << 8) | it[3];

    return true;
}

bool midi_processor::process_standard_midi_track(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail, midi_container & container, bool trackNeedsEndMarker)
{
    midi_track Track;

    unsigned Timestamp = 0;
    unsigned char LastEventCode = 0xFF;

    unsigned LastSysExSize = 0;
    unsigned LastSysExTimestamp = 0;

    std::vector<uint8_t> Buffer;

    Buffer.resize(3);

    for (;;)
    {
        if (!trackNeedsEndMarker && (data == tail))
            break;

        int Delta = decode_delta(data, tail);

        if (!trackNeedsEndMarker && (data == tail))
            break;

        if (Delta < 0)
            Delta = -Delta; // "Encountered negative delta: " << delta << "; flipping sign."

        Timestamp += Delta;

        if (data == tail)
            return false;

        unsigned BytesRead = 0;

        unsigned char EventCode = *data++;

        if (EventCode < 0x80)
        {
            if (LastEventCode == 0xFF)
                return false; // "First MIDI track event is short encoded."

            Buffer.resize(3);

            Buffer[BytesRead++] = EventCode;

            EventCode = LastEventCode;
        }

        if (EventCode < 0xF0)
        {
            if (LastSysExSize)
            {
                Track.add_event(midi_event(LastSysExTimestamp, midi_event::extended, 0, &Buffer[0], LastSysExSize));
                LastSysExSize = 0;
            }

            LastEventCode = EventCode;

            if (!trackNeedsEndMarker && ((EventCode & 0xF0) == 0xE0))
                continue;

            if (BytesRead == 0)
            {
                if (data == tail)
                    return false;

                Buffer.resize(3);

                Buffer[0] = *data++;
                BytesRead = 1;
            }

            switch (EventCode & 0xF0)
            {
                case 0xC0:
                case 0xD0:
                    break;

                default:
                    if (data == tail)
                        return false;

                    Buffer[BytesRead++] = *data++;
            }

            Track.add_event(midi_event(Timestamp, (midi_event::event_type) ((EventCode >> 4) - 8), EventCode & 0x0F, &Buffer[0], BytesRead));
        }
        else
        if (EventCode == 0xF0)
        {
            if (LastSysExSize)
            {
                Track.add_event(midi_event(LastSysExTimestamp, midi_event::extended, 0, &Buffer[0], LastSysExSize));
                LastSysExSize = 0;
            }

            int data_count = decode_delta(data, tail);

            if (data_count < 0)
                return false; // Invalid System Exclusive message.

            if (tail - data < data_count)
                return false;

            Buffer.resize(data_count + 1);
            Buffer[0] = 0xF0;

            std::copy(data, data + data_count, Buffer.begin() + 1);
            data += data_count;

            LastSysExSize = data_count + 1;
            LastSysExTimestamp = Timestamp;
        }
        else
        if (EventCode == 0xF7) // SysEx continuation
        {
            if (LastSysExSize == 0)
                return false;

            int data_count = decode_delta(data, tail);

            if (data_count < 0)
                return false;

            if (tail - data < data_count)
                return false;

            Buffer.resize(LastSysExSize + data_count);

            std::copy(data, data + data_count, Buffer.begin() + LastSysExSize);
            data += data_count;

            LastSysExSize += data_count;
        }
        else
        if (EventCode == 0xFF)
        {
            if (LastSysExSize)
            {
                Track.add_event(midi_event(LastSysExTimestamp, midi_event::extended, 0, &Buffer[0], LastSysExSize));
                LastSysExSize = 0;
            }

            if (data == tail)
                return false;

            unsigned char meta_type = *data++;

            int data_count = decode_delta(data, tail);

            if (data_count < 0)
                return false; // Invalid meta message.

            if (tail - data < data_count)
                return false;

            Buffer.resize(data_count + 2);

            Buffer[0] = 0xFF;
            Buffer[1] = meta_type;

            std::copy(data, data + data_count, Buffer.begin() + 2);
            data += data_count;

            Track.add_event(midi_event(Timestamp, midi_event::extended, 0, &Buffer[0], data_count + 2));

            if (meta_type == 0x2F)
            {
                trackNeedsEndMarker = true;
                break;
            }
        }
        else
        if (EventCode >= 0xF8 && EventCode <= 0xFE) //Sequencer specific events, single byte.
        {
            Buffer[0] = EventCode;
            Track.add_event(midi_event(Timestamp, midi_event::extended, 0, &Buffer[0], 1));
        }
        else
            return false; // Unhandled MIDI status code.
    }

    if (!trackNeedsEndMarker)
    {
        Buffer[0] = 0xFF;
        Buffer[1] = 0x2F;

        Track.add_event(midi_event(Timestamp, midi_event::extended, 0, &Buffer[0], 2));
    }

    container.add_track(Track);

    return true;
}

bool midi_processor::process_standard_midi(std::vector<uint8_t> const & data, midi_container & container)
{
    if (data.size() < (4 + 4 + 6))
        return false;

    if (data[0] != 'M' || data[1] != 'T' || data[2] != 'h' || data[3] != 'd')
        return false; // Bad MIDI header chunk type

    if (data[4] != 0 || data[5] != 0 || data[6] != 0 || data[7] != 6)
        return false; // Bad MIDI header size

    std::vector<uint8_t>::const_iterator Data = data.begin() + 8;
    std::vector<uint8_t>::const_iterator Tail = data.end();

    uint16_t Format = (Data[0] << 8) | Data[1];

    if (Format > 2)
        return false; // Bad MIDI format

    std::size_t TrackCount = (Data[2] << 8) | Data[3];
    uint16_t Division = (Data[4] << 8) | Data[5];

    if ((TrackCount == 0) || (Division == 0))
        return false;

    container.initialize(Format, Division);

    Data += 6;

    for (std::size_t i = 0; i < TrackCount; ++i)
    {
        if (Tail - Data < 8)
            return false;

        if (Data[0] != 'M' || Data[1] != 'T' || Data[2] != 'r' || Data[3] != 'k')
            return false;

        uint32_t TrackSize = (Data[4] << 24) | (Data[5] << 16) | (Data[6] << 8) | Data[7];

        Data += 8;

        if ((unsigned long) (Tail - Data) < TrackSize)
            return false;

        intptr_t TrackData = Data - data.begin();

        if (!process_standard_midi_track(Data, Data + TrackSize, container, true))
            return false;

        TrackData += TrackSize;

        if (Data - data.begin() != TrackData)
            Data = data.begin() + TrackData;
    }

    return true;
}
