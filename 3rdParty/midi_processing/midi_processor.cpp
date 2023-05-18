#include "midi_processor.h"

const uint8_t midi_processor::end_of_track[2] = { StatusCodes::MetaData, MetaDataTypes::EndOfTrack };
const uint8_t midi_processor::loop_start[11] = { 0xFF, 0x06, 'l', 'o', 'o', 'p', 'S', 't', 'a', 'r', 't' };
const uint8_t midi_processor::loop_end[9] = { 0xFF, 0x06, 'l', 'o', 'o', 'p', 'E', 'n', 'd' };

bool midi_processor::Process(std::vector<uint8_t> const & data, const char * fileExtension, midi_container & container)
{
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

    if (is_mids(data))
        return process_mids(data, container);

    if (is_lds(data, fileExtension))
        return process_lds(data, container);

    if (is_gmf(data))
        return process_gmf(data, container);

    return false;
}

bool midi_processor::ProcessSysEx(std::vector<uint8_t> const & data, midi_container & container)
{
    if (IsSysEx(data))
        return process_syx(data, container);

    return false;
}

bool midi_processor::GetTrackCount(std::vector<uint8_t> const & data, const char * fileExtension, size_t & trackCount)
{
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

    if (is_mids(data))
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

/// <summary>
/// Decode a variable-length quantity.
/// </summary>
int midi_processor::DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail)
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
