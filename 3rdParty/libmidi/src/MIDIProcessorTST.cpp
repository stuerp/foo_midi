
/** $VER: MIDIProcessorTST.cpp (2025.07.22) Test File **/

#include "pch.h"

#include "MIDIProcessor.h"
#include "Encoding.h"

namespace midi
{

#ifdef _DEBUG

/// <summary>
/// Returns true if the byte vector contains TST data.
/// </summary>
bool processor_t::IsTST(std::vector<uint8_t> const & data, const std::wstring & fileExtension) noexcept
{
    if (::_wcsicmp(fileExtension.c_str(), L"tst"))
        return false;

    return true;
};

/// <summary>
/// Processes a byte vector with TST data.
/// </summary>
bool processor_t::ProcessTST(std::vector<uint8_t> const & data, container_t & container)
{
    container.FileFormat = FileFormat::TST;

    container.Initialize(1u, 500);

    track_t Track;
/*
    uint8_t Data[2] = { 60, 0x7Fu };

    Track.AddEvent(event_t(      0, event_t::NoteOn, (uint32_t) 0, Data, 2));

    Data[0] = 62;

    Track.AddEvent(event_t(   1000, event_t::NoteOn, (uint32_t) 0, Data, 2));

    Data[0] = 64;

    Track.AddEvent(event_t(   2000, event_t::NoteOn, (uint32_t) 0, Data, 2));

    Data[0] = 60;

    Track.AddEvent(event_t(   3000, event_t::NoteOn, (uint32_t) 0, Data, 2));

    Data[0] = StatusCodes::MetaData;
    Data[1] = MetaDataTypes::EndOfTrack;

    Track.AddEvent(event_t(   4000, event_t::Extended, (uint32_t) 0, Data, 2));
*/
    const uint8_t Channel = 14;

    {
        const uint8_t XGSystemOn[] = { 0xF0, 0x43, 0x00, 0x4C, 0x00, 0x00, 0x7E, 0x00, 0xF7 };

        Track.AddEvent(event_t(0, event_t::Extended, 0, XGSystemOn, _countof(XGSystemOn)));

        const uint8_t Part = Channel;
        const uint8_t Mode = 0x02; // Drum Setup 1

        const uint8_t XGSetDrumChannel[] = { 0xF0, 0x43, 0x10, 0x4C, 0x08, Part, 0x07, Mode, 0xF7 };

        Track.AddEvent(event_t(0, event_t::Extended, 0, XGSetDrumChannel, _countof(XGSetDrumChannel)));
    }

    {
        uint8_t Data[1] = { 0x2Au };

        Track.AddEvent(event_t(      0, event_t::ProgramChange, (uint32_t) Channel, Data, 1));
    }

    {
        uint8_t Data[2] = { 0x3Eu, 0x7Fu };

        Track.AddEvent(event_t(      0, event_t::NoteOn,  (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(     50, event_t::NoteOff, (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(    500, event_t::NoteOn,  (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(    550, event_t::NoteOff, (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(   1000, event_t::NoteOn,  (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(   1050, event_t::NoteOff, (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(   1500, event_t::NoteOn,  (uint32_t) Channel, Data, 2));
        Track.AddEvent(event_t(   1550, event_t::NoteOff, (uint32_t) Channel, Data, 2));

        Data[0] = StatusCode::MetaData;
        Data[1] = MetaDataType::EndOfTrack;

        Track.AddEvent(event_t(   2000, event_t::Extended, (uint32_t) 0, Data, 2));
    }

    container.AddTrack(Track);

    return true;
}

#endif

}
