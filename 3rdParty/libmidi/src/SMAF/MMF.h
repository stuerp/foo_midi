
/** $VER: MMF.h (2025.04.05) MMF/SMAF types and definitions **/

#pragma once

#pragma warning(disable: 4820)  // 'x' bytes padding added after data member

enum SMAFFormat : uint8_t
{
    HandyPhoneStandard,         // Size:  2, Compressed: No

    MobileStandard_Compress,    // Size: 16, Compressed: Yes (Huffman encoding)
    MobileStandard_NoCompress,  // Size: 16, Compressed: No

    SEQU,                       // Size: 32, Compressed: No
};

enum SequenceType : uint8_t
{
    StreamSequence, // Sequence Data is one continuous sequence data. Seek Point and Phrase List are used to refer to meaningful positions in a sequence from the outside.
    SubSequence     // Sequence Data is a continuous representation of multiple phrase data. Phrase List is used to recognize individual phrases from the outside.
};

struct state_t
{
    uint8_t FormatType;
    uint8_t SequenceType;
    uint8_t DurationBase;
    uint8_t GateTimeBase;

    bool    IsMTSU;         // Is it a Setup track?
    uint8_t ChannelOffset;

    std::unordered_map<std::string, std::string> Metadata;
};

enum TrackFormat : uint8_t
{
    UnknownTrackFormat = 0,
    MA1,
    MA2,
    MA3,
    MA5,
    MA7,
    UTA2,
    UTA3,
};

class channel_t
{
public:
    enum ChannelType : uint8_t
    {
        NoCare,
        Melody,
        NoMelody,
        Rhythm
    };

    int Channel; // SMAF channel

    bool _KeyControlStatus;
    bool _VibrationStatus;
    bool _LED;
    ChannelType _Type;
    TrackFormat _TrackFormat;

    channel_t(int channel, bool keyControlstatus, bool vibrationStatus, int type, TrackFormat trackFormat) noexcept
    {
        Channel = channel;

        _KeyControlStatus = keyControlstatus;
        _VibrationStatus  = vibrationStatus;
        _LED              = false;
        _Type             = (ChannelType) type;
        _TrackFormat      = trackFormat;

        _Format = SMAFFormat::HandyPhoneStandard;
    }

    channel_t(int channel, int value) noexcept
    {
        Channel = channel;

        _KeyControlStatus = false;
        _VibrationStatus  = (((value & 0x20) >> 5) != 0);
        _LED              = (((value & 0x10) >> 4) != 0);
        _Type             = (ChannelType) (value & 0x03);

        _Format = SMAFFormat::MobileStandard_NoCompress;
    }

private:
    SMAFFormat _Format; // Internal use
};

#pragma region mmftool

#define VOICE_FM    0
#define VOICE_PCM   1

#pragma pack(1)

struct CHPARAM
{
    BYTE bm;
    BYTE bl;
    BYTE pc;

    // Drum parameters
    BYTE na;    // NoteAssign
    BYTE dk;    // DrumKey

    BYTE lfo;
    BYTE pan;
    BOOL pe;
    BYTE alg;   // Only FM

    // Extended member
    UINT type;
//  BYTE version;

    // PCM
    DWORD fs;
    BOOL rm;
    BYTE wavno;
    DWORD lp;
    DWORD ep;
};

struct OPPARAM
{
    BYTE multi; // 15    FM only
    BYTE dt;    //  7    FM only
    BYTE ar;    // 15
    BYTE dr;    // 15
    BYTE sr;    // 15
    BYTE rr;    // 15
    BYTE sl;    // 15
    BYTE tl;    // 63
    BYTE ksl;   //  3   FM only
    BYTE dam;   //  3
    BYTE dvb;   //  3
    BYTE fb;    //  7   FM only
    BYTE ws;    // 31   FM only

    BOOL xof;
    BOOL sus;
    BOOL ksr;   //      FM only
    BOOL eam;
    BOOL evb;
};

#pragma pack()

bool GetHPSExclusiveFMMessage(const uint8_t * data, CHPARAM * chp, OPPARAM * opp);
size_t SetMA3ExclusiveMessage(uint8_t * data, CHPARAM * chp, OPPARAM * opp);

#pragma endregion
