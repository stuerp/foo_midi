
/** $VER: RCPProcessor.cpp (2026.11.25) P. Stuer - Based on Valley Bell's rpc2mid (https://github.com/ValleyBell/MidiConverters). **/

#include "pch.h"

#include "RCPProcessor.h"

#include <RCP.h>

namespace rcp
{

/// <summary>
/// Returns true if data points to an RCP sequence.
/// </summary>
bool processor_t::IsRCP(bytes_t const & data, const std::wstring & fileExtension) noexcept
{
    if (fileExtension.empty())
        return false;

    if (data.size() < 28)
        return false;

    if (::strncmp((const char *) data.data(), "RCM-PC98V2.0(C)COME ON MUSIC", 28) == 0)
    {
        if (::_wcsicmp(fileExtension.c_str(), L"rcp") == 0)
            return true;

        if (::_wcsicmp(fileExtension.c_str(), L"r36") == 0)
            return true;

        return false;
    }

    if (data.size() < 31)
        return false;

    if (::strncmp((const char *) data.data(), "COME ON MUSIC RECOMPOSER RCP3.0", 31) == 0)
    {
        if (::_wcsicmp(fileExtension.c_str(), L"g18") == 0)
            return true;

        if (::_wcsicmp(fileExtension.c_str(), L"g36") == 0)
            return true;

        return false;
    }

    return false;
}

/// <summary>
/// Processes the sequence data.
/// </summary>
bool processor_t::Process(bytes_t const & data, const std::wstring & filePath, container_t & container)
{
    rcp::converter_t RCPConverter;

    RCPConverter.SetFilePath(filePath);

    rcp::options_t & Options = RCPConverter._Options;

    Options.RCPLoopExpansion   = _Options.LoopExpansion;

    Options.WriteBarMarkers    = _Options.WriteBarMarkers;
    Options.WriteSysExNames    = _Options.WriteSysExNames;
    Options.ExpandLoops        = _Options.ExpandLoops;
    Options.WolfteamLoopMode   = _Options.WolfteamLoopMode;
    Options.KeepMutedChannels  = _Options.KeepMutedChannels;
    Options.IncludeControlData = _Options.IncludeControlData;

    rcp::buffer_t DstData;

    {
        rcp::buffer_t SrcData;

        SrcData.Copy(data.data(), data.size());

        RCPConverter.ToSMF(SrcData, DstData);
    }

    bytes_t Data;

    Data.insert(Data.end(), DstData.Data, DstData.Data + DstData.Size);

    bool Result = midi::processor_t::Process(Data, filePath.c_str(), container);

    container.FileFormat = FileFormat::RCP;

    return Result;
}

}
