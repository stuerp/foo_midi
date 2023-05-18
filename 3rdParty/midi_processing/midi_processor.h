#pragma once

#include "midi_container.h"

#ifndef _countof
template <typename T, size_t N>
char(&_ArraySizeHelper(T(&array)[N]))[N];
#define _countof(array) (sizeof( _ArraySizeHelper(array)))
#endif

class midi_processor
{
public:
    static bool Process(std::vector<uint8_t> const & data, const char * fileExtension, midi_container & container);
    static bool ProcessSysEx(std::vector<uint8_t> const & data, midi_container & container);

    static bool GetTrackCount(std::vector<uint8_t> const & data, const char * fileExtension, size_t & trackCount);

private:
    static const uint8_t end_of_track[2];
    static const uint8_t loop_start[11];
    static const uint8_t loop_end[9];

    static const uint8_t hmp_default_tempo[5];

    static const uint8_t xmi_default_tempo[5];

    static const uint8_t mus_default_tempo[5];
    static const uint8_t mus_controllers[15];

    static const uint8_t lds_default_tempo[5];

    static int DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end);
    static unsigned decode_hmp_delta(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end);
    static unsigned decode_xmi_delta(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end);

    static bool IsSMF(std::vector<uint8_t> const & data);
    static bool IsRIFF(std::vector<uint8_t> const & data);
    static bool is_hmp(std::vector<uint8_t> const & data);
    static bool is_hmi(std::vector<uint8_t> const & data);
    static bool is_xmi(std::vector<uint8_t> const & data);
    static bool is_mus(std::vector<uint8_t> const & data);
    static bool is_mids(std::vector<uint8_t> const & data);
    static bool is_lds(std::vector<uint8_t> const & data, const char * fileExtension);
    static bool is_gmf(std::vector<uint8_t> const & data);
    static bool IsSysEx(std::vector<uint8_t> const & data);

    static bool ProcessSMF(std::vector<uint8_t> const & data, midi_container & container);
    static bool ProcessRIFF(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_hmp(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_hmi(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_xmi(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_mus(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_mids(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_lds(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_gmf(std::vector<uint8_t> const & data, midi_container & container);
    static bool process_syx(std::vector<uint8_t> const & data, midi_container & container);

    static bool GetTrackCount(std::vector<uint8_t> const & data, size_t & trackCount);
    static bool GetTrackCountFromRIFF(std::vector<uint8_t> const & data, size_t & trackCount);
    static bool GetTrackCountFromXMI(std::vector<uint8_t> const & data, size_t & trackCount);

    static bool ProcessSMFTrack(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, midi_container & container, bool needs_end_marker);
};
