
/** $VER: MIDIProcessor.h (2026.11.21) **/

#pragma once

#include "pch.h"

#include "MIDIContainer.h"
#include "IFF.h"
#include "Exception.h"

#include <string>

namespace midi
{
typedef std::vector<uint8_t> bytes_t;
typedef bytes_t::const_iterator iterator_t;

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
    uint16_t DefaultTempo = 160; // in bpm

    // SMF
    bool _IsEndOfTrackRequired = true;
    bool _DetectExtraPercussionChannel = true;
};

const processor_options_t DefaultOptions(0, false, false, true, false, false, true, 160, true, true);

class processor_t
{
public:
    static bool Process(bytes_t const & data, const wchar_t * filePath, container_t & container, const processor_options_t & options = DefaultOptions);

    static int Inflate(const bytes_t & src, bytes_t & dst) noexcept;
    static int InflateRaw(const bytes_t & src, bytes_t & dst) noexcept;

protected:
    static int32_t DecodeVariableLengthQuantity(iterator_t & it, const iterator_t & end) noexcept;
    static bool DecodeMIDIVariableLengthQuantity(iterator_t & it, const iterator_t & end, uint32_t & value) noexcept;

    static inline bool StartsWith(bytes_t const & data, size_t offset, char const * text, size_t length) noexcept
    {
        return HasBytes(data, offset, length) && (::memcmp(data.data() + offset, text, length) == 0);
    }

    static inline bool HasBytes(bytes_t const & data, size_t offset, size_t count) noexcept
    {
        return (offset <= data.size()) && (count <= data.size() - offset);
    }

    static uint16_t ReadLE16(bytes_t const & data, size_t offset)
    {
        if (!HasBytes(data, offset, 2))
            throw midi::exception("Insufficient data");

        return (uint16_t) ((uint16_t) data[offset] | (uint16_t)(data[offset + 1]) << 8);
    }

    static uint32_t ReadLE32(bytes_t const & data, size_t offset)
    {
        if (!HasBytes(data, offset, 4))
            throw midi::exception("Insufficient data");

        return (uint32_t) data[offset] | ((uint32_t) data[offset + 1] << 8) | ((uint32_t) data[offset + 2] << 16) | ((uint32_t) data[offset + 3] << 24);
    }

    static uint32_t ReadLE32(iterator_t data, iterator_t tail)
    {
        if (tail - data < 4)
            throw midi::exception("Insufficient data");

        return (uint32_t) data[0] | ((uint32_t) data[1] <<  8) | ((uint32_t) data[2] << 16) | ((uint32_t) data[3] << 24);
    }

    static bool IsChannelStatus(uint8_t status) noexcept
    {
        return (status >= StatusCode::NoteOff) && (status < StatusCode::SysEx);
    }

    static void Skip(iterator_t & it, iterator_t tail, size_t size, char const * error)
    {
        if ((size_t) (tail - it) < size)
            throw midi::exception(error);

        it += (bytes_t::difference_type) size;
    }

private:
    static bool IsSMF  (bytes_t const & data) noexcept;
    static bool IsRMI  (bytes_t const & data) noexcept;
    static bool IsHMP  (bytes_t const & data) noexcept;
    static bool IsHMI  (bytes_t const & data) noexcept;
    static bool IsXMI  (bytes_t const & data) noexcept;
    static bool IsMUS  (bytes_t const & data) noexcept;
    static bool IsMDS  (bytes_t const & data) noexcept;
    static bool IsLDS  (bytes_t const & data, const std::wstring & fileExtension) noexcept;
    static bool IsGMF  (bytes_t const & data) noexcept;
    static bool IsRCP  (bytes_t const & data, const std::wstring & fileExtension) noexcept;
    static bool IsXMF  (bytes_t const & data) noexcept;
    static bool IsMMF  (bytes_t const & data) noexcept;
    static bool IsMMD  (bytes_t const & data, const std::wstring & fileExtension) noexcept;
    static bool IsMIDI2(bytes_t const & data) noexcept;
#ifdef _DEBUG
    static bool IsTST  (bytes_t const & data, const std::wstring & fileExtension) noexcept;
#endif
    static bool IsSYX  (bytes_t const & data) noexcept;

    static bool ProcessSMF  (bytes_t const & data, container_t & container);
    static bool ProcessRMI  (bytes_t const & data, container_t & container);
    static bool ProcessHMI  (bytes_t const & data, container_t & container);
    static bool ProcessXMI  (bytes_t const & data, container_t & container);
    static bool ProcessMUS  (bytes_t const & data, container_t & container);
    static bool ProcessMDS  (bytes_t const & data, container_t & container);
    static bool ProcessLDS  (bytes_t const & data, container_t & container);
    static bool ProcessGMF  (bytes_t const & data, container_t & container);
    static bool ProcessRCP  (bytes_t const & data, const std::wstring & filePath, container_t & container);
    static bool ProcessXMF  (bytes_t const & data, container_t & container);
    static bool ProcessMMF  (bytes_t const & data, container_t & container);
    static bool ProcessMMD  (bytes_t const & data, const std::wstring & filePath, container_t & container);
    static bool ProcessMIDI2(bytes_t const & data, container_t & container);
#ifdef _DEBUG
    static bool ProcessTST(bytes_t const & data, container_t & container);
#endif
    static bool ProcessSYX(bytes_t const & data, container_t & container);

    static bool ProcessSMFTrack(iterator_t & it, iterator_t end, container_t & container);

    static bool ReadStream(bytes_t const & data, iff_stream_t & stream);
    static bool ReadChunk(iterator_t & it, iterator_t end, iff_chunk_t & chunk, bool isFirstChunk);
    static uint32_t DecodeVariableLengthQuantityXMI(iterator_t & it, iterator_t end) noexcept;

    static bool ProcessNode(iterator_t & head, iterator_t tail, iterator_t & data, metadata_table_t & metaData, container_t & container);

protected:
    static const uint8_t MIDIEventEndOfTrack[2];
    static const uint8_t LoopBeginMarker[11];
    static const uint8_t LoopEndMarker[9];

    static const uint8_t DefaultTempoXMI[5];

    static const uint8_t DefaultTempoLDS[5];

    static processor_options_t _Options;
};

}
