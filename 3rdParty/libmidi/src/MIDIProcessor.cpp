
/** $VER: MIDIProcessor.cpp (2026.05.01) **/

#include "pch.h"

#include "MIDIProcessor.h"

#include <filesystem>

namespace midi
{

processor_options_t processor_t::_Options;

const uint8_t processor_t::MIDIEventEndOfTrack[2] = { StatusCode::MetaData, MetaDataType::EndOfTrack };
const uint8_t processor_t::LoopBeginMarker[11]    = { StatusCode::MetaData, MetaDataType::Marker, 'l', 'o', 'o', 'p', 'S', 't', 'a', 'r', 't' };
const uint8_t processor_t::LoopEndMarker[9]       = { StatusCode::MetaData, MetaDataType::Marker, 'l', 'o', 'o', 'p', 'E', 'n', 'd' };

/// <summary>
/// Processes a stream of bytes.
/// </summary>
bool processor_t::Process(std::vector<uint8_t> const & data, const wchar_t * filePath, container_t & container, const processor_options_t & options)
{
    _Options = options;

    std::wstring FileExtension;

    if (filePath != nullptr)
    {
        std::filesystem::path FilePath(filePath);

        FileExtension = FilePath.extension().wstring();

        if (!FileExtension.empty() && FileExtension[0] == L'.')
            FileExtension = FileExtension.substr(1);
    }

    if (IsSMF(data))
        return ProcessSMF(data, container);

    // .RMI
    if (IsRMI(data))
        return ProcessRMI(data, container);

    // .XMI, .XFM
    if (IsXMI(data))
        return ProcessXMI(data, container);

    if (IsMDS(data))
        return ProcessMDS(data, container);

    if (IsHMP(data))
        return ProcessHMP(data, container);

    if (IsHMI(data))
        return ProcessHMI(data, container);

    if (IsMUS(data))
        return ProcessMUS(data, container);

    if (IsLDS(data, FileExtension))
        return ProcessLDS(data, container);

    if (IsGMF(data))
        return ProcessGMF(data, container);

    if (IsRCP(data, FileExtension))
        return ProcessRCP(data, filePath, container);

    if (IsXMF(data))
        return ProcessXMF(data, container);

    // .MMF
    if (IsMMF(data))
        return ProcessMMF(data, container);

    // .MMD
   if (IsMMD(data, FileExtension))
        return ProcessMMD(data, filePath, container);

#ifdef _DEBUG
    // .TST
    if (IsTST(data, FileExtension))
        return ProcessTST(data, container);
#endif
    if (IsSYX(data))
        return ProcessSYX(data, container);

    return false;
}

/// <summary>
/// Returns true if the data represents a SysEx message.
/// </summary>
bool processor_t::IsSYX(std::vector<uint8_t> const & data) noexcept
{
    if (data.size() < 2)
        return false;

    if (data[0] != StatusCode::SysEx || data[data.size() - 1] != StatusCode::SysExEnd)
        return false;

    return true;
}

/// <summary>
/// Processes a byte stream containing 1 or more SysEx messages.
/// </summary>
bool processor_t::ProcessSYX(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::SYX;

    const size_t Size = data.size();

    size_t Index = 0;

    container.Initialize(0, 1);

    track_t Track;

    while (Index < Size)
    {
        size_t MessageLength = 1;

        if (data[Index] != StatusCode::SysEx)
            return false;

        while (data[Index + MessageLength++] != StatusCode::SysExEnd);

        Track.AddEvent(event_t(0, event_t::Extended, 0, &data[Index], MessageLength));

        Index += MessageLength;
    }

    container.AddTrack(Track);

    return true;
}

/// <summary>
/// Decodes a variable-length quantity.
/// </summary>
int processor_t::DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator & data, std::vector<uint8_t>::const_iterator tail) noexcept
{
    int Quantity = 0;

    uint8_t Byte;

    do
    {
        if (data == tail)
            return 0;

        Byte = *data++;
        Quantity = (Quantity << 7) + (Byte & 0x7F);
    }
    while (Byte & 0x80);

    return Quantity;
}

}
