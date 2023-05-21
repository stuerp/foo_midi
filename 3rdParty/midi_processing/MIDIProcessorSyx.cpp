#include "MIDIProcessor.h"

bool MIDIProcessor::IsSysEx(std::vector<uint8_t> const & data)
{
    if (data.size() < 2)
        return false;

    if (data[0] != StatusCodes::SysEx || data[data.size() - 1] != StatusCodes::SysExEnd)
        return false;

    return true;
}

bool MIDIProcessor::process_syx(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    const size_t Size = data.size();

    size_t Index = 0;

    container.Initialize(0, 1);

    MIDITrack Track;

    while (Index < Size)
    {
        size_t MessageLength = 1;

        if (data[Index] != StatusCodes::SysEx)
            return false;

        while (data[Index + MessageLength++] != StatusCodes::SysExEnd);

        Track.AddEvent(MIDIEvent(0, MIDIEvent::Extended, 0, &data[Index], MessageLength));

        Index += MessageLength;
    }

    container.AddTrack(Track);

    return true;
}
