
/** $VER: MIDIProcessorMMD.cpp (2026.05.06) P. Stuer **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "MIDIContainer.h"

#include <MMD.h>

namespace midi
{

/// <summary>
/// Returns true if data points to an MMD sequence.
/// </summary>
bool processor_t::IsMMD(std::vector<uint8_t> const & data, const std::wstring & fileExtension) noexcept
{
    if (fileExtension.empty())
        return false;

    if (::_wcsicmp(fileExtension.c_str(), L"mmd"))
        return false;

    const size_t HeaderSize = 2 + (18 * 4);

    if (data.size() < HeaderSize)
        return false;

    return true;
}

/// <summary>
/// Processes the MMD data.
/// </summary>
bool processor_t::ProcessMMD(std::vector<uint8_t> const & data, const std::wstring & filePath, container_t & container)
{
    mmd::converter_t Converter;

    std::vector<uint8_t> Data;

    if (!Converter.ToSMF(data.data(), (uint32_t) data.size(), Data))
        return false;

    container.FileFormat = FileFormat::MMD;

    return processor_t::Process(Data, filePath.c_str(), container);
}

}
