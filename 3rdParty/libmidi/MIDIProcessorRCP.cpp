
/** $VER: MIDIProcessorRCP.cpp (2024.05.06) Recomposer Format (http://www.vgmpf.com/Wiki/index.php?title=GMF) **/

#include "MIDIProcessor.h"

#include <windows.h>

bool MIDIProcessor::IsRCP(std::vector<uint8_t> const & data, const char * fileExtension)
{
    if (fileExtension == nullptr)
        return false;

    if (data.size() < 28)
        return false;

    if (::strncmp((const char *) data.data(), "RCM-PC98V2.0(C)COME ON MUSIC", 28) == 0)
    {
        if (::_stricmp(fileExtension, "rcp") == 0)
            return true;

        if (::_stricmp(fileExtension, "r36") == 0)
            return true;

        return false;
    }

    if (data.size() < 31)
        return false;

    if (::strncmp((const char *) data.data(), "COME ON MUSIC RECOMPOSER RCP3.0", 31) == 0)
    {
        if (::_stricmp(fileExtension, "g18") == 0)
            return true;

        if (::_stricmp(fileExtension, "g36") == 0)
            return true;

        return false;
    }

    return false;
}

bool MIDIProcessor::ProcessRCP(std::vector<uint8_t> const & data, MIDIContainer & container)
{
    const size_t HEADER_LENGTH   = 44;
    const size_t EVENT_LENGTH    =  4;

    const size_t RCP_MAXCHANNELS = 32;

    const uint8_t * Data = data.data();

    size_t Offset = 32; // Skip past the file identifier

    {
        char Title[64];

        ::memcpy(Title, Data + Offset, _countof(Title));
        Offset += _countof(Title);
    }

    {
        for (size_t i = 0; i < 12; ++i)
        {
            char Memo[28];

            ::memcpy(Memo, Data + Offset, _countof(Memo));
            Offset += _countof(Memo);
        }
    }

    uint16_t timeBase = (Data[0x01E7] << 8) | Data[0x01C0]; // Divisions

    uint8_t tempo = Data[0x01C1];

    if (tempo < 8 || tempo > 250)
        tempo = 120;

    uint8_t beatN = Data[0x01C2]; // Time signature Nominator
    uint8_t beatD = Data[0x01C3]; // Time signature Denominator

    uint8_t key = Data[0x01C4]; // Key signature

    if (key < 0 || key >= 32)
        key = 0;

    int playBias = (int8_t) Data[0x01C5];

    if (playBias < -36 || playBias > 36)
        playBias = 0;

    char fileNameCM6[MAX_PATH]; ::memcpy(fileNameCM6, Data + 0x01C6, 12);
    // Reserved (4 bytes)
    char fileNameGSD[MAX_PATH]; ::memcpy(fileNameGSD, Data + 0x01D6, 12);
    // Reserved (4 bytes)

    int trackNum = (int) Data[0x01E6];

    if (trackNum == -1)
        return false;

    if (trackNum != 18 && trackNum != 36)
        trackNum = 18;

    bool isF = (trackNum != 0);

    uint8_t maxTracks = (trackNum == 0) ? 18 : trackNum;

    {
        Offset = 0x0406;

        for (size_t i = 0; i < 8; ++i)
        {
            uint8_t userSysExsMemo[24];

            ::memcpy(userSysExsMemo, Data + Offset, 24);
            Offset += 24;

            uint8_t userSysExsData[24];

            ::memcpy(userSysExsData, Data + Offset, 24);
            Offset += 24;
        };
    }

    bool gfmt = false; // RCP or R36
    size_t cmdlen = gfmt ? 6 : EVENT_LENGTH;

    container.Initialize((uint32_t) 1, (uint32_t) timeBase);

    // Read the tracks
    WCHAR Text[256];

    Offset = 0x0586;

    for (size_t i = 0; (i < maxTracks) && (Offset + HEADER_LENGTH < data.size()); ++i)
    {
        MIDITrack Track;

        uint32_t Timestamp = 0;

        {
            Data = data.data() + Offset;

            // Terminate the loop if the footer data is found.
            if ((Data[0] == 'R') && (Data[1] == 'C') && (Data[2] == 'F') && (Data[3] == 'W'))
                break;

            // Track header
            size_t Size = ((size_t) Data[1] << 8) | Data[0];

            if (Size < HEADER_LENGTH || Offset + Size > data.size())
                break; // Invalid track size;

            ::swprintf_s(Text, _countof(Text), L"%08X: Track %2d/%2d, %5d bytes\n", Offset, i + 1, maxTracks, (int) Size);
            ::OutputDebugStringW(Text);

            uint8_t trackNo = Data[2]; // Track Number
    //      Data[3] // Rhythm

            int8_t midiCh = Data[4];

            if (midiCh == -1) // Don't play
            {
                Offset += Size;
                continue;
            }

            if (midiCh >= RCP_MAXCHANNELS)
            {
                // Warning: Invalid channel
            }

            int8_t keyShift = Data[5]; // Key Offset

            if (keyShift > 64)
                keyShift -= 128;

            keyShift += playBias;

            int8_t stShiftS  = Data[6];
            uint8_t stShiftU = Data[6];

            int time_offset = stShiftS;

            if (time_offset < -99 || 99 < time_offset)
            {
                // RCP: Invalid time offset
                if (time_offset < -99)
                    time_offset = -99;
                else
                    time_offset = 99;
            }

            uint8_t mode = Data[7];

            if (mode == 0x01) // Muted channel
            {
                Offset += Size;
                continue;
            }

            char memo[37]; ::memcpy(memo, Data + 8, 36); memo[36] = '\0';

            for (size_t j = HEADER_LENGTH; j < Size; j += EVENT_LENGTH)
            {
                ::swprintf_s(Text, _countof(Text), L"%08X: %02X %02X %02X %02X\n", j, Data[j], Data[j + 1], Data[j + 2], Data[j + 3]);
                ::OutputDebugStringW(Text);
            }

            Offset += Size;
        }

        {
            const uint8_t EventData[] = { StatusCodes::MetaData, MetaDataTypes::EndOfTrack };

            Track.AddEvent(MIDIEvent(Timestamp, MIDIEvent::Extended, 0, EventData, _countof(EventData)));

            container.AddTrack(Track);
        }
    }

    return true;
}
