#include "MIDIProcessor.h"

bool MIDIProcessor::is_gmf(std::vector<uint8_t> const & p_file)
{
    if (p_file.size() < 32) return false;
    if (p_file[0] != 'G' || p_file[1] != 'M' || p_file[2] != 'F' || p_file[3] != 1) return false;
    return true;
}

bool MIDIProcessor::process_gmf(std::vector<uint8_t> const & p_file, MIDIContainer & p_out)
{
    uint8_t buffer[10];

    p_out.Initialize(0, 0xC0);

    uint16_t tempo = (p_file[4] << 8) | p_file[5];
    uint32_t tempo_scaled = tempo * 100000;

    MIDITrack track;

    buffer[0] = 0xFF;
    buffer[1] = 0x51;
    buffer[2] = tempo_scaled >> 16;
    buffer[3] = tempo_scaled >> 8;
    buffer[4] = tempo_scaled;

    track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, buffer, 5));

    buffer[0] = 0xF0;
    buffer[1] = 0x41;
    buffer[2] = 0x10;
    buffer[3] = 0x16;
    buffer[4] = 0x12;
    buffer[5] = 0x7F;
    buffer[6] = 0x00;
    buffer[7] = 0x00;
    buffer[8] = 0x01;
    buffer[9] = 0xF7;

    track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, buffer, 10));

    buffer[0] = 0xFF;
    buffer[1] = 0x2F;

    track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, buffer, 2));

    p_out.AddTrack(track);

    std::vector<uint8_t>::const_iterator it = p_file.begin() + 7;

    return ProcessSMFTrack(it, p_file.end(), p_out, false);
}
