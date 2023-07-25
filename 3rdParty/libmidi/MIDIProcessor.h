#pragma once

#include "MIDIContainer.h"

enum MIDIError
{
    None = 0,

    UnknownStatusCode,                  // Unknown MIDI status code

    InsufficientData,                   // Insufficient data in the stream

    InvalidSysExMessage,                // Invalid System Exclusive message
    InvalidSysExMessageContinuation,    // Invalid System Exclusive message
    InvalidSysExEndMessage,             // Invalid System Exclusive End message

    InvalidMetaDataMessage,             // Invalid Meta Data message

    // SMF
    SMFBadHeaderChunkType,              // Bad SMF header chunk type
    SMFBadHeaderChunkSize,              // Bad SMF header chunk size
    SMFBadHeaderFormat,                 // Bad SMF header format
    SMFBadHeaderTrackCount,             // Bad SMF header track count
    SMFBadHeaderTimeDivision,           // Bad SMF header time division

    SMFUnknownChunkType,                // Unknown type specified in chunk

    SMFBadFirstMessage,                 // Bad first message of a track

    // XMI
    XMIFORMXDIRNotFound,                // FORM XDIR chunk not found
    XMICATXMIDNotFound,                 // CAT XMID chunk not found
    XMIFORMXMIDNotFound,                // FORM XMID chunk not found
    XMIEVNTChunkNotFound,               // EVNT chunk not found
    XMIInvalidNoteMessage,              // Invalid note message
};

class MIDIProcessor
{
public:
    static bool Process(std::vector<uint8_t> const & data, const char * fileExtension, MIDIContainer & container);
    static bool ProcessSysEx(std::vector<uint8_t> const & data, MIDIContainer & container);

    static MIDIError GetLastErrorCode() noexcept { return _ErrorCode; }

private:
    static int DecodeVariableLengthQuantity(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end);
    static uint32_t DecodeVariableLengthQuantityHMP(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end);
    static uint32_t DecodeVariableLengthQuantityXMI(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end);

    static bool IsSMF(std::vector<uint8_t> const & data);
    static bool IsRIFF(std::vector<uint8_t> const & data);
    static bool is_hmp(std::vector<uint8_t> const & data);
    static bool is_hmi(std::vector<uint8_t> const & data);
    static bool IsXMI(std::vector<uint8_t> const & data);
    static bool is_mus(std::vector<uint8_t> const & data);
    static bool IsMIDS(std::vector<uint8_t> const & data);
    static bool is_lds(std::vector<uint8_t> const & data, const char * fileExtension);
    static bool is_gmf(std::vector<uint8_t> const & data);
    static bool IsSysEx(std::vector<uint8_t> const & data);

    static bool ProcessSMF(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool ProcessRIFF(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool process_hmp(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool process_hmi(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool ProcessXMI(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool process_mus(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool ProcessMIDS(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool process_lds(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool process_gmf(std::vector<uint8_t> const & data, MIDIContainer & container);
    static bool ProcessSysExInternal(std::vector<uint8_t> const & data, MIDIContainer & container);

//  static bool GetTrackCount(std::vector<uint8_t> const & data, const char * fileExtension, size_t & trackCount);
//  static bool GetTrackCountFromRIFF(std::vector<uint8_t> const & data, size_t & trackCount);
    static bool GetTrackCountFromXMI(std::vector<uint8_t> const & data, size_t & trackCount);

    static bool ProcessSMFTrack(std::vector<uint8_t>::const_iterator & it, std::vector<uint8_t>::const_iterator end, MIDIContainer & container, bool needs_end_marker);

    static bool SetLastErrorCode(MIDIError errorCode) noexcept
    {
        _ErrorCode = errorCode;

        return false;
    }

private:
    static MIDIError _ErrorCode;

    static const uint8_t MIDIEventEndOfTrack[2];
    static const uint8_t LoopBeginMarker[11];
    static const uint8_t LoopEndMarker[9];

    static const uint8_t XMIDefaultTempo[5];

    static const uint8_t hmp_default_tempo[5];

    static const uint8_t mus_default_tempo[5];
    static const uint8_t mus_controllers[15];

    static const uint8_t lds_default_tempo[5];
};
