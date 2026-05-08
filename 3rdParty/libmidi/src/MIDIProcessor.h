
/** $VER: MIDIProcessor.h (2026.05.03) **/

#pragma once

#include "pch.h"

#include "MIDIContainer.h"
#include "IFF.h"

#include <string>

namespace midi
{

struct processor_options_t
{
    // RCP
    uint16_t _LoopExpansion = 0;
    bool _WriteBarMarkers = false;
    bool _WriteSysExNames = false;
    bool _ExtendLoops = true;
    bool _WolfteamLoopMode = false;
    bool _KeepMutedChannels = false;
    bool _IncludeControlData = true;

    // HMI / HMP
    uint16_t _DefaultTempo = 160; // in bpm

    // SMF
    bool _IsEndOfTrackRequired = true;
    bool _DetectExtraPercussionChannel = true;
};

const processor_options_t DefaultOptions(0, false, false, true, false, false, true, 160, true, true);

class processor_t
{
public:
    static bool Process(std::vector<uint8_t> const & data, const wchar_t * filePath, container_t & container, const processor_options_t & options = DefaultOptions);

    static int Inflate(const std::vector<uint8_t> & src, std::vector<uint8_t> & dst) noexcept;
    static int InflateRaw(const std::vector<uint8_t> & src, std::vector<uint8_t> & dst) noexcept;

private:
    static bool IsSMF(std::vector<uint8_t> const & data) noexcept;
    static bool IsRMI(std::vector<uint8_t> const & data) noexcept;
    static bool IsHMP(std::vector<uint8_t> const & data) noexcept;
    static bool IsHMI(std::vector<uint8_t> const & data) noexcept;
    static bool IsXMI(std::vector<uint8_t> const & data) noexcept;
    static bool IsMUS(std::vector<uint8_t> const & data) noexcept;
    static bool IsMDS(std::vector<uint8_t> const & data) noexcept;
    static bool IsLDS(std::vector<uint8_t> const & data, const std::wstring & fileExtension) noexcept;
    static bool IsGMF(std::vector<uint8_t> const & data) noexcept;
    static bool IsRCP(std::vector<uint8_t> const & data, const std::wstring & fileExtension) noexcept;
    static bool IsXMF(std::vector<uint8_t> const & data) noexcept;
    static bool IsMMF(std::vector<uint8_t> const & data) noexcept;
    static bool IsMMD(std::vector<uint8_t> const & data, const std::wstring & fileExtension) noexcept;
#ifdef _DEBUG
    static bool IsTST(std::vector<uint8_t> const & data, const std::wstring & fileExtension) noexcept;
#endif
    static bool IsSYX(std::vector<uint8_t> const & data) noexcept;

    static bool ProcessSMF(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessRMI(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessHMP(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessHMI(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessXMI(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessMUS(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessMDS(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessLDS(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessGMF(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessRCP(std::vector<uint8_t> const & data, const std::wstring & filePath, container_t & container);
    static bool ProcessXMF(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessMMF(std::vector<uint8_t> const & data, container_t & container);
    static bool ProcessMMD(std::vector<uint8_t> const & data, const std::wstring & filePath, container_t & container);
#ifdef _DEBUG
    static bool ProcessTST(std::vector<uint8_t> const & data, container_t & container);
#endif
    static bool ProcessSYX(std::vector<uint8_t> const & data, container_t & container);

    static bool ProcessSMFTrack(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, container_t & container);
    static int DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) noexcept;

    static uint32_t DecodeVariableLengthQuantityHMP(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) noexcept;

    static bool ReadStream(std::vector<uint8_t> const & data, iff_stream_t & stream);
    static bool ReadChunk(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, iff_chunk_t & chunk, bool isFirstChunk);
    static uint32_t DecodeVariableLengthQuantityXMI(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end) noexcept;

    static bool ProcessNode(std::vector<uint8_t>::const_iterator & head, std::vector<uint8_t>::const_iterator tail, std::vector<uint8_t>::const_iterator & data, metadata_table_t & metaData, container_t & container);

private:
    static const uint8_t MIDIEventEndOfTrack[2];
    static const uint8_t LoopBeginMarker[11];
    static const uint8_t LoopEndMarker[9];

    static const uint8_t DefaultTempoXMI[5];

    static const uint8_t DefaultTempoLDS[5];

    static processor_options_t _Options;
};

}
