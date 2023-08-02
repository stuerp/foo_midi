#include "MIDIProcessor.h"

MIDIError MIDIProcessor::_ErrorCode;

const uint8_t MIDIProcessor::MIDIEventEndOfTrack[2] = { StatusCodes::MetaData, MetaDataTypes::EndOfTrack };
const uint8_t MIDIProcessor::LoopBeginMarker[11]    = { StatusCodes::MetaData, MetaDataTypes::Marker, 'l', 'o', 'o', 'p', 'S', 't', 'a', 'r', 't' };
const uint8_t MIDIProcessor::LoopEndMarker[9]       = { StatusCodes::MetaData, MetaDataTypes::Marker, 'l', 'o', 'o', 'p', 'E', 'n', 'd' };

/// <summary>
/// Processes a stream of bytes.
/// </summary>
bool MIDIProcessor::Process(std::vector<uint8_t> const & data, const char * fileExtension, MIDIContainer & container)
{
    _ErrorCode = MIDIError::None;

    if (IsSMF(data))
        return ProcessSMF(data, container);

    // .RMI
    if (IsRIFF(data))
        return ProcessRIFF(data, container);

    // .XMI, .XFM
    if (IsXMI(data))
        return ProcessXMI(data, container);

    if (IsMIDS(data))
        return ProcessMIDS(data, container);

    if (is_hmp(data))
        return process_hmp(data, container);

    if (is_hmi(data))
        return process_hmi(data, container);

    if (is_mus(data))
        return process_mus(data, container);

    if (is_lds(data, fileExtension))
        return process_lds(data, container);

    if (is_gmf(data))
        return process_gmf(data, container);

    if (IsSysEx(data))
        return ProcessSysEx(data, container);

    return false;
}

/// <summary>
/// Returns true if the data represents a SysEx message.
/// </summary>
bool MIDIProcessor::IsSysEx(std::vector<uint8_t> const & data)
{
    if (data.size() < 2)
        return false;

    if (data[0] != StatusCodes::SysEx || data[data.size() - 1] != StatusCodes::SysExEnd)
        return false;

    return true;
}

/// <summary>
/// Processes a byte stream containing 1 or more SysEx messages.
/// </summary>
bool MIDIProcessor::ProcessSysEx(std::vector<uint8_t> const & data, MIDIContainer & container)
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

/// <summary>
/// Gets the number of tracks in the MIDI stream.
/// </summary>
/*
bool MIDIProcessor::GetTrackCount(std::vector<uint8_t> const & data, const char * fileExtension, size_t & trackCount)
{
    _ErrorCode = MIDIError::None;

    trackCount = 0;

    if (IsSMF(data))
        return GetTrackCount(data, trackCount);

    if (IsRIFF(data))
        return GetTrackCountFromRIFF(data, trackCount);

    if (is_hmp(data))
    {
        trackCount = 1;
        return true;
    }

    if (is_hmi(data))
    {
        trackCount = 1;
        return true;
    }

    if (IsXMI(data))
        return GetTrackCountFromXMI(data, trackCount);

    if (is_mus(data))
    {
        trackCount = 1;
        return true;
    }

    if (IsMIDS(data))
    {
        trackCount = 1;
        return true;
    }

    if (is_lds(data, fileExtension))
    {
        trackCount = 1;
        return true;
    }

    if (is_gmf(data))
    {
        trackCount = 1;
        return true;
    }

    return false;
}
*/
/// <summary>
/// Decodes a variable-length quantity.
/// </summary>

int MIDIProcessor::DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail)
{
    int Quantity = 0;

    uint8_t byte;

    do
    {
        if (data == tail)
            return 0;

        byte = *data++;
        Quantity = (Quantity << 7) + (byte & 0x7F);
    }
    while (byte & 0x80);

    return Quantity;
}
