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

    if (IsRIFF(data))
        return ProcessRIFF(data, container);

    if (is_hmp(data))
        return process_hmp(data, container);

    if (is_hmi(data))
        return process_hmi(data, container);

    if (is_xmi(data))
        return process_xmi(data, container);

    if (is_mus(data))
        return process_mus(data, container);

    if (IsMIDS(data))
        return ProcessMIDS(data, container);

    if (is_lds(data, fileExtension))
        return process_lds(data, container);

    if (is_gmf(data))
        return process_gmf(data, container);

    return false;
}

/// <summary>
/// Processes a stream of bytes.
/// </summary>
bool MIDIProcessor::ProcessSysEx(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    _ErrorCode = MIDIError::None;

    if (IsSysEx(data))
        return ProcessSysExInternal(data, container);

    return false;
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

    if (is_xmi(data))
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
